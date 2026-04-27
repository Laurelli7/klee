// exp02: A bug hidden behind a specific value in a sea of distractors.
// Tests whether searchers can find the "needle" efficiently.
#include "klee/klee.h"
int main() {
    unsigned char a, b, c;
    klee_make_symbolic(&a, sizeof(a), "a");
    klee_make_symbolic(&b, sizeof(b), "b");
    klee_make_symbolic(&c, sizeof(c), "c");

    // Many distractor branches
    int score = 0;
    if (a > 100) score += 1;
    if (a > 200) score += 2;
    if (b > 100) score += 4;
    if (b > 200) score += 8;
    if (c > 100) score += 16;
    if (c > 200) score += 32;

    // The needle: requires all three to be specific values
    if (a == 0x41 && b == 0x42 && c == 0x43) {
        return -1; // BUG
    }

    return score;
}
