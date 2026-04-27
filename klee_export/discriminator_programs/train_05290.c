#include "klee/klee.h"
#include<stdio.h>
int p[8192],wz[8192];
long long dp[5005][5005];
int main()
{
  klee_make_symbolic(&p, sizeof(p), "p");

	int n,a,b;
  klee_make_symbolic(&n, sizeof(n), "n");
	for(int i=1;i<=n;i++)
	{
		wz[p[i]]=i;
	}
	for(int i=0;i<=n;i++)
	{
		for(int j=0;j<=n;j++)
		{
			if(i==0&&j==0)
			{
				dp[i][j]=0;
				continue;
			}
			dp[i][j]=1000000000000000000LL;
			if(i)
			{
				if(dp[i-1][j]<dp[i][j])
				{
					dp[i][j]=dp[i-1][j];
				}
			}
			if(j)
			{
				int inc=a;
				if(wz[j]>i)
				{
					inc=b;
				}
				if(dp[i][j-1]+inc<dp[i][j])
				{
					dp[i][j]=dp[i][j-1]+inc;
				}
			}
			if(i&&j)
			{
				if(wz[j]==i)
				{
					if(dp[i-1][j-1]<dp[i][j])
					{
						dp[i][j]=dp[i-1][j-1];
					}
				}
			}
		}
	}
	return 0;
}