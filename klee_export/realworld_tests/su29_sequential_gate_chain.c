/*
 * Structural Unit 29: Sequential Gate Chain (R1 — first real-world SU test)
 * Source: sorting networks, authentication chains, protocol validators
 * CFG: 20 sequential gates, each 3-way branching. Only the pass-pass-...-pass
 *      path through all 20 gates reaches the target code at the end.
 *      Total paths: 3^20 ≈ 3.5 billion (massive state space).
 * Hypothesis: DFS commits to one path and follows it through all gates.
 *   Others fork at every gate and drown in pending states.
 * Prediction: DFS (R1: sequential gating)
 */
#include <klee/klee.h>

int main() {
    unsigned char key[20];
    klee_make_symbolic(key, sizeof(key), "key");

    int stage = 0;
    int result = 0;

    /* Gate 0: three-way classification */
    if (key[0] < 85) {
        result += 1;
        stage = 1;
    } else if (key[0] < 170) {
        result += 2;
        stage = 1;
    } else {
        result += 4;
        stage = 1;
    }

    /* Gates 1-19: each gate requires passing the previous */
    if (stage >= 1) {
        if (key[1] < 85) { result += 10; stage = 2; }
        else if (key[1] < 170) { result += 20; stage = 2; }
        else { result += 40; stage = 2; }
    }
    if (stage >= 2) {
        if (key[2] < 85) { result += 100; stage = 3; }
        else if (key[2] < 170) { result += 200; stage = 3; }
        else { result += 400; stage = 3; }
    }
    if (stage >= 3) {
        if (key[3] < 85) { result += 1000; stage = 4; }
        else if (key[3] < 170) { result += 2000; stage = 4; }
        else { result += 4000; stage = 4; }
    }
    if (stage >= 4) {
        if (key[4] < 85) { result += 10000; stage = 5; }
        else if (key[4] < 170) { result += 20000; stage = 5; }
        else { result += 40000; stage = 5; }
    }
    if (stage >= 5) {
        if (key[5] < 100) { result ^= 0x1; stage = 6; }
        else if (key[5] < 200) { result ^= 0x2; stage = 6; }
        else { result ^= 0x4; stage = 6; }
    }
    if (stage >= 6) {
        if (key[6] < 100) { result ^= 0x10; stage = 7; }
        else if (key[6] < 200) { result ^= 0x20; stage = 7; }
        else { result ^= 0x40; stage = 7; }
    }
    if (stage >= 7) {
        if (key[7] < 100) { result ^= 0x100; stage = 8; }
        else if (key[7] < 200) { result ^= 0x200; stage = 8; }
        else { result ^= 0x400; stage = 8; }
    }
    if (stage >= 8) {
        if (key[8] < 100) { result += key[8]; stage = 9; }
        else if (key[8] < 200) { result -= key[8]; stage = 9; }
        else { result *= 2; stage = 9; }
    }
    if (stage >= 9) {
        if (key[9] < 100) { result += key[9] * 3; stage = 10; }
        else if (key[9] < 200) { result -= key[9] * 2; stage = 10; }
        else { result ^= key[9]; stage = 10; }
    }
    if (stage >= 10) {
        if (key[10] < 80) { result += 111; stage = 11; }
        else if (key[10] < 160) { result += 222; stage = 11; }
        else { result += 333; stage = 11; }
    }
    if (stage >= 11) {
        if (key[11] < 80) { result ^= 0xAA; stage = 12; }
        else if (key[11] < 160) { result ^= 0xBB; stage = 12; }
        else { result ^= 0xCC; stage = 12; }
    }
    if (stage >= 12) {
        if (key[12] < 80) { result += key[12]; stage = 13; }
        else if (key[12] < 160) { result -= key[12]; stage = 13; }
        else { result ^= key[12]; stage = 13; }
    }
    if (stage >= 13) {
        if (key[13] < 80) { result += 500; stage = 14; }
        else if (key[13] < 160) { result += 600; stage = 14; }
        else { result += 700; stage = 14; }
    }
    if (stage >= 14) {
        if (key[14] < 90) { result ^= 0xFF; stage = 15; }
        else if (key[14] < 180) { result ^= 0xF0; stage = 15; }
        else { result ^= 0x0F; stage = 15; }
    }
    if (stage >= 15) {
        if (key[15] < 90) { result += key[15] + key[0]; stage = 16; }
        else if (key[15] < 180) { result += key[15] - key[0]; stage = 16; }
        else { result += key[15] ^ key[0]; stage = 16; }
    }
    if (stage >= 16) {
        if (key[16] < 90) { result += 1111; stage = 17; }
        else if (key[16] < 180) { result += 2222; stage = 17; }
        else { result += 3333; stage = 17; }
    }
    if (stage >= 17) {
        if (key[17] < 90) { result ^= 0x5555; stage = 18; }
        else if (key[17] < 180) { result ^= 0xAAAA; stage = 18; }
        else { result ^= 0x3333; stage = 18; }
    }
    if (stage >= 18) {
        if (key[18] < 90) { result += key[18]; stage = 19; }
        else if (key[18] < 180) { result -= key[18]; stage = 19; }
        else { result *= 3; stage = 19; }
    }
    if (stage >= 19) {
        if (key[19] < 90) { result += 99999; }
        else if (key[19] < 180) { result += 88888; }
        else { result += 77777; }
    }

    /* Target: only reachable after all 20 gates */
    if (result > 100000) {
        result += 42;
    } else if (result > 50000) {
        result += 24;
    } else if (result > 10000) {
        result += 12;
    } else {
        result += 6;
    }

    return result;
}
