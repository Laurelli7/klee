#include "klee/klee.h"
#include<stdio.h>
int main(void){
int w,h;
  klee_make_symbolic(&w, sizeof(w), "w");
for(;;){
if(w==0&&h==0)break;
else{
for(int i=0;i<w;i++){
for(int j=0;j<h;j++){
}
}
}
}
return 0;
}