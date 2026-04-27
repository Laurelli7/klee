#include "klee/klee.h"
#include <stdio.h>
#define LL long long
char s[200010];
int cnt[10010];
LL ans;
int main(){
	int n,p,i,pow10=1,now=0;
  klee_make_symbolic(&n, sizeof(n), "n");
	if(p==2||p==5){
		for(i=n;i>=1;i--)//从数据的最低位开始处理
			if((s[i]-'0')%p==0)ans+=i;
	}else{//p!=2,p!=5
		cnt[0]=1;
		for(i=n;i>=1;i--){
			now=(now+(s[i]-'0')*pow10)%p;
			pow10=pow10*10%p;
			ans+=cnt[now];
			cnt[now]++;
		}
	}
	return 0;
}