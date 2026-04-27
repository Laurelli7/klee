// exp03: Loop with symbolic bound — path depth depends on input value.
// Creates paths of wildly varying depth (0 to 100 iterations).
#include "klee/klee.h"
int main() {
    unsigned char n;
    klee_make_symbolic(&n, sizeof(n), "n");

    int sum = 0;
    for (unsigned char i = 0; i < n && i < 100; i++) {
        sum += i;
        if (sum > 1000) {
            return -1; // "overflow" bug
        }
    }
    return sum;
}
