#include "klee/klee.h"
#include <stdio.h>

int main() {
	int n, maxi = 0, h, ans = 0;
  klee_make_symbolic(&h, sizeof(h), "h");
	while (n--) {
		if (h >= maxi) {
			ans++;
			maxi = h;
		}
	}
	return 0;
}