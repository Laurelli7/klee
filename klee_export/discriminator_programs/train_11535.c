#include "klee/klee.h"
#include<limits.h>
#include<stdio.h>
#include<string.h>

inline int dif(int a,int b){ return a<b?b-a:a-b; }

int h,w,i,j,l,k;
int a[81][81];
char d[81][81][12801];

int main(void){
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&h, sizeof(h), "h");
  klee_make_symbolic(&k, sizeof(k), "k");

  l=(h+w-1)*80;
  for(i=0;i<h;i++){
    for(j=0;j<w;j++){
    }
  }
  for(i=0;i<h;i++){
    for(j=0;j<w;j++){
      a[i][j]=dif(k,a[i][j]);
    }
  }
  d[0][0][a[0][0]]=1;
  for(i=0;i<h;i++){
    for(j=0;j<w;j++){
      for(k=0;k<=l;k++){
        if(d[i][j][k]==1){
          d[i+1][j][k+a[i+1][j]]=1;
          d[i+1][j][dif(k,a[i+1][j])]=1;
          d[i][j+1][k+a[i][j+1]]=1;
          d[i][j+1][dif(k,a[i][j+1])]=1;
        }
      }
    }
  }
  for(k=0;k<=l;k++){
    if(d[h-1][w-1][k]==1)break;
  }
  return 0;
}

    