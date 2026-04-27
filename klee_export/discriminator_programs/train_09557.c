#include "klee/klee.h"
#include <stdio.h>

int main() {
	int N;
  klee_make_symbolic(&N, sizeof(N), "N");

	int t = 1;
	while(t < N) t *= 2;
	if(t == N) {
		return 0;
	}
	for(int i = 1; i <= (N - 1) / 2; i++) {
	}
	if(N % 2 == 0) {
		int a = N & (-N);
		int b = N + 1 - a;
	}
	return 0;
}
