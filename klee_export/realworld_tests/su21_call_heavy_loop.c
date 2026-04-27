/*
 * Structural Unit 21: Call-Heavy Loop
 * Source: call_heavy_loop (228 functions across ALL 8 programs)
 *         gnumake rule expansion, lua table iteration, readelf section walks,
 *         bison closure computation, nasm macro expansion.
 * CFG: Loop body dominated by function calls (not inline computation).
 *      Each call may fork paths internally, creating "hidden" state explosion.
 *      Different from callback_dispatch (SU12) — here calls are to known
 *      functions, not through function pointers.
 * Hypothesis: Calls create sub-trees. DFS gets stuck in one call's subtree.
 *   covnew should rotate between calls to cover all call targets.
 * Prediction: nurs:covnew (coverage guidance visits each call's coverage space)
 */
#include <klee/klee.h>

/* Simulate different processing functions with internal branching */
static int process_alpha(unsigned char a, unsigned char b) {
    if (a > b) return a - b;
    if (a == b) return 0;
    return b - a + (a & 0x0F);
}

static int process_beta(unsigned char x) {
    if (x < 64) return x * 2;
    if (x < 128) return x + 10;
    if (x < 192) return x - 50;
    return x ^ 0xFF;
}

static int process_gamma(unsigned char p, unsigned char q) {
    int r = p ^ q;
    if (r & 0x80) r = ~r;
    if (r & 0x40) r >>= 1;
    if (r & 0x20) r <<= 2;
    if (r & 0x10) r += p;
    return r & 0xFF;
}

static int process_delta(unsigned char v) {
    switch (v & 0x07) {
        case 0: return v;
        case 1: return v + 1;
        case 2: return v + 2;
        case 3: return v * 2;
        case 4: return v ^ 0xAA;
        case 5: return v >> 1;
        case 6: return v & 0xF0;
        default: return v | 0x0F;
    }
}

static int process_epsilon(unsigned char a, unsigned char b) {
    if (a == 0) return b;
    if (b == 0) return a;
    return (a % b) + (b % a);
}

int main() {
    unsigned char data[10];
    klee_make_symbolic(data, sizeof(data), "data");

    int accum = 0;

    /* Call-heavy loop: body is dominated by function calls */
    for (int i = 0; i < 5; i++) {
        unsigned char a = data[i * 2];
        unsigned char b = data[i * 2 + 1];

        int r1 = process_alpha(a, b);
        int r2 = process_beta(a);
        int r3 = process_gamma(a, b);
        int r4 = process_delta(b);
        int r5 = process_epsilon(a, b);

        /* Combine results */
        accum += r1 + r2 + r3 + r4 + r5;

        /* Early exit based on call results */
        if (r1 == 0 && r3 == 0 && r5 == 0)
            return -1;
    }

    if (accum > 5000)
        return 1;
    else if (accum < -100)
        return 2;
    return 0;
}
