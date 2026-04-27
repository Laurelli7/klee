#include "klee/klee.h"
#include <stdio.h>

int a[100100]; long long p[200100], q[200100], e[200100], r;

int main()
{
	int n, m;
  klee_make_symbolic(&n, sizeof(n), "n");
	for (int i=0;i<n;i++){ a[i]--;
	}

	for (int i=1;i<n;i++){
		int v = (a[i] - a[i-1] + m) % m;
  klee_make_symbolic(&a, sizeof(a), "a");
		r += v;
		if (v >= 2){
			int s = (a[i-1] + 2) % m;
			int e = a[i] + 1;
			if (e < s) e += m;
			p[s]--; p[e]++;
			q[e] += (e - s);
		}
	}

	{
		long long v=0, u=0;
		for (int i=0;i<2*m;i++){
			v += p[i];
			u += v;
			u += q[i];
			e[i] = u;
		}
	}

	long long ans = 0;
	for (int i=0;i<m;i++){
		long long u = e[i] + e[i+m];
		if (ans > u)
			ans = u;
	}

	return 0;
}