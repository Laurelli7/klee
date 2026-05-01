# KLEE Searcher Selection: Comprehensive Rules Reference

**Evidence base**:
- 123 hand-crafted experiments (76 discriminating)
- 484 CodeContests programs (38 discriminating)
- 28 real-world structural units from ICFG analysis of gnumake, lua, bc, readelf, nasm, bison, tiffinfo, flvmeta
  - Wave 1 (SU01–SU08): 5/8 discriminating, 40% prediction accuracy → 3 rule evolutions
  - Wave 2 (SU09–SU28): 7/20 discriminating, 71.4% prediction accuracy → 4 rule evolutions
- **Combined prediction accuracy on discriminating real-world patterns: 62.5%**

**Searchers available**: `dfs`, `bfs`, `random-path`, `nurs:covnew`, `nurs:md2u`, `nurs:qc`

**Key finding**: 57% of real-world structural patterns are non-discriminating — all searchers tie because the state space fits in the time budget. Rules only matter when exploration is incomplete.

---

## Priority Order

When multiple rules fire simultaneously, apply the highest-priority match:

```
R9 > R2 > R1 > R3 > R4 > R10 > R8 > R7 > R11 > R6 > R12 > default
```

**Why R9 is first**: R9 is the only rule that makes coverage heuristics *completely blind*. Every other rule relies on the coverage signal being meaningful. When R9 fires, NURS/covnew/md2u are actively counterproductive — they compute an expensive signal that contains no information. R9 supersedes all coverage-based verdicts.

**Default** (no rule fires): `--search=random-path --search=nurs:covnew` interleaved.

---

## Rule 1: Sequential Gating — DFS

**What it looks like**: Target code is reachable only by passing N sequential symbolic checks in series. Each check is a branch fork with a "pass" and "fail" edge. The fail edge is a dead end. Only the path that takes the pass branch at every single fork reaches the target. Think: a combination lock where you must get every digit right.

**CFG shape**: A long chain of diamonds in series. N diamonds deep, each with a dead-end fail branch.

**Threshold**: N ≥ 15 simple binary gates, OR N ≥ 6 gates where each has ≥3 arms, OR gates with expensive constraints.

**Why DFS wins**: DFS commits to one path and follows the full chain to completion. Other searchers fork at every gate and accumulate 2^N pending states — at depth 15 that's 32,768 half-finished paths, none of which has reached the target.

**Static signal**: `bit_test_chain_len ≥ 4` with `independent_call_regions == 0` (no dispatch targets reachable).

**Evidence**: exp75 Symbolic Sort (gap 185), exp78 Tournament (gap 161), exp76 Aliased Pointers (gap 90), exp64 Breadcrumb Trail (gap 124).

**Critical caveat**: If N is small enough that all searchers exhaust the space within the budget, all searchers tie. Tested with cut.c/seq.c/sort.c patterns — with N=6 and 16 symbolic bytes, all 6 searchers tied at CovI=437 in 0.3s. Rule only discriminates when N × branching_factor × path_cost exceeds the time budget.

---

## Rule 2: Useless Fork Barrier — DFS

**What it looks like**: Many symbolic branches that produce **no new code coverage** are placed *before* the useful target code in topological order. The branches all converge before reaching any interesting functions. The symbolic variables that drive these branches don't affect which code regions are ultimately reachable — they're noise branches.

**CFG shape**: Wide fan-out of useless branches near the root → narrow bottleneck → target code.

**Threshold**: ≥ 8 bit-test or noise branches before first useful dispatch call.

**Why DFS wins**: DFS picks one path through the useless region and reaches target code quickly. BFS/NURS/RP fork at every useless branch and spend their entire budget managing tens of thousands of pending states that all lead to the same post-barrier code. Observed: BFS accumulated 140K pending states in the barrier while DFS was already exploring target code.

**Static signal**: `bit_test_chain_len ≥ 8` (same detector as R1 but stronger threshold, indicating the chain is so wide it's barrier-like).

**Evidence**: exp40 Priority Inversion (DFS 100%, others 44.7%), exp39 Inverse Funnel (DFS finishes in 1.5s, BFS takes 475s), exp41 BFS Paradise (DFS 100%, others 94.4%).

**Key distinction from R1**: In R1, the chain gates lead to different target code (different branches = different coverage). In R2, all branches in the barrier lead to the *same* post-barrier code (same coverage). The barrier is pure noise.

---

## Rule 3: Symbolic Setup → Large Concrete Sweep — DFS

**What it looks like**: A small number of symbolic variables (1–5) are read at the start. The symbolic choice determines a concrete value. The rest of the program is a very large concrete loop (100K–1M iterations) that runs based on that concrete value. Different symbolic choices lead to loops that cover different instructions, but each loop is entirely concrete once the choice is made.

**CFG shape**: Small fork on symbolic variables → very long linear chain for each fork outcome.

**Threshold**: ≤ 5 symbolic variables AND concrete loop size ≥ 100,000 iterations.

**Why DFS wins**: DFS picks one concrete assignment and runs the entire sweep to completion, covering all instructions along that path. Other searchers fork on the symbolic variables and start multiple sweeps in parallel — but none finishes within the time budget because each sweep takes too long.

**Static signal**: Few `klee_make_symbolic` calls; dominant loop bound is a concrete (non-symbolic) integer.

**Evidence**: train_00389 DFS 987 vs BFS 956 (838K queries DFS vs 8K BFS — DFS is racing through concrete code), train_02647 DFS 998 vs BFS 965.

---

## Rule 4: Wide Independent Regions — NURS or random-path

**What it looks like**: The program branches into N ≥ 4 completely independent code regions at the top level. Each region covers different instructions. There is no data dependency between regions — what happens in region A has no effect on what's reachable in region B.

**CFG shape**: Root node fans out to N subtrees, each containing distinct code.

**Threshold**: `independent_call_regions ≥ 4` (4+ distinct user-defined callees behind independent branches).

**Why DFS loses**: DFS exhausts one region completely and never visits the others. On a 10-region program, DFS covers 1/10th of the code. Gap up to 200 coverage points.

**Sub-rules** (ordered by priority within R4):

### R4a — Bitwise Flag Decoding → nurs:covnew
Independent `if (flags & 0x01)`, `if (flags & 0x02)`, ... bit-test branches on a symbolic bitfield. Each bit-test is an independent coverage target — setting bit 0 covers different code than setting bit 1.

Strongest discriminator found: SU24 produced the widest gradient across all 6 searchers:
`covnew=256 > qc=255 > md2u=253 > DFS=249 >> RP=204 >> BFS=192` (gap=64, 33%)

**Static signal**: `has_bitwise_branch_conds == True` AND `nested_loop_count < 2` (not inside a loop, which would trigger R10).

### R4b — Multi-Dimensional Dispatch → random-path
Two or more *independent* dispatch points (e.g., two separate switches on different symbolic variables, or two independent object processors). NURS greedily optimizes one dispatch dimension and ignores the combinatorial product of both.

SU08 (readelf-style multi-level dispatch): `RP=596, BFS=587, NURS=452, DFS=306` — RP beat NURS by 144 (32%).

**Static signal**: `independent_call_regions ≥ 6`.

### R4c — Nested Scanner → random-path
Outer loop over positions, inner loop over token types. Both loops branch on symbolic data, creating a 2D search space (position × token_type). NURS optimizes one dimension greedily and misses the other.

SU26: `RP=277 > BFS=275 > DFS=265 > covnew=255 = md2u=255 = qc=255` (gap=22, NURS worst).

**Static signal**: `nested_loop_count ≥ 2` AND `independent_call_regions ≥ 4`.

### R4d — Default Wide Dispatch → nurs:covnew
Wide independent regions with no bitwise patterns and no nesting. Standard NURS case.

**Evidence**: exp70 Multi-Objective (NURS 360, DFS 292), exp93 Breadth Lottery (non-DFS 500, DFS 164), exp97 Symmetric Siblings (non-DFS 386, DFS 153).

**Loop dominance warning**: If wide branches are *inside* a byte-processing loop, R10 takes precedence. After the first iteration, all branch arms in the loop body are "already covered." DFS wins, not NURS. Confirmed: tr.c/wc.c 12-way switch inside 8-byte loop → DFS won (518 vs 495).

---

## Rule 5: Growing Constraint Complexity — NURS

**What it looks like**: A shared variable accumulates solver constraints at each pipeline stage. Stage 1 adds constraint C1, stage 2 adds C2 ON TOP OF C1, etc. The constraint set grows with execution depth, so solver queries at depth 10 are 10× harder than at depth 1.

**CFG shape**: A pipeline where each stage adds a new constraint involving the same symbolic variable.

**Why NURS wins**: DFS dives deep and gets stuck at states with maximal constraint complexity — each query takes seconds. NURS re-prioritizes toward states where the solver cost is still manageable. DFS makes 10× fewer queries than NURS in the same time, but each DFS query is 10× slower.

**Static signal**: Hard to detect statically. Indirect indicator: pipeline of divisions/modulos on the same variable (`srem` in multiple sequential blocks). `constraint_growth_rate` is not currently computed.

**Evidence**: exp77 Constraint Explosion (NURS 342, DFS 273), exp83 Symbolic Division (NURS 290, DFS 274), exp71 Solver Cost Cliff (non-DFS 312, DFS 219).

---

## Rule 6: Convergent-Divergent Flow — nurs:covnew

**What it looks like**: Multiple execution paths merge at a "hub" node (high fan-in), then the hub branches again (high fan-out). At the merge point, all states arriving from different paths have the *same coverage footprint* — they've all executed the same instructions, just via different routes. Coverage heuristics temporarily lose their signal at this merge point. The post-merge branches cover different code.

**CFG shape**: Multiple paths → single merge node → multiple diverging paths.

**Threshold**: `convergent_diverg ≥ 2` (blocks with ≥2 predecessors AND ≥2 successors).

**Why covnew wins**: After the merge, covnew re-evaluates which post-merge branches have uncovered instructions. DFS picks one entry path and gets stuck there — it never passes through the hub to reach the post-merge diversification.

**Hub node confirmation** (SU16): 5 entry functions → hub_process → flag-based fan-out: `DFS=244, all non-DFS=306` (gap=62, 25%). All non-DFS searchers tied — confirming the pattern purely punishes DFS's commitment.

**Static signal**: `convergent_diverg ≥ 2`.

**Note**: All non-DFS searchers perform similarly. The pattern punishes DFS specifically, not one non-DFS over another.

---

## Rule 7: Symbolic Loop Bound — BFS

**What it looks like**: A symbolic variable `n` controls how many times a loop executes. Crucially, different values of `n` produce *detectably different instruction coverage* — not just the same loop body N times, but N=1 covers base-case code that N=5 never reaches, and N=5 covers the post-loop analysis code with a full dataset.

**CFG shape**: Fork on `n` → n different chain lengths, each sharing the loop body but with different lengths and different end-of-loop behavior.

**Threshold**: `symbolic_loop_bound == True` AND `coverage_blind_score == 0` (the loop body is NOT coverage-identical across iterations).

**Why BFS wins**: BFS explores all values of `n` at uniform depth — it reaches the n=1 complete execution first, covering base case + output code, then n=2, etc. DFS picks one value of `n` and follows it to completion, which may be a deep trace that covers many iterations of the same body but misses the n=1 base case entirely.

**Static signal**: `symbolic_loop_bound == True` — detected when loop header has `icmp` comparing two `%register` values (both sides are symbolic, not a constant).

**Evidence**: train_08285 BFS 1175 vs others 1009 (gap 166), train_04207 BFS 1523 vs DFS 1433 (gap 96). GNU coreutils: seq.c/factor.c/comm.c → BFS 447 vs DFS 382, NURS 369.

**Interaction with R10**: When the loop body IS identical across iterations (R10 applies simultaneously), NURS becomes the *worst* searcher — it gets trapped expanding identical iterations. BFS still wins over NURS, and DFS is in the middle.

---

## Rule 8: Recursive Combinatorial Explosion — BFS

**What it looks like**: A recursive function makes K ≥ 2 recursive calls per invocation. The recursion tree has K^L leaves for L levels. Each leaf (base case) contains different code. To achieve full coverage you need to visit all leaves, which means exploring the full tree.

**CFG shape**: A tree rooted at the recursive call site. Each internal node has K children. Leaves contain the base-case code.

**Threshold**: `has_direct_recursion == True` AND K ≥ 2 recursive calls per invocation.

**Why BFS wins**: BFS reaches all depth-1 leaves before exploring any at depth 2. For small inputs, BFS completes all base cases and covers the result/output code. DFS picks one branch at every level and follows it deep — it reaches one base case but misses K^L − 1 others. DFS scored 46% of NURS in pure recursive tests.

**Static signal**: `has_direct_recursion == True` (function's callee set includes itself).

**Evidence**: train_07438 BFS 1197 vs DFS 1083 (gap 139), SU03 recursive tree: md2u=216 >> DFS=99 (DFS scored only 46%).

**Amendment**: When the recursive function has multiple *distinct* base cases (leaf/unary/binary/ternary handlers with different code), NURS can detect and prioritize unexplored base cases, slightly outperforming BFS by 2–5%. BFS still far beats DFS. Key: DFS is always catastrophic here.

---

## Rule 9: Same Code, Different State (Coverage-Blind) — DFS

**What it looks like**: Every execution path runs the exact same instructions but computes different *values* via bitfield operations, hash functions, arithmetic accumulators, CRC. There are no branches inside the computation that vary by path — all branching happens at the very end, on the computed value. Coverage heuristics see "already covered" everywhere because the same instructions execute on every path.

**CFG shape**: A single pipeline or accumulator loop. All states take the same path through every branch. Only the final check on the accumulated value differentiates.

**Threshold**: `coverage_blind_score ≥ 2` AND `nested_loop_count ≥ 1`.

**Why DFS wins**: NURS computes a coverage signal that contains zero information — all states look identical. It wastes time on this useless computation. BFS builds an enormous queue of coverage-identical states and makes 25K–32K queries with zero additional coverage vs DFS's 4K queries. DFS avoids all overhead and runs each path cheaply to the final check.

**Static signal**: `coverage_blind_score` counts loop-body blocks dominated by arithmetic ops (XOR/SHL/AND/OR/MUL/ADD) with no new `icmp` branches. Score ≥ 2 indicates a pure computation loop.

**Evidence**:
- exp82 Bitfield FSM: `DFS=216, NURS=148–154, BFS=107` (2× over BFS)
- SU13 FNV hash + CRC32: `DFS=142, all others=110` (gap=32, 29%). Others used 25K–32K queries with NO additional coverage vs DFS's 4,483 queries.
- SU07 nested loop matcher (both patterns AND input symbolic): `DFS=222, all others=192` (gap=30, 15.6%). ~10^58 path combinations with identical loop body coverage.

**Why R9 is the highest priority**: It is the only rule that makes ALL coverage-based heuristics completely useless simultaneously. R9 supersedes R4, R6, R7, R8 — even if those patterns are also present, the coverage signal is gone so their verdicts are wrong.

---

## Rule 10: Identical Loop Body, Different Memory Targets — Avoid NURS

**What it looks like**: A loop body executes the *same instructions* every iteration (same control flow, same branch structure) but writes to different memory locations per iteration — different array slots, different struct fields. After the first iteration, ALL branches in the loop body are "already covered" from NURS's perspective. But the different write targets affect post-loop behavior, so the iterations still matter.

**CFG shape**: A loop with a constant body, where the write target is `a[symbolic_expr] = value`. Loop reconverges after each iteration.

**Threshold**: Loop body has K conditional branches where all K branches are visited in a single iteration. After iteration 1: NURS coverage signal is dead.

**Why NURS loses**: covnew/md2u see the loop body as "already covered" and deprioritize further iterations. They make 1.2M+ queries achieving LESS coverage than DFS's 5.4K queries — the definition of coverage-blind waste.

**Use**: `dfs`, `bfs`, or `random-path`. Never `nurs:covnew` or `nurs:md2u`.

**Static signal**: `coverage_blind_score > 0` in the loop region; loop body branches all covered in first iteration (K branches ≤ one iteration's worth of path).

**Evidence**:
- SU20 transform loop (12-byte input, 8 conditional categories per byte): `DFS=147, RP=147, BFS=145, NURS=122` (NURS 17% below DFS).
- train_02120: BFS/RP 1129, DFS 1083, NURS 1057 (gap 72).
- train_06744: DFS/BFS 1012, NURS 970 (gap 42).

**Transform loop sub-case**: Loops where each iteration applies a conditional transformation to an element (character class mapping, value normalization per byte) look like R4 (wide independent branches) but ARE R10 after the first iteration. The branches look like independent coverage targets — but they're all covered in one pass. Detection: if K branches in loop body and K is small enough to be visited in one iteration, apply R10 not R4.

**Interaction with R7**: When both R7 (symbolic loop bound) and R10 (identical body) fire, NURS is the WORST choice (not just "not best"). BFS explores different loop *lengths* and wins; NURS gets trapped in identical-bodied iterations.

---

## Rule 11: Constraint Cost Asymmetry — nurs:qc

**What it looks like**: Some branches lead to code behind expensive constraints — modular arithmetic (`x % 97 == 13`), division, nonlinear expressions. Other branches reach equally novel code behind cheap constraints — simple equality (`x == 5`), range checks (`x < 10`). All branches have uncovered code, so covnew/md2u treat them equally. But the expensive branches waste solver time.

**CFG shape**: Multiple branches from a decision point. Branch A requires solving `x % p == k` (expensive). Branch B requires solving `x == c` (cheap). Both branches reach uncovered instructions.

**Threshold**: `srem_in_branch == True` AND `cheap_branch_count ≥ 2`.

**Why qc wins**: covnew/md2u weight by novelty of coverage — both branches look equally attractive. qc tracks total query cost per state and deprioritizes states where the solver is slow, spending its budget on cheap paths and covering more instructions per second.

**Static signal**: `srem_in_branch` detects `srem/urem/sdiv/udiv` instruction in a block with a conditional branch. `cheap_branch_count` counts blocks with `icmp eq %reg, <integer_literal>` feeding a branch.

**Evidence**:
- exp85 Poison Path: `qc=223, covnew/md2u=217` (gap 6)
- GNU coreutils factor.c (modular exponentiation) vs basenc.c (base16 classification): `qc=474, covnew/md2u=443` (gap=31; real code creates larger cost differences than hand-crafted experiments)

**Critical caveat**: qc is the *worst* searcher when all queries are similarly cheap — its tiebreaking is worse than random. train_04164: `qc=984, others=1006`. Only use qc when you can confirm cost asymmetry exists.

---

## Rule 12: Symbolic Pointer/Index Chain — random-path

**What it looks like**: A chain of dependent symbolic loads where the loaded value is used as the next index: `x = a[x]`. This creates a chain where each step forks on the array contents, and the next fork depends on which value was loaded at the previous step. Array of pointers, linked list traversal via symbolic index, hash table chaining.

**CFG shape**: A linear chain where each node forks N ways (N = array size), but the fork outcome determines the input to the next fork.

**Threshold**: `gep_chain_depth ≥ 2`.

**Why random-path wins**: RP randomly samples from all pending states, creating diverse starting points along the chain. Coverage heuristics can't distinguish chain positions — the same load instruction executes at every step, so covnew has no signal. DFS follows one chain to completion but misses all alternative routes. BFS spreads uniformly but doesn't focus on chain diversity.

**Static signal**: `gep_chain_depth` counts blocks where a `getelementptr` instruction follows a `load` instruction — a proxy for load-indexed-by-load chaining.

**Evidence**: exp61 Scaled Pointer Chase (RP 268, DFS 260, gap 8), train_08183 Fenwick tree (RP 1016, DFS/BFS 1006, NURS 983).

**Note**: This is the weakest rule. Gaps of 7–33 only. RP's advantage is narrow and inconsistent. Apply only when no stronger rule fires.

---

## Meta-Rules

### M1 — Time Pressure Required
Rules only discriminate when the state space exceeds the time budget. Programs that complete within the budget produce zero discrimination. Feature: if `total_queries < 100K` or the symbolic range is small, expect all searchers to tie.

### M2 — Position of Useless Code
Where useless forking branches sit relative to target code is the single strongest predictor:
- Useless branches **BEFORE** targets → **DFS** (skips the noise)
- Useless branches **AFTER** targets → **BFS or NURS** (reaches targets before the noise)

### M3 — Coverage Blindness Is the Strongest Signal
When all paths execute the same code, coverage heuristics carry zero information. This is the single most diagnostically valuable feature — it separates all 6 searchers from each other and determines why R9 has the highest priority.

### M4 — NURS Is the Safe Default
NURS (covnew or md2u) is never catastrophically wrong except in R10 (identical loop body). Median or better choice in >80% of discriminating programs. When in doubt, use `nurs:covnew`.

### M5 — BFS Is Situational, Not General
BFS strict wins exist (11 cases) but require specific conditions: symbolic loop bound with value-dependent coverage (R7) or recursive combinatorial explosion (R8). Outside these patterns, BFS is frequently the *worst* choice due to state explosion.

### M6 — DFS Is High-Variance
Most strict wins (~17) but also most strict losses (~16). Optimal for R1/R2/R3/R9. Catastrophic for R4 (wide independent regions). Never use DFS as default.

### M7 — qc Has a Narrow Niche
nurs:qc is strictly better than covnew/md2u only when there is a confirmed mix of expensive and cheap constraints (R11). Using qc as a default makes things worse.

### M8 — Loop Dominance
When a structural pattern is nested inside a loop, the loop's properties dominate:
- Wide switch inside a byte-processing loop → R10, not R4
- Sequential gate chain inside a small loop → R10, not R1
- **Hierarchy**: Loop structure > Branch structure when nested

Confirmed: tr.c/wc.c 12-way switch inside 8-byte loop → DFS won (Rule 10), not NURS (Rule 4).

### M9 — Rule Interactions Compound
Rules can combine to produce effects neither rule predicts alone:
- R7 + R10 → NURS is the *worst* (not just not best). BFS explores different loop lengths; NURS gets trapped in identical-body iterations.
- R4 (wide dispatch) + multiple independent instances → RP beats NURS. NURS greedily chases one dimension.
- R9 + nested loops → DFS dominance amplified. Fully symbolic data on both sides creates pure coverage blindness.
- R4 (wide branches) inside loop → becomes R10 when branches exhausted after one iteration. Transform loops are a subtle trap.

### M10 — Non-Discrimination Is Common in Real Programs
57% of real-world structural patterns (16/28 structural units) showed no meaningful searcher preference. Patterns that never discriminated: cascaded switches, dense/sparse switches, callback dispatch, diamond validation, serialization, equality chains, struct fields, multi-loops, range checks, merge points, tail-call chains. Searcher selection only matters when the state space vastly exceeds the time budget.

---

## Feature Vector (Static Detectors)

| Feature | Type | Detected From IR | Primary Rule(s) |
|---------|------|-----------------|-----------------|
| `bit_test_chain_len` | int | Longest chain of AND+branch blocks from entry before any call | R1, R2 |
| `independent_call_regions` | int | Count of distinct user-defined callees in target function | R4, R4b, R4c |
| `has_bitwise_branch_conds` | bool | AND/OR/XOR/SHL instruction in same block as `br i1` | R4a |
| `nested_loop_count` | int | Number of CFG back-edges via DFS | R4c, R7, R9 |
| `symbolic_loop_bound` | bool | Loop header `icmp` compares two `%register` values (not constant) | R7 |
| `has_direct_recursion` | bool | Function's callee set contains its own name | R8 |
| `coverage_blind_score` | int | Loop-body blocks with ≥2 arithmetic ops and no `icmp` | R9, R10 |
| `srem_in_branch` | bool | `srem/urem/sdiv/udiv` instruction co-occurring with `br i1` | R11 |
| `cheap_branch_count` | int | Blocks with `icmp eq %reg, <integer_const>` + branch | R11 |
| `gep_chain_depth` | int | GEP instruction following a load in the same block | R12 |
| `convergent_diverg` | int | Blocks with ≥2 predecessors AND ≥2 successors | R6 |

---

## Known Gaps and Open Questions

1. **R3 has no static detector**: The concrete loop size after symbolic fork resolution cannot be determined from IR alone without execution.

2. **R5 has no static detector**: Constraint growth rate requires running the solver. Indirect proxy: chains of `srem/div` on the same variable, but this overlaps with R11.

3. **R9 detector is incomplete**: `coverage_blind_score` only checks back-edge head/tail blocks, missing intermediate loop body blocks where arithmetic typically lives.

4. **R4a vs R2 ambiguity**: A chain of N independent bit-tests on a bitfield (`if (flags & 0x01) do_A(); if (flags & 0x02) do_B();`) looks identical to R2 (useless fork barrier) from the IR scanner. The distinction — whether each branch leads to *independent useful code* vs *the same dead-end* — requires dataflow analysis the current scanner does not perform.

5. **Scaling behavior unknown**: All experiments used 10–30s timeouts. At 300s or 3600s (ParaSuit's budget), rankings may shift. DFS's R1/R9 advantages may shrink as other searchers eventually reach targets.

6. **Nondeterminism**: Some discriminators may be artifacts of KLEE's internal scheduling. Running each configuration 3× with different seeds would filter these out.

7. **Real-world programs**: Both the hand-crafted experiments and CodeContests programs are artificial or competitive-programming style. The 28 structural units address this partially but 57% are non-discriminating, limiting sample size for rule validation.
