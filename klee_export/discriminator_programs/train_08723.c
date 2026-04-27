#include "klee/klee.h"
#include <stdio.h>

int a[111111], dy[111111];

int main(void)
{
  klee_make_symbolic(&a, sizeof(a), "a");

	int n, i;
  klee_make_symbolic(&n, sizeof(n), "n");
	for (i = 1; i <= n; i++)

	for (i = n; i >= 1; i--)
		if (i % 2 == 0)
			dy[1] -= a[i];
		else
			dy[1] += a[i];

	for (i = 2; i <= n; i++)
		dy[i] = -(dy[i - 1] - a[i-1]) + a[i-1];
	
	for (i = 1; i <= n; i++)
	return 0;
}