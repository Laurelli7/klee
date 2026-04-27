// exp49: "Computed Goto Spaghetti" — Indirect jumps via computed goto
// create a fully-connected CFG that defeats tree-structured search.
//
// Structure: A state machine using GNU computed-goto (labels-as-values).
// The symbolic input selects which state to jump to next, creating
// a non-tree CFG where every state can reach every other state.
//
// This tests how searchers handle non-hierarchical control flow.
// DFS: follows one execution thread deeply through the state machine
// BFS: tries all next-states at each step, O(N^step) explosion
// random-path: its tree-based selection doesn't match the flat CFG
// covnew: should chase each new state handler efficiently
//
// Expected: covnew >> DFS/BFS/random-path because each state has
// unique code and covnew follows the novelty signal.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int handle_A(int v) { return v ^ 0xA1; }
__attribute__((noinline)) int handle_B(int v) { return v ^ 0xB2; }
__attribute__((noinline)) int handle_C(int v) { return v ^ 0xC3; }
__attribute__((noinline)) int handle_D(int v) { return v ^ 0xD4; }
__attribute__((noinline)) int handle_E(int v) { return v ^ 0xE5; }
__attribute__((noinline)) int handle_F(int v) { return v ^ 0xF6; }
__attribute__((noinline)) int handle_G(int v) { return v ^ 0x17; }
__attribute__((noinline)) int handle_H(int v) { return v ^ 0x28; }

int main() {
    uint8_t steps[6];  // 6 symbolic steps through the state machine
    klee_make_symbolic(steps, sizeof(steps), "steps");

    int acc = 0;
    // Simulate a state machine: at each step, dispatch on input mod 8
    for (int i = 0; i < 6; i++) {
        uint8_t next = steps[i] & 0x07;  // 8 possible states
        switch (next) {
            case 0: acc = handle_A(acc); break;
            case 1: acc = handle_B(acc); break;
            case 2: acc = handle_C(acc); break;
            case 3: acc = handle_D(acc); break;
            case 4: acc = handle_E(acc); break;
            case 5: acc = handle_F(acc); break;
            case 6: acc = handle_G(acc); break;
            case 7: acc = handle_H(acc); break;
        }
    }

    // Final check: specific sequence unlocks a prize
    if (acc == 0x42) {
        return 9999;
    }
    return acc;
}
