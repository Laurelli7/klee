#include "klee/klee.h"
#include "stdio.h"

int n,ptr;
int a[200005];
long long sum,d;
int stV[200005];
int stC[200005];


int main(){
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&n, sizeof(n), "n");

	for(int i = 0; i < n; i++){
		int c = 1;
		while(ptr>0&&stV[ptr-1]>a[i]){
			d -= stV[ptr-1]*(long long)stC[ptr-1];
			c += stC[ptr-1];
			ptr--;
		}

		d += a[i]*(long long)c;
		stC[ptr] = c;
		stV[ptr] = a[i];
		ptr++;

		sum += d;
	}
}