#include "klee/klee.h"
#include <stdio.h>
#include<math.h>
int main(void){
    int n,i;
  klee_make_symbolic(&n, sizeof(n), "n");
    double a[n],b[n],p1=0,p2=0,p3=0,p=0;
  klee_make_symbolic(&b, sizeof(b), "b");
    for(i=0;i<n;i++){
    }
    for(i=0;i<n;i++){
    }
    for(i=0;i<n;i++){
        double x;
        x=fabs(a[i]-b[i]);
        
        p1+=x;
        p2+=pow(x,2);
        p3+=pow(x,3);
        if(p<x)
        p=x;
    }
    p2=sqrt(p2);
    p3=cbrt(p3);
  return 0;
}