#include "klee/klee.h"
#include<stdio.h>
#define MAX 10000
#define NUL -1

struct Node{ int l,p,r; };

int D[MAX], H[MAX], n, l, r, v, i;
struct Node T[MAX];

void start(int);
int check(int);
int divide(int);

int main()
{
  klee_make_symbolic(&n, sizeof(n), "n");
  klee_make_symbolic(&v, sizeof(v), "v");


  for ( i = 0; i < n; i++ ) { T[i].p = NUL; }

  for ( i = 0; i < n; i++ ) {

    T[v].l = l;
    T[v].r = r;

    if ( l != NUL ) T[l].p = v;
    if ( r != NUL ) T[r].p = v;

  }

  for ( i = 0; i < n; i++ ){

    D[i] = check(i);
    H[i] = 0;

  }

  for ( i = 0; i < n; i++ ){
    if ( T[i].l == NUL && T[i].r == NUL ) { // leaf
      H[i] = 0;
      int p = T[i].p;
      int z = 1;
      while(1){
    if ( p == NUL ) break;
    if ( H[p] < z ) H[p] = z;
    z++;
    p = T[p].p;
      }
    }
  }

  for ( i = 0; i < n; i++ ) start(i);


  return 0;
}

int divide(int u){
  if ( T[u].p == NUL ) return NUL;
  if ( T[T[u].p].l != u && T[T[u].p].l != NUL ) return T[T[u].p].l;
  if ( T[T[u].p].r != u && T[T[u].p].r != NUL ) return T[T[u].p].r;
  return NUL;
}

int check(int u){
  int d = 0;
  while(1){
    u = T[u].p;
    if ( u == NUL ) break;
    d++;
  }
  return d;
}


void start(int u){
  int deg = 0;

  if ( T[u].l != NUL ) deg++;
  if ( T[u].r != NUL ) deg++;

  if ( T[u].p == NUL ){
  } else if ( T[u].l == NUL && T[u].r == NUL ){
  } else {
  }
}