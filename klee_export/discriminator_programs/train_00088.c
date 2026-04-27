#include "klee/klee.h"
#include<stdio.h>
int n[1000001]={};
int main(){

int N;
  klee_make_symbolic(&N, sizeof(N), "N");

int c=0;
while(1){
if(N==0)break;
int c=0;

for(int i=0;i<N;i++){}

for(int i=0;i<N;i++)
for(int j=N-1;j>i;j--)
{if(n[j]<n[j-1]){int T=n[j];n[j]=n[j-1];n[j-1]=T;c++;}
  klee_make_symbolic(&n, sizeof(n), "n");


}
for(int i=0;i<N;i++)
	n[i]=0;

}
return 0;
}