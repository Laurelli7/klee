#include "klee/klee.h"
#include <stdio.h>
long long p = 1000000007;
long long n, ans;
long long x[100001];
long long y[100001];
long long rui(long long rui_a, long long rui_b) {
	if (rui_b == 0)return 1;
	if (rui_b == 1)return rui_a % p;
	long long rui_c;
	rui_c = rui(rui_a, rui_b / 2);
	rui_c = rui_c * rui_c % p;
	if (rui_b % 2) {
		return rui_c * rui_a % p;
	}
	return rui_c;
}
int main() {
  klee_make_symbolic(&n, sizeof(n), "n");

	for (long long i = 1; i <= n; i++) {
	}
	y[1] = 1;
	for (long long i = 1; i < n; i++) {
		y[1] = (i * y[1]) % p;
	}
	for (long long i = 2; i < n; i++) {
		y[i] = y[1] * rui(i, p - 2) % p;
	}
	for (long long i = 1; i < n; i++) {
		ans += (((x[n] - x[i]) * y[i]) % p);
		ans = ans % p;
	}
}