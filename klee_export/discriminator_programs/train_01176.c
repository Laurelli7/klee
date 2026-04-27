#include "klee/klee.h"
#include<stdio.h>
int main()
{
   int t;
  klee_make_symbolic(&t, sizeof(t), "t");
   while(t--){
   int n,k;
  klee_make_symbolic(&n, sizeof(n), "n");
   if(n-k+k/2!=0) 
   {
      for(int i=n;i>=k+1;i--)
      {
      }
      for(int i=k-1;i>=(k+1)/2;i--)
      {
      }





   }
   }

}