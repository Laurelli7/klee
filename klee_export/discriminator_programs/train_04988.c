#include "klee/klee.h"
#include <stdio.h>
//#include <windows.h>

int ans = 0;
int x[4],y[4];
int depth = 0;

int put_map(int m[4][4],int current){
	
	if(depth == 8){
		ans++;
		return 0;
	}

	int i,j,k,a,b,pointer;
	int next1,next2;
	int copy[4][4];

	for(pointer = current; pointer < 15; pointer++){
		i = pointer / 4;
		j = pointer % 4;

		if(m[i][j] == 0){
			for(k = 0; k < 4; k++){
				next1 = i + x[k];
				next2 = j + y[k];
				if(0 <= next1 && next1 <= 3 && 0 <= next2 && next2 <= 3){
					if(m[next1][next2] == 0){
						for(a = 0; a < 4; a++){
							for(b = 0; b < 4; b++){
								copy[a][b] = m[a][b];
							}
						}
						copy[i][j] = 1;
						copy[next1][next2] = 1;
						depth++;
						
					//	for(a = 0; a < 4; a++){
					//		for(b = 0; b < 4; b++){
					//
					//		}
					//
					//	}

						put_map(copy,pointer);

						depth--;
						copy[i][j] = 0;
						copy[next1][next2] = 0;
					}
				}
				next1 = i - x[k];
				next2 = j - y[k];
				if(0 <= next1 && next1 <= 3 && 0 <= next2 && next2 <= 3){
					if(m[next1][next2] == 0){
						for(a = 0; a < 4; a++){
							for(b = 0; b < 4; b++){
								copy[a][b] = m[a][b];
							}
						}
						copy[i][j] = 1;
						copy[next1][next2] = 1;
						depth++;
						
						// for(a = 0; a < 4; a++){
						// 	for(b = 0; b < 4; b++){
						//
						// 	}
						//
						// }

						put_map(copy,pointer);

						depth--;
						copy[i][j] = 0;
						copy[next1][next2] = 0;
					}
				}
			}
			return 0;
		}
	}
	return 0;
}

int main()
{
  klee_make_symbolic(&x, sizeof(x), "x");
  klee_make_symbolic(&y, sizeof(y), "y");

	int map[4][4];
	int i,j;
	while(x[0] < 4){
		for(i = 1; i < 4; i++){ // x,yの読み取り
		}

		ans = 0; // 解の個数を0に戻す
		depth = 0; // 探索中の深さを0に戻す

		for(i = 0; i < 4; i++){
			for(j = 0; j < 4; j++){
				map[i][j] = 0; // マップの初期化
			}
		}
		i = put_map(map,0); // 解の出力
	}
	return 0;
}