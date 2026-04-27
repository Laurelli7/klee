#include "klee/klee.h"
#include<stdio.h>
int main(void)
{
	int m,n,i,j,x;
  klee_make_symbolic(&n, sizeof(n), "n");
	int kyori[100001],kyori2[100001];
  klee_make_symbolic(&kyori, sizeof(kyori), "kyori");
	int nittei[100000];
  klee_make_symbolic(&nittei, sizeof(nittei), "nittei");
	int sum,idou,p;
	kyori[1]=0;
	for(i=2;i<=n;i++)
	for(i=0;i<m;i++)
	sum=0;
	for(i=1;i<=n;i++){
		sum=sum+kyori[i];
		kyori2[i]=sum;
//
	}
	p=1;
	idou=0;
	for(i=0;i<m;i++){
		x=nittei[i];
//
		if(x>=0)	idou=idou+(kyori2[p+x]-kyori2[p]);
		else idou=idou+(kyori2[p]-kyori2[p+x]);
		idou=idou%100000;
//
		p=p+x;
//
	}
	return 0;
}