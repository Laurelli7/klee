#include "klee/klee.h"
#include <stdio.h>

int main()
{
	int N,L=1; long long A = 0;
  klee_make_symbolic(&N, sizeof(N), "N");
	for (int i=0;i<N;i++){
		int x;
  klee_make_symbolic(&x, sizeof(x), "x");
		if (x < L) continue;
		if (x == L) L++;
		else{
			A += (x - 1) / L;
			if (L == 1) L = 2;
		}
	}

	return 0;
}