#include "klee/klee.h"
#include<stdio.h>
#include<string.h>
int tcn, n, tmp, x[200010], y[200010], cx[200010], cy[200010], a, b, k;;
int main() {
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&tcn, sizeof(tcn), "tcn");
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&y, sizeof(y), "y");

	for (int tc = 1; tc <= tcn; tc++) {
		for (int i = 1; i <= k; i++) {
			cx[x[i]]++;
		}
		for (int i = 1; i <= k; i++) {
			cy[y[i]]++;
		}
		long long int res = 0;
		for (int i = 1; i <= k; i++) {
			res += k + 1 - cx[x[i]] - cy[y[i]];
		}
		memset(cx, 0, sizeof(cx));
		memset(cy, 0, sizeof(cy));
	}
}