/*
 * Structural Unit 16: Hub Node (High Fan-In + Fan-Out)
 * Source: hub_node (167 functions across all 8 programs)
 *         bc/yyparse (BBs=347, fan-in from error recovery, fan-out to actions),
 *         readelf display functions, bison/prepare
 * CFG: Central node with many incoming edges (from different callers/gotos)
 *      AND many outgoing edges (dispatch to handlers). Creates a bottleneck
 *      where many paths converge then immediately diverge.
 *      Different from SU08 (multi-level) — here it's a SINGLE node as hub.
 * Hypothesis: Hub creates convergent-divergent (R6) at one point.
 *   All states accumulate at hub, then diverge. NURS should re-evaluate
 *   coverage potential at divergence point.
 * Prediction: nurs:covnew (R6: convergent-divergent at hub)
 */
#include <klee/klee.h>

int flags;
int state;

/* Multiple entry points converge to hub */
void entry_alpha(unsigned char a, unsigned char b) {
    state = a + b;
    flags = 1;
}

void entry_beta(unsigned char a, unsigned char b) {
    state = a * 2 - b;
    flags = 2;
}

void entry_gamma(unsigned char a, unsigned char b) {
    state = (a ^ b) + 100;
    flags = 4;
}

void entry_delta(unsigned char a, unsigned char b) {
    state = (a & 0xF0) | (b & 0x0F);
    flags = 8;
}

void entry_epsilon(unsigned char a, unsigned char b) {
    state = a - b + 200;
    flags = 16;
}

/* Hub function: high fan-in (5 callers) + high fan-out (dispatch based on state) */
int hub_process(unsigned char selector) {
    /* This is the convergence point — state computed differently by each entry */
    int result = 0;

    /* Now fan-out based on flags (entry identity) AND selector */
    if (flags & 1) {
        if (selector < 50) result = state + 10;
        else if (selector < 100) result = state + 20;
        else if (selector < 150) result = state + 30;
        else result = state + 40;
    }
    if (flags & 2) {
        if (selector & 0x01) result += state * 2;
        if (selector & 0x02) result += state * 3;
        if (selector & 0x04) result -= state;
    }
    if (flags & 4) {
        result += (state ^ selector) & 0xFF;
        if (state > 150) result += 1000;
    }
    if (flags & 8) {
        if (state == selector) result += 5000;
        else if (state > selector) result += state - selector;
        else result += selector - state;
    }
    if (flags & 16) {
        result += state;
        if (selector == 0xFF) result *= 2;
        if (selector == 0x00) result = -result;
    }

    return result;
}

int main() {
    unsigned char input[8];
    klee_make_symbolic(input, sizeof(input), "input");

    /* Select entry point based on input[0] — creates 5 paths converging to hub */
    int entry_sel = input[0] % 5;

    switch (entry_sel) {
        case 0: entry_alpha(input[1], input[2]); break;
        case 1: entry_beta(input[1], input[2]); break;
        case 2: entry_gamma(input[1], input[2]); break;
        case 3: entry_delta(input[1], input[2]); break;
        case 4: entry_epsilon(input[1], input[2]); break;
    }

    /* Hub: convergence then divergence */
    int r1 = hub_process(input[3]);
    int r2 = hub_process(input[4]);

    /* Second layer of hub convergence */
    state = r1 + r2;
    flags = (input[5] & 0x1F);
    int r3 = hub_process(input[6]);

    return r1 + r2 + r3;
}
