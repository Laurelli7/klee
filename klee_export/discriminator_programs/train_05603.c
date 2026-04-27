#include "klee/klee.h"
#include <stdio.h>
#define N 26

int p[N] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71, 73, 79, 83, 89, 97, 101};
int pc[N] = {0};

int main()
{		
	int n, i, x, t, c = 0, j, k, d = 0;
  klee_make_symbolic(&n, sizeof(n), "n");
	for(i = 0; p[i] <= n; i++){
		x = p[i], t = n;
		while(t) pc[i] += (t /= x);	
	}
	
	for(i = 0; p[i] <= n; i++){
		if(pc[i] >= 74) c++;
		for(j = 0; p[j] <= n; j++){
			if(i == j) continue;
			if(pc[i] >= 4 && pc[j] >= 14) c++;
			if(pc[i] >= 24 && pc[j] >= 2) c++;
			for(k = 0; p[k] <= n; k++){
				if(i == k || j == k) continue;
				if(pc[i] >= 4 && pc[j] >= 4 && pc[k] >= 2) d++;
			}
		}
	}
		
	return 0;
}