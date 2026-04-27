#include "klee/klee.h"
#include <stdio.h>

int main(void) {

  int n;
  klee_make_symbolic(&n, sizeof(n), "n");

  int hit;
  klee_make_symbolic(&hit, sizeof(hit), "hit");
  int sum;

  int i;


  while(1) {

    if(n==0) break;

    sum=0;

    for(i=0;i<n/4;i++) {

      sum+=hit;

    }

  }

  return 0;

}