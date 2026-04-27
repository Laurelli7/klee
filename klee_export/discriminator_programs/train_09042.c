#include "klee/klee.h"
#include<stdio.h>
int main(){
  int i,j,n,a[15][15],x,y;
  klee_make_symbolic(&n, sizeof(n), "n");
  while(1){
    for(i=0;i<15;i++)
      for(j=0;j<15;j++)
	a[i][j]=0;
    if(n==0)break;
    a[n/2+1][n/2]=1;
    x=n/2;
    y=n/2+1;
    for(i=2;i<=n*n;i++){
      x++;
      y++;
      if(x>n-1){
	x=0;

      }
      if(y>n-1){
	y=0;

      }
      if(a[y][x]!=0){
	y++;
	x--;
	if(x<0){
	  x=n-1;

	}
	if(y>n-1){
	  y=0;

	}
      }
      a[y][x]=i;
    }
    for(i=0;i<n;i++){
      for(j=0;j<n;j++){
      }
    }
  }
  return 0;
}