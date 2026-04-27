// exp37: "State starvation" — A program where one path hogs all
// the execution time by creating an explosion of states, starving
// a competing path that would find a bug quickly.
// The key question: which searchers resist starvation?
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int find_bug_here(int v) {
    if (v == 42) return -1; // BUG
    return v;
}

int main() {
    uint8_t choice;
    uint8_t hog[6];    // state hog
    uint8_t shortcut;  // quick bug path
    klee_make_symbolic(&choice, sizeof(choice), "choice");
    klee_make_symbolic(hog, sizeof(hog), "hog");
    klee_make_symbolic(&shortcut, sizeof(shortcut), "shortcut");

    if (choice < 128) {
        // HOG PATH: 6 bytes × 6 bits = 36 branches → massive explosion
        int r = 0;
        for (int i = 0; i < 6; i++) {
            if (hog[i] & 0x01) r++;
            if (hog[i] & 0x02) r++;
            if (hog[i] & 0x04) r++;
            if (hog[i] & 0x08) r++;
            if (hog[i] & 0x10) r++;
            if (hog[i] & 0x20) r++;
        }
        return r;
    } else {
        // SHORTCUT: 1 check directly to the bug
        return find_bug_here(shortcut);
    }
}
