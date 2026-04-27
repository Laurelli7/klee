// exp62: "Asymmetric Cost Tree" — A binary tree where the LEFT child
// is always CHEAP (one branch) and the RIGHT child is always EXPENSIVE
// (4 branches from bit tests). Both sides have unique coverage.
//
// After 8 levels of this asymmetric tree:
//   - Left-only path: depth 8, 8 unique functions, 0 extra branches
//   - Right-only path: depth 8, 8 unique functions, 32 extra branches
//   - Mixed paths: somewhere in between
//
// NURS:qc should prefer LEFT (cheaper solver queries).
// NURS:covnew should explore both equally (both have new coverage).
// DFS picks left or right at each level, committing to one subtree.
// BFS tries both at each level, exploding on the right side.
//
// This should be the FIRST experiment to split qc from covnew.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int left_0(int x) { return x + 10; }
__attribute__((noinline)) int left_1(int x) { return x + 20; }
__attribute__((noinline)) int left_2(int x) { return x + 30; }
__attribute__((noinline)) int left_3(int x) { return x + 40; }
__attribute__((noinline)) int left_4(int x) { return x + 50; }
__attribute__((noinline)) int left_5(int x) { return x + 60; }
__attribute__((noinline)) int left_6(int x) { return x + 70; }
__attribute__((noinline)) int left_7(int x) { return x + 80; }

__attribute__((noinline)) int right_0(int x, uint8_t n) {
    if (n & 0x01) x += 100; if (n & 0x02) x += 101;
    if (n & 0x04) x += 102; if (n & 0x08) x += 103;
    return x;
}
__attribute__((noinline)) int right_1(int x, uint8_t n) {
    if (n & 0x01) x += 200; if (n & 0x02) x += 201;
    if (n & 0x04) x += 202; if (n & 0x08) x += 203;
    return x;
}
__attribute__((noinline)) int right_2(int x, uint8_t n) {
    if (n & 0x01) x += 300; if (n & 0x02) x += 301;
    if (n & 0x04) x += 302; if (n & 0x08) x += 303;
    return x;
}
__attribute__((noinline)) int right_3(int x, uint8_t n) {
    if (n & 0x01) x += 400; if (n & 0x02) x += 401;
    if (n & 0x04) x += 402; if (n & 0x08) x += 403;
    return x;
}
__attribute__((noinline)) int right_4(int x, uint8_t n) {
    if (n & 0x01) x += 500; if (n & 0x02) x += 501;
    if (n & 0x04) x += 502; if (n & 0x08) x += 503;
    return x;
}
__attribute__((noinline)) int right_5(int x, uint8_t n) {
    if (n & 0x01) x += 600; if (n & 0x02) x += 601;
    if (n & 0x04) x += 602; if (n & 0x08) x += 603;
    return x;
}
__attribute__((noinline)) int right_6(int x, uint8_t n) {
    if (n & 0x01) x += 700; if (n & 0x02) x += 701;
    if (n & 0x04) x += 702; if (n & 0x08) x += 703;
    return x;
}
__attribute__((noinline)) int right_7(int x, uint8_t n) {
    if (n & 0x01) x += 800; if (n & 0x02) x += 801;
    if (n & 0x04) x += 802; if (n & 0x08) x += 803;
    return x;
}

int main() {
    uint8_t choices;       // 8 bits = 8 left/right decisions
    uint8_t noise[8];      // noise bytes for the right-side branches
    klee_make_symbolic(&choices, sizeof(choices), "choices");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int result = 0;

    // Level 0
    if (choices & 0x01) result = left_0(result);
    else result = right_0(result, noise[0]);

    // Level 1
    if (choices & 0x02) result = left_1(result);
    else result = right_1(result, noise[1]);

    // Level 2
    if (choices & 0x04) result = left_2(result);
    else result = right_2(result, noise[2]);

    // Level 3
    if (choices & 0x08) result = left_3(result);
    else result = right_3(result, noise[3]);

    // Level 4
    if (choices & 0x10) result = left_4(result);
    else result = right_4(result, noise[4]);

    // Level 5
    if (choices & 0x20) result = left_5(result);
    else result = right_5(result, noise[5]);

    // Level 6
    if (choices & 0x40) result = left_6(result);
    else result = right_6(result, noise[6]);

    // Level 7
    if (choices & 0x80) result = left_7(result);
    else result = right_7(result, noise[7]);

    return result;
}
