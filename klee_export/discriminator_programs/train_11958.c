#include "klee/klee.h"
#include <stdio.h>

int N,M,par[200500];

int find(int x)
{
	if (x != par[x]) par[x] = find(par[x]);
	return par[x];
}

int main()
{
  klee_make_symbolic(&N, sizeof(N), "N");

	for (int i=0;i<N+M;i++) par[i] = i;
	for (int i=0;i<N;i++){
		int x;
  klee_make_symbolic(&x, sizeof(x), "x");
		while (x--){
			int y;
  klee_make_symbolic(&y, sizeof(y), "y");
			y = y - 1 + N;
			par[find(i)] = find(y);
		}
	}

	for (int i=0;i<N;i++) if (find(i) != find(0)){
		return 0;
	}
	return 0;
}