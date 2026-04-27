#include "klee/klee.h"
#include<stdio.h>
int main(){
int a[10],b=0,c;
  klee_make_symbolic(&a, sizeof(a), "a");
for(int i=0;i<10;i++){
}
for(int i=0;i<3;i++){
for(int j=0;j<10;j++){
if(a[j]>b){
b=a[j];
c=j;
}
}
b=0;
a[c]=0;
}
return 0;
}