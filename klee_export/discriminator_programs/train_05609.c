#include "klee/klee.h"
#include <stdio.h>
int n,i,j,x,a[10][10],list[1000][3],size;

void push(int x,int y,int s){list[size][0]=i;list[size][1]=j;list[size][2]=s;size++;}

int check(int x,int y,int s){
	int d;
	if(s<3 && (x<1 || x>9 || y<1 || y>9)) return 0;
	if(s==3 && (x<2 || x>8 || y<2 || y>8)) return 0;
	d=a[x][y];
	if(a[x-1][y]<d)d=a[x-1][y];
	if(a[x+1][y]<d)d=a[x+1][y];
	if(a[x][y-1]<d)d=a[x][y-1];
	if(a[x][y+1]<d)d=a[x][y+1];
	if(s==1 || d==0)return d;
	if(a[x-1][y-1]<d)d=a[x-1][y-1];
	if(a[x+1][y+1]<d)d=a[x+1][y+1];
	if(a[x+1][y-1]<d)d=a[x+1][y-1];
	if(a[x-1][y+1]<d)d=a[x-1][y+1];
	if(s==2 || d==0)return d;
	if(a[x-2][y]<d)d=a[x-2][y];
	if(a[x+2][y]<d)d=a[x+2][y];
	if(a[x][y-2]<d)d=a[x][y-2];
	if(a[x][y+2]<d)d=a[x][y+2];
	return d;
}

void draw(int x,int y,int s,int d){
	if(size==3 && (x<2 || x>8 || y<2 || y>8)) return;
	a[x][y]+=d;
	a[x-1][y]+=d;
	a[x+1][y]+=d;
	a[x][y-1]+=d;
	a[x][y+1]+=d;
	if(s==1)return;
	a[x-1][y-1]+=d;
	a[x+1][y+1]+=d;
	a[x+1][y-1]+=d;
	a[x-1][y+1]+=d;
	if(s==2)return;
	a[x-2][y]+=d;
	a[x+2][y]+=d;
	a[x][y-2]+=d;
	a[x][y+2]+=d;
}

int search(int nn,int from){
	int i,x,y,s;
	for(i=from;i>=0;i--){
		x=list[i][0];y=list[i][1];s=list[i][2];
		if(a[x][y]==0 && a[x+1][y]>0)return 0;
		if(s<3 && x<8 && a[x+2][y]>0)return 0;
		if(check(x,y,s)>0){
			draw(x,y,s,-1);
			if(s==1 && a[x+1][y+1]>0 && check(x-1,y+1,3)==0){draw(x,y,s,+1);return 0;}
			if(nn>1){
				if(search(nn-1,i-1)>0){return 1;}
			}else{
				int p,q;
				for(p=0;p<10;p++)for(q=0;q<10;q++){{if(a[p][q]!=0)goto miss;}}return 1;miss:;
			}
			draw(x,y,s,+1);
		}
	}
	return 0;
}

int main(void){
  klee_make_symbolic(&a, sizeof(a), "a");
  klee_make_symbolic(&n, sizeof(n), "n");
size=0;
	for(i=0;i<10;i++){for(j=0;j<10;j++){}}
	for(i=1;i<9;i++){
		for(j=1;j<9;j++){
			x=check(i,j,1); if(x==0)continue; else for(;x>0;x--)push(i,j,1);
			x=check(i,j,2); if(x==0)continue; else for(;x>0;x--)push(i,j,2);
			x=check(i,j,3); if(x==0)continue; else for(;x>0;x--)push(i,j,3);
		}
	}
	//for(i=0;i<size;i++)
	search(n,size-1);
	return 0;
}