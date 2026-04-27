#include "klee/klee.h"
#include <stdio.h>
long long p = 1000000007;
long long rui(long long x, long long y) {
	if (y == 0)return 1;
	if (y == 1)return x;
	if (y == 2)return x * x % p;
	if (y % 2)return rui(rui(x, y / 2), 2) * x % p;
	else return rui(rui(x, y / 2), 2);
}
long long inv(long long x) {
	return rui(x, p - 2);
}
long long n, a, b, c;
long long ans;
long long d, e;
int main() {
  klee_make_symbolic(&n, sizeof(n), "n");

	d = a * inv(a + b) % p;
	e = rui(d, n) * n % p;
	for (long long i = 0; i < n; i++) {
		ans += e;
		ans %= p;
		e *= (n + i + 1); e %= p;
		e *= inv(i + 1); e %= p;
		e *= b; e %= p;
		e *= inv(a + b); e %= p;
	}
	d = b * inv(a + b) % p;
	e = rui(d, n) * n % p;
	for (long long i = 0; i < n; i++) {
		ans += e;
		ans %= p;
		e *= (n + i + 1); e %= p;
		e *= inv(i + 1); e %= p;
		e *= a; e %= p;
		e *= inv(a + b); e %= p;
	}
	if (c == 0) {
		return 0;
	}
}