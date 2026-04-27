// exp65: "State Machine with Back-Edges" — A state machine where some
// transitions go BACKWARDS (to earlier states), creating loops in the
// execution graph. The loop bound depends on symbolic input.
//
// This is fundamentally different from tree-structured programs:
// the execution can revisit states, creating unbounded depth paths.
// KLEE normally unrolls loops, so this tests how searchers handle
// loop unrolling with symbolic trip counts.
//
// Structure: 4 states (A, B, C, D). Transitions based on input:
//   A: go to B or C
//   B: go to C or back to A (loop!)
//   C: go to D or back to B (loop!)
//   D: EXIT or back to A (loop!)
//
// Each visit to a state accumulates work. After 8 transitions, force exit.
//
// DFS: may loop many times before exiting
// BFS: tries all transitions at each step, but loops create infinite depth
// covnew: looping creates no new coverage after first visit
// random-path: randomly samples, may or may not loop
//
// Expected: covnew should break loops fastest (no coverage novelty).
// DFS may get stuck. BFS may loop-explode.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int state_A(int v) { return v + 11; }
__attribute__((noinline)) int state_B(int v) { return v + 22; }
__attribute__((noinline)) int state_C(int v) { return v + 33; }
__attribute__((noinline)) int state_D(int v) { return v + 44; }
__attribute__((noinline)) int state_EXIT(int v) { return v + 9999; }

int main() {
    uint8_t transitions[8];  // 8 transition decisions
    klee_make_symbolic(transitions, sizeof(transitions), "trans");

    int acc = 0;
    int state = 0; // 0=A, 1=B, 2=C, 3=D

    for (int step = 0; step < 8; step++) {
        uint8_t t = transitions[step];

        switch (state) {
            case 0: // State A
                acc = state_A(acc);
                if (t & 0x01) state = 1; // -> B
                else          state = 2; // -> C
                break;

            case 1: // State B
                acc = state_B(acc);
                if (t & 0x01) state = 2; // -> C (forward)
                else          state = 0; // -> A (BACK EDGE!)
                break;

            case 2: // State C
                acc = state_C(acc);
                if (t & 0x01) state = 3; // -> D (forward)
                else          state = 1; // -> B (BACK EDGE!)
                break;

            case 3: // State D
                acc = state_D(acc);
                if (t & 0x01) {
                    acc = state_EXIT(acc);
                    return acc;  // EXIT
                } else {
                    state = 0; // -> A (BACK EDGE!)
                }
                break;
        }
    }

    return acc; // ran out of steps
}
