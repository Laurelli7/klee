// exp73: "Symbolic Array Write" — A program that WRITES to an array
// at a symbolic index, then reads back. This creates fork-on-write
// semantics that are fundamentally different from fork-on-read.
//
// When KLEE writes to array[symbolic_index], it must consider ALL
// possible index values and create a state for each. This is a
// common real-world pattern (hash table insert, buffer write).
//
// Structure: 4 symbolic writes to a 16-element array, then 4 reads.
// Each write creates up to 16 forks. Reads of symbolic-write cells
// create additional forks.
//
// DFS: follows one write sequence, completes it
// BFS: tries all 16 write targets at each step: 16→256→4096→65536
// covnew: writes to different cells = different code? Not quite.
//         Actually, the code is the same — the CONSTRAINT differs.
// random-path: samples write sequences uniformly
//
// Expected: DFS should complete most paths. BFS should explode.
// This pattern should differentiate DFS from everything else.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int handle_hit(int x)  { return x + 1000; }
__attribute__((noinline)) int handle_miss(int x) { return x + 1; }
__attribute__((noinline)) int handle_collision(int x) { return x + 5000; }
__attribute__((noinline)) int handle_full(int x) { return x + 9000; }

int main() {
    uint8_t write_idx[4];  // 4 symbolic write indices
    uint8_t write_val[4];  // 4 symbolic values to write
    uint8_t read_idx[2];   // 2 symbolic read indices
    klee_make_symbolic(write_idx, sizeof(write_idx), "widx");
    klee_make_symbolic(write_val, sizeof(write_val), "wval");
    klee_make_symbolic(read_idx, sizeof(read_idx), "ridx");

    // The array
    int table[16] = {0};  // all zeroes initially
    int result = 0;

    // 4 symbolic writes
    for (int i = 0; i < 4; i++) {
        uint8_t idx = write_idx[i] & 0x0F;  // 16 possible positions
        if (table[idx] != 0) {
            // Collision detected
            result = handle_collision(result);
        }
        table[idx] = write_val[i] + 1; // +1 so non-zero means occupied
    }

    // 2 symbolic reads
    for (int i = 0; i < 2; i++) {
        uint8_t idx = read_idx[i] & 0x0F;
        if (table[idx] != 0) {
            result = handle_hit(result);
        } else {
            result = handle_miss(result);
        }
    }

    // Check occupancy
    int count = 0;
    for (int i = 0; i < 16; i++) {
        if (table[i] != 0) count++;
    }

    if (count == 4) result = handle_full(result); // all 4 writes to distinct cells

    return result;
}
