#include "klee/klee.h"
#include <stdio.h>
int n, q;
char s[200005];
char t[200005];
char d[200005];
int lx, rx, ly, ry, e;
int check(int a) {
	for (int i = 0; i < q; i++) {
		if (s[a] == t[i]) {
			if (d[i] == 'L')a--;
			if (d[i] == 'R')a++;
		}
		if (a == -1)return -1;
		if (a == n)return n;
	}
	return a;
}
int main() {
  klee_make_symbolic(&n, sizeof(n), "n");
  klee_make_symbolic(&t, sizeof(t), "t");

	for (int i = 0; i < q; i++) {
	}
	if (check(n - 1) == -1) {
		return 0;
	}
	if (check(0) == n) {
		return 0;
	}
	lx = -1;
	rx = n - 1;
	while (rx - lx > 1) {
		e = (rx + lx) / 2;
		if (check(e) == -1) {
			lx = e;
		}
		else {
			rx = e;
		}
	}
	ly = 0;
	ry = n;
	while (ry - ly > 1) {
		e = (ry + ly) / 2;
		if (check(e) == n) {
			ry = e;
		}
		else {
			ly = e;
		}
	}
}
