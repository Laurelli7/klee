#include "klee/klee.h"
#include <stdio.h>

#define MOD 1000000007
int n;
int dp[220000] = {1};
int memo[220000];
int main(){
  klee_make_symbolic(&n, sizeof(n), "n");

	for(int i = 1; i <= n; ++i){
		int c;
  klee_make_symbolic(&c, sizeof(c), "c");
		if(memo[c] == 0 || memo[c] == i - 1){
			dp[i] = dp[i - 1];
		}else{
			dp[i] = (dp[i - 1] + dp[memo[c]]) % MOD;
		}
		memo[c] = i;
	}
}
