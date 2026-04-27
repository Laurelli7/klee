#include "klee/klee.h"
#include<stdio.h>
#define fo(i,a,b) for(int i=a;i<=b;i++)
#define fd(i,a,b) for(int i=a;i>=b;i--)
const int p=1e9+7;
int n;
long long f[110][4][4][4],ans;
int main(){
  klee_make_symbolic(&n, sizeof(n), "n");

	fo(a,0,3)
		fo(b,0,3)
			fo(c,0,3){
				if (a==0&&b==1&&c==2) continue;
				if (a==1&&b==0&&c==2) continue;
				if (a==0&&b==2&&c==1) continue;
				f[3][a][b][c]++;
			}
	fo(i,3,n-1)
		fo(a,0,3)
			fo(b,0,3)
				fo(c,0,3)
					fo(d,0,3){
						if (b==0&&c==1&&d==2) continue;
						if (b==1&&c==0&&d==2) continue;
						if (b==0&&c==2&&d==1) continue;
						if (a==0&&c==1&&d==2) continue;
						if (a==0&&b==1&&d==2) continue;
						f[i+1][b][c][d]+=f[i][a][b][c];
						if (f[i+1][b][c][d]>=p) f[i+1][b][c][d]-=p;						
					}
	fo(a,0,3)
		fo(b,0,3)
			fo(c,0,3)
				ans+=f[n][a][b][c];
	ans%=p;
	return 0;
}