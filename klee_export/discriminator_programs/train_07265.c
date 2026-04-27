#include "klee/klee.h"
#include <stdio.h>
long long p = 998244353;
long long n, y, ans;
char x[200001];
char z[200001];
long long o[100000];
long long de[100000];
int d;
int che(void) {
	for (int i = 0; i < n; i++) {
		if (x[i] > z[i])return 1;
		if (x[i] < z[i])return 0;
	}
	return 1;
}
int main() {
  klee_make_symbolic(&n, sizeof(n), "n");

	for (int i = 3; i <= n; i++) {
		if (i % 2 && n % i == 0) {
			o[d] = i;
			d++;
		}
	}
	for (int i = d - 1; i >= 0; i--) {
		y = 0;
		for (int j = 0; j < n / o[i]; j++) {
			y *= 2;
			y += (x[j] - '0');
			y %= p;
			for (int k = 0; k < o[i]; k++) {
				if (k % 2 == 0)z[j + k * n / o[i]] = x[j];
				else z[j + k * n / o[i]] = '0' + ('1' - x[j]);
			}
		}
		y += che();
		y %= p;
		for (int j = d - 1; j > i; j--) {
			if (o[j] % o[i] == 0) {
				y -= de[j];
				y += p;
				y %= p;
			}
		}
		de[i] = y;
		ans += y * 2 * n / o[i];
		ans %= p;
	}
	y = 0;
	for (int i = 0; i < n; i++) {
		y *= 2;
		y += (x[i] - '0');
		y %= p;
	}
	for (int i = 0; i < d; i++) {
		y -= de[i];
		y += p;
		y %= p;
	}
	y++;
	ans += y * 2 * n;
	ans %= p;
}