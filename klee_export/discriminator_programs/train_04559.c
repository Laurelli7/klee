#include "klee/klee.h"
#include<stdio.h>

int na,nb,nc,mina,minb,ans,a[100001],b[100001],c,x,y;

int main(){
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&b, sizeof(b), "b");
  klee_make_symbolic(&na, sizeof(na), "na");
  klee_make_symbolic(&x, sizeof(x), "x");

  mina=minb=0x7fffffff;
  for(int i=0;i<na;i++){
    if(mina>a[i]){
      mina=a[i];
    }
  }
  for(int i=0;i<nb;i++){
    if(minb>b[i])minb=b[i];
  }
  ans=mina+minb;
  for(int i=0;i<nc;i++){
    if(a[x-1]+b[y-1]-c<ans)ans=a[x-1]+b[y-1]-c;
  }
  return 0;
}