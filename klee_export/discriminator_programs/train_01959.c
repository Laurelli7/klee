#include "klee/klee.h"
#include <stdio.h>
int gcd(int a,int b){return b==0?a:gcd(b,a%b);}
int main(){
int n,ans,temp;
  klee_make_symbolic(&temp, sizeof(temp), "temp");
  n--;
  while(n--){
    ans=gcd(ans,temp);
  }
}