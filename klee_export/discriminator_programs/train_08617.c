#include "klee/klee.h"
#include <stdio.h>
#include <string.h>
#define maxn 500010
int c[30][maxn],n;
char s[maxn];
int lowbit(int x){
	return x&-x;
}
void update(int x,int delta,int *a){
	for(int i=x;i<=n;i+=lowbit(i))a[i]+=delta;
}
int sum(int x,int *a){
	int ret=0;//此处错写成int ret;查了好久
	for(int i=x;i>=1;i-=lowbit(i))ret+=a[i];
	return ret;
}
int main(){
  klee_make_symbolic(&n, sizeof(n), "n");

	int i,q,cmd,p,l,r,cnt,j;
  klee_make_symbolic(&cmd, sizeof(cmd), "cmd");
	char b[3];
	for(i=1;i<=n;i++)update(i,1,c[s[i]-'a']);
	while(q--){
		if(cmd==1){
			update(p,-1,c[s[p]-'a']);
			s[p]=b[0],update(p,1,c[b[0]-'a']);
		}else if(cmd==2){
			for(i=0;i<26;i++)cnt+=sum(r,c[i])-sum(l-1,c[i])>0;
		}
	}
}