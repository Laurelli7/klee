#include "klee/klee.h"
#include<stdio.h>
int main(){
long n,m,i,j,pi,pj,a,b,c,ans;
  klee_make_symbolic(&n, sizeof(n), "n");
char s[111111],t[111111];
a=n;
b=m;
c=n%m;
while(c!=0){
a=b;
b=c;
c=a%b;
}
pi=n/b;
pj=m/b;
i=0;
j=0;
while(i<n){
if(s[i]!=t[j]){
return 0;
}
i+=pi;
j+=pj;
}
ans=n*m/b;
return 0;
}
