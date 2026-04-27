#include "klee/klee.h"
#include<stdio.h>  

int main(void){
  long int N,X,D,a,i;
  klee_make_symbolic(&N, sizeof(N), "N");
  
  if(D==0){
    if(X!=0){
      a=N+1;
    }else{
    }
    return 0;
  }
  
  if(D<0){
    X=X+(N-1)*D;
    D=-D;
  }
  if(X==0){
    a=N*(N-1)/2+1;
    return 0;
  }
  a=(N+1)*(N*N-N+6)/6;
  long int b,c,k,j,l,r;
  b=0;
  k=1;
  while((k*X)%D!=0&&k!=N){
    k++;
  }
  if(k==N){
    return 0;
  }else{
    c=k*X/D;
    if(X>0){
      for(i=0;i<=N-k;i++){
        j=i+k;
        if(i*(2*N-i-1)/2-j*(j-1)/2-c+1>0){
          b=b+i*(2*N-i-1)/2-j*(j-1)/2-c+1;
        }
      }
    }else{
      for(i=0;i<=N-k;i++){
        j=i+k;
        if(j*(j-1)/2+c>i*(i-1)/2){
          l=j*(j-1)/2+c;
        }else{
          l=i*(i-1)/2;
        }
        if(j*(2*N-j-1)/2+c>i*(2*N-i-1)/2){
          r=i*(2*N-i-1)/2;
        }else{
          r=j*(2*N-j-1)/2+c;
        }
        if(r-l+1>0){
          b=b+r-l+1;
        }
      }
    }
  }
  
  a=a-b;
  
  return 0;
}