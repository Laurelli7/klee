// exp31: "Nested loops with early exit" — Two nested loops where
// the inner loop can break early. This creates a state tree where
// early-exit paths are shallow and full-iteration paths are deep.
// DFS should get stuck in the deep full-iteration paths.
// BFS should find the early exits first.
#include "klee/klee.h"
#include <stdint.h>

int main() {
    uint8_t outer_data[4];
    uint8_t inner_data[4];
    klee_make_symbolic(outer_data, sizeof(outer_data), "outer");
    klee_make_symbolic(inner_data, sizeof(inner_data), "inner");

    int total = 0;

    for (int i = 0; i < 4; i++) {
        int inner_sum = 0;
        for (int j = 0; j < 4; j++) {
            if (inner_data[j] == (i * 4 + j)) {
                // Early exit from inner loop
                inner_sum = -1;
                break;
            }
            if (outer_data[i] & (1 << j)) {
                inner_sum += (1 << j);
            }
        }
        total += inner_sum;
        if (total < -2) return -1; // BUG: multiple early exits
    }
    return total;
}
