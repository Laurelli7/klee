// exp01: One extremely deep path vs many shallow paths.
// DFS should get trapped in the deep path. BFS should find shallow bugs first.
#include "klee/klee.h"
int main() {
    unsigned char x;
    klee_make_symbolic(&x, sizeof(x), "x");

    if (x < 200) {
        // Shallow: 200 easy exits
        return x;  // each value of x is a different return
    } else {
        // Deep: A long chain of dependent branches
        int acc = x;
        if (acc & 1)   acc += 3;  else acc -= 1;
        if (acc & 2)   acc += 7;  else acc -= 2;
        if (acc & 4)   acc += 13; else acc -= 3;
        if (acc & 8)   acc += 17; else acc -= 5;
        if (acc & 16)  acc += 23; else acc -= 7;
        if (acc & 32)  acc += 29; else acc -= 11;
        if (acc & 64)  acc += 37; else acc -= 13;
        if (acc & 128) acc += 41; else acc -= 17;
        if (acc == 42) return -1; // "bug" — hard to reach
        return acc;
    }
}
