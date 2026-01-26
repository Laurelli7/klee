///===-- LLMGuidedSearcher.h - Policy-server guided search ------*- C++ -*-===//
///
/// A searcher that communicates with an external policy server to decide
/// which KLEE searcher to use. The LLM picks from all existing searchers:
/// DFS, BFS, RandomState, RandomPath, NURS variants, etc.
///
/// Architecture:
///   KLEE ──[features]──► Policy Server ──[LLM]──► Searcher Name
///        ◄──[decision]──
///
///===----------------------------------------------------------------------===//

#ifndef KLEE_LLMGUIDEDSEARCHER_H
#define KLEE_LLMGUIDEDSEARCHER_H

#include "Searcher.h"
#include "ExecutionState.h"
#include "klee/ADT/RNG.h"

#include <memory>
#include <vector>
#include <string>
#include <map>

namespace klee {

class Executor;
class InMemoryExecutionTree;

/// Lightweight features sent to policy server
struct StateFeatures {
  // Function info
  std::string functionName;
  std::string functionSignature;
  unsigned stackDepth;
  
  // State-specific info
  unsigned constraintCount;
  unsigned depth;
  unsigned steppedInstructions;
  unsigned instsSinceCovNew;
  bool coveredNew;
  unsigned symbolicVarCount;
  
  // Global statistics
  unsigned activeStates;
  unsigned totalForks;
  unsigned inhibitedForks;
  unsigned coveredInstructions;
  unsigned uncoveredInstructions;
  unsigned coveredBranches;
  unsigned totalInstructions;
  unsigned externalCalls;
  
  // Solver stats
  uint64_t solverTime;
  uint64_t forkTime;
  
  // Distance heuristics
  unsigned minDistToUncovered;
  unsigned minDistToReturn;
  
  std::string toJson() const;
};

/// Client for communicating with the policy server via Unix socket
class PolicyClient {
public:
  explicit PolicyClient(const std::string &socketPath);
  ~PolicyClient();
  
  bool connect();
  void disconnect();
  bool isConnected() const;
  
  /// Send features, receive searcher name
  std::string query(const StateFeatures &features);
  
private:
  std::string socketPath;
  int sockfd;
};

/// LLMGuidedSearcher - Delegates to real KLEE searchers based on LLM decision
class LLMGuidedSearcher : public Searcher {
public:
  LLMGuidedSearcher(Executor &executor, RNG &rng, 
                    InMemoryExecutionTree *executionTree);
  ~LLMGuidedSearcher() override;

  ExecutionState &selectState() override;
  void update(ExecutionState *current,
              const std::vector<ExecutionState *> &addedStates,
              const std::vector<ExecutionState *> &removedStates) override;
  bool empty() override;
  void printName(llvm::raw_ostream &os) override;

private:
  Executor &executor;
  RNG &theRNG;
  InMemoryExecutionTree *executionTree;
  
  // All available searchers - LLM picks which one to use
  std::map<std::string, std::unique_ptr<Searcher>> searchers;
  
  // Currently active searcher (chosen by LLM)
  std::string currentSearcherName;
  Searcher *currentSearcher;
  
  // Default searcher when policy server unavailable
  std::string defaultSearcherName;
  
  // Policy server client
  std::unique_ptr<PolicyClient> policyClient;
  
  // Track current function to detect changes
  std::string lastFunction;
  
  // Statistics
  unsigned totalSelections = 0;
  unsigned policyQueries = 0;
  std::map<std::string, unsigned> searcherUsage;
  
  // Helpers
  void initSearchers();
  std::string getCurrentFunction(ExecutionState *state) const;
  std::string getFunctionSignature(ExecutionState *state) const;
  StateFeatures buildFeatures(ExecutionState *state) const;
  void switchSearcher(const std::string &name);
};

} // namespace klee

#endif // KLEE_LLMGUIDEDSEARCHER_H
