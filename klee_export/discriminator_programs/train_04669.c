#include "klee/klee.h"
#include<stdio.h>
int main(){
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	int max=-1,minVal;
	for(int i=0;i<n;i++){
		int r,v;
  klee_make_symbolic(&r, sizeof(r), "r");
		if(r>max){
			max=r;
			minVal=v;
		}
	}
}