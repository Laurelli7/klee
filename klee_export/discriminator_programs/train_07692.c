#include "klee/klee.h"
#include <stdio.h>

char S[55][55];

int main() {
  int N, M, i, j, k, c, cnt, row;
  klee_make_symbolic(&N, sizeof(N), "N");
  for (i = 0; i < N; i++) {
  }
  int ans = 999999;
  for (i = 1; i <= N-2; i++) {
    for (j = 1; i+j <= N-1; j++) {
      cnt = 0;
      row = 0;
      for (k = 0; k < i; k++) {
        for (c = 0; c < M; c++) {
          if (S[row][c] != 'W') cnt++;
        }
        row++;
      }
      for (k = 0; k < j; k++) {
        for (c = 0; c < M; c++) {
          if (S[row][c] != 'B') cnt++;
        }
        row++;
      }
      for (k = 0; k < N-i-j; k++) {
        for (c = 0; c < M; c++) {
          if (S[row][c] != 'R') cnt++;
        }
        row++;
      }
      if (cnt < ans) ans = cnt;
    }
  }
  return 0;
}