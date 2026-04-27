#include "klee/klee.h"
#include <stdio.h>

int main(void){
  int n, i, j, k, l, m, o,  f[5][5], ans;
  klee_make_symbolic(&f, sizeof(f), "f");
  while(n--){
    for(i = 0;i < 5;i++){
      for(j = 0;j < 5;j++){
      }
    }
    ans = 0;
    for(i = 0;i < 5;i++){
      for(j = 0;j < 5;j++){
	for(k = 1;i + k <= 5;k++){
	  for(l = 1;j + l <= 5;l++){
	    int flag = 1;
	    for(m = 0;m < k;m++){
	      for(o = 0;o < l;o++){
		if(f[i + m][j + o] != 1){
		  flag = 0;
		}
	      }
	    }
	    if(flag && ans < k * l){
	      ans = k * l;
	    }
	  }
	}
      }
    }
  }
  return 0;
}