/*
 * Structural Unit 27: Tail-Call Chain
 * Source: tail_call_chain (74 functions, flvmeta ONLY)
 *         Functions that end with a call to another function (tail position).
 *         AMF type parsers chain: parse_amf_string → parse_amf_value → parse_amf_object.
 *         This creates a linear chain of function calls without stack growth.
 * CFG: Each function has branching + a tail call. The chain of tail calls
 *      creates a deep linear path through multiple function bodies.
 *      Different from complex_recursive (SU18) — no recursion, just chaining.
 * Hypothesis: Tail-call chains create depth like DFS-friendly code.
 *   But each function has independent branches → coverage can guide sideways.
 * Prediction: DFS (linear depth via chaining, R1-like sequential structure)
 */
#include <klee/klee.h>

/* Forward declarations for tail-call chain */
static int phase_e(unsigned char *data, int offset, int accum);
static int phase_d(unsigned char *data, int offset, int accum);
static int phase_c(unsigned char *data, int offset, int accum);
static int phase_b(unsigned char *data, int offset, int accum);
static int phase_a(unsigned char *data, int offset, int accum);

static int phase_a(unsigned char *data, int offset, int accum) {
    unsigned char v = data[offset];
    if (v < 50) accum += v;
    else if (v < 100) accum -= v;
    else if (v < 150) accum ^= v;
    else accum += v * 2;
    /* tail call to phase_b */
    return phase_b(data, offset + 1, accum);
}

static int phase_b(unsigned char *data, int offset, int accum) {
    unsigned char v = data[offset];
    if (v & 0x80) {
        accum = accum | (v << 8);
    } else {
        accum = accum & ~(v << 4);
    }
    if (accum > 10000) accum = 10000;
    if (accum < -10000) accum = -10000;
    /* tail call to phase_c */
    return phase_c(data, offset + 1, accum);
}

static int phase_c(unsigned char *data, int offset, int accum) {
    unsigned char v = data[offset];
    switch (v & 0x03) {
        case 0: accum += 10; break;
        case 1: accum -= 10; break;
        case 2: accum *= 2; break;
        case 3: accum /= 2; break;
    }
    if (v & 0x04) accum = -accum;
    /* tail call to phase_d */
    return phase_d(data, offset + 1, accum);
}

static int phase_d(unsigned char *data, int offset, int accum) {
    unsigned char v = data[offset];
    unsigned char w = data[offset + 1];
    int combined = v * 256 + w;
    if (combined > accum) accum = combined - accum;
    else accum = accum - combined;
    /* tail call to phase_e */
    return phase_e(data, offset + 2, accum);
}

static int phase_e(unsigned char *data, int offset, int accum) {
    unsigned char v = data[offset];
    if (v == 0x42 && accum > 100) return 99;  /* magic result */
    if (v == 0x00) return accum & 0xFF;
    if (v > 200) return -(accum & 0xFF);
    return accum;
}

int main() {
    unsigned char data[8];
    klee_make_symbolic(data, sizeof(data), "data");
    return phase_a(data, 0, 0);
}
