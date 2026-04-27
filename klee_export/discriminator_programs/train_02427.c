#include "klee/klee.h"
#include<stdio.h>
#include<stdlib.h> 

int main(void)
{
	int t,i;
  klee_make_symbolic(&t, sizeof(t), "t");
	for(i=0;i<t;i++)
	{
		long long int n,m,x,num,l,h;
  klee_make_symbolic(&n, sizeof(n), "n");
		if(x%n==0)
		{
			num=x/n+(n-1)*m;
		}
		else
		{
			l=x/n;
			h=x-n*l;
			num=l+1+(h-1)*m;
		}
	} 
	return 0;
}
