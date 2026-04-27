#include "klee/klee.h"
#include<stdio.h>
int main()
{
  int N,eliminate;
  klee_make_symbolic(&N, sizeof(N), "N");
  int i,j;
  if(N%2 != 1)
  {
    eliminate = N + 1;
  }
  else
  {
    eliminate = N;
  }
  for(i=1;i<N;i++)
  {
    for(j=i+1;j<=N;j++)
    {
      if((i+j)!=eliminate)
      {
      }
    }
  }
  return 0;
}