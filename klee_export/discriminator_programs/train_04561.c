#include "klee/klee.h"
#include<stdio.h>
#include<string.h>
#define int long long
char s[1000000];
int l,cnt,ans;
signed main(){
  klee_make_symbolic(&s, sizeof(s), "s");

	l=strlen(s);
	for(int i=0;i<l;i++){
		if(s[i]=='A') cnt++;
		else if(s[i]=='B'&&s[i+1]=='C'){
			i++;
			ans+=cnt;
		}else cnt=0;
	}
}
