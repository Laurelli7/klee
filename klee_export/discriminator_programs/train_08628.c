#include "klee/klee.h"
#include<stdio.h>
#include<string.h>
#include<math.h>
int main(void)
{
	double s[1001],d,f=1000000000,g[4],h;
  klee_make_symbolic(&s, sizeof(s), "s");
	double z;
	int i,j,a;
  klee_make_symbolic(&a, sizeof(a), "a");
	for(i=0;i<a;i++){
	}
	for(i=0;i<a-1;i++){
		for(j=i+1;j<a;j++){
			if(s[i]>s[j]){
				d=s[i];
				s[i]=s[j];
				s[j]=d;
			}
		}
	}
	g[2]=-1; g[3]=-1;
	for(i=0;i<a-1;i++){
		if(s[i+1]-s[i]<f){
			f=s[i+1]-s[i];
			if(i==a-2){
				g[2]=g[0];
				g[3]=g[1];
			}
			g[0]=s[i+1];
			g[1]=s[i];
		}
	}
	if(g[2]!=-1){
		if((s[a-1]+s[a-2])/(g[2]-g[3])>(s[a-3]+s[a-4])/(g[0]-g[1])){
			g[0]=g[2];
			g[1]=g[3];
		}
		else{
			s[a-1]=s[a-3];
			s[a-2]=s[a-4];
		}
	}
	z=(s[a-1]+s[a-2])/(g[0]-g[1]);
	//
	return 0;
}