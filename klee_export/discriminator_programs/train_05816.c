#include "klee/klee.h"
#include <stdio.h>
#include <math.h>
int n, c, b;
long long xt, yt;
long long x[1002];
long long y[1002];
long long r[1002];
double m[1002][1002];
double f;
int e[1002];
double d[1002];
double l = 10000000000;
int main() {
  klee_make_symbolic(&n, sizeof(n), "n");
  klee_make_symbolic(&xt, sizeof(xt), "xt");

	n += 2;
	x[n - 1] = xt; y[n - 1] = yt;
	for (int i = 1; i < n - 1; i++) {
	}
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			f = (x[i] - x[j]) * (x[i] - x[j]) + (y[i] - y[j]) * (y[i] - y[j]);
			f = sqrt(f);
			f -= (r[i] + r[j]);
			if (f < 0)f = 0;
			m[i][j] = f;
		}
	}
	c = 1;
	e[0] = 1;
	b = 0;
	for (int i = 1; i < n; i++)d[i] = l;
	while (c < n) {
		for (int i = 0; i < n; i++) {
			if (m[b][i] + d[b] < d[i])d[i] = m[b][i] + d[b];
		}
		f = l;
		for (int i = 0; i < n; i++) {
			if (!e[i] && d[i] < f) {
				b = i;
				f = d[i];
			}
		}
		e[b] = 1;
		c++;
	}
}