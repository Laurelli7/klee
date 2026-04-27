#include "klee/klee.h"
#pragma warning(disable:4996)
#include<stdio.h>
#define N_ 100010
int H, HP, n;
int Q2[N_], h = 1, t, C;
long long R, SS, Q[N_];
void ins(long long a, int b){
	while (t >= h && (Q[t] >= a || Q2[t] <= b)){
		if (b < Q2[t]) b = Q2[t];
		t--;
	}
	if (b > a) b = a;
	Q[++t] = a;
	Q2[t] = b;
}
long long Gap(int x){
	return Q[x] - SS;
}
int main()
{
  klee_make_symbolic(&n, sizeof(n), "n");

	int i, a, b, top = 0;
  klee_make_symbolic(&a, sizeof(a), "a");
	long long tp;
	H = HP;
	for (i = 1; i < n; i++){
		ins(HP - H + SS, b);
		while (1){
			if (H > a){
				break;
			}
			if (Gap(h) < Q2[h]){
				Q2[h] = Q[h] - SS;
				continue;
			}
			if (h < t && Q2[h] <= Q2[h+1]){
				h++;
				continue;
			}
			if (Gap(h) <= a - H || a - H >= Q2[h]){
				tp = Gap(h);
				if (tp > a - H) tp = a - H;
				C = tp / Q2[h];
				H += Q2[h] * C;
				SS += Q2[h] * C;
				R += C;
				continue;
			}
			H += Q2[h];
			SS += Q2[h];
			R++;
		}
		H -= a;
	}
}