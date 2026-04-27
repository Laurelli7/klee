#include "klee/klee.h"
#include <stdio.h>

int main(){
int n,i,j;
  klee_make_symbolic(&n, sizeof(n), "n");
int a,b,c,f;
  klee_make_symbolic(&c, sizeof(c), "c");
for(i=0;i<n;i++){
b=0;
f=0;
for(j=1;j<10;j++){
if(a>c&&b>c)f=1;
else if(a<c&&b<c){
if(a>b)a=c;
else b=c;
}
else{
if(a>b)b=c;
else a=c;
}
}
if(f==0){
}
else if(f==1){
}
}
return 0;
}