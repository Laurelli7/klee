#include "klee/klee.h"
#include <stdio.h>
#define maxn 200010
int n,k,a[maxn];
int judge(int x){
	int i,cnt=0;
	for(i=1;i<=n;i++)
		cnt+=(a[i]-1+x)/x-1;
	if(cnt>k)return 0;
	else return 1;
}
int main(){
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&n, sizeof(n), "n");

	int i,l,r,mid;
	for(i=1;i<=n;i++)
	l=0,r=1000000000;
	while(l+1<r){
		mid=(l+r)/2;
		if(judge(mid))r=mid;
		else l=mid;
	}
	return 0;
}