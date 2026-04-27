// exp93: "Breadth Lottery" — N independent sub-problems, each requiring
// a DIFFERENT constraint to be solved to reach unique code. The constraints
// are simple (single byte == constant) but there are many sub-problems.
//
// DFS picks one sub-problem, solves it, then must backtrack through ALL
// the others sequentially. BFS has one state per sub-problem simultaneously
// and solves all N constraints in parallel (across scheduling rounds).
//
// Key: after each unique function, there's a BINARY TREE of choices on
// additional symbolic data. DFS gets trapped in the first sub-problem's
// tree. covnew/md2u should also get trapped because the tree states
// don't have "nearby uncovered" to guide them out.
//
// Using 32 sub-problems (5 bits of selector) with very distinctive functions.
//
// Expected: BFS explores all 32 arms at depth 1 before going deep.
#include "klee/klee.h"
#include <stdint.h>

// 32 unique handlers — use volatile to prevent optimization
__attribute__((noinline)) int h00(int x) { return x + 100; }
__attribute__((noinline)) int h01(int x) { return x + 201; }
__attribute__((noinline)) int h02(int x) { return x + 302; }
__attribute__((noinline)) int h03(int x) { return x + 403; }
__attribute__((noinline)) int h04(int x) { return x + 504; }
__attribute__((noinline)) int h05(int x) { return x + 605; }
__attribute__((noinline)) int h06(int x) { return x + 706; }
__attribute__((noinline)) int h07(int x) { return x + 807; }
__attribute__((noinline)) int h08(int x) { return x + 908; }
__attribute__((noinline)) int h09(int x) { return x + 109; }
__attribute__((noinline)) int h10(int x) { return x + 210; }
__attribute__((noinline)) int h11(int x) { return x + 311; }
__attribute__((noinline)) int h12(int x) { return x + 412; }
__attribute__((noinline)) int h13(int x) { return x + 513; }
__attribute__((noinline)) int h14(int x) { return x + 614; }
__attribute__((noinline)) int h15(int x) { return x + 715; }
__attribute__((noinline)) int h16(int x) { return x + 816; }
__attribute__((noinline)) int h17(int x) { return x + 917; }
__attribute__((noinline)) int h18(int x) { return x + 118; }
__attribute__((noinline)) int h19(int x) { return x + 219; }
__attribute__((noinline)) int h20(int x) { return x + 320; }
__attribute__((noinline)) int h21(int x) { return x + 421; }
__attribute__((noinline)) int h22(int x) { return x + 522; }
__attribute__((noinline)) int h23(int x) { return x + 623; }
__attribute__((noinline)) int h24(int x) { return x + 724; }
__attribute__((noinline)) int h25(int x) { return x + 825; }
__attribute__((noinline)) int h26(int x) { return x + 926; }
__attribute__((noinline)) int h27(int x) { return x + 127; }
__attribute__((noinline)) int h28(int x) { return x + 228; }
__attribute__((noinline)) int h29(int x) { return x + 329; }
__attribute__((noinline)) int h30(int x) { return x + 430; }
__attribute__((noinline)) int h31(int x) { return x + 531; }

// Shared deep tail — same code for all branches, no new coverage
__attribute__((noinline)) int deep_tail(int seed, uint8_t *bits, int n) {
    int acc = seed;
    for (int i = 0; i < n; i++) {
        if (bits[i] & 0x01) acc ^= 0x01;
        if (bits[i] & 0x02) acc ^= 0x02;
        if (bits[i] & 0x04) acc ^= 0x04;
        if (bits[i] & 0x08) acc ^= 0x08;
        if (bits[i] & 0x10) acc ^= 0x10;
        if (bits[i] & 0x20) acc ^= 0x20;
        if (bits[i] & 0x40) acc ^= 0x40;
        if (bits[i] & 0x80) acc ^= 0x80;
    }
    return acc;
}

int main() {
    uint8_t sel;
    uint8_t tail_bits[5]; // 5 bytes = 40 binary branches = ~1 trillion paths
    klee_make_symbolic(&sel, sizeof(sel), "sel");
    klee_make_symbolic(tail_bits, sizeof(tail_bits), "tb");

    int r = 0;

    // The switch is on ALL 8 bits of sel (256 cases → 32 groups of 8)
    // But we only use top 5 bits for the handler selection
    switch (sel >> 3) {
        case  0: r = h00(sel); break;
        case  1: r = h01(sel); break;
        case  2: r = h02(sel); break;
        case  3: r = h03(sel); break;
        case  4: r = h04(sel); break;
        case  5: r = h05(sel); break;
        case  6: r = h06(sel); break;
        case  7: r = h07(sel); break;
        case  8: r = h08(sel); break;
        case  9: r = h09(sel); break;
        case 10: r = h10(sel); break;
        case 11: r = h11(sel); break;
        case 12: r = h12(sel); break;
        case 13: r = h13(sel); break;
        case 14: r = h14(sel); break;
        case 15: r = h15(sel); break;
        case 16: r = h16(sel); break;
        case 17: r = h17(sel); break;
        case 18: r = h18(sel); break;
        case 19: r = h19(sel); break;
        case 20: r = h20(sel); break;
        case 21: r = h21(sel); break;
        case 22: r = h22(sel); break;
        case 23: r = h23(sel); break;
        case 24: r = h24(sel); break;
        case 25: r = h25(sel); break;
        case 26: r = h26(sel); break;
        case 27: r = h27(sel); break;
        case 28: r = h28(sel); break;
        case 29: r = h29(sel); break;
        case 30: r = h30(sel); break;
        case 31: r = h31(sel); break;
    }

    // ALL branches share deep_tail — no new coverage
    r = deep_tail(r, tail_bits, 5);
    return r;
}
