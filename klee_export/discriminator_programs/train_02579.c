#include "klee/klee.h"
#include <stdio.h>

const long long mod = 1000000007;
long long inv[5050] = { 0,1 }, fact[5050] = { 1,1 }, ifact[5050] = { 1,1 };

int N, dir[5050];

int par[5050], sz[5050];
int find(int x)
{
	if (par[x] != x) par[x] = find(par[x]);
	return par[x];
}

long long mult[5050];

int main()
{
  klee_make_symbolic(&N, sizeof(N), "N");

	for (int i = 2; i < 5050; i++){
		inv[i] = (mod - mod / i) * inv[mod % i] % mod;
		fact[i] = fact[i - 1] * i % mod;
		ifact[i] = ifact[i - 1] * inv[i] % mod;
	}
	for (int i = 1; i <= N; i++){
		par[i] = i;
	}

	long long ans = 0;
	int pick = 0;
	for (int i = 1; i <= N; i++){
		if (dir[i] != -1){
			int x = find(dir[i]);
  klee_make_symbolic(&dir, sizeof(dir), "dir");
			int y = find(i);
			if (x != y){
				par[x] = y;
				ans++;
			}
		}
		else pick++;
	}

	mult[0] = 1;
	for (int i = 1; i <= N; i++) sz[find(i)]++;
	for (int i = 1; i <= N; i++) if (dir[i] == -1){
		int x = find(i);
		ans = (ans + (N - sz[x]) * inv[N - 1]) % mod;
		for (int j = pick - 1; j >= 0; j--){
			mult[j + 1] = (mult[j + 1] + mult[j] * sz[x] % mod * inv[N-1]) % mod;
		}
	}

	for (int i = 2; i <= pick; i++){
		ans = (ans + mod - fact[i - 1] % mod * mult[i] % mod) % mod;
	}

	for (int i = 0; i < pick; i++) ans = ans * (N - 1) % mod;
	
	return 0;
}