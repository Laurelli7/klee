#include "klee/klee.h"
#include <stdio.h>
#include <math.h>
#define maxn 65
#define eps 1e-8
double x[maxn],y[maxn],c[maxn],X[65*65*2],Y[65*65*2],r[maxn];
int tot,N,K;
double dis(double x,double y,double c,double X,double Y){
	return c*sqrt((X-x)*(X-x)+(Y-y)*(Y-y));
}
void cal(double x1,double y1,double r1,double x2,double y2,double r2){//计算两圆交点坐标
	double D;//A=a*a,H=h*h,D=d*d;
	double ad,h;//ad=a*d;
	D=(x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);
	if(D==0)return;
	ad=(r1*r1+D-r2*r2)/2;//2*a*d=r1*r1+D-r2*r2
	h=D*r1*r1-ad*ad;
	if(h<0)return;//漏了此句的判定
	h=sqrt(h);
	tot++,X[tot]=x1+ad*(x2-x1)/D-(y2-y1)*h/D,Y[tot]=y1+ad*(y2-y1)/D+(x2-x1)*h/D;
	tot++,X[tot]=x1+ad*(x2-x1)/D+(y2-y1)*h/D,Y[tot]=y1+ad*(y2-y1)/D-(x2-x1)*h/D;
}
int judge(double t){
	int i,j,cnt;
	for(i=1;i<=N;i++)r[i]=t/c[i];
	tot=0;
	for(i=1;i<=N;i++){
		tot++,X[tot]=x[i],Y[tot]=y[i];//tot++,X[i]=x[i],Y[i]=y[i];
		for(j=i+1;j<=N;j++)
			cal(x[i],y[i],r[i],x[j],y[j],r[j]);
	}
	for(i=1;i<=tot;i++){
		cnt=0;
		for(j=1;j<=N;j++)
			if(dis(x[j],y[j],c[j],X[i],Y[i])<t+eps)cnt++;//浮点比大小无等于的情况，这是由浮点误差决定的。if(dis(x[j],y[j],c[j],X[i],Y[i])<t+eps)cnt++;
		if(cnt>=K)return 1;
	}
	return 0;
}
int main(){
  klee_make_symbolic(&N, sizeof(N), "N");
  klee_make_symbolic(&x, sizeof(x), "x");

	double l,r,mid;
	int i;
	for(i=1;i<=N;i++)
	l=0,r=1000010;
	for(i=1;i<=100;i++){
		mid=(l+r)/2;
		if(judge(mid))r=mid;
		else l=mid;
	}
	return 0;
}