#include "klee/klee.h"
#include <stdio.h>
int n;
char a[200010];
long long t[200010];
long long mod = 1e9+7;
long long ans = 1;
int main(){
  klee_make_symbolic(&n, sizeof(n), "n");

	for(int i=0;i<2*n;i++){
		if(a[i]=='B'){
			if(i>0) t[i] = t[i-1]+(t[i-1]%2?-1:1);
			else t[i] = 1;
		} else {
			if(i>0) t[i] = t[i-1]+(t[i-1]%2?1:-1);
			else t[i] = -1;
		}
	}
	for(int i=0;i<2*n;i++){
		if(t[i]<0){
			return 0;
		} else if(i>0){
			if(t[i]==t[i-1]-1){
				ans = ans * t[i-1];
				ans = ans % mod;
			}
		}
	}
	if(t[2*n-1]!=0){
		return 0;
	}
	for(long long i=1;i<=n;i++){
		ans = ans * i;
		ans = ans % mod;
	}
}