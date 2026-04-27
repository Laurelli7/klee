#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int G[100][100];
char use_v[100];

int main(void)
{
	int v, e;
  klee_make_symbolic(&v, sizeof(v), "v");
	int ans;
	
	while (1){
		memset(G, -1, sizeof(G));
		memset(use_v, 0, sizeof(use_v));
		if (v == 0){
			break;
		}
		for (int i = 0; i < e; i++){
			int a, b, c;
  klee_make_symbolic(&a, sizeof(a), "a");
			
			G[a][b] = c / 100 - 1;
			G[b][a] = c / 100 - 1;
		}
		/*
		for (int i = 0; i < v; i++){
			for (int j = 0; j < v; j++){
			}
		}*/
		ans = 0;
		use_v[0] = 1;
		while (1){
			int i, j;
			int min;
			int min_e;
			
			for (i = 0; i < v; i++){
				if (use_v[i] == 0){
					break;
				}
			}
			if (i == v){
				break;
			}
			
			min = 100000000;
			for (i = 0; i < v; i++){
				if (use_v[i] == 1){
					for (j = 0; j < v; j++){
						if (min > G[i][j] && use_v[j] == 0){
							min = G[i][j];
							min_e = j;
						}
					}
				}
			}
			
			use_v[min_e] = 1;
			ans += min;
		}
	}
	
	return (0);
}