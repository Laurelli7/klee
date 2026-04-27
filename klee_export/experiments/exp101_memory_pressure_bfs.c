// exp101: "Memory Pressure BFS" — BFS's state queue grows linearly with
// breadth. This test has WIDE but SHALLOW branching where BFS thrives
// because total state count stays manageable.
//
// Key difference from exp91: NO deep tail at all. Just many independent
// conditions each calling unique functions. The question is whether BFS
// covers them all before NURS variants, which must randomly select among
// the states.
//
// 32 independent if-statements on different bits of 4 symbolic bytes.
// Each true branch calls a unique function. Each false branch is trivial.
// No state explosion — exactly 2^32 paths but each path is very short.
// Under a 10s timeout, all searchers hit the state explosion,
// but BFS explores all 32 "true" branches at depth 1 before going deeper.
//
// Expected: BFS covers all 32 unique functions first.
#include "klee/klee.h"
#include <stdint.h>

// 32 unique functions
__attribute__((noinline)) int u00(int x) { return x + 1; }
__attribute__((noinline)) int u01(int x) { return x + 2; }
__attribute__((noinline)) int u02(int x) { return x + 3; }
__attribute__((noinline)) int u03(int x) { return x + 4; }
__attribute__((noinline)) int u04(int x) { return x + 5; }
__attribute__((noinline)) int u05(int x) { return x + 6; }
__attribute__((noinline)) int u06(int x) { return x + 7; }
__attribute__((noinline)) int u07(int x) { return x + 8; }
__attribute__((noinline)) int u08(int x) { return x + 9; }
__attribute__((noinline)) int u09(int x) { return x + 10; }
__attribute__((noinline)) int u10(int x) { return x + 11; }
__attribute__((noinline)) int u11(int x) { return x + 12; }
__attribute__((noinline)) int u12(int x) { return x + 13; }
__attribute__((noinline)) int u13(int x) { return x + 14; }
__attribute__((noinline)) int u14(int x) { return x + 15; }
__attribute__((noinline)) int u15(int x) { return x + 16; }
__attribute__((noinline)) int u16(int x) { return x + 17; }
__attribute__((noinline)) int u17(int x) { return x + 18; }
__attribute__((noinline)) int u18(int x) { return x + 19; }
__attribute__((noinline)) int u19(int x) { return x + 20; }
__attribute__((noinline)) int u20(int x) { return x + 21; }
__attribute__((noinline)) int u21(int x) { return x + 22; }
__attribute__((noinline)) int u22(int x) { return x + 23; }
__attribute__((noinline)) int u23(int x) { return x + 24; }
__attribute__((noinline)) int u24(int x) { return x + 25; }
__attribute__((noinline)) int u25(int x) { return x + 26; }
__attribute__((noinline)) int u26(int x) { return x + 27; }
__attribute__((noinline)) int u27(int x) { return x + 28; }
__attribute__((noinline)) int u28(int x) { return x + 29; }
__attribute__((noinline)) int u29(int x) { return x + 30; }
__attribute__((noinline)) int u30(int x) { return x + 31; }
__attribute__((noinline)) int u31(int x) { return x + 32; }

int main() {
    uint8_t data[4];
    klee_make_symbolic(data, sizeof(data), "data");

    int r = 0;

    // 32 independent bit checks — each "true" path has unique code
    // "false" paths all share the same trivial instruction
    if (data[0] & 0x01) r = u00(r); else r++;
    if (data[0] & 0x02) r = u01(r); else r++;
    if (data[0] & 0x04) r = u02(r); else r++;
    if (data[0] & 0x08) r = u03(r); else r++;
    if (data[0] & 0x10) r = u04(r); else r++;
    if (data[0] & 0x20) r = u05(r); else r++;
    if (data[0] & 0x40) r = u06(r); else r++;
    if (data[0] & 0x80) r = u07(r); else r++;

    if (data[1] & 0x01) r = u08(r); else r++;
    if (data[1] & 0x02) r = u09(r); else r++;
    if (data[1] & 0x04) r = u10(r); else r++;
    if (data[1] & 0x08) r = u11(r); else r++;
    if (data[1] & 0x10) r = u12(r); else r++;
    if (data[1] & 0x20) r = u13(r); else r++;
    if (data[1] & 0x40) r = u14(r); else r++;
    if (data[1] & 0x80) r = u15(r); else r++;

    if (data[2] & 0x01) r = u16(r); else r++;
    if (data[2] & 0x02) r = u17(r); else r++;
    if (data[2] & 0x04) r = u18(r); else r++;
    if (data[2] & 0x08) r = u19(r); else r++;
    if (data[2] & 0x10) r = u20(r); else r++;
    if (data[2] & 0x20) r = u21(r); else r++;
    if (data[2] & 0x40) r = u22(r); else r++;
    if (data[2] & 0x80) r = u23(r); else r++;

    if (data[3] & 0x01) r = u24(r); else r++;
    if (data[3] & 0x02) r = u25(r); else r++;
    if (data[3] & 0x04) r = u26(r); else r++;
    if (data[3] & 0x08) r = u27(r); else r++;
    if (data[3] & 0x10) r = u28(r); else r++;
    if (data[3] & 0x20) r = u29(r); else r++;
    if (data[3] & 0x40) r = u30(r); else r++;
    if (data[3] & 0x80) r = u31(r); else r++;

    return r;
}
