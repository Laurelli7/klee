/*
 * Structural Unit 22: Multi-Loop (Same Nesting Level)
 * Source: multi_loop (224 functions across ALL 8 programs)
 *         E.g., readelf processes sections loop then relocations loop then symbols loop.
 *         Bison: first pass loops + second pass loops in same function.
 * CFG: Multiple sequential loops at same nesting level.
 *      Each loop is independent — completing one loop is required before
 *      entering the next. Creates a "staircase" pattern in the CFG.
 * Hypothesis: DFS will explore the first loop exhaustively before moving on.
 *   covnew should be attracted to the second/third loop sooner.
 * Prediction: nurs:covnew (coverage gradient pulls toward unexplored later loops)
 */
#include <klee/klee.h>

int main() {
    unsigned char data[12];
    klee_make_symbolic(data, sizeof(data), "data");

    int pass1_result = 0;
    int pass2_result = 0;
    int pass3_result = 0;

    /* Pass 1: Scan and classify — 4 iterations */
    for (int i = 0; i < 4; i++) {
        unsigned char v = data[i];
        if (v < 64) pass1_result += 1;
        else if (v < 128) pass1_result += 2;
        else if (v < 192) pass1_result += 4;
        else pass1_result += 8;
    }

    /* Pass 2: Process based on pass 1 result — 4 iterations */
    for (int i = 4; i < 8; i++) {
        unsigned char v = data[i];
        if (pass1_result & 0x01) {
            v = v ^ 0xAA;
        }
        if (pass1_result & 0x02) {
            v = (v >> 4) | (v << 4);
        }
        if (pass1_result & 0x04) {
            v = v + (unsigned char)pass1_result;
        }
        if (pass1_result & 0x08) {
            v = ~v;
        }
        pass2_result += v;
    }

    /* Pass 3: Validate — 4 iterations, depends on pass 2 */
    for (int i = 8; i < 12; i++) {
        unsigned char v = data[i];
        unsigned char threshold = (unsigned char)(pass2_result & 0xFF);
        if (v > threshold) {
            pass3_result += 1;
        } else if (v == threshold) {
            pass3_result += 10;
        } else {
            pass3_result -= 1;
        }
    }

    /* Final classification based on all three passes */
    if (pass3_result > 20)
        return 1;
    else if (pass3_result > 0 && pass1_result > 10)
        return 2;
    else if (pass3_result < -2 && pass2_result > 500)
        return 3;
    else
        return 0;
}
