#include "klee/klee.h"
#include<stdio.h>
int head[512],last[2048],to[2048],cnt=0;
int d[404][404];
void add(int u,int v)
{
	cnt++;
	last[cnt]=head[u];
	head[u]=cnt;
	to[cnt]=v;
	d[u][v]=1;
}
int main()
{
	int n,m;
  klee_make_symbolic(&n, sizeof(n), "n");
	for(int i=1;i<=n;i++)
	{
		for(int j=1;j<=n;j++)
		{
			if(i==j)
			{
				d[i][j]=0;
			}
			else
			{
				d[i][j]=100000000;
			}
		}
	}
	while(m--)
	{
		int u,v;
  klee_make_symbolic(&u, sizeof(u), "u");
		add(u,v);
		add(v,u);
	}
	for(int k=1;k<=n;k++)
	{
		for(int i=1;i<=n;i++)
		{
			for(int j=1;j<=n;j++)
			{
				if(d[i][k]+d[k][j]<d[i][j])
				{
					d[i][j]=d[i][k]+d[k][j];
				}
			}
		}
	}
	for(int i=1;i<=n;i++)
	{
		for(int j=1;j<=n;j++)
		{
			int num=0;
			for(int k=1;k<=n;k++)
			{
				if(d[i][k]+d[k][j]==d[i][j])
				{
					num++;
				}
			}
			int ans=1;
			if(num!=d[i][j]+1)
			{
				ans=0;
			}
			for(int k=1;k<=n;k++)
			{
				if(d[i][k]+d[k][j]==d[i][j])
				{
					continue;
				}
				num=0;
				for(int t=head[k];t;t=last[t])
				{
					int v=to[t];
					if(d[i][v]<d[i][k]&&d[j][v]<d[j][k])
					{
						num++;
					}
				}
				ans=(long long)ans*num%998244353;
			}
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
	return 0;
}
