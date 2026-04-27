#include "klee/klee.h"
#include <stdio.h>
int n,a[10][10],now;
void go(int x,int y){
    a[x][y]=now;
    now++;
    if(x==n-1){
        if(y==n-1)return;
        if((x+y)%2==1){
            go(x,y+1);
            return;
        }
    }
    if(y==n-1){
        if((x+y)%2==0){
            go(x+1,y);
            return;
        }
    }
    if(y==0){
        if(x%2==1){
            go(x+1,y);
            return;
        }
    }
    if(x==0){
        if(y%2==0){
            go(x,y+1);
            return;
        }
    }
    if((x+y)%2==1){
        go(x+1,y-1);
        return;
    }
    go(x-1,y+1);
    return;
    
}
int main(){
  klee_make_symbolic(&n, sizeof(n), "n");

    int i=1;
    while(1){
        if(n==0)return 0;
    now=1;
    go(0,0);
        i++;
    for(int i=0;i<n;i++){
        for(int j=0;j<n;j++){
        }
    }
    }
}