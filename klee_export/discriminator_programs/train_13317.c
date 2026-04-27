#include "klee/klee.h"
#include <stdio.h>

#pragma warning(disable : 4996)

int n, m, c, d, imos[1111];

int main()
{
  klee_make_symbolic(&c, sizeof(c), "c");
  klee_make_symbolic(&d, sizeof(d), "d");
  klee_make_symbolic(&m, sizeof(m), "m");


	for (int i = 0; i < m; i++)
	{

		imos[c--]++;
		imos[d--]--;
	}

	int ret = n + 1, sum = 0;
  klee_make_symbolic(&n, sizeof(n), "n");

	for (int i = 0; i < n; i++)
	{
		sum += imos[i];

		if (sum > 0) ret += 2;
	}

	return 0;
}