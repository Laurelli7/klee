/*
 * Structural Unit 28: Mostly-Sequential (Long Chain)
 * Source: mostly_sequential (40 functions across gnumake, nasm, readelf)
 *         Long sequential chains with minimal branching. E.g., readelf
 *         output formatting, gnumake variable expansion steps, nasm
 *         listing generation.
 * CFG: Very long basic block chain with only occasional branches.
 *      Most of the path is forced — no choice points.
 *      Different from range_check_heavy (SU23) — here branches are rare.
 * Hypothesis: With few branches, all searchers should perform similarly.
 *   If anything, DFS should trivially reach the end fastest.
 * Prediction: DFS ≈ all others (minimal branching means searcher doesn't matter much)
 */
#include <klee/klee.h>

int main() {
    unsigned char data[6];
    klee_make_symbolic(data, sizeof(data), "data");

    /* Long sequential computation with rare branches */
    int a = data[0];
    int b = data[1];
    int c = data[2];
    int d = data[3];
    int e = data[4];
    int f = data[5];

    /* Step 1-5: Sequential arithmetic */
    int r1 = a + b;
    int r2 = r1 * 3;
    int r3 = r2 - c;
    int r4 = r3 ^ d;
    int r5 = r4 + (e << 2);

    /* Step 6: Rare branch point */
    if (r5 > 2000) r5 = 2000;

    /* Step 7-12: More sequential */
    int r6 = r5 | f;
    int r7 = r6 & 0xFFFF;
    int r8 = r7 + a + c;
    int r9 = r8 ^ (b + d);
    int r10 = r9 - (e * f);
    int r11 = r10 + (r1 ^ r3);

    /* Step 13: Another rare branch */
    if (r11 < -5000) r11 = -5000;

    /* Step 14-20: Even more sequential */
    int r12 = r11 * 2 + r5;
    int r13 = r12 ^ r7;
    int r14 = r13 + (a * b);
    int r15 = r14 - (c * d);
    int r16 = r15 | (e + f);
    int r17 = r16 & 0x7FFF;
    int r18 = r17 + r11;

    /* Step 21: Only meaningful branch */
    if (r18 > 10000)
        return 1;
    else if (r18 < -3000)
        return 2;
    else if (r18 == 42)
        return 3;
    else
        return 0;
}
