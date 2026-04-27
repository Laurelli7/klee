#include "klee/klee.h"
#include<stdio.h>
int t[101][3];
int d[200][3];
int main(){
  klee_make_symbolic(&d, sizeof(d), "d");

	int a;
  klee_make_symbolic(&a, sizeof(a), "a");
	for(int i=0;i<a;i++)for(int j=0;j<3;j++){t[d[i][j]][j]++;}
	for(int i=0;i<a;i++){
		int ret=0;
		if(t[d[i][0]][0]==1)ret+=d[i][0];
		if(t[d[i][1]][1]==1)ret+=d[i][1];
		if(t[d[i][2]][2]==1)ret+=d[i][2];
	}
}