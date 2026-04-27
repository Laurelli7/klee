#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>

long common_elem(long i, long j) {
  if (j < i) 
    return i * (i - 1) / 2 + 1 + j;
  else
    return common_elem(j, i);
}

int main(int argc, char *argv[]) {
  long N;
  klee_make_symbolic(&N, sizeof(N), "N");
  //
  
  // find k
  long k = -1;
  for (long i = 2; i * (i - 1) <= 2 * N; i++) {
    if (i * (i - 1) == 2 * N) {
      k = i;
      break;
    }
  }
  
  if (k < 0) {
  } else {
    for (long i = 0; i < k; i++) {
      for (long j = 0; j < k; j++) {
        if (j != i) {
        }
      }
    }
  }
  
  return 0;
}
