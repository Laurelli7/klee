# KLEE Searcher Selection: Structural Rules Policy

Version: 1.0 (seed from 123 hand-crafted + 484 CodeContests + 28 real-world structural units)
Accuracy: 62.5% on discriminating real-world patterns. Non-discriminating is common (57% of real programs).

Searchers: `dfs`, `bfs`, `random-path`, `nurs:covnew`, `nurs:md2u`, `nurs:qc`
Default (no strong signal): `random-path` + `nurs:covnew` interleaved

---

## Priority Order

When multiple rules fire, use the highest-priority match:

1. R9  — coverage-blind computation → DFS
2. R2  — useless fork barrier → DFS
3. R1  — sequential gating → DFS
4. R3  — symbolic→concrete sweep → DFS
5. R4  — wide independent regions → NURS or RP (see sub-rules)
6. R10 — identical loop body → avoid NURS (use DFS/BFS/RP)
7. R8  — recursive combinatorial explosion → BFS
8. R7  — symbolic loop bound → BFS
9. R11 — constraint cost asymmetry → nurs:qc
10. R6  — convergent-divergent flow → nurs:covnew
11. R12 — symbolic pointer/index chain → random-path
12. default → random-path + nurs:covnew

---

## Rules

### R1: Sequential Gating — DFS

**Trigger**: Target code is reachable only by passing N sequential symbolic checks on the same path. Each check is a fork; you must take the "pass" branch at every fork to reach the target.

**Threshold**: N ≥ 15 simple binary forks, OR N ≥ 6 forks where each has ≥ 3 arms, OR forks with expensive constraints.

**Detection**:
- Long chain of diamonds in series (each with pass/fail edges)
- Sequential if-else on same variable, sorting network, linked-list traversal
- `bit_test_chain_len ≥ 8` (proxy: consecutive bit-tests before any call)
- `sequential_gate_depth ≥ 15`

**Verdict**: `dfs`

**Why**: DFS commits to one path and follows it to completion. Others fork at every gate and accumulate 2^N pending states without completing the chain.

**Caveat**: Rule does NOT apply if the chain is short enough to exhaust (< 15 simple gates). All searchers will tie.

---

### R2: Useless Fork Barrier — DFS

**Trigger**: Many symbolic branches that produce no new coverage sit BEFORE the useful target code in topological order.

**Threshold**: ≥ 8 bit-test / noise branches before first useful dispatch call.

**Detection**:
- `bit_test_chain_len ≥ 8`
- Large state count with no new coverage discovery in the barrier region
- The barrier branches must come BEFORE target code in CFG order

**Verdict**: `dfs`

**Why**: DFS picks one path through the useless region and reaches target code quickly. Other searchers spend their entire budget managing exponentially many pending states in the barrier.

---

### R3: Symbolic Setup → Concrete Sweep — DFS

**Trigger**: Very few symbolic variables (1–5), followed by a large concrete loop (100K+ iterations) whose path depends on which concrete values were chosen. Different symbolic choices lead to different loop bodies covering different instructions.

**Threshold**: `symbolic_var_count ≤ 5` AND `concrete_loop_iterations ≥ 100000`

**Detection**:
- Few klee_make_symbolic calls
- Dominant loop bound is concrete (not a symbolic variable)
- DFS makes 10–100× more queries than BFS in same time (racing through concrete code)

**Verdict**: `dfs`

**Why**: DFS picks one assignment and runs the full sweep, completing the trace. Others fork on symbolic variables and start multiple sweeps that none finishes within the budget.

---

### R4: Wide Independent Regions — NURS or RP

**Trigger**: Program branches into N ≥ 4 independent code regions at the top level, each covering different instructions with no data dependency between regions.

**Threshold**: `independent_call_regions ≥ 4`

**Sub-rules** (in priority order):

**R4a – Bitwise Flag Decoding** (strongest gradient, all 6 searchers separated):
- Detection: non-loop function body with independent `(flags & 0x01)`, `(flags & 0x02)`, ... bit-test branches on a symbolic variable; `has_bitwise_branch_conds == True` AND `nested_loop_count < 2`
- Verdict: `nurs:covnew` (covnew > qc > md2u > DFS >> RP >> BFS; gap up to 64)

**R4b – Multi-Dimensional Dispatch** (multiple independent dispatch points):
- Detection: ≥ 2 independent dispatch mechanisms (e.g., two independent switches/handlers); `independent_call_regions ≥ 6`
- Verdict: `random-path` (RP samples the combinatorial product; NURS greedily chases one dimension)

**R4c – Nested Scanner** (outer loop for positions, inner loop for tokens):
- Detection: nested loops where BOTH inner and outer loop branch on symbolic data; `nested_loop_count ≥ 2` AND `independent_call_regions ≥ 4`
- Verdict: `random-path` (2D search space; RP uniform, NURS worst)

**R4d – Default Wide Dispatch**:
- Detection: `independent_call_regions ≥ 4`, no bitwise flag pattern, no nested loops
- Verdict: `nurs:covnew`

**Why NURS/RP wins over DFS**: DFS exhausts one region and never visits others. DFS is worst here (gap up to 200).

**Warning**: Wide branches INSIDE a loop body → check R10 first (loop dominance). If all branches in the loop body get covered in the first iteration, R10 applies, not R4.

---

### R5: Growing Constraint Complexity — NURS

**Trigger**: A shared variable accumulates constraints at each pipeline stage. Solver time grows exponentially with depth.

**Threshold**: DFS makes 10–100× fewer queries than NURS in same time (DFS queries are slow).

**Detection**:
- A variable constrained at multiple program points along the same path
- Division/modulo/nonlinear ops in pipeline (each stage adds to path condition)
- `constraint_growth_rate` high (hard to detect statically)

**Verdict**: `nurs:covnew` (or any NURS)

**Why**: DFS dives deep into expensive-constraint states. NURS re-prioritizes toward states where the solver cost is still manageable.

---

### R6: Convergent-Divergent Flow — nurs:covnew

**Trigger**: Multiple paths merge at a hub node, then diverge again. Post-merge branches cover different code but all states at the merge point have the same coverage footprint.

**Threshold**: `convergent_diverg ≥ 2` (blocks with ≥ 2 preds AND ≥ 2 succs); OR hub functions with high fan-in + fan-out.

**Detection**:
- Functions called from multiple call sites (fan-in) with internal branching (fan-out)
- Loops where the loop body is a merge point for multiple paths
- `convergent_diverg ≥ 2`

**Verdict**: `nurs:covnew`

**Why**: After the merge, covnew re-evaluates which post-merge branches have uncovered instructions and redirects. DFS gets stuck in one entry path and misses the post-convergence diversification.

**Note**: All non-DFS searchers perform similarly in this pattern; DFS is the main loser (gap ~62, 25%).

---

### R7: Symbolic Loop Bound — BFS

**Trigger**: A symbolic variable `n` controls loop iteration count AND different values of `n` cover different instructions (value-dependent coverage). The loop body itself is NOT identical across iterations.

**Threshold**: `symbolic_loop_bound == True` AND `coverage_blind_score == 0`

**Detection**:
- `for (i = 0; i < n; i++)` where `n` is symbolic
- Small symbolic range for `n` (e.g., 1–5) with each value producing different coverage
- Loop body has branches on loop-local variables (not the same every iteration)

**Verdict**: `bfs`

**Why**: BFS explores all values of `n` at uniform depth. DFS picks one value and follows it deep, missing shorter/longer traces. NURS may deprioritize additional iterations if the loop body looks "already covered."

**Interaction with R10**: If the loop body IS identical across iterations (R10 applies), NURS is the WORST choice, not BFS. BFS still wins, but NURS actively loses.

---

### R8: Recursive Combinatorial Explosion — BFS

**Trigger**: A recursive function branches K ways per invocation. Different branches cover different instructions at the base case.

**Threshold**: `has_direct_recursion == True` AND K ≥ 2 recursive calls per invocation.

**Detection**:
- Recursive function with multiple recursive calls per invocation
- Base case contains different code from the recursive case
- `has_direct_recursion == True`

**Verdict**: `bfs` (primary); `nurs:covnew` is close when base cases are distinct

**Why**: BFS reaches all depth-1 leaves before going deeper. DFS picks one branch at every level — it reaches one base case but misses all others. DFS scored 46% of NURS in pure recursive tests.

**Note**: When the recursive function has MULTIPLE DISTINCT BASE CASES (leaf/unary/binary/ternary handlers), NURS can beat BFS by 2–5% because it detects uncovered base case code. BFS still far beats DFS.

---

### R9: Coverage-Blind Computation — DFS

**Trigger**: Every execution path runs the same instructions but computes different values (bitfields, hash accumulators, CRC, arithmetic pipelines). Coverage heuristics see "already covered" everywhere and make no useful decisions.

**Threshold**: `coverage_blind_score ≥ 2` AND `nested_loop_count ≥ 1`

**Detection**:
- Bitfield set/clear/toggle operations (same instructions, different bit patterns)
- Hash functions, CRC computation, arithmetic accumulators
- No symbolic branches WITHIN the computation — all branching at the very end
- `coverage_blind_score ≥ 2` (arithmetic-only loop blocks)

**Verdict**: `dfs`

**Why**: NURS overhead is wasted when there's no coverage signal. BFS explodes states for identical coverage. DFS picks states cheaply and follows to completion, where the final check on computed values finally differentiates. Gap up to 2× BFS.

**Sub-cases**:
- FNV hash / CRC32 loops: DFS=142, all others=110 (gap=32, 29%)
- Fully-symbolic nested loop matcher (both pattern AND data symbolic): DFS=222, others=192 (gap=30)
- Bitfield FSM with 8 operations: DFS=216, NURS=148–154, BFS=107

---

### R10: Identical Loop Body — Avoid NURS

**Trigger**: A loop body runs the same instructions every iteration but writes to different memory locations or produces different values. After the first iteration, NURS sees "already covered" and stops prioritizing further iterations — but the different write targets matter for post-loop code.

**Threshold**: Loop body branches can ALL be covered in a single iteration (K branches, each taken at least once in iteration 1).

**Detection**:
- `a[symbolic_expr] = value` inside a loop
- Transform loops: conditional per-element transformation (char classification, normalization)
- Loop body with K ≤ 8 branches where K branches fit in one iteration
- `coverage_blind_score > 0` in the loop region

**Verdict**: avoid `nurs:covnew` and `nurs:md2u`; prefer `dfs`, `bfs`, or `random-path`

**Why**: NURS makes 1.2M+ queries achieving LESS coverage than DFS's 5.4K queries — classic coverage-blind waste. DFS completes the loop for one input; BFS progresses all iterations uniformly.

**Interaction with R7**: R7 (symbolic loop bound) + R10 = NURS is WORST. BFS wins because it naturally explores different loop lengths while avoiding NURS's identical-body trap.

---

### R11: Constraint Cost Asymmetry — nurs:qc

**Trigger**: Some branches lead to expensive constraints (modular arithmetic, division, nonlinear) while other branches reach equally novel code behind cheap constraints (equality, range checks).

**Threshold**: `srem_in_branch == True` AND `cheap_branch_count ≥ 2`

**Detection**:
- Mix of `x % p == k` (expensive) alongside `x == c` (cheap) branches
- Some branches have SolverTime 10–100× higher than others

**Verdict**: `nurs:qc`

**Why**: covnew/md2u are equally attracted to both branch types (both have uncovered code). qc tracks query cost and deprioritizes the expensive branch, covering more instructions per second.

**Caveat**: qc is WORST when all queries are similarly cheap — its tiebreaking degrades. Use qc only when you can detect the cost asymmetry.

---

### R12: Symbolic Pointer/Index Chain — random-path

**Trigger**: A chain of dependent symbolic loads: `x = a[x]` where the loaded value feeds the next index. Each step in the chain forks on the array contents.

**Threshold**: `gep_chain_depth ≥ 2`

**Detection**:
- `x = a[x]` pattern — index is itself loaded
- Array of pointers, linked-list traversal via symbolic index
- `gep_chain_depth ≥ 2` (load-indexed-by-load chains)

**Verdict**: `random-path`

**Why**: RP samples from all pending states, creating diverse starting points along the chain. Coverage heuristics can't distinguish chain positions (same load instruction at every step). DFS follows one chain; BFS spreads evenly.

**Note**: Weak rule (gap 7–33). RP's advantage is narrow. Use only when no stronger rule applies.

---

## Meta-Rules

**M1 – Time pressure required**: Rules only matter when the state space exceeds the budget. Programs that complete within time → all searchers equivalent.

**M2 – Position of useless code**: Useless branches BEFORE targets → DFS. Useless branches AFTER targets → BFS/NURS.

**M3 – Coverage blindness is the strongest signal**: When all paths execute the same code, coverage heuristics are useless. This separates all 6 searchers most strongly.

**M4 – NURS is the safe default**: Never catastrophically wrong except in R10 (identical loop body). Median or better in >80% of discriminating programs.

**M5 – BFS is situational**: Only wins on R7 (symbolic loop bound) and R8 (recursive explosion). Outside these, frequently worst due to state explosion.

**M6 – DFS is high-variance**: Most strict wins (~17) but also most strict losses (~16). Optimal for R1/R2/R3/R9; catastrophic for R4.

**M7 – qc has a narrow niche**: Only strictly better than covnew when there is a mixed expensive/cheap constraint environment (R11).

**M8 – Loop dominance**: When a structural pattern is nested inside a loop, the loop's properties dominate. Wide switch INSIDE a byte-processing loop → R10 (not R4). Sequential gate INSIDE a small loop → R10 (not R1).

**M9 – Non-discrimination is common**: 57% of real-world structural patterns produce no meaningful preference. Searcher selection matters only when the state space vastly exceeds the time budget.

---

## Feature Summary

| Feature | Type | Used By |
|---------|------|---------|
| `bit_test_chain_len` | int | R1, R2 — consecutive bit-tests before any call |
| `independent_call_regions` | int | R4 — distinct user-defined callees |
| `has_bitwise_branch_conds` | bool | R4a — AND/OR/XOR/SHL in branch conditions |
| `nested_loop_count` | int | R4c, R9 — back-edges in CFG |
| `symbolic_loop_bound` | bool | R7 — loop header icmp compares two %-registers |
| `has_direct_recursion` | bool | R8 — function calls itself |
| `coverage_blind_score` | int | R9, R10 — arith-only loop blocks |
| `srem_in_branch` | bool | R11 — srem/urem/sdiv in block with cond-br |
| `cheap_branch_count` | int | R11 — icmp eq against constant |
| `gep_chain_depth` | int | R12 — load-indexed-by-load chains |
| `convergent_diverg` | int | R6 — blocks with ≥2 preds and ≥2 succs |
| `total_blocks` | int | scale indicator |
| `total_branches` | int | scale indicator |
