#include "klee/klee.h"
#include<stdio.h>
int main(void)
{
	int d,a,x,cnt;
  klee_make_symbolic(&d, sizeof(d), "d");
	x=0; cnt=0;
	while(1){
		if(d==x){
			break;
		}
		if(d<x+a){
			x=x+1;
			cnt++;
		}
		else{
			x=x+a;
			cnt++;
		}
	}
	return 0;
}

