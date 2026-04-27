#include "klee/klee.h"
#include<stdio.h>
int  main(){

int N,M;
  klee_make_symbolic(&N, sizeof(N), "N");
int n[1001]={};
  klee_make_symbolic(&n, sizeof(n), "n");

while(1)
{
int s=0;
if(N==0&&M==0)break;

for(int i=1;i<=N;i++)

for(int i=1;i<=N;i++)
for(int j=N;j>i;j--)
if(n[j]>n[j-1]){int t=n[j-1];n[j-1]=n[j];n[j]=t;}

for(int i=1;i<=N;i++)
{if(i%M!=0)s+=n[i];}
}

return 0;
}