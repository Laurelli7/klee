// exp06: Cascading modular arithmetic — generates many states that differ
// only in constraints but not in future execution, wasting work.
#include "klee/klee.h"
int main() {
    unsigned x;
    klee_make_symbolic(&x, sizeof(x), "x");

    int bucket;
    if (x % 7 == 0) bucket = 0;
    else if (x % 7 == 1) bucket = 1;
    else if (x % 7 == 2) bucket = 2;
    else if (x % 7 == 3) bucket = 3;
    else if (x % 7 == 4) bucket = 4;
    else if (x % 7 == 5) bucket = 5;
    else bucket = 6;

    // Second layer of modular branching
    int sub;
    if (x % 3 == 0) sub = 0;
    else if (x % 3 == 1) sub = 1;
    else sub = 2;

    if (bucket == 3 && sub == 1 && x < 100) {
        return -1; // BUG: needs x%7==3 AND x%3==1 AND x<100
    }
    return bucket * 10 + sub;
}
