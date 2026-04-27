#include "klee/klee.h"
#include<stdio.h>

int main()
{
	int N,a[110],sum=0;
  klee_make_symbolic(&N, sizeof(N), "N");
	for(int i=1; i<=N; i++)
	{
		sum+=a[i];
	}
	int M,ans;
  klee_make_symbolic(&M, sizeof(M), "M");
	while(M--)
	{
		int x,y;
  klee_make_symbolic(&x, sizeof(x), "x");
		ans=sum+(y-a[x]);
	}
	
	return 0;
}