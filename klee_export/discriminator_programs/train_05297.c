#include "klee/klee.h"
#include<stdio.h>
int main(){

int N[10001]={};
  klee_make_symbolic(&N, sizeof(N), "N");
long a=0,s=0;
int n;
  klee_make_symbolic(&n, sizeof(n), "n");

while(1){
if(n==0)break;
for(int i=0;i<n;i++)

for(int i=0;i<n;i++)
for(int j=n-1;j>i;j--)
if(N[j]<N[j-1]){int t=N[j];N[j]=N[j-1];N[j-1]=t;}

//if(n==1)s=0;
//else{
for(int i=1;i<n;i++)
{
a=a+N[i-1];s+=a;
}

//}
a=0;s=0;
for(int i=0;i<n;i++)
	N[i]=0;
}
return 0;
}