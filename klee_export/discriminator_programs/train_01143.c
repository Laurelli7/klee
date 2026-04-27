#include "klee/klee.h"
#include<stdio.h>
int main(){
	int N,Y,b,sum;
  klee_make_symbolic(&N, sizeof(N), "N");
	for(int i = 0;i<Y;i++){
	}
	sum = 0;
	for(;;){
		N = N / 2;
		sum = sum + N;
		if(N==1)	break;
	}
}