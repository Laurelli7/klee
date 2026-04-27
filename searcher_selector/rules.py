"""LLVM IR feature extraction for KLEE searcher selection.

Parses LLVM IR text (.ll) files produced by llvm-dis-14 and computes
structural features that map to the 12 searcher selection rules
defined in klee_export/analysis/searcher_selection_rules.md.

The parser is intentionally pure-Python (regex + line scanning) so
it needs no LLVM bindings — we only need the C-compiled bitcode to
be disassembled to text IR.
"""

from __future__ import annotations

import re
from dataclasses import dataclass, field
from typing import Dict, List, Set, Tuple

# ----- IR tokenization ----------------------------------------------------

_FUNC_START = re.compile(r'^\s*define\s+[^@]*@([A-Za-z_][\w.]*)\s*\(([^)]*)\)[^{]*\{')
_BB_LABEL   = re.compile(r'^\s*(\d+|[A-Za-z_][\w.]*):\s*(?:;.*)?$')
_BB_FIRST   = re.compile(r'^\s*(\d+|[A-Za-z_][\w.]*):\s*;\s*preds')

_BR_UNCOND  = re.compile(r'^\s*br\s+label\s+%([A-Za-z_0-9.]+)')
_BR_COND    = re.compile(r'^\s*br\s+i1\s+[^,]+,\s+label\s+%([A-Za-z_0-9.]+),\s+label\s+%([A-Za-z_0-9.]+)')
_SWITCH     = re.compile(r'^\s*switch\s+.*?\[\s*(.*)\s*\]', re.DOTALL)
_SWITCH_DEF = re.compile(r'label\s+%([A-Za-z_0-9.]+)')

_CALL       = re.compile(r'^\s*(?:%[\w.]+\s*=\s*)?(?:tail\s+|musttail\s+)?call\b.*?@([A-Za-z_][\w.]*)\s*\(')

_AND_CONST  = re.compile(r'^\s*%[\w.]+\s*=\s*and\s+i\d+\s+[^,]+,\s+(-?\d+)\b')
_OR_CONST   = re.compile(r'^\s*%[\w.]+\s*=\s*or\s+i\d+\s+[^,]+,\s+(-?\d+)\b')
_XOR_INST   = re.compile(r'^\s*%[\w.]+\s*=\s*xor\s+')
_SHL_INST   = re.compile(r'^\s*%[\w.]+\s*=\s*(shl|lshr|ashr)\s+')
_MUL_INST   = re.compile(r'^\s*%[\w.]+\s*=\s*mul\s+')
_ADD_INST   = re.compile(r'^\s*%[\w.]+\s*=\s*(add|sub)\s+')
_SREM_INST  = re.compile(r'^\s*%[\w.]+\s*=\s*(srem|urem|sdiv|udiv)\s+')

_ICMP       = re.compile(r'^\s*%([\w.]+)\s*=\s*icmp\s+(\w+)\s+i\d+\s+([^,]+),\s+(.+?)$')
_LOAD       = re.compile(r'^\s*%([\w.]+)\s*=\s*load\s+')
_GEP        = re.compile(r'^\s*%([\w.]+)\s*=\s*(?:tail\s+)?getelementptr\s+(?:inbounds\s+)?')

_KLEE_SYM   = re.compile(r'call\s+.*@klee_make_symbolic\s*\(')

# ----- Data model ---------------------------------------------------------

@dataclass
class BasicBlock:
    name: str
    lines: List[str] = field(default_factory=list)
    succs: List[str] = field(default_factory=list)
    preds: List[str] = field(default_factory=list)


@dataclass
class Function:
    name: str
    blocks: Dict[str, BasicBlock] = field(default_factory=dict)
    block_order: List[str] = field(default_factory=list)
    calls: Set[str] = field(default_factory=set)   # callees
    back_edges: List[Tuple[str, str]] = field(default_factory=list)  # loop back edges


@dataclass
class ProgramFeatures:
    # Core structural counts
    bit_test_chain_len: int = 0
    independent_call_regions: int = 0
    has_bitwise_branch_conds: bool = False
    nested_loop_count: int = 0
    symbolic_loop_bound: bool = False
    has_direct_recursion: bool = False
    coverage_blind_score: int = 0
    srem_in_branch: bool = False
    cheap_branch_count: int = 0
    gep_chain_depth: int = 0
    convergent_diverg: int = 0

    # Metadata
    total_functions: int = 0
    total_blocks: int = 0
    total_branches: int = 0
    has_klee_symbolic: bool = False
    dominant_function: str = ""

    def summary_lines(self) -> List[str]:
        return [
            f"  total_functions         = {self.total_functions}",
            f"  total_basic_blocks      = {self.total_blocks}",
            f"  total_branches          = {self.total_branches}",
            f"  bit_test_chain_len      = {self.bit_test_chain_len}  (max sequential bit-tests in any function)",
            f"  independent_call_regions= {self.independent_call_regions}  (distinct callees gated by branches in '{self.dominant_function}')",
            f"  has_bitwise_branch_conds= {self.has_bitwise_branch_conds}",
            f"  nested_loop_count       = {self.nested_loop_count}  (back-edges across all functions)",
            f"  symbolic_loop_bound     = {self.symbolic_loop_bound}",
            f"  has_direct_recursion    = {self.has_direct_recursion}",
            f"  coverage_blind_score    = {self.coverage_blind_score}  (arith-only loop blocks)",
            f"  srem_in_branch          = {self.srem_in_branch}",
            f"  cheap_branch_count      = {self.cheap_branch_count}  (icmp-eq against const)",
            f"  gep_chain_depth         = {self.gep_chain_depth}  (load-indexed-by-load chains)",
            f"  convergent_diverg       = {self.convergent_diverg}  (merge-then-re-branch BBs)",
        ]


# ----- IR parsing ---------------------------------------------------------

def parse_ir(text: str) -> List[Function]:
    """Parse LLVM IR text into a list of Functions with CFG edges."""
    functions: List[Function] = []
    lines = text.splitlines()
    i, n = 0, len(lines)
    current_func: Function | None = None
    current_bb: BasicBlock | None = None

    while i < n:
        line = lines[i]
        # Function start
        m = _FUNC_START.match(line)
        if m and current_func is None:
            current_func = Function(name=m.group(1))
            # entry block has implicit name "0" or first numbered label
            current_bb = BasicBlock(name="entry")
            current_func.blocks["entry"] = current_bb
            current_func.block_order.append("entry")
            i += 1
            continue

        # Function end
        if line.strip() == "}" and current_func is not None:
            functions.append(current_func)
            current_func = None
            current_bb = None
            i += 1
            continue

        if current_func is None:
            i += 1
            continue

        # Basic-block label (e.g., "5:" or "foo:")
        m = _BB_LABEL.match(line)
        if m:
            bb_name = m.group(1)
            # Only switch blocks if the label is followed by instructions,
            # not just a pred comment.
            if bb_name not in current_func.blocks:
                current_bb = BasicBlock(name=bb_name)
                current_func.blocks[bb_name] = current_bb
                current_func.block_order.append(bb_name)
            else:
                current_bb = current_func.blocks[bb_name]
            i += 1
            continue

        # Instruction line — belongs to current basic block
        if current_bb is not None:
            current_bb.lines.append(line)

        # Branch edges
        m = _BR_COND.search(line)
        if m and current_bb is not None:
            current_bb.succs.append(m.group(1))
            current_bb.succs.append(m.group(2))
        else:
            m = _BR_UNCOND.search(line)
            if m and current_bb is not None:
                current_bb.succs.append(m.group(1))
            else:
                m = _SWITCH.search(line)
                if m and current_bb is not None:
                    for lbl in _SWITCH_DEF.findall(m.group(1)):
                        current_bb.succs.append(lbl)

        # Calls
        m = _CALL.search(line)
        if m and current_func is not None:
            current_func.calls.add(m.group(1))

        i += 1

    # Populate predecessors & back-edges (simple DFS)
    for fn in functions:
        for bb_name, bb in fn.blocks.items():
            for s in bb.succs:
                if s in fn.blocks:
                    fn.blocks[s].preds.append(bb_name)
        fn.back_edges = find_back_edges(fn)

    return functions


def find_back_edges(fn: Function) -> List[Tuple[str, str]]:
    """Return CFG back edges using iterative DFS (u → v where v is on the DFS stack)."""
    back: List[Tuple[str, str]] = []
    if not fn.block_order:
        return back
    start = fn.block_order[0]
    color: Dict[str, int] = {}  # 0=white, 1=gray, 2=black
    stack = [(start, iter(fn.blocks[start].succs))]
    color[start] = 1
    while stack:
        node, it = stack[-1]
        try:
            succ = next(it)
        except StopIteration:
            color[node] = 2
            stack.pop()
            continue
        if succ not in fn.blocks:
            continue
        if color.get(succ, 0) == 1:
            back.append((node, succ))
        elif color.get(succ, 0) == 0:
            color[succ] = 1
            stack.append((succ, iter(fn.blocks[succ].succs)))
    return back


# ----- Feature extraction -------------------------------------------------

def _bb_contains(bb: BasicBlock, regex: re.Pattern) -> bool:
    return any(regex.search(l) for l in bb.lines)


def _is_bit_test_block(bb: BasicBlock) -> bool:
    """A block that ends in a conditional branch whose condition is derived
    from an `and <val>, <const>` (a bit-test)."""
    has_and = any(_AND_CONST.match(l) for l in bb.lines)
    has_cond_br = any(_BR_COND.search(l) for l in bb.lines)
    return has_and and has_cond_br


def _has_real_call(bb: BasicBlock) -> bool:
    for l in bb.lines:
        m = _CALL.search(l)
        if not m:
            continue
        callee = m.group(1)
        if callee.startswith(("llvm.", "klee_")):
            continue
        return True
    return False


def _max_bit_test_chain(fn: Function) -> int:
    """Longest chain of consecutive bit-test branches reachable from the
    function entry WITHOUT passing through any non-intrinsic call.

    This captures Rule 2's 'useless fork barrier' — many bit-tests that
    block exploration before any user code gets called. Bit-tests that
    appear AFTER dispatch (e.g., the 'tail' in exp41) don't count.
    """
    if not fn.block_order:
        return 0

    # Mark blocks reachable from entry before any call (the pre-call frontier).
    # Walk forward; stop descending into any block that has a non-intrinsic
    # call — its successors are past the barrier.
    entry = fn.block_order[0]
    pre_call: Set[str] = set()
    stack = [entry]
    while stack:
        name = stack.pop()
        if name in pre_call or name not in fn.blocks:
            continue
        pre_call.add(name)
        bb = fn.blocks[name]
        if _has_real_call(bb):
            # include this block in pre_call (it's still "before" the call
            # in the sense that its branches are still barrier branches) but
            # don't descend past it.
            continue
        for s in bb.succs:
            stack.append(s)

    # Now find the longest path of bit-test blocks within pre_call.
    memo: Dict[str, int] = {}

    def walk(name: str, visiting: Set[str]) -> int:
        if name in memo:
            return memo[name]
        if name in visiting or name not in pre_call:
            return 0
        bb = fn.blocks[name]
        if not _is_bit_test_block(bb) or _has_real_call(bb):
            return 0
        visiting.add(name)
        best = 0
        for s in bb.succs:
            best = max(best, walk(s, visiting))
        visiting.remove(name)
        memo[name] = 1 + best
        return memo[name]

    best_overall = 0
    for name in pre_call:
        best_overall = max(best_overall, walk(name, set()))
    return best_overall


def _count_independent_call_regions(fn: Function) -> int:
    """How many DIFFERENT user-defined callees are dispatched by
    branches within this function? (Rule 4 — wide independent regions.)"""
    callees: Set[str] = set()
    for bb in fn.blocks.values():
        for l in bb.lines:
            m = _CALL.search(l)
            if not m:
                continue
            callee = m.group(1)
            if callee.startswith(("llvm.", "klee_")):
                continue
            callees.add(callee)
    return len(callees)


def _has_bitwise_branch_cond(fn: Function) -> bool:
    """Check if any conditional branch's condition is derived from
    bitwise operations (and/or/xor/shl)."""
    for bb in fn.blocks.values():
        if not any(_BR_COND.search(l) for l in bb.lines):
            continue
        if (_bb_contains(bb, _AND_CONST) or _bb_contains(bb, _OR_CONST)
                or _bb_contains(bb, _XOR_INST) or _bb_contains(bb, _SHL_INST)):
            return True
    return False


def _detect_recursion(functions: List[Function]) -> bool:
    """Direct recursion: any function F calls F."""
    return any(fn.name in fn.calls for fn in functions)


def _symbolic_loop_bound(fn: Function) -> bool:
    """Loop header contains an icmp comparing against a %-register that
    is NOT a constant (it's a load or function parameter)."""
    loop_headers = {dst for (_, dst) in fn.back_edges}
    for hdr in loop_headers:
        if hdr not in fn.blocks:
            continue
        bb = fn.blocks[hdr]
        for l in bb.lines:
            m = _ICMP.match(l)
            if not m:
                continue
            lhs, rhs = m.group(3).strip(), m.group(4).strip()
            # if either side references a %register (not just a constant int) → symbolic
            if lhs.startswith('%') and rhs.startswith('%'):
                return True
    return False


def _coverage_blind_score(fn: Function) -> int:
    """Count loop-body blocks that are 'coverage-blind' — dominated by
    arithmetic / bitwise ops and lack new coverage-relevant branching."""
    if not fn.back_edges:
        return 0
    # Approximate loop body = all blocks reachable from a back-edge tail,
    # going back to its head.
    loop_bodies: Set[str] = set()
    for tail, head in fn.back_edges:
        loop_bodies.add(tail)
        loop_bodies.add(head)
    score = 0
    for name in loop_bodies:
        if name not in fn.blocks:
            continue
        bb = fn.blocks[name]
        arith = sum(1 for l in bb.lines if _XOR_INST.match(l) or _SHL_INST.match(l)
                    or _AND_CONST.match(l) or _OR_CONST.match(l)
                    or _MUL_INST.match(l) or _ADD_INST.match(l))
        has_new_branch_cond = any(_ICMP.match(l) for l in bb.lines)
        if arith >= 2 and not has_new_branch_cond:
            score += 1
    return score


def _srem_in_branch(fn: Function) -> bool:
    """Does any block use srem/urem and end in a conditional branch?"""
    for bb in fn.blocks.values():
        if _bb_contains(bb, _SREM_INST) and any(_BR_COND.search(l) for l in bb.lines):
            return True
    return False


def _cheap_branch_count(fn: Function) -> int:
    """Count blocks with icmp eq <reg>, <const> → br  (cheap equality)."""
    count = 0
    for bb in fn.blocks.values():
        has_cheap = False
        for l in bb.lines:
            m = _ICMP.match(l)
            if m and m.group(2) in ("eq", "ne"):
                rhs = m.group(4).strip()
                # literal integer RHS
                if re.match(r'^-?\d+', rhs):
                    has_cheap = True
                    break
        if has_cheap and any(_BR_COND.search(l) for l in bb.lines):
            count += 1
    return count


def _gep_chain_depth(fn: Function) -> int:
    """Depth of GEP → load → GEP chains (R12: symbolic pointer chases)."""
    load_regs: Set[str] = set()
    gep_regs: Set[str] = set()
    for bb in fn.blocks.values():
        for l in bb.lines:
            m = _LOAD.match(l)
            if m:
                load_regs.add(m.group(1))
            m = _GEP.match(l)
            if m:
                gep_regs.add(m.group(1))
    # Heuristic: count GEP instructions whose address operand is a previously
    # loaded register. Simple proxy: number of GEPs that follow a load in the
    # same block, capped.
    chain = 0
    for bb in fn.blocks.values():
        saw_load = False
        for l in bb.lines:
            if _LOAD.match(l):
                saw_load = True
            elif _GEP.match(l) and saw_load:
                chain += 1
                saw_load = False
    return chain


def _convergent_divergent(fn: Function) -> int:
    """Count blocks with ≥2 preds AND ≥2 succs (merge-then-re-branch)."""
    return sum(1 for bb in fn.blocks.values() if len(bb.preds) >= 2 and len(bb.succs) >= 2)


def compute_features(functions: List[Function]) -> ProgramFeatures:
    f = ProgramFeatures()
    f.total_functions = len(functions)
    f.total_blocks = sum(len(fn.blocks) for fn in functions)

    # Pick the most interesting function — typically 'main' or the largest.
    target = None
    for fn in functions:
        if fn.name == "main":
            target = fn
            break
    if target is None and functions:
        target = max(functions, key=lambda fn: len(fn.blocks))
    if target is None:
        return f

    f.dominant_function = target.name
    f.total_branches = sum(
        1 for bb in target.blocks.values() for l in bb.lines if _BR_COND.search(l)
    )
    f.has_klee_symbolic = any(
        _KLEE_SYM.search(l) for fn in functions for bb in fn.blocks.values() for l in bb.lines
    )

    f.bit_test_chain_len       = _max_bit_test_chain(target)
    f.independent_call_regions = _count_independent_call_regions(target)
    f.has_bitwise_branch_conds = _has_bitwise_branch_cond(target)
    f.nested_loop_count        = sum(len(fn.back_edges) for fn in functions)
    f.symbolic_loop_bound      = any(_symbolic_loop_bound(fn) for fn in functions)
    f.has_direct_recursion     = _detect_recursion(functions)
    f.coverage_blind_score     = sum(_coverage_blind_score(fn) for fn in functions)
    f.srem_in_branch           = any(_srem_in_branch(fn) for fn in functions)
    f.cheap_branch_count       = _cheap_branch_count(target)
    f.gep_chain_depth          = max((_gep_chain_depth(fn) for fn in functions), default=0)
    f.convergent_diverg        = _convergent_divergent(target)
    return f


# ----- Rule application ---------------------------------------------------

def select_searcher(f: ProgramFeatures) -> Tuple[List[str], str]:
    """Return (list-of-klee-flags, human-readable-explanation).

    Rule priority (from searcher_selection_rules.md meta-rule M8):
        R9 > R1/R2 > R4 > R11 > R7/R8 > R6 > R12 > default NURS
    """
    # R9: coverage-blind computation (all paths run same insts, differ in values)
    if f.coverage_blind_score >= 2 and f.nested_loop_count >= 1:
        return (["--search=dfs"],
                f"R9 (coverage-blind computation): {f.coverage_blind_score} "
                f"arithmetic-only loop blocks — DFS commits to one path "
                f"through identical code, avoiding coverage-heuristic overhead")

    # R2: useless fork barrier (many symbolic branches before useful code)
    if f.bit_test_chain_len >= 8:
        return (["--search=dfs"],
                f"R2 (useless fork barrier): chain of {f.bit_test_chain_len} "
                f"bit-test branches before unique code — DFS picks one path, "
                f"others drown in 2^{f.bit_test_chain_len} pending states")

    # R1: sequential gating chain with no dispatch
    if f.bit_test_chain_len >= 4 and f.independent_call_regions == 0:
        return (["--search=dfs"],
                f"R1 (sequential gating): {f.bit_test_chain_len} sequential "
                f"checks before target code — DFS pursues one validated path")

    # R4 variants: wide independent regions
    if f.independent_call_regions >= 4:
        if f.has_bitwise_branch_conds and f.nested_loop_count < 2:
            return (["--search=nurs:covnew"],
                    f"R4b (bitwise flag decoding): {f.independent_call_regions} "
                    f"regions gated by bitwise conditions — covnew specifically "
                    f"wins on this pattern")
        if f.nested_loop_count >= 2 and f.independent_call_regions >= 4:
            return (["--search=random-path"],
                    f"R4c (nested scanner): {f.nested_loop_count} loops with "
                    f"{f.independent_call_regions} dispatch regions — RP samples "
                    f"the 2D search space uniformly")
        if f.independent_call_regions >= 6:
            return (["--search=random-path"],
                    f"R4a (multi-dimensional dispatch): {f.independent_call_regions} "
                    f"independent regions — RP distributes across dispatch points")
        return (["--search=nurs:covnew"],
                f"R4 (wide independent regions): {f.independent_call_regions} "
                f"callees behind independent branches — covnew prioritises "
                f"unvisited regions")

    # R11: constraint cost asymmetry
    if f.srem_in_branch and f.cheap_branch_count >= 2:
        return (["--search=nurs:qc"],
                f"R11 (constraint cost asymmetry): modular arithmetic branches "
                f"alongside {f.cheap_branch_count} cheap equality branches — "
                f"query-cost weighting prefers solvable paths first")

    # R8: recursion takes precedence over R7 when both present
    if f.has_direct_recursion:
        return (["--search=bfs"],
                "R8 (recursive combinatorial branching): BFS reaches all "
                "leaves at depth 1 before descending further")

    # R7: symbolic loop bound
    if f.symbolic_loop_bound and f.coverage_blind_score == 0:
        return (["--search=bfs"],
                "R7 (symbolic loop bound): value-dependent coverage per "
                "iteration — BFS explores all iteration counts uniformly")

    # R6: convergent-divergent flow
    if f.convergent_diverg >= 2:
        return (["--search=nurs:covnew"],
                f"R6 (convergent-divergent flow): {f.convergent_diverg} "
                f"merge-then-re-branch blocks — covnew re-evaluates coverage "
                f"signals after each merge")

    # R12: pointer chains
    if f.gep_chain_depth >= 2:
        return (["--search=random-path"],
                f"R12 (symbolic pointer chain): chain depth {f.gep_chain_depth} "
                f"— random-path samples independent chain prefixes")

    # Default: KLEE's proven baseline
    return (["--search=random-path", "--search=nurs:covnew"],
            "default: no strong structural signal — using KLEE's baseline "
            "interleaved random-path + nurs:covnew")
