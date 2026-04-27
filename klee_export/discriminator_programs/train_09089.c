#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

int main() {
	long long h, i, j, k;
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	char oper;
  klee_make_symbolic(&oper, sizeof(oper), "oper");
	long long s[500];
  klee_make_symbolic(&s, sizeof(s), "s");
	long long c[500];
	int max;
	long long sum;
	long long answer = 0;
	int mod = 998244353;
	for(i=0; i<n; i++) {
		if(oper == '+') {
		} else if(oper == '-') {
			s[i] = 0;
		}
	}
	for(i=0; i<n; i++) {
		if(s[i] > 0) {
			for(j=0; j<n; j++) {
				c[j] = 0;
			}
			c[0] = 1;
			max = 0;
			for(j=0; j<n; j++) {
				if(s[j] == 0) {
					if(j < i) {
						c[0] = (2*c[0])%mod;
					}
					for(k=0; k<max; k++) {
						c[k] = (c[k]+c[k+1])%mod;
					}
				} else if(s[j] < s[i] || (s[j] == s[i] && j < i)) {
					for(k=max; k>=0; k--) {
						c[k+1] = (c[k+1]+c[k])%mod;
					}
					max++;
				}
			}
			sum = 0;
			for(j=0; j<n; j++) {
				sum = (sum+c[j])%mod;
			}
			for(j=0; j<n; j++) {
				if(s[j] > s[i] || (s[j] == s[i] && j > i)) {
					sum = (2*sum)%mod;
				}
			}
			answer = (answer+s[i]*sum)%mod;
		}
	}
}