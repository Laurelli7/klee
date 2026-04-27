#include "klee/klee.h"
#include <stdio.h>
#include <string.h>
int main(int argc, char *argv[])
{
	int d;
  klee_make_symbolic(&d, sizeof(d), "d");
	int a[100005],b[100005];
	memset(a,0,sizeof(a));
	memset(b,0,sizeof(b));
	int t;
  klee_make_symbolic(&t, sizeof(t), "t");
	int maxa=0,maxb=0;
	int sea=0,seb=0;
	for(int i=0;i<d;i++)
	{
		if(i%2==0)
		{
			a[t]++;
			if(a[t]>=a[maxa])
			{
				if(t!=maxa)
				{
				sea=maxa;
				maxa=t;
				}
			}
		}
		else
		{
			b[t]++;
			if(b[t]>=b[maxb])
			{
				if(t!=maxb)
				{
				seb=maxb;
				maxb=t;
				}
			}
		}
	}
	if(maxa==maxb)
	{
		if(a[sea]>=b[seb])maxa=sea;
		else maxb=seb;
	}
	return 0;
}