#include "klee/klee.h"
#include <stdio.h>

int main() {
  int n, f = 0;
  klee_make_symbolic(&n, sizeof(n), "n");
  char c;
  klee_make_symbolic(&c, sizeof(c), "c");
  for(int i = 0; i < n; i++) {
    if(c == 'Y')
      f = 1;
  }
}