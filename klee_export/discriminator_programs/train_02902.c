#include "klee/klee.h"
#include<stdio.h>
int main(void)
{
	int y,y2;
  klee_make_symbolic(&y, sizeof(y), "y");
	int i;
	int flg,flg2;
	flg=0;
	while(y!=0 && y2!=0){
		if(flg==0){
			flg=1;
		}
		else{
		}
		flg2=0;
		for(i=y;i<=y2;i++){
			if(y%400==0){
				flg2=1;
			}
			 if(y%4==0){
				if(y%100!=0){
				flg2=1;
				}
			}
			y++;
		}
		if(flg2==0){
		}
	}
	return 0;
}