// exp13: Asymmetric tree — one very wide subtree, one narrow subtree.
// random-path biases toward wider subtrees (more children = more likely selected).
// BFS is level-fair. Tests random-path's width bias.
#include "klee/klee.h"
#include <stdint.h>

int main() {
    uint8_t selector;
    uint8_t wide[5], narrow[5];
    klee_make_symbolic(&selector, sizeof(selector), "selector");
    klee_make_symbolic(wide, sizeof(wide), "wide");
    klee_make_symbolic(narrow, sizeof(narrow), "narrow");

    if (selector < 200) {
        // WIDE subtree: 5 bytes × 4 bits = 20 branches = 2^20 = 1M paths
        // This is the "distractor" — lots of paths, but they're all boring
        int w = 0;
        for (int i = 0; i < 5; i++) {
            if (wide[i] & 0x01) w++;
            if (wide[i] & 0x02) w++;
            if (wide[i] & 0x04) w++;
            if (wide[i] & 0x08) w++;
        }
        return w;
    } else {
        // NARROW subtree: Sequential unlock of 5 "rooms"
        // Only ~11 paths total, but contains a BUG
        int room = 0;
        if (narrow[0] == 0x10) room++;
        if (room == 1 && narrow[1] == 0x20) room++;
        if (room == 2 && narrow[2] == 0x30) room++;
        if (room == 3 && narrow[3] == 0x40) room++;
        if (room == 4 && narrow[4] == 0x50) room++;
        if (room == 5) return -1; // BUG: requires exact sequence
        return 100 + room;
    }
}
