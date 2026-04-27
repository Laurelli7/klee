#include "klee/klee.h"
#include<stdio.h>
#include<math.h>	
int main(){
int t,count;
  klee_make_symbolic(&t, sizeof(t), "t");
long num,i;
  klee_make_symbolic(&num, sizeof(num), "num");
while(t--){count=0;
	for(i=3;i*i<=2*num-1;i+=2)
		count++;
}
return 0;
}