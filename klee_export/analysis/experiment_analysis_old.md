# KLEE Search Heuristic Experiment Analysis

All experiment results and analysis are logged here. Updated after each wave.

**Searchers tested**: DFS, BFS, random-path, NURS:qc, NURS:covnew, NURS:md2u, default (random-path interleaved with NURS:covnew)

**Key metrics**: CoveredInstr (CovI), UncoveredInstr (UnI), Queries (Q), WallTime (ms), SolverTime (ms), NumStates, TermExit, TermEarly, TermProgramError

---

## Waves 1–5 (exp01–exp38, 10s timeout, re-run with full 7-searcher suite)

### Non-Discriminators (all searchers identical)

| Experiment | CovI | States | Time | Notes |
|---|---|---|---|---|
| exp01: Deep vs Shallow | 122 | 57 | <0.5s | Too small — 57 states fully explored |
| exp02: Needle in Haystack | 81 | 40 | <0.1s | Too small |
| exp03: Symbolic Loop | 41 | 47 | <0.1s | Bounded loop |
| exp04: Diamond | 57 | 16 | <0.1s | Classic diamond CFG |
| exp05: Lopsided Tree | 31 | 11 | <0.1s | Trivially small |
| exp06: State Merging Stress | 81 | 22 | <0.7s | Too few states |
| exp07: State Explosion | 107 | 3,125 | <0.7s | All explore fully |
| exp08: Staircase | 112 | 9 | <0.1s | Trivially small |
| exp09: Coverage Plateau | 57 | 4 | <0.1s | Trivially small |
| exp10: Hot vs Cold | 128 | 84 | <0.2s | Too small |
| exp16: Solver Gradient | 176 | 56 | <0.2s | Too small |
| exp17: Progressive Unlock | 180 | 16,384 | 2–2.5s | All complete |
| exp20: Narrow Deep Maze | 52 | 41 | <0.2s | Too small |
| exp25: Solver Timeout | 122 | 514 | <0.3s | Too small |
| exp26: Heavy Hash | 122 | 514 | 3.6s | Solver-dominated but small |
| exp27: Function Pointer | 120 | 13 | <0.3s | Trivially small |
| exp29: Switch Maze | 179 | 1,054 | <0.8s | All complete |
| exp30: Symbolic Index | 55 | 2 | <0.4s | Only 2 states! |
| exp31: Nested Loops | 96 | 43K–81K | 10–20s | Same coverage despite different state counts |
| exp35: Tightening | 123 | 24,576 | 3.8–4.3s | All complete |
| exp36: Dependent Vars | 123 | 16 | <0.1s | Trivially small |

> [!NOTE]
> 21 of 38 early experiments are non-discriminators — most were too small to create scheduling pressure.

---

### Discriminators

#### exp11: Wide Then Deep

| Searcher | CovI | TermExit | States |
|---|---|---|---|
| **DFS** | **122** | 49,159 | 49,170 |
| All others | 116 | 7 | 100K–158K |

**Verdict**: ⚡ Weak discriminator. DFS (122) > all others (116). DFS finishes paths; others timeout with states stuck.

---

#### exp12: Trapdoor

| Searcher | CovI | TermExit | States |
|---|---|---|---|
| **DFS** | **95** | 48,272 | 48,290 |
| All others | 91 | 1 | 103K–163K |

**Verdict**: ⚡ Weak discriminator. DFS (95) > all others (91). DFS gets slightly more coverage by completing paths.

---

#### exp13: Asymmetric

| Searcher | CovI |
|---|---|
| DFS / NURS | **153** |
| BFS / RP / default | 150 |

**Verdict**: ⚡ Weak discriminator. DFS and all NURS (153) > BFS/RP/default (150).

---

#### exp15: Distance

| Searcher | CovI |
|---|---|
| DFS / NURS | **103** |
| BFS / RP / default | 97 |

**Verdict**: ⚡ Weak discriminator. Same split as exp13.

---

#### exp18: Shallow Bug 

| Searcher | CovI |
|---|---|
| All non-DFS | **82** |
| **DFS** | **78** |

**Verdict**: ⚡ Weak — BFS advantage. DFS (78) < all others (82). BFS finds the shallow targets that DFS bypasses.

---

#### exp19: Solver Cost War 🏆

| Searcher | CovI | Queries | States |
|---|---|---|---|
| All non-DFS | **91** | 1.3K–2.9K | 69–202 |
| **DFS** | **67** | 606 | 40 |

**Verdict**: 🏆 **Strong discriminator**. DFS (67) << all others (91). DFS gets trapped in expensive solver queries (11.6s solver time in 10s wall). All others find cheaper paths to higher coverage. All non-DFS are solver-dominated (~10s solver) but with more queries and states.

---

#### exp21: Interleaved Hot/Cold 🏆

| Searcher | CovI | States |
|---|---|---|
| **NURS (qc/covnew/md2u)** | **295** | 101K–102K |
| DFS | 269 | 46,941 |
| default | 231 | 106,626 |
| random-path | 217 | 124,851 |
| BFS | 207 | 165,583 |

**Verdict**: 🏆 **Strong discriminator**. NURS (295) >> DFS (269) >> default (231) >> RP (217) >> BFS (207). Interleaved hot/cold code is exactly the structure NURS is designed for — it correctly prioritizes hot (novel) code over cold (already-covered) noise. However, all three NURS variants identical.

---

#### exp22: State Pressure

| Searcher | CovI | TermErr |
|---|---|---|
| **DFS** | **149** | 52,320 |
| All others | 139 | 0 |

**Verdict**: ⚡ Moderate. DFS (149) > others (139). DFS is the only searcher that triggers program errors (52K!), finding more edge cases through deep exploration.

---

#### exp23: CovNew vs MD2U

| Searcher | CovI |
|---|---|
| DFS / NURS | **163** |
| BFS / RP / default | 157 |

**Verdict**: ⚡ Weak discriminator. Same DFS/NURS vs BFS/RP/default split. Failed to differentiate covnew from md2u.

---

#### exp24: Interleaved Default

| Searcher | CovI |
|---|---|
| **DFS** | **147** |
| NURS (qc/covnew/md2u) | 146 |
| default | 134 |
| random-path | 129 |
| BFS | 123 |

**Verdict**: ⚡ Moderate. Clear stratification: DFS ≈ NURS (147/146) > default (134) > RP (129) > BFS (123).

---

#### exp28: Reconvergence

| Searcher | CovI |
|---|---|
| DFS / NURS | **135** |
| BFS / RP / default | 120 |

**Verdict**: ⚡ Moderate. DFS and NURS (135) > BFS/RP/default (120). After reconvergence, DFS and NURS push further than breadth-first approaches.

---

#### exp32: Coverage Cliff

| Searcher | CovI |
|---|---|
| All non-DFS | **139** |
| DFS | 138 |

**Verdict**: ❌ Barely distinguishable. DFS misses 1 instruction — functionally a non-discriminator.

---

#### exp33: DFS Trap

| Searcher | CovI |
|---|---|
| DFS / NURS | **125** |
| BFS / RP / default | 122 |

**Verdict**: ⚡ Weak. Designed as a "DFS trap" but DFS actually wins (125 > 122). The trap is too small.

---

#### exp34: Time to Coverage 🏆

| Searcher | CovI | States |
|---|---|---|
| All non-DFS | **209** | 91K–140K |
| **DFS** | **114** | 42,023 |

**Verdict**: 🏆 **Strong discriminator — BFS advantage!** BFS/NURS/RP/default (209) >> DFS (114). Nearly 2× coverage for non-DFS. The "time to coverage" structure has breadth-first targets that DFS misses entirely.

---

#### exp37: Starvation

| Searcher | CovI |
|---|---|
| DFS | **126** |
| All others | 123 |

**Verdict**: ⚡ Weak discriminator. DFS (126) > others (123). Marginal.

---

#### exp38: Mixed Depth 🏆

| Searcher | CovI | States |
|---|---|---|
| All non-DFS | **269** | 83K–135K |
| **DFS** | **137** | 44,144 |

**Verdict**: 🏆 **Strong discriminator — BFS advantage!** Non-DFS (269) >> DFS (137). DFS misses 49% of the coverage. The mixed-depth structure has many independent wide targets that DFS's sequential exploration completely misses.

---

### Waves 1–5 Summary

Out of 38 experiments:
- **21 non-discriminators** (too small / too simple)
- **3 strong discriminators**: exp19 (solver cost), exp34 (time to coverage), exp38 (mixed depth)
- **1 strong NURS discriminator**: exp21 (interleaved hot/cold)
- **13 weak/moderate discriminators**: small splits, usually DFS vs all

**Key finding**: The early experiments were overwhelmingly too small. Programs with <1000 states or <1s runtime never differentiate searchers; KLEE fully explores them regardless of strategy.

---

## Wave 6 (exp39–exp48, 10s timeout)

> [!NOTE]
> Wave 6 used a different CSV schema with ICov% instead of raw CoveredInstr. Results below use that format.

### exp39: Inverse Funnel

**Structure**: Diverge then converge through a funnel. Noise amplifies states.

| Searcher | ICov% | Completed | Time (s) | MaxStates |
|---|---|---|---|---|
| DFS | 100 | 60,928 | 1.51 | 13 |
| BFS | 96.15 | 10,304 | 474.80 | 129,868 |
| random-path | 100 | 2,006 | 367.19 | 93,512 |
| nurs:qc | 100 | 17,662 | 227.31 | 56,120 |
| nurs:covnew | 100 | 18,029 | 228.37 | 56,339 |
| nurs:md2u | 100 | 18,203 | 228.67 | 56,380 |
| default | 100 | 2,890 | 324.77 | 81,026 |

**Verdict**: ⚡ Moderate discriminator. DFS finishes 300× faster than BFS. Same coverage for most, but BFS stuck at 96%. NURS variants identical.

---

### exp40: Priority Inversion

**Structure**: 3 noise bytes (2^24 paths) followed by 10 code "islands" behind equality checks on a key byte. Designed to favor covnew.

| Searcher | ICov% | Completed | Time (s) | MaxStates |
|---|---|---|---|---|
| **DFS** | **100** | 46,612 | 1.59 | 28 |
| BFS | 44.74 | 0 | 586.68 | 140,337 |
| random-path | 44.74 | 0 | 411.72 | 91,773 |
| nurs:qc | 44.74 | 0 | 344.29 | 78,782 |
| nurs:covnew | 44.74 | 0 | 336.82 | 77,074 |
| nurs:md2u | 44.74 | 0 | 340.82 | 78,001 |
| default | 44.74 | 0 | 344.60 | 75,540 |

**Verdict**: 🏆 **Strong discriminator**. DFS 100% vs all others 44.7%. The 2^24 noise "sea" comes BEFORE the islands, so non-DFS searchers drown in managing 75K–140K pending sea states and never complete any paths through to the islands. DFS commits to one sea path, finishes it end-to-end, and backtracks efficiently.

**Key insight**: Noise placement matters enormously. Noise BEFORE targets = DFS wins (it ignores the noise structurally). Noise AFTER targets would favor covnew.

---

### exp41: BFS Paradise

**Structure**: Designed to favor BFS with wide shallow coverage targets.

| Searcher | ICov% | Completed | Time (s) |
|---|---|---|---|
| **DFS** | **100** | 44,932 | 1.67 |
| All others | 94.44 | 0 | 268–528 |

**Verdict**: ⚡ Moderate. Ironic result — DFS beats the "BFS paradise." Same pattern as exp40.

---

### exp42: Random Path Wins

**Structure**: Designed to favor random-path via tree-structured targets.

| Searcher | ICov% | Completed |
|---|---|---|
| DFS | 100 | 56,312 |
| nurs:qc | 100 | 103 |
| nurs:covnew | 100 | 136 |
| nurs:md2u | 100 | 113 |
| BFS | 94.44 | 0 |
| random-path | 94.44 | 0 |
| default | 94.44 | 0 |

**Verdict**: ⚡ Moderate. DFS and NURS reach 100%, BFS/random-path/default stuck at 94.4%.

---

### exp43: QC Exploit
**Verdict**: ❌ Non-discriminator. All identical (84.62% ICov, 11 completions, <0.1s). Too small.

### exp44: MD2U Sniper
**Verdict**: ❌ Non-discriminator. All complete in <4s. Too small.

### exp45: Memory Cliff

| Searcher | ICov% |
|---|---|
| DFS | 96.43 |
| BFS | 96.43 |
| nurs:covnew | 75.00 |
| nurs:md2u | 75.00 |
| random-path | 71.43 |
| nurs:qc | 71.43 |
| default | 71.43 |

**Verdict**: ⚡ Moderate. DFS/BFS (96.4%) > covnew/md2u (75%) > random-path/qc/default (71.4%).

### exp46: CovNew Trap
**Verdict**: ❌ Non-discriminator. All identical. Too small.

### exp47: Adversarial Ordering
**Verdict**: ❌ Non-discriminator. All identical. Too small.

### exp48: Cascade Unlock
**Verdict**: ❌ Non-discriminator. All identical. Too small.

---

## Wave 7 (exp49–exp60, 10s timeout)

### exp49: Goto Spaghetti

**Structure**: 8-state machine via switch in a loop (6 iterations). Symbolic input selects state. 8^6 = 262K paths.

| Searcher | CovI | TermExit | TermEarly | NumStates |
|---|---|---|---|---|
| DFS | 121 | 95,428 | 25 | 95,453 |
| BFS | 121 | 45,386 | 216,758 | 262,144 |
| random-path | 121 | 49,005 | 73,874 | 122,879 |
| nurs:qc | 121 | 9,406 | 142,530 | 151,936 |
| nurs:covnew | 121 | 9,607 | 143,260 | 152,867 |
| nurs:md2u | 121 | 9,559 | 143,063 | 152,622 |
| default | 121 | 29,215 | 85,208 | 114,423 |

**Verdict**: ⚡ Moderate. Same coverage (121 CovI) but vastly different state management. DFS terminates 95K paths; NURS variants only terminate ~9.5K. BFS explodes to 262K states. Differentiates on efficiency, not coverage.

---

### exp50: Symbolic Struct
**Verdict**: ❌ Non-discriminator. All identical (191 CovI, 17 completions, <0.1s).

### exp51: Coroutine Ping-Pong

| Searcher | CovI |
|---|---|
| DFS | 328 |
| All others | 344 |

**Verdict**: ⚡ Moderate. DFS (328) < all others (344). DFS misses coroutine paths.

### exp52: Array OOB Hunt
**Verdict**: ❌ Non-discriminator. All identical (169 CovI, 1808 states).

### exp53: Recursive Descent

| Searcher | CovI | TermExit | TermEarly |
|---|---|---|---|
| DFS | 131 | 11,747 | 0 |
| BFS | 131 | 11,747 | 0 |
| random-path | 131 | 11,747 | 0 |
| default | 131 | 11,747 | 0 |
| nurs:qc | 131 | 6,783 | 4,957 |
| nurs:covnew | 131 | 6,629 | 5,107 |
| nurs:md2u | 131 | 6,950 | 4,794 |

**Verdict**: ⚡ Moderate. Same coverage but NURS variants terminate early on ~5K states while DFS/BFS/default complete all. Differentiates on completion pattern, not coverage.

### exp54: Diamond Lattice
**Verdict**: ❌ Non-discriminator. All identical (202 CovI, 1296 states).

### exp55: Symbolic Memcpy
**Verdict**: ❌ Non-discriminator. All identical (172 CovI, 11 completions).

### exp56: Error Cascade
**Verdict**: ❌ Non-discriminator. All identical (215 CovI, 17041 states).

### exp57: Pointer Chase

| Searcher | CovI | Instructions | States |
|---|---|---|---|
| DFS | 176 | 1,934 | 57 |
| random-path | 178 | 9,827 | 568 |
| default | 178 | 13,893 | 589 |
| nurs:qc | 178 | 14,120 | 582 |
| BFS | 166 | 34,064 | 4,229 |
| nurs:covnew | 166 | 60,982 | 2,885 |
| nurs:md2u | 166 | 57,462 | 2,745 |

**Verdict**: ⚡ Moderate. random-path/default/qc (178) > DFS (176) > BFS/covnew/md2u (166). Interesting split where covnew/md2u perform worse than qc.

### exp58: Phase Transition
**Verdict**: ❌ Non-discriminator. All identical (179 CovI, 320 states).

### exp59: Constraint Reuse
**Verdict**: ❌ Non-discriminator. All identical (190 CovI, 12 completions).

### exp60: Infeasible Maze
**Verdict**: ❌ Non-discriminator. All identical (153 CovI, 25 completions).

---

## Wave 8 (exp61–exp70, 10s timeout)

### exp61: Scaled Pointer Chase

| Searcher | CovI |
|---|---|
| random-path | 268 |
| default | 268 |
| DFS | 260 |
| All NURS | 251 |

**Verdict**: ⚡ Moderate. random-path/default (268) > DFS (260) > NURS (251).

---

### exp62: Asymmetric Cost Tree

**Structure**: 8 levels of left (cheap, 1 branch) vs right (expensive, 4 branches per level). 8 noise bytes fuel right-side branching.

| Searcher | CovI | TermExit | States |
|---|---|---|---|
| **All non-DFS** | **534** | 3K–145K | 87K–153K |
| **DFS** | **416** | 37,451 | 37,480 |

**Verdict**: 🏆 **Strong discriminator**. DFS misses 22% of coverage by committing to one subtree. All others explore both cheap and expensive sides. Notably: all NURS variants are identical (534 CovI).

---

### exp63: Fibonacci Tree

| Searcher | CovI |
|---|---|
| DFS | 204 |
| All others | 212 |

**Verdict**: ⚡ Moderate. DFS (204) < all others (212). Fibonacci branching traps DFS.

---

### exp64: Breadcrumb Trail

| Searcher | CovI |
|---|---|
| DFS | 244 |
| All others | 120 |

**Verdict**: 🏆 **Strong discriminator**. DFS (244) >> all others (120). DFS follows the breadcrumb trail linearly; others spread out and miss the deep trail.

---

### exp65: Back Edges
**Verdict**: ❌ Non-discriminator. All identical (119 CovI, 177 completions).

### exp66: Constraint Entangle
**Verdict**: ❌ Non-discriminator. All identical (168 CovI, 40 completions).

### exp67: Conditional Loop
**Verdict**: ❌ Non-discriminator. All identical (108 CovI, 1022 completions).

### exp68: Fork Bomb Decoy

| Searcher | CovI | Queries |
|---|---|---|
| DFS | 188 | 571K |
| All others | 185 | 562K–1M |

**Verdict**: ⚡ Weak discriminator. DFS slightly better (188 vs 185 CovI).

---

### exp69: Delayed Reward
**Verdict**: ❌ Non-discriminator. All identical (120 CovI, 16 completions).

### exp70: Multi-Objective

| Searcher | CovI | Queries | States |
|---|---|---|---|
| **nurs:qc** | **360** | 531K | 61,553 |
| **nurs:covnew** | **360** | 533K | 61,773 |
| **nurs:md2u** | **360** | 514K | 59,510 |
| DFS | 292 | 518K | 30,453 |
| default | 198 | 580K | 62,670 |
| random-path | 170 | 710K | 69,171 |
| BFS | 147 | 1.1M | 125,656 |

**Verdict**: 🏆 **Strong discriminator**. NURS (360) >> DFS (292) >> default (198) >> random-path (170) >> BFS (147). NURS variants 2.4× better than BFS. However, all three NURS variants are nearly identical.

---

## Wave 9 (exp71–exp80, 10s timeout)

### exp71: Solver Cost Cliff

| Searcher | CovI | Queries |
|---|---|---|
| **DFS** | **219** | 444K |
| **All others** | **312** | 497K–768K |

**Verdict**: 🏆 **Strong discriminator**. DFS stuck at 219 CovI; all others reach 312. DFS falls off the "solver cost cliff" — it dives deep into expensive constraints and can't backtrack in time.

---

### exp72: Distance vs Novelty

| Searcher | CovI |
|---|---|
| **DFS** | **234** |
| All others | 123 |

**Verdict**: 🏆 **Strong discriminator**. DFS (234) >> all others (123). Nearly 2× coverage advantage for DFS through deep exploration.

---

### exp73: Symbolic Write

| Searcher | CovI |
|---|---|
| BFS | 172 |
| All others | 162–167 |

**Verdict**: ⚡ Weak discriminator. All solver-time-dominated (~10s solver out of ~10s wall). Marginal coverage differences.

---

### exp74: Longjmp Simulation

**Structure**: try/catch simulation via goto. 3 try blocks, 3 handlers, symbolic exception triggering. Non-local jumps create cross-edges.

| Searcher | CovI | TermExit | States |
|---|---|---|---|
| **DFS** | **276** | 49,587 | 49,602 |
| nurs:covnew | 267 | 30 | 85,553 |
| nurs:qc | 266 | 26 | 83,175 |
| nurs:md2u | 266 | 30 | 85,338 |
| BFS | 181 | 6,057 | 137,128 |
| default | 169 | 0 | 69,900 |
| random-path | 162 | 0 | 81,397 |

**Verdict**: 🏆 **Strong discriminator**. DFS (276) > NURS (266–267) > BFS (181) > default (169) > random-path (162). The non-local goto jumps favor DFS's deep exploration. Slight NURS split: covnew (267) > qc/md2u (266).

---

### exp75: Symbolic Sort

| Searcher | CovI |
|---|---|
| **DFS** | **305** |
| All others | 120 |

**Verdict**: 🏆 **Strong discriminator**. DFS 2.5× more coverage. DFS follows the sorting network deeply.

---

### exp76: Aliased Pointers

| Searcher | CovI |
|---|---|
| DFS | 253 |
| All others | 163 |

**Verdict**: 🏆 **Strong discriminator**. DFS (253) >> all others (163).

---

### exp77: Constraint Explosion

**Structure**: Shared variable x accumulates constraints (range checks, modular arithmetic, bit masking). Noise bytes between stages amplify states.

| Searcher | CovI | FullBranch |
|---|---|---|
| **nurs:qc** | **342** | 30 |
| **nurs:covnew** | **342** | 30 |
| **nurs:md2u** | **342** | 30 |
| DFS | 273 | 28 |
| default | 274 | 23 |
| random-path | 266 | 22 |
| BFS | 249 | 22 |

**Verdict**: 🏆 **Strong discriminator** (NURS vs others). NURS (342) >> DFS (273) >> BFS (249). But all three NURS variants are identical — no split.

---

### exp78: Tournament
**Verdict**: 🏆 Strong DFS (281 CovI) >> all others (120 CovI). Pattern: deep sequential comparison tree.

### exp79: Taint Propagation

| Searcher | CovI |
|---|---|
| DFS | 210 |
| nurs:qc/covnew/md2u | 191 |
| random-path | 172 |
| default | 191 |
| BFS | 142 |

**Verdict**: ⚡ Moderate. DFS (210) > NURS (191) > random-path (172) > BFS (142).

---

### exp80: Island Archipelago

**Structure**: 16 coverage islands behind 4 difficulty tiers (equality, range, modular, multi-variable). 2 noise bytes.

| Searcher | CovI |
|---|---|
| **DFS** | **367** |
| nurs:qc | 276 |
| nurs:covnew | 276 |
| nurs:md2u | 276 |
| default | 165 |
| BFS | 156 |
| random-path | 152 |

**Verdict**: 🏆 **Strong discriminator**. DFS (367) >> NURS (276) >> BFS (156). But NURS variants are identical.

---

## Wave 10 (exp81–exp90, 30s timeout)

### exp81: Mutual Recursion Web

**Structure**: Two mutually recursive functions (f→g→f→...), each branching on a fresh symbolic byte per recursion level. 12 symbolic bytes total (2 per level, 6 levels). Creates exponential constraint histories.

| Searcher | CovI | UnI | TermExit | TermEarly | States | Queries | Wall (s) | Solver (s) |
|---|---|---|---|---|---|---|---|---|
| **DFS** | **217** | 12 | 744 | 16 | 760 | 17K | 30.5 | 30.1 |
| **nurs:qc** | **217** | 12 | 14 | 145,503 | 145,517 | 2.1M | 64.3 | 16.1 |
| **nurs:covnew** | **217** | 12 | 8 | 146,854 | 146,862 | 2.1M | 64.7 | 15.7 |
| **nurs:md2u** | **217** | 12 | 8 | 146,258 | 146,266 | 2.1M | 64.2 | 15.5 |
| BFS | 217 | 12 | 56 | 251,068 | 251,124 | 2.9M | 89.1 | 30.7 |
| random-path | **182** | 47 | 0 | 227,183 | 227,183 | 3.0M | 85.2 | 18.4 |
| default | **182** | 47 | 0 | 176,403 | 176,403 | 2.6M | 71.6 | 14.6 |

**Verdict**: 🏆 **Strong discriminator** — splits random-path/default from the pack.
- DFS/BFS/NURS: 217 CovI | random-path/default: 182 CovI (−16%)
- DFS absurdly efficient: only 760 states, 17K queries, fully solver-bound (30s solver in 30s wall)
- NURS variants identical on coverage, but generate 146K states with 64s wall time
- random-path's tree model can't represent the mutual recursion call graph

---

### exp82: Bitfield FSM ⭐

**Structure**: 8 symbolic operations on a uint16_t bitfield (set/clear/toggle/conditional-set on symbolic bit positions). Final `check_state()` has 16 equality checks for specific bit patterns. The same instructions execute for all paths but compute different logical states.

| Searcher | CovI | UnI | FullBr | PartBr | TermExit | States | Queries | Wall (s) | Solver (s) |
|---|---|---|---|---|---|---|---|---|---|
| **DFS** | **216** | 43 | 8 | **12** | 578 | 600 | 24K | 30.3 | 29.9 |
| **random-path** | **210** | 49 | 7 | **13** | 3 | 8,993 | 124K | 34.8 | 30.5 |
| **default** | **179** | 80 | 8 | 5 | 0 | 8,491 | 120K | 32.3 | 27.1 |
| **nurs:covnew** | **154** | 105 | 4 | **6** | 0 | 11,471 | 177K | 32.4 | 26.2 |
| **nurs:md2u** | **150** | 109 | 4 | 5 | 0 | 10,873 | 167K | 32.3 | 25.7 |
| **nurs:qc** | **148** | 111 | 4 | 5 | 0 | 10,800 | 165K | 32.3 | 25.8 |
| **BFS** | **107** | 152 | 1 | 2 | 0 | 26,714 | 250K | 35.1 | 29.6 |

**Verdict**: 🏆🏆🏆 **BEST DISCRIMINATOR — All 7 searchers produce different CovI!**

DFS (216) >> random-path (210) >> default (179) >> covnew (154) > md2u (150) > qc (148) >> BFS (107)

**Why it works**: The bitfield FSM has **implicit state** — execution follows the same instructions regardless of input, but the computed bitfield value determines which `check_state()` branches are taken. Coverage heuristics see "already covered" instructions and deprioritize exploring new bitfield combinations, while DFS follows one manipulation chain deeply and discovers target patterns. DFS also gets the most partial branches (12), indicating it explores more of the check_state equality comparisons.

> [!IMPORTANT]
> **Key pattern**: Programs where coverage is orthogonal to actual computed state are the strongest NURS splitters. When the same code computes different states, coverage-based heuristics lose their signal.

---

### exp83: Symbolic Division Cascade

**Structure**: Pipeline of divisions/modulos by symbolic values. Each stage divides by a fresh symbolic divisor (implicit != 0 check + non-linear constraint). 2 noise bytes, 5 divisor stages with range checks and chained modulo.

| Searcher | CovI | UnI | TermExit | States | Queries | Wall (s) | Solver (s) |
|---|---|---|---|---|---|---|---|
| **nurs:qc** | **290** | **1** | 50,745 | 113,694 | 785K | 48.6 | 22.7 |
| **nurs:covnew** | **290** | **1** | 50,357 | 113,017 | 779K | 48.5 | 22.7 |
| **nurs:md2u** | **290** | **1** | 50,444 | 113,169 | 780K | 48.7 | 22.7 |
| DFS | 274 | 17 | 19,020 | 19,032 | 222K | 30.3 | 26.7 |
| default | 271 | 20 | 40,481 | 151,769 | 883K | 63.2 | 26.9 |
| random-path | 268 | 23 | 43,565 | 171,064 | 987K | 69.1 | 30.4 |
| BFS | 261 | 30 | 32,672 | 229,089 | 1.2M | 97.1 | 48.5 |

**Verdict**: 🏆 **Strong discriminator**. NURS (290, 1 uncovered!) >> DFS (274) >> default (271) >> random-path (268) >> BFS (261). Near-full coverage for NURS. DFS is solver-bound (26.7s solver in 30.3s wall). All NURS variants identical.

---

### exp84: Knotted CFG

**Structure**: Irreducible control flow via goto-connected loop nests. Two loops share a common body with symbolic dispatch back to either loop. 3 control bytes + 1 noise byte.

| Searcher | CovI | UnI | Instructions | TermExit | States | Queries |
|---|---|---|---|---|---|---|
| **DFS** | **184** | **35** | **37.5M** | 35 | **47** | **13.6M** |
| BFS | 216 | 3 | 36.5M | 513 | 7,689 | 13.3M |
| random-path | 216 | 3 | 19.0M | 34,816 | 35,328 | 7.2M |
| nurs:qc | 216 | 3 | 16.7M | 34,816 | 35,328 | 6.4M |
| nurs:covnew | 216 | 3 | 16.5M | 34,816 | 35,328 | 6.3M |
| nurs:md2u | 216 | 3 | 16.6M | 34,816 | 35,328 | 6.3M |
| default | 216 | 3 | 15.3M | 34,816 | 35,328 | 5.9M |

**Verdict**: 🏆 **Strong discriminator** — DFS uniquely penalized. DFS (184) misses 15% of coverage by getting trapped in the goto loops, executing 37.5M instructions but exploring only **47 states** (vs 35K for others). 13.6M queries with only 47 states means DFS is stuck in a tight loop revisiting the same goto paths. All non-DFS searchers reach 216 CovI with identical state counts (35,328).

---

### exp85: Poison Path ⭐

**Structure**: 8 "poison" code islands behind expensive modular arithmetic constraints + 2 cheap "target" islands behind trivial equality checks. The target path also contains a klee_assert. 2 noise bytes + 1 trigger byte.

| Searcher | CovI | UnI | TermExit | TermErr | States | Queries | Wall (s) | Solver (s) |
|---|---|---|---|---|---|---|---|---|
| **DFS** | **262** | 26 | 5,930 | 0 | 5,943 | 65K | 30.6 | 29.9 |
| **nurs:qc** | **223** | 65 | 0 | 0 | 56,533 | 418K | 43.1 | 29.8 |
| **nurs:covnew** | **217** | 71 | 0 | 0 | 62,799 | 465K | 44.1 | 29.3 |
| **nurs:md2u** | **217** | 71 | 0 | 0 | 62,854 | 465K | 44.1 | 29.3 |
| default | 196 | 92 | 0 | 0 | 171,522 | 1.2M | 70.9 | 26.9 |
| random-path | 182 | 106 | 0 | 0 | 290,617 | 1.9M | 104.9 | 28.4 |
| BFS | 163 | 125 | 0 | 0 | 400,997 | 2.3M | 133.7 | 35.9 |

**Verdict**: 🏆🏆 **Strong discriminator — NURS variant split achieved!**

DFS (262) >> **qc (223) > covnew (217) = md2u (217)** >> default (196) >> random-path (182) >> BFS (163)

**Why qc beats covnew/md2u**: qc correctly avoids the expensive "poison" islands (modular arithmetic like `key % 97 == 13`, `(key * key) % 101 == 42`) because those queries are expensive. It instead explores the cheap target path (equality check `trigger == 0x42`). covnew/md2u are attracted to the poison islands because they have "novel" code but don't consider query cost.

> [!TIP]
> **Key pattern**: Asymmetric solver cost with symmetric coverage novelty splits qc from covnew/md2u. qc cares about query cost; covnew and md2u don't.

---

### exp86: Duff's Device

**Structure**: Canonical irreducible CFG — switch cases inside a do-while loop, with symbolic entry point and operations. 1 noise byte.

| Searcher | CovI | TermExit | States |
|---|---|---|---|
| DFS | 238 | 130,560 | 130,560 |
| BFS | 238 | 130,560 | 130,560 |
| All NURS | 238 | 49K–50K | 120K |
| random-path | 238 | 52,321 | 130,560 |
| default | 238 | 39,959 | 124,244 |

**Verdict**: ❌ Non-discriminator on coverage. All reach 238 CovI. The Duff's device pattern creates enough states to fill the budget but the state space is fully explorable by all searchers. DFS/BFS complete all 130K paths identically.

---

### exp87: Convergent-Divergent Pipeline

**Structure**: Phase 1 (diverge: 2 bytes × 8 bits = 2^16 paths) → Phase 2 (converge: everyone runs `converge()`) → Phase 3 (diverge again: branches depend on accumulated Phase 1 state). At the convergence point, all states have identical coverage but different constraints.

| Searcher | CovI | UnI | TermExit | States | Queries | Wall (s) |
|---|---|---|---|---|---|---|
| **nurs:qc** | **299** | **12** | 2,751 | 280,881 | 2.2M | 100.3 |
| **nurs:covnew** | **299** | **12** | 2,803 | 283,267 | 2.2M | 100.3 |
| **nurs:md2u** | **299** | **12** | 2,805 | 283,371 | 2.2M | 100.6 |
| DFS | 279 | 32 | 155,197 | 155,207 | 3.5M | 30.0 |
| BFS | 275 | 36 | 2 | 458,738 | 2.1M | 147.1 |
| default | 260 | 51 | 0 | 286,454 | 1.7M | 101.1 |
| random-path | 245 | 66 | 0 | 322,725 | 1.9M | 111.0 |

**Verdict**: 🏆 **Strong discriminator**. NURS (299) >> DFS (279) >> BFS (275) >> default (260) >> random-path (245). The convergent-divergent structure rewards searchers that can navigate past the convergence point where covnew/md2u lose their signal (all code "already covered"). NURS variants still find novel post-convergence targets. However, all three NURS variants identical.

---

### exp88: Symbolic-Length Processing Pipeline
**Verdict**: ❌ Non-discriminator. All identical (228 CovI, 65,280 states). The 0–7 loop bound creates a bounded, fully explorable state space.

### exp89: Hash Inversion
**Verdict**: ❌ Non-discriminator. All identical (270 CovI, 4,608 states, ~2s). Z3 trivially inverts the simple hash function.

### exp90: Allocation Pattern
**Verdict**: ❌ Non-discriminator. All identical (404 CovI, 1,536 states, ~11s). Memory allocation differences don't affect KLEE's scheduling.

---

## Wave 11 (exp91–exp102, 10s timeout) — BFS Advantage Hunt

> [!NOTE]
> These 12 experiments were specifically designed to find cases where BFS strictly outperforms all other searchers. Strategies tested: wide-shallow trees, infeasible lures, symbolic writes, solver traps, and state explosion patterns.

### exp91: Shallow Spread

**Structure**: Wide shallow branching tree designed for breadth-first advantage.

| Searcher | CovI | TermExit | States |
|---|---|---|---|
| **All non-DFS** | **346** | 0 | 73K–167K |
| DFS | 171 | 36,633 | 36,673 |

**Verdict**: ⚡ Moderate. All non-DFS (346) >> DFS (171). BFS ties with NURS/RP/default — no strict BFS advantage.

---

### exp92: Infeasible Lure
**Verdict**: ❌ Non-discriminator. All identical (180 CovI, 224 states, <0.2s). Too small.

### exp93: Breadth Lottery

**Structure**: Lottery-style branching where many paths lead to unique coverage islands.

| Searcher | CovI | TermExit | States |
|---|---|---|---|
| **All non-DFS** | **500** | 0 | 75K–176K |
| DFS | 164 | 31,012 | 31,075 |

**Verdict**: ⚡ Moderate. All non-DFS (500) >> DFS (164). BFS ties with all others.

---

### exp94: State Flood Trap
**Verdict**: ❌ Non-discriminator. All identical (312 CovI, 129 states, <0.1s). Too small.

### exp95: Anti-Heuristic
**Verdict**: ❌ Non-discriminator. All identical (436 CovI, 1,616 states, ~1s). Fully explored.

### exp96: Layered Breadth

**Structure**: Multiple breadth layers with state explosion between levels.

| Searcher | CovI | States |
|---|---|---|
| **nurs:qc/covnew/md2u** | **208** | 102K |
| DFS | 198 | 35,613 |
| default | 193 | 95,450 |
| random-path | 171 | 109,147 |
| **BFS** | **155** | 134,672 |

**Verdict**: ⚡ Moderate. BFS is **worst** (155) — state explosion between layers kills it. NURS (208) > DFS (198) > default (193) > RP (171) > BFS (155).

---

### exp97: Symmetric Siblings

| Searcher | CovI | States |
|---|---|---|
| **All non-DFS** | **386** | 74K–147K |
| DFS | 153 | 31,246 |

**Verdict**: ⚡ Moderate. All non-DFS (386) >> DFS (153). BFS ties with NURS/RP/default.

---

### exp98: Write Scatter ⭐

**Structure**: Symbolic writes scattered across a table, then reads. Closest wave 11 approach to exp73's pattern.

| Searcher | CovI | States | Queries |
|---|---|---|---|
| dfs / bfs / RP / qc | 261 | 278–397 | 8K–12K |
| default | 258 | 356 | 11K |
| **covnew / md2u** | **244** | 409–412 | 13K |

**Verdict**: ⚡ Moderate. Interesting split: covnew/md2u (244) < all others (258–261). Symbolic writes confuse coverage heuristics but BFS doesn't beat DFS/qc/RP here. **BFS ties best rather than winning strictly.**

> [!NOTE]
> This is the closest to a BFS win in wave 11 — BFS matches DFS at 261 but can't beat it.

---

### exp99: Cascade Breadth

| Searcher | CovI | States |
|---|---|---|
| DFS | 357 | 36,986 |
| nurs:qc/covnew/md2u | 333 | 80K–81K |
| default | 282 | 82,930 |
| random-path | 269 | 96,043 |
| **BFS** | **245** | 159,531 |

**Verdict**: ⚡ Moderate. DFS wins (357). BFS is **worst** again (245). Cascade structure with state explosion penalizes breadth-first.

---

### exp100: Solver Quicksand

| Searcher | CovI | Queries |
|---|---|---|
| **All non-DFS** | **233** | 71K–95K |
| DFS | 161 | 10K |

**Verdict**: ⚡ Moderate. DFS gets trapped in expensive solver queries (161 CovI). All others reach 233. BFS ties with NURS.

---

### exp101: Memory Pressure BFS

| Searcher | CovI | States |
|---|---|---|
| DFS | 491 | 39,302 |
| nurs:covnew/md2u | 462 | 72K |
| nurs:qc | 461 | 71,335 |
| random-path | 356 | 78,985 |
| default | 365 | 70,188 |
| **BFS** | **335** | 128,777 |

**Verdict**: ⚡ Moderate. DFS (491) >> NURS (461–462) >> RP/default (356–365) >> **BFS (335)**. BFS is worst again — memory pressure kills breadth-first.

---

### exp102: BFS Pipeline

| Searcher | CovI | States |
|---|---|---|
| nurs:qc/covnew/md2u | 430 | 72K–74K |
| DFS | 412 | 36,207 |
| default | 345 | 72,709 |
| random-path | 310 | 86,407 |
| **BFS** | **297** | 134,170 |

**Verdict**: ⚡ Moderate. NURS (430) > DFS (412) >> default (345) >> RP (310) >> **BFS (297)**. BFS worst again. The pipeline structure needs smart scheduling, not breadth.

---

### Wave 11 Summary

**No strict BFS wins in 12 experiments.** Key findings:

- **BFS was WORST in 4 experiments** (exp96, exp99, exp101, exp102) — state explosion between layers penalizes breadth-first
- **BFS tied best in 4 experiments** (exp91, exp93, exp97, exp100) — but only by beating DFS; couldn't outperform NURS/RP
- **exp98 (Write Scatter) closest to BFS win**: BFS=261, ties DFS/qc/RP, beats covnew/md2u (244). Confirms that symbolic writes confuse coverage heuristics but BFS can't edge out DFS/qc/RP on coverage
- **Pattern**: Programs with state explosion BETWEEN coverage layers are BFS's worst case (it keeps all states alive at each level)

---

## Wave 12 (exp103–exp113, 10s timeout) — Targeted BFS Advantage Experiments

> [!NOTE]
> These experiments combine lessons from wave 11 and exp73 analysis. Focus areas: symbolic writes + switch dispatch (exp103–107), state machines and cascades (exp108–109), collision branching (exp110), write-read interleaving (exp111–112), and symbolic permutations (exp113).

### exp103: Multi-Table Write

**Structure**: 4 sub-problems selected by a switch on a selector byte. Each sub-problem performs 3 symbolic writes to a 4-element table + 1 read, with unique handlers per sub-problem. A 3-byte deep tail follows.

| Searcher | CovI | TermExit | States |
|---|---|---|---|
| **All non-DFS** | **348** | 0 | 57K–103K |
| DFS | 234 | 28,528 | 28,546 |

**Verdict**: ⚡ Moderate. All non-DFS (348) >> DFS (234). BFS ties with NURS/RP/default.

---

### exp104: Write Then Dispatch
**Verdict**: ❌ Non-discriminator. All identical (239 CovI, 72 completions, ~1.6s). Too small.

### exp105: Constraint Diversity
**Verdict**: ❌ Non-discriminator. All identical (254 CovI, 64 completions, <0.3s). Too small.

### exp106: Hash Map Simulation

**Structure**: Realistic hash table with 6-slot table, 5 symbolic key inserts (with linear probing), 3 lookup/delete operations. 8 unique handler functions.

| Searcher | CovI | TermExit | States |
|---|---|---|---|
| **nurs:qc** | **363** | 7 | 251 |
| BFS / RP / covnew / md2u / default | 361 | 0 | 236–285 |
| DFS | 322 | 104 | 114 |

**Verdict**: ⚡ Weak. nurs:qc (363) slightly edges out all others (361). BFS loses by 2. DFS much lower (322).

---

### exp107: Minimal BFS Win
**Verdict**: ❌ Non-discriminator. All identical (229 CovI, 8,192 completions, ~3.5s). Fully explored.

### exp108: State Machine
**Verdict**: ❌ Non-discriminator. All identical (168 CovI, 1,024 completions, ~3.7s). All states explored.

### exp109: Index Cascade
**Verdict**: ❌ Non-discriminator. All identical (233 CovI, 512 completions, ~1.4s). Fully explored.

### exp110: Collision Branch
**Verdict**: ❌ Non-discriminator. All identical (318 CovI, 41 completions, ~1.5s). Fully explored.

### exp111: Write-Read Interleave
**Verdict**: ❌ Non-discriminator. All identical (241 CovI, 42 completions, ~1s). Too few states.

### exp112: Double Write
**Verdict**: ❌ Non-discriminator. All identical (217 CovI, 16 completions, <0.4s). Too small.

### exp113: Symbolic Permutation
**Verdict**: ❌ Non-discriminator. All identical (242 CovI, 16 completions, <0.2s). Too small.

---

### Wave 12 Summary

**No strict BFS wins in 11 experiments.** Key findings:

- **8 of 11 experiments were non-discriminators** — programs completed fully within 10s, making all searchers equivalent
- **exp103 (Multi-Table Write)**: BFS ties all non-DFS at 348 (beats only DFS at 234)
- **exp106 (Hash Map Sim)**: nurs:qc slightly wins (363) over BFS/others (361); DFS worst (322)
- **The "symbolic write" pattern alone is insufficient** — when programs are small enough to explore fully, all searchers converge. The magic of exp73 is the specific combination of table size (16), write count (4), symbolic write values, and solver pressure that created a "Goldilocks zone" where BFS could explore more collision patterns than covnew/md2u but the problem was too large for any searcher to complete

> [!IMPORTANT]
> **Updated understanding of exp73**: Re-analysis shows exp73 is solver-time-dominated (~10s solver time for ALL searchers). BFS gets 172 CovI with 137 states and 11 completions. DFS gets 162 with 99 states and 81 completions. The key is not that BFS explores more states (it doesn't by much), but that BFS's level-order exploration of the write-collision tree produces more DIVERSE constraint sets earlier, leading to marginally more coverage of the hit/miss/collision handlers. This is a narrow, fragile advantage — 10 instructions out of 172 (5.8%).

---

## Wave 13 (exp114–exp123, 10s timeout) — exp73 Systematic Variants

> [!NOTE]
> These 10 experiments are systematic parameter sweeps of the exp73 (Symbolic Write) pattern — the only known strict BFS win. Variants test different table sizes, write/read counts, value-dependent collisions, cross-table references, unique handlers, and removal of symbolic write values.

### exp114: 5 Writes, 3 Reads (16-element table)

**Structure**: exp73 with 5 writes (up from 4) and 3 reads (up from 2). Deeper collision tree + more read branches.

**Verdict**: ❌ Non-discriminator. All identical (163 CovI, 128 completions, ~5.5s). Fully explored despite more writes — the 128 paths are still manageable.

### exp115: Small Table (8-element)

**Structure**: exp73 with 8-element table instead of 16. Higher collision rate due to smaller table.

| Searcher | CovI | States |
|---|---|---|
| **nurs:qc** | **182** | 328 |
| DFS / BFS | 172 | 144 / 229 |
| RP / covnew / md2u / default | 167 | 287–354 |

**Verdict**: ⚡ Moderate. nurs:qc wins (182) — small table creates constraint diversity that qc navigates best. BFS (172) ties DFS but loses to qc by 10. Interesting that shrinking the table helped qc more than BFS.

---

### exp116: More Reads (3 writes, 4 reads, 16-element table)
**Verdict**: ❌ Non-discriminator. All identical (234 CovI, 64 completions, ~2s). Fewer writes = fewer states = fully explored.

### exp117: Valued Collision (write count tracking)
**Verdict**: ❌ Non-discriminator. All identical (187 CovI, 40 completions, ~2s). Fully explored.

### exp118: Cross Tables (two 8-element tables)
**Verdict**: ❌ Non-discriminator. All identical (226 CovI, 64 completions, ~1.7s). Cross-references didn't create enough states.

### exp119: Unique Handlers (per-write unique functions)
**Verdict**: ❌ Non-discriminator. All identical (277 CovI, 32 completions, <1s). Unique handlers don't help if all paths are explored.

---

### exp120: Big Table (32-element) ⭐⭐

**Structure**: exp73 with 32-element table (5-bit index mask). 4 writes, 2 reads. Larger table = fewer collisions but exponentially more write combinations (32^4 ≈ 1M).

| Searcher | CovI | TermExit | States |
|---|---|---|---|
| **BFS** | **172** | 4 | 82 |
| RP / qc / covnew / md2u / default | 167 | 0 | 129–190 |
| DFS | 162 | 40 | 74 |

**Verdict**: 🏆 **BFS STRICT WIN!** BFS (172) > all others (167) > DFS (162). Same CovI pattern as exp73 (172/167/162) but with a 32-element table. Solver-dominated (~10.4s). BFS's level-by-level write exploration produces optimal constraint diversity in the collision tree.

---

### exp121: No Symbolic Values (deterministic write values) ⭐⭐

**Structure**: exp73 with deterministic write values (i+1 instead of symbolic wval[i]). Removes symbolic value bytes — simplifies solver constraints while keeping symbolic write indices.

| Searcher | CovI | TermExit | States |
|---|---|---|---|
| **BFS** | **164** | 12 | 140 |
| RP / qc / covnew / md2u / default | 159 | 0 | 224–279 |
| DFS | 154 | 93 | 110 |

**Verdict**: 🏆 **BFS STRICT WIN!** BFS (164) > all others (159) > DFS (154). Same stratification as exp73/exp120 but with simpler constraints. Confirms that the BFS advantage comes from write INDEX diversity, not value diversity. The 5-point margin is consistent across the trio (exp73: 5, exp120: 5, exp121: 5).

---

### exp122: More Writes (6 writes, 2 reads, 16-element table)
**Verdict**: ❌ Non-discriminator. All identical (145 CovI, 128 completions, ~5.5s). 6 writes to 16-element table is still fully explorable.

---

### exp123: Max Handlers (unique handlers per write step + per read) ⭐⭐⭐

**Structure**: exp73 with deterministic write values AND unique handler functions for each write step (fresh_w0/coll_w1/fresh_w1/coll_w2/etc.) and each read (hit_r0/miss_r0/hit_r1/miss_r1). Also includes a "full check" handler for 4-unique-writes-to-distinct-slots.

| Searcher | CovI | TermExit | States |
|---|---|---|---|
| **BFS** | **277** | 12 | 140 |
| RP / qc / covnew / md2u / default | 272 | 0 | 214–277 |
| DFS | 237 | 93 | 110 |

**Verdict**: 🏆🏆 **STRONGEST BFS STRICT WIN!** BFS (277) > all others (272) > DFS (237). **40-point gap** between BFS and DFS. The 5-point BFS-vs-second margin is consistent, but the unique handlers amplify the overall coverage spectrum (277 vs 172 CovI for exp73) — more instructions to cover means more opportunity for the BFS advantage to manifest.

> [!IMPORTANT]
> **Key insight**: The 5-instruction BFS advantage is consistent across exp73, exp120, exp121, and exp123. The advantage comes from BFS exploring the collision tree breadth-first, which produces more diverse collision/no-collision patterns at each write step. covnew/md2u can't distinguish between different collision patterns because the SAME code (collision handler) is executed regardless of WHICH slots collide — the coverage signal doesn't capture which constraint produced the collision.

---

### Wave 13 Summary

**3 strict BFS wins out of 10 experiments!** This is the most productive wave for the BFS-advantage hunt.

| Experiment | BFS CovI | Next-best | DFS CovI | Margin | Key Change from exp73 |
|---|---|---|---|---|---|
| exp120 (Big Table) | 172 | 167 | 162 | +5 | 32-element table (up from 16) |
| exp121 (No Sym Val) | 164 | 159 | 154 | +5 | Deterministic write values |
| exp123 (Max Handlers) | 277 | 272 | 237 | +5 | Unique handlers + deterministic vals |

**Pattern confirmed**: The BFS advantage is exactly **+5 CovI over second-best** in all 4 known wins (exp73, exp120, exp121, exp123). This suggests BFS covers exactly one additional branch combination that no other searcher reaches in the 10s budget.

**What matters for BFS wins:**
- ✅ Symbolic write INDICES to a table (collision detection branching)
- ✅ Solver pressure (must be time-limited, not completion-limited)
- ✅ 16+ element table (enough write combinations to prevent full exploration)
- ❌ Symbolic write VALUES (unnecessary — deterministic values work fine)
- ❌ Table size scaling (32-element gives same margin as 16-element)
- ❌ More writes/reads (if the search space is still completable, no advantage)
- ❌ Unique handlers (amplify total CovI but BFS margin stays at 5)

---

## Cross-Wave Summary

### Discriminative Power Ranking

| Rank | Experiment | Discriminates | CovI Range | Key Feature |
|---|---|---|---|---|
| 🥇 | **exp82 (Bitfield FSM)** | All 7 searchers | 107–216 | Implicit state orthogonal to coverage |
| 🥈 | **exp85 (Poison Path)** | qc vs covnew/md2u | 163–262 | Asymmetric solver cost |
| 🥉 | **exp40 (Priority Inversion)** | DFS vs all | 44.7%–100% | Noise before targets |
| 4 | exp70 (Multi-Objective) | NURS vs DFS vs BFS | 147–360 | Multiple independent objectives |
| 5 | exp77 (Constraint Explosion) | NURS vs DFS vs BFS | 249–342 | Progressive constraint accumulation |
| 6 | exp84 (Knotted CFG) | DFS vs all | 184–216 | Irreducible control flow |
| 7 | exp87 (Convergent Paths) | NURS vs DFS vs RP | 245–299 | Convergent-divergent pipeline |
| 8 | exp80 (Island Archipelago) | DFS vs NURS vs BFS | 152–367 | Tiered difficulty islands |
| 9 | exp62 (Asymmetric Cost) | DFS vs all | 416–534 | Cheap/expensive subtrees |
| 10 | exp83 (Symbolic Division) | NURS vs DFS vs BFS | 261–290 | Non-linear constraint cascade |

### What Splits Each Searcher Pair

| Pair | Best Discriminator | Pattern |
|---|---|---|
| DFS vs all | exp40 (priority inversion) | Noise sea before islands |
| DFS vs NURS | exp77 (constraint explosion) | Progressive constraints |
| NURS vs BFS | exp70 (multi-objective) | Independent objectives |
| NURS vs random-path | exp87 (convergent paths) | Converge-then-diverge |
| **qc vs covnew** | **exp85 (poison path)** | **Expensive poison + cheap target** |
| **covnew vs md2u** | **exp82 (bitfield FSM)** | **Implicit state (154 vs 150)** |
| **qc vs md2u** | **exp82 (bitfield FSM)** | **Implicit state (148 vs 150)** |

### BFS Strict Win Analysis (Waves 1–13)

> [!TIP]
> **After 123 experiments across 13 waves, 4 strict BFS wins found** — all sharing the same "symbolic write to table + collision check" pattern with a consistent +5 CovI margin.

| Experiment | BFS | 2nd-best | DFS | Margin | Table Size | Writes | Reads | Sym Values? |
|---|---|---|---|---|---|---|---|---|
| exp73 (Symbolic Write) | 172 | 167 | 162 | +5 | 16 | 4 | 2 | Yes |
| exp120 (Big Table) | 172 | 167 | 162 | +5 | 32 | 4 | 2 | Yes |
| exp121 (No Sym Val) | 164 | 159 | 154 | +5 | 16 | 4 | 2 | No |
| exp123 (Max Handlers) | 277 | 272 | 237 | +5 | 16 | 4 | 2 | No |

**The BFS advantage mechanism**: When KLEE writes to `table[symbolic_index]`, it forks into N states (one per possible index value). BFS explores ALL first-write targets before ANY second-write, creating maximum diversity of collision states at each write depth. This matters because the collision check (`if (table[idx] != 0)`) branches based on which slots are already occupied — a constraint-dependent property invisible to coverage heuristics.

**Why covnew/md2u can't match BFS here**: The collision handler is the SAME function regardless of which slot collided. covnew sees "collision handler already covered" and deprioritizes those states, but different collision patterns lead to different table states for subsequent reads. BFS doesn't have this bias — it treats all states at the same depth equally.

**Why the margin is always +5**: BFS reaches exactly one additional combination of collision/fresh writes at depth 4 that produces a unique hit/miss pattern at the read stage, covering 5 additional instructions. This is a structural property of the write-collision branching tree under solver-time pressure.

### Key Design Principles for Discriminating Experiments

1. **Time pressure is essential** — Experiments must fill the timeout budget with meaningful state exploration. Too-small programs (<1s) never discriminate.
2. **Noise bytes amplify state count** — But place them carefully. Noise BEFORE targets favors DFS; noise AFTER targets favors breadth-first approaches.
3. **Implicit state breaks coverage heuristics** — When the same code computes different states (bitfield FSM), coverage-based searchers lose their signal. This is the #1 NURS splitter.
4. **Asymmetric solver cost splits qc from covnew/md2u** — qc cares about query cost; the others don't.
5. **Irreducible CFGs trap DFS** — goto loops where DFS can cycle without backtracking.
6. **Convergence points confuse covnew** — When all paths merge, covnew sees "nothing new" even though constraints differ.
7. **30s timeouts >> 10s timeouts** — Longer runs let scheduling differences accumulate.
8. **Symbolic array writes confuse coverage heuristics** — Write collision patterns create constraint-dependent branching that covnew/md2u can't efficiently prioritize (exp73, exp98). But this alone doesn't guarantee BFS wins — it needs precise sizing to avoid full exploration.
9. **BFS is structurally penalized by inter-level state explosion** — Programs that fork between coverage layers are BFS's worst case (exp96, exp99, exp101, exp102). Design BFS-target experiments to avoid this.
10. **Strict BFS wins are inherently rare** — covnew/md2u subsume most of BFS's advantages. The only window is when coverage heuristic signals are misleading AND the problem is the right size.
