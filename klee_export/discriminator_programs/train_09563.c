#include "klee/klee.h"
#include <stdio.h>

int N,C[86400*2];

int main()
{
  klee_make_symbolic(&N, sizeof(N), "N");

	int t = 0; while (N--){
		int a,b;
  klee_make_symbolic(&a, sizeof(a), "a");
		t = (t + a) % 86400;
		C[t]++;
		t = (t + b) % 86400;
	}

	for (int i=0;i<86400;i++) C[i+86400] = C[i];
	for (int j=1;j<86400*2;j++) C[j] += C[j-1];

	int ans = 0;
	for (int j=10800+1;j<86400*2;j++){
		int now = C[j] - C[j-10800-1];
		if (ans < now)
			ans = now;
	}

	return 0;
}