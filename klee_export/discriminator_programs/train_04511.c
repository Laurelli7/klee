#include "klee/klee.h"
#include<stdio.h>
int main()
{
    int t;
  klee_make_symbolic(&t, sizeof(t), "t");
    for (int i = 0; i < t; i++)
    {
        int n;
  klee_make_symbolic(&n, sizeof(n), "n");
        if (n%2 == 0)
        {
            for (int j = 2; j <= n; j+=2)
            {
            }
            
        }
        else
        {
            for (int j = 2; j < n-1; j+=2)
            {
            }
        }
        
        
    }
    
}