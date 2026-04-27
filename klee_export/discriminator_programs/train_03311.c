#include "klee/klee.h"
#include "stdio.h"
int main ()
{
	int r,g,b,n;
  klee_make_symbolic(&r, sizeof(r), "r");
	int i=0;
	for (int j=0;j<=n/r;j++)
	  for (int x=0;x<=n/g;x++)
	    { 
	      if (n-j*r-x*g>=0&&(n-j*r-x*g)%b==0)
	        i++;
		}
	
	return 0;
}