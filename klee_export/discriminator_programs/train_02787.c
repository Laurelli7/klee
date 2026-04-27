#include "klee/klee.h"
#include <stdio.h>
int D;
int c[26];
int l[367][26];
int s[366][26];
int t[366];
int m, p, q, d, k;
int md[100000];
int mq[100000];
int ans;
int main() {
  klee_make_symbolic(&D, sizeof(D), "D");
  klee_make_symbolic(&m, sizeof(m), "m");

	for (int i = 0; i < 26; i++) {
	}
	for (int i = 1; i <= D; i++) {
		for (int j = 0; j < 26; j++) {
		}
	}
	for (int i = 1; i <= D; i++) {
		t[i]--;
	}
	for (int i = 0; i < m; i++) {
		mq[i]--;
	}
	for (int i = 1; i <= D; i++) {
		ans += s[i][t[i]];
		for (int j = 0; j < 26; j++) {
			l[i][j] = l[i - 1][j] + 1;
		}
		l[i][t[i]] = 0;
		for (int j = 0; j < 26; j++) {
			ans -= c[j] * l[i][j];
		}
	}
	for (int i = 0; i < m; i++) {
		q = mq[i];
		d = md[i];
		p = t[d];
		t[d] = q;
		ans -= s[d][p];
		ans += s[d][q];
		k = d;
		while (l[k][q] && k <= D) {
			ans -= c[q] * (k - d - l[k][q]);
			l[k][q] = k - d;
			k++;
		}
		k = d;
		do {
			ans -= c[p] * (l[k - 1][p] + 1 - l[k][p]);
			l[k][p] = l[k - 1][p] + 1;
			k++;
		} while (l[k][p] && k <= D);
	}
}