// exp05: Extremely lopsided tree — each branch creates one leaf and
// one continuation, like a linked list. DFS goes to max depth
// before backtracking; BFS finds all leaves level by level.
#include "klee/klee.h"
int main() {
    unsigned char data[10];
    klee_make_symbolic(data, sizeof(data), "data");

    for (int i = 0; i < 10; i++) {
        if (data[i] == 0xFF) {
            return i; // early exit (leaf at depth i)
        }
    }
    // Only reached if no byte is 0xFF
    return -1;
}
