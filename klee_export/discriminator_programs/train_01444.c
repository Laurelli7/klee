#include "klee/klee.h"
#include<stdio.h>
long long int ds(long long int n, long long int b){
	long long int s=0;
	while(n>0){
		s=s+n%b;
		n=n/b;
	}
	return s;
}

int main(){
	long long int n,s,i, b;
  klee_make_symbolic(&n, sizeof(n), "n");
	if(n-s<0) {
		return 0;
	}
	if(n-s==0){
		return  0;
	} 
	for(i=2;i*i<=n;i++){
		if(ds(n,i)==s){
			return 0;
		} 
	}
	for(;i>0;i--){
		b=(n-s)/i+1;
		if(b>1&&b>s-i&&b>i){
			if(ds(n,b)==s){
				return 0;
			}
		}
	}
	return 0;
}