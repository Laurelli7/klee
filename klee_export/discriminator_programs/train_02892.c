#include "klee/klee.h"
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#define rep(i,N) for(int i=0;i<(int)N;i++)
int N,x,y,pos,tri0,tri1,tri2,pw3[13]={1},A[531441],salsa[531441],sw[200001];
char T[200001];
void OUT(int x){if(x<0)putchar('-'),x=-x;if(x>=10)OUT(x/10);putchar(x%10+48);}
char* replace(char* s,const char* before,const char* after)
{
  assert(s!=(char*)NULL);assert(before!=(char*)NULL);assert(after!=(char*)NULL);
  const size_t szb=strlen(before),sza=strlen(after);
  char* pt=s;
  if(szb==0)return s;
  for(;;)
  {
    pt=strstr(pt,before);if(pt==(char*)NULL)break;const char* remain=pt+szb;
    memmove(pt+sza,remain,strlen(remain)+1);
    memcpy(pt,after,sza);
    pt+=sza;
  }
  return s;
}
signed main(void)
{
  klee_make_symbolic(&N, sizeof(N), "N");

  x=
  replace(T+1,"SS","");if(T[0]=='\n')return 0;
  rep(i,N)pw3[i+1]=3*pw3[i];
  tri0=pw3[N],tri1=pw3[N/2],tri2=pw3[N-N/2];
  rep(i,tri0)salsa[i]=salsa[i/3]*3+(3-i%3)%3;
  rep(i,tri1)
  {
    x=i,pos=0;
    rep(j,strlen(T+1))
    {
      if(T[j+1]=='S'){x=salsa[x];if(sw[pos]>0)pos--;else sw[++pos]=1;}
      if(T[j+1]=='R'){x++;if(x==tri1)sw[++pos]=-1,x=0;}
    }
    rep(j,tri2)
    {
      y=j;
      rep(k,pos){if(sw[k+1]>0)y=salsa[y];else if(sw[k+1]<0){if((++y)==tri2)y=0;}}
      A[i+j*tri1]=y*tri1+x;
    }
  }
  rep(i,tri0)OUT(A[i]),putchar('\t');
}