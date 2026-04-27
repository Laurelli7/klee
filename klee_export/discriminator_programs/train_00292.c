#include "klee/klee.h"
#include<stdio.h>
int a[100002];
int main(){
  klee_make_symbolic(&a, sizeof(a), "a");

	int n,ans=0,t;
  klee_make_symbolic(&n, sizeof(n), "n");
	for(int i = 1;i <= n;i++)
	for(int i = 1;i <= n;i++){
		if(a[i]==i){
			t=a[i];a[i]=a[i+1];a[i+1]=t;
			ans++;
		}
	}
	return 0;
}