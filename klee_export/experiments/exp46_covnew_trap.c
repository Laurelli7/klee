// exp46: "Coverage-New Trap" — A program where covnew HURTS.
// The program sprinkles cheap-to-reach but meaningless "new coverage"
// breadcrumbs that lure covnew away from the real target.
//
// Structure: 8 stages. Each stage has:
//   - A breadcrumb: a unique function easily reached that provides
//     "new coverage" but leads to a dead end
//   - A continuation: only reachable by IGNORING the breadcrumb
//     and continuing deeper
//
// The REAL valuable code (a bug) is at the end of stage 8.
// NURS:covnew chases each breadcrumb and resets → never reaches stage 8.
// DFS plows straight through to the bug.
//
// Expected: DFS >>> NURS:covnew
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int breadcrumb_0(int x) { return x + 10; }
__attribute__((noinline)) int breadcrumb_1(int x) { return x + 20; }
__attribute__((noinline)) int breadcrumb_2(int x) { return x + 30; }
__attribute__((noinline)) int breadcrumb_3(int x) { return x + 40; }
__attribute__((noinline)) int breadcrumb_4(int x) { return x + 50; }
__attribute__((noinline)) int breadcrumb_5(int x) { return x + 60; }
__attribute__((noinline)) int breadcrumb_6(int x) { return x + 70; }
__attribute__((noinline)) int breadcrumb_7(int x) { return x + 80; }
__attribute__((noinline)) int final_treasure(int x) { return x + 9999; }

int main() {
    uint8_t path[8];
    klee_make_symbolic(path, sizeof(path), "path");

    int r = 0;

    // Stage 0: breadcrumb or continue
    if (path[0] < 128) { return breadcrumb_0(r); }
    r += 1;

    // Stage 1
    if (path[1] < 128) { return breadcrumb_1(r); }
    r += 2;

    // Stage 2
    if (path[2] < 128) { return breadcrumb_2(r); }
    r += 3;

    // Stage 3
    if (path[3] < 128) { return breadcrumb_3(r); }
    r += 4;

    // Stage 4
    if (path[4] < 128) { return breadcrumb_4(r); }
    r += 5;

    // Stage 5
    if (path[5] < 128) { return breadcrumb_5(r); }
    r += 6;

    // Stage 6
    if (path[6] < 128) { return breadcrumb_6(r); }
    r += 7;

    // Stage 7
    if (path[7] < 128) { return breadcrumb_7(r); }

    // TREASURE: Only DFS-like strategies reach here quickly
    // because they plow through all 8 false branches
    return final_treasure(r);
}
