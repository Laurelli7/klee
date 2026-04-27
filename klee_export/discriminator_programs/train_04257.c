#include "klee/klee.h"
#include <stdio.h>
int a[9][9];
void check(int x,int y){
    for(int i=0;i<9;i++){
        if(a[i][y]==a[x][y]&&i!=x){
            return;
        }
    }
    for(int i=0;i<9;i++){
        if(a[x][i]==a[x][y]&&i!=y){
            return;
        }
    }
    for(int i=x-x%3;i<x-x%3+3;i++){
        for(int j=y-y%3;j<y-y%3+3;j++){
            if(a[i][j]==a[x][y]&&(i!=x||j!=y)){
                return;
            }
        }
    }
    return;
}
int main(){
  klee_make_symbolic(&a, sizeof(a), "a");

    int n;
  klee_make_symbolic(&n, sizeof(n), "n");
    for(int i=0;i<n;i++){
        if(i>0)
        for(int j=0;j<9;j++){
            for(int k=0;k<9;k++){
            }
        }
        for(int j=0;j<9;j++){
            for(int k=0;k<9;k++){
                check(j,k);
            }
        }
    }
}