#include "klee/klee.h"
#include<stdio.h>
int main(void){
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	long long ans=0,now=0;
	for (int i=1;i<=n;i++){
		int a;
  klee_make_symbolic(&a, sizeof(a), "a");
		if (a) now+=a;
		else ans+=now/2,now=0;
	}
	ans+=now/2;
}