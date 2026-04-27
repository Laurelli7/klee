#include "klee/klee.h"
#include<stdio.h>
#include<math.h>
char s1[1001];
char s2[1001];
 
int main()
{
    int n,i,j,k;
  klee_make_symbolic(&n, sizeof(n), "n");
    int t,d;
  klee_make_symbolic(&d, sizeof(d), "d");
    int o1=0;
    int o2=0;
    for(i=1;i<=n;i++)
    {
    	for(int k=0;k<=d-1;k++)
    	{
    		if(s1[k]>s2[k])
    		o1++;
			else if(s1[k]<s2[k])
			o2++; 
 		}
		if(o1>o2)
		if(o1==o2)
		if(o1<o2)
		o1=0;
		o2=0;
	}
	return 0;
 } 