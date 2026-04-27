#include "klee/klee.h"
#include<stdio.h>

#define rep(i,n) for(int i=0;i<n;i++)

int main(void){
	while(1){
		int n;
  klee_make_symbolic(&n, sizeof(n), "n");
		if(n==0){
			break;
		}
		int sonzai[6][6][6][6][6][6];
		rep(a,6){
			rep(b,6){
				rep(c,6){
					rep(d,6){
						rep(e,6){
							rep(f,6){
								sonzai[a][b][c][d][e][f]=0;
							}
						}
					}
				}
			}
		}
		
		int now[6];
		for(int q=0;q<n;q++){
			for(int i=0;i<6;i++){
				char str[10];
				switch(str[0]){
					case 'C':now[i]=0;break;
					case 'Y':now[i]=1;break;
					case 'R':now[i]=2;break;
					case 'M':now[i]=3;break;
					case 'G':now[i]=4;break;
					case 'B':now[i]=5;break;
				}
			}
			int temp;
			for(int i=0;i<4;i++){
				for(int j=0;j<4;j++){
					
					sonzai[now[0]][now[1]][now[2]][now[3]][now[4]][now[5]]=1;
					
					temp=now[2];
					now[2]=now[4];
					now[4]=now[3];
					now[3]=now[1];
					now[1]=temp;
				}
				temp=now[0];
				now[0]=now[2];
				now[2]=now[5];
				now[5]=now[3];
				now[3]=temp;
			}
			temp=now[0];
			now[0]=now[1];
			now[1]=now[5];
			now[5]=now[4];
			now[4]=temp;
			
			for(int j=0;j<4;j++){
					
				sonzai[now[0]][now[1]][now[2]][now[3]][now[4]][now[5]]=1;
				
				temp=now[2];
				now[2]=now[4];
				now[4]=now[3];
				now[3]=now[1];
				now[1]=temp;
			}
			
			temp=now[0];
			now[0]=now[1];
			now[1]=now[5];
			now[5]=now[4];
			now[4]=temp;
			temp=now[0];
			now[0]=now[1];
			now[1]=now[5];
			now[5]=now[4];
			now[4]=temp;
			
			for(int j=0;j<4;j++){
					
				sonzai[now[0]][now[1]][now[2]][now[3]][now[4]][now[5]]=1;
				
				temp=now[2];
				now[2]=now[4];
				now[4]=now[3];
				now[3]=now[1];
				now[1]=temp;
			}
		}
		int cont=0;
		rep(a,6){
			rep(b,6){
				rep(c,6){
					rep(d,6){
						rep(e,6){
							rep(f,6){
								cont+=sonzai[a][b][c][d][e][f];
							}
						}
					}
				}
			}
		}
	}
	return 0;
}