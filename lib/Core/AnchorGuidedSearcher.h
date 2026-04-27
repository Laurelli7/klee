//===-- AnchorGuidedSearcher.h - Static-map searcher switcher ---*- C++ -*-===//
//
// Switches between per-function searcher instances based on a static
// "anchor map" produced offline by searcher_selector/chunk_planner.py.
//
// Each anchor function in the map gets its own searcher instance of the
// recommended kind. A state's governing searcher is the one belonging to
// the deepest anchor function currently on its call stack. Functions not
// in the map (transparent functions, library code, uclibc) do not change
// the state's bucket — execution flows through them under whichever
// anchor was already governing.
//
// No async, no LLM, no socket. The map is loaded once at construction.
//
//===----------------------------------------------------------------------===//

#ifndef KLEE_ANCHORGUIDEDSEARCHER_H
#define KLEE_ANCHORGUIDEDSEARCHER_H

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

class AnchorGuidedSearcher final : public Searcher {
public:
  AnchorGuidedSearcher(const std::string &mapPath,
                       Executor &executor,
                       RNG &rng,
                       InMemoryExecutionTree *executionTree);
  ~AnchorGuidedSearcher() override;

  ExecutionState &selectState() override;
  void update(ExecutionState *current,
              const std::vector<ExecutionState *> &addedStates,
              const std::vector<ExecutionState *> &removedStates) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;

private:
  static constexpr const char *DEFAULT_BUCKET = "<default>";

  Executor &executor;
  RNG &theRNG;
  InMemoryExecutionTree *executionTree;

  // anchor-function-name -> recommended searcher kind ("dfs", "bfs", ...)
  std::unordered_map<std::string, std::string> anchorKind;

  // per-bucket searcher instance; key is anchor function name or
  // DEFAULT_BUCKET for the catch-all bucket.
  std::unordered_map<std::string, std::unique_ptr<Searcher>> bucketSearcher;

  // states currently governed by each bucket
  std::unordered_map<std::string, std::unordered_set<ExecutionState *>>
      bucketStates;

  // per-state, the bucket key that currently owns it
  std::unordered_map<ExecutionState *, std::string> stateBucket;

  // round-robin schedule across non-empty buckets
  std::vector<std::string> rrOrder;
  unsigned rrIndex = 0;

  // Budget for random-path instances. KLEE's ExecutionTree only supports 3
  // (PtrBitCount = 3 bits of alignment slack). After this is exhausted,
  // requests for random-path are downgraded to random-state.
  unsigned remainingRandomPathSlots = 3;

  // statistics
  unsigned totalSelections = 0;
  unsigned totalMigrations = 0;
  std::unordered_map<std::string, unsigned> bucketSelections;

  // ---------- helpers ----------

  /// Load the JSON map from disk. Aborts KLEE on parse failure.
  void loadMap(const std::string &mapPath);

  /// Lazy-instantiate a searcher of the given kind. Returns a non-owning ptr;
  /// ownership is held in bucketSearcher.
  Searcher *makeSearcherForKind(const std::string &kind);

  /// Walk state's call stack from top to bottom; return the deepest frame's
  /// function name that is in anchorKind. Returns DEFAULT_BUCKET if none.
  std::string findGoverningBucket(ExecutionState *state) const;

  /// Move a state from oldBucket to newBucket, forwarding the
  /// add/remove updates to the underlying searchers.
  void migrate(ExecutionState *state, const std::string &oldBucket,
               const std::string &newBucket);

  /// Refresh the rrOrder vector from the live (non-empty) buckets.
  void refreshLiveBuckets();
};

} // namespace klee

#endif // KLEE_ANCHORGUIDEDSEARCHER_H
