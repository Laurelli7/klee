#include "klee/klee.h"
#include<stdio.h>
int isPrime(int a){
	int i;
	if(a==1) return 0;
	if(a==2) return 1;
	if(a%2==0) return 0;
	for(i=3;i*i<=a;i=i+2){
		if(a%i==0) return 0;
	}
	return 1;
}

int main(){
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	while(isPrime(n)==0) n++;
}