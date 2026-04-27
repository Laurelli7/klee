#include "klee/klee.h"
#include <stdio.h>
#define MN 200000
int n,m,cnt[MN+5],p[MN+5],a[MN+5],ans;
void add(int x){
	cnt[x]++;
	if(x-cnt[x]+1>0){
		if(p[x-cnt[x]+1]==0)ans--;
		p[x-cnt[x]+1]++;
	}
}
void rem(int x){
	if(x-cnt[x]+1>0){
		if(p[x-cnt[x]+1]==1)ans++;
		p[x-cnt[x]+1]--;
	}
	cnt[x]--;
}
int main(){
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&n, sizeof(n), "n");

	ans=n;
	for(int i=1;i<=n;i++){
		add(a[i]);
	}
	while(m--){
		int x,y;
  klee_make_symbolic(&x, sizeof(x), "x");
		rem(a[x]);
		add(y);
		a[x]=y;
	}
}