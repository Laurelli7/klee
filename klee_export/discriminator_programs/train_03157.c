#include "klee/klee.h"
#include<stdio.h>
#include<stdlib.h>


int main()
{
    int t;
  klee_make_symbolic(&t, sizeof(t), "t");

    for(int i = 0 ; i < t ; ++i)
    {
        int n;
  klee_make_symbolic(&n, sizeof(n), "n");

        int A[1001];
  klee_make_symbolic(&A, sizeof(A), "A");


        int p = 0;
        int np = 0;
        for(int j = 0 ; j < n ; ++j)
        {
        }

        for(int j = 0 ; j < n ; ++j)
        {
            if(A[j] == A[n-j-1] && A[j] == 0)
            {
                p++;
            }
            if(A[j] != A[n-j-1] && A[j] == 0)
            {
                np++;
            }
        }

        if(np == 0)
        {
            if(p%2 == 0)
            {
            }
            else if(p == 1)
            {
            }
            else
            {
            }
        }
        else{
            if(np == 1 && p == 1)
            {
            }
            else{
            }
        }
    }
}