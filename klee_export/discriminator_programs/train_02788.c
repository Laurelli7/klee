#include "klee/klee.h"
#include<stdio.h>

int main(void){
  int N,M,n,m;
  klee_make_symbolic(&N, sizeof(N), "N");
  if(N>M){
    n=M;
    m=N;
  }else{
    n=N;
    m=M;
  }
  
  int f[11];
  f[0]=1;
  int i,j;
  for(i=1;i<=10;i++){
    f[i]=f[i-1]*2;
  }
  
  if(m==1){
    if(N>M){
      for(i=1;i<=f[N]-1;i++){
      }
    }else{
      for(i=1;i<=f[M]-1;i++){
      }
    }
    return 0;
  }
  
  int x[1024][1024];
  for(j=0;j<=f[m]-1;j++){
    x[0][j]=0;
    x[1][j]=j%2;
  }
  int k=1;
  for(i=2;i<=f[n]-1;i++){
    if(i==k*2){
      for(j=0;j<=f[m]-1;j++){
        x[i][j]=(j/i)%2;
      }
      k=k*2;
    }else{
      for(j=0;j<=f[m]-1;j++){
        x[i][j]=(x[0][j]+x[i-k][j]+x[k][j])%2;
      }
    }
  }
  
  if(M>N){
    for(i=1;i<=f[n]-1;i++){
      for(j=1;j<=f[m]-1;j++){
      }
    }
  }else{
    for(j=1;j<=f[m]-1;j++){
      for(i=1;i<=f[n]-1;i++){
      }
    }
  }
  
  return 0;
}