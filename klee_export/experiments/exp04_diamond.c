// exp04: Diamond reconvergence — paths fork then merge, creating
// redundant constraint states that say the same thing differently.
#include "klee/klee.h"
int main() {
    int x, y;
    klee_make_symbolic(&x, sizeof(x), "x");
    klee_make_symbolic(&y, sizeof(y), "y");

    int result;

    // Fork 1
    if (x > 0)
        result = x + 1;
    else
        result = -x + 1;

    // Fork 2 — reconverges to same variable
    if (y > 0)
        result += y;
    else
        result += -y;

    // After reconvergence, this check applies to all 4 paths
    if (result == 42) {
        return -1; // BUG — reachable from multiple diamond paths
    }

    if (result > 100) return 1;
    if (result > 50) return 2;
    return 0;
}
