#include "klee/klee.h"
#include<stdio.h>
int i,j,a,b,c[200],d[200],e[200],f[200],g[200],h[1000][1000],m=0,k,kf=0,fkk=0,im=-1;
int main(void)
{
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&c, sizeof(c), "c");
  klee_make_symbolic(&d, sizeof(d), "d");

	for(i=0;i<a;i++){
	}
	for(i=0;i<a;i++){
		for(j=0;j<b;j++){
			h[i][j]=-1;
		}
	}
	for(i=0;i<b;i++){
	}
	for(i=0;i<b;i++){
		if(c[0]>=d[i] && c[0]<=e[i]){
			h[0][i]=0;
		}
	}
	for(i=0;i<a-1;i++){
		for(j=0;j<b;j++){
			if(h[i][j]!=-1){
				for(k=0;k<b;k++){
					if(c[i+1]>=d[k] && c[i+1]<=e[k]){
						kf=f[j]-f[k];
						if(kf<0){
							kf*=-1;
						}
						if(kf+h[i][j]>h[i+1][k]){
							h[i+1][k]=h[i][j]+kf;
						}
					}
				}
			}
		}
	}
	for(i=0;i<a;i++){
		if(m<h[a-1][i]){
			m=h[a-1][i];
		}
	}
	return 0;
}