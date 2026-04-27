#include "klee/klee.h"
#include <stdio.h>
#pragma warning(disable : 4996)
int s, m, n, r, c;
int solve(int x) {
	int ret = 0;
	for (int i = 0; i * i <= x; i++) {
		for (int j = 0; i * i + j * j <= x; j++) {
			if (i * i + j * j == x) {
				ret++;
			}
		}
	}
	return ret;
}
int main() {
  klee_make_symbolic(&m, sizeof(m), "m");
  klee_make_symbolic(&s, sizeof(s), "s");

	for (int i = 0; i < s; i++) {
		r = m * m + n * n; c = 0;
		for (int i = 1; i <= r; i++) {
			if (r % i == 0) {
				c += solve(i) * solve(r / i);
			}
		}
	}
	return 0;
}