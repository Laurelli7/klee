#include "klee/klee.h"
#include <stdio.h>
int arr[31];
int main(){
	int N,M;
  klee_make_symbolic(&N, sizeof(N), "N");
	for(int i=0;i<N;i++){
		int num;
  klee_make_symbolic(&num, sizeof(num), "num");
		for(int j=0;j<num;j++){
			int tmp;
  klee_make_symbolic(&tmp, sizeof(tmp), "tmp");
			arr[tmp]++;
		}
	}
	int cnt=0;
	for(int i=0;i<=M;i++) cnt+=arr[i]==N;
}