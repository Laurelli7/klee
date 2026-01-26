///===-- LLMGuidedSearcher.cpp - Policy-server guided search ----*- C++ -*-===//
///
/// Implementation of searcher that delegates to real KLEE searchers
/// based on decisions from an external LLM policy server.
///
///===----------------------------------------------------------------------===//

#include "LLMGuidedSearcher.h"
#include "CoreStats.h"
#include "Executor.h"
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
      cl::desc("Timeout for policy server response in milliseconds (default=100)"),
      cl::init(100));

  cl::opt<bool> PolicyVerbose(
      "policy-verbose",
      cl::desc("Print verbose policy server logs"),
      cl::init(false));

  cl::opt<std::string> DefaultSearcher(
      "policy-default",
      cl::desc("Default searcher when policy server unavailable (default=nurs:covnew)"),
      cl::init("nurs:covnew"));
}

//===----------------------------------------------------------------------===//
// StateFeatures Implementation
//===----------------------------------------------------------------------===//

std::string StateFeatures::toJson() const {
  std::ostringstream ss;
  ss << "{"
     // Schema - explains each field to the LLM
     << "\"_schema\":{"
     << "\"function\":\"Current function name being executed\","
     << "\"signature\":\"Function type signature\","
     << "\"stack_depth\":\"Call stack depth\","
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
     << "\"min_dist_to_return\":\"Distance to function return\""
     << "},"
     // Actual data
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
     << "\"min_dist_to_return\":" << minDistToReturn
     << "}"
     << "}";
  return ss.str();
}

//===----------------------------------------------------------------------===//
// PolicyClient Implementation (Unix Socket)
//===----------------------------------------------------------------------===//

PolicyClient::PolicyClient(const std::string &socketPath)
    : socketPath(socketPath), sockfd(-1) {}

PolicyClient::~PolicyClient() {
  disconnect();
}

bool PolicyClient::connect() {
  if (sockfd >= 0) return true;
  
  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sockfd < 0) {
    if (PolicyVerbose) {
      llvm::errs() << "[Policy] Failed to create socket\n";
    }
    return false;
  }
  
  struct sockaddr_un addr;
  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, socketPath.c_str(), sizeof(addr.sun_path) - 1);
  
  if (::connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    if (PolicyVerbose) {
      llvm::errs() << "[Policy] Failed to connect to " << socketPath << "\n";
    }
    close(sockfd);
    sockfd = -1;
    return false;
  }
  
  if (PolicyVerbose) {
    llvm::errs() << "[Policy] Connected to " << socketPath << "\n";
  }
  return true;
}

void PolicyClient::disconnect() {
  if (sockfd >= 0) {
    close(sockfd);
    sockfd = -1;
  }
}

bool PolicyClient::isConnected() const {
  return sockfd >= 0;
}

std::string PolicyClient::query(const StateFeatures &features) {
  if (!isConnected() && !connect()) {
    return "";  // Empty = use default
  }
  
  std::string msg = features.toJson() + "\n";
  ssize_t sent = write(sockfd, msg.c_str(), msg.size());
  if (sent != static_cast<ssize_t>(msg.size())) {
    disconnect();
    return "";
  }
  
  struct pollfd pfd;
  pfd.fd = sockfd;
  pfd.events = POLLIN;
  
  int ret = poll(&pfd, 1, PolicyTimeout);
  if (ret <= 0) {
    if (PolicyVerbose) {
      llvm::errs() << "[Policy] Timeout waiting for response\n";
    }
    return "";
  }
  
  char buf[256];
  ssize_t n = read(sockfd, buf, sizeof(buf) - 1);
  if (n <= 0) {
    disconnect();
    return "";
  }
  buf[n] = '\0';
  
  // Trim whitespace
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
    // Fallback to nurs:covnew if default is invalid
    currentSearcherName = "nurs:covnew";
    currentSearcher = searchers[currentSearcherName].get();
  }
  
  policyClient = std::make_unique<PolicyClient>(PolicySocketPath);
  
  if (PolicyVerbose) {
    llvm::errs() << "[Policy] LLMGuidedSearcher initialized\n"
                 << "  Socket: " << PolicySocketPath << "\n"
                 << "  Default: " << defaultSearcherName << "\n"
                 << "  Available searchers: ";
    for (const auto &kv : searchers) {
      llvm::errs() << kv.first << " ";
    }
    llvm::errs() << "\n";
  }
}

LLMGuidedSearcher::~LLMGuidedSearcher() {
  if (PolicyVerbose) {
    llvm::errs() << "\n[Policy] Final Statistics\n"
                 << "  Total selections: " << totalSelections << "\n"
                 << "  Policy queries: " << policyQueries << "\n"
                 << "  Searcher usage:\n";
    for (const auto &kv : searcherUsage) {
      llvm::errs() << "    " << kv.first << ": " << kv.second << "\n";
    }
  }
}

void LLMGuidedSearcher::initSearchers() {
  // Create all available KLEE searchers
  
  // Basic searchers
  searchers["dfs"] = std::make_unique<DFSSearcher>();
  searchers["bfs"] = std::make_unique<BFSSearcher>();
  searchers["random-state"] = std::make_unique<RandomSearcher>(theRNG);
  
  // Random path (needs execution tree)
  if (executionTree) {
    searchers["random-path"] = std::make_unique<RandomPathSearcher>(executionTree, theRNG);
  }
  
  // NURS variants (WeightedRandomSearcher)
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
  
  f.functionName = getCurrentFunction(state);
  f.functionSignature = getFunctionSignature(state);
  f.stackDepth = state->stack.size();
  f.constraintCount = state->constraints.size();
  f.depth = state->depth;
  f.steppedInstructions = state->steppedInstructions;
  f.instsSinceCovNew = state->instsSinceCovNew;
  f.coveredNew = state->coveredNew;
  f.symbolicVarCount = state->symbolics.size();
  
  f.activeStates = stats::states;
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
  
  return f;
}

void LLMGuidedSearcher::switchSearcher(const std::string &name) {
  auto it = searchers.find(name);
  if (it != searchers.end() && it->second) {
    currentSearcherName = name;
    currentSearcher = it->second.get();
    
    if (PolicyVerbose) {
      llvm::errs() << "[Policy] Switched to: " << name << "\n";
    }
  } else {
    if (PolicyVerbose) {
      llvm::errs() << "[Policy] Unknown searcher: " << name 
                   << ", keeping: " << currentSearcherName << "\n";
    }
  }
}

ExecutionState &LLMGuidedSearcher::selectState() {
  assert(!empty() && "Selecting from empty searcher");
  
  totalSelections++;
  searcherUsage[currentSearcherName]++;
  
  return currentSearcher->selectState();
}

void LLMGuidedSearcher::update(ExecutionState *current,
                                const std::vector<ExecutionState *> &addedStates,
                                const std::vector<ExecutionState *> &removedStates) {
  // Update ALL searchers so they stay in sync
  for (auto &kv : searchers) {
    if (kv.second) {
      kv.second->update(current, addedStates, removedStates);
    }
  }
  
  // Check if we should query the policy server
  if (current && !removedStates.empty()) {
    // State terminated - might want to reconsider strategy
  }
  
  if (current) {
    std::string newFunc = getCurrentFunction(current);
    
    // Query LLM when entering a new function
    if (newFunc != lastFunction && !newFunc.empty()) {
      lastFunction = newFunc;
      policyQueries++;
      
      StateFeatures features = buildFeatures(current);
      std::string decision = policyClient->query(features);
      
      if (!decision.empty()) {
        // Normalize the response (lowercase, handle variations)
        std::string normalized = decision;
        std::transform(normalized.begin(), normalized.end(), 
                       normalized.begin(), ::tolower);
        
        // Try exact match first (policy server should return exact names)
        if (searchers.count(normalized)) {
          switchSearcher(normalized);
        }
        // Fuzzy matching as fallback
        else if (normalized.find("dfs") != std::string::npos &&
                 normalized.find("bfs") == std::string::npos) {
          switchSearcher("dfs");
        } else if (normalized.find("bfs") != std::string::npos) {
          switchSearcher("bfs");
        } else if (normalized.find("random-path") != std::string::npos || 
                   normalized.find("randompath") != std::string::npos) {
          if (searchers.count("random-path")) {
            switchSearcher("random-path");
          }
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
        }
        // If no match, keep current searcher
        
        if (PolicyVerbose) {
          llvm::errs() << "[Policy] Function: " << newFunc 
                       << " -> " << currentSearcherName << "\n";
        }
      }
    }
  }
}

bool LLMGuidedSearcher::empty() {
  return currentSearcher->empty();
}

void LLMGuidedSearcher::printName(llvm::raw_ostream &os) {
  os << "LLMGuidedSearcher(" << currentSearcherName << ")";
}
