#include "klee/klee.h"
#include <stdio.h>
#include <stdlib.h>

int main(){
    int t,cont=0;
  klee_make_symbolic(&t, sizeof(t), "t");
    long long int n;
  klee_make_symbolic(&n, sizeof(n), "n");

    for(int i =0;i < t;++i){

            if(n==3)
                cont = 2;
            else if(n==2)
                cont = 1;
            else if(n==1)
                cont =0;
            else if(n > 3 && n%2==0)
                cont =2;
            else if(n > 3 && n%2!=0 )
                cont =3;


    }



    




    return 0;
}