#include "klee/klee.h"
#include <stdio.h>
#include <string.h>

int dots[][5] = {
	{ 1, 0, 0, 1, 0 }, { 1, 1, 0, 2, 0 }, { 2, 0, 0, 1, 1 }, { 2, 1, 0, 1, 2 },
	{ 1, 1, 0, 1, 1 }, { 2, 1, 0, 2, 1 }, { 2, 2, 0, 2, 2 }, { 1, 2, 0, 2, 1 },
	{ 1, 1, 0, 1, 1 }, { 1, 2, 0, 1, 2 }, { 1, 0, 1, 2, 0 }, { 1, 1, 1, 3, 0 },
	{ 2, 0, 1, 2, 1 }, { 2, 1, 1, 2, 2 }, { 1, 1, 1, 2, 1 }, { 2, 1, 1, 3, 1 },
	{ 2, 2, 1, 3, 2 }, { 1, 2, 1, 3, 1 }, { 1, 1, 1, 2, 1 }, { 1, 2, 1, 2, 2 },
	{ 1, 0, 2, 2, 1 }, { 1, 1, 2, 3, 1 }, { 1, 2, 1, 1, 3 }, { 2, 0, 2, 2, 2 },
	{ 2, 1, 2, 2, 3 }, { 1, 1, 2, 2, 2 }
};

int main() {
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	while (n--) {
		static int aa[5];
  klee_make_symbolic(&aa, sizeof(aa), "aa");
		int i, a;

		for (i = 0; i < 5; i++)
		for (a = 0; a < 26; a++)
			if (memcmp(dots[a], aa, 5 * sizeof *aa) == 0) {
				break;
			}
	}
	return 0;
}
