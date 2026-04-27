// exp15: "Nearby vs Faraway" — Bug is 1 step away from some states,
// 20 steps away from others. NURS:md2u should find the short path.
// Tests min-distance-to-uncovered heuristic.
#include "klee/klee.h"
#include <stdint.h>

__attribute__((noinline)) int rare_function(int x) {
    return x + 999;
}

int main() {
    uint8_t choice;
    uint8_t path_a[5], path_b;
    klee_make_symbolic(&choice, sizeof(choice), "choice");
    klee_make_symbolic(path_a, sizeof(path_a), "path_a");
    klee_make_symbolic(&path_b, sizeof(path_b), "path_b");

    int result = 0;

    if (choice < 128) {
        // PATH A: Long winding road through 5 bytes of bitfield
        // (20 branches before reaching the rare function)
        for (int i = 0; i < 5; i++) {
            if (path_a[i] & 0x01) result += 1;
            if (path_a[i] & 0x02) result += 2;
            if (path_a[i] & 0x04) result += 4;
            if (path_a[i] & 0x08) result += 8;
        }
        // After 20 branches, FINALLY can reach the rare function
        if (result == 75) {
            return rare_function(result);
        }
        return result;
    } else {
        // PATH B: Shortcut — 1 branch directly to the same rare function
        if (path_b == 42) {
            return rare_function(path_b);
        }
        return 0;
    }
}
