#include "klee/klee.h"
#include<stdio.h>
int n, w[4010][4010], mod = 1000000007, P[201000][2], res, F[8010],Inv[8010],InvF[8010];
int main(){
  klee_make_symbolic(&P, sizeof(P), "P");
  klee_make_symbolic(&n, sizeof(n), "n");

    int i, j;
    F[0]=InvF[0]=Inv[1]=1;
    for(i=2;i<=8000;i++){
        Inv[i] = 1ll*Inv[mod%i]*(mod-mod/i)%mod;
    }
    for(i=1;i<=8000;i++){
        F[i]=1ll*F[i-1]*i%mod;
        InvF[i]=1ll*InvF[i-1]*Inv[i]%mod;
    }
    for(i=1;i<=n;i++){
        w[2001-P[i][0]][2001-P[i][1]]++;
        res = (res + mod - 1ll*F[(P[i][0]+P[i][1])*2]*InvF[P[i][0]*2]%mod*InvF[P[i][1]*2]%mod)%mod;
    }
    for(i=1;i<=4001;i++){
        for(j=1;j<=4001;j++){
            w[i][j]=(w[i][j]+w[i-1][j]+w[i][j-1])%mod;
        }
    }
    for(i=1;i<=n;i++){
        res = (res+w[2001+P[i][0]][2001+P[i][1]])%mod;
    }
}