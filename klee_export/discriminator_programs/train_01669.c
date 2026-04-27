#include "klee/klee.h"
#include <stdio.h>
int main(){
	int H,W,i,j;
  klee_make_symbolic(&H, sizeof(H), "H");
	while(1){
	if(H!=0 && W!=0){
		for(i=0;i<H;i++){
			for(j=0;j<W;j++){
				if(i==0 || j==0 || i == H-1 ||j == W-1){
				}else{
				}
			}
		}
	}else{
		break;
	}
	}
}