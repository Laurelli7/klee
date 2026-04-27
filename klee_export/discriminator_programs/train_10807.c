#include "klee/klee.h"
#include<stdio.h>
int head[262144],last[524288],to[524288],cnt=0;
void add(int u,int v)
{
	cnt++;
	last[cnt]=head[u];
	head[u]=cnt;
	to[cnt]=v;
}
int val0[262144],val1[262144],dp0[262144],dp1[262144],dp[262144];
void dfs(int u,int f)
{
	val0[u]=val0[f];
	val1[u]=val1[f];
	if(u>val0[u])
	{
		val1[u]=val0[u];
		val0[u]=u;
	}
	dp0[u]=1;
	dp1[u]=1;
	for(int i=head[u];i;i=last[i])
	{
		int v=to[i];
		if(v==f)
		{
			continue;
		}
		dfs(v,u);
		if(v<val0[u])
		{
			dp0[u]+=dp0[v];
		}
		if(v<val1[u])
		{
			dp1[u]+=dp1[v];
		}
	}
}
void dfs2(int u,int f)
{
	if(u==1)
	{
		dp[u]=0;
	}
	else if(val0[u]==u)
	{
		dp[u]=dp[f]+dp1[u];
	}
	else if(val0[f]==f&&u>val1[f])
	{
		dp[u]=dp[f]+dp0[u];
	}
	else if(val0[f]==f)
	{
		dp[u]=dp[f]+dp0[u]-dp1[u];
	}
	else
	{
		dp[u]=dp[f];
	}
	for(int i=head[u];i;i=last[i])
	{
		int v=to[i];
		if(v==f)
		{
			continue;
		}
		dfs2(v,u);
	}
}
int main()
{
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	for(int i=1;i<n;i++)
	{
		int u,v;
  klee_make_symbolic(&u, sizeof(u), "u");
		add(u,v);
		add(v,u);
	}
	dfs(1,0);
	dfs2(1,0);
	for(int i=2;i<=n;i++)
	{
		if(i==n)
		{
			putchar('\n');
		}
		else
		{
			putchar(' ');
		}
	}
	return 0;
}