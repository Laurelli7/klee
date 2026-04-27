// exp107: "Minimal BFS Win" — Direct copy of exp73 pattern with small
// parameter tweaks to verify reproducibility and explore the parameter
// space around the known BFS-winning configuration.
//
// exp73: 4 writes, 16 table, 2 reads → BFS won (+5 over others, +10 DFS)
// exp107: 3 writes, 8 table, 3 reads → smaller table for more collisions
//
// Also adds a tiny deep tail (2 bytes) to weaken DFS slightly.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int handle_hit(int x)  { return x + 1000; }
__attribute__((noinline)) int handle_miss(int x) { return x + 1; }
__attribute__((noinline)) int handle_collision(int x) { return x + 5000; }

int main() {
    uint8_t write_idx[3];
    uint8_t write_val[3];
    uint8_t read_idx[3];
    uint8_t tail[2];
    klee_make_symbolic(write_idx, sizeof(write_idx), "widx");
    klee_make_symbolic(write_val, sizeof(write_val), "wval");
    klee_make_symbolic(read_idx, sizeof(read_idx), "ridx");
    klee_make_symbolic(tail, sizeof(tail), "tail");

    int table[8] = {0};
    int result = 0;

    // 3 symbolic writes
    for (int i = 0; i < 3; i++) {
        uint8_t idx = write_idx[i] & 0x07;
        if (table[idx] != 0) {
            result = handle_collision(result);
        }
        table[idx] = write_val[i] + 1;
    }

    // 3 symbolic reads
    for (int i = 0; i < 3; i++) {
        uint8_t idx = read_idx[i] & 0x07;
        if (table[idx] != 0) {
            result = handle_hit(result);
        } else {
            result = handle_miss(result);
        }
    }

    // Small tail to slightly penalize DFS
    if (tail[0] & 0x01) result++;
    if (tail[0] & 0x02) result++;
    if (tail[0] & 0x04) result++;
    if (tail[0] & 0x08) result++;
    if (tail[1] & 0x01) result++;
    if (tail[1] & 0x02) result++;
    if (tail[1] & 0x04) result++;
    if (tail[1] & 0x08) result++;

    return result;
}
