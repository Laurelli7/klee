// exp70: "Multi-Objective Conflict" — The program has THREE desirable
// outcomes (unique functions), each optimized for a DIFFERENT searcher:
//
//   Target A: Behind a deep sequential chain → DFS optimal
//   Target B: Behind a wide independent fan → BFS optimal  
//   Target C: Behind a coverage-novel path → covnew optimal
//
// After a shared preamble of noise branches, the program splits 3 ways.
// Each target has enough states to consume the full time budget.
// No single searcher can reach all three targets in time.
//
// The multi-objective nature means the BEST searcher depends on which
// target you care about. This creates genuine differentiation.
//
// Expected: DFS gets Target A, BFS gets Target B (partially),
// covnew gets Target C, random-path/default get a mix.
#include "klee/klee.h"
#include <stdint.h>

// Targets: each is unique code
__attribute__((noinline)) int target_A(int x) { return x + 11111; }
__attribute__((noinline)) int target_B(int x) { return x + 22222; }
__attribute__((noinline)) int target_C(int x) { return x + 33333; }

// Shared noise handlers
__attribute__((noinline)) int preamble_fn(int x) { return x + 1; }

// Target C breadcrumbs: unique functions that guide covnew
__attribute__((noinline)) int breadcrumb_1(int x) { return x + 1000; }
__attribute__((noinline)) int breadcrumb_2(int x) { return x + 2000; }
__attribute__((noinline)) int breadcrumb_3(int x) { return x + 3000; }
__attribute__((noinline)) int breadcrumb_4(int x) { return x + 4000; }

int main() {
    uint8_t route;
    uint8_t noise[2];
    uint8_t chain_key;
    uint8_t fan[3];
    uint8_t trail;
    klee_make_symbolic(&route, sizeof(route), "route");
    klee_make_symbolic(noise, sizeof(noise), "noise");
    klee_make_symbolic(&chain_key, sizeof(chain_key), "chain");
    klee_make_symbolic(fan, sizeof(fan), "fan");
    klee_make_symbolic(&trail, sizeof(trail), "trail");

    int result = 0;

    // Shared preamble: 16 noise branches (creates 2^16 = 65536 states)
    for (int i = 0; i < 2; i++) {
        if (noise[i] & 0x01) result = preamble_fn(result);
        if (noise[i] & 0x02) result = preamble_fn(result);
        if (noise[i] & 0x04) result = preamble_fn(result);
        if (noise[i] & 0x08) result = preamble_fn(result);
        if (noise[i] & 0x10) result = preamble_fn(result);
        if (noise[i] & 0x20) result = preamble_fn(result);
        if (noise[i] & 0x40) result = preamble_fn(result);
        if (noise[i] & 0x80) result = preamble_fn(result);
    }

    // Route selection: 3 paths
    uint8_t r = route % 3;

    if (r == 0) {
        // Path A: DEEP sequential chain (8 sequential equality gates)
        if ((chain_key & 0x03) != 0x02) return result;
        result += 100;
        if (((chain_key >> 2) & 0x03) != 0x01) return result;
        result += 200;
        if (((chain_key >> 4) & 0x03) != 0x03) return result;
        result += 300;
        if (((chain_key >> 6) & 0x03) != 0x00) return result;
        result = target_A(result);

    } else if (r == 1) {
        // Path B: WIDE independent fan (3 bytes × 8 bits = 24 branches)
        for (int i = 0; i < 3; i++) {
            if (fan[i] & 0x01) result++;
            if (fan[i] & 0x02) result++;
            if (fan[i] & 0x04) result++;
            if (fan[i] & 0x08) result++;
            if (fan[i] & 0x10) result++;
            if (fan[i] & 0x20) result++;
            if (fan[i] & 0x40) result++;
            if (fan[i] & 0x80) result++;
        }
        result = target_B(result);

    } else {
        // Path C: Coverage-novel trail (4 unique breadcrumb functions)
        result = breadcrumb_1(result);
        if (trail > 64) {
            result = breadcrumb_2(result);
            if (trail > 128) {
                result = breadcrumb_3(result);
                if (trail > 192) {
                    result = breadcrumb_4(result);
                    result = target_C(result);
                }
            }
        }
    }

    return result;
}
