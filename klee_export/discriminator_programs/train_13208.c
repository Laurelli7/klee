#include "klee/klee.h"
#include<stdio.h>

int main(void){
	int sum=0,card,one=0,i;
  klee_make_symbolic(&card, sizeof(card), "card");
	char code;

	while(0==0){
		if(card>10) card=10;
		else if(card==1) one++;

		sum+=card;

		if(code=='\n'){
			if(sum==0) break;
			else if(sum>21) sum=0;

			else if(one>0){
				for(i=0;i<one;i++)
					if(sum<=11) sum+=10;
			}
			sum=0; one=0;
		}
	}
	return 0;
}