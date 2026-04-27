#include "klee/klee.h"
/* https://csacademy.com/submission/2759503/ (rainboy) */
#include <stdio.h>
#include <string.h>

#define N	8
#define L	5
#define N_	(1 + N * L)
#define M	(N_ * N * 3)
#define A	26
#define MD	998244353

int tt[1 + N_][A]; char word[1 + N_];
int ii[M], jj[M], ll[M], m;

void dfs(int i, int s1, int s2, int l) {
	int a;

	if (s1 == 0 || s2 == 0)
		return;
	if (l > 0) {
		if (word[s1])
			ii[m] = i, jj[m] = s2 - 1, ll[m] = l, m++;
		if (word[s2])
			ii[m] = i, jj[m] = s1 - 1, ll[m] = l, m++;
		if (word[s1] && word[s2])
			ii[m] = i, jj[m] = 0, ll[m] = l, m++;
	}
	for (a = 0; a < A; a++)
		dfs(i, tt[s1][a], tt[s2][a], l + 1);
}

void apply(int aa[][N_][L], int n) {
	static int bb[N_][N_];
	int h, i, j, k, l;

	for (i = 0; i < n; i++)
		memset(bb[i], 0, n * sizeof *bb[i]);
	for (i = 0; i < n; i++)
		for (h = 0; h < m; h++) {
			j = ii[h], k = jj[h], l = ll[h];
			if ((bb[i][k] += aa[i][j][l - 1]) >= MD)
				bb[i][k] -= MD;
		}
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			for (l = L - 1; l >= 0; l--)
				aa[i][j][l] = l == 0 ? bb[i][j] : aa[i][j][l - 1];
}

void mult(int aa[][N_][L], int bb[][N_][L], int cc[][N_][L], int n) {
	static int aa_[N_][N_][L], bb_[N_][N_][L + L];
	int h, i, j, k, l, l1, l2;

	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			memcpy(aa_[i][j], aa[i][j], L * sizeof *aa[i][j]);
	for (l = 0; l < L; l++)
		for (i = 0; i < n; i++)
			for (h = 0; h < m; h++) {
				j = ii[h], k = jj[h], l1 = ll[h];
				if (l + l1 < L && (aa_[i][k][l] -= aa_[i][j][l + l1]) < 0)
					aa_[i][k][l] += MD;
			}
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++) {
			memset(bb_[i][j], 0, L * sizeof *bb_[i][j]);
			memcpy(bb_[i][j] + L, bb[i][j], L * sizeof *bb[i][j]);
		}
	for (l = L - 1; l >= 0; l--)
		for (k = 0; k < n; k++)
			for (h = 0; h < m; h++) {
				i = ii[h], j = jj[h], l1 = ll[h];
				if ((bb_[i][k][l] += bb_[j][k][l + l1]) >= MD)
					bb_[i][k][l] -= MD;
			}
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			memset(cc[i][j], 0, L * sizeof *cc[i][j]);
	for (i = 0; i < n; i++)
		for (j = 0; j < n; j++)
			for (k = 0; k < n; k++)
				for (l1 = 0; l1 < L; l1++)
					for (l2 = L - l1; l1 + l2 - L < L; l2++)
						cc[i][k][l1 + l2 - L] = (cc[i][k][l1 + l2 - L] + (long long) aa_[i][j][l1] * bb_[j][k][l2]) % MD;
}

void power(int pp[][N_][L], int tt[][N_][L], int n, int k) {
	int i, j, l;

	if (k == 0) {
		for (i = 0; i < n; i++)
			for (j = 0; j < n; j++)
				for (l = 0; l < L; l++)
					pp[i][j][l] = i == j && l == 0;
		return;
	}
	power(tt, pp, n, k / 2);
	mult(tt, tt, pp, n);
	if (k & 1)
		apply(pp, n);
}

int main() {
	static int pp[N_][N_][L], tt_[N_][N_][L];
	int n, n_, k, i, s;
  klee_make_symbolic(&n, sizeof(n), "n");
	n_ = 1;
	for (i = 0; i < n; i++) {
		static char cc[L + 1];
		int l, h;
		for (h = 0, s = 1; h < l; h++) {
			int a = cc[h] - 'a';

			if (!tt[s][a])
				tt[s][a] = ++n_;
			s = tt[s][a];
		}
		word[s] = 1;
	}
	for (s = 0; s < n_; s++)
		dfs(s, 1, s + 1, 0);
	power(pp, tt_, n_, k);
	return 0;
}
