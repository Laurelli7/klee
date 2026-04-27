#include "klee/klee.h"
#include <stdio.h>

int sum1to2(int* a,int x,int y){
  int ans=0;
  for(int i=x;i<=y;i++){
    ans+=a[i];
  }
  return ans;
}
int main(){
  int N;
  klee_make_symbolic(&N, sizeof(N), "N");
  int lt[N];
  int lv[N+1];
  for(int i=0;i<N;i++){
  }
  for(int i=0;i<N;i++){
  }
  lv[N]=0;
  double x=0;
  double v=0;
  double pv=0;
  double t;
  double dt=0.5;
  int flag=0;
  for(int i=0;i<N;i++){
    t=0;
    	if(v>lv[i]){
	  v=(double)lv[i];
	  }
    while(t<lt[i]){
      flag=0;
      pv=v;
      for(int j=i;j<N;j++){
	if(sum1to2(lt,i,j)-t<=v-lv[j+1]){
	  flag=1;
	}
      }
      if(flag==1){//残り時間で下げられる速度が下げなきゃいけない速度より小さい
	v-=1*dt;
      }else if(v<lv[i]){
	v+=1*dt;
	if(v>lv[i]){
	  v=(double)lv[i];
	  }
      }
      x+=(v+pv)/2.0*dt;
      t+=dt;
    }
  }
  return 0;
}
