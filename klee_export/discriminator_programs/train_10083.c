#include "klee/klee.h"
#define _CRT_SECURE_NO_WARNINGS
#include<stdio.h>
int main() {
long N,A,B;
  klee_make_symbolic(&N, sizeof(N), "N");
long i,a[100000];
  klee_make_symbolic(&a, sizeof(a), "a");
for(i=0;i<N;i++){
}
long sum=0;
for(i=0;i<N-1;i++){
if(A*(a[i+1]-a[i])>=B){
sum+=B;
}
else{
sum+=A*(a[i+1]-a[i]);
}
}
}
