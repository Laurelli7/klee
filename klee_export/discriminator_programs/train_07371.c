#include "klee/klee.h"
#include <stdio.h>
int a[10000];
int x, n;
int main() {
  klee_make_symbolic(&x, sizeof(x), "x");

	while (a[x] < 2) {
		a[x]++;
		if (x % 2)x = 3 * x + 1;
		else x /= 2;
		n++;
	}
}