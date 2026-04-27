#include "klee/klee.h"
#include<stdio.h>
int i,j;
int k[10];
int ck;
int n;

int main(){
  klee_make_symbolic(&n, sizeof(n), "n");

	while(1){
		if(n==0)return 0;
		ck=0;
		while(n>=1){
			if(n%8<=3)k[ck++]=n%8;
			else if(n%8<=4)k[ck++]=n%8+1;
			else k[ck++]=n%8+2;
			n/=8;
		}
		for(i=ck-1;i>=0;i--){
		}
	}
}

			