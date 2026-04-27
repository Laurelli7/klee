#include "klee/klee.h"
#include <stdio.h>
int h,w,n,x,y,f[1005][1005],dp[1005][1005];
int main(){
  klee_make_symbolic(&f, sizeof(f), "f");
  klee_make_symbolic(&h, sizeof(h), "h");

	for(;;){if(!h) return 0;
		for(int i=1;i<=h;i++)for(int j=1;j<=w;j++)
		dp[1][1]=n;
		for(int i=1;i<=h;i++)for(int j=1;j<=w;j++){
			if(i>=2) dp[i][j]+=(dp[i-1][j]+!f[i-1][j])/2;
			if(j>=2) dp[i][j]+=(dp[i][j-1]+f[i][j-1])/2;
		}
		for(x=1,y=1;x<=h&&y<=w;){ if((f[x][y]^dp[x][y])&1) x++; else y++;}
	}
}