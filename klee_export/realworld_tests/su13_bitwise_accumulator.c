/*
 * Structural Unit 13: Bitwise Accumulator Loop (Coverage-Blind)
 * Source: tiffinfo Fax decoders (bitwise_accumulator_loop, 744 BBs),
 *         bison bitset operations, readelf adler32_z,
 *         lua luaV_execute (crypto_pattern), 8 programs total
 * CFG: Loop iterates over symbolic bytes. Each iteration performs bitwise
 *      accumulation (XOR, shift, mask). Same instructions every iteration,
 *      but different bit patterns in the accumulator. Branches at end on
 *      accumulated value.
 * Hypothesis: Pure coverage-blind loop → R9 applies. All searchers see
 *   same coverage after iteration 1. DFS should dominate.
 * Prediction: DFS (R9: coverage-blind bitwise computation)
 */
#include <klee/klee.h>

unsigned int hash_step(unsigned int state, unsigned char byte) {
    state ^= byte;
    state = (state << 5) | (state >> 27);
    state ^= (state >> 16);
    state += byte * 0x5BD1E995;
    state ^= (state >> 13);
    return state;
}

/* CRC-like computation */
unsigned int crc_step(unsigned int crc, unsigned char byte) {
    crc ^= byte;
    for (int i = 0; i < 8; i++) {
        if (crc & 1)
            crc = (crc >> 1) ^ 0xEDB88320;
        else
            crc = crc >> 1;
    }
    return crc;
}

int main() {
    unsigned char data[8];
    klee_make_symbolic(data, sizeof(data), "data");

    unsigned int h = 0x811C9DC5;  /* FNV offset basis */
    unsigned int c = 0xFFFFFFFF;

    for (int i = 0; i < 8; i++) {
        h = hash_step(h, data[i]);
        c = crc_step(c, data[i]);
    }
    c ^= 0xFFFFFFFF;

    int result = 0;
    /* Final check on accumulated values */
    if ((h & 0xFF) == 0x42) result |= 1;
    if ((h & 0xFF00) == 0x1300) result |= 2;
    if ((c & 0xFF) == 0xAA) result |= 4;
    if ((c & 0xFF00) == 0xBB00) result |= 8;
    if (h == c) result |= 16;
    if ((h ^ c) == 0xDEADBEEF) result |= 32;

    return result;
}
