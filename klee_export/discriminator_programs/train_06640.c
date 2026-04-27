#include "klee/klee.h"
#include <stdio.h>
int main(){
	int N;
  klee_make_symbolic(&N, sizeof(N), "N");
	for(int i=0;i<N;i++){
		int t;
  klee_make_symbolic(&t, sizeof(t), "t");
		if(t%2==0&&(t%3!=0&&t%5!=0)){
			return 0;
		}
	}
}