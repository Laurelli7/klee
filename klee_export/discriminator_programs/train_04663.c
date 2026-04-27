#include "klee/klee.h"
#include <stdio.h>
#define LL long long
LL n,cnt;
int judge(LL k,LL m){
	while(m>=k){
		if(m%k==0)m/=k;
		else m%=k;
	}
	if(m==1)return 1;
	else return 0;
}
int main(){
  klee_make_symbolic(&n, sizeof(n), "n");

	LL i;
	if(n==2){return 0;}//注意n=2需特判
	for(i=2;i*i<=n-1;i++)//找n-1的约数
		if((n-1)%i==0){
			cnt++;
			if(i!=(n-1)/i)cnt++;
		}
	cnt++;//加上n-1本身
	for(i=2;i*i<=n;i++)
		if(n%i==0){
			if(judge(i,n))cnt++;
			if(i!=n/i&&judge(n/i,n))cnt++;
		}
	cnt++;//加上n本身
}