#include "klee/klee.h"
#include<stdio.h>

int main() {

	int k;
  klee_make_symbolic(&k, sizeof(k), "k");
	int i, j;
	int n;
	int c[500][500];

	//入力

	if (k == 1) {
		return 0;
	}

	n = (k + 3) / 4 * 2;

	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
			c[i][j] = (i + j) % n + 1;
		}
	}
	for (i = 1; i < n; i += 2) {
		for (j = 0; j < n; j++) {
			if (c[i][j] + n <= k)c[i][j] += n;
		}
	}

	//出力
	for (i = 0; i < n; i++) {
		for (j = 0; j < n; j++) {
		}
	}

	return 0;
}