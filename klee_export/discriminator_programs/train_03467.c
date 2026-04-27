#include "klee/klee.h"
#include<stdio.h>

char l[1000002],r[1000002];

int smaller_or_equal(int n){
    int i;
    for(i = 0;i <= n;i++){
        if(l[i] > r[i]) return 0;
        if(l[i] < r[i]) return 1;
    }
    return 1;
}

void add_1(char num[],int n){
    int i = n;
    while(num[i] == '1'){
        num[i] = '0';
        i--;
    }
    num[i] = '1';
}

void solve(int n){
    int i;
    if(r[1] != '0' && l[1] == '0'){
        for(i = 1;i <= n;i++) r[i] = '1';
        return;
    }
    add_1(l,n);
    add_1(l,n);
    //
    //
    if(smaller_or_equal(n + 1)) r[n] = '1';
}

int main(){
    int n,n_,i;
  klee_make_symbolic(&n, sizeof(n), "n");
    //
    //
    solve(n);
    return 0;
}