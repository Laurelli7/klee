#include "klee/klee.h"
#include<stdio.h>

int main(){
  int X,n;
  klee_make_symbolic(&X, sizeof(X), "X");
  for(n=1;n<=1000;n++)
  if(n*100 <= X && X <= n*105){
    return 0;
  }
}