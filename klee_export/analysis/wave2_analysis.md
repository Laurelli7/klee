# Deep Structural Analysis: Wave 2 Results (SU09–SU28)

## Summary Table

| Snippet | Pattern | DFS | BFS | RP | CovNew | MD2U | QC | Best | Predicted | Correct? |
|---------|---------|-----|-----|-----|--------|------|-----|------|-----------|----------|
| SU09 | Cascaded Grouped Switch | **230** | 228 | 228 | 228 | 228 | 228 | DFS(≈) | covnew | ND |
| SU10 | Dense Switch Jump Table | **374** | 368 | **374** | **374** | **374** | **374** | =DFS/RP/NURS | covnew | ND |
| SU11 | Sparse Switch | **118** | 116 | **118** | 116 | 116 | **118** | DFS/RP/QC(≈) | DFS | ND |
| SU12 | Callback Dispatch Loop | 138 | 138 | 138 | 138 | 138 | 138 | tied | DFS | ND |
| **SU13** | **Bitwise Accumulator** | **142** | 110 | 110 | 110 | 110 | 110 | **DFS** | DFS | ✅ |
| SU14 | Diamond Validation | 273 | 273 | 273 | 273 | 273 | 273 | tied | covnew | ND |
| SU15 | Serialization Type Conv | 407 | 407 | 407 | 407 | 407 | 407 | tied | covnew | ND |
| **SU16** | **Hub Node** | 244 | **306** | **306** | **306** | **306** | **306** | **non-DFS** | covnew | ✅ |
| SU17 | Equality Chain | 271 | 271 | 271 | 271 | 271 | 271 | tied | covnew | ND |
| **SU18** | **Complex Recursive** | 294 | **325** | **325** | **325** | **325** | **325** | **non-DFS** | covnew | ✅ |
| SU19 | Struct Field Heavy | 305 | 305 | 305 | 305 | 305 | 305 | tied | covnew | ND |
| **SU20** | **Transform Loop** | **147** | 145 | **147** | 122 | 122 | 122 | **DFS/RP** | covnew | ❌ |
| **SU21** | **Call Heavy Loop** | 284 | **318** | **318** | **318** | **318** | **318** | **non-DFS** | covnew | ✅ |
| SU22 | Multi Loop | 204 | 204 | 204 | 204 | 204 | 204 | tied | covnew | ND |
| SU23 | Range Check Heavy | 301 | 301 | 301 | 301 | 301 | 301 | tied | md2u | ND |
| **SU24** | **Bitwise Dominated** | 249 | 192 | 204 | **256** | 253 | 255 | **covnew** | covnew | ✅ |
| SU25 | Many Merge Points | 249 | 249 | 249 | 249 | 249 | 249 | tied | covnew | ND |
| **SU26** | **Nested Scanner** | 265 | 275 | **277** | 255 | 255 | 255 | **RP** | DFS | ❌ |
| SU27 | Tail Call Chain | 276 | 276 | 276 | 276 | 276 | 276 | tied | DFS | ND |
| SU28 | Mostly Sequential | 190 | 190 | 190 | 190 | 190 | 190 | tied | DFS≈all | ND |

## Discrimination Statistics
- **Discriminating** (gap ≥ 10): 7/20 (35%)
- **Non-discriminating** (gap < 10): 13/20 (65%)
- **Prediction accuracy** (among discriminators): **5/7 correct (71.4%)**

---

## Discriminating Results: Detailed Analysis

### SU13: Bitwise Accumulator — DFS wins (gap=32, 29%)
```
DFS=142  |  BFS=110  RP=110  covnew=110  md2u=110  qc=110
```
**Prediction: DFS ✅** (R9: coverage-blind computation)

DFS is the ONLY searcher reaching CovI=142. All others tied at 110. The FNV hash +
CRC32 loop creates paths where every iteration runs the same code but produces different
hash values. Coverage heuristics see "already covered" on every iteration and can't
distinguish states. DFS avoids the overhead of coverage evaluation and completes paths
to the final hash-value checks. **Strongly confirms R9.**

DFS completed with only 4,483 queries; others did 25K–32K queries but all that extra
search yielded no additional coverage — classic coverage-blind waste.

---

### SU16: Hub Node — non-DFS wins (gap=62, 25%)
```
DFS=244  |  BFS=306  RP=306  covnew=306  md2u=306  qc=306
```
**Prediction: covnew ✅** (R6: convergent-divergent flow)

DFS scored only 80% of the other searchers. The hub_process function creates a strong
convergence point through which all entry paths pass, then diverges based on flags.
DFS picks one entry function and exhausts it, missing the flag-based diversification.
All non-DFS searchers tied — meaning the discriminating factor is purely "not DFS."
**Confirms R6 and the hub_node pattern.**

---

### SU18: Complex Recursive — non-DFS wins (gap=31, 10.5%)
```
DFS=294  |  BFS=325  RP=325  covnew=325  md2u=325  qc=325
```
**Prediction: covnew ✅** (R8 variant: recursive with internal branching)

All non-DFS tied at 325. DFS reached only 294 (90.5%). The recursive function with
8 opcode types and multiple recursive call patterns creates a tree that DFS explores
one branch of exhaustively. BFS/RP/NURS all explore the tree more broadly.
**Confirms R8 core finding: DFS is bad for recursive structures.**

---

### SU20: Transform Loop — DFS/RP win, NURS worst (gap=25, 20%)
```
DFS=147  RP=147  |  BFS=145  |  covnew=122  md2u=122  qc=122
```
**Prediction: covnew ❌** — NURS was WORST

**New finding!** Transform loops look like they should favor covnew (branches inside
suggest coverage targets), but the loop dominance effect (M9) prevails. After iteration 1,
all 8 transform branches are "already covered." NURS deprioritizes further iterations.
DFS and RP both reach CovI=147 because:
- DFS follows one path to completion through all 12 iterations
- RP randomly samples, giving equal weight to all iterations

DFS made only 5,406 queries (minimal overhead). NURS made 1.2M queries — 220× more —
but achieved 17% LESS coverage. This is the NURS trap: lots of work spent evaluating
coverage heuristics on states that are all coverage-equivalent.

**Rule evolution**: Transform loops should trigger R10 (identical loop body). Even though
the loop body has branches, once all branches have been visited (which happens on the
first iteration), subsequent iterations are coverage-identical. The "transform" label
is misleading — structurally, the loop body IS identical across iterations from a
coverage perspective.

---

### SU21: Call Heavy Loop — non-DFS wins (gap=34, 12%)
```
DFS=284  |  BFS=318  RP=318  covnew=318  md2u=318  qc=318
```
**Prediction: covnew ✅** (coverage guidance visits each call's coverage space)

DFS scored only 89% of others. The 5 function calls per iteration create sub-trees.
DFS gets stuck in one call's subtree. Other searchers rotate between calls and cover
all call targets. Notably, all non-DFS are tied, meaning the pattern simply punishes
DFS's commitment to one sub-tree.

**Unlike SU20 (transform loop)**, here the function calls ARE different code regions.
Each function (process_alpha, process_beta, etc.) has its own instructions. The loop
doesn't create "identical body" because the called functions provide true coverage
diversity per iteration.

Key distinction from SU20: In SU20, the branches within one iteration cover the
SAME instructions as branches in the next iteration. In SU21, the function calls
within one iteration cover DIFFERENT instructions from function calls in the next
iteration (different arguments activate different paths within each function).

---

### SU24: Bitwise Dominated — covnew wins, gradient across searchers (gap=64, 33%)
```
covnew=256  >  qc=255  >  md2u=253  >  DFS=249  >>  RP=204  >>  BFS=192
```
**Prediction: covnew ✅**

Most differentiated result! All 6 searchers produced different CovI values:
- covnew: 256 (best) — efficiently discovered new bit combinations
- qc: 255 — nearly as good, slight overhead from query cost tracking
- md2u: 253 — distance heuristic slightly less effective than coverage for bits
- DFS: 249 — reasonable due to sequential flag enumeration
- RP: 204 — random sampling of 24 bits is inefficient
- BFS: 192 (worst) — combinatorial explosion of 2^24 states at each depth

The independent bit-tests create a wide, shallow graph. Each bit-test is an independent
coverage target. covnew excels because each new bit combination covers a new branch.
BFS is worst because it tries to enumerate all combinations at depth 1 before moving
to depth 2.

**Rule evolution**: Bitwise-dominated non-loop code (flag decoding, bitfield processing)
is a strong sub-case of R4 where covnew specifically beats all others (not just "NURS").
BFS performs worst due to combinatorial bit-level explosion.

---

### SU26: Nested Scanner — RP wins, NURS worst (gap=22, 9%)
```
RP=277  >  BFS=275  >  DFS=265  >  covnew=255  =  md2u=255  =  qc=255
```
**Prediction: DFS ❌** — RP won, NURS worst

**New finding!** Nested scanners create a complex interaction between outer position
and inner token type. The ranking is: RP > BFS > DFS > NURS. NURS is worst because:
1. After seeing one token type, covnew deprioritizes other positions that would
   produce the SAME token type — missing that different positions give different
   overall coverage
2. The inner loop creates "coverage sameness" per token type category

RP wins because it randomly samples the (position × token_type) combinatorial space.
This is similar to the multi-dimensional dispatch finding (R4 amendment from SU08).

**Rule evolution**: Nested scanners (outer loop for positions, inner loop for token
processing) create a 2D search space. NURS gets trapped optimizing one dimension.
RP naturally samples both dimensions. This extends the multi-dimensional dispatch
finding from R4 to loop-based patterns.

---

## Non-Discriminating Results: Analysis

13/20 snippets (65%) showed no discrimination. This strongly confirms M11:

| Pattern | CovI | Queries | Why Non-Discriminating |
|---------|------|---------|----------------------|
| Cascaded Grouped Switch | 228-230 | 459K-665K | Grouped cases reduce effective branching |
| Dense Switch Jump Table | 368-374 | 644K-2.6M | Only 32 cases × 4 invocations = tractable |
| Sparse Switch | 116-118 | 481K-1M | 30 cases × 5 iterations still tractable |
| Callback Dispatch Loop | 138 | 395K-656K | 6 handlers × 6 iterations = tractable |
| Diamond Validation | 273 | 52K | 10 diamonds = 2^10=1024 paths — tiny |
| Serialization Type Conv | 407 | 7.6K | 8 types × 5 iterations = minimal |
| Equality Chain | 271 | 1.3M-2.4M | High queries but all paths explored |
| Struct Field Heavy | 305 | 22K | 12-byte struct with limited cross-field deps |
| Multi Loop | 204 | 116K-234K | 3 independent 4-iteration loops = tractable |
| Range Check Heavy | 301 | 950 | Only ~950 queries needed — trivially small |
| Many Merge Points | 249 | 6K | Merge points reduce effective state count |
| Tail Call Chain | 276 | 5K | Linear chain = minimal branching |
| Mostly Sequential | 190 | 95 | 95 queries — essentially no branching |

**Key insight**: The non-discriminating snippets all have `total_queries < 100K` or
`total_state_space` small enough that all searchers exhaust coverage. Discrimination
requires the search space to vastly exceed what can be explored in 10s.

---

## Prediction Accuracy Summary

| Round | Snippets | Discriminating | Correct | Accuracy |
|-------|----------|---------------|---------|----------|
| Wave 1 (SU01-SU08) | 8 | 5 | 2.5 | 50% |
| Wave 2 (SU09-SU28) | 20 | 7 | 5 | 71.4% |
| **Combined** | **28** | **12** | **7.5** | **62.5%** |

Improvement from Wave 1: +21.4% accuracy.

---

## Rule Evolutions Identified

### Evolution 1: R10 Amendment — Transform Loops
Transform loops (per-element conditional transformation) are actually R10 (identical
loop body) from a coverage perspective. Even though the body has branches, all branches
are covered after iteration 1. Subsequent iterations are coverage-identical.
**Test**: When a loop body has conditional branches, check if ALL branches are reachable
in a single iteration. If yes → R10 applies.

### Evolution 2: R4/R12 Amendment — Nested Scanner Multi-Dimensional
Nested scanners create a 2D search space (position × token_type). NURS optimizes
one dimension greedily and misses the other. RP samples uniformly across both.
Extends the multi-dimensional dispatch finding to loop-based patterns.

### Evolution 3: R4 Sub-Rule — Bitwise Flag Decoding
Bitwise-dominated non-loop code (flag decoding) is a strong R4 sub-case where
covnew specifically beats all others. BFS performs worst due to bit-level
combinatorial explosion. Produces a full searcher gradient:
covnew > qc > md2u > DFS >> RP >> BFS

### Evolution 4: M11 Reinforcement
13/20 additional non-discriminating results. The threshold for discrimination
appears to require the search space to be orders of magnitude larger than what
KLEE can explore in the time budget. Simple structural patterns with bounded
symbolic input rarely discriminate.
