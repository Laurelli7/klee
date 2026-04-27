#include "klee/klee.h"
#include<stdio.h>

char a[50][50];
int w,h;

void white(int p,int q);
void black(int p,int q);

int main(){
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&h, sizeof(h), "h");

    while(1){
        int resb=0,resw=0;
        if(w==0&&h==0) return 0;
        for(int i=0;i<w;i++)
        for(int i=0;i<w;i++){
            for(int j=0;j<h;j++){
                if(a[i][j]=='W') white(i,j);
                if(a[i][j]=='B') black(i,j);
            }
        }
        for(int i=0;i<w;i++){
            for(int j=0;j<h;j++){
                if(a[i][j]=='b') resb++;
                if(a[i][j]=='w') resw++;
            }
        }
    }
}

void white(int p,int q){
    for(int i=-1;i<=1;i++){
        for(int j=-1;j<=1;j++){
            if((i==0||j==0)&&p+i>=0&&p+i<w&&q+j>=0&&q+j<h){
                if(a[p+i][q+j]=='.'){
                    a[p+i][q+j]='w';
                    white(p+i,q+j);
                }else if(a[p+i][q+j]=='b'){
                    a[p+i][q+j]='g';
                    white(p+i,q+j);
                }
            }
        }
    }
}


void black(int p,int q){
    for(int i=-1;i<=1;i++){
        for(int j=-1;j<=1;j++){
            if((i==0||j==0)&&p+i>=0&&p+i<w&&q+j>=0&&q+j<h){
                if(a[p+i][q+j]=='.'){
                    a[p+i][q+j]='b';
                    black(p+i,q+j);
                }else if(a[p+i][q+j]=='w'){
                    a[p+i][q+j]='g';
                    black(p+i,q+j);
                } 
            }
        }
    }
}