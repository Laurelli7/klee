// exp30: "Symbolic array index" — Access array with symbolic index.
// Creates a state per possible index value. Tests how searchers
// handle memory resolution forks (not branch forks).
#include "klee/klee.h"
#include <stdint.h>

int lookup_table[256];

int main() {
    uint8_t idx1, idx2;
    klee_make_symbolic(&idx1, sizeof(idx1), "idx1");
    klee_make_symbolic(&idx2, sizeof(idx2), "idx2");

    // Initialize table
    for (int i = 0; i < 256; i++) {
        lookup_table[i] = i * 3 + 1;
    }

    int val1 = lookup_table[idx1]; // Fork per possible index
    int val2 = lookup_table[idx2]; // Fork per possible index

    if (val1 + val2 == 42) return -1; // BUG
    if (val1 > val2) return 1;
    return 0;
}
