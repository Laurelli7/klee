#include "klee/klee.h"
#include <stdio.h>
int main( void )
{
	int N ;
  klee_make_symbolic(&N, sizeof(N), "N");
	char s;
  klee_make_symbolic(&s, sizeof(s), "s");
	int R=0, B=0;
	for( int n = 0 ; n < N ; n++ ) {
		s == 'R' ? R++ : B++ ;
	}
	return 0 ;
}