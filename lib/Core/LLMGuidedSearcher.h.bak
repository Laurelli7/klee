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
  // Query type: "function" or "health"
  std::string queryType;
  
  // Health status (only for health queries)
  std::string healthStatus;
  
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
  
  // Distance heuristics (global)
  unsigned minDistToUncovered;
  unsigned minDistToReturn;
  
  // Per-state completion proximity
  unsigned stateDistToReturn;      // This state's distance to return instruction
  bool inMainFunction;             // True if current function is main
  bool nearTermination;            // True if very close to completing (dist <= 5 && shallow stack)
  unsigned completedStates;        // Total states that have terminated (ktest potential)
  
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
  
  // ===== Global Health Monitoring =====
  // Coverage history for stall detection
  unsigned lastCoveredInstructions = 0;
  unsigned lastCoveredBranches = 0;
  unsigned coverageStallCounter = 0;
  
  // Test generation tracking
  unsigned lastCompletedStates = 0;
  unsigned testGenStallCounter = 0;
  
  // Timing for periodic health checks
  unsigned instructionsSinceHealthCheck = 0;
  unsigned healthCheckInterval = 1000;  // Check every N instructions
  
  // State explosion tracking
  unsigned peakStates = 0;
  
  // Tracked active state count (stats::states doesn't work - it's indexed only)
  unsigned trackedStateCount = 1;  // Start with 1 (initial state)
  
  // Last health status sent to LLM
  std::string lastHealthStatus;
  
  // Statistics
  unsigned totalSelections = 0;
  unsigned policyQueries = 0;
  unsigned healthQueries = 0;
  std::map<std::string, unsigned> searcherUsage;
  
  // Helpers
  void initSearchers();
  std::string getCurrentFunction(ExecutionState *state) const;
  std::string getFunctionSignature(ExecutionState *state) const;
  StateFeatures buildFeatures(ExecutionState *state) const;
  void switchSearcher(const std::string &name);
  
  // Health monitoring
  std::string checkGlobalHealth(ExecutionState *state);
  void queryOnHealthChange(ExecutionState *state, const std::string &healthStatus);
};

} // namespace klee

#endif // KLEE_LLMGUIDEDSEARCHER_H
