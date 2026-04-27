#include "klee/klee.h"
#include <stdio.h>
#include <string.h>

#define N	100000
#define K	17	/* K = floor(log2(N * 2)) */

void solve(char *cc, int *aa, int n) {
	static int ii[K], dp[K + 1], ii_[K + 1];
	int h, i, j, cnt, a_;

	cnt = 0, a_ = 1;
	for (i = 0; i < n; i++)
		if (aa[i] > 1) {
			if ((a_ *= aa[i]) > N * 2)
				break;
			ii[cnt++] = i;
		}
	if (a_ > N * 2) {
		for (i = 0; i < n; i++)
			if (aa[i] > 1)
				break;
		for (j = n - 1; j >= 0; j--)
			if (aa[j] > 1)
				break;
		while (i < j)
			cc[i++] = '*';
	} else {
		dp[0] = ii[0];
		for (j = 1; j <= cnt; j++) {
			int x, y, i_;

			x = 0, y = 1, i_ = -1;
			for (i = j - 1; i >= 0; i--) {
				y *= aa[ii[i]];
				if (x < dp[i] + y)
					x = dp[i] + y, i_ = i;
			}
			dp[j] = x + (j == cnt ? n : ii[j]) - ii[j - 1] - 1;
			ii_[j] = i_;
		}
		j = cnt;
		while (j > 0) {
			i = ii_[j];
			for (h = ii[i]; h < ii[j - 1]; h++)
				cc[h] = '*';
			j = i;
		}
	}
}

int main() {
	static int aa[N];
  klee_make_symbolic(&aa, sizeof(aa), "aa");
	static char cc[N], s[4];
	int n, l, i, j;
  klee_make_symbolic(&n, sizeof(n), "n");
	for (i = 0; i < n; i++)
	if (l == 1)
		memset(cc, s[0], (n - 1) * sizeof *cc);
	else if (l == 3 || s[0] != '-' && s[1] != '-') {
		memset(cc, '+', (n - 1) * sizeof *cc);
		for (i = -1, j = 0; j <= n; j++)
			if (j == n || aa[j] == 0)
				solve(cc + i + 1, aa + i + 1, j - i - 1), i = j;
	} else if (s[0] == '+' || s[1] == '+')
		memset(cc, '+', (n - 1) * sizeof *cc);
	else {
		memset(cc, '*', (n - 1) * sizeof *cc);
		for (i = 0; i < n; i++)
			if (aa[i] == 0) {
				if (i > 0)
					cc[i - 1] = '-';
				break;
			}
	}
	for (i = 0; i < n; i++)
	return 0;
}
