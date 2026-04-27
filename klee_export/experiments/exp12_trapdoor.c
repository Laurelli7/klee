// exp12: "Trap door" — one easy-to-find entry that leads to an
// exponentially expanding region. Tests whether searchers get
// trapped after finding initial coverage.
#include "klee/klee.h"
#include <stdint.h>

int main() {
    uint8_t key;
    uint8_t maze[5];
    klee_make_symbolic(&key, sizeof(key), "key");
    klee_make_symbolic(maze, sizeof(maze), "maze");

    if (key != 42) {
        return 0; // 255 values lead here: trivial
    }

    // key == 42: Trap door opens into a maze
    // 5 bytes × 5 bit-tests = 25 branches = 2^25 = 33M paths
    int score = 0;
    for (int i = 0; i < 5; i++) {
        if (maze[i] & 0x01) score += 1;
        if (maze[i] & 0x02) score += 2;
        if (maze[i] & 0x04) score += 4;
        if (maze[i] & 0x08) score += 8;
        if (maze[i] & 0x10) score += 16;
    }

    if (score == 155) return -1; // Deep bug
    return score;
}
