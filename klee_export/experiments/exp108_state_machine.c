// exp108: "State Machine" — A symbolic finite automaton where the
// transition table is partially symbolic. Different states call
// different handler functions. The automaton state AFTER N steps
// depends on the full history of symbolic inputs.
//
// BFS explores all possible state transitions at each step, creating
// maximum diversity of automaton states → maximum coverage of handlers.
// DFS follows one input sequence deeply. covnew/md2u chase specific
// handlers but can't efficiently explore the combinatorial state space.
//
// FSM: 8 states, 4 input symbols (2 bits), 5 steps
// Each state has a unique handler function.
#include "klee/klee.h"
#include <stdint.h>

// 8 state handler functions
__attribute__((noinline)) int state_0(int x) { return x + 0x10; }
__attribute__((noinline)) int state_1(int x) { return x + 0x21; }
__attribute__((noinline)) int state_2(int x) { return x + 0x32; }
__attribute__((noinline)) int state_3(int x) { return x + 0x43; }
__attribute__((noinline)) int state_4(int x) { return x + 0x54; }
__attribute__((noinline)) int state_5(int x) { return x + 0x65; }
__attribute__((noinline)) int state_6(int x) { return x + 0x76; }
__attribute__((noinline)) int state_7(int x) { return x + 0x87; }

// Transition: deterministic but complex (like a real protocol FSM)
static inline int transition(int state, int input) {
    // Each state has different transitions per input
    static const int table[8][4] = {
        {1, 2, 3, 0},  // state 0
        {4, 0, 5, 1},  // state 1
        {0, 6, 2, 7},  // state 2
        {3, 1, 7, 4},  // state 3
        {5, 3, 0, 6},  // state 4
        {2, 7, 6, 5},  // state 5
        {7, 4, 1, 3},  // state 6
        {6, 5, 4, 2},  // state 7
    };
    return table[state][input];
}

int main() {
    uint8_t inputs[5]; // 5 symbolic input bytes (only 2 bits used each)
    klee_make_symbolic(inputs, sizeof(inputs), "inputs");

    int r = 0;
    int state = 0;

    for (int step = 0; step < 5; step++) {
        int inp = inputs[step] & 0x03; // 4 possible inputs

        // Call current state's handler
        switch (state) {
            case 0: r = state_0(r); break;
            case 1: r = state_1(r); break;
            case 2: r = state_2(r); break;
            case 3: r = state_3(r); break;
            case 4: r = state_4(r); break;
            case 5: r = state_5(r); break;
            case 6: r = state_6(r); break;
            case 7: r = state_7(r); break;
        }

        // Transition
        state = transition(state, inp);
    }

    // Final state handler
    switch (state) {
        case 0: r = state_0(r); break;
        case 1: r = state_1(r); break;
        case 2: r = state_2(r); break;
        case 3: r = state_3(r); break;
        case 4: r = state_4(r); break;
        case 5: r = state_5(r); break;
        case 6: r = state_6(r); break;
        case 7: r = state_7(r); break;
    }

    return r;
}
