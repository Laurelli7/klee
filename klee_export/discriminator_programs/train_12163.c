#include "klee/klee.h"
#include <stdio.h>
int n, a[100100], m;
int abs(int x)
{
	return x > 0 ? x : -x;
}
int main(void)
{
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&n, sizeof(n), "n");

	for (int i = 1; i <= n+1; i++)
	{
		if(i<=n)
		m += abs(a[i] - a[i - 1]);
	}
	for (int i = 1; i <= n; i++)
	{
	}
	return 0;
}