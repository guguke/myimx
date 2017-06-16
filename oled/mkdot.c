#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mydot.h"

struct st_mydot *gp_mydot;
void add1(int sn,int wh,char *p)
{
	int n;
	int len;

	//printf("add sn(%d) size:%d\n",sn,wh);
	if(sn<1)return;
	n=gp_mydot->num;
	gp_mydot->num++;
	gp_mydot->idx[n]=sn;
	gp_mydot->wh[n]=wh;
	len = wh * ((wh+7)>>3);
	gp_mydot->offset[n+1] = len + gp_mydot->offset[n];
	memcpy(gp_mydot->dot + gp_mydot->offset[n],p,len);
	
}
void dot1(char *buf,char *p,int wh,int y)
{
	int len,i;
	int n,n1;
	char *p1;
	//printf("add dot, line:%d\n",y);
	len = strlen(buf);
	n1=(wh+7)>>3;
	p1 = p + n1 * y;
	for(i=0;i<len;i++){
		if(buf[i]!='*')continue;
		p1[i/8] |= 1<<(7-(i%8));
	}
}
void show1line(char *p,int wh)
{
	int n=(wh+7)>>3;
	int i,j;
	for(i=0;i<n;i++){
		for(j=0;j<8;j++){
			if(p[i] & (1<<(7-j)))printf("* ");
			else printf("  ");
		}
	}
	printf("\n");
}

void show1(int n)
{
	int i;
	int wh;
	char *p;
	int n1;
	p=gp_mydot->dot + gp_mydot->offset[n];
	wh=gp_mydot->wh[n];
	n1 = (wh+7)>>3;
	for(i=0;i<wh;i++) show1line(p+ i*n1,wh);
}
void show()
{
	int i,j,k;
	for(i=0;i<gp_mydot->num;i++){
		printf("sn:%d size:%d\n",gp_mydot->idx[i],gp_mydot->wh[i]);
		show1(i);
	}
}
void initDot()
{
	FILE *fp;
	int idx=0;
	char line[300];
	int num;
	int sn,sn1;
	char p[640];
	int wh,wh1;
	char buf[100];
	int x,y;
	int i,n;

	gp_mydot=malloc(sizeof(struct st_mydot));
	memset(gp_mydot,0,sizeof(struct st_mydot));
	gp_mydot->num=1;
	gp_mydot->idx[idx]=0;
	gp_mydot->offset[idx]=0;
	gp_mydot->wh[idx]=8;
	for(i=0;i<8;i++) gp_mydot->dot[i]=0x81;
	gp_mydot->dot[0]=0x0ff;
	gp_mydot->dot[7]=0x0ff;
	idx++;
	gp_mydot->offset[idx]=8;

	fp=fopen("mydot.txt","rt");
	if(fp==NULL)return;
	sn=-1;
	for(;;){
		if( NULL==fgets(line,100,fp)){
			add1(sn,wh,p);
			break;
		}
		n=sscanf(line,"%s%d%d",buf,&sn1,&wh1);
		if(n>=3){
			add1(sn,wh,p);
			sn=sn1;
			wh=wh1;
			for(i=0;i<640;i++)p[i]=0;
			y=0;
		}
		else if(1==n){
			dot1(buf,p,wh,y);
			y++;
		}
	}
	fclose(fp);
	show();
	fp=fopen("mydot.bin","wb");
	fwrite(gp_mydot,sizeof(struct st_mydot),1,fp);
	fclose(fp);
	free(gp_mydot);
}

int main(int argc,char *argv[])
{
	initDot();

	return 0;
}

