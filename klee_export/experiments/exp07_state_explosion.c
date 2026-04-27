// exp07: State explosion with interleaved symbolic bytes.
// 5 symbolic bytes, each tested against 4 thresholds = 20 branches.
// Combined with inter-byte comparisons to create deep constraint chains.
#include "klee/klee.h"
int main() {
    unsigned char a[5];
    klee_make_symbolic(a, sizeof(a), "a");

    int score = 0;
    // 5 bytes × 4 thresholds = creates up to 2^20 = 1M paths
    for (int i = 0; i < 5; i++) {
        if (a[i] > 50)  score |= (1 << (i*4));
        if (a[i] > 100) score |= (1 << (i*4+1));
        if (a[i] > 150) score |= (1 << (i*4+2));
        if (a[i] > 200) score |= (1 << (i*4+3));
    }

    // Bug: requires a very specific scoring pattern
    if (score == 0b10101010101010101010) {
        return -1; // BUG
    }
    return score & 0xFF;
}
