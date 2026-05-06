//===-- LLMProgramSearcher.cpp --------------------------------------------===//
//
// See LLMProgramSearcher.h for the design rationale.
//
//===----------------------------------------------------------------------===//

#include "LLMProgramSearcher.h"

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

LLMProgramSearcher::LLMProgramSearcher(const std::string &mapPath,
                                       Executor &executor, RNG &rng,
                                       InMemoryExecutionTree *executionTree)
    : executor(executor), theRNG(rng), executionTree(executionTree) {
  loadMap(mapPath);

  // Pre-create the fallback bucket so very-early states (no anchor on
  // stack yet) have somewhere to land.
  kindSearcher[FALLBACK_BUCKET].reset(makeSearcherForKind("nurs:covnew"));

  klee_message(
      "LLMProgramSearcher: loaded %u function->searcher mappings from %s",
      (unsigned)funcKind.size(), mapPath.c_str());
}

LLMProgramSearcher::~LLMProgramSearcher() {
  llvm::errs() << "LLMProgramSearcher: " << totalSelections << " selections, "
               << totalMigrations << " migrations, "
               << bucketSelections.size() << " active buckets\n";
  for (auto &kv : bucketSelections) {
    llvm::errs() << "  [" << kv.first << "] " << kv.second << " selections\n";
  }
}

//===----------------------------------------------------------------------===//
// JSON loading
//
// Schema (v1):
// {
//   "version": 1,
//   "backend": "llm",
//   "default_searcher": "nurs:covnew",      // optional; default below
//   "anchors": {
//     "<func>": { "searcher": "<kind>", "reason": "..." },
//     ...
//   }
// }
//
// Recognised kinds: dfs, bfs, random-state, random-path, nurs:covnew,
//                   nurs:md2u, nurs:depth, nurs:rp, nurs:icnt,
//                   nurs:cpicnt, nurs:qc.
//===----------------------------------------------------------------------===//

void LLMProgramSearcher::loadMap(const std::string &mapPath) {
  auto bufOrErr = llvm::MemoryBuffer::getFile(mapPath);
  if (!bufOrErr) {
    klee_error("LLMProgramSearcher: cannot read map file '%s': %s",
               mapPath.c_str(), bufOrErr.getError().message().c_str());
  }
  auto valOrErr = llvm::json::parse((*bufOrErr)->getBuffer());
  if (!valOrErr) {
    klee_error("LLMProgramSearcher: invalid JSON in '%s'", mapPath.c_str());
  }
  const llvm::json::Object *root = valOrErr->getAsObject();
  if (!root) {
    klee_error("LLMProgramSearcher: top-level JSON must be an object");
  }
  const llvm::json::Object *anchors = root->getObject("anchors");
  if (!anchors) {
    klee_error("LLMProgramSearcher: missing 'anchors' object");
  }
  for (const auto &kv : *anchors) {
    const std::string fname = kv.first.str();
    const llvm::json::Object *info = kv.second.getAsObject();
    if (!info)
      continue;
    auto kind = info->getString("searcher");
    if (!kind)
      continue;
    funcKind[fname] = kind->str();
  }
}

//===----------------------------------------------------------------------===//
// Searcher kind dispatch
//===----------------------------------------------------------------------===//

Searcher *LLMProgramSearcher::makeSearcherForKind(const std::string &kind) {
  if (kind == "dfs")
    return new DFSSearcher();
  if (kind == "bfs")
    return new BFSSearcher();
  if (kind == "random-state")
    return new RandomSearcher(theRNG);
  if (kind == "random-path") {
    if (!executionTree) {
      klee_warning_once(this, "LLMProgramSearcher: random-path requested but "
                              "no execution tree; using random-state");
      return new RandomSearcher(theRNG);
    }
    if (remainingRandomPathSlots == 0) {
      klee_warning_once(this, "LLMProgramSearcher: random-path slots "
                              "exhausted (KLEE limit = 3); downgrading to "
                              "random-state");
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
    return new WeightedRandomSearcher(WeightedRandomSearcher::InstCount, theRNG);
  if (kind == "nurs:cpicnt")
    return new WeightedRandomSearcher(WeightedRandomSearcher::CPInstCount,
                                      theRNG);
  if (kind == "nurs:qc")
    return new WeightedRandomSearcher(WeightedRandomSearcher::QueryCost, theRNG);

  klee_warning("LLMProgramSearcher: unknown kind '%s'; using nurs:covnew",
               kind.c_str());
  return new WeightedRandomSearcher(WeightedRandomSearcher::CoveringNew,
                                    theRNG);
}

//===----------------------------------------------------------------------===//
// Bucketing: deepest mapped function on the call stack -> kind
//===----------------------------------------------------------------------===//

std::string LLMProgramSearcher::findBucket(ExecutionState *state) const {
  // Walk from the top frame downwards; first mapped function we see is
  // the most recently entered, and that's the one currently in scope.
  for (auto it = state->stack.rbegin(); it != state->stack.rend(); ++it) {
    if (!it->kf || !it->kf->function)
      continue;
    auto found = funcKind.find(it->kf->function->getName().str());
    if (found != funcKind.end())
      return found->second; // bucket key == kind
  }
  return FALLBACK_BUCKET;
}

//===----------------------------------------------------------------------===//
// Bucket transitions
//===----------------------------------------------------------------------===//

void LLMProgramSearcher::migrate(ExecutionState *state,
                                 const std::string &oldBucket,
                                 const std::string &newBucket) {
  kindStates[oldBucket].erase(state);
  if (auto *s = kindSearcher[oldBucket].get()) {
    std::vector<ExecutionState *> rem{state};
    s->update(nullptr, {}, rem);
  }
  auto &slot = kindSearcher[newBucket];
  if (!slot) {
    slot.reset(makeSearcherForKind(
        newBucket == FALLBACK_BUCKET ? std::string("nurs:covnew") : newBucket));
  }
  kindStates[newBucket].insert(state);
  std::vector<ExecutionState *> add{state};
  slot->update(nullptr, add, {});
  stateBucket[state] = newBucket;
  ++totalMigrations;
}

void LLMProgramSearcher::refreshLiveBuckets() {
  rrOrder.clear();
  for (auto &kv : kindStates) {
    if (!kv.second.empty())
      rrOrder.push_back(kv.first);
  }
  if (rrIndex >= rrOrder.size())
    rrIndex = 0;
}

//===----------------------------------------------------------------------===//
// Searcher API
//===----------------------------------------------------------------------===//

bool LLMProgramSearcher::empty() {
  for (auto &kv : kindStates) {
    if (!kv.second.empty())
      return false;
  }
  return true;
}

ExecutionState &LLMProgramSearcher::selectState() {
  refreshLiveBuckets();
  if (rrOrder.empty()) {
    klee_error("LLMProgramSearcher::selectState called with no live buckets");
  }
  const std::string &bucket = rrOrder[rrIndex % rrOrder.size()];
  rrIndex = (rrIndex + 1) % rrOrder.size();

  Searcher *s = kindSearcher[bucket].get();
  ExecutionState &es = s->selectState();
  ++totalSelections;
  ++bucketSelections[bucket];
  return es;
}

void LLMProgramSearcher::update(
    ExecutionState *current,
    const std::vector<ExecutionState *> &addedStates,
    const std::vector<ExecutionState *> &removedStates) {

  // Removals
  for (auto *st : removedStates) {
    auto it = stateBucket.find(st);
    if (it == stateBucket.end())
      continue;
    const std::string bucket = it->second;
    kindStates[bucket].erase(st);
    if (auto *s = kindSearcher[bucket].get()) {
      std::vector<ExecutionState *> rem{st};
      s->update(nullptr, {}, rem);
    }
    stateBucket.erase(it);
  }

  // Additions
  for (auto *st : addedStates) {
    const std::string bucket = findBucket(st);
    auto &slot = kindSearcher[bucket];
    if (!slot) {
      slot.reset(makeSearcherForKind(
          bucket == FALLBACK_BUCKET ? std::string("nurs:covnew") : bucket));
    }
    kindStates[bucket].insert(st);
    std::vector<ExecutionState *> add{st};
    slot->update(nullptr, add, {});
    stateBucket[st] = bucket;
  }

  // Current may have crossed an anchor boundary -> re-bucket
  if (current) {
    auto it = stateBucket.find(current);
    if (it != stateBucket.end()) {
      const std::string oldBucket = it->second;
      const std::string newBucket = findBucket(current);
      if (newBucket != oldBucket) {
        migrate(current, oldBucket, newBucket);
      } else if (auto *s = kindSearcher[oldBucket].get()) {
        s->update(current, {}, {});
      }
    }
  }
}

void LLMProgramSearcher::printName(llvm::raw_ostream &os) {
  os << "<LLMProgramSearcher: " << funcKind.size() << " mapped functions, "
     << kindStates.size() << " active buckets>\n";
}
