///===-- LLMGuidedSearcher.h - Policy-server guided search ------*- C++ -*-===//
///
/// A searcher that communicates with an external LLM policy server to decide
/// which KLEE searcher to use. All LLM communication is FULLY ASYNCHRONOUS:
/// KLEE never blocks waiting for an LLM response.
///
/// Architecture:
///   KLEE main loop --> update()/selectState() --> zero-latency delegation
///                                                      |
///   Background thread --[features]--> Policy Server --[LLM]--> response
///                     <--[decision]--                        stored in
///                                                         pendingResponse
///
/// The main loop picks up the response on the NEXT update() call.
/// Between sends, the current searcher keeps running at full speed.
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
#include <thread>
#include <mutex>
#include <atomic>
#include <condition_variable>

namespace klee {

class Executor;
class InMemoryExecutionTree;

/// Lightweight features sent to policy server (JSON-serializable)
struct StateFeatures {
  std::string queryType;
  std::string healthStatus;

  std::string functionName;
  std::string functionSignature;
  unsigned stackDepth = 0;

  unsigned constraintCount = 0;
  unsigned depth = 0;
  unsigned steppedInstructions = 0;
  unsigned instsSinceCovNew = 0;
  bool coveredNew = false;
  unsigned symbolicVarCount = 0;

  unsigned activeStates = 0;
  unsigned totalForks = 0;
  unsigned inhibitedForks = 0;
  unsigned coveredInstructions = 0;
  unsigned uncoveredInstructions = 0;
  unsigned coveredBranches = 0;
  unsigned totalInstructions = 0;
  unsigned externalCalls = 0;

  uint64_t solverTime = 0;
  uint64_t forkTime = 0;

  unsigned minDistToUncovered = 0;
  unsigned minDistToReturn = 0;

  unsigned stateDistToReturn = 0;
  bool inMainFunction = false;
  bool nearTermination = false;
  unsigned completedStates = 0;

  std::string toJson() const;
};

/// Async client for communicating with the policy server.
/// All socket I/O runs on a background thread; the main thread never blocks.
class PolicyClient {
public:
  explicit PolicyClient(const std::string &socketPath);
  ~PolicyClient();

  /// Start the background I/O thread.
  void start();

  /// Stop the background thread and close the socket.
  void stop();

  /// Enqueue a query (non-blocking). Drops if one is already in-flight.
  void sendAsync(const StateFeatures &features);

  /// Check if a response arrived (non-blocking). Returns true + fills out.
  bool tryRecv(std::string &response);

  bool isRunning() const { return running.load(); }

private:
  std::string socketPath;
  int sockfd = -1;

  // Background thread
  std::thread ioThread;
  std::atomic<bool> running{false};
  std::atomic<bool> shutdownRequested{false};

  // Outbound: main thread -> background thread
  std::mutex sendMutex;
  std::condition_variable sendCv;
  std::string pendingSend;
  bool hasPendingSend = false;

  // Inbound: background thread -> main thread
  std::mutex recvMutex;
  std::string pendingResponse;
  bool hasResponse = false;

  void ioLoop();
  bool connectSocket();
  void disconnectSocket();
  std::string doBlockingQuery(const std::string &json);
};

/// LLMGuidedSearcher - Zero-latency searcher with async LLM guidance
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

  std::map<std::string, std::unique_ptr<Searcher>> searchers;

  std::string currentSearcherName;
  Searcher *currentSearcher;
  std::string defaultSearcherName;

  std::unique_ptr<PolicyClient> policyClient;

  std::string lastFunction;

  // Health monitoring
  unsigned lastCoveredInstructions = 0;
  unsigned lastCoveredBranches = 0;
  unsigned coverageStallCounter = 0;
  unsigned lastCompletedStates = 0;
  unsigned testGenStallCounter = 0;
  unsigned instructionsSinceHealthCheck = 0;
  unsigned peakStates = 0;
  unsigned trackedStateCount = 1;
  std::string lastHealthStatus;

  // Async query management - only one in-flight at a time
  bool queryInFlight = false;

  // Statistics
  unsigned totalSelections = 0;
  unsigned policyQueries = 0;
  unsigned healthQueries = 0;
  unsigned asyncResponsesApplied = 0;
  std::map<std::string, unsigned> searcherUsage;

  void initSearchers();
  std::string getCurrentFunction(ExecutionState *state) const;
  std::string getFunctionSignature(ExecutionState *state) const;
  StateFeatures buildFeatures(ExecutionState *state) const;
  void switchSearcher(const std::string &name);
  void applyDecision(const std::string &decision);
  std::string checkGlobalHealth(ExecutionState *state);
  void checkAsyncResponse();
};

} // namespace klee

#endif // KLEE_LLMGUIDEDSEARCHER_H
