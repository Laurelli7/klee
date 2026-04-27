#include "klee/klee.h"
#include <stdio.h>
#include <math.h>

int main(void) {
  int i, j, k, d, e;
  klee_make_symbolic(&d, sizeof(d), "d");
  while( 1 ) {
    double ans = 10e9;
    if(!d && !e) break;
    for(i = 0; i <= d / 2; ++i) {
      double t = sqrt(i * i + (d - i) * (d - i)) - (double)e;
      if((t >= 0 ? t : -t) < ans) ans = t >= 0 ? t : -t;
    }
  }
  return 0;
}
