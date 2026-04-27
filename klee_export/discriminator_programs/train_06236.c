#include "klee/klee.h"
#include<stdio.h>
int main(void)
{
	int n,s,ss,z,z1,z2,z3,z4;
  klee_make_symbolic(&n, sizeof(n), "n");
	while(n!=0){
	ss=1000-n;
	s=ss/500;
	z=ss%500;
	s+=z/100;
	z1=z%100;
	s+=z1/50;
	z2=z1%50;
	s+=z2/10;
	z3=z2%10;
	s+=z3/5;
	z4=z3%5;
	s+=z4/1;
	}
	return 0;
}