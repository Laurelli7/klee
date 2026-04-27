#include "klee/klee.h"
#include<stdio.h>
int main(void)
{
		int n,t,i,k,j,g,s,r,a;
  klee_make_symbolic(&t, sizeof(t), "t");
	while(1)	{
		if(t==0)	break;
		g=0;
		for(i=0;i<n;i++){
			k=s-a;
			g+=k;
		}
		if(g>=t){
		}
		if(g<t){
			r=t-g;
		}
		k=0;	
	}	
	return 0;
}	