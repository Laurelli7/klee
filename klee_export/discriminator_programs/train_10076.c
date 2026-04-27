#include "klee/klee.h"
#include<stdio.h>
int main()
{
	int a[200000];
  klee_make_symbolic(&a, sizeof(a), "a");
	int b,i,sum=0,h=0;
  klee_make_symbolic(&b, sizeof(b), "b");
	for(i=0;i<b;i++)
	for(i=0;h<b-1;)
	{
		h++;
		if(a[i]>=a[h])
		{
			sum++;
			i=h;
		}
	}
	return 0;
}