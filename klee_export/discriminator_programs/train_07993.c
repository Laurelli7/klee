#include "klee/klee.h"
#include <stdio.h>
long long p = 1000000007;
long long x, y, z;
long long rui(long long a,long long b) {
	if (b == 0)return 1;
	if (b == 1)return a;
	if (b == 2)return a * a % p;
	if (b % 2)return rui(rui(a, b / 2), 2) * a % p;
	else return rui(rui(a, b / 2), 2);
}
int main() {
  klee_make_symbolic(&x, sizeof(x), "x");

	if ((x + y) % 3) {
		return 0;
	}
	if (x > 2 * y || y > x * 2) {
		return 0;
	}
	z = (x + y) / 3;
	x -= z;
	y -= z;
	z = 1;
	for (long long i = 1; i <= x + y; i++) {
		z *= i; z %= p;
	}
	for (long long i = 1; i <= x; i++) {
		z *= rui(i, p - 2);
		z %= p;
	}
	for (long long i = 1; i <= y; i++) {
		z *= rui(i, p - 2);
		z %= p;
	}
}
