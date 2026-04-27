#include "klee/klee.h"
#include <stdio.h>

int solve(int n, int array[])
{
	if(n <= 0) return 0;
	
	int rot = array[0], tmp, min = n+1;
	if(rot == 0) return solve(n-1, array+1);
	
	for(int i = 1; i <= n; i++){
		for(int j = 0; j < i; j++) array[j] = (array[j] - rot + 10) % 10;
		tmp = solve(n-1, array+1);
		for(int j = 0; j < i; j++) array[j] = (array[j] + rot) % 10;
		
		if(tmp < min) min = tmp;
	}
	return min + 1;
}


int main(void)
{
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	int init[15], goal[15];
	char init_str[15], goal_str[15];
	
	while(1){
		if(n == 0) break;
		
		for(int i = 0; i < n; i++){
			init[i] = init_str[i] - '0';
			goal[i] = goal_str[i] - '0';
		}
		
		for(int i = 0; i < n; i++){
			goal[i] = (goal[i] - init[i] + 10) % 10;
		}
		
		int min = solve(n, goal);
	}
	
	return 0;
}