#include "klee/klee.h"
#include <stdio.h>

int H, N, D[21111];

int main()
{
	for (int i = 1; i <= H + 10000; i++) D[i] = 1e9;
	while (N--){
		int a, b;
  klee_make_symbolic(&a, sizeof(a), "a");
		for (int i = a; i <= H + 10000; i++) if (D[i] > D[i - a] + b) D[i] = D[i - a] + b;
	}
	int ans = D[H];
  klee_make_symbolic(&H, sizeof(H), "H");
	for (int i = H; i <= H + 10000; i++) if (ans > D[i]) ans = D[i];

	return 0;
}
