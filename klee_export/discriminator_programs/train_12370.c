#include "klee/klee.h"
#include<stdio.h>
int main(){
	int n,a[36],i,k;
  klee_make_symbolic(&n, sizeof(n), "n");
	if(n==0){
		return 0;
	}
	for(i=0;;i++){
		if(n==1){
			a[i]=1;
			for(k=i;k>=0;k--){
			}
			return 0;
		}
		if(n%2!=0){
			a[i]=1;
			n=n-1;
		}
		else a[i]=0;
		n=n/-2;
	}
}