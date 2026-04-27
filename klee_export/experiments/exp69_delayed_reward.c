// exp69: "Delayed Reward" — Coverage is ONLY available after passing
// through a long gauntlet of branches that all cover the SAME code.
// The reward (unique functions) is at the END, N steps away.
//
// This specifically tests md2u: the unique code is at a MEASURABLE
// distance, and md2u should prioritize states closer to it.
// covnew: no coverage difference between steps 1 and N-1, so it
// should be INDIFFERENT (random among gauntlet states).
//
// Structure: 12 gauntlet steps (same function), each with a
// symbolic fork. At the end, 4 unique reward functions.
// Total states in gauntlet: 2^12 = 4096. Only the states that
// reach step 12 see new coverage.
//
// Expected: md2u best (tracks distance to reward).
// DFS best if it picks the right fork each time.
// BFS creates 4096 states then tries all at the next step.
// covnew no preference among gauntlet states.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int gauntlet_step(int x) { return x + 1; }

__attribute__((noinline)) int reward_alpha(int x)  { return x + 10000; }
__attribute__((noinline)) int reward_beta(int x)   { return x + 20000; }
__attribute__((noinline)) int reward_gamma(int x)  { return x + 30000; }
__attribute__((noinline)) int reward_delta(int x)  { return x + 40000; }

int main() {
    uint8_t gauntlet[2]; // 2 bytes = 16 bits, use 12
    uint8_t reward_key;
    klee_make_symbolic(gauntlet, sizeof(gauntlet), "gauntlet");
    klee_make_symbolic(&reward_key, sizeof(reward_key), "rkey");

    int result = 0;

    // 12-step gauntlet: same function, but each step forks
    // bit=1 path: continue gauntlet
    // bit=0 path: early exit (no reward)
    // This means only the states where all 12 bits are 1 reach the reward.
    for (int i = 0; i < 12; i++) {
        int byte_idx = i / 8;
        int bit_idx = i % 8;
        if (!((gauntlet[byte_idx] >> bit_idx) & 1)) {
            return result; // early exit — no reward
        }
        result = gauntlet_step(result);
    }

    // Reward zone: 4 unique functions based on reward_key
    uint8_t rk = reward_key & 0x03;
    switch (rk) {
        case 0: return reward_alpha(result);
        case 1: return reward_beta(result);
        case 2: return reward_gamma(result);
        case 3: return reward_delta(result);
    }
    return result;
}
