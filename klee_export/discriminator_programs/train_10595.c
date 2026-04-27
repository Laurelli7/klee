#include "klee/klee.h"
#include <stdio.h>
#include <string.h>
#define MOD 998244353

char x[310];
int next[310];
int p[5];
long long int check[310][310][310];
long long int func(int k, int s, int t)
{
	if(k==0) return 1;
	else if(k<0) return 0;
	if(check[k][s][t]!=-1) return check[k][s][t];
	
	//s>0 : 1, s<0 : 0
	long long int s1 = 0, s2 = 0;
	if(x[k]=='0')
	{
		s1 = func(k-1,s,t);
		if(s>=1) s2 = func(k,s-1,t);
	}
	else
	{
		if(t>= (k-next[k])) s1 = func(next[k]-1,s + (k-next[k]), t - (k-next[k]));
		s2 = func(k-1,s,t);
	}
	return check[k][s][t] = (s1+s2)%MOD;
}

int main()
{
	int a,b;
  klee_make_symbolic(&b, sizeof(b), "b");
	a = strlen(x+1);
	if(b>=300) b = 300;
	for(int i=0;i<=a;i++) for(int j=0;j<=b;j++) for(int k=0;k<=b;k++) check[i][j][k] = -1;
	
	for(int i=1;i<=a;i++)
	{
		int k = x[i]-'0';
		next[i] = p[1-k];
		p[k] = i;
	}
	//n k 
}