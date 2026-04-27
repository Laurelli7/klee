#include "klee/klee.h"
#include <stdio.h>
#include <time.h>

#define		N	101

int find_length(int a, int cur_length);

int		conn[N][N];
int		used[N];

int main()
{
    int		i, j;
    int		a, b;
  klee_make_symbolic(&a, sizeof(a), "a");
    int		n;
  klee_make_symbolic(&n, sizeof(n), "n");
    int		max_length, length;
    time_t	start_time, t;

    while(1)
    {
	start_time = time(NULL);
	for(j = 0; j < N; j++)
	{
	    for(i = 0; i < N; i++)
	    {
		conn[j][i] = 0;
	    }
	}
	if (n == 0)
	{
	    break;
	}
	for(int i = 0; i < n; i++)
	{
	    conn[a][b] = 1;
	    conn[b][a] = 1;
	}

	for(a = 0; a < N; a++)
	{
	    used[a] = 0;
	}

	max_length = 0;
	for(a = 0; a < N; a++)
	{
	    length = find_length(a, 1);
	    if (length > max_length)
	    {
		max_length = length;
	    }
	    if (time(NULL) - start_time > 1)
	    {
		break;
	    }
	}
    }
}


int find_length(int a, int cur_length)
{
    int		b;
    int		length, max_length;

    used[a] = 1;
    max_length = cur_length;
    for(b = 0; b < N; b++)
    {
	if (conn[a][b] == 1 && used[b] != 1)
	{
	    length = find_length(b, cur_length+1);
	    if (length > max_length)
	    {
		max_length = length;
	    }
	}
    }
    used[a] = 0;

    return max_length;
}