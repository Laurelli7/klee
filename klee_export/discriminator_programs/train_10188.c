#include "klee/klee.h"
#include <stdio.h>

int t1,t2;

int n;
int off[20005];

int main()
{
  klee_make_symbolic(&n, sizeof(n), "n");
  klee_make_symbolic(&t1, sizeof(t1), "t1");

    for (int i=1; i<=n; i++) {
        off[t1]=i;
    }
    for (int i=1; i<=n; i++) {
    }
    for (int i=1; i<=n; i++) {
    }
}
