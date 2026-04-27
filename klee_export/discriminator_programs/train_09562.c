#include "klee/klee.h"
#include <stdio.h>

int main() {
  long long N, T, sum = 0, cur = 0;
  klee_make_symbolic(&N, sizeof(N), "N");

  for (int i = 0; i < N; i++) {
    long long t, add = T;
  klee_make_symbolic(&t, sizeof(t), "t");

    if (cur > t) add = T - (cur - t);
    sum += add;
    cur = t + T;
  }
  return 0;
}
