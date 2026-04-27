#include "klee/klee.h"
#include <stdio.h>

int main(void){
  int m;
  klee_make_symbolic(&m, sizeof(m), "m");
  unsigned long long int money;
  klee_make_symbolic(&money, sizeof(money), "money");
  int year;
  klee_make_symbolic(&year, sizeof(year), "year");
  int n;
  klee_make_symbolic(&n, sizeof(n), "n");
  int fukuri;
  klee_make_symbolic(&fukuri, sizeof(fukuri), "fukuri");
  double nenri;
  unsigned long long int risi;
  unsigned int tesu;
  unsigned long long int tempmoney;
  unsigned long long int maxmoney;
  for(int i=0; i<m; i++){
    maxmoney = money;
    for(int j=0; j<n; j++){
      tempmoney = money;
      risi = 0;
      if(fukuri){
	for(int y=0; y<year; y++){
	  tempmoney += (tempmoney*nenri-tesu);
	}
      }else{
	for(int y=0; y<year; y++){
	  risi += tempmoney*nenri;
	  tempmoney -= tesu;
	}
	tempmoney += risi;
      }
      if(tempmoney > maxmoney){
	maxmoney = tempmoney;
      }
    }
  }
  return 0;
}