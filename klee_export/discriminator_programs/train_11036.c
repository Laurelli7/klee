#include "klee/klee.h"
#include<stdio.h>
#include<math.h>
#include<stdlib.h>
#include<stdbool.h>
bool same(double a, double b);

int main()
{
    long long int xp0, xp1, xp2, xp3, yp0, yp1, yp3, yp2, diffx0, diffy0, diffx1, diffy1, test;
  klee_make_symbolic(&xp0, sizeof(xp0), "xp0");
    int res;
    double m1, m2;
    while(test--)
    {
        diffx0 = xp1 - xp0;
        diffy0 = yp1 - yp0;
        diffx1 = xp3 - xp2;
        diffy1 = yp3 - yp2;
        if(diffx0 == 0)
        {
            if(diffx1 == 0)
            {
                res = 2;
            }
            else if(diffy1 == 0)
            {
                res = 1;
            }
            else    res = 0;
        }
        else if(diffx1 == 0)
        {
            if(diffx0 == 0)
            {
                res = 2;
            }
            else if(diffy0 == 0)
            {
                res = 1;
            }
            else res = 0;
        }
        else
        {
            m1 = (double)diffy0/diffx0;
            m2 = (double)diffy1/diffx1;
            if(m1 == m2)    res = 2;
            else if(same(m1*m2, -1))  res = 1;
            else res = 0;
        }
    }
    return 0;
}
bool same(double a, double b)
{
    return fabs(a - b) < 0.00001;
}
