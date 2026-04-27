// exp89: "Hash Inversion" — Input goes through a simple hash function,
// then the HASH VALUE determines which code path executes.
//
// The hash is simple enough that Z3 can solve it, but complex enough
// to create significant per-query cost. Each target hash value
// requires inverting the hash, which is a non-trivial constraint.
//
// The key insight: there are 8 "buckets" of code, each behind a
// different hash value. Reaching all 8 requires 8 separate hash
// inversions. The solver cost per bucket is roughly constant,
// but searchers differ in HOW they schedule these inversions.
//
// covnew: should be best — each bucket is novel, and it will
//         prioritize states heading toward uncovered buckets
// DFS: will find ONE bucket deeply, then slowly backtrack
// qc: all inversions cost the same, so no cost gradient to exploit
// bfs: will try all inversions simultaneously (wasteful)
//
// Expected: covnew > md2u > qc > DFS for bucket coverage.
#include "klee/klee.h"
#include <stdint.h>

// Simple hash — invertible but non-trivial
__attribute__((noinline)) uint8_t simple_hash(uint8_t a, uint8_t b) {
    uint8_t h = a;
    h = h ^ (h << 3);
    h = h + b;
    h = h ^ (h >> 2);
    h = h + (a & b);
    h = h ^ (h << 1);
    return h;
}

__attribute__((noinline)) int bucket_0(int x) { return x + 0x100; }
__attribute__((noinline)) int bucket_1(int x) { return x + 0x200; }
__attribute__((noinline)) int bucket_2(int x) { return x + 0x300; }
__attribute__((noinline)) int bucket_3(int x) { return x + 0x400; }
__attribute__((noinline)) int bucket_4(int x) { return x + 0x500; }
__attribute__((noinline)) int bucket_5(int x) { return x + 0x600; }
__attribute__((noinline)) int bucket_6(int x) { return x + 0x700; }
__attribute__((noinline)) int bucket_7(int x) { return x + 0x800; }

int main() {
    uint8_t input[2];    // hash input
    uint8_t noise;       // state amplifier
    klee_make_symbolic(input, sizeof(input), "input");
    klee_make_symbolic(&noise, sizeof(noise), "noise");

    int result = 0;

    // Noise
    if (noise & 0x01) result++;
    if (noise & 0x02) result++;
    if (noise & 0x04) result++;
    if (noise & 0x08) result++;
    if (noise & 0x10) result++;
    if (noise & 0x20) result++;
    if (noise & 0x40) result++;
    if (noise & 0x80) result++;

    // Hash the input
    uint8_t h = simple_hash(input[0], input[1]);

    // Dispatch on hash value — each requires solving h == target
    uint8_t bucket = h & 0x07;  // 8 buckets
    switch (bucket) {
        case 0: result = bucket_0(result); break;
        case 1: result = bucket_1(result); break;
        case 2: result = bucket_2(result); break;
        case 3: result = bucket_3(result); break;
        case 4: result = bucket_4(result); break;
        case 5: result = bucket_5(result); break;
        case 6: result = bucket_6(result); break;
        case 7: result = bucket_7(result); break;
    }

    // Second hash dispatch — doubles the inversion cost
    uint8_t h2 = simple_hash(h, input[0] ^ input[1]);
    if ((h2 & 0x03) == 0) {
        result += 0x1000;  // rare: requires double inversion
    }

    // Triple prize: specific hash chain
    if (h == 0x42 && h2 == 0x24) {
        return 99999;
    }

    return result;
}
