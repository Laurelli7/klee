#include "klee/klee.h"
#include <stdio.h>
int n;
long long p, e, a;
long long d[101][301];
long long w[100];
long long v[100];
int main() {
  klee_make_symbolic(&n, sizeof(n), "n");

	for (int i = 0; i < n; i++) {
	}
	e = w[0];
	for (int i = 0; i < n; i++)w[i] -= e;
	for (int i = 0; i < n; i++) {
		for (int j = 100; j >= 0; j--) {
			for (int k = 300; k >= 0; k--) {
				if (j + 1 < 100 && k + w[i] <= 300) {
					if (d[j + 1][k + w[i]] < d[j][k] + v[i]) {
						d[j + 1][k + w[i]] = d[j][k] + v[i];
					}
				}
			}
		}
	}
	for (int i = 0; i <= 100; i++) {
		for (int j = 0; j <= 300; j++) {
			if (i * e + j > p)d[i][j] = 0;
			if (a < d[i][j])a = d[i][j];
		}
	}
}