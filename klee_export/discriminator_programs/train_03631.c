#include "klee/klee.h"
#include<stdio.h>
int main(void)
{
	int n,m,c;
  klee_make_symbolic(&n, sizeof(n), "n");
	while(n!=0){
		c=0; m=n;
		while(m!=1){
			if(m%2==0) m=m/2;
			else m=m*3+1; c++;
		}
	}
	return 0;
}	