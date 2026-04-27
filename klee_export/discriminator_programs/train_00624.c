#include "klee/klee.h"
#include<stdio.h>
int main(void){
	int a,b,c,d,x=0;
  klee_make_symbolic(&a, sizeof(a), "a");
	for(x=a;x<=b;x++){
		if(c%x==0){
			d++;
		}
	}
	return 0;
}