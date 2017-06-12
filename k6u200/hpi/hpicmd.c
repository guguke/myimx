#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc,char *argv[])
{
	int fd;
	short p16[512];

	if(argc<4){
		printf("para < 3 , error\n");
		return -1;
	}
	sscanf(argv[1],"%i",p16);
	sscanf(argv[2],"%i",p16+1);
	sscanf(argv[3],"%i",p16+2);
	printf("cmd : 0x%04x 0x%04x 0x%04x\n",p16[0],p16[1],p16[2]);

	fd=open("/proc/myhpi",O_RDWR);
	if(fd==-1){
		printf("open proc.myhpi error\n");
		return -2;
	}
	write(fd,p16,6);
	close(fd);
	return 0;
}
