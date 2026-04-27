#include "klee/klee.h"
#include<stdio.h>
int main(){
	int n,m,dist[15][15],sum=0,i,j,k,a;
  klee_make_symbolic(&dist, sizeof(dist), "dist");
	for(i=0;i<10;i++){
		for(j=0;j<10;j++){
		}
	}
	for(k=0;k<10;k++){
		for(i=0;i<10;i++){
			for(j=0;j<10;j++){
				if(dist[i][k]+dist[k][j]<dist[i][j])
				dist[i][j]=dist[i][k]+dist[k][j];
			}
		}
	}
	for(i=0;i<n;i++){
		for(j=0;j<m;j++){
			if(a!=-1){
				sum+=dist[a][1];
			}
		}
	}
	return 0;
}