// exp08: "Staircase" — each step requires the PREVIOUS step's result.
// Creates sequential dependencies that force linear exploration.
// DFS should handle this naturally; BFS wastes effort on infeasible states.
#include "klee/klee.h"
int main() {
    unsigned char key[8];
    klee_make_symbolic(key, sizeof(key), "key");

    // Each check depends on success of the previous
    int level = 0;
    if (key[0] == 'K') level++;
    if (level == 1 && key[1] == 'L') level++;
    if (level == 2 && key[2] == 'E') level++;
    if (level == 3 && key[3] == 'E') level++;
    if (level == 4 && key[4] == '_') level++;
    if (level == 5 && key[5] == 'F') level++;
    if (level == 6 && key[6] == 'T') level++;
    if (level == 7 && key[7] == 'W') level++;

    if (level == 8) return -1; // Full key match: "KLEE_FTW"
    return level;
}
