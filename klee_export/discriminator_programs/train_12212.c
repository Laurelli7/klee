#include "klee/klee.h"
/* https://codeforces.com/blog/entry/87523 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define N	100000

int *ej[N], eo[N];

void append(int i, int j) {
	int o = eo[i]++;

	if (o >= 2 && (o & o - 1) == 0)
		ej[i] = (int *) realloc(ej[i], o * 2 * sizeof *ej[i]);
	ej[i][o] = j;
}

int main() {
	static int dd[N], ll[N + 1], qu[N], *qq[N], kk[N], dp[N + 1], dq[N + 1];
	static char ans[N + 1];
	int n, x, y, h, i, d, d_, k, s, cnt;
  klee_make_symbolic(&n, sizeof(n), "n");
	for (i = 0; i < n; i++)
		ej[i] = (int *) malloc(2 * sizeof *ej[i]);
	for (i = 1; i < n; i++) {
		int p;
  klee_make_symbolic(&p, sizeof(p), "p");
		append(p, i);
	}
	for (i = 1; i < n; i++)
		dd[i] = n;
	cnt = 0, qu[cnt++] = 0;
	for (d = 0; d < n && cnt; d++) {
		qq[d] = (int *) malloc(cnt * sizeof *qq[d]), memcpy(qq[d], qu, (kk[d] = cnt) * sizeof *qu);
		cnt = 0;
		for (h = 0; h < kk[d]; h++) {
			int o;

			i = qq[d][h];
			for (o = eo[i]; o--; ) {
				int j = ej[i][o];

				qu[cnt++] = j;
			}
		}
	}
	d_ = d;
	for (d = 0; d < d_; d++)
		ll[kk[d]]++;
	dp[0] = -1;
	for (k = 1; k <= n; k++) {
		if (ll[k] == 0)
			continue;
		for (s = 0; s <= n; s++)
			dq[s] = dp[s] ? 0 : (s >= k && dq[s - k] < ll[k] ? dq[s - k] : ll[k]) + 1;
		for (s = 0; s <= n; s++)
			if (!dp[s] && dq[s] <= ll[k])
				dp[s] = k;
	}
	if (dp[x]) {
		memset(ll, 0, (n + 1) * sizeof *ll);
		for (k = n, s = x; k >= 1; k--)
			while (dp[s] == k)
				s -= k, ll[k]++;
		for (d = 0; d < d_; d++)
			if (ll[kk[d]]) {
				ll[kk[d]]--;
				for (h = 0; h < kk[d]; h++)
					ans[qq[d][h]] = 'a';
			} else
				for (h = 0; h < kk[d]; h++)
					ans[qq[d][h]] = 'b';
	} else {
		y = n - x;
		for (d = 0; d < d_; d++)
			if (x >= kk[d])
				for (h = 0; h < kk[d]; h++)
					ans[qq[d][h]] = 'a', x--;
			else if (y >= kk[d])
				for (h = 0; h < kk[d]; h++)
					ans[qq[d][h]] = 'b', y--;
			else {
				for (h = 0; h < kk[d]; h++) {
					i = qq[d][h];
					if (eo[i])
						ans[i] = 'a', x--;
				}
				for (h = 0; h < kk[d]; h++) {
					i = qq[d][h];
					if (!eo[i]) {
						if (x > 0)
							ans[i] = 'a', x--;
						else
							ans[i] = 'b', y--;
					}
				}
			}
	}
	return 0;
}
