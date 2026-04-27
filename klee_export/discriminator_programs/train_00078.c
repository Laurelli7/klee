#include "klee/klee.h"
#include<stdio.h>
char st[15];
void out(int n,int now,int max){
	if(now>=n){
		return ;
	}
	for(int i=97;i<=max+1;i++){
		st[now]=i;
		out(n,now+1,i>max?i:max);
	}
}
int main(){
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	out(n,0,96);
	return 0;
	
}