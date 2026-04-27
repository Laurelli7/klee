#include "klee/klee.h"
#include <stdio.h>
int main()
{
  int a,y,t,max,m[105][105],i,j,k,l,yo,ta,cou;
  klee_make_symbolic(&yo, sizeof(yo), "yo");
  while(1){
    if(a==0)break;
    for(i=1;i<=t;i++){
      for(j=1;j<=y;j++){
	m[j][i]=0;
      }
    }
    for(i=0;i<a;i++){
      m[j][k]=1;
    }
    /////
    max=0;
    for(i=1;i<=y-yo+1;i++){
      for(j=1;j<=t-ta+1;j++){
	cou=0;
	for(k=i;k<=i+yo-1;k++){
	  for(l=j;l<=j+ta-1;l++){
	    if(m[k][l]==1)cou++;
	  }
	}
	if(max<cou)max=cou;
      }
    }
  }
  return 0;
}