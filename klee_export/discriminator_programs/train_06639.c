#include "klee/klee.h"
#include <stdio.h>

int count[1000010],check[1000010],x[200010];
int main()
{
  klee_make_symbolic(&x, sizeof(x), "x");

	int a;
  klee_make_symbolic(&a, sizeof(a), "a");
	for(int i=1;i<=a;i++)
	for(int i=1;i<=a;i++) count[x[i]]++;
	
	int ans = 0;
	for(int i=1;i<=1000000;i++)
	{
		if(count[i]>0)
		{
			if(count[i]==1 && check[i]==0) ans++;
			for(int j=i;j<=1000000;j+=i) check[j] = 1;
		}
	}
}