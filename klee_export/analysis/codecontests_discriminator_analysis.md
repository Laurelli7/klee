# CodeContests Searcher Discriminator Analysis

**Dataset**: 284 complete programs (all 6 searchers finished), from 484 CodeContests bitcode files  
**Searchers**: dfs, bfs, random-path, nurs:covnew, nurs:md2u, nurs:qc  
**Settings**: `--max-time=10s --max-solver-time=5s`, `ulimit -s unlimited`, `timeout --kill-after=5 30`  
**Discriminators**: 24 / 284 programs (8.5%) have any CovI difference between searchers  
**Run still in progress** — 284/484 programs complete at time of analysis

---

## Summary Table

| Category | Count | Programs (gap) |
|----------|-------|----------------|
| BFS strict win | 7 | train_08285 (166), train_07438 (139), train_04207 (96), train_06827 (60), train_04115 (56), train_08415 (45), train_03628 (42) |
| non-DFS strict win | 5 | train_06293 (142), train_05400 (51), train_08408 (49), train_07095 (5), train_04662 (1) |
| {DFS,BFS,RP} > NURS | 4 | train_02120 (72), train_08412 (48), train_08183 (33), train_01754 (15) |
| DFS+BFS > rest | 3 | train_00473 (460), train_06156 (57), train_06744 (42) |
| DFS strict win | 2 | train_02647 (33), train_00389 (31) |
| Other pattern | 4 | train_05467 (54), train_04164 (22), train_04744 (4), train_05935 (4) |

---

## BFS Strict Wins (7 programs)

BFS alone gets the highest CovI; all other searchers score lower.

### train_08285 — BFS gap=166  (BFS=1175, others=1009–1014)

**Structure**: Triple-nested DP loop over `c[500][500]` and `dp[500][500]`. Builds Pascal's triangle in `c[][]`, then a DP table `dp[i][l+r]` with multiplications and additions mod 1e9+7. Single symbolic variable `n` bounded [1,5]. The DP loop depth depends on `n` — for `n>=2`, enters a 400×400×400 inner loop.

**Why BFS wins**: BFS explores n=1 first (trivial — just prints dp[1][1]), banking coverage from the setup loops (Pascal's triangle: 401 iterations × ~5 instructions). For n>=2, all searchers enter the massive 400³ DP kernel. BFS has already banked the n=1 print path coverage. DFS/NURS may commit to n=2+ first and spend all execution time inside the DP kernel, never finishing enough iterations to reach the print statement, so they get less distinct instruction coverage.

**Stats**: BFS: 6.5M queries, 0s solver, 10s wall. Others: 1.5M queries, 11s solver, ~14s wall. BFS runs entirely without solver overhead (no symbolic in the hot loop), while others somehow incur solver time.

### train_07438 — BFS gap=139  (BFS=1197, others=1058–1133)

**Structure**: Finds `n` points on a circle-annulus that maximize pairwise distance. `go()` is recursive: chooses `n` points from `k` candidates via recursive enumeration (combinatorial C(k,n)). Two symbolic vars: `n` [1,5] and `r` [-20,20]. `k = 30 - n*2` — so k ranges 20–28 depending on n.

**Why BFS wins**: The recursive `go()` function has combinatorial explosion. BFS explores shallow paths first — for small n (1,2), the recursion tree is tiny and completes quickly, covering the pairwise-distance computation and print logic. For large n with many candidates, the recursion tree is deep and wide. DFS commits to one deep recursive branch and never completes enough `go()` calls to reach the bottom (where coverage of the comparison/assignment logic happens). BFS systematically reaches the base case for many parameter combinations.

**Stats**: BFS: 373K queries, 12.9s solver. DFS: 4M queries, 4.2s solver. DFS churns through queries fast (shallow solver work) but in a narrow part of the recursion tree.

### train_04207 — BFS gap=96  (BFS=1523, RP=1520, DFS=1433, NURS=1427)

**Structure**: Constructs a tournament graph. Uses hardcoded 6×6 and 3×3 adjacency matrices for small cases. For `n>6`, extends by adding pairs of vertices with edges to all previous. Then fills in remaining edges to make it a tournament. Single symbolic var `n` [1,5].

**Why BFS wins**: n=4 returns immediately with "-1". For n=1,2,3,5 the code follows different paths (odd vs even). BFS covers all values of n at each decision point before going deep. The nested loops `for i, for j` iterate up to n² times. Since n is symbolic, KLEE forks on loop bounds. BFS ensures all 5 values of n progress equally through the loops, while NURS heuristics may focus on one particular n value and miss coverage from others.

### train_06827 — BFS gap=60  (BFS=993, DFS=966, RP=944, NURS=933)

**Structure**: Complex recursive tree algorithm. Builds a binary tree with `ls[]`/`rs[]`/`f[]` parent pointers. `dfz()` finds centroids, calls itself recursively. `solve()` rebuilds adjacency list, runs DFS then `dfz()`. Interactive: reads a symbolic char to decide which subtree to recurse into (X, Z, or other). Multiple symbolic vars.

**Why BFS wins**: The recursive `dfz()` branches three ways based on symbolic char input (X/Z/else). BFS explores all three branches at each recursion level before going deeper. DFS picks one branch per level and follows it to the bottom, missing the other two subtrees' code. The centroid decomposition creates different tree shapes depending on the branch taken, so different branches cover different instruction sequences.

### train_04115 — BFS gap=56  (BFS=1052, DFS=1033, RP=1016, NURS=996)

**Structure**: Array `a[1000005]` (4MB). Fills array with a pattern based on `n`: odd/even elements placed at positions computed from n. Single symbolic var `n` [1,5]. Loops iterate up to `2*n`. Large array but only small portions accessed for n≤5.

**Why BFS wins**: For n=1, the loops don't execute (1<1 is false), so it goes straight to the print loop which is also trivial (print one element). For n>1, fills array positions up to 2n. BFS explores n=1 first (fast, covers print logic), then progresses to n=2,3,4,5 which each cover a few more loop iterations. DFS likely picks one value of n and exhausts it, but the array indexing creates solver overhead. NURS heuristics deprioritize states that produce "familiar" coverage patterns, but the print loop at the end covers unique instructions per iteration.

### train_08415 — BFS gap=45 (marginal)  (BFS=1074, others=1029–1073)

**Structure**: Bit manipulation. Loops over bit positions of B, computes ranges with shifts and bitwise AND/OR. Two symbolic vars A, B in [-20,20].

**Why BFS wins**: The `for(i=0; B>>i; i++)` loop iterates based on the number of bits in B. Different values of B produce different loop iteration counts. BFS explores all combinations of (A,B) at uniform depth, ensuring it covers both the small-B paths (few iterations) and large-B paths. Others slightly favor one direction.

### train_03628 — BFS gap=42  (BFS=993, others=951–953)

**Structure**: Simple DP. `f[1]=n`, `f[2]=n*n%P`, then `f[i]` computed from previous values in a loop `for i=3..n`. Single symbolic var `n` [1,5].

**Why BFS wins**: For n=1 and n=2, the loop body never executes (3≤1 and 3≤2 both false), covering only the array initialization and printf. For n≥3, the loop executes and covers the `sum`, modular arithmetic, and accumulation logic. BFS explores n=1,2 first (immediate coverage of init+print), then n=3,4,5 (loop body coverage). Critical detail: BFS finishes in 1.1s with 3 states remaining — it completed execution. Others finish in 1.1s with 2 states, suggesting they skipped one path. The gap of 42 CovI comes from BFS covering one additional n value's unique loop iterations.

---

## DFS Strict Wins (2 programs)

### train_02647 — DFS gap=33  (DFS=998, NURS≈971, RP=987, BFS=965)

**Structure**: Loop writes 1 to `xx[x]` and `yy[y]` where x,y are symbolic, then counts how many slots are set. Three symbolic vars: `n` [1,5], `x` [-20,20], `y` [-20,20], with `x` and `y` re-symbolized each iteration.

**Why DFS wins**: DFS commits to one sequence of (x,y) values and explores it deeply. Since x and y are re-symbolized each loop iteration, each iteration forks on the symbolic write targets. DFS's depth-first approach means it reaches the counting loop (which covers unique instructions per distinct count value) sooner for specific input sequences. BFS spreads across all loop-iteration forks equally, meaning it never finishes any single execution trace through to the counting loop — it's stuck at the first fork point with many pending states.

### train_00389 — DFS gap=31  (DFS=987, NURS=962–963, RP=959, BFS=956)

**Structure**: Union-find on `f[1202020]`. Reads `n` values of `aa` (symbolic) and increments `f[aa]++`. Then iterates `i=2..1000000`, for each `i` sums `f[i*j]` for `j*i ≤ 1000000`. Determines whether numbers are pairwise coprime, setwise coprime, or not coprime.

**Why DFS wins**: DFS: 838K queries in 10.4s. BFS: 8K queries in 17.3s. DFS makes 100× more queries — it races through the inner loop (purely concrete arithmetic on the `f[]` array after the symbolic input phase). The symbolic inputs determine which `f[]` slots are incremented, and DFS picks one concrete assignment and runs the entire million-element scan quickly. BFS/NURS fork on the symbolic `aa` writes and never reach the coprime-checking phase in the allotted time.

---

## Non-DFS Strict Wins (5 programs)

All non-DFS searchers beat DFS.

### train_06293 — non-DFS gap=142  (RP/covnew/md2u=1127, qc=1105, BFS=992, DFS=985)

**Structure**: Floating-point computation with `log()` and `exp()`. Three symbolic vars: `len`, `maxz`, `numx` all in [-20,20]. Double-nested loop `for f1..len, for f2..len` with condition `(f1+f2)*len - f1*f2 <= numx`. Accesses `szh[100100]` array and `szz[500]` array.

**Why non-DFS wins**: DFS picks one assignment of (len,maxz,numx) and follows it deep into the nested loop. But the loop body's instruction coverage depends heavily on which branch of the conditional is taken, and different (len,maxz,numx) values exercise different branch patterns. RP/NURS diversify across the 3-variable symbolic space. covnew/md2u specifically target uncovered instructions (different conditional branches), which map to different variable combinations. BFS does better than DFS but worse than NURS — it explores some diversification but not as efficiently as coverage-directed search.

### train_05400 — non-DFS gap=51  (all non-DFS=993, DFS=942)

**Structure**: Recursive factorial `fact(n)`. Single symbolic var `a` [-20,20].

**Why DFS loses**: The recursive `fact()` unfolds differently per value of `a`. For a=0, returns 1 immediately. For a=1, one recursive call. For a=20, 20 recursive calls. DFS picks one value of `a` and follows the full recursion — if it picks a large value, it covers the recursive descent instructions but misses the base case logic for small values. If it picks a small value, it misses the deep-recursion instructions. All non-DFS searchers diversify: they try multiple values of `a`, covering both shallow and deep recursion branches. Since coverage is cumulative, covering a=0,1,2,...,20 gives more total unique instructions than any single deep trace.

### train_08408 — non-DFS gap=49  (all non-DFS=1033, DFS=984)

**Structure**: Brute force: nested loops `for i=1..150, for j=-150..150`, checks `i^5 - j^5 == x` where x is symbolic [-20,20]. Returns first (i,j) found.

**Why DFS loses**: DFS picks one value of x and searches the entire 150×300 grid. All other searchers try multiple x values concurrently, and different x values hit the `return printf(...)` statement at different (i,j) points, covering the print/return logic sooner. DFS may pick an x with no solution (stalls in the loop) or one with a late-occurring solution (covers only loop overhead, not the print path).

### train_07095 — non-DFS gap=5 (marginal)
### train_04662 — non-DFS gap=1 (noise)

---

## DFS+BFS Strict Win (3 programs)

DFS and BFS both beat RP and all NURS variants.

### train_00473 — DFS+BFS gap=460  (DFS=BFS=1401, RP=942, NURS=941)

**Structure**: Union-find-like grouping. Array `g[kMaxn]`, `h[kMaxn]` (100K elements each). Loop `for i=1..n` with conditional assignments based on `g[i]`, `g[a[i]]`. After grouping, validation loops check consistency. Single symbolic var `n` [1,5]. Uses `scanf()` to read `a[i]`.

**Why DFS+BFS win**: All searchers make ~9-10M queries in 10s with 0s solver time — this is entirely concrete execution (the scanf reads are concrete after the initial fork on n). The gap of 460 CovI is massive. DFS and BFS both reach 1401, while RP/NURS are stuck at 941–942. The key difference: `scanf()` returns concrete values that depend on stdin state. DFS/BFS process the states in an order where the scanf calls return values that exercise more array paths. RP/NURS select states in a different order, and the scanf calls hit different stdin positions, producing different (less diverse) `a[]` values. This is an **artifact of stdin interaction order**, not a fundamental structural advantage.

### train_06156 — DFS+BFS gap=57  (DFS=BFS=1065, RP=1061, covnew/md2u=1049, qc=1008)

**Structure**: String processing. QuickMul (binary multiplication mod P). Scans string to find first and last positions where adjacent chars differ. Computes Left×Right or Left+Right-1 depending on whether first and last chars match. Single symbolic var `n` [1,5], string `Str[]` uninitialized (zero-filled).

**Why DFS+BFS win**: With Str uninitialized (all zeros), `Str[i] != Str[i-1]` is always false, so Left stays 0 and Right stays N+1. The conditional `Str[1] == Str[N]` is always true (both 0), calling QuickMul(0, N+1). DFS/BFS explore different n values and cover the full QuickMul bit-loop (log₂(N+1) iterations). The gap between DFS/BFS and NURS suggests NURS spends time on states that don't contribute additional coverage.

### train_06744 — DFS+BFS gap=42  (DFS=BFS=1012, RP=1008, NURS=970–972)

**Structure**: Reads n-1 symbolic values `tmp` into `a[tmp]++` (frequency counting on array[200010]). Then prints `a[1]` through `a[n]`. Two symbolic vars: `n` [1,5], `tmp` re-symbolized each iteration [-20,20].

**Why DFS+BFS win**: Symbolic writes to `a[tmp]` fork on the write target. DFS/BFS both execute the print loop for at least some execution traces (DFS picks one trace and completes it; BFS reaches the print loop at uniform depth). NURS heuristics get distracted by the symbolic-array-write forks and never reach the print loop for enough traces.

---

## {DFS, BFS, RP} > NURS (4 programs)

All three non-NURS searchers beat all three NURS variants.

### train_02120 — gap=72  (BFS=RP=1129, DFS=1083, NURS=1057)

**Structure**: Two-matrix convolution. Nested loops over 51×51 matrices. Slides one matrix over another (`for x=-50..50, y=-50..50`) counting element-wise products. Four symbolic vars: n1, m1, n2, m2 all in [-20,20]. Uses `scanf()` for matrix contents.

**Why NURS loses**: NURS tries to maximize coverage by picking states with uncovered instructions, but the convolution loop body is the same code for all (x,y) offsets — coverage heuristics see "already covered" and deprioritize new offset explorations. DFS/BFS/RP don't have this bias and explore more offset combinations, reaching the `printf("%d %d", X, Y)` output with more diverse (X,Y) values. This is the classic "BFS wins" pattern from the hand-crafted experiments: many states execute identical code but produce different internal state.

### train_08412 — gap=48  (DFS=BFS=RP=1038, covnew/md2u=999, qc=990)

**Structure**: Grid-based DP with 505×505 arrays (sumv, rpos, minn, maxn). Reads an n×n grid of chars, computes aggregates with XOR toggling between two layers. Single symbolic var `n` [1,5].

**Why NURS loses**: The DP kernel iterates over the grid in nested loops. Each iteration covers the same instructions. covnew/md2u see "already covered" in the DP body and deprioritize further iterations, but the accumulator `ans` changes differently depending on which cells are processed. qc is worse because the DP involves no solver queries at all (purely concrete), so query-cost heuristic gives no useful signal.

### train_08183 — gap=33  (RP=1016, DFS=BFS=1006, NURS=983–984)

**Structure**: Fenwick tree (BIT). Loop inserts symbolic values via `add(x)`, then queries `qu(R)` in a while loop that decrements R. Two symbolic vars: `n` [1,5], `x` re-symbolized each iteration [-20,20].

**Why NURS loses**: The Fenwick tree operations (`add`, `qu`) traverse a fixed set of instructions for any input. NURS deprioritizes states whose next instructions are "already covered." But different symbolic `x` values modify different tree cells, leading to different query results and different R values — so the `printf` at the end produces different outputs with different format-string arguments. RP wins because it randomly selects among all waiting states, naturally diversifying.

### train_01754 — gap=15 (small)

---

## Other Patterns (4 programs)

### train_05467 — {RP, NURS} > {DFS, BFS}, gap=54

**Structure**: Grid simulation. Moves on a grid with symbolic string directions (R/L/U/D). Tracks boundaries (xmin/xmax/ymin/ymax) and accumulates answer. Three loops: first pass, second pass, then infinite loop. Three symbolic vars: n [1,5], a [-20,20], b [-20,20]. Reads direction string via scanf.

**Why RP+NURS win**: The boundary conditions (xmax+x >= a, xmin+x < 0, etc.) create many forks. RP/NURS diversify across these forks, covering both the boundary-hit and boundary-miss branches. DFS commits to one sequence of boundary outcomes. BFS spreads too thinly across the combinatorial explosion of boundary conditions at each step.

### train_04164 — {BFS, RP, covnew, md2u} > {DFS, qc}, gap=22

All except DFS and qc tie at 1006. qc is worst at 984 — it deprioritizes states with expensive queries, but here all queries are similarly cheap, so qc's heuristic gives no advantage and its arbitrary tiebreaking is worse than random.

### train_04744 — gap=4 (noise)
### train_05935 — gap=4 (noise)

---

## Structural Patterns That Create Searcher Differences

### Pattern 1: Symbolic loop bound with value-dependent coverage (BFS wins)
Programs where `n` is symbolic [1,5] and determines loop iteration count. Different n values cover different instructions (loop body iterations, base cases). BFS explores all n values at uniform depth.  
**Examples**: train_08285, train_04207, train_03628, train_04115

### Pattern 2: Recursive/combinatorial explosion (BFS wins)
Programs with recursive functions whose call tree grows combinatorially. BFS reaches base cases for small inputs while DFS gets lost in deep branches.  
**Examples**: train_07438, train_06827

### Pattern 3: Large concrete sweep after symbolic setup (DFS wins)
Programs that read symbolic input (small), then do a massive concrete computation (e.g., scan 1M array elements). DFS picks one concrete assignment and finishes the sweep. Others spread across symbolic forks and never complete the sweep.  
**Examples**: train_00389, train_02647

### Pattern 4: Many-valued branching with uniform code (non-DFS wins)
Programs where a single symbolic variable controls which of many code paths executes. DFS picks one path. Non-DFS searchers cover multiple paths.  
**Examples**: train_05400 (factorial recursion depth), train_08408 (brute force grid search)

### Pattern 5: Repeated identical code with different internal state (NURS loses)
Loops where every iteration runs the same instructions but mutates state differently. Coverage heuristics see "already covered" and deprioritize, but the mutations matter for downstream coverage.  
**Examples**: train_02120, train_08412, train_08183

### Pattern 6: Symbolic array writes creating fork explosion (DFS+BFS > NURS)
Programs that write to `a[symbolic_index]` inside loops. Each write forks on the target index. NURS gets distracted by these forks; DFS/BFS both reach the post-loop code through at least some traces.  
**Examples**: train_06744, train_00473

---

## Raw Numbers

| Program | DFS | BFS | RP | covnew | md2u | qc | Category |
|---------|-----|-----|----|--------|------|----|----------|
| train_08285 | 1009 | **1175** | 1012 | 1014 | 1014 | 1014 | BFS strict |
| train_07438 | 1083 | **1197** | 1133 | 1085 | 1085 | 1058 | BFS strict |
| train_04207 | 1433 | **1523** | 1520 | 1427 | 1427 | 1427 | BFS strict |
| train_06827 | 966 | **993** | 944 | 933 | 933 | 933 | BFS strict |
| train_04115 | 1033 | **1052** | 1016 | 996 | 996 | 996 | BFS strict |
| train_08415 | 1029 | **1074** | 1073 | 1073 | 1073 | 1073 | BFS strict |
| train_03628 | 951 | **993** | 953 | 953 | 953 | 953 | BFS strict |
| train_02647 | **998** | 965 | 987 | 971 | 970 | 972 | DFS strict |
| train_00389 | **987** | 956 | 959 | 963 | 963 | 962 | DFS strict |
| train_06293 | 985 | 992 | **1127** | **1127** | **1127** | 1105 | non-DFS |
| train_05400 | 942 | **993** | **993** | **993** | **993** | **993** | non-DFS |
| train_08408 | 984 | **1033** | **1033** | **1033** | **1033** | **1033** | non-DFS |
| train_07095 | 1112 | **1117** | **1117** | **1117** | **1117** | **1117** | non-DFS |
| train_04662 | 1065 | **1066** | **1066** | **1066** | **1066** | **1066** | non-DFS |
| train_00473 | **1401** | **1401** | 942 | 941 | 941 | 941 | DFS+BFS |
| train_06156 | **1065** | **1065** | 1061 | 1049 | 1049 | 1008 | DFS+BFS |
| train_06744 | **1012** | **1012** | 1008 | 972 | 970 | 972 | DFS+BFS |
| train_02120 | 1083 | **1129** | **1129** | 1057 | 1057 | 1057 | DFS+BFS+RP > NURS |
| train_08412 | **1038** | **1038** | **1038** | 999 | 999 | 990 | DFS+BFS+RP > NURS |
| train_08183 | 1006 | 1006 | **1016** | 983 | 983 | 984 | DFS+BFS+RP > NURS |
| train_01754 | **1057** | **1057** | **1057** | 1053 | 1053 | 1042 | DFS+BFS+RP > NURS |
| train_05467 | 1267 | 1260 | **1314** | **1314** | **1314** | **1314** | RP+NURS > DFS+BFS |
| train_04164 | 1002 | **1006** | **1006** | **1006** | **1006** | 984 | other |
| train_04744 | **1450** | 1446 | 1446 | 1446 | **1450** | **1450** | noise |
| train_05935 | 1175 | 1175 | **1179** | **1179** | **1179** | **1179** | noise |
