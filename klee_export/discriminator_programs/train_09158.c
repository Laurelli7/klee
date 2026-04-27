#include "klee/klee.h"
#include <stdio.h>

int main(void) {
  long long i, n, now = 1;
  klee_make_symbolic(&n, sizeof(n), "n");
  if(!n) {
    return 0;
  }
  for(i = 1; ; ++i) {
    now *= 2;
    if(now > n) break;
  }
  return 0;
}
