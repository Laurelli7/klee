#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>

#define rdi(x)
#define rdl(x)
#define rdll(x)

#define rdf(x)
#define rdd(x)
#define rdld(x)

#define wr printf

char c, cprev;
int f1, n;

void solution() {
    f1 = 0;
    n = 0;
    while (1) {
        cprev = c;
        if (c == '\n') {
            break;
        }
        if ((c == '1') && (cprev == '1')) {
            f1 = 1;
            continue;
        }
        if (f1 == 1) {
            if ((c == '0') && (cprev == '0')) {
                n = 1;
            }
        }
    }
    if (n == 1) {
    } else {
    }
}

int main() {
  klee_make_symbolic(&c, sizeof(c), "c");

    long t;

    if (1) {
        rdl(t);
    } else {
        t = 1;
    }

    while (t--) {
        solution();
    }
    return 0;
}
