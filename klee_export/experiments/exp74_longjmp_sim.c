// exp74: "Longjmp Simulation" — Simulates setjmp/longjmp-style
// non-local control flow using goto. When a "longjmp" fires, execution
// jumps backward, potentially revisiting code with different constraints.
//
// This creates a control flow pattern that doesn't match any searcher's
// model: not a tree, not a DAG, but a graph with cross-edges.
//
// Structure: 3 "try blocks" that can "throw" (goto to handler).
// Each handler processes the exception differently.
// Symbolic input determines which blocks throw.
// After all try blocks, a final section combines results.
//
// The non-local jumps mean that coverage-guided searchers see
// "handler code" as highly novel (each handler is unique), but the
// handler may be reached from multiple try blocks with different
// constraint histories.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int try_a_work(int x) { return x + 100; }
__attribute__((noinline)) int try_b_work(int x) { return x + 200; }
__attribute__((noinline)) int try_c_work(int x) { return x + 300; }
__attribute__((noinline)) int handler_overflow(int x) { return x * 2 + 1; }
__attribute__((noinline)) int handler_underflow(int x) { return x / 2 + 1; }
__attribute__((noinline)) int handler_invalid(int x) { return -x; }
__attribute__((noinline)) int finally_clean(int x) { return x + 9999; }

int main() {
    uint8_t input[4];
    uint8_t noise[2];
    klee_make_symbolic(input, sizeof(input), "input");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int result = 0;
    int exception = 0; // 0=none, 1=overflow, 2=underflow, 3=invalid

    // Noise preamble
    for (int i = 0; i < 2; i++) {
        if (noise[i] & 0x01) result++;
        if (noise[i] & 0x02) result++;
        if (noise[i] & 0x04) result++;
        if (noise[i] & 0x08) result++;
        if (noise[i] & 0x10) result++;
        if (noise[i] & 0x20) result++;
        if (noise[i] & 0x40) result++;
        if (noise[i] & 0x80) result++;
    }

    // Try block A
    result = try_a_work(result);
    if (input[0] > 200) {
        exception = 1;
        goto handle_exception;
    }
    if (input[0] < 10) {
        exception = 2;
        goto handle_exception;
    }

    // Try block B (only reached if A didn't throw)
    result = try_b_work(result);
    if (input[1] > 200) {
        exception = 1;
        goto handle_exception;
    }
    if (input[1] == input[0]) {
        exception = 3;
        goto handle_exception;
    }

    // Try block C
    result = try_c_work(result);
    if ((uint16_t)input[2] + (uint16_t)input[3] > 400) {
        exception = 1;
        goto handle_exception;
    }
    if (input[2] == 0 && input[3] == 0) {
        exception = 2;
        goto handle_exception;
    }

    // No exception: clean exit
    goto finally;

handle_exception:
    switch (exception) {
        case 1: result = handler_overflow(result); break;
        case 2: result = handler_underflow(result); break;
        case 3: result = handler_invalid(result); break;
    }

finally:
    result = finally_clean(result);

    // Post-finally classification
    if (exception == 0 && result > 10000) return 1; // clean big result
    if (exception == 1) return 2;           // overflow handled
    if (exception == 2) return 3;           // underflow handled
    if (exception == 3) return 4;           // invalid handled
    return 0;
}
