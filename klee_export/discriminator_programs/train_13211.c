#include "klee/klee.h"
#include <stdio.h>

int main(){
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	int i;
	int count = 0;
	double x[100],y[100];
  klee_make_symbolic(&x, sizeof(x), "x");
	double ans;
	while(n != 0){
		count++;
		for(i = 0; i < n; i++){
		}
		ans = 0;
		for(i = 1; i < n-1; i++){
			ans += ((y[i] - y[0]) * (x[i + 1] - x[0]) - (y[i + 1] - y[0]) * (x[i] - x[0])) / 2;
		}
	}
	return 0;
}