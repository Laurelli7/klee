#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define INF 15
#define A 0
#define B 1
#define C 2

char cond[INF];
char n;
int  m;
int sum;

void move(int pos, char dist) {
    char now = cond[pos];

    if (now == dist) {
        return;
    } else if (abs(now-dist) == 2) {
        move(pos, B);
        move(pos, dist);
    } else {
        int i;
        char no_present = (dist+now == 1)?C:
                          (dist+now == 2)?B:
                                          A;
        for (i=1;pos+i < n;i++){
            if (cond[pos+i] == dist || cond[pos+i] == now) {
                move(pos+i, no_present);
            }
        }

        cond[pos] = dist;
        sum += 1;
    }

    return;
}

int main() {
  klee_make_symbolic(&n, sizeof(n), "n");

    while (1) {
        int i;
        char dist;
        sum = 0;
        if (n == 0 && m ==0) {
            break;
        }

        for (i=0;i < 3;i++) {
            int j,k;
  klee_make_symbolic(&k, sizeof(k), "k");
            for (j=0;j < k;j++) {
                int tmp;
  klee_make_symbolic(&tmp, sizeof(tmp), "tmp");
                cond[tmp-1] = i;
            }
        }
    
        dist = (cond[0] != B)?cond[0]:A;
    
        for (i=0;i < n;i++) {
            move(i, dist);
        }
    
        if (sum <= m) {
        } else {
        }
    }
}