//===-- AnchorGuidedSearcher.cpp ------------------------------------------===//
//
// See AnchorGuidedSearcher.h for the design rationale.
//
//===----------------------------------------------------------------------===//

#include "AnchorGuidedSearcher.h"

#include "Executor.h"
#include "ExecutionTree.h"
#include "klee/Module/KInstruction.h"
#include "klee/Module/KModule.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/IR/Function.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

using namespace klee;

//===----------------------------------------------------------------------===//
// Construction / teardown
//===----------------------------------------------------------------------===//

AnchorGuidedSearcher::AnchorGuidedSearcher(const std::string &mapPath,
                                           Executor &executor,
                                           RNG &rng,
                                           InMemoryExecutionTree *executionTree)
    : executor(executor), theRNG(rng), executionTree(executionTree) {
  loadMap(mapPath);

  // Always have a default bucket ready for states whose stack contains no
  // anchor function (e.g., very early bootstrap states).
  bucketSearcher[DEFAULT_BUCKET].reset(makeSearcherForKind("nurs:covnew"));

  klee_message("AnchorGuidedSearcher: loaded %u anchor functions from %s",
               (unsigned)anchorKind.size(), mapPath.c_str());
}

AnchorGuidedSearcher::~AnchorGuidedSearcher() {
  llvm::errs() << "AnchorGuidedSearcher: " << totalSelections
               << " selections, " << totalMigrations << " migrations across "
               << bucketSelections.size() << " active buckets\n";
}

//===----------------------------------------------------------------------===//
// JSON loading
//===----------------------------------------------------------------------===//

void AnchorGuidedSearcher::loadMap(const std::string &mapPath) {
  auto bufOrErr = llvm::MemoryBuffer::getFile(mapPath);
  if (!bufOrErr) {
    klee_error("AnchorGuidedSearcher: cannot read map file '%s': %s",
               mapPath.c_str(), bufOrErr.getError().message().c_str());
  }

  auto valOrErr = llvm::json::parse((*bufOrErr)->getBuffer());
  if (!valOrErr) {
    klee_error("AnchorGuidedSearcher: invalid JSON in '%s'", mapPath.c_str());
  }

  const llvm::json::Object *root = valOrErr->getAsObject();
  if (!root) {
    klee_error("AnchorGuidedSearcher: top-level JSON must be an object");
  }

  const llvm::json::Object *anchors = root->getObject("anchors");
  if (!anchors) {
    klee_error("AnchorGuidedSearcher: missing 'anchors' object");
  }

  for (const auto &kv : *anchors) {
    const std::string funcName = kv.first.str();
    const llvm::json::Object *info = kv.second.getAsObject();
    if (!info)
      continue;
    auto kind = info->getString("searcher");
    if (!kind)
      continue;
    anchorKind[funcName] = kind->str();
  }
}

//===----------------------------------------------------------------------===//
// Searcher kind dispatch (mirrors UserSearcher::getNewSearcher but for the
// kinds we know we'll receive from chunk_planner)
//===----------------------------------------------------------------------===//

Searcher *AnchorGuidedSearcher::makeSearcherForKind(const std::string &kind) {
  if (kind == "dfs")
    return new DFSSearcher();
  if (kind == "bfs")
    return new BFSSearcher();
  if (kind == "random-state")
    return new RandomSearcher(theRNG);
  if (kind == "random-path") {
    // RandomPathSearcher requires the in-memory execution tree, and KLEE
    // caps the number of distinct RP instances at 3 (3 bits of pointer
    // alignment slack used to tag tree nodes). Beyond that limit, fall
    // back to random-state, which has the same "random sampling" character
    // without the per-instance tree-tagging requirement.
    if (!executionTree) {
      klee_warning("AnchorGuidedSearcher: random-path requested but no "
                   "execution tree available; falling back to random-state");
      return new RandomSearcher(theRNG);
    }
    if (remainingRandomPathSlots == 0) {
      klee_warning_once(this,
                        "AnchorGuidedSearcher: random-path slots exhausted "
                        "(KLEE limit = 3); subsequent random-path anchors "
                        "downgraded to random-state");
      return new RandomSearcher(theRNG);
    }
    --remainingRandomPathSlots;
    return new RandomPathSearcher(executionTree, theRNG);
  }
  if (kind == "nurs:covnew")
    return new WeightedRandomSearcher(WeightedRandomSearcher::CoveringNew,
                                      theRNG);
  if (kind == "nurs:md2u")
    return new WeightedRandomSearcher(WeightedRandomSearcher::MinDistToUncovered,
                                      theRNG);
  if (kind == "nurs:depth")
    return new WeightedRandomSearcher(WeightedRandomSearcher::Depth, theRNG);
  if (kind == "nurs:rp")
    return new WeightedRandomSearcher(WeightedRandomSearcher::RP, theRNG);
  if (kind == "nurs:icnt")
    return new WeightedRandomSearcher(WeightedRandomSearcher::InstCount,
                                      theRNG);
  if (kind == "nurs:cpicnt")
    return new WeightedRandomSearcher(WeightedRandomSearcher::CPInstCount,
                                      theRNG);
  if (kind == "nurs:qc")
    return new WeightedRandomSearcher(WeightedRandomSearcher::QueryCost,
                                      theRNG);
  klee_warning("AnchorGuidedSearcher: unknown kind '%s'; falling back to "
               "nurs:covnew",
               kind.c_str());
  return new WeightedRandomSearcher(WeightedRandomSearcher::CoveringNew,
                                    theRNG);
}

//===----------------------------------------------------------------------===//
// Stack walk: deepest anchor on the call stack governs
//===----------------------------------------------------------------------===//

std::string
AnchorGuidedSearcher::findGoverningBucket(ExecutionState *state) const {
  // state->stack is a std::vector of StackFrames; the BOTTOM of the call
  // stack is index 0 (where the program entered), the TOP is back().
  // "Deepest anchor on the stack" means the nearest-to-the-bottom frame
  // whose function is in anchorKind. So walk from index 0 upward and
  // pick the FIRST anchor encountered? No — that's the oldest anchor.
  //
  // What we want is the most recently entered anchor, since that's the
  // one currently in scope. That's the LAST anchor we'd hit walking
  // top-down. So: walk from back() toward front(), return the first
  // anchor frame's function name.
  for (auto it = state->stack.rbegin(); it != state->stack.rend(); ++it) {
    if (!it->kf || !it->kf->function)
      continue;
    const std::string fname = it->kf->function->getName().str();
    auto found = anchorKind.find(fname);
    if (found != anchorKind.end()) {
      return fname;
    }
  }
  return DEFAULT_BUCKET;
}

//===----------------------------------------------------------------------===//
// Bucket migration
//===----------------------------------------------------------------------===//

void AnchorGuidedSearcher::migrate(ExecutionState *state,
                                   const std::string &oldBucket,
                                   const std::string &newBucket) {
  // Remove from old
  auto &oldSet = bucketStates[oldBucket];
  oldSet.erase(state);
  if (auto *s = bucketSearcher[oldBucket].get()) {
    std::vector<ExecutionState *> rem{state};
    s->update(nullptr, {}, rem);
  }

  // Add to new (lazy-create searcher if first time)
  auto &slot = bucketSearcher[newBucket];
  if (!slot) {
    std::string kind = (newBucket == DEFAULT_BUCKET)
                           ? std::string("nurs:covnew")
                           : anchorKind[newBucket];
    slot.reset(makeSearcherForKind(kind));
  }
  bucketStates[newBucket].insert(state);
  std::vector<ExecutionState *> add{state};
  slot->update(nullptr, add, {});

  stateBucket[state] = newBucket;
  ++totalMigrations;
}

void AnchorGuidedSearcher::refreshLiveBuckets() {
  rrOrder.clear();
  for (auto &kv : bucketStates) {
    if (!kv.second.empty()) {
      rrOrder.push_back(kv.first);
    }
  }
  if (rrIndex >= rrOrder.size())
    rrIndex = 0;
}

//===----------------------------------------------------------------------===//
// Searcher API
//===----------------------------------------------------------------------===//

bool AnchorGuidedSearcher::empty() {
  for (auto &kv : bucketStates) {
    if (!kv.second.empty())
      return false;
  }
  return true;
}

ExecutionState &AnchorGuidedSearcher::selectState() {
  refreshLiveBuckets();
  if (rrOrder.empty()) {
    klee_error("AnchorGuidedSearcher::selectState called with no live buckets");
  }
  // Round-robin across non-empty buckets.
  const std::string &bucket = rrOrder[rrIndex % rrOrder.size()];
  rrIndex = (rrIndex + 1) % rrOrder.size();

  Searcher *s = bucketSearcher[bucket].get();
  ExecutionState &es = s->selectState();
  ++totalSelections;
  ++bucketSelections[bucket];
  return es;
}

void AnchorGuidedSearcher::update(
    ExecutionState *current,
    const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {

  // 1) Removals: drop from whichever bucket owned the state.
  for (auto *st : removedStates) {
    auto it = stateBucket.find(st);
    if (it == stateBucket.end())
      continue;
    const std::string bucket = it->second;
    bucketStates[bucket].erase(st);
    if (auto *s = bucketSearcher[bucket].get()) {
      std::vector<ExecutionState *> rem{st};
      s->update(nullptr, {}, rem);
    }
    stateBucket.erase(it);
  }

  // 2) Additions: classify each, route to the right bucket.
  for (auto *st : addedStates) {
    const std::string bucket = findGoverningBucket(st);
    auto &slot = bucketSearcher[bucket];
    if (!slot) {
      std::string kind = (bucket == DEFAULT_BUCKET)
                             ? std::string("nurs:covnew")
                             : anchorKind[bucket];
      slot.reset(makeSearcherForKind(kind));
    }
    bucketStates[bucket].insert(st);
    std::vector<ExecutionState *> add{st};
    slot->update(nullptr, add, {});
    stateBucket[st] = bucket;
  }

  // 3) The current state may have moved into / out of an anchor.
  //    Recompute its bucket and migrate if changed.
  if (current) {
    auto it = stateBucket.find(current);
    if (it != stateBucket.end()) {
      const std::string oldBucket = it->second;
      const std::string newBucket = findGoverningBucket(current);
      if (newBucket != oldBucket) {
        migrate(current, oldBucket, newBucket);
      } else {
        // Same bucket — forward the update so the searcher can do its
        // own bookkeeping (e.g., RandomPathSearcher tracks the tree).
        if (auto *s = bucketSearcher[oldBucket].get()) {
          s->update(current, {}, {});
        }
      }
    }
  }
}

void AnchorGuidedSearcher::printName(llvm::raw_ostream &os) {
  os << "<AnchorGuidedSearcher: " << anchorKind.size() << " anchor functions, "
     << bucketStates.size() << " active buckets>\n";
}
