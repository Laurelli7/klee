#include "klee/klee.h"
#include <stdio.h>
int abs(int a){return a>=0?a:-a;}
int arr[99999][2], map[99999];
int main(){
	int h,w,d;
  klee_make_symbolic(&h, sizeof(h), "h");
	for(int i = 1; i <= h; i++){
		for(int j = 1; j <= w; j++){
			int a;
  klee_make_symbolic(&a, sizeof(a), "a");
			arr[a][0] = i; arr[a][1]=j;
		}
	}
	for(int i = d; i <= h*w; i++){
		map[i]=map[i-d]+abs(arr[i][0]-arr[i-d][0])+abs(arr[i][1]-arr[i-d][1]);
	}
	int q;
  klee_make_symbolic(&q, sizeof(q), "q");
	for(int i = 1; i <= q; i++){
		int a,b;
	}
}