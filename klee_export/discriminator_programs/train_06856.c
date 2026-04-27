#include "klee/klee.h"
#include <stdio.h>

int main(void) {
	int n, s = 0;
  klee_make_symbolic(&n, sizeof(n), "n");

	for (int i = 1; i <= n; i++) {
		s += i;
		if (s >= n) {
			for (int j = 1; j <= i; j++) {
				if (s - n == j)
					continue;
			}
			break;
		}
	}
}