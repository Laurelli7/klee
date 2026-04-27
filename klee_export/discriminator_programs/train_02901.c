#include "klee/klee.h"
#include <stdio.h>

int N,M;
char S[511][511],R[511][511],B[511][511];
int main()
{
  klee_make_symbolic(&N, sizeof(N), "N");


	for (int i=0;i<N;i++)

	for (int i=0;i<N;i++) for (int j=0;j<M;j++) R[i][j] = B[i][j] = '.';

	for (int i=0;i<M;i++){
		R[0][i] = B[N-1][i] = '#';
	}

	for (int i=1;i<N-1;i++) for (int j=1;j<M-1;j++){
		if (S[i][j] == '#') R[i][j] = B[i][j] = '#';
		if (j % 2) R[i][j] = '#';
		else B[i][j] = '#';
	}

	for (int i=0;i<N;i++)
	for (int i=0;i<N;i++);

	return 0;
}