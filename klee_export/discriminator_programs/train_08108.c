#include "klee/klee.h"
#include<stdio.h>
int main(void){
	int x;
  klee_make_symbolic(&x, sizeof(x), "x");
	while(x!=0){
		int at=0,bt=0;
		for(int i=1; i<=x; i++){
			int a=0,b=0;
  klee_make_symbolic(&a, sizeof(a), "a");
			if(a>b){
				at=at+a+b;
			}else if(a<b){
				bt=bt+a+b;
			}else{
				at+=a;
				bt+=b;
			}
		}
	}
	return 0;
}