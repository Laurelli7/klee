#include "klee/klee.h"
#include <stdio.h>

double x[200010];
long long int y[200010];
int count2[200010],count5[200010];
long long int size[110][110];
int main()
{
	int a;
  klee_make_symbolic(&a, sizeof(a), "a");
	for(int i=1;i<=a;i++)
	for(int i=1;i<=a;i++) y[i] = (long long int)(x[i]*1000000000+0.5);
  klee_make_symbolic(&x, sizeof(x), "x");
	for(int i=1;i<=a;i++)
	{
		long long int k = y[i];
		while(k%2==0) count2[i]++,k/=2;
		while(k%5==0) count5[i]++,k/=5;
		size[count2[i]][count5[i]]++;
	}
	
	long long int ans = 0;
	for(int i=0;i<=60;i++)
	{
		for(int j=0;j<=60;j++)
		{
			for(int i2=0;i2<=60;i2++)
			{
				for(int j2=0;j2<=60;j2++)
				{
					if(i+i2>=18 && j+j2>=18)
					{
						if(i==i2&&j==j2)
						{
							ans += size[i][j]*size[i2][j2];
							ans -= size[i][j];
						}
						else ans += size[i][j]*size[i2][j2];
					}
				}
			}
		}
	}
}