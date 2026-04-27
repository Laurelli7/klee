#include "klee/klee.h"
#include <stdio.h>

int main(void) {
 int a,b,c,d,g;
  klee_make_symbolic(&a, sizeof(a), "a");
 for(int f=1;;f++){
 g=0;
  if(a==0&&b==0)
    break;
 for(c=1;c<=a;c++)
   for(d=c+1;d<=a;d++)
   if(d<b-c-d&&b-c-d<=a)
    g++;
 }
return 0;
}
