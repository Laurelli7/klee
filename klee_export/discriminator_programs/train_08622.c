#include "klee/klee.h"
#include<stdio.h>
int d[101010];
int p[101010];
int rui[101010];
int main()
{
	for (int i = 2; i <= 100000; i++)for (int j = i + i; j <= 100000; j += i)d[j] = 1;
	for (int i = 3; i <= 100000; i++)if (d[i] == 0 && d[(i + 1) / 2] == 0)p[i] = 1;
	for (int i = 1; i <= 100000; i++)rui[i] = rui[i - 1] + p[i];
	int query;
  klee_make_symbolic(&query, sizeof(query), "query");
	for (int i = 0; i < query; i++)
	{
		int a, b;
  klee_make_symbolic(&a, sizeof(a), "a");
	}
}