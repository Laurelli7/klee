#include "klee/klee.h"
#include <stdio.h>

int main(void) {
  int i, j, n, m;
  klee_make_symbolic(&n, sizeof(n), "n");
  while( 1 ) {
    if(!n && !m) break;
    int mmax[m];
    for(i = 0; i < m; ++i) mmax[i] = 0;
    for(i = 0; i < n; ++i) {
      int d, v;
  klee_make_symbolic(&d, sizeof(d), "d");
      d--;
      if(mmax[d] < v) mmax[d] = v;
    }
    int ans = 0;
    for(i = 0; i < m; ++i) ans += mmax[i];
  }
  return 0;
}
