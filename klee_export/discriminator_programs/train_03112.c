#include "klee/klee.h"
#include <stdio.h>
int a[9],ans=0,used[10];
void check(){
    int temp,now;
    now=a[0]+a[2]+a[5];
    temp=now/10;
    now%=10;
    if(now!=a[8])return ;
    now=temp+a[1]+a[4];
    temp=now/10;
    now%=10;
    if(now!=a[7])return ;
    now=temp+a[3];
    temp=now/10;
    now%=10;
    if(temp>0)return ;
    if(now!=a[6])return ;
    ans++;
    return ;
}
void dfs(int now){
    if(now>=9)check();
    else{
        if(a[now]==-1){
            for(int i=1;i<10;i++)if(used[i]==0){
                used[i]=1;
                a[now]=i;
                dfs(now+1);
                used[i]=0;
            }
            a[now]=-1;
        }
        else dfs(now+1);
    }
    return ;
}
int main(){
  klee_make_symbolic(&a, sizeof(a), "a");

    for(int i=0;i<10;i++)used[i]=0;
    for(int i=0;i<9;i++){
        if(a[i]>0)used[a[i]]++;
    }
    dfs(0);
}
