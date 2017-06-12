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

	if(argc>1)loop=atoi(argv[1]);

	fd=open("/proc/myhpi",O_RDWR);
	if(fd==-1){
		printf("open proc.myhpi error\n");
		return -2;
	}
	for(i=0;i<loop;i++){
		len=read(fd,p16,512);
		if(len > 0) write(fd,p16,len);
	}
	close(fd);
	return 0;
}
