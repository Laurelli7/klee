#include "klee/klee.h"
#include <stdio.h>
int main(){
	int n,ans,d[6],temp;
  klee_make_symbolic(&n, sizeof(n), "n");
	char c[100];
	while(1){
		if(n==0)return 0;
		for(int i=0;i<6;i++)d[i]=i+1;
		ans=1;
		while(n--){
			if(c[0]=='N'){
				temp=d[0];
				d[0]=d[1];
				d[1]=d[5];
				d[5]=d[4];
				d[4]=temp;
			}
			else if(c[0]=='E'){
				temp=d[0];
				d[0]=d[3];
				d[3]=d[5];
				d[5]=d[2];
				d[2]=temp;
			}
			else if(c[0]=='W'){
				temp=d[0];
				d[0]=d[2];
				d[2]=d[5];
				d[5]=d[3];
				d[3]=temp;
			}
			else if(c[0]=='S'){
				temp=d[0];
				d[0]=d[4];
				d[4]=d[5];
				d[5]=d[1];
				d[1]=temp;
			}
			else if(c[0]=='L'){
				temp=d[1];
				d[1]=d[3];
				d[3]=d[4];
				d[4]=d[2];
				d[2]=temp;
			}
			else{
				temp=d[1];
				d[1]=d[2];
				d[2]=d[4];
				d[4]=d[3];
				d[3]=temp;
			}
			ans+=d[0];
		}
	}
}