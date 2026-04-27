#include "klee/klee.h"
#include<stdio.h>
int n,k,i;
int cade[100001];
int main(void){
  klee_make_symbolic(&n, sizeof(n), "n");

	while(1){
		if(n==0 && k==0){
			break;
		}
		for(i=0;i<100001;i++){
			cade[i]=0;
		}
		int a,flg=0;
  klee_make_symbolic(&a, sizeof(a), "a");
		for(i=0;i<k;i++){
			if(a==0){
				flg=1;
			}
			else {
				cade[a]=1;
			}
		}
		for(i=n;i>=1;i--){
			if(cade[i]>=1 && cade[i-1]>=1 && i-1!=0){
				cade[i-1]=cade[i]+1;
			}
		}
		for(i=1;i<n;i++){
			if(cade[i]>0 && cade[i+1]!=0){
				cade[i+1]=cade[i];
			}
		}
		int max=0;
		for(i=1;i<=n;i++){
			if(max<cade[i]){
				max=cade[i];
			}
		}
		if(flg==1){
			for(i=1;i<n;i++){
				if(cade[i]==0 && cade[i+1]>0){
					if(max<cade[i-1]+cade[i+1]){
						max=cade[i-1]+cade[i+1]+1;
					}
				}
			}
		}
	}	
	return 0;
}