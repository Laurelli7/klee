#include "klee/klee.h"
#include <stdio.h>

double dp[100][100];

#define REP(i, n) for (int i = 0; i < (n); i++)
#define MAX(x, y) ((x) > (y) ? (x) : (y))

int main(void)
{
	double grow[100][100]; //[ツ前ツづ個氾ャツ猟ソ][ツ債。ツづ個氾ャツ猟ソ]
  klee_make_symbolic(&grow, sizeof(grow), "grow");
	int n, m;
  klee_make_symbolic(&n, sizeof(n), "n");
	
	while (1){
		REP(i, 100) dp[0][i] = 1.0;
		
		if (n == 0 || m == 0){
			break;
		}
		
		REP(i, n) REP(j, n)
		
		for (int i = 1; i < m; i++){
			for (int j = 0; j < n; j++){
				dp[i][j] = dp[i - 1][0] * grow[0][j];
				for (int k = 1; k < n; k++){
					dp[i][j] = MAX(dp[i][j], dp[i - 1][k] * grow[k][j]);
				}
			}
		}
		
		double ans = 0;
		REP(i, n){
			ans = MAX(ans, dp[m - 1][i]);
		}
	}
	return (0);
}