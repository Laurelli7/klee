#include "klee/klee.h"
#include <stdio.h>
int n, x;
long long a[100001];
long long ans = 1, p = 1000000007;
int main() {
  klee_make_symbolic(&n, sizeof(n), "n");
  klee_make_symbolic(&x, sizeof(x), "x");

	a[0] = 3;
	for (int i = 0; i < n; i++) {
		x++;
		ans *= (a[x - 1] - a[x]);
		ans %= p;
		a[x]++;
	}
}