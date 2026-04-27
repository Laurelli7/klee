#include "klee/klee.h"
#include <stdio.h>
int n,k;
int main(){
  klee_make_symbolic(&n, sizeof(n), "n");

    while (n>0){
        for (int i=0;i<k;i++){
            if (--n<1) return 0;
            for (int j=i+1;j<k;j++){
                if (--n<1) return 0;
                if (--n<1) return 0;
            }
        }
    }

    return 0;
}
