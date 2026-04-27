#include "klee/klee.h"
#include <stdio.h>

int main()
{
	int N,K;
  klee_make_symbolic(&N, sizeof(N), "N");
	long long A = 0;
	if (K == 0) A -= N;

	for (int b=K+1;b<=N;b++){
		A += (N + 1) / b * (b - K);
		int s = (N + 1) % b;
		if (s){
			if (s > K) A += s - K;
		}
	}

	return 0;
}