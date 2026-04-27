#include "klee/klee.h"
#include <stdio.h>
int n, k;
long long t, s, e;
long long a[100001];
long long x[100001];
int main() {
  klee_make_symbolic(&n, sizeof(n), "n");
  klee_make_symbolic(&t, sizeof(t), "t");

	for (int i = 0; i < n; i++) {
		a[i + 1] = a[i] + t;
		if (t < 0)t = 0;
		x[i + 1] = x[i] + t;
	}
	for (int i = 0; i + k <= n; i++) {
		t = x[n] - (x[i + k] - x[i]);
		s = a[i + k] - a[i];
		if (s < 0)s = 0;
		if (e < t + s)e = t + s;
	}
}