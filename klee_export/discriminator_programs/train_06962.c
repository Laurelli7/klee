#include "klee/klee.h"
#include<stdio.h>

int n,i;
int dp[31];
int main(){
  klee_make_symbolic(&n, sizeof(n), "n");

  dp[0]=1;
  dp[1]=1;
  dp[2]=2;
  dp[3]=4;
  for(i=4;i<=30;i++){
    dp[i]=dp[i-1]*2-dp[i-4];
  }
  
  while(1){
    if(n==0)break;
  }
  return 0;
}