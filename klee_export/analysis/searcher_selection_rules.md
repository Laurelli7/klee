# Structural Rules for KLEE Search Algorithm Selection

**Evidence base**: 123 hand-crafted experiments (76 discriminators) + 484 real-world CodeContests programs (38 discriminators) + 6 GNU coreutils structural snippets (4/6 correct) + 8 real-world structural units from ICFG analysis of gnumake/lua/bc/readelf/nasm/bison/tiffinfo/flvmeta (5 discriminators, 2/5 correct → 3 rule evolutions) + 20 deep fine-grained structural units SU09–SU28 from 30+ ICFG pattern categories (7/20 discriminators, 5/7 correct → 4 rule evolutions)  
**Searchers**: DFS, BFS, random-path (RP), nurs:covnew, nurs:md2u, nurs:qc, default (RP interleaved with covnew)

---

## Decision Tree

```
START
  │
  ├─ Is the program fully explorable within the time budget?
  │   YES → All searchers equivalent. Use any.
  │
  ├─ RULE 1: Sequential gating pattern?
  │   (target code behind a chain of checks, only reachable by committing to one path)
  │   YES → DFS
  │
  ├─ RULE 2: Useless forking branches BEFORE target code?
  │   (many symbolic branches that don't contribute coverage, placed before useful code)
  │   YES → DFS
  │
  ├─ RULE 3: Small symbolic setup → large concrete sweep?
  │   (few symbolic forks, then a million-iteration concrete loop)
  │   YES → DFS
  │
  ├─ RULE 4: Wide independent regions at top level?
  │   (program branches into N independent subproblems, each covering different code)
  │   YES → NURS (covnew or md2u)
  │
  ├─ RULE 5: Constraint complexity grows with depth?
  │   (shared variable accumulates constraints at each stage)
  │   YES → NURS (any)
  │
  ├─ RULE 6: Convergent-divergent control flow?
  │   (paths merge at a point then branch again)
  │   YES → NURS (covnew or md2u)
  │
  ├─ RULE 7: Symbolic loop bound with value-dependent coverage?
  │   (n is symbolic; different n values produce different loop iteration counts
  │    and each iteration count covers different instructions)
  │   YES → BFS
  │
  ├─ RULE 8: Recursive function with combinatorial branching?
  │   (each recursion level branches K ways; tree has K^L leaves)
  │   YES → BFS
  │
  ├─ RULE 9: Same code computes different state (bitfields, accumulators)?
  │   (every path runs identical instructions but produces different values)
  │   YES → DFS (coverage heuristics blind; DFS avoids state explosion overhead)
  │
  ├─ RULE 10: Identical loop body mutates different memory locations?
  │   (loop body always covers the same instructions but writes to different array slots)
  │   YES → Avoid NURS. Use DFS, BFS, or RP.
  │
  ├─ RULE 11: Expensive vs cheap constraint asymmetry?
  │   (some paths have modular arithmetic/nonlinear constraints, others have simple equality)
  │   YES → nurs:qc
  │
  ├─ RULE 12: Symbolic pointer/index chains?
  │   (each step loads from a[symbolic_index] creating one fork per possible value)
  │   YES → random-path
  │
  └─ Default → NURS (covnew) — safest overall choice
```

---

## Detailed Rules

### RULE 1: Sequential Gating — Use DFS

**Structure**: Target code is reachable only after passing through a sequence of gates (comparisons, pointer dereferences, sorting stages) on a single execution path. Each gate is a fork; to reach gate N, you must have already passed gates 1 through N-1 on the same path.

**CFG shape**: A long chain of diamonds in series. Each diamond has a "pass" and "fail" edge. Only the pass-pass-pass-...-pass path reaches the target.

**Why DFS wins**: DFS commits to one path and follows it to termination. If it picks the correct branch at each gate, it reaches the target. Other searchers fork at every gate and accumulate pending states without ever completing the full chain.

**Detectable features**:
- Deep call chain or loop with sequential if/else checks on the same variable
- Sorting networks, tournament brackets, linked-list traversals
- Number of sequential symbolic branch points on the longest path

**Evidence**:
- exp75 Symbolic Sort: DFS 305, best other 120 (gap 185) — 15-stage sorting network
- exp78 Tournament: DFS 281, best other 120 (gap 161) — deep tournament bracket
- exp76 Aliased Pointers: DFS 253, best other 163 (gap 90) — 8-hop pointer chain
- exp64 Breadcrumb Trail: DFS 244, best other 120 (gap 124) — linear clue sequence
- exp72 Distance vs Novelty: DFS 234, best other 123 (gap 111) — deep linear chain

**Strength**: Strong. Gaps of 90–185 are common. This is DFS's dominant pattern.

**Caveat (evolved)**: Sequential gating only discriminates if the path space exceeds
what any searcher can exhaust in the time budget. A chain of N simple if/else gates
creates at most 2N+1 paths. Tested with cut.c/seq.c/sort.c gate patterns (16 symbolic
bytes, ~6753 queries): all 6 searchers tied at CovI=437 in 0.3s — zero discrimination.
For Rule 1 to apply, need N ≥ 15 simple gates, OR gates with wide branching (3+ arms),
OR expensive gate constraints.

---

### RULE 2: Useless Fork Barrier — Use DFS

**Structure**: Many symbolic branches that produce paths covering no new code are placed *before* the useful code in the control flow. All searchers except DFS fork at each useless branch and drown in pending states.

**CFG shape**: A wide fan-out of useless branches near the root, followed by a narrow bottleneck, followed by the target code.

**Why DFS wins**: DFS picks one path through the useless region (never forks sideways), reaches the target code quickly. Others spend their entire budget managing 75K–140K pending states from the useless branches.

**Detectable features**:
- Symbolic bytes/variables whose values don't affect which code regions are reachable
- Large state count with no new coverage being discovered
- The useless branches must be *before* the useful code in the CFG topological order

**Evidence**:
- exp40 Priority Inversion: DFS 100%, others 44.7% — 3 noise bytes (2^24 paths) before 10 code islands
- exp39 Inverse Funnel: DFS finishes in 1.5s, BFS takes 475s — useless divergence before convergence
- exp41 BFS Paradise (ironic): DFS 100%, others 94.4% — decoy label

**Strength**: Very strong when the pattern is present. DFS can be 2× better.

---

### RULE 3: Small Symbolic Setup → Large Concrete Sweep — Use DFS

**Structure**: A small number of symbolic variables are read at the start. The bulk of the program is a large concrete loop (hundreds of thousands to millions of iterations) that depends on which concrete values were chosen.

**CFG shape**: A small diamond (the symbolic fork) followed by a very long straight chain (the concrete computation). Different fork outcomes lead to chains that cover different instructions.

**Why DFS wins**: DFS picks one concrete assignment and runs the entire sweep, completing the full execution trace. Others fork on the symbolic variables and start multiple sweeps but none finishes within the time budget.

**Detectable features**:
- Few symbolic variables (1–5)
- Large loop bounds that are concrete after the symbolic fork resolves
- High instruction count per path but low query count per instruction (queries only at the fork)
- DFS makes 10–100× more queries than BFS in the same time (because it's racing through concrete code)

**Evidence**:
- train_00389 (CodeContests): DFS 987, BFS 956 — 838K queries (DFS) vs 8K (BFS) in same time; million-element coprime scan
- train_02647 (CodeContests): DFS 998, BFS 965 — DFS completes the counting loop

**Strength**: Moderate. Gaps of 25–33. This pattern is common in competitive programming.

---

### RULE 4: Wide Independent Subproblems — Use NURS

**Structure**: The program branches into N independent code regions at the top level. Each region covers different instructions. Coverage requires visiting multiple regions, not going deep in one.

**CFG shape**: A root node fans out to N subtrees, each containing different code. There is no data dependency between subtrees.

**Why NURS wins**: covnew/md2u detect that switching to a different region exposes more uncovered instructions than continuing in the current region. DFS exhausts one region and never visits the others. BFS spreads evenly but doesn't prioritize regions with more uncoverable code.

**Detectable features**:
- Switch/case with many independent handlers
- Multiple function calls to different functions at the same level
- Top-level if/else chain where each branch contains unique code

**Evidence**:
- exp70 Multi-Objective: NURS 360, DFS 292, BFS 147 — several independent code regions
- exp102 BFS Pipeline: NURS 430, DFS 412, BFS 297 — multi-stage pipeline with independent branches per stage
- exp93 Breadth Lottery: non-DFS 500, DFS 164 — many top-level branches
- exp97 Symmetric Siblings: non-DFS 386, DFS 153 — many equal-width branches
- train_05400 (CodeContests): non-DFS 993, DFS 942 — factorial recursion: DFS picks one depth, NURS covers many depths
- train_08408 (CodeContests): non-DFS 1033, DFS 984 — brute-force grid: DFS picks one x value

**Strength**: Strong. Gaps of 50–200. This is NURS's dominant pattern.

**Caveat (evolved)**: Wide branches must be at the **top level** of the control flow.
When a switch is nested inside a byte-processing loop (e.g., tr.c's character
classification inside a per-byte loop, or wc.c's switch inside a block-read loop),
Rule 10 (identical loop body) takes precedence. Tested with tr.c/wc.c combined pattern
(8 symbolic bytes, 12-way switch per byte): DFS won (518 vs 495, gap=23) because after
iteration 1: all switch cases are "already covered" and the loop's repetition dominates.
DFS maintained 107 max states; others accumulated 44K–85K states.

**Detection**: If the wide branch is dominated by a loop header with symbolic bound,
apply Rule 10/9 instead of Rule 4.

**Bitwise Flag Decoding Sub-Rule (evolved from deep structural units)**:
When the "wide independent regions" are individual bit-tests on a symbolic bitfield
(e.g., `if (flags & 0x01)`, `if (flags & 0x02)`, etc.), covnew specifically wins
over all other searchers, and BFS is worst. Each bit-test is an independent coverage
target that covnew efficiently enumerates. BFS suffers from combinatorial explosion
(2^N states for N bits at each depth level).
Tested with SU24 (16-bit flags + 8-bit mode, bit-test branches + switch on modifier):
covnew=256 > qc=255 > md2u=253 > DFS=249 >> RP=204 >> BFS=192. This produced the
most differentiated gradient across all 6 searchers (gap=64, 33%). The full ranking
is covnew > qc ≈ md2u > DFS >> RP >> BFS. **Detection**: Non-loop function body
dominated by AND+compare bit-test patterns on symbolic variables → strong R4, use
covnew specifically.

**Nested Scanner Multi-Dimensional Amendment (evolved from deep structural units)**:
Nested scanners (outer loop for positions, inner loop for token processing) create a
2D search space: (position × token_type). This is structurally similar to the
multi-dimensional dispatch case (R4 amendment from SU08). NURS optimizes one dimension
greedily and misses the other; RP samples uniformly across both dimensions.
Tested with SU26 (10-byte tokenizer with 4 token categories in inner scanner):
RP=277 > BFS=275 > DFS=265 > covnew=255 = md2u=255 = qc=255. NURS was worst (gap=22).
This extends the multi-dimensional dispatch finding to loop-based patterns. **Detection**:
Nested loops where both inner and outer loops branch on symbolic data → treat as
multi-dimensional dispatch, prefer RP over NURS.

**Multi-Dimensional Dispatch Amendment (evolved from real-world structural units)**:
When a program has **multiple independent dispatch points** (e.g., processing N
independent objects each with its own switch/dispatch), **random-path outperforms NURS**.
NURS greedily optimizes one dispatch dimension; RP uniformly samples the combinatorial
space of {dispatch_1 × dispatch_2 × ...}. Tested with SU08 (readelf-style multi-level
dispatch: 2 independent sections × 8-way type dispatch × sub-handler dispatch):
RP=596, BFS=587, NURS=452, DFS=306. RP beat NURS by 144 (32%). The two independent
dispatch points created a combinatorial space that RP sampled effectively while NURS
focused too narrowly on one section's handlers.

---

### RULE 5: Growing Constraint Complexity — Use NURS

**Structure**: A shared variable accumulates constraints at each stage. Stage 1 adds constraint C1, stage 2 adds C2, etc. Deeper stages have increasingly expensive solver queries because the constraint set grows.

**CFG shape**: A pipeline where each stage adds a new constraint to the path condition. The pipeline may branch at each stage, but the constraints on the shared variable accumulate across all stages.

**Why NURS wins**: NURS re-prioritizes toward states where the solver cost is still manageable. DFS dives deep and gets stuck on states with maximal constraint complexity. BFS creates many shallow states but doesn't know which are cheapest to continue.

**Detectable features**:
- A variable that is constrained at multiple program points along the same path
- Solver time grows with execution depth
- DFS makes few queries (each slow); others make many queries (each fast)

**Evidence**:
- exp77 Constraint Explosion: NURS 342, DFS 273, BFS 249 — shared variable accumulates constraints
- exp83 Symbolic Division: NURS 290, DFS 274, BFS 261 — pipeline of divisions/modulos
- exp100 Solver Quicksand: non-DFS 233, DFS 161 — DFS makes 10K slow queries, others 71K–95K fast ones
- exp71 Solver Cost Cliff: non-DFS 312, DFS 219 — exponentially harder constraints per level

**Strength**: Moderate-strong. Gaps of 30–90.

---

### RULE 6: Convergent-Divergent Flow — Use NURS

**Structure**: Multiple paths converge at a single program point (merge), then diverge again. After the merge, all states have the same coverage footprint, so coverage heuristics lose their signal temporarily. The post-merge branches cover different code.

**CFG shape**: A fan-in to a single node, then a fan-out. The coverage signal is the same for every state at the merge point.

**Why NURS wins**: After the merge, NURS re-evaluates which post-merge branches have uncovered instructions and can redirect. RP carries stale priorities from before the merge. BFS doesn't have this problem but is generally less efficient than NURS at the post-merge re-diversification.

**Detectable features**:
- Functions called from multiple callers (many paths merge at function entry)
- Loops where the loop body is a merge point for multiple paths
- DFS/BFS/RP coverage nearly equals each other but all are less than NURS

**Evidence**:
- exp87 Convergent-Divergent: NURS 299, DFS 279, RP 245 — diverge, converge, diverge again

**Strength**: Moderate. Gaps of 20–50. This pattern is subtle and hard to detect statically.

**Hub Node Confirmation (from deep structural units)**:
Hub nodes (functions with high fan-in + fan-out) are a strong R6 instance.
Tested with SU16 (5 entry functions → hub_process → flag-based fan-out):
DFS=244, all non-DFS=306 (gap=62, 25%). DFS gets stuck in one entry path
and misses the flag-based diversification post-convergence. All non-DFS
searchers tied — confirming the pattern purely punishes DFS's commitment.

---

### RULE 7: Symbolic Loop Bound — Use BFS

**Structure**: A symbolic variable `n` controls how many times a loop executes. Different values of `n` produce different loop iteration counts. Each additional iteration covers at least some new instructions (different printf arguments, different array accesses, different conditional branches inside the loop body).

**CFG shape**: A fork on `n` at the top, then `n` different chain lengths — one per possible value. The chains share the loop body but have different lengths.

**Why BFS wins**: BFS explores all values of `n` at uniform depth. It reaches the n=1 complete execution first (covering base case + print logic), then n=2, etc. DFS picks one value of `n` and follows it to completion, which may be a deep trace that covers many iterations of the same loop body but misses the base case logic. NURS may focus on the value of `n` that produces the most uncovered instructions, but if each iteration covers the same loop body, NURS deprioritizes new iterations.

**Detectable features**:
- A symbolic variable used as a loop bound: `for(i=0; i<n; i++)` where `n` is symbolic
- Small range of `n` (e.g., 1–5) but each value produces detectably different instruction coverage
- Programs that complete for small `n` but time out for large `n`

**Evidence**:
- train_08285 (CodeContests): BFS 1175, others 1009–1014 (gap 166) — n controls triple-nested DP depth
- train_04207 (CodeContests): BFS 1523, DFS 1433, NURS 1427 (gap 96) — n controls adjacency matrix loops
- train_03628 (CodeContests): BFS 993, others 951–953 (gap 42) — n controls DP loop iterations
- train_04115 (CodeContests): BFS 1052, DFS 1033, NURS 996 (gap 56) — n controls array fill pattern
- train_11610 (CodeContests): BFS 1052, DFS/RP 990, NURS 1048 (gap 62) — n controls matrix DP
- train_08912 (CodeContests): DFS+BFS+RP 1051, NURS 972 (gap 79) — n controls character scanning loop

**Strength**: Moderate-strong. Gaps of 40–166. Most common BFS-winning pattern in real code.

**GNU coreutils validation**: Tested with seq.c/factor.c/comm.c combined pattern
(12 symbolic bytes → seq_loop, trial_divide, merge_count). BFS won: 447 vs DFS 382,
NURS 369 (gap=78). Confirmed.

**Interaction (evolved)**: When the loop body is also identical across iterations
(Rule 10), NURS can be the **worst** searcher, not just "not the best." In the
test, NURS (369) was worst — 78 below BFS and 13 below DFS. NURS gets trapped
expanding identical loop iterations in trial_divide while BFS naturally explores
different loop *lengths*.

---

### RULE 8: Recursive Combinatorial Explosion — Use BFS

**Structure**: A recursive function branches K ways at each level. The recursion tree has K^L leaves for L levels. Different branches cover different instructions at the base case.

**CFG shape**: A tree rooted at the recursive call site. Each internal node has K children (the K recursive calls). Leaves contain the base-case code.

**Why BFS wins**: BFS reaches all leaves at depth 1 before exploring any at depth 2. For small inputs (few recursion levels), BFS completes all base cases and covers the result-comparison/output code. DFS picks one branch at every level and follows it deep — it may reach one base case but misses the others.

**Detectable features**:
- Recursive function with multiple recursive calls per invocation
- Base case contains different code than the recursive case
- The branching factor and recursion depth both depend on symbolic variables

**Evidence**:
- train_07438 (CodeContests): BFS 1197, DFS 1083 (gap 139) — recursive combinatorial search C(k,n)
- train_06827 (CodeContests): BFS 993, DFS 966, RP 944 (gap 60) — centroid decomposition with 3-way branching

**Strength**: Strong for programs with high branching factors. Gaps of 60–139.

**Call-Heavy Loop Confirmation (from deep structural units)**:
When a loop body is dominated by calls to distinct functions (each with its own
internal branching), DFS gets stuck in one function's subtree. Unlike transform
loops (R10), here the called functions ARE genuinely different code regions.
Tested with SU21 (5 functions called per iteration, 5-iteration loop):
DFS=284, all non-DFS=318 (gap=34, 12%). The key distinction from R10/SU20:
in call-heavy loops, each function call covers DIFFERENT instructions, so
iterations are NOT coverage-identical. NURS correctly identifies uncovered
paths in different function bodies.

**Distinct Base Cases Amendment (evolved from real-world structural units)**:
When the recursive function has **multiple distinct code regions at the base case**
(e.g., leaf/unary/binary/ternary handlers with different code), NURS can detect and
prioritize unexplored base cases, slightly outperforming BFS. BFS still vastly
outperforms DFS. Tested with SU03 (recursive tree with 4 node types × 6 depth):
md2u=216, covnew/qc=215, RP=214, BFS=211, DFS=99. NURS beat BFS by 5 (2.4%) because
covnew/md2u detected that ternary-node base cases had uncovered code while BFS was
still exploring all depth-1 nodes uniformly. Key: DFS scored barely 46% of NURS,
confirming R8's core finding that depth-first is terrible for recursive structures.

---

### RULE 9: Same Code, Different State (Coverage-Blind) — Use DFS

**Structure**: Every execution path runs the same instructions but computes different values (via bitfield operations, accumulators, hash functions, numerical computation). Coverage-based heuristics see "already covered" everywhere and stop making useful decisions.

**CFG shape**: A single pipeline or loop body where all states take the same path through every branch, but the computed values differ. The final check (if any) branches on the computed value, exposing the hidden state difference.

**Why DFS wins**: When coverage heuristics are blind, NURS is no better than random — but it pays overhead for the heuristic computation. BFS suffers from state explosion (same coverage at every depth). DFS avoids both problems: it picks states cheaply and follows them to completion, where the final check on computed values finally differentiates.

**Detectable features**:
- Bitfield operations (set/clear/toggle) — same instructions, different bit patterns
- Accumulators/counters that modify the same variable differently per path
- Hash computation on symbolic input
- No branches on symbolic values within the computation — all branching happens at the very end

**Evidence**:
- exp82 Bitfield FSM: All 7 searchers distinct. DFS 216, NURS 148–154, BFS 107 — 8 bitfield operations, final 16-way check
- exp80 Island Archipelago: DFS 367, NURS 276, BFS 152 — code islands behind difficulty tiers

**Strength**: Very strong when the pattern is pure. DFS can be 2× BFS. This is the #1 pattern that separates all searchers.

**Bitwise Accumulator Confirmation (from deep structural units)**:
FNV hash + CRC32 loops are a textbook R9 instance. Every iteration runs the
same XOR/multiply/shift instructions; only the accumulated value differs.
Tested with SU13 (FNV hash + CRC32, 8-byte symbolic input):
DFS=142, all others=110 (gap=32, 29%). DFS used only 4,483 queries; others
used 25K–32K queries but gained NO additional coverage — the extra queries
were wasted evaluating coverage-identical states.

**Fully-Symbolic Nested Loops (evolved from real-world structural units)**:
When nested loops have **both** the search target AND the search data fully symbolic
(e.g., pattern matching where both patterns and input are symbolic), the combinatorial
explosion produces effectively coverage-blind paths. DFS wins by avoiding state
management overhead. Tested with SU07 (gnumake-style pattern_search: 6 symbolic
patterns × 8 chars matched against 10 symbolic input bytes): DFS=222, all others=192
(gap=30, 15.6%). The fully symbolic patterns+input created ~10^58 path combinations
with identical loop body coverage. DFS avoided 500K+ state overhead that other
searchers accumulated.

---

### RULE 10: Identical Loop Body, Different Memory Targets — Avoid NURS

**Structure**: A loop body runs the same instructions every iteration, but writes to different memory locations (different array slots, different struct fields). Coverage heuristics see "already covered" and deprioritize further iterations, but the different write targets produce different state that matters for post-loop code.

**CFG shape**: A loop with a constant body, where the write target is determined by a symbolic expression. Each iteration forks on the target but then reconverges at the loop back-edge.

**Why NURS loses**: covnew/md2u see the loop body as "already covered" and switch to other states. But the different write targets affect post-loop behavior. DFS completes the loop for one input value. BFS progresses all loop iterations at uniform depth. RP randomly samples.

**Detectable features**:
- `a[symbolic_expr] = ...` inside a loop
- The loop body itself has no branches on symbolic values
- Post-loop code reads from the array and branches on the values
- covnew/md2u states count is high but CovI is lower than DFS/BFS

**Evidence**:
- exp73/120/121/123 Symbolic Write: BFS +5 CovI in all cases — 16–32 element tables with symbolic write indices
- train_02120 (CodeContests): BFS/RP 1129, DFS 1083, NURS 1057 (gap 72) — convolution with identical-code iterations mutating different cells
- train_08412 (CodeContests): DFS/BFS/RP 1038, NURS 999 (gap 48) — grid DP with same instructions per cell
- train_08183 (CodeContests): RP 1016, DFS/BFS 1006, NURS 983 (gap 33) — Fenwick tree operations
- train_06744 (CodeContests): DFS/BFS 1012, NURS 970 (gap 42) — frequency counting a[symbolic]++

**Strength**: Consistent. NURS reliably loses in this pattern. Gaps of 30–72.

**Transform Loop Amendment (evolved from deep structural units)**:
Transform loops — where the loop body applies a conditional transformation to each
element (e.g., character class mapping, value normalization, type conversion per byte)
— are a subtle R10 instance. The loop body *has* branches (e.g., if val<32 → escape,
if val<48 → normalize, etc.), which makes it look like R4 (wide independent). But
after the first iteration, ALL branches in the body are "already covered." Subsequent
iterations are coverage-identical from NURS's perspective.
Tested with SU20 (12-byte transform loop, 8 conditional transform categories):
DFS=147, RP=147, BFS=145, NURS=122. NURS scored 17% below DFS/RP. NURS made 1.2M
queries (220× more than DFS's 5.4K) but achieved LESS coverage — classic coverage-blind
waste. **Detection**: If a loop body has K conditional branches and K is small enough
that one iteration visits all K branches, apply R10 not R4.

---

### RULE 11: Constraint Cost Asymmetry — Use nurs:qc

**Structure**: Some branches lead to code behind expensive constraints (modular arithmetic, nonlinear expressions, division/modulo), while other branches reach equally novel code behind cheap constraints (simple equality, range checks).

**CFG shape**: Multiple branches from a decision point. Some branches have expensive path conditions; others have cheap ones. All branches lead to uncovered code.

**Why qc wins**: covnew/md2u are attracted to both branches equally because both lead to uncovered code. But the expensive branch wastes solver time. qc tracks total query cost per state and deprioritizes the expensive branch, spending its time on the cheap branch and covering more instructions per second.

**Detectable features**:
- Mix of constraint types: modular arithmetic (`x%97==13`) alongside simple equality (`x==5`)
- Some branches have much higher SolverTime than others
- qc queries count is similar to covnew/md2u but CovI is higher

**Evidence**:
- exp85 Poison Path: qc 223, covnew/md2u 217 — 8 poison islands with expensive modular constraints
- exp115 Small Table: qc 182, covnew/md2u 167 — hash table collision queries are expensive
- exp106 Hash Map: qc 363, covnew/md2u 361 — marginal edge on hash probe sequences

**Strength**: Moderate. Gaps of 6–15 between qc and covnew/md2u. Larger gap (up to 57) between qc and worse searchers when combined with other patterns (train_09592: qc last at 1039 when it misidentifies all queries as expensive).

**GNU coreutils validation (strengthened)**: Tested with factor.c modular exponentiation
(expensive) vs basenc.c base16 classification (cheap). qc won: 474 vs covnew/md2u 443
(gap=31; vs DFS 233, gap=241). The 31-point qc-over-NURS gap is **2–5× larger** than
in hand-crafted experiments (6–15). Real code's modular exponentiation creates much
larger solver cost differences than artificial constraints. The ranking qc > RP > BFS >
covnew/md2u > DFS also confirms that random sampling (RP) naturally gives ~50% time
to cheap paths, which beats covnew's equal attraction but loses to qc's informed avoidance.

**Caveat**: qc can be *worst* when all queries are similarly cheap — its tiebreaking is worse than random (train_04164: qc 984, others 1006).

---

### RULE 12: Symbolic Pointer/Index Chains — Use random-path

**Structure**: Each step loads from `a[symbolic_index]`, and the loaded value is used as the index for the next load. This creates a chain of dependent symbolic reads.

**CFG shape**: A linear chain where each node forks N ways (N = array size), but only one fork per chain step is taken. The chain length is the number of dependent loads.

**Why RP wins**: RP randomly samples from all pending states, naturally creating diverse starting points along the chain. Coverage heuristics can't distinguish chain positions (same load instruction at every step). DFS follows one chain to completion but misses alternative chain routes. BFS spreads across all chain positions at equal depth.

**Detectable features**:
- `x = a[x]` pattern — the index is itself loaded from the array
- Array of pointers, linked list traversal, hash table probing with chaining

**Evidence**:
- exp61 Scaled Pointer Chase: RP 268, DFS 260, NURS 251 — 8-hop chain
- exp57 Pointer Chase: RP/default/qc 178, DFS 176, covnew/md2u 166 — 6-hop chain
- train_08183 (CodeContests): RP 1016, DFS/BFS 1006, NURS 983 — Fenwick tree (implicit index chain via x -= x&-x)

**Strength**: Weak. Gaps of 7–33. RP's advantage is narrow and inconsistent.

---

## Meta-Rules

### M1: Time Pressure Required
Programs that complete within the time budget never discriminate. The searcher matters only when exploration is incomplete.

### M2: Position of Useless Code
Where useless forking branches sit relative to target code is the single strongest predictor:
- Useless branches BEFORE targets → **DFS** (ignores them)
- Useless branches AFTER targets → **BFS** or **NURS** (reaches targets first)

### M3: Coverage-Blind Computation
When all paths execute the same code but compute different values, coverage heuristics lose their signal. This is the single most diagnostically valuable feature — it separates all 7 searchers from each other.

### M4: NURS Is The Safe Default
In the full dataset, NURS (covnew or md2u) is never the worst choice by a large margin, except in RULE 10 (identical loop body). It's the median or better choice in >80% of discriminators.

### M5: BFS Is Situational, Not General
BFS strict wins exist (11 cases across both datasets) but require specific conditions: symbolic loop bound with value-dependent coverage (RULE 7) or recursive combinatorial explosion (RULE 8). Outside these patterns, BFS is frequently worst due to state explosion.

### M6: DFS Is High-Variance
DFS has the most strict wins (~17) but also the most strict losses (~16). It's the optimal choice when you can identify sequential gating (RULE 1), useless fork barriers (RULE 2), or coverage-blind computation (RULE 9). It's the worst choice for wide independent subproblems (RULE 4).

### M7: qc Has a Narrow Niche
nurs:qc is strictly better than covnew/md2u only when there is a mix of expensive and cheap constraints (RULE 11). In all other cases, it performs identically or slightly worse than covnew/md2u. Using qc as default is suboptimal.

### M8: Multiple Rules Can Apply
Programs may exhibit multiple patterns simultaneously. When rules conflict, prioritize by strength:
1. RULE 9 (coverage-blind) — DFS if coverage heuristics are completely blind
2. RULE 1/2 (sequential gating / useless fork barrier) — DFS if target code is behind a deep chain
3. RULE 4 (wide independent regions) — NURS if there are multiple independent code regions
4. RULE 7/8 (symbolic loop bound / recursive explosion) — BFS if loop bound is symbolic with value-dependent coverage
5. RULE 10 (identical loop body) — avoid NURS
6. Everything else — use NURS:covnew as default

### M9: Loop Dominance (evolved from GNU coreutils validation)
When a structural pattern is nested inside a loop, the loop's properties dominate.
A wide switch inside a byte-processing loop → Rule 10 wins over Rule 4.
A sequential gate chain inside a small loop → Rule 10 wins over Rule 1.
**Hierarchy**: Loop structure > Branch structure when nested.
Confirmed: tr.c/wc.c 12-way switch inside 8-byte loop → DFS won (Rule 10), not NURS (Rule 4).

### M10: Rule Interactions Compound (evolved from GNU coreutils + deep structural units)
Rules can combine to produce effects neither rule alone predicts:
- Rule 7 (symbolic loop bound) + Rule 10 (identical loop body) = "NURS worst" outcome.
  NURS gets trapped in identical-bodied iterations; BFS explores different loop lengths.
  Confirmed: seq.c/factor.c/comm.c combined → BFS 447, DFS 382, NURS 369.
- Rule 4 (wide dispatch) + multiple independent instances = RP wins over NURS.
  NURS greedily chases one dimension; RP samples the combinatorial product uniformly.
  Confirmed: SU08 multi-level dispatch → RP 596, BFS 587, NURS 452, DFS 306.
  Extended: SU26 nested scanner (2D loop space) → RP 277, BFS 275, NURS 255. Same
  principle applies to loop-based multi-dimensional structures, not just dispatch.
- Rule 9 (coverage-blind) + nested loops = DFS dominance amplified.
  Fully symbolic data on both sides of a matcher creates pure coverage blindness.
  Confirmed: SU07 nested loop matcher → DFS 222, all others 192.
- Rule 4 (wide branches) inside loop = becomes R10 when branches are exhausted
  after one iteration. Transform loops are a subtle trap: they LOOK like R4 but
  ARE R10. Confirmed: SU20 transform loop → DFS/RP 147, NURS 122 (NURS worst).

### M11: Non-Discrimination Is Common in Real Programs (evolved from real-world structural units)
3 out of 8 structural unit snippets in Wave 1 and 13 out of 20 in Wave 2 were
non-discriminating (gap < 10). Total: 16/28 structural units (57%) showed no
meaningful searcher preference. Patterns that never discriminated include:
cascaded switches, dense/sparse switches, callback dispatch, diamond validation,
serialization, equality chains, struct fields, multi-loops, range checks, merge
points, tail-call chains, mostly-sequential code.
**Implication**: Many real-world structural patterns produce programs with tractable
coverage spaces. Searcher selection matters only when the program's state space vastly
exceeds the time budget. Feature: if `total_queries < 100K` or
`total_symbolic_range / time_budget_queries < 10`, expect non-discrimination.

---

## Feature Vector for Automated Classification

Based on these rules, the minimal feature set to predict the best searcher:

| # | Feature | Type | Rules |
|---|---------|------|-------|
| 1 | `sequential_gate_depth` | int | R1 — max number of sequential symbolic branch points on one path |
| 2 | `useless_fork_before_target` | bool | R2 — symbolic branches before target code that don't affect reachability |
| 3 | `concrete_loop_size` | int | R3 — max concrete loop iteration count after symbolic fork resolves |
| 4 | `independent_regions` | int | R4 — number of independent code regions (different functions/switch cases) |
| 5 | `constraint_growth_rate` | float | R5 — how fast path condition size grows with depth |
| 6 | `convergent_divergent` | bool | R6 — paths merge then branch again |
| 7 | `symbolic_loop_bound` | bool | R7 — loop bound depends on symbolic variable |
| 8 | `recursive_branch_factor` | int | R8 — branching factor of recursive functions |
| 9 | `coverage_blind_ops` | int | R9 — count of operations that change value but not coverage (bitfield, accumulator) |
| 10 | `identical_loop_body` | bool | R10 — loop body covers same instructions every iteration |
| 11 | `constraint_cost_variance` | float | R11 — variance in solver cost across branches |
| 12 | `symbolic_index_chain_length` | int | R12 — length of dependent a[a[a[x]]] chains |
| 13 | `symbolic_var_count` | int | All — number of symbolic variables |
| 14 | `total_symbolic_range` | int | All — product of symbolic variable ranges |
| 15 | `max_loop_nesting` | int | All — max loop nesting depth |
| 16 | `loop_body_branch_coverage_in_one_iter` | bool | R10 — can all branches in loop body be covered in a single iteration? |
| 17 | `multi_dimensional_symbolic_loops` | bool | R4/R10 — nested loops where both inner/outer branch on symbolic data |
| 18 | `bitwise_test_count` | int | R4 — number of independent bit-test branches (AND+compare pattern) |
| 19 | `hub_node_count` | int | R6 — functions with high fan-in AND high fan-out |

---

## Evidence Summary

| Rule | Searcher | Hand-Crafted Evidence | CodeContests Evidence | Real-World SU Evidence | Total Cases | Typical Gap |
|------|----------|----------------------|----------------------|----------------------|-------------|-------------|
| R1 Sequential gate | DFS | 5 experiments (gap 90–185) | 0 | 0 | 5 | 120 |
| R2 Useless fork barrier | DFS | 3 experiments (gap 6–55%) | 0 | 0 | 3 | large |
| R3 Symbolic→concrete sweep | DFS | 3 experiments (gap 24–91) | 2 programs (gap 31–33) | 0 | 5 | 40 |
| R4 Wide independent regions | NURS/RP | 7 experiments (gap 32–336) | 5 programs (gap 29–142) | SU02,SU08,SU24(covnew=256,BFS=192),SU26(RP=277,NURS=255) | 16 | 100 |
| R5 Growing constraints | NURS | 4 experiments (gap 16–72) | 0 | 0 | 4 | 50 |
| R6 Convergent-divergent | NURS | 1 experiment (gap 20) | 0 | SU16: non-DFS=306, DFS=244 (gap 62) | 2 | 40 |
| R7 Symbolic loop bound | BFS | 4 experiments (gap 5) | 7 programs (gap 42–166) | 0 | 11 | 60 |
| R8 Recursive explosion | BFS/NURS | 0 | 2 programs (gap 60–139) | SU03(gap 117), SU18(non-DFS=325,DFS=294), SU21(non-DFS=318,DFS=284) | 5 | 80 |
| R9 Coverage-blind | DFS | 2 experiments (gap 109–215) | 0 | SU05(gap 19), SU07(gap 30), SU13(DFS=142,others=110,gap 32) | 5 | 40 |
| R10 Identical loop body | ¬NURS | 4 experiments (gap 5) | 5 programs (gap 33–72) | SU20: DFS/RP=147, NURS=122 (gap 25) | 10 | 35 |
| R11 Constraint asymmetry | qc | 3 experiments (gap 2–15) | 0 | SU06: non-discriminating | 3 | 10 |
| R12 Symbolic pointer chain | RP | 2 experiments (gap 12–17) | 1 program (gap 33) | 0 | 3 | 15 |

---

## What Remains Unknown

1. **Interaction effects**: When multiple rules apply to the same program, we have no systematic data on which rule dominates. The priority ordering in M8 is a hypothesis.

2. **Scaling behavior**: All experiments used 10–30s timeouts. At 60s, 300s, or 3600s, the rankings may shift — DFS's advantage in sequential gating may diminish as other searchers eventually reach the targets.

3. **Solver-specific effects**: All results use STP with `--max-solver-time=5s`. With Z3, qc's advantage may change because query costs are different.

4. **Memory pressure**: BFS and NURS can run out of memory before time. We observed this (exp101: BFS 335, DFS 491) but don't have a systematic rule for predicting it.

5. **Nondeterminism**: Some discriminators may be artifacts of KLEE's internal scheduling nondeterminism. Running each configuration 3× with different seeds would filter these out.

6. **Real-world programs**: Both hand-crafted experiments and CodeContests are artificial. The real-world ICFG analysis addressed this in two waves:
   - **Wave 1 (SU01–SU08)**: 8 coarse structural units from ICFG analysis of gnumake, lua, bc, readelf, nasm, bison, tiffinfo, flvmeta. 5/8 discriminating, 2/5 correctly predicted (40%) → 3 rule evolutions (R4 multi-dimensional dispatch, R8 distinct base cases, R9 fully-symbolic nested loops).
   - **Wave 2 (SU09–SU28)**: Deep fine-grained analysis mined 30+ ICFG pattern categories (transform_loop, call_heavy_loop, hub_node, bitwise_accumulator, nested_scanner, etc.) across all 8 programs. Created 20 targeted snippets. 7/20 discriminating, 5/7 correctly predicted (71.4%) → 4 rule evolutions (R10 transform loop amendment, R4 bitwise flag sub-rule, R4 nested scanner multi-dimensional amendment, R6 hub node confirmation).
   - **Combined**: 28 structural units, 12 discriminating (43%), 7.5/12 correct (62.5%). Prediction accuracy improved from 40% (Wave 1) to 71.4% (Wave 2) as rules evolved.
   - **Key finding**: 57% of real-world-derived patterns (16/28) are non-discriminating — most structural patterns have tractable state spaces where all searchers converge. The patterns that DO discriminate cluster around: coverage-blind loops (R9/R10), convergent-divergent flow (R6), recursive/call-heavy structures (R8), multi-dimensional search spaces (R4 variants), and bitwise flag processing (R4 sub-rule). Two surprise findings: (a) transform loops punish NURS despite having branches (R10 applies), (b) nested scanners punish NURS and favor RP (multi-dimensional effect).
