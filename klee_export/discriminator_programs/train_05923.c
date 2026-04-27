#include "klee/klee.h"
#include<stdio.h>
int main(void)
{
	int q,i;
  klee_make_symbolic(&q, sizeof(q), "q");
	int c,a,n;
  klee_make_symbolic(&c, sizeof(c), "c");
	int max=0;
	for(i=0;i<q;i++){
		if(c!=0){
			while(c>=1 && a>=1 && n>=1){
				max+=1;
				c=c-1;
				a=a-1;
				n=n-1;
			}
			while(c>=2 && a>=1){
				max+=1;
				c=c-2;
				a=a-1;
			}
			while(c>=3){
				max+=1;
				c=c-3;
			}
		}
		max=0;
	}
	return 0;
}