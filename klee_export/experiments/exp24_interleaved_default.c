// exp24: "Default searcher test" — Run KLEE's default interleaved
// searcher (random-path + nurs:covnew) against individuals.
// The interleaved searcher should hedge between strategies.
// Use the most challenging program (exp21 interleaved_hot_cold).
// This test just runs with --search=random-path:nurs:covnew explicitly.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int fn_alpha(int x)   { return x * 2 + 10; }
__attribute__((noinline)) int fn_beta(int x)    { return x * 3 + 20; }
__attribute__((noinline)) int fn_gamma(int x)   { return x * 5 + 30; }
__attribute__((noinline)) int fn_delta(int x)   { return x * 7 + 40; }
__attribute__((noinline)) int fn_epsilon(int x) { return x * 11 + 50; }

int main() {
    uint8_t key[5];
    uint8_t noise[5];
    klee_make_symbolic(key, sizeof(key), "key");
    klee_make_symbolic(noise, sizeof(noise), "noise");

    int r = 0;

    // 5 rounds: hot zone (5 bits) → cold gate → repeat
    for (int round = 0; round < 5; round++) {
        // Hot zone: 5 bit-tests per round
        if (noise[round] & 0x01) r++;
        if (noise[round] & 0x02) r++;
        if (noise[round] & 0x04) r++;
        if (noise[round] & 0x08) r++;
        if (noise[round] & 0x10) r++;

        // Cold gate: unique function per round
        switch (round) {
            case 0: if (key[0] == 'A') r = fn_alpha(r); break;
            case 1: if (key[1] == 'B') r = fn_beta(r); break;
            case 2: if (key[2] == 'C') r = fn_gamma(r); break;
            case 3: if (key[3] == 'D') r = fn_delta(r); break;
            case 4: if (key[4] == 'E') r = fn_epsilon(r); break;
        }
    }
    return r & 0xFF;
}
