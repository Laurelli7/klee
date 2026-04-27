# CodeContests Programs — KLEE Searcher Benchmark

**484 competitive programming solutions** from Google's CodeContests dataset, transformed to KLEE-compatible C and benchmarked across 6 search strategies.

## Directory Structure

```
codecontests_programs/
├── src/           484 transformed .c source files (KLEE-compatible)
├── bitcode/       484 compiled LLVM bitcode (.bc) files
├── results/       2904 KLEE output directories (484 × 6 searchers, symlinked)
├── analysis/
│   ├── searcher_comparison.csv      Full results: CovI, States, Queries, etc.
│   └── discriminator_analysis.md    Detailed analysis of 24 discriminating programs
└── README.md
```

## Experiment Settings

- **KLEE**: 3.2-pre, LLVM 13.0.1
- **Searchers**: dfs, bfs, random-path, nurs:covnew, nurs:md2u, nurs:qc
- **Limits**: `--max-time=10s --max-solver-time=5s`, `ulimit -s unlimited`
- **Timeout wrapper**: `timeout --kill-after=5 30`

## Pipeline

1. Downloaded competitive programming solutions from CodeContests (Google Research)
2. Transformed C++ solutions to KLEE-compatible C via `transform.py` → `transform4.py`
3. Compiled 1027 transformed .c → 484 successfully compiled to LLVM bitcode
4. Ran each of 484 programs with all 6 searchers → 2904 result directories

## Key Results

- **284/484** programs completed all 6 searchers within the timeout
- **24/284** (8.5%) showed any CovI difference between searchers (discriminators)
- **91.5%** of programs are non-discriminating — all searchers get equal coverage

### Discriminator Categories

| Category | Count | Key Programs (CovI gap) |
|----------|-------|------------------------|
| BFS strict win | 7 | train_08285 (166), train_07438 (139), train_04207 (96) |
| Non-DFS strict win | 5 | train_06293 (142), train_05400 (51), train_08408 (49) |
| {DFS,BFS,RP} > NURS | 4 | train_02120 (72), train_08412 (48), train_08183 (33) |
| DFS+BFS > rest | 3 | train_00473 (460), train_06156 (57), train_06744 (42) |
| DFS strict win | 2 | train_02647 (33), train_00389 (31) |
| Other/noise | 4 | train_05467 (54), train_04164 (22), train_04744 (4), train_05935 (4) |

### 6 Structural Patterns Discovered

1. **Symbolic loop bound → BFS wins**: Different `n` values cover different instructions
2. **Recursive/combinatorial explosion → BFS wins**: BFS reaches base cases first
3. **Large concrete sweep after symbolic setup → DFS wins**: DFS completes one full sweep
4. **Many-valued branching → non-DFS wins**: Diversification covers more paths
5. **Repeated identical code → NURS loses**: Coverage heuristics see "already covered"
6. **Symbolic array writes → DFS+BFS > NURS**: NURS distracted by index forks

## CSV Schema

```
program,searcher,CovI,States,Queries,SolverTime_us,WallTime_s,CompletedPaths,NumBranches
```

## Origin

Original data and scripts: `../codecontests/`
Results are symlinked from: `../codecontests/results/`
