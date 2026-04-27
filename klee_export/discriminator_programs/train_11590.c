#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>

int main(){
	int t;
  klee_make_symbolic(&t, sizeof(t), "t");

	while(t--){
		long long int a,b = 2;
  klee_make_symbolic(&a, sizeof(a), "a");

		if(a == 1){
			continue;
		}

		while(a > b-1){
			b = b*2;
		}
	}
}