#include "klee/klee.h"
#include<stdio.h>
int main()
{
	int q;
  klee_make_symbolic(&q, sizeof(q), "q");
	while(q--)
	{
		int a,b,c,d;
  klee_make_symbolic(&a, sizeof(a), "a");
		int k;
		if(a<b)
		{
			k=(a+b)/(a+1);
		}
		else
		{
			k=(a+b)/(b+1);
		}
		if(k==1)
		{
			for(int i=c;i<=d;i++)
			{
				if(b>a)
				{
					putchar('A'+(i&1));
				}
				else
				{
					putchar('B'-(i&1));
				}
			}
			putchar('\n');
			continue;
		}
		long long rm=(long long)k*(a+1)-b;
		int wz=0;
		if(rm)
		{
			int tim=0;
			if(rm>=(long long)k*k)
			{
				tim=(rm-1)/((long long)k*k-1);
			}
			wz+=tim*(k+1);
			rm-=tim*((long long)k*k-1);
			wz+=rm/k;
			rm-=rm/k*k;
		}
		for(int i=c;i<=d;i++)
		{
			if(i<=wz)
			{
				putchar('A'+(i%(k+1)==0));
			}
			else
			{
				putchar('A'+((i-wz+rm)%(k+1)!=0));
			}
		}
		putchar('\n');
	}
	return 0;
}