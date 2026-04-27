#include "klee/klee.h"
#include <stdio.h>
int main(){
    int n,ans,temp;
  klee_make_symbolic(&temp, sizeof(temp), "temp");
    ans=n;
    while(n--){
        for(int i=1;i*i<temp;i++)if((temp-i)%(i*2+1)==0){
            ans--;
            break;
        }
    }
}
