#include "klee/klee.h"
#include<stdio.h>
int main(void)
{
	int i,j,n,m;
  klee_make_symbolic(&m, sizeof(m), "m");
	char s[100],t[100];
  klee_make_symbolic(&s, sizeof(s), "s");
	char a;
  klee_make_symbolic(&a, sizeof(a), "a");
	while (n!=0) {
		for (i=0;i<n;i++) {
		}
		for (i=0;i<m;i++) {
			for (j=0;j<n;j++) {
				if(a == s[j]) {
					a=t[j];
					break;
				}
			}
		}
	}
	return 0;
}