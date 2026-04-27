#include "klee/klee.h"
#include <stdio.h>

#define NMAX 1000
#define WMAX 10000

int mincost(short *, int, int);

int main(){

  int i, j, n, d, wtemp, wmin=WMAX, cost=0;
  klee_make_symbolic(&n, sizeof(n), "n");
  short w[NMAX], s[NMAX], l[NMAX];
  klee_make_symbolic(&w, sizeof(w), "w");
  char c[WMAX+1]={0}, flag[NMAX]={0};

  for (i=0; i<n; i++) {
    if (wmin > w[i]) wmin = w[i];
    c[w[i]]++;
  }

  j=0;

  for (i=0; i<=WMAX; i++) {
    if(c[i]) {
      s[j] = i;
      j++;
    }
  }

  for (i=0; i<n; i++) {
    if( flag[i]==0 ) {
      d = 0;
      flag[i]++;
      l[d] = w[i];
      d++;
      wtemp = w[i];
      while (1) {
        j = 0;
        while (s[j] != wtemp) j++;
        if(j == i) break;
        flag[j]++;
        l[d] = w[j];
        d++;
        wtemp = w[j];
      }
      cost += mincost(l, d, wmin);
    }
  }

  return 0;
}

int mincost(short *l, int d, int wmin)
{
    int i, c1=0, c2, lmin=WMAX, imin;

    for (i=0; i<d; i++) {
        c1 += l[i];
        if(lmin > l[i]) {
            lmin = l[i];
        }
    }

    c2 = c1;
    c1 += (d-2)*lmin;
    c2 += lmin + (d+1)*wmin;

    if (c1 < c2) return c1;
    else return c2;
}