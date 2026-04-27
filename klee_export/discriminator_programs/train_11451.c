#include "klee/klee.h"
#include <stdio.h>

int main(){
	int N,M;
  klee_make_symbolic(&N, sizeof(N), "N");
	int A[1001],i;
  klee_make_symbolic(&A, sizeof(A), "A");
	for(i=1;i<=N;i++){
	}

	int max=0,min=100,iM,i_check,range_sum=0;
	for(i=1;;i++){
		for(iM=1;iM<=M;iM++){
			i_check=i+iM-1;
			if(i_check>N)i_check-=N;
			if(A[i_check]>max)max=A[i_check];
			if(A[i_check]<min)min=A[i_check];
		}

		i=i_check;
		
		range_sum=range_sum+max-min;
		max=0;
		min=100;

		if(i_check==N)break;
	}
}