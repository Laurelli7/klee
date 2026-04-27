#include "klee/klee.h"
#include <stdio.h>
int n;
int g[500][500];
void solve(int bgn, int end, int k) {
	if (bgn == end)return;
	int h = (bgn + end) / 2;
	h++;
	for (int i = bgn; i < h; i++) {
		for (int j = h; j <= end; j++) {
			g[i][j] = k;
		}
	}
	solve(bgn, h - 1, k + 1);
	solve(h, end, k + 1);
	return;
}
int main() {
  klee_make_symbolic(&n, sizeof(n), "n");

	solve(0, n - 1, 1);
	for (int i = 0; i < n - 1; i++) {
		for (int j = i + 1; j < n - 1; j++) {
		}
	}
}