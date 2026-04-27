#include "klee/klee.h"
#include <stdio.h>
int main()
{
	char b[305][305];
	for (int i = 0; i < 305; i++) {
		for (int j = 0; j < 305; j++) {
			b[i][j] = '.';
		}
	}
	int h, w;
  klee_make_symbolic(&h, sizeof(h), "h");
	for (int i = 0; i < h; i++) {
	}
	int a[600][600];
	for (int i = 0; i < 600; i++) {
		for (int j = 0; j < 600; j++) {
			a[i][j] = 0;
		}
	}
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			if (b[i][j] == '#') {
				a[i - j + 300][i + j + 1] = 1;
			}
		}
	}
	int r[600][600];
	for (int i = 0; i < 600; i++) {
		r[i][0] = 0;
		for (int j = 1; j < 600; j++) {
			if (a[i][j])r[i][j] = r[i][j - 1] + 1;
			else r[i][j] = r[i][j - 1];
		}
	}
	int y[600][600];
	for (int i = 0; i < 600; i++) {
		y[i][0] = 0;
		for (int j = 1; j < 600; j++) {
			if (a[j][i])y[i][j] = y[i][j - 1] + 1;
			else y[i][j] = y[i][j - 1];
		}
	}
	int u, v;
	long long ans = 0;
	for (int i = 0; i < 600; i++) {
		for (int j = 0; j < 600; j++) {
			for (int k = j + 1; k < 600; k++) {
				if (a[i][j] && a[i][k]) {
					u = i - (k - j);
					v = i + (k - j);
					if (u >= 0) {
						ans += (r[u][k-1] - r[u][j-1]);
					}
					if (v < 600) {
						ans += (r[v][k] - r[v][j]);
					}
				}
				if (a[j][i] && a[k][i]) {
					u = i - (k - j);
					v = i + (k - j);
					if (u >= 0) {
						ans += (y[u][k] - y[u][j]);
					}
					if (v < 600) {
						ans += (y[v][k - 1] - y[v][j - 1]);
					}
				}
			}
		}
	}
}