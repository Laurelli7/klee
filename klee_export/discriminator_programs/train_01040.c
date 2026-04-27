#include "klee/klee.h"
#include<stdio.h>
int mbk(long long a,int n,int m){return n?(mbk((a*a)%m,n/2,m)*(n%2?a:1))%m:1;}
long long n,s=0,i,M=998244353,d[1000010],b2[1000010]={1};
int main(){
  klee_make_symbolic(&d, sizeof(d), "d");
  klee_make_symbolic(&n, sizeof(n), "n");

  for(i=0;i<n;i++)b2[i+1]=(b2[i]*2)%M;
  for(i=0;i<n;i++)
  for(i=0;i<n;i++){
    s=(s+(d[i]*mbk(1+d[i],i,M))%M*b2[n-i-1])%M;
    //
  }
  return 0;
}

