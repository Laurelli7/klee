///===-- LLMGuidedSearcher.cpp - Async policy-server guided search -*- C++ -*-//
///
/// Fully asynchronous searcher: KLEE main loop NEVER blocks on LLM.
/// A background thread handles all socket I/O. The main thread sends
/// features fire-and-forget and picks up responses when they arrive.
///
///===----------------------------------------------------------------------===//

#include "LLMGuidedSearcher.h"
#include "CoreStats.h"
#include "Executor.h"
#include "SearcherDefs.h"
#include "klee/Module/KInstruction.h"
#include "klee/Statistics/Statistics.h"

#include "klee/Support/CompilerWarning.h"
DISABLE_WARNING_PUSH
DISABLE_WARNING_DEPRECATED_DECLARATIONS
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"
DISABLE_WARNING_POP

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>

#include <algorithm>
#include <sstream>
#include <chrono>

using namespace klee;
using namespace llvm;

//===----------------------------------------------------------------------===//
// Command Line Options
//===----------------------------------------------------------------------===//

namespace {
  cl::opt<std::string> PolicySocketPath(
      "policy-socket",
      cl::desc("Unix socket path for policy server (default: /tmp/klee-policy.sock)"),
      cl::init("/tmp/klee-policy.sock"));

  cl::opt<unsigned> PolicyTimeout(
      "policy-timeout",
      cl::desc("Timeout for policy server response in ms (default=30000)"),
      cl::init(30000));

  cl::opt<bool> PolicyVerbose(
      "policy-verbose",
      cl::desc("Print verbose policy server logs"),
      cl::init(false));

  cl::opt<std::string> DefaultSearcher(
      "policy-default",
      cl::desc("Default searcher when policy server unavailable (default=nurs:covnew)"),
      cl::init("nurs:covnew"));

  cl::opt<unsigned> HealthCheckInterval(
      "policy-health-interval",
      cl::desc("Instructions between health checks (default=1000)"),
      cl::init(1000));

  cl::opt<unsigned> CoverageStallThreshold(
      "policy-stall-threshold",
      cl::desc("Health checks without coverage progress to trigger stall (default=5)"),
      cl::init(5));

  cl::opt<unsigned> StateExplosionThreshold(
      "policy-explosion-threshold",
      cl::desc("Number of states to consider 'explosion' (default=500)"),
      cl::init(500));
}

//===----------------------------------------------------------------------===//
// StateFeatures::toJson
//===----------------------------------------------------------------------===//

std::string StateFeatures::toJson() const {
  std::ostringstream ss;
  ss << "{"
     << "\"query_type\":\"" << queryType << "\","
     << "\"health_status\":\"" << healthStatus << "\","
     << "\"_schema\":{"
     << "\"function\":\"Current function name being executed\","
     << "\"signature\":\"Function type signature\","
     << "\"stack_depth\":\"Call stack depth (1 = in main)\","
     << "\"constraints\":\"Number of symbolic constraints (more = harder to solve)\","
     << "\"depth\":\"Execution depth - branches taken to reach this state\","
     << "\"stepped_instructions\":\"Total instructions executed by this state\","
     << "\"insts_since_cov_new\":\"Instructions since last new coverage (high = stuck)\","
     << "\"covered_new\":\"True if this state just covered new code\","
     << "\"symbolic_vars\":\"Number of symbolic variables\","
     << "\"active_states\":\"Total execution states being tracked\","
     << "\"total_forks\":\"Total forks so far\","
     << "\"inhibited_forks\":\"Forks that were blocked\","
     << "\"covered_instructions\":\"Unique instructions covered\","
     << "\"uncovered_instructions\":\"Instructions not yet covered\","
     << "\"covered_branches\":\"Branch sides covered (true + false)\","
     << "\"total_instructions\":\"Total instructions executed globally\","
     << "\"external_calls\":\"External function calls\","
     << "\"solver_time_us\":\"Microseconds in constraint solver\","
     << "\"fork_time_us\":\"Microseconds forking states\","
     << "\"min_dist_to_uncovered\":\"Distance to nearest uncovered code\","
     << "\"min_dist_to_return\":\"Distance to function return\","
     << "\"state_dist_to_return\":\"THIS STATE's distance to return\","
     << "\"in_main_function\":\"True if executing in main()\","
     << "\"near_termination\":\"True if state is close to completing\","
     << "\"completed_states\":\"Total states that have terminated\""
     << "},"
     << "\"data\":{"
     << "\"function\":\"" << functionName << "\","
     << "\"signature\":\"" << functionSignature << "\","
     << "\"stack_depth\":" << stackDepth << ","
     << "\"constraints\":" << constraintCount << ","
     << "\"depth\":" << depth << ","
     << "\"stepped_instructions\":" << steppedInstructions << ","
     << "\"insts_since_cov_new\":" << instsSinceCovNew << ","
     << "\"covered_new\":" << (coveredNew ? "true" : "false") << ","
     << "\"symbolic_vars\":" << symbolicVarCount << ","
     << "\"active_states\":" << activeStates << ","
     << "\"total_forks\":" << totalForks << ","
     << "\"inhibited_forks\":" << inhibitedForks << ","
     << "\"covered_instructions\":" << coveredInstructions << ","
     << "\"uncovered_instructions\":" << uncoveredInstructions << ","
     << "\"covered_branches\":" << coveredBranches << ","
     << "\"total_instructions\":" << totalInstructions << ","
     << "\"external_calls\":" << externalCalls << ","
     << "\"solver_time_us\":" << solverTime << ","
     << "\"fork_time_us\":" << forkTime << ","
     << "\"min_dist_to_uncovered\":" << minDistToUncovered << ","
     << "\"min_dist_to_return\":" << minDistToReturn << ","
     << "\"state_dist_to_return\":" << stateDistToReturn << ","
     << "\"in_main_function\":" << (inMainFunction ? "true" : "false") << ","
     << "\"near_termination\":" << (nearTermination ? "true" : "false") << ","
     << "\"completed_states\":" << completedStates
     << "}"
     << "}";
  return ss.str();
}

//===----------------------------------------------------------------------===//
// PolicyClient Implementation (Async, background thread)
//===----------------------------------------------------------------------===//

PolicyClient::PolicyClient(const std::string &socketPath)
    : socketPath(socketPath) {}

PolicyClient::~PolicyClient() {
  stop();
}

void PolicyClient::start() {
  if (running.load()) return;
  shutdownRequested.store(false);
  running.store(true);

  // Try an eager connection so we can detect issues early
  llvm::errs() << "[Policy] Starting background I/O thread...\n"
               << "[Policy] Attempting initial connection to " << socketPath << "\n";
  if (connectSocket()) {
    llvm::errs() << "[Policy] Initial connection successful!\n";
  } else {
    llvm::errs() << "[Policy] WARNING: Could not connect to " << socketPath
                 << " - will retry on each query.\n"
                 << "[Policy] Make sure the policy server is running BEFORE starting KLEE:\n"
                 << "[Policy]   python3 tools/klee/policy_server.py --provider openai\n";
  }

  ioThread = std::thread(&PolicyClient::ioLoop, this);
}

void PolicyClient::stop() {
  if (!running.load()) return;

  {
    std::lock_guard<std::mutex> lk(sendMutex);
    shutdownRequested.store(true);
    sendCv.notify_one();
  }

  if (ioThread.joinable())
    ioThread.join();

  disconnectSocket();
  running.store(false);
}

void PolicyClient::sendAsync(const StateFeatures &features) {
  std::string json = features.toJson() + "\n";
  {
    std::lock_guard<std::mutex> lk(sendMutex);
    pendingSend = std::move(json);
    hasPendingSend = true;
  }
  sendCv.notify_one();
}

bool PolicyClient::tryRecv(std::string &response) {
  std::lock_guard<std::mutex> lk(recvMutex);
  if (!hasResponse) return false;
  response = std::move(pendingResponse);
  hasResponse = false;
  return true;
}

// Background thread: waits for sends, does blocking socket I/O, stores results
void PolicyClient::ioLoop() {
  while (!shutdownRequested.load()) {
    std::string toSend;

    // Wait for something to send (or shutdown)
    {
      std::unique_lock<std::mutex> lk(sendMutex);
      sendCv.wait(lk, [this] {
        return hasPendingSend || shutdownRequested.load();
      });
      if (shutdownRequested.load()) break;
      toSend = std::move(pendingSend);
      hasPendingSend = false;
    }

    // Do the blocking socket I/O on THIS thread (not the main KLEE thread)
    std::string result = doBlockingQuery(toSend);

    // Store result for the main thread to pick up
    if (!result.empty()) {
      std::lock_guard<std::mutex> lk(recvMutex);
      pendingResponse = std::move(result);
      hasResponse = true;
    }
  }
}

bool PolicyClient::connectSocket() {
  if (sockfd >= 0) return true;

  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sockfd < 0) return false;

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);

  if (::connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    if (PolicyVerbose)
      llvm::errs() << "[Policy] Connection failed to " << socketPath
                   << " (errno=" << errno << "). Will retry on next query.\n";
    close(sockfd);
    sockfd = -1;
    return false;
  }

  llvm::errs() << "[Policy] Connected to policy server at " << socketPath << "\n";
  return true;
}

void PolicyClient::disconnectSocket() {
  if (sockfd >= 0) {
    close(sockfd);
    sockfd = -1;
  }
}

std::string PolicyClient::doBlockingQuery(const std::string &json) {
  // Try to connect if not connected
  if (sockfd < 0 && !connectSocket()) {
    // Periodically log that we can't connect (not every single attempt)
    static unsigned failCount = 0;
    if (++failCount % 100 == 1)
      llvm::errs() << "[Policy] Cannot connect to server (attempt #" << failCount
                   << "). Running without LLM guidance.\n";
    return "";
  }

  // Send
  ssize_t sent = write(sockfd, json.c_str(), json.size());
  if (sent != static_cast<ssize_t>(json.size())) {
    if (PolicyVerbose)
      llvm::errs() << "[Policy] Write failed (sent=" << sent
                   << ", expected=" << json.size() << ", errno=" << errno << ")\n";
    disconnectSocket();
    return "";
  }

  // Wait for response with timeout
  struct pollfd pfd;
  pfd.fd = sockfd;
  pfd.events = POLLIN;

  int ret = poll(&pfd, 1, PolicyTimeout);
  if (ret <= 0) {
    if (PolicyVerbose)
      llvm::errs() << "[Policy] Timeout waiting for response (" << PolicyTimeout << "ms)\n";
    return "";
  }

  // Read response
  char buf[256];
  ssize_t n = read(sockfd, buf, sizeof(buf) - 1);
  if (n <= 0) {
    disconnectSocket();
    return "";
  }
  buf[n] = '\0';

  std::string response(buf);
  response.erase(response.find_last_not_of(" \n\r\t") + 1);
  return response;
}

//===----------------------------------------------------------------------===//
// LLMGuidedSearcher Implementation
//===----------------------------------------------------------------------===//

LLMGuidedSearcher::LLMGuidedSearcher(Executor &executor, RNG &rng,
                                       InMemoryExecutionTree *executionTree)
    : executor(executor), theRNG(rng), executionTree(executionTree) {

  defaultSearcherName = DefaultSearcher;
  currentSearcherName = defaultSearcherName;

  initSearchers();

  currentSearcher = searchers[currentSearcherName].get();
  if (!currentSearcher) {
    currentSearcherName = "nurs:covnew";
    currentSearcher = searchers[currentSearcherName].get();
  }

  // Create and start the async policy client
  policyClient = std::make_unique<PolicyClient>(PolicySocketPath);
  policyClient->start();

  if (PolicyVerbose) {
    llvm::errs() << "[Policy] LLMGuidedSearcher initialized (ASYNC mode)\n"
                 << "  Socket: " << PolicySocketPath << "\n"
                 << "  Default: " << defaultSearcherName << "\n"
                 << "  Available searchers: ";
    for (const auto &kv : searchers)
      llvm::errs() << kv.first << " ";
    llvm::errs() << "\n";
  }
}

LLMGuidedSearcher::~LLMGuidedSearcher() {
  // Stop background thread before destroying searchers
  if (policyClient)
    policyClient->stop();

  if (PolicyVerbose) {
    llvm::errs() << "\n[Policy] Final Statistics\n"
                 << "  Total selections: " << totalSelections << "\n"
                 << "  Policy queries sent: " << policyQueries << "\n"
                 << "  Health queries sent: " << healthQueries << "\n"
                 << "  Async responses applied: " << asyncResponsesApplied << "\n"
                 << "  Searcher usage:\n";
    for (const auto &kv : searcherUsage)
      llvm::errs() << "    " << kv.first << ": " << kv.second << "\n";
  }
}

/// Check if a function name is a libc/runtime function (not application code).
/// These are skipped for LLM queries - we use the default searcher instead.
static bool isLibcFunction(const std::string &name) {
  if (name.empty()) return true;
  // Prefixes: underscore-prefixed are libc/compiler internals
  if (name[0] == '_') return true;
  // Common libc functions
  static const char *libcNames[] = {
    "strlen", "strcpy", "strncpy", "strcat", "strncat", "strcmp", "strncmp",
    "strchr", "strrchr", "strstr", "strdup", "strtok", "strtol", "strtoul",
    "memcpy", "memmove", "memset", "memcmp", "memchr",
    "malloc", "calloc", "realloc", "free",
    "read", "write", "open", "close", "fopen", "fclose", "fread", "fwrite",
    "fgets", "fputs", "printf", "fprintf", "sprintf", "snprintf",
    "scanf", "fscanf", "sscanf", "fflush", "fseek", "ftell",
    "puts", "gets", "perror", "exit", "abort", "atexit",
    "getenv", "setenv", "getpid", "fork", "execve", "wait", "waitpid",
    "signal", "sigaction", "kill", "isatty", "ioctl",
    "isalpha", "isdigit", "isalnum", "isspace", "toupper", "tolower",
    "qsort", "bsearch", "atoi", "atol", "rand", "srand", "time", "clock",
    "stat", "fstat", "lstat", "access", "chmod", "mkdir", "rmdir",
    "getcwd", "chdir", "opendir", "readdir", "closedir",
    nullptr
  };
  for (const char **p = libcNames; *p; ++p)
    if (name == *p) return true;
  return false;
}

void LLMGuidedSearcher::initSearchers() {
  // Basic searchers
  searchers["dfs"] = std::make_unique<DFSSearcher>();
  searchers["bfs"] = std::make_unique<BFSSearcher>();
  searchers["random-state"] = std::make_unique<RandomSearcher>(theRNG);

  if (executionTree)
    searchers["random-path"] = std::make_unique<RandomPathSearcher>(executionTree, theRNG);

  // NURS variants
  searchers["nurs:covnew"] = std::make_unique<WeightedRandomSearcher>(
      WeightedRandomSearcher::CoveringNew, theRNG);
  searchers["nurs:md2u"] = std::make_unique<WeightedRandomSearcher>(
      WeightedRandomSearcher::MinDistToUncovered, theRNG);
  searchers["nurs:depth"] = std::make_unique<WeightedRandomSearcher>(
      WeightedRandomSearcher::Depth, theRNG);
  searchers["nurs:rp"] = std::make_unique<WeightedRandomSearcher>(
      WeightedRandomSearcher::RP, theRNG);
  searchers["nurs:icnt"] = std::make_unique<WeightedRandomSearcher>(
      WeightedRandomSearcher::InstCount, theRNG);
  searchers["nurs:cpicnt"] = std::make_unique<WeightedRandomSearcher>(
      WeightedRandomSearcher::CPInstCount, theRNG);
  searchers["nurs:qc"] = std::make_unique<WeightedRandomSearcher>(
      WeightedRandomSearcher::QueryCost, theRNG);

  // EMPC (if inter-procedural CFG is available from executor)
  if (executor.mpcICFG && executor.mpcIPDA) {
    searchers["empc"] = std::make_unique<EmpcSearcher>(
        executor.mpcICFG, executor.mpcIPDA, theRNG);
  }

  // SGS: 4 SubpathGuidedSearchers interleaved
  {
    std::vector<Searcher *> sgsSearchers;
    for (unsigned i = 0; i <= 3; i++)
      sgsSearchers.push_back(new SubpathGuidedSearcher(executor, i, theRNG));
    searchers["sgs"] = std::unique_ptr<Searcher>(new InterleavedSearcher(sgsSearchers));
  }

  // Interleaved combos (KLEE default = random-path + nurs:covnew)
  if (executionTree) {
    std::vector<Searcher *> defaultCombo;
    defaultCombo.push_back(new RandomPathSearcher(executionTree, theRNG));
    defaultCombo.push_back(new WeightedRandomSearcher(
        WeightedRandomSearcher::CoveringNew, theRNG));
    searchers["default"] = std::unique_ptr<Searcher>(new InterleavedSearcher(defaultCombo));
  }
}

std::string LLMGuidedSearcher::getCurrentFunction(ExecutionState *state) const {
  if (!state || !state->pc || !state->pc->inst) return "";
  const llvm::Function *func = state->pc->inst->getParent()->getParent();
  return func ? func->getName().str() : "";
}

std::string LLMGuidedSearcher::getFunctionSignature(ExecutionState *state) const {
  if (!state || !state->pc || !state->pc->inst) return "";
  const llvm::Function *func = state->pc->inst->getParent()->getParent();
  if (!func) return "";

  std::string sig;
  llvm::raw_string_ostream rso(sig);
  func->getReturnType()->print(rso);
  rso << " " << func->getName() << "(";
  for (auto it = func->arg_begin(); it != func->arg_end(); ++it) {
    if (it != func->arg_begin()) rso << ", ";
    it->getType()->print(rso);
  }
  rso << ")";
  return sig;
}

StateFeatures LLMGuidedSearcher::buildFeatures(ExecutionState *state) const {
  StateFeatures f;
  f.queryType = "function";
  f.healthStatus = "";

  f.functionName = getCurrentFunction(state);
  f.functionSignature = getFunctionSignature(state);
  f.stackDepth = state->stack.size();
  f.constraintCount = state->constraints.size();
  f.depth = state->depth;
  f.steppedInstructions = state->steppedInstructions;
  f.instsSinceCovNew = state->instsSinceCovNew;
  f.coveredNew = state->coveredNew;
  f.symbolicVarCount = state->symbolics.size();

  f.activeStates = trackedStateCount;
  f.totalForks = stats::forks;
  f.inhibitedForks = stats::inhibitedForks;
  f.coveredInstructions = stats::coveredInstructions;
  f.uncoveredInstructions = stats::uncoveredInstructions;
  f.coveredBranches = stats::trueBranches + stats::falseBranches;
  f.totalInstructions = stats::instructions;
  f.externalCalls = stats::externalCalls;

  f.solverTime = stats::solverTime;
  f.forkTime = stats::forkTime;

  f.minDistToUncovered = stats::minDistToUncovered;
  f.minDistToReturn = stats::minDistToReturn;

  if (state->pc && state->pc->info) {
    f.stateDistToReturn = theStatisticManager->getIndexedValue(
        stats::minDistToReturn, state->pc->info->id);
  } else {
    f.stateDistToReturn = UINT_MAX;
  }

  f.inMainFunction = (f.functionName == "main");
  f.nearTermination = (f.stateDistToReturn > 0 &&
                       f.stateDistToReturn <= 10 &&
                       f.stackDepth <= 2);
  f.completedStates = stats::terminationExit;

  return f;
}

void LLMGuidedSearcher::switchSearcher(const std::string &name) {
  auto it = searchers.find(name);
  if (it != searchers.end() && it->second) {
    currentSearcherName = name;
    currentSearcher = it->second.get();
    if (PolicyVerbose)
      llvm::errs() << "[Policy] Switched to: " << name << "\n";
  } else {
    if (PolicyVerbose)
      llvm::errs() << "[Policy] Unknown searcher: " << name
                   << ", keeping: " << currentSearcherName << "\n";
  }
}

void LLMGuidedSearcher::applyDecision(const std::string &decision) {
  std::string normalized = decision;
  std::transform(normalized.begin(), normalized.end(),
                 normalized.begin(), ::tolower);

  if (searchers.count(normalized)) {
    switchSearcher(normalized);
  } else if (normalized.find("dfs") != std::string::npos &&
             normalized.find("bfs") == std::string::npos) {
    switchSearcher("dfs");
  } else if (normalized.find("bfs") != std::string::npos) {
    switchSearcher("bfs");
  } else if (normalized.find("random-path") != std::string::npos ||
             normalized.find("randompath") != std::string::npos) {
    if (searchers.count("random-path"))
      switchSearcher("random-path");
  } else if (normalized.find("random-state") != std::string::npos ||
             normalized.find("randomstate") != std::string::npos) {
    switchSearcher("random-state");
  } else if (normalized.find("md2u") != std::string::npos ||
             normalized.find("mindist") != std::string::npos) {
    switchSearcher("nurs:md2u");
  } else if (normalized.find("nurs:depth") != std::string::npos) {
    switchSearcher("nurs:depth");
  } else if (normalized.find("nurs:rp") != std::string::npos) {
    switchSearcher("nurs:rp");
  } else if (normalized.find("cpicnt") != std::string::npos) {
    switchSearcher("nurs:cpicnt");
  } else if (normalized.find("icnt") != std::string::npos ||
             normalized.find("instcount") != std::string::npos) {
    switchSearcher("nurs:icnt");
  } else if (normalized.find("qc") != std::string::npos ||
             normalized.find("querycost") != std::string::npos) {
    switchSearcher("nurs:qc");
  } else if (normalized.find("covnew") != std::string::npos ||
             normalized.find("coverage") != std::string::npos) {
    switchSearcher("nurs:covnew");
  } else if (normalized.find("empc") != std::string::npos ||
             normalized.find("path cover") != std::string::npos) {
    if (searchers.count("empc"))
      switchSearcher("empc");
  } else if (normalized.find("sgs") != std::string::npos ||
             normalized.find("subpath") != std::string::npos) {
    switchSearcher("sgs");
  } else if (normalized.find("default") != std::string::npos ||
             normalized.find("interleaved") != std::string::npos) {
    if (searchers.count("default"))
      switchSearcher("default");
  }
}

void LLMGuidedSearcher::checkAsyncResponse() {
  std::string response;
  if (policyClient->tryRecv(response)) {
    // A response has arrived from the background thread
    queryInFlight = false;
    asyncResponsesApplied++;

    if (!response.empty()) {
      applyDecision(response);
    }

    if (PolicyVerbose)
      llvm::errs() << "[Policy] Async response applied: " << response
                   << " -> " << currentSearcherName << "\n";
  }
}

//===----------------------------------------------------------------------===//
// selectState / update - THE HOT PATH (must be zero-latency)
//===----------------------------------------------------------------------===//

ExecutionState &LLMGuidedSearcher::selectState() {
  assert(!empty() && "Selecting from empty searcher");

  totalSelections++;
  searcherUsage[currentSearcherName]++;

  // Zero-cost: just delegates to current searcher
  return currentSearcher->selectState();
}

void LLMGuidedSearcher::update(ExecutionState *current,
                                const std::vector<ExecutionState *> &addedStates,
                                const std::vector<ExecutionState *> &removedStates) {
  // 1) Update ALL delegate searchers (same cost as before, unavoidable)
  for (auto &kv : searchers) {
    if (kv.second)
      kv.second->update(current, addedStates, removedStates);
  }

  // 2) Track state count
  trackedStateCount += addedStates.size();
  if (removedStates.size() <= trackedStateCount)
    trackedStateCount -= removedStates.size();
  else
    trackedStateCount = 0;

  if (!current) return;

  // 3) Check for async response from previous query (NON-BLOCKING: just a mutex try)
  checkAsyncResponse();

  // 4) Health monitoring (pure local computation, no I/O)
  instructionsSinceHealthCheck++;
  if (instructionsSinceHealthCheck >= HealthCheckInterval) {
    instructionsSinceHealthCheck = 0;

    std::string healthStatus = checkGlobalHealth(current);

    if (healthStatus != "healthy" && healthStatus != lastHealthStatus) {
      lastHealthStatus = healthStatus;

      // Send health query async (fire-and-forget)
      if (!queryInFlight) {
        healthQueries++;
        StateFeatures features = buildFeatures(current);
        features.queryType = "health";
        features.healthStatus = healthStatus;
        policyClient->sendAsync(features);
        queryInFlight = true;

        if (PolicyVerbose)
          llvm::errs() << "[Policy] Health query sent (async): " << healthStatus << "\n";
      }
    } else if (healthStatus == "healthy") {
      lastHealthStatus = healthStatus;
    }
  }

  // 5) Function-based query: fire-and-forget (NON-BLOCKING)
  //    Skip libc/runtime functions - only query LLM for program-specific code
  std::string newFunc = getCurrentFunction(current);
  if (newFunc != lastFunction && !newFunc.empty()) {
    lastFunction = newFunc;

    if (isLibcFunction(newFunc)) {
      // Silently skip libc functions, keep current searcher
      if (PolicyVerbose)
        llvm::errs() << "[Policy] Skip libc: " << newFunc << "\n";
    } else if (!queryInFlight) {
      // Only send for actual program functions
      policyQueries++;
      StateFeatures features = buildFeatures(current);
      policyClient->sendAsync(features);
      queryInFlight = true;

      if (PolicyVerbose)
        llvm::errs() << "[Policy] Query sent (async): " << newFunc << "\n";
    }
  }
}

//===----------------------------------------------------------------------===//
// Health Monitoring (pure local computation, no I/O)
//===----------------------------------------------------------------------===//

std::string LLMGuidedSearcher::checkGlobalHealth(ExecutionState *state) {
  unsigned currentCovered = stats::coveredInstructions;
  unsigned currentBranches = stats::trueBranches + stats::falseBranches;
  unsigned currentStates = trackedStateCount;
  unsigned currentCompleted = stats::terminationExit;

  std::vector<std::string> issues;

  if (currentCovered == lastCoveredInstructions &&
      currentBranches == lastCoveredBranches) {
    coverageStallCounter++;
    if (coverageStallCounter >= CoverageStallThreshold)
      issues.push_back("coverage_stalled");
  } else {
    coverageStallCounter = 0;
    lastCoveredInstructions = currentCovered;
    lastCoveredBranches = currentBranches;
  }

  if (currentCompleted == lastCompletedStates && currentStates > 50) {
    testGenStallCounter++;
    if (testGenStallCounter >= CoverageStallThreshold)
      issues.push_back("low_test_generation");
  } else {
    testGenStallCounter = 0;
    lastCompletedStates = currentCompleted;
  }

  if (currentStates > StateExplosionThreshold)
    issues.push_back("state_explosion");
  if (currentStates > peakStates)
    peakStates = currentStates;

  uint64_t totalTime = stats::solverTime + stats::forkTime + 1;
  double solverRatio = static_cast<double>(stats::solverTime) / totalTime;
  if (solverRatio > 0.7)
    issues.push_back("solver_pressure");

  if (state && state->constraints.size() > 50 && currentStates > 100)
    issues.push_back("memory_pressure");

  if (issues.empty()) return "healthy";

  std::string status;
  for (size_t i = 0; i < issues.size(); i++) {
    if (i > 0) status += ",";
    status += issues[i];
  }
  return status;
}

bool LLMGuidedSearcher::empty() {
  return currentSearcher->empty();
}

void LLMGuidedSearcher::printName(llvm::raw_ostream &os) {
  os << "LLMGuidedSearcher(" << currentSearcherName << ", async)";
}
