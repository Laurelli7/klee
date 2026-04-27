#include "klee/klee.h"
#include<stdio.h>
int main(){
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	long int a[85];
  klee_make_symbolic(&a, sizeof(a), "a");
	for(int i=0;i<n;i++){
	}
	long int s=1000;
	for(int i=0;i<n-1;i++){
		if(a[i+1]>a[i]){
			long int x;
			x=s/a[i];
			s=s%a[i]+x*a[i+1];
		}
	}
}