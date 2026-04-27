#include "klee/klee.h"
#include <stdio.h>
int h, w;
void solve(int a[50][50],int x,int y){
	int i, j;
	a[y][x] = 0;
	for(i = y - 1;i <= y + 1;i++){
		for(j = x - 1;j <= x + 1;j++)
			if(0 <= i && i < h && 0 <= j && j < w && a[i][j]) solve(a,j,i);
	}
	return;
}

int main(void){
  klee_make_symbolic(&w, sizeof(w), "w");

	int i, j, flag[50][50], count;
  klee_make_symbolic(&flag, sizeof(flag), "flag");
	while(w != 0 || h != 0){
		for(i = 0;i < h;i++)
			for(j = 0;j < w;j++)
		count = 0;
		for(i = 0;i < h;i++){
			for(j = 0;j < w;j++){
				if(flag[i][j]){
					count++;
					solve(flag,j,i);
				}
			}
		}
	}
	return 0;
}