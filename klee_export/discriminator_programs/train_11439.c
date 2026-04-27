#include "klee/klee.h"
#include<stdio.h>
int d[10][10];
int t[10][10];
int n;
int i,j;
int res;

int main(){
  klee_make_symbolic(&d, sizeof(d), "d");
  klee_make_symbolic(&n, sizeof(n), "n");

	while(1){
	if(n==0)return 0;
	for(i=0;i<n;i++)for(j=0;j<n;j++){
		t[i][j]=0;
	}
	for(i=0;i<n;i++){
		res=0;
		for(j=0;j<n;j++){
			res+=d[i][j];
		}
	}
	for(i=0;i<n;i++){
		res=0;
		for(j=0;j<n;j++){
			res+=d[j][i];
		}
	}
	res=0;
	for(i=0;i<n;i++)for(j=0;j<n;j++){
		res+=d[i][j];
	}
	}
}

		