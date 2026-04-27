#include "klee/klee.h"
#include<stdio.h>
#include<math.h>
#define N 100000000
#define M 100005
int main()
{
	int n;
  klee_make_symbolic(&n, sizeof(n), "n");
	double x[M],y[M];
  klee_make_symbolic(&x, sizeof(x), "x");
	char c[M];
	double minS=-1,S1,S2,l=0,r=N;
	double minl1=N,maxr1=-N,maxu1=-N,mind1=N,xx1,yy1,move1=0;
	double minl2=N,maxr2=-N,maxu2=-N,mind2=N,xx2,yy2,move2=N;
    for(int i=0;i<n;i++)
    for(int k=0;k<=10000&&(move2-move1>0.00000000001);k++)
	{
		minl1=N,maxr1=-N,maxu1=-N,mind1=N;
		minl2=N,maxr2=-N,maxu2=-N,mind2=N;
        move1=(r/3+2*l/3),move2=(2*r/3+l/3);
        for(int i=0;i<n;i++)
		{
	        xx1=xx2=x[i];
			yy1=yy2=y[i];
			if(c[i]=='U')
			{
				yy1+=move1;
				yy2+=move2;
			}
			if(c[i]=='D')
			{
				yy1-=move1;
				yy2-=move2;
			}
			if(c[i]=='L')
			{
				xx1-=move1;
				xx2-=move2;
			}
			if(c[i]=='R')
			{
				xx1+=move1;
				xx2+=move2;
			}
	        if(xx1<minl1)minl1=xx1;
	        if(xx1>maxr1)maxr1=xx1;
	        if(yy1<mind1)mind1=yy1;
	        if(yy1>maxu1)maxu1=yy1;
	        if(xx2<minl2)minl2=xx2;
	        if(xx2>maxr2)maxr2=xx2;
	        if(yy2<mind2)mind2=yy2;
	        if(yy2>maxu2)maxu2=yy2;
	    }
	    S1=(maxr1-minl1)*(maxu1-mind1);
	    S2=(maxr2-minl2)*(maxu2-mind2);
        if(S1<minS||minS==-1)minS=S1;
        if(S2<minS||minS==-1)minS=S2;
        if(S1>S2)l=move1;
        else r=move2;
    }
}