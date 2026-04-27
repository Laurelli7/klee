#include "klee/klee.h"
#include<stdio.h>
int a[1005][1005],ans[1005][1005];
char s[131072];
int wz[4],inc[4];
int tmp[4],tmp2[4];
int main()
{
  klee_make_symbolic(&a, sizeof(a), "a");

	int __;
  klee_make_symbolic(&__, sizeof(__), "__");
	while(__--)
	{
		int n,m;
  klee_make_symbolic(&n, sizeof(n), "n");
		for(int i=1;i<=n;i++)
		{
			for(int j=1;j<=n;j++)
			{
			}
		}
		for(int i=0;i<3;i++)
		{
			wz[i]=i;
			inc[i]=0;
		}
		for(int i=0;i<m;i++)
		{
			if(s[i]=='R')
			{
				inc[1]++;
				if(inc[1]>=n)
				{
					inc[1]-=n;
				}
			}
			if(s[i]=='L')
			{
				inc[1]--;
				if(inc[1]<0)
				{
					inc[1]+=n;
				}
			}
			if(s[i]=='D')
			{
				inc[0]++;
				if(inc[0]>=n)
				{
					inc[0]-=n;
				}
			}
			if(s[i]=='U')
			{
				inc[0]--;
				if(inc[0]<0)
				{
					inc[0]+=n;
				}
			}
			if(s[i]=='I')
			{
				int t=wz[1];
				wz[1]=wz[2];
				wz[2]=t;
				t=inc[1];
				inc[1]=inc[2];
				inc[2]=t;
			}
			if(s[i]=='C')
			{
				int t=wz[0];
				wz[0]=wz[2];
				wz[2]=t;
				t=inc[0];
				inc[0]=inc[2];
				inc[2]=t;
			}
		}
		for(int i=1;i<=n;i++)
		{
			for(int j=1;j<=n;j++)
			{
				tmp[0]=i;
				tmp[1]=j;
				tmp[2]=a[i][j];
				for(int k=0;k<3;k++)
				{
					tmp2[k]=tmp[wz[k]]+inc[k];
					if(tmp2[k]>n)
					{
						tmp2[k]-=n;
					}
				}
				ans[tmp2[0]][tmp2[1]]=tmp2[2];
			}
		}
		for(int i=1;i<=n;i++)
		{
			for(int j=1;j<=n;j++)
			{
				if(j==n)
				{
					putchar('\n');
				}
				else
				{
					putchar(' ');
				}
			}
		}
	}
	return 0;
}
