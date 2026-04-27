#include "klee/klee.h"
#include <stdio.h>

#define MD	1000000007

long long gcd(long long a, long long b) {
	return b == 0 ? a : gcd(b, a % b);
}

int main() {
	int t;
  klee_make_symbolic(&t, sizeof(t), "t");
	while (t--) {
		long long n, a, b, ans;
  klee_make_symbolic(&n, sizeof(n), "n");
		ans = n, b = 1;
		for (a = 1; a <= n; a++) {
			b *= a / gcd(a, b);
			if (b > n)
				break;
			ans = (ans + n / b) % MD;
		}
	}
	return 0;
}
