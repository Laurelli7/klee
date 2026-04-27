#include "klee/klee.h"
#include <stdio.h>
const int MAX = 200001;
int i, n, p[MAX], a[MAX], ans[MAX];
int cnt, min_y, cur;
int main() {
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&n, sizeof(n), "n");
  klee_make_symbolic(&p, sizeof(p), "p");

    for (i = 1; i <= n; ++i)
    min_y = n + 2, cur = 0;
    ans[0] = 1;
    for (i = 1; i <= n; ++i) {
        if (min_y > a[i]) min_y = a[i];
        if (n - i + 1 == min_y) {
            cnt = i - cur;
            while (cur < i) ans[++cur] = cnt;
        }
    }
    for (i = 1; i <= n; ++i)
    return 0;
}