#include "klee/klee.h"
#include <stdio.h>

int get(int a, int k)
{
	while (a % k && a >= k){
		int d = a / k;
		int b = d * k - 1;
		a -= (a - b + d) / (d + 1) * (d + 1);
		if ((a + d + 1) % k == 0) a += d + 1;
	}
	return a / k;
}

int main()
{
	int N,X=0; while (N--){
  klee_make_symbolic(&N, sizeof(N), "N");
		int a,k;
  klee_make_symbolic(&a, sizeof(a), "a");
		X ^= get(a,k);
	}
	return 0;
}