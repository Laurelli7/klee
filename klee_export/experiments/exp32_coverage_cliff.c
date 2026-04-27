// exp32: "Coverage cliff" — Program has two halves. First half is
// cheap to cover (5 branches). Second half is expensive (20 branches).
// But the second half is ONLY REACHABLE if first half returns a
// specific value. Tests whether searchers recognize the gate.
#include "klee/klee.h"
#include <stdint.h>

int main() {
    uint8_t key;
    uint8_t phase2[4];
    klee_make_symbolic(&key, sizeof(key), "key");
    klee_make_symbolic(phase2, sizeof(phase2), "phase2");

    // Phase 1: Simple gate
    int gate = 0;
    if (key & 0x01) gate++;
    if (key & 0x02) gate++;
    if (key & 0x04) gate++;
    if (key & 0x08) gate++;
    if (key & 0x10) gate++;

    if (gate != 3) return gate; // Most paths exit here

    // Phase 2: Only when exactly 3 bits set in key
    // 4 bytes × 5 bits = 20 branches
    int result = 0;
    for (int i = 0; i < 4; i++) {
        if (phase2[i] & 0x01) result++;
        if (phase2[i] & 0x02) result++;
        if (phase2[i] & 0x04) result++;
        if (phase2[i] & 0x08) result++;
        if (phase2[i] & 0x10) result++;
    }

    if (result == 20) return -1; // BUG at max score
    return 100 + result;
}
