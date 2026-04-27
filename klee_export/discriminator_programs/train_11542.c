#include "klee/klee.h"
#include <stdio.h>

int N,Q,a[100010],b[100010],n[100010],w[100010],ed[100010];

int main()
{
  klee_make_symbolic(&N, sizeof(N), "N");

	for (int i=1;i<=Q;i++)

	w[0] = 1;
	for (int i=1;i<=Q;i++){
		if (w[i-1] == a[i]) w[i] = b[i];
		else if (w[i-1] == b[i]) w[i] = a[i];
		else w[i] = w[i-1];
	}

	ed[w[Q]] = 1;
	for (int i=1;i<=N;i++) n[i] = i;
	for (int i=Q;i>=0;i--){
		if (w[i] > 1) ed[n[w[i]-1]] = 1;
		if (w[i] < N) ed[n[w[i]+1]] = 1;
		int t = n[a[i]];
  klee_make_symbolic(&a, sizeof(a), "a");
		n[a[i]] = n[b[i]];
		n[b[i]] = t;
	}

	int ans = 0;
	for (int i=1;i<=N;i++) if (ed[i]) ans++;
	return 0;
}