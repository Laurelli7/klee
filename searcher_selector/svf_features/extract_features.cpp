//===- extract_features.cpp -- SVF-based feature extractor for KLEE searcher
// selection ------------------------------------------------------------===//
//
// Reads an LLVM bitcode (.bc) module and prints a JSON object with the 12
// structural features defined in
// `klee_export/analysis/searcher_selection_rules.md`.
//
// Replaces the regex-based parser in `searcher_selector/rules.py` with a
// real SVF + LLVM analysis:
//   * Loops & dominators come from SVFLoopAndDomInfo (LLVM LoopInfo under
//     the hood) — populated automatically when SVFIR is built.
//   * Call graph (and recursion / call-region count) comes from
//     PTACallGraph built by Andersen's pointer analysis, so indirect
//     calls are resolved.
//   * Per-instruction pattern matching (bit-tests, srem-in-branch,
//     icmp-eq-const, GEP-after-load, ...) is done on the LLVM IR
//     reached via LLVMModuleSet::getLLVMValue(...).
//
// Output: a single JSON object on stdout.
//===----------------------------------------------------------------------===//

#include "SVF-LLVM/LLVMUtil.h"
#include "SVF-LLVM/LLVMModule.h"
#include "SVF-LLVM/SVFIRBuilder.h"
#include "SVFIR/SVFIR.h"
#include "Graphs/ICFG.h"
#include "Graphs/PTACallGraph.h"
#include "WPA/Andersen.h"
#include "Util/CommandLine.h"
#include "Util/Options.h"

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Module.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using namespace llvm;
using namespace SVF;

namespace {

// ----- Predicates over LLVM IR ------------------------------------------

bool isUserCall(const Instruction &I) {
  const auto *CB = dyn_cast<CallBase>(&I);
  if (!CB) return false;
  const Function *Callee = CB->getCalledFunction();
  if (!Callee) return true;        // indirect call: treat as user call
  StringRef N = Callee->getName();
  if (N.startswith("llvm.") || N.startswith("klee_")) return false;
  if (Callee->isIntrinsic()) return false;
  return true;
}

bool bbHasUserCall(const BasicBlock &BB) {
  for (const Instruction &I : BB)
    if (isUserCall(I)) return true;
  return false;
}

bool bbEndsCondBranch(const BasicBlock &BB) {
  const auto *Term = BB.getTerminator();
  if (const auto *Br = dyn_cast<BranchInst>(Term))
    return Br->isConditional();
  if (isa<SwitchInst>(Term)) return true;
  return false;
}

// Does this block contain `and reg, <const>`?  Bit-test marker for R1/R2.
bool bbHasAndConst(const BasicBlock &BB) {
  for (const Instruction &I : BB) {
    if (I.getOpcode() == Instruction::And) {
      if (isa<ConstantInt>(I.getOperand(1)) || isa<ConstantInt>(I.getOperand(0)))
        return true;
    }
  }
  return false;
}

bool isBitwiseOp(unsigned op) {
  return op == Instruction::And || op == Instruction::Or ||
         op == Instruction::Xor || op == Instruction::Shl ||
         op == Instruction::LShr || op == Instruction::AShr;
}

bool isArithOp(unsigned op) {
  return isBitwiseOp(op) || op == Instruction::Add || op == Instruction::Sub ||
         op == Instruction::Mul;
}

bool isModOrDiv(unsigned op) {
  return op == Instruction::SRem || op == Instruction::URem ||
         op == Instruction::SDiv || op == Instruction::UDiv;
}

// Pick the "dominant" function across all loaded modules: prefer `main`,
// else the largest by BB count.
const Function *pickDominant(
    const std::vector<std::reference_wrapper<Module>> &mods) {
  for (auto &mref : mods)
    if (Function *Main = mref.get().getFunction("main"))
      if (!Main->isDeclaration()) return Main;
  const Function *Best = nullptr;
  size_t BestSz = 0;
  for (auto &mref : mods)
    for (Function &F : mref.get()) {
      if (F.isDeclaration()) continue;
      if (F.getName().startswith("llvm.") || F.getName().startswith("klee_"))
        continue;
      size_t sz = F.size();
      if (sz > BestSz) { BestSz = sz; Best = &F; }
    }
  return Best;
}

// ----- Per-feature analyses ---------------------------------------------

// R1/R2: longest chain of bit-test (and-const + cond-br) blocks reachable
// from entry of `F` BEFORE any non-intrinsic call (the "useless fork
// barrier" pattern).  Memoized DFS on the LLVM CFG, with cycle protection.
int maxBitTestChain(const Function &F) {
  // 1) Compute the pre-call frontier: BBs reachable from entry; do not
  //    descend past any block that contains a real call.
  std::unordered_set<const BasicBlock *> preCall;
  std::vector<const BasicBlock *> stack{&F.getEntryBlock()};
  while (!stack.empty()) {
    const BasicBlock *BB = stack.back(); stack.pop_back();
    if (!preCall.insert(BB).second) continue;
    if (bbHasUserCall(*BB)) continue;
    for (const BasicBlock *S : successors(BB)) stack.push_back(S);
  }

  // 2) Longest chain of bit-test blocks within the frontier.
  std::unordered_map<const BasicBlock *, int> memo;
  std::function<int(const BasicBlock *, std::unordered_set<const BasicBlock *> &)> walk;
  walk = [&](const BasicBlock *BB,
             std::unordered_set<const BasicBlock *> &visiting) -> int {
    auto it = memo.find(BB);
    if (it != memo.end()) return it->second;
    if (visiting.count(BB)) return 0;
    if (!preCall.count(BB)) return 0;
    if (!bbHasAndConst(*BB) || !bbEndsCondBranch(*BB) || bbHasUserCall(*BB))
      return 0;
    visiting.insert(BB);
    int best = 0;
    for (const BasicBlock *S : successors(BB))
      best = std::max(best, walk(S, visiting));
    visiting.erase(BB);
    memo[BB] = 1 + best;
    return memo[BB];
  };

  int best = 0;
  for (const BasicBlock *BB : preCall) {
    std::unordered_set<const BasicBlock *> visiting;
    best = std::max(best, walk(BB, visiting));
  }
  return best;
}

// R4: distinct user callees in F (intra-procedural fan-out).  Indirect
// calls are resolved via the PTA call graph.
int countIndependentCallRegions(const Function &F, PTACallGraph &CG) {
  std::unordered_set<const Function *> callees;
  const SVFFunction *svfF =
      LLVMModuleSet::getLLVMModuleSet()->getSVFFunction(&F);
  PTACallGraphNode *Node = CG.getCallGraphNode(svfF);
  if (Node) {
    for (auto eit = Node->OutEdgeBegin(); eit != Node->OutEdgeEnd(); ++eit) {
      const SVFFunction *callee = (*eit)->getDstNode()->getFunction();
      if (!callee || callee == svfF) continue;
      const Value *V = LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(callee);
      const Function *FF = dyn_cast_or_null<Function>(V);
      if (!FF) continue;
      StringRef N = FF->getName();
      if (N.startswith("llvm.") || N.startswith("klee_")) continue;
      if (FF->isIntrinsic()) continue;
      callees.insert(FF);
    }
  }
  return static_cast<int>(callees.size());
}

bool hasBitwiseBranchCond(const Function &F) {
  for (const BasicBlock &BB : F) {
    if (!bbEndsCondBranch(BB)) continue;
    for (const Instruction &I : BB)
      if (isBitwiseOp(I.getOpcode())) return true;
  }
  return false;
}

// Direct + simple mutual recursion (one-hop) via PTACallGraph SCC.
bool hasRecursion(SVFModule &M, PTACallGraph &CG) {
  for (const SVFFunction *F : M) {
    PTACallGraphNode *N = CG.getCallGraphNode(F);
    if (!N) continue;
    // direct self-call?
    for (auto eit = N->OutEdgeBegin(); eit != N->OutEdgeEnd(); ++eit) {
      if ((*eit)->getDstNode() == N) return true;
    }
    // 1-hop mutual recursion?
    for (auto eit = N->OutEdgeBegin(); eit != N->OutEdgeEnd(); ++eit) {
      PTACallGraphNode *M2 = (*eit)->getDstNode();
      if (M2 == N) continue;
      for (auto eit2 = M2->OutEdgeBegin(); eit2 != M2->OutEdgeEnd(); ++eit2)
        if ((*eit2)->getDstNode() == N) return true;
    }
  }
  return false;
}

// R7: any loop header whose icmp compares two non-constants (proxy for
// "bound is symbolic / loaded").
bool hasSymbolicLoopBound(const Function &F) {
  SVFFunction *svfF =
      LLVMModuleSet::getLLVMModuleSet()->getSVFFunction(&F);
  SVFLoopAndDomInfo *LD = svfF->getLoopAndDomInfo();
  // Collect unique loop headers from bb2LoopMap.  Each loop's header is
  // the first BB in its LoopBBs vector.
  std::unordered_set<const BasicBlock *> headers;
  for (const BasicBlock &BB : F) {
    const SVFBasicBlock *svfBB =
        LLVMModuleSet::getLLVMModuleSet()->getSVFBasicBlock(&BB);
    if (!LD->hasLoopInfo(svfBB)) continue;
    const auto &lp = LD->getLoopInfo(svfBB);
    if (lp.empty()) continue;
    const SVFBasicBlock *hdrSVF = LD->getLoopHeader(lp);
    const Value *V =
        LLVMModuleSet::getLLVMModuleSet()->getLLVMValue(hdrSVF);
    if (const auto *HBB = dyn_cast_or_null<BasicBlock>(V))
      headers.insert(HBB);
  }
  for (const BasicBlock *H : headers) {
    for (const Instruction &I : *H) {
      const auto *CMP = dyn_cast<ICmpInst>(&I);
      if (!CMP) continue;
      bool lhsConst = isa<Constant>(CMP->getOperand(0));
      bool rhsConst = isa<Constant>(CMP->getOperand(1));
      if (!lhsConst && !rhsConst) return true;
    }
  }
  return false;
}

// Total unique loop headers across all functions (R-series "nested loop"
// signal — same as Python's `nested_loop_count = #back_edges`).
int countLoopHeaders(
    const std::vector<std::reference_wrapper<Module>> &mods) {
  int total = 0;
  auto *LMS = LLVMModuleSet::getLLVMModuleSet();
  for (auto &mref : mods)
    for (Function &F : mref.get()) {
      if (F.isDeclaration()) continue;
      SVFFunction *svfF = LMS->getSVFFunction(&F);
      SVFLoopAndDomInfo *LD = svfF->getLoopAndDomInfo();
      std::unordered_set<const SVFBasicBlock *> headers;
      for (const SVFBasicBlock *BB : svfF->getBasicBlockList()) {
        if (!LD->hasLoopInfo(BB)) continue;
        const auto &lp = LD->getLoopInfo(BB);
        if (lp.empty()) continue;
        headers.insert(LD->getLoopHeader(lp));
      }
      total += static_cast<int>(headers.size());
    }
  return total;
}

// R9: BBs inside any loop body that are arithmetic-heavy and contain no
// new branch condition.  Summed across all functions.
int coverageBlindScore(const Function &F) {
  SVFFunction *svfF =
      LLVMModuleSet::getLLVMModuleSet()->getSVFFunction(&F);
  SVFLoopAndDomInfo *LD = svfF->getLoopAndDomInfo();
  int score = 0;
  for (const BasicBlock &BB : F) {
    const SVFBasicBlock *svfBB =
        LLVMModuleSet::getLLVMModuleSet()->getSVFBasicBlock(&BB);
    if (!LD->hasLoopInfo(svfBB)) continue;
    int arith = 0;
    bool hasICmp = false;
    for (const Instruction &I : BB) {
      if (isArithOp(I.getOpcode())) ++arith;
      if (isa<ICmpInst>(I)) hasICmp = true;
    }
    if (arith >= 2 && !hasICmp) ++score;
  }
  return score;
}

// R11: any BB containing srem/urem/sdiv/udiv whose terminator is a
// conditional branch.
bool sremInBranch(const Function &F) {
  for (const BasicBlock &BB : F) {
    if (!bbEndsCondBranch(BB)) continue;
    for (const Instruction &I : BB)
      if (isModOrDiv(I.getOpcode())) return true;
  }
  return false;
}

// R11 helper: cheap equality branches (icmp eq/ne reg, <int-const>).
int cheapBranchCount(const Function &F) {
  int cnt = 0;
  for (const BasicBlock &BB : F) {
    if (!bbEndsCondBranch(BB)) continue;
    bool cheap = false;
    for (const Instruction &I : BB) {
      const auto *CMP = dyn_cast<ICmpInst>(&I);
      if (!CMP) continue;
      auto p = CMP->getPredicate();
      if (p != CmpInst::ICMP_EQ && p != CmpInst::ICMP_NE) continue;
      if (isa<ConstantInt>(CMP->getOperand(1)) ||
          isa<ConstantInt>(CMP->getOperand(0))) {
        cheap = true; break;
      }
    }
    if (cheap) ++cnt;
  }
  return cnt;
}

// R12: GEP whose base pointer is the result of a Load (= "indirect via
// loaded value") — proxy for symbolic pointer chains.  Counted across F.
int gepChainDepth(const Function &F) {
  int chain = 0;
  for (const BasicBlock &BB : F) {
    for (const Instruction &I : BB) {
      const auto *GEP = dyn_cast<GetElementPtrInst>(&I);
      if (!GEP) continue;
      const Value *Base = GEP->getPointerOperand()->stripPointerCasts();
      if (isa<LoadInst>(Base)) ++chain;
      // also: GEP whose any *index* is a loaded value
      else {
        for (auto it = GEP->idx_begin(); it != GEP->idx_end(); ++it) {
          if (isa<LoadInst>((*it)->stripPointerCasts())) { ++chain; break; }
        }
      }
    }
  }
  return chain;
}

// R6: BBs with ≥2 predecessors AND ≥2 successors (merge-then-re-fork).
int convergentDivergent(const Function &F) {
  int n = 0;
  for (const BasicBlock &BB : F) {
    int preds = 0;
    for (auto it = pred_begin(&BB); it != pred_end(&BB); ++it) ++preds;
    int succs = BB.getTerminator()->getNumSuccessors();
    if (preds >= 2 && succs >= 2) ++n;
  }
  return n;
}

// Misc counts on the dominant function.
int countCondBranches(const Function &F) {
  int n = 0;
  for (const BasicBlock &BB : F)
    if (bbEndsCondBranch(BB)) ++n;
  return n;
}

bool moduleHasKleeSymbolic(Module &M) {
  for (Function &F : M)
    for (BasicBlock &BB : F)
      for (Instruction &I : BB)
        if (const auto *CB = dyn_cast<CallBase>(&I))
          if (const Function *C = CB->getCalledFunction())
            if (C->getName() == "klee_make_symbolic") return true;
  return false;
}

// ----- Tiny JSON emitter ------------------------------------------------

std::string jq(const std::string &s) {
  std::string out = "\"";
  for (char c : s) {
    switch (c) {
      case '"':  out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n";  break;
      case '\r': out += "\\r";  break;
      case '\t': out += "\\t";  break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[8]; std::snprintf(buf, sizeof(buf), "\\u%04x", c);
          out += buf;
        } else out += c;
    }
  }
  out += "\"";
  return out;
}

} // anonymous namespace

int main(int argc, char **argv) {
  std::vector<std::string> moduleNames =
      OptionBase::parseOptions(argc, argv, "KLEE searcher feature extractor",
                               "[options] <input-bitcode>");
  if (moduleNames.empty()) {
    std::cerr << "usage: extract_features <input.bc>\n";
    return 1;
  }

  // Build SVFIR (this also populates SVFLoopAndDomInfo for every fn).
  SVFModule *svfModule = LLVMModuleSet::buildSVFModule(moduleNames);
  SVFIRBuilder builder(svfModule);
  SVFIR *pag = builder.build();

  // Andersen → call graph (resolves indirect calls).
  Andersen *ander = AndersenWaveDiff::createAndersenWaveDiff(pag);
  PTACallGraph *CG = ander->getPTACallGraph();

  // Get the underlying LLVM Modules.  SVF may load extapi.bc alongside
  // the user input, so iterate all of them.
  auto &mods = LLVMModuleSet::getLLVMModuleSet()->getLLVMModules();
  if (mods.empty()) { std::cerr << "no LLVM modules loaded\n"; return 2; }

  const Function *Dom = pickDominant(mods);
  if (!Dom) { std::cerr << "no defined function found\n"; return 3; }
  Module &M = const_cast<Module &>(*Dom->getParent());

  // ---- Compute features ------------------------------------------------
  int bit_test_chain_len     = maxBitTestChain(*Dom);
  int independent_call_regs  = countIndependentCallRegions(*Dom, *CG);
  bool bitwise_branch_conds  = hasBitwiseBranchCond(*Dom);
  int nested_loop_count      = countLoopHeaders(mods);
  bool symbolic_loop_bound   = false;
  int coverage_blind_score   = 0;
  bool srem_in_branch_any    = false;
  int gep_chain_depth_max    = 0;
  for (auto &mref : mods) {
    for (Function &F : mref.get()) {
      if (F.isDeclaration()) continue;
      if (hasSymbolicLoopBound(F))     symbolic_loop_bound  = true;
      if (sremInBranch(F))             srem_in_branch_any   = true;
      coverage_blind_score            += coverageBlindScore(F);
      gep_chain_depth_max  = std::max(gep_chain_depth_max, gepChainDepth(F));
    }
  }
  bool direct_recursion      = hasRecursion(*svfModule, *CG);
  int cheap_branch_count_v   = cheapBranchCount(*Dom);
  int convergent_diverg      = convergentDivergent(*Dom);

  int total_functions = 0;
  int total_blocks    = 0;
  bool has_klee_sym   = false;
  for (auto &mref : mods) {
    Module &MM = mref.get();
    if (moduleHasKleeSymbolic(MM)) has_klee_sym = true;
    for (Function &F : MM) {
      if (F.isDeclaration()) continue;
      ++total_functions;
      total_blocks += static_cast<int>(F.size());
    }
  }
  int total_branches  = countCondBranches(*Dom);
  std::string dom_name = Dom->getName().str();

  // ---- Emit JSON -------------------------------------------------------
  std::ostream &os = std::cout;
  os << "{\n";
  os << "  \"source\":              \"svf\",\n";
  os << "  \"input\":               " << jq(moduleNames.front()) << ",\n";
  os << "  \"dominant_function\":   " << jq(dom_name) << ",\n";
  os << "  \"total_functions\":     " << total_functions << ",\n";
  os << "  \"total_blocks\":        " << total_blocks << ",\n";
  os << "  \"total_branches\":      " << total_branches << ",\n";
  os << "  \"has_klee_symbolic\":   " << (has_klee_sym ? "true" : "false") << ",\n";
  os << "  \"bit_test_chain_len\":  " << bit_test_chain_len << ",\n";
  os << "  \"independent_call_regions\": " << independent_call_regs << ",\n";
  os << "  \"has_bitwise_branch_conds\": " << (bitwise_branch_conds ? "true" : "false") << ",\n";
  os << "  \"nested_loop_count\":   " << nested_loop_count << ",\n";
  os << "  \"symbolic_loop_bound\": " << (symbolic_loop_bound ? "true" : "false") << ",\n";
  os << "  \"has_direct_recursion\":" << (direct_recursion ? "true" : "false") << ",\n";
  os << "  \"coverage_blind_score\":" << coverage_blind_score << ",\n";
  os << "  \"srem_in_branch\":      " << (srem_in_branch_any ? "true" : "false") << ",\n";
  os << "  \"cheap_branch_count\":  " << cheap_branch_count_v << ",\n";
  os << "  \"gep_chain_depth\":     " << gep_chain_depth_max << ",\n";
  os << "  \"convergent_diverg\":   " << convergent_diverg;

  // Per-function block: triggered by env var SVF_PER_FUNCTION=1.
  // Emits feature record per defined function so chunk_planner can apply
  // the rule cascade per-function.
  const char *perFn = std::getenv("SVF_PER_FUNCTION");
  if (perFn && perFn[0] == '1') {
    os << ",\n  \"per_function\": {";
    bool first = true;
    for (auto &mref : mods) {
      Module &MM = mref.get();
      for (Function &F : MM) {
        if (F.isDeclaration()) continue;
        llvm::StringRef nm = F.getName();
        if (nm.startswith("llvm.") || nm.startswith("klee_")) continue;
        if (!first) os << ",";
        first = false;
        int f_btc   = maxBitTestChain(F);
        int f_icr   = countIndependentCallRegions(F, *CG);
        bool f_bbc  = hasBitwiseBranchCond(F);
        int f_nlc   = 0;
        {
          SVFFunction *svfF = LLVMModuleSet::getLLVMModuleSet()->getSVFFunction(&F);
          if (svfF) {
            SVFLoopAndDomInfo *LD = svfF->getLoopAndDomInfo();
            if (LD) {
              std::unordered_set<const SVFBasicBlock *> headers;
              for (const SVFBasicBlock *BB : svfF->getBasicBlockList()) {
                if (!LD->hasLoopInfo(BB)) continue;
                const auto &lp = LD->getLoopInfo(BB);
                if (lp.empty()) continue;
                headers.insert(LD->getLoopHeader(lp));
              }
              f_nlc = static_cast<int>(headers.size());
            }
          }
        }
        bool f_slb  = hasSymbolicLoopBound(F);
        int f_cbs   = coverageBlindScore(F);
        bool f_srem = sremInBranch(F);
        int f_cbr   = cheapBranchCount(F);
        int f_gep   = gepChainDepth(F);
        int f_cnv   = convergentDivergent(F);
        int f_brn   = countCondBranches(F);
        int f_bb    = static_cast<int>(F.size());
        bool f_rec  = false; // recursion is a global property; leave false
        os << "\n    " << jq(nm.str()) << ": {"
           << "\"total_blocks\":" << f_bb
           << ",\"total_branches\":" << f_brn
           << ",\"bit_test_chain_len\":" << f_btc
           << ",\"independent_call_regions\":" << f_icr
           << ",\"has_bitwise_branch_conds\":" << (f_bbc ? "true" : "false")
           << ",\"nested_loop_count\":" << f_nlc
           << ",\"symbolic_loop_bound\":" << (f_slb ? "true" : "false")
           << ",\"has_direct_recursion\":" << (f_rec ? "true" : "false")
           << ",\"coverage_blind_score\":" << f_cbs
           << ",\"srem_in_branch\":" << (f_srem ? "true" : "false")
           << ",\"cheap_branch_count\":" << f_cbr
           << ",\"gep_chain_depth\":" << f_gep
           << ",\"convergent_diverg\":" << f_cnv
           << "}";
      }
    }
    os << "\n  }";
  }
  os << "\n}\n";

  AndersenWaveDiff::releaseAndersenWaveDiff();
  SVFIR::releaseSVFIR();
  LLVMModuleSet::releaseLLVMModuleSet();
  return 0;
}
