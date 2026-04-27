#include "klee/klee.h"
#include<stdio.h>

int main(){

int N;
  klee_make_symbolic(&N, sizeof(N), "N");
for(int iii=0;iii<N;iii++){

int n;int ma=0,mi=0;
  klee_make_symbolic(&n, sizeof(n), "n");
int t=0;int h;
  klee_make_symbolic(&h, sizeof(h), "h");
for(int i=0;i<n;i++){if(i==0)t=h;
if(h-t>ma)ma=h-t;
if(h>0&&t-h>mi)mi=t-h;
t=h;
}
}
return 0;
}