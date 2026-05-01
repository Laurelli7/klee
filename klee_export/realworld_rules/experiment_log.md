# KLEE Searcher Selection — Experiment Log

Each entry records one feedback-loop iteration: the program tested, the LLM prediction,
the actual KLEE results, whether the prediction was correct, and any rule change triggered.

Format per entry:
- **Round**: iteration number and date
- **Source**: synthetic (LLM-generated to target a rule) or real (from GNU ICFG chunk)
- **Target rule**: which rule the program was designed to exercise
- **Program**: path to the generated .c file
- **Prediction**: what the LLM predicted and which rules it cited
- **KLEE results**: coverage by searcher (sorted)
- **Outcome**: CORRECT / WRONG
- **Rule change**: what was updated in rules_summary.md (if anything)

---

## Round Template

```
### Round N — YYYY-MM-DD

**Source**: synthetic | real (fn_name from program.bc)
**Target rule**: RX
**Program**: /tmp/klee_exp_roundN.c

**LLM prediction**: searcher_name
**Rules cited**: [RX, RY, ...]
**Reasoning**: one sentence

**KLEE results** (Xsec budget):
  searcher        coverage   queries
  dfs                  NNN    NNNNN
  bfs                  NNN    NNNNN
  random-path          NNN    NNNNN
  nurs:covnew          NNN    NNNNN
  nurs:md2u            NNN    NNNNN
  nurs:qc              NNN    NNNNN

**Actual winner**: searcher_name
**Outcome**: CORRECT | WRONG (predicted X, actual Y)

**Analysis**: what structural feature drove the result

**Rule change**: none | [description of change to rules_summary.md]
```

---

## Rounds

*(no rounds logged yet — run `experiment.py` to populate)*

---

## Summary Statistics

| Metric | Value |
|--------|-------|
| Total rounds | 0 |
| Correct predictions | 0 |
| Wrong predictions | 0 |
| Prediction accuracy | — |
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
