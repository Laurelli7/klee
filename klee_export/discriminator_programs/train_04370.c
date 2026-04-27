#include "klee/klee.h"
#include <stdio.h>
#include <string.h>
#define MAX_NUM 1000000000000000000ll


long long FizzBuzz(long long X){ // X - 1までのfizzとbuzzの数を数え、その文字数分(4)をかけている
	return ((X - 1) / 3) * 4 + ((X - 1) / 5) * 4;
}

long long theOtherNum(long long x){ // x - 1 までに出てくる通常数字（fizzbuzz以外)を数えている
	return (x - 1) - ((x - 1) / 3 + (x - 1) / 5 - (x - 1) / 15);
}


long long theOther(long long x,long long X,long long keta){ // xは1, 10, 100, 1000 ...とカウントアップしていき,
											 // そして最終的にXを超えるようなxが出たときXの値までの通常数字だけの文字数を返す
											 // ※ketaはxの桁数を表す
	if(x * 10 - 1 < X){
		return (theOtherNum(x * 10) - theOtherNum(x)) * keta + theOther(x * 10,X,keta + 1);
	}else{
		return (theOtherNum(X) - theOtherNum(x)) * keta;
	}
}

long long headNumber(long long X){ // Xというあたいが与えられたときfizzbuzzを含む文字列を見てXの値（3か5かで割れる場合はfかb)が入るの先頭を返す関数
	return 1 + FizzBuzz(X) + theOther(1,X,1);
}

long long binarySearch(long long s){
	long long lb = 1, ub = MAX_NUM;
	while(ub - lb > 1){
		long long mid = (ub + lb) / 2;
		if(headNumber(mid) >= s){
			ub = mid;
		}else{
			lb = mid;
		}
	}
	return lb;
}


int main(void){
	long long s, thrgh, hn, i, j;
  klee_make_symbolic(&s, sizeof(s), "s");
	char ans[4096];
	thrgh = s - (headNumber(hn = binarySearch(s)));
	char *p = ans;
	for(i = hn;i < hn + 10;i++){
		if(i % 15 == 0){
			strcpy(p,"FizzBuzz");
			p+=8;
		}else if(i % 5 == 0){
			strcpy(p,"Buzz");
			p+=4;
		}else if(i % 3 == 0){
			strcpy(p,"Fizz");
			p+=4;
		}else{
			long long keta = 0;
			long long  tmp = i;
			do{
				keta++;
				tmp /= 10;
			}while(tmp != 0);
			tmp = i;
			for(j = 0;j < keta;j++){
				*(p + keta - 1 - j) = '0' + tmp % 10;
				tmp /= 10;
			}
			p += keta;
		}
	}
	*p = '\0';
	for(p = ans + thrgh;p != ans + thrgh + 20;p++){
	}
	return 0;
}