#include "klee/klee.h"
#include<stdio.h>
#include<limits.h>
#define ll long long
ll K[20],cost,ans=LLONG_MAX,a,b,i;
ll min1(ll a,ll b){
	if(a<b)
		return a;
	else
		return b;
}
void match(ll c,ll d,ll e){
	if(c==a){
		if(d>=b){
			ans=min1(ans,cost);
		}
		return ;
	}
	if(K[c]>e){
		match(c+1,d+1,K[c]);
	}
	else{
		match(c+1,d,e);
		cost+=(e+1-K[c]);
		match(c+1,d+1,e+1);
		cost-=(e+1-K[c]);
	}
}
int main(){
  klee_make_symbolic(&K, sizeof(K), "K");
  klee_make_symbolic(&a, sizeof(a), "a");

	for(i=0;i<a;i++)
	match(1,1,K[0]);
	return 0;
} 