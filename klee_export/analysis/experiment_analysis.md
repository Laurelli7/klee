# KLEE Search Heuristic Experiment Analysis

## Methodology

- **123 experiments** across **13 waves** (exp01–exp123)
- **7 searchers**: DFS, BFS, random-path, nurs:qc, nurs:covnew, nurs:md2u, default (random-path interleaved with nurs:covnew)
- **Timeouts**: 10s (waves 1–9, 11–13), 30s (wave 10)
- **Metrics**: CoveredInstr (CovI), UncoveredInstr (UnI), Queries, WallTime, SolverTime, NumStates, TermExit, TermEarly
- **Discriminator** = experiment where at least one searcher achieves strictly different coverage than others

---

## Executive Summary

Of 123 experiments, **47 are non-discriminators** (all searchers identical — programs too small or fully explorable). The remaining **76 discriminators** reveal clear patterns about when each searcher excels or fails.

DFS is the most polarizing searcher: it produces the most strict wins (15) but is also the most frequently worst (11 cases where all others beat it). NURS variants are the safest overall — they rarely lose badly and dominate programs with several independent code regions. BFS strict wins are rare (4 cases), confined to one pattern: a branch point forks into many states that all execute the same handler code, but each fork produces different downstream behavior — coverage heuristics see "already covered" and deprioritize, but BFS explores all forks at each depth equally. The qc variant has a unique edge over covnew/md2u when some paths have expensive solver constraints but no more coverage than cheap paths.

---

## Strict Win Analysis

A "strict win" means one searcher (or group) achieves strictly higher coverage than all others within the time budget. These are the most actionable findings.

### DFS Strictly Better Than All Others

DFS wins when (1) reaching target code requires passing through many sequential checks on one path — sorting networks, tournament brackets, alias chains — so only a fully committed traversal reaches the end, (2) the program has many useless forking branches placed before the useful code in the control flow, so breadth-first searchers drown in the useless forks and never reach the targets, or (3) coverage depends on following a single chain of pointer dereferences or value comparisons to completion, where switching away mid-chain wastes all prior work. DFS picks one path, runs it to termination, then backtracks — it never pays the cost of managing thousands of half-explored states.

| Exp | Name | DFS CovI | Best Other | Gap | Why DFS Wins |
|-----|------|----------|-----------|-----|-------------|
| exp40 | Priority Inversion | 100% | 44.7% | +55% | 2^24 noise paths BEFORE 10 islands — others drown in 75K–140K pending states, never reach islands |
| exp80 | Island Archipelago | 367 | 276 (NURS) | +91 | 16 code islands gated by 4 difficulty tiers — DFS finishes one tier completely then moves to the next; others spread across tiers and finish none |
| exp75 | Symbolic Sort | 305 | 120 | +185 | Sorting network requires completing all comparison stages on one path to reach output code — DFS does this; others start many partial sorts and finish none |
| exp78 | Tournament | 281 | 120 | +161 | Deep sequential comparison tree — linear path through bracket |
| exp74 | Longjmp Simulation | 276 | 267 (NURS) | +9 | Simulated longjmp skips stack frames — only a searcher that stays on one execution path sees the full jump chain; others lose context when switching states |
| exp76 | Aliased Pointers | 253 | 163 | +90 | Each step reads a pointer that points to the next pointer — must follow the entire chain on one path to reach the end; switching away mid-chain loses all progress |
| exp64 | Breadcrumb Trail | 244 | 120 | +124 | Linear trail of clues — DFS follows sequentially, others scatter |
| exp72 | Distance vs Novelty | 234 | 123 | +111 | Coverage is only reachable at the bottom of a long chain — DFS reaches it; others spend the budget exploring the top half repeatedly |
| exp99 | Cascade Breadth | 357 | 333 (NURS) | +24 | Each cascade stage forks many states — breadth-first searchers accumulate too many pending states and slow down; DFS finishes each cascade before starting the next |
| exp101 | Memory Pressure BFS | 491 | 462 (NURS) | +29 | Memory pressure kills BFS (335), DFS barely affected |

Weaker DFS wins (small margins, 3–6 CovI):
- exp11 Wide Then Deep (122 > 116)
- exp12 Trapdoor (95 > 91)
- exp22 State Pressure (149 > 139)
- exp24 Interleaved Default (147 > 146)
- exp37 Starvation (126 > 123)

### NURS Strictly Better Than All Others

All three NURS variants (qc, covnew, md2u) tie in these cases. NURS wins when (1) the program has several independent code regions that each require separate exploration — focusing only on one region wastes the time budget, (2) a shared variable gains new constraints at each stage, so constraint complexity grows with depth and both DFS (trapped deep) and BFS (flooded wide) suffer, while NURS re-prioritizes toward states with solvable constraints, or (3) all paths merge through a single control-flow point and then branch again — after the merge, coverage signals reset and NURS can re-orient, while BFS/RP carry stale scheduling decisions.

| Exp | Name | NURS CovI | DFS | BFS | Why NURS Wins |
|-----|------|-----------|-----|-----|--------------|
| exp102 | BFS Pipeline | 430 | 412 | 297 | Multi-stage pipeline where each stage has independent branches — DFS finishes one pipeline path but misses other stages; BFS floods all stages equally and finishes none; NURS picks the stage with most uncovered code |
| exp70 | Multi-Objective | 360 | 292 | 147 | Several independent code regions each need exploration — DFS exhausts one region and runs out of time; BFS spreads too thin; NURS switches to whichever region has the most uncovered instructions |
| exp77 | Constraint Explosion | 342 | 273 | 249 | A shared variable gains constraints at each stage, making deeper states increasingly expensive to solve — DFS gets stuck solving deep states; BFS creates too many shallow states; NURS picks states where the solver cost is still manageable |
| exp87 | Convergent-Divergent | 299 | 279 | 275 | All paths merge at one point then branch again — after the merge, all states look identical in coverage, but NURS re-evaluates which post-merge branches have uncovered code; others carry stale priorities from the pre-merge phase |
| exp21 | Interleaved Hot/Cold | 295 | 269 | 207 | Branches alternate between paths with many uncovered instructions (hot) and paths with few (cold) — NURS picks the hot paths first; BFS/RP waste equal time on cold paths |
| exp83 | Symbolic Division | 290 | 274 | 261 | Pipeline of divisions and modulos creates solver queries that grow in cost per stage — NURS avoids states where constraints have become too expensive; DFS commits to the deepest (most expensive) states |
| exp96 | Layered Breadth | 208 | 198 | 155 | Each layer forks into many states before the next layer's code — BFS holds all states from all layers in memory at once (worst); NURS picks the states closest to uncovered code across layers |

### nurs:qc Strictly Better Than covnew/md2u

qc (query cost) deprioritizes states whose solver queries are expensive. It wins when some paths lead to code behind expensive constraints (e.g., modular arithmetic like `x%97==13`) while other paths reach equally novel code behind cheap constraints (e.g., simple equality `x==5`). covnew/md2u treat both as equally attractive because both lead to uncovered code, but qc skips the expensive paths and reaches more total coverage in the same time budget.

| Exp | Name | qc | covnew | md2u | Why qc Wins |
|-----|------|--------|--------|------|-------------|
| exp85 | Poison Path | 223 | 217 | 217 | 8 "poison" islands have expensive modular-arithmetic constraints (key%97==13, key*key%101==42). covnew/md2u are attracted because the code is "novel" but qc avoids the cost |
| exp115 | Small Table | 182 | 167 | 167 | 8-element table creates constraint diversity that qc navigates by avoiding costly collision queries |
| exp106 | Hash Map Sim | 363 | 361 | 361 | Marginal edge — qc slightly better at navigating hash table probe sequences |

### All Non-DFS Strictly Better Than DFS

DFS fails when (1) the program has loops where the loop body can jump back to different points inside itself (not just the top), creating cycles that DFS follows endlessly — it executes millions of instructions but explores almost no new states, (2) the program branches widely near the top and each branch covers independent code — DFS picks one branch and misses all the others, or (3) one branch leads to constraints that grow exponentially more expensive to solve — DFS dives into it and burns its entire time budget on a few solver queries. In these cases BFS/NURS/RP/default all tie.

| Exp | Name | Non-DFS CovI | DFS CovI | Gap | Why DFS Loses |
|-----|------|-------------|----------|-----|--------------|
| exp62 | Asymmetric Cost Tree | 534 | 416 | +118 | Left subtree has 1 branch per level (cheap), right has 4 per level (expensive) — DFS commits to whichever side it enters first and misses the other entirely |
| exp93 | Breadth Lottery | 500 | 164 | +336 | Each of many top-level branches leads to a unique code island — DFS follows one branch to completion and never visits the other islands |
| exp97 | Symmetric Siblings | 386 | 153 | +233 | Many equal-width branches at the top, each covering different code — DFS enters one branch and exhausts its budget there |
| exp91 | Shallow Spread | 346 | 171 | +175 | Most coverage is 1–2 branches deep across a wide tree — DFS goes deep on one branch and misses all the shallow coverage on other branches |
| exp103 | Multi-Table Write | 348 | 234 | +114 | Switch dispatches to 4 independent sub-problems — DFS enters one case and explores it deeply, never switching to the other 3 |
| exp84 | Knotted CFG | 216 | 184 | +32 | Loops where the body jumps back to different points inside itself (not just the loop top) — DFS gets trapped cycling through these loops endlessly (37.5M instructions but only 47 states explored) |
| exp71 | Solver Cost Cliff | 312 | 219 | +93 | One branch has constraints that grow exponentially harder per level — DFS enters it and spends all its time on a few expensive solver queries |
| exp38 | Mixed Depth | 269 | 137 | +132 | Coverage is spread across branches at different depths (some shallow, some deep) — DFS picks one deep branch and never visits the shallow ones |
| exp34 | Time to Coverage | 209 | 114 | +95 | Coverage targets appear at different wall-clock times — early targets are on short branches that DFS skips by going deep; by the time DFS backtracks to them, the timeout has passed |
| exp100 | Solver Quicksand | 233 | 161 | +72 | DFS follows a path where each step adds harder constraints — it makes only 10K solver queries (each very slow) while others make 71K–95K cheaper queries and cover more code |
| exp19 | Solver Cost War | 91 | 67 | +24 | Some branches have cheap constraints, others expensive — DFS enters an expensive branch and wastes its budget solving a few hard queries |

### random-path / default Strictly Better

Rare — only 2 cases with small margins, both involving chains of array lookups where each step’s index is symbolic (like traversing a linked list with symbolic next-pointers).

| Exp | Name | RP/default | DFS | NURS (covnew/md2u) | Pattern |
|-----|------|-----------|-----|---------------------|---------|
| exp61 | Scaled Pointer Chase | 268 | 260 | 251 | RP/default > DFS > NURS |
| exp57 | Pointer Chase | 178 | 176 | 166 | RP/default/qc > DFS > covnew/md2u |

### BFS Strictly Better Than All Others

Only 4 cases found across 123 experiments, all sharing the same general pattern: a branch point forks into many states that execute the same code, but each fork leads to different program state downstream. Coverage heuristics deprioritize these forks ("already covered"), but BFS treats them equally. The margin is a consistent +5 CovI.

| Exp | Name | BFS | 2nd-best | DFS | Table Size | Key Variant |
|-----|------|-----|----------|-----|-----------|-------------|
| exp73 | Symbolic Write | 172 | 167 | 162 | 16 | Original pattern |
| exp120 | Big Table | 172 | 167 | 162 | 32 | Larger table (same margin) |
| exp121 | No Sym Values | 164 | 159 | 154 | 16 | Deterministic write values |
| exp123 | Max Handlers | 277 | 272 | 237 | 16 | Unique handlers per write step |

**Why BFS wins here**: When a branch forks into N states that all run the same handler code (e.g., `table[symbolic_index]` forks into one state per slot), coverage-based searchers see "already covered" and deprioritize all but the first fork. But each fork produces a different internal state (which slot was written), leading to different behavior at later stages. BFS has no coverage bias — it explores ALL forks at depth D before ANY fork at depth D+1, creating maximum diversity of internal states at each level.

**Why the margin is always +5**: BFS reaches exactly one additional combination of collision/fresh writes at depth 4 that produces a unique hit/miss pattern at the read stage, covering 5 additional instructions. This is a structural property of the write-collision tree under solver-time pressure.

**What matters for BFS wins**:
- A branch point that forks into many states running the same code (so coverage heuristics see "already covered")
- Each fork produces different internal state that affects later branches
- Solver pressure (must be time-limited, not completion-limited)
- Enough forks to prevent full exploration (≥16 in these experiments)
- More stages don't help if the total space is still fully explorable

---

## The Best Discriminators

Programs that separate the most searchers from each other. These are the most informative experiments for understanding searcher behavior.

### #1: exp82 — Bitfield FSM (All 7 Searchers Distinct)

8 symbolic operations on a uint16_t bitfield (set/clear/toggle/conditional-set). Final `check_state()` has 16 equality checks for specific bit patterns. Same code executes for all paths but computes different logical states.

| Searcher | CovI | UnI | FullBr | PartBr | TermExit | States | Queries | Wall (s) |
|----------|------|-----|--------|--------|----------|--------|---------|----------|
| DFS | 216 | 43 | 8 | 12 | 578 | 600 | 24K | 30.3 |
| random-path | 210 | 49 | 7 | 13 | 3 | 8,993 | 124K | 34.8 |
| default | 179 | 80 | 8 | 5 | 0 | 8,491 | 120K | 32.3 |
| nurs:covnew | 154 | 105 | 4 | 6 | 0 | 11,471 | 177K | 32.4 |
| nurs:md2u | 150 | 109 | 4 | 5 | 0 | 10,873 | 167K | 32.3 |
| nurs:qc | 148 | 111 | 4 | 5 | 0 | 10,800 | 165K | 32.3 |
| BFS | 107 | 152 | 1 | 2 | 0 | 26,714 | 250K | 35.1 |

**Key insight**: When the same instructions are executed on every path but compute different values (via bitfield operations), coverage-based heuristics see "already covered" and stop prioritizing those states. But the different computed values lead to different branches in the final check. This is the strongest NURS splitter — programs where coverage is identical but computed state differs.

### #2: exp85 — Poison Path (qc Split from covnew/md2u)

8 "poison" code islands behind expensive modular-arithmetic constraints + 2 cheap "target" islands behind trivial equality checks.

| Searcher | CovI | UnI | TermExit | States | Queries | Wall (s) |
|----------|------|-----|----------|--------|---------|----------|
| DFS | 262 | 26 | 5,930 | 5,943 | 65K | 30.6 |
| nurs:qc | 223 | 65 | 0 | 56,533 | 418K | 43.1 |
| nurs:covnew | 217 | 71 | 0 | 62,799 | 465K | 44.1 |
| nurs:md2u | 217 | 71 | 0 | 62,854 | 465K | 44.1 |
| default | 196 | 92 | 0 | 171,522 | 1.2M | 70.9 |
| random-path | 182 | 106 | 0 | 290,617 | 1.9M | 104.9 |
| BFS | 163 | 125 | 0 | 400,997 | 2.3M | 133.7 |

**Key insight**: When some paths have expensive constraints and others have cheap constraints, but both lead to equally novel code: covnew/md2u are attracted to the expensive paths because the code is uncovered, wasting solver time. qc deprioritizes them because the queries cost too much.

### #3: exp40 — Priority Inversion (DFS 100% vs Others 44.7%)

3 noise bytes (2^24 paths) followed by 10 code "islands" behind equality checks on a key byte.

| Searcher | ICov% | Completed | Time (s) | MaxStates |
|----------|-------|-----------|----------|-----------|
| DFS | 100 | 46,612 | 1.59 | 28 |
| BFS | 44.74 | 0 | 586.68 | 140,337 |
| random-path | 44.74 | 0 | 411.72 | 91,773 |
| nurs:qc | 44.74 | 0 | 344.29 | 78,782 |
| nurs:covnew | 44.74 | 0 | 336.82 | 77,074 |
| nurs:md2u | 44.74 | 0 | 340.82 | 78,001 |
| default | 44.74 | 0 | 344.60 | 75,540 |

**Key insight**: Where useless branches sit matters enormously. Useless branches BEFORE target code = DFS wins (it picks one path through the useless branches and reaches the targets quickly; all others fork at each useless branch and drown in pending states). Useless branches AFTER targets would favor breadth-first approaches.

### Top 10 Discriminators Summary

| Rank | Experiment | Discriminates | CovI Range | Key Feature |
|------|-----------|---------------|-----------|-------------|
| 1 | exp82 Bitfield FSM | All 7 searchers | 107–216 | Same code computes different bitfield values — coverage heuristics are blind to computed state |
| 2 | exp85 Poison Path | qc vs covnew/md2u | 163–262 | Some paths have expensive constraints, others cheap, but both lead to novel code |
| 3 | exp40 Priority Inversion | DFS vs all | 44.7%–100% | Useless forking branches placed before target code |
| 4 | exp70 Multi-Objective | NURS vs DFS vs BFS | 147–360 | Several independent code regions each needing exploration |
| 5 | exp77 Constraint Explosion | NURS vs DFS vs BFS | 249–342 | Shared variable gains constraints at each stage |
| 6 | exp84 Knotted CFG | DFS vs all | 184–216 | Loops with jumps back into the middle of the loop body — DFS cycles endlessly |
| 7 | exp87 Convergent Paths | NURS vs DFS vs RP | 245–299 | All paths merge then branch again |
| 8 | exp80 Island Archipelago | DFS vs NURS vs BFS | 152–367 | Code islands gated by sequential difficulty tiers |
| 9 | exp62 Asymmetric Cost | DFS vs all | 416–534 | Left subtree cheap (1 branch/level), right expensive (4 branches/level) |
| 10 | exp83 Symbolic Division | NURS vs DFS vs BFS | 261–290 | Division/modulo constraints grow in solver cost per stage |

### What Splits Each Searcher Pair

| Pair | Best Discriminator | Pattern |
|------|--------------------|---------|
| DFS vs all | exp40 (priority inversion) | Useless forking branches placed before target code — DFS ignores them, others drown |
| DFS vs NURS | exp77 (constraint explosion) | Shared variable gains constraints at each stage — DFS gets stuck deep, NURS re-prioritizes |
| NURS vs BFS | exp70 (multi-objective) | Several independent code regions — NURS switches between them, BFS spreads too thin |
| NURS vs random-path | exp87 (convergent paths) | All paths merge then branch again — NURS re-orients after merge, RP carries stale priorities |
| qc vs covnew | exp85 (poison path) | Expensive constraints on some paths, cheap on others — qc avoids the expensive ones |
| covnew vs md2u | exp82 (bitfield FSM) | Same code computes different bitfield values (154 vs 150) |
| qc vs md2u | exp82 (bitfield FSM) | Same code computes different bitfield values (148 vs 150) |

---

## Searcher Scorecard

| Searcher | Strict Wins | Best When | Worst When |
|----------|-------------|-----------|------------|
| DFS | ~15 | Program requires passing many sequential checks on one path to reach targets; useless forking branches appear before useful code; coverage depends on following a single chain of lookups or comparisons to completion | Loops with jumps back into the middle of the loop body that trap DFS in endless cycles; many top-level branches each covering independent code; branches with exponentially expensive solver constraints |
| NURS (all) | ~7 | Several independent code regions each needing exploration; a shared variable gains constraints at each stage; all paths merge then branch again | Rarely worst — safest overall choice |
| nurs:qc | 3 (unique) | Some paths have expensive constraints (modular arithmetic) while other paths reach equally novel code with cheap constraints | Same instructions compute different values via bitfield/accumulator — coverage signal is useless (exp82: worst NURS at 148) |
| random-path | 2 | Chains of array lookups where each step’s index is symbolic — random sampling visits more diverse chains than coverage-guided selection (weak margins) | Mutually recursive functions (exp81); paths that merge then branch again (exp87) |
| BFS | 4 | A branch point forks into many states that all run the same handler code, but each fork produces different downstream behavior — coverage heuristics deprioritize the forks, but BFS explores all of them equally at each depth | Programs that fork many states between each coverage layer; programs requiring many sequential steps; high memory from storing all pending states |
| default | 0 unique | Decent all-rounder — inherits RP+covnew traits | Paths that merge then branch again — RP component carries stale priorities from before the merge (exp87: 260 vs NURS 299) |

---

## Design Principles for Discriminating Experiments

1. **Time pressure is essential.** Programs must fill the timeout budget. Anything that completes in <1s never discriminates.
2. **Where useless branches sit relative to target code decides DFS vs others.** Useless branches BEFORE targets = DFS wins (it ignores them by committing to one path). Useless branches AFTER targets = breadth-first wins (it reaches the targets before encountering the branches).
3. **Same code computing different values (bitfields, accumulators, registers) breaks coverage heuristics.** Coverage-based searchers see "already covered" and deprioritize, but the computed values differ across paths. This is the #1 way to split all 7 searchers.
4. **Mixing expensive and cheap constraints on different branches splits qc from covnew/md2u.** Put modular-arithmetic or non-linear constraints on some paths and simple equality checks on others. covnew/md2u treat both as equally novel; qc avoids the expensive ones.
5. **Loops with jumps back into the middle of the loop body trap DFS.** DFS follows these cycles endlessly, re-entering the same code without making progress.
6. **When all paths merge at one point, covnew loses its signal.** After the merge, every state has the same coverage footprint, so covnew cannot tell which post-merge branch has uncovered code.
7. **30s timeouts >> 10s timeouts.** Longer runs let scheduling differences accumulate.
8. **BFS wins when a branch forks into many states running identical code, but each fork changes internal state that matters later.** Coverage heuristics see "already covered" and deprioritize, but BFS explores all forks at each depth equally. The fork count must be large enough (≥16) to prevent full exploration.
9. **BFS is worst when the program forks many states between coverage layers.** BFS holds all states from all layers in memory simultaneously, causing memory pressure and slowdown.
10. **Strict BFS wins are inherently rare.** covnew/md2u subsume most of BFS's advantages. BFS only wins when coverage heuristics are actively misled (all collisions run the same handler code, so "already covered" deprioritization is wrong) AND the state space is in a narrow size range — too large to explore fully, but not so large that BFS drowns in pending states.

---

## Detailed Results by Wave

### Non-Discriminators (47 experiments — all searchers identical)

These experiments completed fully within the time budget, making all searchers equivalent. Listed for reference.

| Exp | Name | CovI | States | Time | Notes |
|-----|------|------|--------|------|-------|
| exp01 | Deep vs Shallow | 122 | 57 | <0.5s | Too small |
| exp02 | Needle in Haystack | 81 | 40 | <0.1s | Too small |
| exp03 | Symbolic Loop | 41 | 47 | <0.1s | Bounded loop |
| exp04 | Diamond | 57 | 16 | <0.1s | Classic diamond CFG |
| exp05 | Lopsided Tree | 31 | 11 | <0.1s | Trivially small |
| exp06 | State Merging Stress | 81 | 22 | <0.7s | Too few states |
| exp07 | State Explosion | 107 | 3,125 | <0.7s | All explore fully |
| exp08 | Staircase | 112 | 9 | <0.1s | Trivially small |
| exp09 | Coverage Plateau | 57 | 4 | <0.1s | Trivially small |
| exp10 | Hot vs Cold | 128 | 84 | <0.2s | Too small |
| exp16 | Solver Gradient | 176 | 56 | <0.2s | Too small |
| exp17 | Progressive Unlock | 180 | 16,384 | 2–2.5s | All complete |
| exp20 | Narrow Deep Maze | 52 | 41 | <0.2s | Too small |
| exp25 | Solver Timeout | 122 | 514 | <0.3s | Too small |
| exp26 | Heavy Hash | 122 | 514 | 3.6s | Solver-dominated but small |
| exp27 | Function Pointer | 120 | 13 | <0.3s | Trivially small |
| exp29 | Switch Maze | 179 | 1,054 | <0.8s | All complete |
| exp30 | Symbolic Index | 55 | 2 | <0.4s | Only 2 states |
| exp31 | Nested Loops | 96 | 43K–81K | 10–20s | Same coverage despite different state counts |
| exp35 | Tightening | 123 | 24,576 | 3.8–4.3s | All complete |
| exp36 | Dependent Vars | 123 | 16 | <0.1s | Trivially small |
| exp43 | QC Exploit | 84.62% | 11 | <0.1s | Too small |
| exp44 | MD2U Sniper | — | — | <4s | Too small |
| exp46 | CovNew Trap | — | — | — | Too small |
| exp47 | Adversarial Ordering | — | — | — | Too small |
| exp48 | Cascade Unlock | — | — | — | Too small |
| exp50 | Symbolic Struct | 191 | 17 | <0.1s | Too small |
| exp52 | Array OOB Hunt | 169 | 1,808 | — | Too small |
| exp54 | Diamond Lattice | 202 | 1,296 | — | Too small |
| exp55 | Symbolic Memcpy | 172 | 11 | — | Too small |
| exp56 | Error Cascade | 215 | 17,041 | — | Too small |
| exp58 | Phase Transition | 179 | 320 | — | Too small |
| exp59 | Constraint Reuse | 190 | 12 | — | Too small |
| exp60 | Infeasible Maze | 153 | 25 | — | Too small |
| exp65 | Back Edges | 119 | 177 | — | Too small |
| exp66 | Constraint Entangle | 168 | 40 | — | Too small |
| exp67 | Conditional Loop | 108 | 1,022 | — | Too small |
| exp69 | Delayed Reward | 120 | 16 | — | Too small |
| exp86 | Duff's Device | 238 | 130,560 | — | All explore fully |
| exp88 | Symbolic-Length Pipeline | 228 | 65,280 | — | Bounded, fully explorable |
| exp89 | Hash Inversion | 270 | 4,608 | ~2s | Z3 trivially inverts |
| exp90 | Allocation Pattern | 404 | 1,536 | ~11s | Memory alloc doesn't affect scheduling |
| exp92 | Infeasible Lure | 180 | 224 | <0.2s | Too small |
| exp94 | State Flood Trap | 312 | 129 | <0.1s | Too small |
| exp95 | Anti-Heuristic | 436 | 1,616 | ~1s | Fully explored |
| exp104 | Write Then Dispatch | 239 | 72 | ~1.6s | Too small |
| exp105 | Constraint Diversity | 254 | 64 | <0.3s | Too small |
| exp107 | Minimal BFS Win | 229 | 8,192 | ~3.5s | Fully explored |
| exp108 | State Machine | 168 | 1,024 | ~3.7s | All explored |
| exp109 | Index Cascade | 233 | 512 | ~1.4s | Fully explored |
| exp110 | Collision Branch | 318 | 41 | ~1.5s | Fully explored |
| exp111 | Write-Read Interleave | 241 | 42 | ~1s | Too few states |
| exp112 | Double Write | 217 | 16 | <0.4s | Too small |
| exp113 | Symbolic Permutation | 242 | 16 | <0.2s | Too small |
| exp114 | 5 Writes 3 Reads | 163 | 128 | ~5.5s | Fully explored |
| exp116 | More Reads | 234 | 64 | ~2s | Fully explored |
| exp117 | Valued Collision | 187 | 40 | ~2s | Fully explored |
| exp118 | Cross Tables | 226 | 64 | ~1.7s | Fully explored |
| exp119 | Unique Handlers | 277 | 32 | <1s | Fully explored |
| exp122 | More Writes | 145 | 128 | ~5.5s | Fully explored |

### Wave 6 Discriminators (exp39–exp48, 10s timeout)

#### exp39: Inverse Funnel
Diverge then converge through a funnel.

| Searcher | ICov% | Completed | Time (s) | MaxStates |
|----------|-------|-----------|----------|-----------|
| DFS | 100 | 60,928 | 1.51 | 13 |
| BFS | 96.15 | 10,304 | 474.80 | 129,868 |
| random-path | 100 | 2,006 | 367.19 | 93,512 |
| nurs:qc | 100 | 17,662 | 227.31 | 56,120 |
| nurs:covnew | 100 | 18,029 | 228.37 | 56,339 |
| nurs:md2u | 100 | 18,203 | 228.67 | 56,380 |
| default | 100 | 2,890 | 324.77 | 81,026 |

DFS finishes 300× faster than BFS. Same coverage for most, but BFS stuck at 96%.

#### exp40: Priority Inversion
See [Top Discriminators, #3](#3-exp40--priority-inversion-dfs-100-vs-others-447).

#### exp41: BFS Paradise
Ironic result — DFS (100%) beats the "BFS paradise" (94.4% for all others).

#### exp42: Random Path Wins
DFS/NURS reach 100%, BFS/random-path/default stuck at 94.4%.

#### exp45: Memory Cliff

| Searcher | ICov% |
|----------|-------|
| DFS / BFS | 96.43 |
| nurs:covnew / md2u | 75.00 |
| random-path / qc / default | 71.43 |

DFS/BFS (96.4%) > covnew/md2u (75%) > random-path/qc/default (71.4%).

### Wave 7 Discriminators (exp49–exp60, 10s timeout)

#### exp49: Goto Spaghetti
8-state machine via switch in a loop (6 iterations), 8^6 = 262K paths. Same coverage (121 CovI) for all, but vastly different state management:

| Searcher | TermExit | TermEarly | States |
|----------|----------|-----------|--------|
| DFS | 95,428 | 25 | 95,453 |
| BFS | 45,386 | 216,758 | 262,144 |
| NURS variants | ~9,500 | ~143K | ~152K |

Differentiates on efficiency, not coverage.

#### exp51: Coroutine Ping-Pong
DFS (328) < all others (344). DFS misses coroutine paths.

#### exp53: Recursive Descent
Same coverage (131) but NURS variants terminate early on ~5K states while DFS/BFS/default complete all 11,747.

#### exp57: Pointer Chase
random-path/default/qc (178) > DFS (176) > BFS/covnew/md2u (166). Interesting covnew/md2u penalty.

### Wave 8 Discriminators (exp61–exp70, 10s timeout)

#### exp61: Scaled Pointer Chase
random-path/default (268) > DFS (260) > NURS (251).

#### exp62: Asymmetric Cost Tree
8 levels of cheap-left vs expensive-right subtrees. All non-DFS (534) >> DFS (416). DFS commits to one subtree and misses 22% of coverage.

#### exp63: Fibonacci Tree
DFS (204) < all others (212). Fibonacci branching traps DFS.

#### exp64: Breadcrumb Trail
DFS (244) >> all others (120). DFS follows the linear trail; others scatter.

#### exp68: Fork Bomb Decoy
DFS slightly better (188 vs 185). Weak discriminator.

#### exp70: Multi-Objective
See [Top Discriminators](#top-10-discriminators-summary). NURS (360) >> DFS (292) >> default (198) >> RP (170) >> BFS (147).

### Wave 9 Discriminators (exp71–exp80, 10s timeout)

#### exp71: Solver Cost Cliff
DFS stuck at 219; all others reach 312. DFS dives deep into expensive constraints.

#### exp72: Distance vs Novelty
DFS (234) >> all others (123). Nearly 2× coverage.

#### exp73: Symbolic Write
BFS strict win. See [BFS Strict Wins](#bfs-strictly-better-than-all-others).

#### exp74: Longjmp Simulation
DFS (276) > NURS (266–267) > BFS (181) > default (169) > RP (162). Non-local goto jumps favor deep exploration, with slight NURS split: covnew (267) > qc/md2u (266).

#### exp75: Symbolic Sort
DFS (305) >> all others (120). 2.5× coverage advantage.

#### exp76: Aliased Pointers
DFS (253) >> all others (163).

#### exp77: Constraint Explosion
Shared variable x accumulates constraints across stages with noise bytes between.

| Searcher | CovI | FullBranch |
|----------|------|-----------|
| NURS (all) | 342 | 30 |
| DFS | 273 | 28 |
| default | 274 | 23 |
| random-path | 266 | 22 |
| BFS | 249 | 22 |

NURS (342) >> DFS (273) >> BFS (249). All NURS variants identical.

#### exp78: Tournament
DFS (281) >> all others (120). Deep sequential comparison tree.

#### exp79: Taint Propagation
DFS (210) > NURS (191) > RP (172) > BFS (142). Moderate spread.

#### exp80: Island Archipelago
16 coverage islands behind 4 difficulty tiers. DFS (367) >> NURS (276) >> default (165) >> BFS (156) >> RP (152). All NURS variants identical.

### Wave 10 Discriminators (exp81–exp90, 30s timeout)

#### exp81: Mutual Recursion Web
Two mutually recursive functions, 12 symbolic bytes, 6 levels.

| Searcher | CovI | TermExit | States | Wall (s) |
|----------|------|----------|--------|----------|
| DFS / BFS / NURS | 217 | varies | 760–251K | 30–89 |
| random-path / default | 182 | 0 | 176K–227K | 72–85 |

Splits random-path/default (182) from the pack (217). DFS absurdly efficient: 760 states, 17K queries.

#### exp82: Bitfield FSM
See [Top Discriminators, #1](#1-exp82--bitfield-fsm-all-7-searchers-distinct). The crown jewel — all 7 searchers produce different CovI.

#### exp83: Symbolic Division Cascade
Pipeline of divisions/modulos by symbolic values. NURS (290, only 1 uncovered!) >> DFS (274) >> default (271) >> RP (268) >> BFS (261).

#### exp84: Knotted CFG
Loops with jumps back into the middle of the loop body. All non-DFS (216) >> DFS (184). DFS executes 37.5M instructions but explores only 47 states — trapped cycling through these loops.

#### exp85: Poison Path
See [Top Discriminators, #2](#2-exp85--poison-path-qc-split-from-covnewmd2u). First nurs:qc > covnew/md2u split.

#### exp87: Convergent-Divergent Pipeline
Phase 1 diverge → Phase 2 converge → Phase 3 diverge again.

| Searcher | CovI | States | Wall (s) |
|----------|------|--------|----------|
| NURS (all) | 299 | ~283K | ~100 |
| DFS | 279 | 155K | 30.0 |
| BFS | 275 | 459K | 147.1 |
| default | 260 | 286K | 101.1 |
| random-path | 245 | 323K | 111.0 |

NURS navigates past the convergence point where coverage signals are misleading.

### Wave 11 Discriminators (exp91–exp102, 10s timeout) — BFS Advantage Hunt

12 experiments specifically designed to find BFS strict wins. **Result: no strict BFS wins found.**

#### exp91: Shallow Spread
All non-DFS (346) >> DFS (171). BFS ties with NURS/RP/default.

#### exp93: Breadth Lottery
All non-DFS (500) >> DFS (164). BFS ties with all others.

#### exp96: Layered Breadth
NURS (208) > DFS (198) > default (193) > RP (171) > BFS (155). BFS WORST — state explosion between layers kills it.

#### exp97: Symmetric Siblings
All non-DFS (386) >> DFS (153). BFS ties.

#### exp98: Write Scatter

| Searcher | CovI |
|----------|------|
| DFS / BFS / RP / qc | 261 |
| default | 258 |
| covnew / md2u | 244 |

Closest to a BFS win — BFS ties DFS/qc/RP but can't beat them. covnew/md2u penalized by symbolic writes.

#### exp99: Cascade Breadth
DFS (357) > NURS (333) > default (282) > RP (269) > BFS (245). BFS worst again.

#### exp100: Solver Quicksand
All non-DFS (233) >> DFS (161). DFS trapped in expensive queries.

#### exp101: Memory Pressure BFS
DFS (491) >> NURS (461–462) >> RP/default (356–365) >> BFS (335). BFS worst.

#### exp102: BFS Pipeline
NURS (430) > DFS (412) >> default (345) >> RP (310) >> BFS (297). BFS worst.

**Wave 11 takeaway**: BFS was WORST in 4 experiments (exp96, exp99, exp101, exp102) and merely tied-best in 4 others. Programs with state explosion between layers are BFS's worst case.

### Wave 12 Discriminators (exp103–exp113, 10s timeout)

#### exp103: Multi-Table Write
4 sub-problems via switch, each with 3 symbolic writes. All non-DFS (348) >> DFS (234). BFS ties.

#### exp106: Hash Map Simulation
6-slot hash table, 5 symbolic key inserts. nurs:qc (363) slightly edges BFS/others (361). DFS worst (322).

**Wave 12 takeaway**: 8 of 11 experiments were non-discriminators — too small to differentiate. Symbolic writes alone are insufficient for BFS wins.

### Wave 13 Discriminators (exp114–exp123, 10s timeout) — exp73 Variants

Systematic parameter sweeps of the symbolic-write-to-table pattern (the only known BFS niche).

#### exp115: Small Table (8 elements)
nurs:qc wins (182) — small table helps qc more than BFS. DFS/BFS tie at 172. covnew/md2u/RP/default at 167.

#### exp120: Big Table (32 elements) — BFS Win
BFS (172) > all others (167) > DFS (162). Same CovI structure as exp73.

#### exp121: No Symbolic Values — BFS Win
BFS (164) > all others (159) > DFS (154). Confirms BFS advantage comes from write INDEX diversity, not value diversity.

#### exp123: Max Handlers — Strongest BFS Win
BFS (277) > all others (272) > DFS (237). Unique handlers amplify total coverage range but BFS margin stays at +5.

**Wave 13 takeaway**: 3 of 10 experiments produced BFS strict wins — the most productive wave. The BFS advantage is exactly +5 CovI in all 4 known wins (exp73, exp120, exp121, exp123).

### Waves 1–5 Discriminators (exp01–exp38, 10s timeout)

These early experiments had mostly small margins due to undersized programs.

#### exp11: Wide Then Deep
DFS (122) > all others (116). DFS finishes paths; others timeout with states stuck.

#### exp12: Trapdoor
DFS (95) > all others (91). DFS gets slightly more coverage by completing paths.

#### exp13: Asymmetric
DFS/NURS (153) > BFS/RP/default (150).

#### exp15: Distance
DFS/NURS (103) > BFS/RP/default (97). Same split as exp13.

#### exp18: Shallow Bug
All non-DFS (82) > DFS (78). BFS finds shallow targets DFS bypasses.

#### exp19: Solver Cost War
All non-DFS (91) >> DFS (67). Solver cost asymmetry traps DFS.

#### exp21: Interleaved Hot/Cold

| Searcher | CovI |
|----------|------|
| NURS (all) | 295 |
| DFS | 269 |
| default | 231 |
| random-path | 217 |
| BFS | 207 |

NURS (295) >> DFS (269) >> default (231) >> RP (217) >> BFS (207).

#### exp22: State Pressure
DFS (149) > all others (139).

#### exp23: CovNew vs MD2U
DFS/NURS (163) > BFS/RP/default (157).

#### exp24: Interleaved Default
DFS (147) ≈ NURS (146) > default (134) > RP (129) > BFS (123).

#### exp28: Reconvergence
DFS/NURS (135) > BFS/RP/default (120).

#### exp32: Coverage Cliff
Barely distinguishable. Small margins.

#### exp33: DFS Trap
DFS/NURS (125) > BFS/RP/default (122).

#### exp34: Time to Coverage
All non-DFS (209) >> DFS (114). Strong — DFS arrives too late.

#### exp37: Starvation
DFS (126) > all others (123).

#### exp38: Mixed Depth
All non-DFS (269) >> DFS (137). Strong — DFS misses shallow targets.
