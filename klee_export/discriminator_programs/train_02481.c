#include "klee/klee.h"
#include <stdio.h>

int n, d, x, asdf, sum;

int main(){
  klee_make_symbolic(&asdf, sizeof(asdf), "asdf");
  klee_make_symbolic(&n, sizeof(n), "n");

	
	for(int i=0; i<n; i++){
		sum += (d-1)/asdf + 1;
	}
	
	return 0;
}
