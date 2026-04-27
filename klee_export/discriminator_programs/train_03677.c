#include "klee/klee.h"
/*code has 3 functions first is the scan function and one logic function and main 
function where all the operations are done and all the functions are called*/
#include <stdio.h>
/*Scan function for all the scanning variables*/
long int scan(long int n)
{
    return n;
}
/*The main logic for the problem*/
void logic(long int varunn,long int varun[varunn])
{
        int temp=0;
        for(int i=0;i<varunn-1;i++)
        {
            if(varun[i]>varun[i+1])
            {
                temp=1;
            }
        }
        if( temp==0)
        {
        }
        else if(varun[0]==1||varun[varunn-1]==varunn)
        {
        }
        else if(varun[0]==varunn&&varun[varunn-1]==1)
        {
        }
        else
        {
        }
}
int main()
{
    long int testcases;
    testcases=scan(testcases);
    while(testcases--)
    {
        long int n;
  klee_make_symbolic(&n, sizeof(n), "n");
        n=scan(n);
        long int a[n];
        for(int i=0;i<n;i++)
        {
            int k = scan(a[i]);
            a[i]=k;
        }
        logic(n,a);
    }

    return 0;
}