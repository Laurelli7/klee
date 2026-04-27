#include "klee/klee.h"
#include<stdio.h>
int map[17][17][17][17];
int dp[17][17];
int p;
int gx,gy;
int ax,ay,bx,by;
int i,j,k,l;
int Tc,tc;
int main(){
  klee_make_symbolic(&Tc, sizeof(Tc), "Tc");
  klee_make_symbolic(&ax, sizeof(ax), "ax");
  klee_make_symbolic(&gx, sizeof(gx), "gx");
  klee_make_symbolic(&p, sizeof(p), "p");

  for(tc=0;tc<Tc;tc++){

    for(i=0;i<17;i++)
      for(j=0;j<17;j++)
	for(k=0;k<17;k++)
	  for(l=0;l<17;l++)
	    map[i][j][k][l]=0;
    gx++;
    gy++;
	
    for(i=0;i<p;i++){
      ax++;
      ay++;
      bx++;
      by++;
      map[ax][ay][bx][by]=1;
      map[bx][by][ax][ay]=1;
    }

    for(i=1;i<=gx;i++){
      for(j=1;j<=gy;j++){
	if(i==1&&j==1){
	  dp[i][j]=1;
	}else{
	  dp[i][j]=0;
	  if(map[i][j][i-1][j]==0)dp[i][j]+=dp[i-1][j];
	  if(map[i][j][i][j-1]==0)dp[i][j]+=dp[i][j-1];
	}
      }
    }

    if(dp[gx][gy]==0){
    }else{
    }

  }
}