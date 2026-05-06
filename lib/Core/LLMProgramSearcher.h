//===-- LLMProgramSearcher.h - LLM-authored static-map searcher -*- C++ -*-===//
//
// Per-function searcher governed by a static "anchor map" hand-authored
// (offline) by an LLM that read the program's source. Each entry maps a
// function name to one of KLEE's built-in searcher kinds.
//
// Behavior:
//   * One searcher instance per kind that appears in the map (lazy-built).
//   * Each ExecutionState is bucketed by the deepest LLM-mapped function
//     currently on its call stack; if none, it falls into a default bucket.
//   * selectState() round-robins across non-empty buckets so every kind
//     gets exploration time proportional to bucket count.
//
// Independent of AnchorGuidedSearcher (different JSON conventions, simpler
// bookkeeping, no random-path slot accounting — random-path bucket falls
// back to random-state if KLEE's 3-instance limit is hit).
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_LLMPROGRAMSEARCHER_H
#define KLEE_LLMPROGRAMSEARCHER_H

#include "Searcher.h"
#include "ExecutionState.h"
#include "klee/ADT/RNG.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace klee {

class Executor;
class InMemoryExecutionTree;

class LLMProgramSearcher final : public Searcher {
public:
  LLMProgramSearcher(const std::string &mapPath,
                     Executor &executor,
                     RNG &rng,
                     InMemoryExecutionTree *executionTree);
  ~LLMProgramSearcher() override;

  ExecutionState &selectState() override;
  void update(ExecutionState *current,
              const std::vector<ExecutionState *> &addedStates,
              const std::vector<ExecutionState *> &removedStates) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;

private:
  static constexpr const char *FALLBACK_BUCKET = "<llm:fallback>";

  Executor &executor;
  RNG &theRNG;
  InMemoryExecutionTree *executionTree;

  // function name -> recommended searcher kind (e.g. "dfs", "bfs", ...)
  std::unordered_map<std::string, std::string> funcKind;

  // bucket key -> searcher instance (one per *kind*, not per function;
  // sharing a kind across many functions concentrates state in the
  // searcher and lets it form better priority queues / weights).
  std::unordered_map<std::string, std::unique_ptr<Searcher>> kindSearcher;

  // bucket key -> live state set (key is the kind string, or FALLBACK_BUCKET)
  std::unordered_map<std::string, std::unordered_set<ExecutionState *>>
      kindStates;

  // per-state, the bucket key currently owning it
  std::unordered_map<ExecutionState *, std::string> stateBucket;

  // round-robin schedule across non-empty buckets
  std::vector<std::string> rrOrder;
  unsigned rrIndex = 0;

  // remaining random-path slots (KLEE caps at 3 instances).
  unsigned remainingRandomPathSlots = 3;

  // statistics
  unsigned totalSelections = 0;
  unsigned totalMigrations = 0;
  std::unordered_map<std::string, unsigned> bucketSelections;

  void loadMap(const std::string &mapPath);
  Searcher *makeSearcherForKind(const std::string &kind);
  std::string findBucket(ExecutionState *state) const;
  void migrate(ExecutionState *state, const std::string &oldBucket,
               const std::string &newBucket);
  void refreshLiveBuckets();
};

} // namespace klee

#endif // KLEE_LLMPROGRAMSEARCHER_H
