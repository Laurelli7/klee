#include "klee/klee.h"
#include <stdio.h>
#include<math.h>
#include <string.h>
#include <ctype.h>

int main(){
int n,i,j,a,suu1,suu2,ba,t;
  klee_make_symbolic(&n, sizeof(n), "n");
int tarou[100];
  klee_make_symbolic(&tarou, sizeof(tarou), "tarou");
int hanako[100];
while(1){
if(n==0)break;

for(i=0;i<n;i++)tarou[i]=0,hanako[i]=0;
for(i=0;i<n;i++)
for(i=0;i<n;i++){
for(j=0;j<n-1;j++){
if(tarou[j]>tarou[j+1]){
a=tarou[j];
tarou[j]=tarou[j+1];
tarou[j+1]=a;
}
}
}
j=0;
a=0;
for(i=1;i<=n*2;i++){
if(tarou[j]!=i){
hanako[a]=i;
a++;
}
else if(tarou[j]==i)j++;
}
suu1=n;
suu2=n;
t=0;
ba=0;
while(suu2!=0&&suu1!=0){
if(t==0){
for(i=0;i<n;i++){
if(ba<tarou[i]&&tarou[i]!=0){
ba=tarou[i];
tarou[i]=0;
t=1;
suu2--;
break;
}
if(i==n-1){
t=1;
ba=0;
}
}
}
else if(t==1){
for(i=0;i<n;i++){
if(ba<hanako[i]&&hanako[i]!=0){
ba=hanako[i];
hanako[i]=0;
t=0;
suu1--;
break;
}
if(i==n-1){
t=0;
ba=0;
}
}
}
}
}
return 0;
}