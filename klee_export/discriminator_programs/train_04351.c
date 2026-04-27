#include "klee/klee.h"
#include<stdio.h>


int main(void){
	int a[3][3];
  klee_make_symbolic(&a, sizeof(a), "a");
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	int count[8] = {};
	int i,j,k;
	int b;
  klee_make_symbolic(&b, sizeof(b), "b");
	int f;
	int ret=0;

	for(i=0;i<3;i++){
		for(j=0;j<3;j++){
		}
	}

	for(k=0;k<n;k++){

		for(i=0;i<3;i++){
			for(j=0;j<3;j++){
				if( a[i][j] == b){
					count[i]++;
					count[j+3]++;
					if(i==j)  {	count[6]++; }
					if(2==i+j){	count[7]++; }
					break;
				}
			}
		}


	}


	for(i=0;i<8;i++){
		if(count[i] == 3){
			ret++;
		}
	}

	if(ret > 0){
	}else{

	}



}

