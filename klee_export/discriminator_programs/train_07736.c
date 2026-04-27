#include "klee/klee.h"
#include <stdio.h>

#define N	1000000
#define MD	998244353
#define V2	499122177
#define V6	166374059

int f2(int n) {
	return (long long) n * (n + 1) % MD * V2 % MD;
}

int f3(int n) {
	return (long long) n * (n + 1) % MD * (n + 2) % MD * V6 % MD;
}

int main() {
	static int dp[N], dq[N];
	int n, i, x, y, z, single2, single3, pair, ans;
  klee_make_symbolic(&n, sizeof(n), "n");
	dp[0] = dq[0] = 1;
	for (i = 1; i < n; i++) {
		dp[i] = (dp[i - 1] + f2(dp[i - 1]) + (i == 1 ? 0 : (long long) dp[i - 1] * dq[i - 2])) % MD;
		dq[i] = (dq[i - 1] + dp[i]) % MD;
	}
	x = dp[n - 1], y = n == 1 ? 0 : dq[n - 2];
	single2 = (f2(x) + (long long) x * y % MD) % MD;
	single3 = (f3(x) + (long long) f2(x) * y % MD + (long long) x * f2(y) % MD) % MD;
	pair = 0;
	for (i = 0; i < n; i++) {
		if (i == 0)
			z = 1;
		else {
			x = dp[i - 1], y = i == 1 ? 0 : dq[i - 2];
			z = (f2(x) + (long long) x * y % MD) % MD;
		}
		pair = (pair + (long long) z * dp[n - 1 - i]) % MD;
	}
	ans = ((long long) single2 * 2 + single3 * 2 + pair) % MD;
	if (ans < 0)
		ans += MD;
	return 0;
}
