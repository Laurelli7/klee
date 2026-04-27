// exp98: "Write-Scatter" — Building on exp73 (symbolic write), the only
// previous BFS win. Amplifies the effect by using MORE symbolic writes
// to a SMALLER table, creating more collision/non-collision combinations.
//
// Symbolic writes create fork-on-write semantics. BFS explores all
// possible write targets at each step evenly. DFS follows one write
// sequence deeply. The critical insight: different ORDERINGS of writes
// create different code paths (collision vs non-collision), and BFS
// evenly samples orderings while DFS deeply follows one.
//
// Additionally, each collision/non-collision event calls a UNIQUE handler
// (not shared like exp73) for more CoveredInstr differentiation.
//
// Expected: BFS > all others (amplified version of exp73)
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int on_empty_write(int acc, int slot) {
    return acc + slot * 10 + 1;
}
__attribute__((noinline)) int on_collision(int acc, int slot, int old_val) {
    return acc + slot * 20 + old_val;
}
__attribute__((noinline)) int on_read_hit(int acc, int val) {
    return acc + val * 3;
}
__attribute__((noinline)) int on_read_miss(int acc, int slot) {
    return acc + slot + 5000;
}
__attribute__((noinline)) int on_overwrite(int acc, int slot, int new_val) {
    return acc + slot * 100 + new_val;
}
__attribute__((noinline)) int finalize(int acc, int count) {
    return acc ^ (count << 8);
}

int main() {
    uint8_t write_idx[6];  // 6 symbolic write indices
    uint8_t write_val[6];  // 6 symbolic values
    uint8_t read_idx[3];   // 3 symbolic read indices
    klee_make_symbolic(write_idx, sizeof(write_idx), "widx");
    klee_make_symbolic(write_val, sizeof(write_val), "wval");
    klee_make_symbolic(read_idx, sizeof(read_idx), "ridx");

    int table[8] = {0};  // 8-element table (smaller = more collisions)
    int result = 0;
    int collision_count = 0;
    int write_count = 0;

    // 6 symbolic writes to 8-element table
    for (int i = 0; i < 6; i++) {
        uint8_t idx = write_idx[i] & 0x07;  // 8 possible positions
        int new_val = write_val[i] + 1;      // +1 so non-zero

        if (table[idx] == 0) {
            result = on_empty_write(result, idx);
            table[idx] = new_val;
            write_count++;
        } else if (table[idx] == new_val) {
            result = on_overwrite(result, idx, new_val);
            // Same value: no-op
        } else {
            result = on_collision(result, idx, table[idx]);
            table[idx] = new_val; // overwrite
            collision_count++;
        }
    }

    // 3 symbolic reads
    for (int i = 0; i < 3; i++) {
        uint8_t idx = read_idx[i] & 0x07;
        if (table[idx] != 0) {
            result = on_read_hit(result, table[idx]);
        } else {
            result = on_read_miss(result, idx);
        }
    }

    result = finalize(result, collision_count);
    return result;
}
