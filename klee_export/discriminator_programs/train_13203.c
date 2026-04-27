#include "klee/klee.h"
#include<stdio.h>

int n,x,y,t,a,b,c;

int abs(int);

int main(void){
  klee_make_symbolic(&c, sizeof(c), "c");
  klee_make_symbolic(&n, sizeof(n), "n");

	register int i;
	for(i=1;i<=n;i++){
		if(((abs(x-a)+abs(y-b))%2!=(c-t)%2)||(abs(x-a)+abs(y-b)>c-t))
			return
		x=a,y=b,t=c;
	}
	return 0;
}

int abs(int a){
	return a>0?a:-a;
}