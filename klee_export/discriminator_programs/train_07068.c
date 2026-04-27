#include "klee/klee.h"
#include <stdio.h>

int N;
int lights[100008];
int list_size=0;
int list[100002];
int main() {
  klee_make_symbolic(&N, sizeof(N), "N");
  klee_make_symbolic(&lights, sizeof(lights), "lights");

	int i;
	int total=0;
	int max=0;
	int sequence=0;
	for(i=0;i<N;i++)
	for(i=0;i<N;i++) {
		if(sequence>0) {
			if(lights[i]==lights[i-1]) {
				list[list_size++]=sequence;
				sequence=0;
			}
		}
		sequence++;
	}
	if(sequence>0) list[list_size++]=sequence;

	for(i=0;i<list_size;i++) {
		total+=list[i];
		if(i>2) total-=list[i-3];
		if(max<total) max=total;
	}
	return 0;
}