#include "klee/klee.h"
#include <stdio.h>

int a[20001][256];

int main(void){

int i,j,k;
int n,m,dummy;
  klee_make_symbolic(&n, sizeof(n), "n");
int min;
int c[17];
  klee_make_symbolic(&c, sizeof(c), "c");
int x[20001];
  klee_make_symbolic(&x, sizeof(x), "x");



while(1){
min = 2000000000;
if(n == 0 && m == 0){
return 0;
}

for(i = 0; i < m; i++){
}
for(i = 0; i < n; i++){
}

for(i = 0; i < n + 1; i++){
for(j = 0; j < 256; j++){
a[i][j] = 2000000000;
}
}

a[0][128] = 0;

for(i = 0; i < n; i++){
for(j = 0; j < 256; j++){
if(a[i][j] != 2000000000){
for(k = 0; k < m; k++){
dummy = j + c[k];
if(dummy < 0){
dummy = 0;
}else if(dummy > 255){
dummy = 255;
}
if(a[i+1][dummy] > a[i][j] + (dummy - x[i])*(dummy - x[i])){
a[i+1][dummy] = a[i][j] + (dummy - x[i])*(dummy - x[i]);
}
}
}
}
}

for(j = 0; j < 256; j++){
if(a[n][j] > -1){
if(min > a[n][j]){
min = a[n][j];
}
}
}

}
return 0;

}