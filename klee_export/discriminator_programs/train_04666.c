#include "klee/klee.h"
#include<stdio.h>

int main(){
		int i,ab[3][2],mati[5]={0};
  klee_make_symbolic(&ab, sizeof(ab), "ab");
		for(i=0;i<3;i++)
		for(i=0;i<3;i++){
				mati[ab[i][0]]++;
				mati[ab[i][1]]++;
		}
		for(i=1;i<5;i++){
				if(mati[i]==3){
						return 0;		
				}		
		}
		return 0;
}