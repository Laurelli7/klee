#include "klee/klee.h"
#include <stdio.h>

int main() {
    int t;
  klee_make_symbolic(&t, sizeof(t), "t");
    while (t--) {
        int x, i;
  klee_make_symbolic(&x, sizeof(x), "x");
        for (i = 1; i * (i + 1) / 2 < x; i++);
        if (i * (i + 1) / 2 - x != 1) {
        } else {
        }
    }
}