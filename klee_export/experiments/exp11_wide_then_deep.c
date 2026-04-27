// exp11: "Wide-then-deep" — first a wide fan (many shallow states),
// then a deep chain. BFS thrives in the wide part,
// DFS thrives in the deep part.
#include "klee/klee.h"
#include <stdint.h>

int main() {
    uint8_t selector;
    uint8_t data[6];
    klee_make_symbolic(&selector, sizeof(selector), "selector");
    klee_make_symbolic(data, sizeof(data), "data");

    if (selector < 128) {
        // WIDE region: 6 bytes, each with 4 bit-tests = 24 branches = 2^24 paths
        int acc = 0;
        for (int i = 0; i < 6; i++) {
            if (data[i] & 0x01) acc++;
            if (data[i] & 0x02) acc++;
            if (data[i] & 0x04) acc++;
            if (data[i] & 0x08) acc++;
        }
        if (acc == 24) return 99;
        return acc;
    } else {
        // DEEP region: sequential key-check staircase
        // Fewer total paths but deeper
        int level = 0;
        for (int i = 0; i < 6; i++) {
            if (level == i && data[i] == ('A' + i)) {
                level++;
            }
        }
        if (level == 6) return -1; // BUG: data = "ABCDEF"
        return level;
    }
}
