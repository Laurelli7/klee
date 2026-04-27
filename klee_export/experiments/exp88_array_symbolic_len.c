// exp88: "Symbolic-Length Processing Pipeline" — A sequence of
// processing stages where the NUMBER of stages is symbolic.
//
// Unlike fixed-depth experiments, here the DEPTH ITSELF is
// symbolic. KLEE must fork on the loop condition at each iteration.
// After N iterations (symbolic), different post-processing
// functions are called based on the accumulated state.
//
// The execution tree has a "staircase" shape: at each loop
// iteration, there's a fork for "continue vs exit." States that
// exit early have fewer constraints; states that exit late have more.
//
// DFS: goes deepest, exits only at max_iter, then backtracks
// BFS: maintains states at ALL depths simultaneously
// qc: should prefer early-exit states (fewer loop constraints)
// covnew: should prefer the first exit at each depth (new exit code)
// md2u: should prefer states near uncovered post-processing code
//
// Expected: clear staircase in time-to-coverage curves.
//           covnew reaches all depths fastest.
//           qc reaches shallow depths fastest.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int process(int x, int step) {
    return x + step * 17 + 1;
}

__attribute__((noinline)) int post_0(int x) { return x + 10000; }
__attribute__((noinline)) int post_1(int x) { return x + 20000; }
__attribute__((noinline)) int post_2(int x) { return x + 30000; }
__attribute__((noinline)) int post_3(int x) { return x + 40000; }
__attribute__((noinline)) int post_4(int x) { return x + 50000; }
__attribute__((noinline)) int post_5(int x) { return x + 60000; }
__attribute__((noinline)) int post_6(int x) { return x + 70000; }
__attribute__((noinline)) int post_7(int x) { return x + 80000; }

int main() {
    uint8_t max_iter_sym;  // symbolic loop bound
    uint8_t ops[8];        // operation per iteration
    uint8_t noise;
    klee_make_symbolic(&max_iter_sym, sizeof(max_iter_sym), "max_iter");
    klee_make_symbolic(ops, sizeof(ops), "ops");
    klee_make_symbolic(&noise, sizeof(noise), "noise");

    int acc = 0;
    int max_iter = (max_iter_sym & 0x07); // 0 to 7

    // Noise
    if (noise & 0x01) acc++;
    if (noise & 0x02) acc++;
    if (noise & 0x04) acc++;
    if (noise & 0x08) acc++;
    if (noise & 0x10) acc++;
    if (noise & 0x20) acc++;
    if (noise & 0x40) acc++;
    if (noise & 0x80) acc++;

    // Symbolic-length pipeline
    for (int i = 0; i < max_iter; i++) {
        acc = process(acc, i);
        // Each iteration also has a branch on the op byte
        if (ops[i] & 0x80) {
            acc = acc * 2;
        } else {
            acc = acc + 1;
        }
    }

    // Post-processing depends on how many iterations ran
    switch (max_iter) {
        case 0: acc = post_0(acc); break;
        case 1: acc = post_1(acc); break;
        case 2: acc = post_2(acc); break;
        case 3: acc = post_3(acc); break;
        case 4: acc = post_4(acc); break;
        case 5: acc = post_5(acc); break;
        case 6: acc = post_6(acc); break;
        case 7: acc = post_7(acc); break;
    }

    if (acc == 42) return 99999;
    return acc & 0xFFFF;
}
