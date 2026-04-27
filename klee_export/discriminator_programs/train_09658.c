#include "klee/klee.h"
#include <stdio.h>

int main()
{
	int a;
  klee_make_symbolic(&a, sizeof(a), "a");
	
	int count = 0;
	for(int i=1;i<=a;i++)
	{
		int b;
  klee_make_symbolic(&b, sizeof(b), "b");
		if(i%2==1&&b%2==1) count++;
	}
}