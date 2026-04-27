#include "klee/klee.h"
#include <stdio.h>
#include <math.h>

int main(void){
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	double v[101][2];
  klee_make_symbolic(&v, sizeof(v), "v");
	double s[2];
	int ans;

	while(1){
		s[0] = 100000;
		if(n == 0) break;
		ans = n;

		for(int i=0; i<n; i++){
			if(s[0] > v[i][0]){
				s[0] = v[i][0];
				s[1] = v[i][1];
			}else if(s[0] == v[i][0] && s[1] < v[i][1]){
				s[1] = v[i][1];
			}
		}

		double t[2];
		t[0] = s[0];
		t[1] = s[1];
		double next[2];
		double bias = -M_PI/2.0;
		double min;
		double theta;
		do{
			min = 2*M_PI;
			for(int i=0; i<n; i++){
				if(v[i][0] == -10000 || v[i][1] == -10000) continue;
				if(v[i][0] == t[0] && v[i][1] == t[1]) continue;
				if(t[0] == v[i][0]){
					if(t[1] < v[i][1]) theta = M_PI/2;
					else if(t[1] > v[i][1]) theta = -M_PI/2;
				}else{
					theta = atan((t[1] - v[i][1]) / (t[0] - v[i][0]));
				}
				if(t[0] > v[i][0]) theta += M_PI;
				if(bias <= theta && min > theta){
					min = theta;
					next[0] = v[i][0];
					next[1] = v[i][1];
				}
			}
			ans--;
			t[0] = next[0];
			t[1] = next[1];
			bias = min;
			for(int i=0; i<n; i++){
				if(v[i][0] == t[0] && v[i][1] == t[1]){
					v[i][0] = -10000;
					v[i][1] = -10000;
					break;
				}
			}
		}while(t[0] != s[0] || t[1] != s[1]);
	}
	
	return 0;
}