// exp76: "Aliased Pointers" — Two pointers that MAY alias the same
// memory location depending on symbolic conditions. This creates
// a fundamentally different kind of fork: not a branch fork but
// an ALIAS fork (does p == q or p != q?).
//
// Structure: Two pointers into a 4-element array, selected by
// symbolic indices. If both point to the same cell, writes through
// one affect reads through the other. If they don't alias,
// they're independent.
//
// This is extremely common in real C programs (container operations,
// pointer arithmetic) and creates a kind of state space that
// branch-focused searchers don't handle well.
//
// KLEE resolves this by forking on pointer equality at each access.
// The fork pattern is: for each access through p, fork on
// "does p point to arr[0]? arr[1]? arr[2]? arr[3]?"
//
// Expected: DFS picks one aliasing pattern and follows it.
// covnew sees new code in each aliasing handler.
// md2u may see different distances depending on aliasing.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int alias_hit(int x) { return x + 5000; }
__attribute__((noinline)) int no_alias(int x) { return x + 1000; }
__attribute__((noinline)) int double_write(int x) { return x + 8000; }
__attribute__((noinline)) int zero_result(int x) { return x + 100; }

int main() {
    uint8_t idx_p, idx_q;
    uint8_t val_p, val_q;
    uint8_t noise[3];
    klee_make_symbolic(&idx_p, sizeof(idx_p), "idxp");
    klee_make_symbolic(&idx_q, sizeof(idx_q), "idxq");
    klee_make_symbolic(&val_p, sizeof(val_p), "valp");
    klee_make_symbolic(&val_q, sizeof(val_q), "valq");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int arr[4] = {0, 0, 0, 0};
    int result = 0;

    // Noise
    for (int i = 0; i < 3; i++) {
        if (noise[i] & 0x01) result++;
        if (noise[i] & 0x02) result++;
        if (noise[i] & 0x04) result++;
        if (noise[i] & 0x08) result++;
        if (noise[i] & 0x10) result++;
        if (noise[i] & 0x20) result++;
        if (noise[i] & 0x40) result++;
        if (noise[i] & 0x80) result++;
    }

    // Constrain indices to valid range
    uint8_t p = idx_p & 0x03;  // 0-3
    uint8_t q = idx_q & 0x03;  // 0-3

    // Write through pointer p
    arr[p] = val_p;

    // Write through pointer q
    arr[q] = val_q;

    // Read back through p — if p == q, we get val_q (aliasing!)
    int read_p = arr[p];

    // Check if aliasing occurred
    if (p == q) {
        // Aliased: read_p should be val_q, not val_p
        if (read_p == val_q) {
            result = alias_hit(result);
        } else {
            result = zero_result(result); // shouldn't happen
        }
    } else {
        // Not aliased: read_p should be val_p
        if (read_p == val_p) {
            result = no_alias(result);
        } else {
            result = zero_result(result); // shouldn't happen
        }
    }

    // Double-write test: write to both then check
    if (p != q && val_p == val_q) {
        result = double_write(result); // same value in different cells
    }

    return result;
}
