#include "klee/klee.h"
#include <stdio.h>
const int N=2e5+10;
int main(){
    int now=0,a[N],k,n;
  klee_make_symbolic(&k, sizeof(k), "k");
    while(n--){
        if(k==0){
            a[now++]=k;
        }
        else if(k==1){
        }
        else now--;
    }
}
