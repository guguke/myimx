#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc,char *argv[])
{
	int fd;
	short p16[512];
	int loop=1000;
	int i,len;

	printf(" usage: ./hpiloop [loop number (default:1000)]\n");
	printf(" usage: ./hpiloop \n");
	printf(" usage: ./hpiloop 10000\n");
	if(argc>1)loop=atoi(argv[1]);

	fd=open("/proc/myhpi",O_RDWR);
	if(fd==-1){
		printf("open proc.myhpi error\n");
		return -2;
	}

	p16[0]=0x13ec;
	p16[1]=0x0b;
	p16[2]=0x1;
	write(fd,p16,6);
	printf("  send.cmd : 0x%x 0x%x 0x%x\n",p16[0],p16[1],p16[2]);


	p16[0]=0x13ec;
	p16[1]=0x4;
	p16[2]=0x1;
	write(fd,p16,6);
	printf("  send.cmd : 0x%x 0x%x 0x%x\n",p16[0],p16[1],p16[2]);

	p16[0]=0x13ec;
	p16[1]=0x6;
	p16[2]=0x0;
	write(fd,p16,6);
	printf("  send.cmd : 0x%x 0x%x 0x%x\n",p16[0],p16[1],p16[2]);

	printf(" start loop test\n");
	for(i=0;i<loop;i++){
		len=read(fd,p16,512);
		if(len > 0) write(fd,p16,len);
		printf("\r read.write loop : %d",i+1);
	}
	printf("\n");
	close(fd);
	return 0;
}
