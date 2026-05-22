//===-- UserSearcher.cpp --------------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "UserSearcher.h"

#include "Executor.h"
#include "MergeHandler.h"
#include "Searcher.h"

#include "klee/Module/KModule.h"
#include "klee/Support/ErrorHandling.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/CommandLine.h"

#include <fstream>
#include <random>
#include <sstream>

using namespace llvm;
using namespace klee;

namespace klee {
llvm::cl::OptionCategory
    SearchCat("Search options", "These options control the search heuristic.");

cl::list<Searcher::CoreSearchType> CoreSearch(
    "search",
    cl::desc("Specify the search heuristic (default=random-path interleaved "
             "with nurs:covnew)"),
    cl::values(
        clEnumValN(Searcher::DFS, "dfs", "use Depth First Search (DFS)"),
        clEnumValN(Searcher::BFS, "bfs",
                   "use Breadth First Search (BFS), where scheduling decisions "
                   "are taken at the level of (2-way) forks"),
        clEnumValN(Searcher::RandomState, "random-state",
                   "randomly select a state to explore"),
        clEnumValN(Searcher::RandomPath, "random-path",
                   "use Random Path Selection (see OSDI'08 paper)"),
        clEnumValN(Searcher::NURS_CovNew, "nurs:covnew",
                   "use Non Uniform Random Search (NURS) with Coverage-New"),
        clEnumValN(Searcher::NURS_MD2U, "nurs:md2u",
                   "use NURS with Min-Dist-to-Uncovered"),
        clEnumValN(Searcher::NURS_Depth, "nurs:depth", "use NURS with depth"),
        clEnumValN(Searcher::NURS_RP, "nurs:rp", "use NURS with 1/2^depth"),
        clEnumValN(Searcher::NURS_ICnt, "nurs:icnt",
                   "use NURS with Instr-Count"),
        clEnumValN(Searcher::NURS_CPICnt, "nurs:cpicnt",
                   "use NURS with CallPath-Instr-Count"),
        clEnumValN(Searcher::NURS_QC, "nurs:qc", "use NURS with Query-Cost"),
        clEnumValN(Searcher::PerFunctionLLM, "per-function-llm",
                   "per-function dispatching searcher; assignment of "
                   "function->searcher loaded from "
                   "--per-function-assignment=<file>"),
        clEnumValN(Searcher::PerFunctionRandom, "per-function-random",
                   "per-function dispatching searcher; assignment is drawn "
                   "uniformly at random per function (seeded by "
                   "--per-function-random-seed)")),
    cl::cat(SearchCat));

cl::opt<std::string> PerFunctionAssignment(
    "per-function-assignment",
    cl::desc("Path to a text file mapping function name to searcher kind, "
             "one entry per line as `<func_name> <searcher_kind>`. "
             "Lines starting with # are ignored. Used by "
             "--search=per-function-llm."),
    cl::init(""), cl::cat(SearchCat));

cl::opt<unsigned> PerFunctionRandomSeed(
    "per-function-random-seed",
    cl::desc("Seed used by --search=per-function-random when drawing the "
             "per-function searcher assignment (default=1)"),
    cl::init(1), cl::cat(SearchCat));

cl::opt<Searcher::CoreSearchType> PerFunctionDefault(
    "per-function-default-search",
    cl::desc("Sub-searcher used for functions absent from the assignment "
             "(default=random-path)"),
    cl::values(
        clEnumValN(Searcher::DFS, "dfs", "DFS"),
        clEnumValN(Searcher::BFS, "bfs", "BFS"),
        clEnumValN(Searcher::RandomState, "random-state", "random-state"),
        clEnumValN(Searcher::RandomPath, "random-path", "random-path"),
        clEnumValN(Searcher::NURS_CovNew, "nurs:covnew", "nurs:covnew"),
        clEnumValN(Searcher::NURS_MD2U, "nurs:md2u", "nurs:md2u"),
        clEnumValN(Searcher::NURS_Depth, "nurs:depth", "nurs:depth"),
        clEnumValN(Searcher::NURS_RP, "nurs:rp", "nurs:rp"),
        clEnumValN(Searcher::NURS_ICnt, "nurs:icnt", "nurs:icnt"),
        clEnumValN(Searcher::NURS_CPICnt, "nurs:cpicnt", "nurs:cpicnt"),
        clEnumValN(Searcher::NURS_QC, "nurs:qc", "nurs:qc")),
    cl::init(Searcher::RandomPath), cl::cat(SearchCat));

cl::opt<std::string> PerFunctionAssignmentDump(
    "per-function-assignment-dump",
    cl::desc("If non-empty, dump the resolved per-function assignment to this "
             "file (useful for reproducing per-function-random runs)."),
    cl::init(""), cl::cat(SearchCat));

cl::opt<std::string> PerFunctionRestrictTo(
    "per-function-restrict-to",
    cl::desc("Restrict --search=per-function-random assignment to the "
             "function names listed (one per line, # comments allowed) in "
             "this file. Functions outside the list use "
             "--per-function-default-search."),
    cl::init(""), cl::cat(SearchCat));

cl::opt<bool> UseIterativeDeepeningTimeSearch(
    "use-iterative-deepening-time-search",
    cl::desc(
        "Use iterative deepening time search (experimental) (default=false)"),
    cl::init(false), cl::cat(SearchCat));

cl::opt<bool> UseBatchingSearch(
    "use-batching-search",
    cl::desc("Use batching searcher (keep running selected state for N "
             "instructions/time, see --batch-instructions and --batch-time) "
             "(default=false)"),
    cl::init(false), cl::cat(SearchCat));

cl::opt<unsigned> BatchInstructions(
    "batch-instructions",
    cl::desc("Number of instructions to batch when using "
             "--use-batching-search.  Set to 0 to disable (default=10000)"),
    cl::init(10000), cl::cat(SearchCat));

cl::opt<std::string> BatchTime(
    "batch-time",
    cl::desc("Amount of time to batch when using "
             "--use-batching-search.  Set to 0s to disable (default=5s)"),
    cl::init("5s"), cl::cat(SearchCat));

void initializeSearchOptions() {
  // default values
  if (CoreSearch.empty()) {
    if (UseMerge) {
      CoreSearch.push_back(Searcher::NURS_CovNew);
      klee_warning(
          "--use-merge enabled. Using NURS_CovNew as default searcher.");
    } else {
      CoreSearch.push_back(Searcher::RandomPath);
      CoreSearch.push_back(Searcher::NURS_CovNew);
    }
  }
}

bool userSearcherRequiresMD2U() {
  // Per-function searchers may instantiate any sub-searcher; conservatively
  // require MD2U metadata when one of them is used.
  bool perFunc =
      std::find(CoreSearch.begin(), CoreSearch.end(),
                Searcher::PerFunctionLLM) != CoreSearch.end() ||
      std::find(CoreSearch.begin(), CoreSearch.end(),
                Searcher::PerFunctionRandom) != CoreSearch.end();
  return perFunc ||
         (std::find(CoreSearch.begin(), CoreSearch.end(),
                    Searcher::NURS_MD2U) != CoreSearch.end() ||
          std::find(CoreSearch.begin(), CoreSearch.end(),
                    Searcher::NURS_CovNew) != CoreSearch.end() ||
          std::find(CoreSearch.begin(), CoreSearch.end(),
                    Searcher::NURS_ICnt) != CoreSearch.end() ||
          std::find(CoreSearch.begin(), CoreSearch.end(),
                    Searcher::NURS_CPICnt) != CoreSearch.end() ||
          std::find(CoreSearch.begin(), CoreSearch.end(), Searcher::NURS_QC) !=
              CoreSearch.end());
}

bool userSearcherRequiresInMemoryExecutionTree() {
  // Per-function searchers may instantiate RandomPath as a sub-searcher.
  bool perFunc =
      std::find(CoreSearch.begin(), CoreSearch.end(),
                Searcher::PerFunctionLLM) != CoreSearch.end() ||
      std::find(CoreSearch.begin(), CoreSearch.end(),
                Searcher::PerFunctionRandom) != CoreSearch.end();
  return perFunc || std::find(CoreSearch.begin(), CoreSearch.end(),
                              Searcher::RandomPath) != CoreSearch.end();
}

} // namespace klee

Searcher *getNewSearcher(Searcher::CoreSearchType type, RNG &rng,
                         InMemoryExecutionTree *executionTree) {
  Searcher *searcher = nullptr;
  switch (type) {
  case Searcher::DFS:
    searcher = new DFSSearcher();
    break;
  case Searcher::BFS:
    searcher = new BFSSearcher();
    break;
  case Searcher::RandomState:
    searcher = new RandomSearcher(rng);
    break;
  case Searcher::RandomPath:
    searcher = new RandomPathSearcher(executionTree, rng);
    break;
  case Searcher::NURS_CovNew:
    searcher =
        new WeightedRandomSearcher(WeightedRandomSearcher::CoveringNew, rng);
    break;
  case Searcher::NURS_MD2U:
    searcher = new WeightedRandomSearcher(
        WeightedRandomSearcher::MinDistToUncovered, rng);
    break;
  case Searcher::NURS_Depth:
    searcher = new WeightedRandomSearcher(WeightedRandomSearcher::Depth, rng);
    break;
  case Searcher::NURS_RP:
    searcher = new WeightedRandomSearcher(WeightedRandomSearcher::RP, rng);
    break;
  case Searcher::NURS_ICnt:
    searcher =
        new WeightedRandomSearcher(WeightedRandomSearcher::InstCount, rng);
    break;
  case Searcher::NURS_CPICnt:
    searcher =
        new WeightedRandomSearcher(WeightedRandomSearcher::CPInstCount, rng);
    break;
  case Searcher::NURS_QC:
    searcher =
        new WeightedRandomSearcher(WeightedRandomSearcher::QueryCost, rng);
    break;
  case Searcher::PerFunctionLLM:
  case Searcher::PerFunctionRandom:
    // Constructed in constructUserSearcher() (needs the module).
    return nullptr;
  }

  return searcher;
}

static bool parseSearcherKind(const std::string &s,
                              Searcher::CoreSearchType &out) {
  static const std::pair<const char *, Searcher::CoreSearchType> table[] = {
      {"dfs", Searcher::DFS},
      {"bfs", Searcher::BFS},
      {"random-state", Searcher::RandomState},
      {"random-path", Searcher::RandomPath},
      {"nurs:covnew", Searcher::NURS_CovNew},
      {"nurs:md2u", Searcher::NURS_MD2U},
      {"nurs:depth", Searcher::NURS_Depth},
      {"nurs:rp", Searcher::NURS_RP},
      {"nurs:icnt", Searcher::NURS_ICnt},
      {"nurs:cpicnt", Searcher::NURS_CPICnt},
      {"nurs:qc", Searcher::NURS_QC},
  };
  for (auto &kv : table)
    if (s == kv.first) {
      out = kv.second;
      return true;
    }
  return false;
}

static const char *searcherKindName(Searcher::CoreSearchType t) {
  switch (t) {
  case Searcher::DFS: return "dfs";
  case Searcher::BFS: return "bfs";
  case Searcher::RandomState: return "random-state";
  case Searcher::RandomPath: return "random-path";
  case Searcher::NURS_CovNew: return "nurs:covnew";
  case Searcher::NURS_MD2U: return "nurs:md2u";
  case Searcher::NURS_Depth: return "nurs:depth";
  case Searcher::NURS_RP: return "nurs:rp";
  case Searcher::NURS_ICnt: return "nurs:icnt";
  case Searcher::NURS_CPICnt: return "nurs:cpicnt";
  case Searcher::NURS_QC: return "nurs:qc";
  case Searcher::PerFunctionLLM: return "per-function-llm";
  case Searcher::PerFunctionRandom: return "per-function-random";
  }
  return "?";
}

static Searcher *buildPerFunctionSearcher(Searcher::CoreSearchType variant,
                                          Executor &executor,
                                          InMemoryExecutionTree *etree,
                                          const std::vector<std::string> &candidateFuncs) {
  // 2) Build raw assignment: function-name -> CoreSearchType.
  std::map<std::string, Searcher::CoreSearchType> rawMap;

  if (variant == Searcher::PerFunctionLLM) {
    if (PerFunctionAssignment.empty()) {
      klee_error("--search=per-function-llm requires "
                 "--per-function-assignment=<file>");
    }
    std::ifstream in(PerFunctionAssignment);
    if (!in) {
      klee_error("could not open --per-function-assignment file: %s",
                 PerFunctionAssignment.c_str());
    }
    std::string line;
    unsigned lineno = 0;
    while (std::getline(in, line)) {
      ++lineno;
      // strip leading whitespace
      size_t b = line.find_first_not_of(" \t\r");
      if (b == std::string::npos) continue;
      if (line[b] == '#') continue;
      std::istringstream iss(line);
      std::string fname, kind;
      if (!(iss >> fname >> kind)) {
        klee_warning("per-function-assignment %s:%u: malformed line, skipped",
                     PerFunctionAssignment.c_str(), lineno);
        continue;
      }
      Searcher::CoreSearchType k;
      if (!parseSearcherKind(kind, k)) {
        klee_warning("per-function-assignment %s:%u: unknown searcher \"%s\", "
                     "skipped", PerFunctionAssignment.c_str(), lineno,
                     kind.c_str());
        continue;
      }
      rawMap[fname] = k;
    }
  } else {
    // PerFunctionRandom: uniformly draw a kind for each candidate function.
    static const Searcher::CoreSearchType pool[] = {
        Searcher::DFS,        Searcher::BFS,        Searcher::RandomState,
        Searcher::RandomPath, Searcher::NURS_CovNew, Searcher::NURS_MD2U,
        Searcher::NURS_Depth, Searcher::NURS_RP,    Searcher::NURS_ICnt,
        Searcher::NURS_CPICnt, Searcher::NURS_QC};
    std::mt19937 gen(PerFunctionRandomSeed.getValue());
    std::uniform_int_distribution<unsigned> dist(
        0, sizeof(pool) / sizeof(pool[0]) - 1);

    // Optional restriction set.
    std::set<std::string> restrict;
    if (!PerFunctionRestrictTo.empty()) {
      std::ifstream in(PerFunctionRestrictTo);
      if (!in) {
        klee_error("could not open --per-function-restrict-to file: %s",
                   PerFunctionRestrictTo.c_str());
      }
      std::string line;
      while (std::getline(in, line)) {
        size_t b = line.find_first_not_of(" \t\r");
        if (b == std::string::npos || line[b] == '#') continue;
        std::istringstream iss(line);
        std::string name;
        if (iss >> name) restrict.insert(name);
      }
      klee_message("per-function-random: restricting assignment to %zu "
                   "functions from %s",
                   restrict.size(), PerFunctionRestrictTo.c_str());
    }

    // Iterate over candidate functions in deterministic (declaration) order so
    // the seed produces a reproducible assignment regardless of std::set order.
    for (const auto &fn : candidateFuncs) {
      if (!restrict.empty() && !restrict.count(fn))
        continue;
      rawMap[fn] = pool[dist(gen)];
    }
  }

  // Ensure the configured default kind is represented.
  Searcher::CoreSearchType defaultKind = PerFunctionDefault.getValue();

  // 3) De-duplicate kinds and instantiate one sub-searcher per kind.
  std::map<Searcher::CoreSearchType, unsigned> kindToIdx;
  std::vector<std::unique_ptr<Searcher>> subs;
  std::vector<Searcher::CoreSearchType> subKinds;
  auto addKind = [&](Searcher::CoreSearchType k) -> unsigned {
    auto it = kindToIdx.find(k);
    if (it != kindToIdx.end()) return it->second;
    Searcher *s = getNewSearcher(k, executor.theRNG, etree);
    if (!s)
      klee_error("per-function searcher: cannot nest searcher kind \"%s\"",
                 searcherKindName(k));
    unsigned idx = static_cast<unsigned>(subs.size());
    subs.emplace_back(s);
    subKinds.push_back(k);
    kindToIdx[k] = idx;
    return idx;
  };
  unsigned defaultIdx = addKind(defaultKind);

  std::map<std::string, unsigned> funcToSub;
  for (auto &kv : rawMap)
    funcToSub[kv.first] = addKind(kv.second);

  // 4) Optionally dump the resolved assignment.
  if (!PerFunctionAssignmentDump.empty()) {
    std::ofstream out(PerFunctionAssignmentDump);
    if (!out) {
      klee_warning("could not open --per-function-assignment-dump %s",
                   PerFunctionAssignmentDump.c_str());
    } else {
      out << "# variant=" << searcherKindName(variant)
          << " seed=" << PerFunctionRandomSeed.getValue()
          << " default=" << searcherKindName(defaultKind) << "\n";
      for (auto &kv : rawMap)
        out << kv.first << " " << searcherKindName(kv.second) << "\n";
    }
  }

  klee_message("per-function searcher: variant=%s, %zu functions assigned, "
               "%zu sub-searchers (default=%s)",
               searcherKindName(variant), funcToSub.size(), subs.size(),
               searcherKindName(defaultKind));

  return new PerFunctionSearcher(std::move(subs), std::move(subKinds),
                                 std::move(funcToSub), defaultIdx,
                                 searcherKindName(variant));
}

Searcher *klee::constructUserSearcher(Executor &executor) {
  auto *etree =
      llvm::dyn_cast<InMemoryExecutionTree>(executor.executionTree.get());

  // Collect candidate function names from the module here, where we have
  // friend access to executor.kmodule.
  std::vector<std::string> candidateFuncs;
  if (executor.kmodule && executor.kmodule->module) {
    for (auto &F : *executor.kmodule->module) {
      if (F.isDeclaration())
        continue;
      candidateFuncs.push_back(F.getName().str());
    }
  }

  auto buildOne = [&](Searcher::CoreSearchType t) -> Searcher * {
    if (t == Searcher::PerFunctionLLM || t == Searcher::PerFunctionRandom)
      return buildPerFunctionSearcher(t, executor, etree, candidateFuncs);
    return getNewSearcher(t, executor.theRNG, etree);
  };

  Searcher *searcher = buildOne(CoreSearch[0]);

  if (CoreSearch.size() > 1) {
    std::vector<Searcher *> s;
    s.push_back(searcher);

    for (unsigned i = 1; i < CoreSearch.size(); i++)
      s.push_back(buildOne(CoreSearch[i]));

    searcher = new InterleavedSearcher(s);
  }

  if (UseBatchingSearch) {
    searcher = new BatchingSearcher(searcher, time::Span(BatchTime),
                                    BatchInstructions);
  }

  if (UseIterativeDeepeningTimeSearch) {
    searcher = new IterativeDeepeningTimeSearcher(searcher);
  }

  if (UseMerge) {
    auto *ms = new MergingSearcher(searcher);
    executor.setMergingSearcher(ms);

    searcher = ms;
  }

  llvm::raw_ostream &os = executor.getHandler().getInfoStream();

  os << "BEGIN searcher description\n";
  searcher->printName(os);
  os << "END searcher description\n";

  return searcher;
}
