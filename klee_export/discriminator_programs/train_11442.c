#include "klee/klee.h"
#include<stdio.h>
int main(){
	long long n;
  klee_make_symbolic(&n, sizeof(n), "n");
	while(1){
		if(n==0)break;
		long long i;
		long long ans=0;
		long long k=n/2;
		for(i=1;i*i<k;i++){
			ans+=((k-1)/i+1)-(i+1);
		}
		ans=(ans+n/2-1)*2+i;
		ans=(ans+n)*8;
	}
	return 0;
}	