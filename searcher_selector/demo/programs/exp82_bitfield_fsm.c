// exp82: "Bitfield State Machine" — State is encoded as individual bits
// in a uint16_t. Each "transition" sets/clears/toggles specific bits
// based on symbolic input. The KLEE state tree doesn't map to the
// logical FSM state, because the same bitfield value can be reached
// via many different paths.
//
// This tests whether searchers can navigate a state space where the
// "coverage" (which instructions were visited) is orthogonal to the
// actual computed state (which bits are set). Two paths may execute
// identical instructions but compute different bitfield states.
//
// DFS: follows one bit-manipulation chain to completion
// BFS: tries all transitions at each step
// covnew: confused because the same instruction covers different states
// md2u: useless since all code is "covered" quickly
// qc: may prefer cheaper transitions
//
// Expected strong discriminator: DFS should find deep bitfield combos
// that require sequential bit-setting, while breadth-first approaches
// waste time on redundant reachability.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int check_state(uint16_t state) {
    int score = 0;
    // Each unique bit pattern unlocks a unique handler
    if (state == 0x0001) score += 1;
    if (state == 0x0003) score += 2;
    if (state == 0x0007) score += 4;
    if (state == 0x000F) score += 8;
    if (state == 0x001F) score += 16;
    if (state == 0x003F) score += 32;
    if (state == 0x007F) score += 64;
    if (state == 0x00FF) score += 128;
    // "Gray code" patterns — only reachable via specific toggle sequences
    if (state == 0x0101) score += 256;
    if (state == 0x0303) score += 512;
    if (state == 0x0707) score += 1024;
    if (state == 0x0F0F) score += 2048;
    // Sparse patterns
    if (state == 0x5555) score += 4096;
    if (state == 0xAAAA) score += 8192;
    if (state == 0xFF00) score += 16384;
    if (state == 0x00FF) score += 32768;
    return score;
}

int main() {
    uint8_t ops[8];  // 8 operations on the bitfield
    klee_make_symbolic(ops, sizeof(ops), "ops");

    uint16_t state = 0;

    for (int i = 0; i < 8; i++) {
        uint8_t op = ops[i];
        uint8_t action = op & 0x03;      // 4 possible actions
        uint8_t bit_pos = (op >> 2) & 0x0F; // which bit (0-15)

        switch (action) {
            case 0: // SET bit
                state |= (1u << bit_pos);
                break;
            case 1: // CLEAR bit
                state &= ~(1u << bit_pos);
                break;
            case 2: // TOGGLE bit
                state ^= (1u << bit_pos);
                break;
            case 3: // CONDITIONAL SET (set if lower nibble != 0)
                if (state & 0x000F)
                    state |= (1u << bit_pos);
                break;
        }
    }

    int result = check_state(state);

    // Grand prize: specific hard-to-reach state
    if (state == 0xDEAD) return 99999;
    return result;
}
