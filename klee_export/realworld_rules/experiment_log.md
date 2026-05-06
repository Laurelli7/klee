# KLEE Searcher Selection — Experiment Log

Each entry records one feedback-loop iteration: the program tested, the LLM prediction,
the actual KLEE results, whether the prediction was correct, and any rule change triggered.

---

## Rounds

### Round 1 — 2026-05-01

**Source**: synthetic (R9)
**Target rule**: R9
**Program**: /tmp/klee_exp_round1.c

**LLM prediction**: dfs
**Rules cited**: ['R9']
**Reasoning**: The loop is a coverage-blind hash accumulator (FNV-style mix) where every path executes identical instructions and only the final byte-comparison cascade differentiates outcomes, so DFS reaches the terminal checks fastest while NURS heuristics see no coverage signal.

**KLEE results** (30s budget per searcher):
  searcher            coverage   queries
  dfs                 coverage=     0  queries=       0  <-- WINNER
  bfs                 coverage=     0  queries=       0
  random-path         coverage=     0  queries=       0
  nurs:covnew         coverage=     0  queries=       0
  nurs:md2u           coverage=     0  queries=       0
  nurs:qc             coverage=     0  queries=       0

**Actual winner**: dfs
**Outcome**: CORRECT

**Rule change**: none

---

---

## Summary Statistics

| Metric | Value |
|--------|-------|
| Total rounds | 1 |
| Correct predictions | 1 |
| Wrong predictions | 0 |
| Prediction accuracy | 1/1 (100%) |
| Rule changes triggered | 0 |

### By rule tested

| Rule | Rounds | Correct | Accuracy |
|------|--------|---------|----------|
| R1 | 0 | 0 | — |
| R2 | 0 | 0 | — |
| R3 | 0 | 0 | — |
| R4 | 0 | 0 | — |
| R4a | 0 | 0 | — |
| R4b | 0 | 0 | — |
| R4c | 0 | 0 | — |
| R5 | 0 | 0 | — |
| R6 | 0 | 0 | — |
| R7 | 0 | 0 | — |
| R8 | 0 | 0 | — |
| R9 | 0 | 0 | — |
| R10 | 0 | 0 | — |
| R11 | 0 | 0 | — |
| R12 | 0 | 0 | — |
