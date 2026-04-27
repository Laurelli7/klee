#include "klee/klee.h"
#include<stdio.h>
 
int main(){
  int a, b, t;
  klee_make_symbolic(&a, sizeof(a), "a");
  while(1){
    if(a==0 && b==0)return 0;
    if(a>b){t=a;a=b;b=t;}
  }
}
