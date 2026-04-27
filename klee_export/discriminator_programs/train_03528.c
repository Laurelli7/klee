#include "klee/klee.h"
#include <stdio.h>
int main(void){
  int j=0,i=0,n=0,a;
  int A[12];
  klee_make_symbolic(&A, sizeof(A), "A");
  for(i=0;i<12;i++){
  }
  for(i=0;i<12;i++){
    for(j=i+1;j<12;j++){
      if(A[i]<A[j]){
        a=A[j];
	A[j]=A[i];
	A[i]=a;

      }
    }
  }
  if(A[0]==A[3]&&A[4]==A[7]&&A[8]==A[11]){
  }else{
  }
  return 0;
}
    

