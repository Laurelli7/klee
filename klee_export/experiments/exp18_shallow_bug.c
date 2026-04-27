// exp18: "Shallow bug" — Bug at depth 2, correct exit at depth 30.
// BFS should find the bug FIRST. DFS will dive to depth 30 first.
#include "klee/klee.h"
#include <stdint.h>

int main() {
    uint8_t data[8];
    klee_make_symbolic(data, sizeof(data), "data");

    // Depth 1-2: Shallow bug — just need data[0]==0xDE && data[1]==0xAD
    if (data[0] == 0xDE && data[1] == 0xAD) {
        return -1; // BUG at depth 2
    }

    // Depth 3-30+: Deep correct paths through bitfield cascade
    int acc = 0;
    for (int i = 0; i < 8; i++) {
        if (data[i] & 0x01) acc++;
        if (data[i] & 0x02) acc++;
        if (data[i] & 0x04) acc++;
        if (data[i] & 0x08) acc++;
    }
    return acc;
}
