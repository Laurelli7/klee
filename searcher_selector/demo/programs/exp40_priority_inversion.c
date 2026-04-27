// exp40: "Priority Inversion" — NURS:covnew should OBVIOUSLY beat all others.
// The program has 10 tiny "islands" of unique code, each behind a different
// byte equality check. Between the islands is a massive sea of redundant
// bitfield branches. Coverage-guided searchers should leap from island to
// island while DFS/BFS drown in the bitfield sea.
//
// Expected: NURS:covnew >>> DFS/BFS/random-path
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int island_0(int x) { return x + 100; }
__attribute__((noinline)) int island_1(int x) { return x + 200; }
__attribute__((noinline)) int island_2(int x) { return x + 300; }
__attribute__((noinline)) int island_3(int x) { return x + 400; }
__attribute__((noinline)) int island_4(int x) { return x + 500; }
__attribute__((noinline)) int island_5(int x) { return x + 600; }
__attribute__((noinline)) int island_6(int x) { return x + 700; }
__attribute__((noinline)) int island_7(int x) { return x + 800; }
__attribute__((noinline)) int island_8(int x) { return x + 900; }
__attribute__((noinline)) int island_9(int x) { return x + 1000; }

int main() {
    uint8_t key;
    uint8_t sea[3]; // 3 bytes × 8 bits = 24 branches of noise per island
    klee_make_symbolic(&key, sizeof(key), "key");
    klee_make_symbolic(sea, sizeof(sea), "sea");

    int result = 0;

    // Sea of noise (24 independent branches = 2^24 paths)
    for (int i = 0; i < 3; i++) {
        if (sea[i] & 0x01) result++;
        if (sea[i] & 0x02) result++;
        if (sea[i] & 0x04) result++;
        if (sea[i] & 0x08) result++;
        if (sea[i] & 0x10) result++;
        if (sea[i] & 0x20) result++;
        if (sea[i] & 0x40) result++;
        if (sea[i] & 0x80) result++;
    }

    // 10 islands of unique code, each behind a specific key value
    if      (key == 0x00) result = island_0(result);
    else if (key == 0x11) result = island_1(result);
    else if (key == 0x22) result = island_2(result);
    else if (key == 0x33) result = island_3(result);
    else if (key == 0x44) result = island_4(result);
    else if (key == 0x55) result = island_5(result);
    else if (key == 0x66) result = island_6(result);
    else if (key == 0x77) result = island_7(result);
    else if (key == 0x88) result = island_8(result);
    else if (key == 0x99) result = island_9(result);

    return result;
}
