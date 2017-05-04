/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>

#define BUFSIZE 1024

static char *gp12,*gp16,*gp24;

struct oled_dot {
	int offsetx;
	int offsety;
	int w;
	int h;
	int style;
	char dot[1024];
};
struct oled_st0 {
	int offsetx;
	int offsety;
	int fontsize;
	int style;
	char str[256];
};
union oled_udata {
	struct oled_dot st_dot;
	struct oled_st0 st0;
};
struct udp_oled {
	char p8;
	char t8;
	short len16;
	union oled_udata udata;
};
/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}
void loaddot()
{
	int fd;
	fd=open("song12.dot",O_RDONLY);
	if(fd==-1) return;
	read(fd,gp12,300000);
	close(fd);
	fd=open("song16.dot",O_RDONLY);
	if(fd==-1) return;
	read(fd,gp16,300000);
	close(fd);
	fd=open("song24.dot",O_RDONLY);
	if(fd==-1) return;
	read(fd,gp24,650000);
	close(fd);

}
int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    struct udp_oled oled;
    struct oled_dot *pDot;
    struct oled_st0 *pSt0;
    int i,j,k;
    char *p;
    int fontsize=16,num=32;

    pSt0 = &(oled.udata.st0);

    gp12=malloc(300000);
    gp16=malloc(300000);
    gp24=malloc(700000);
    loaddot();

    

    oled.p8 = 1;
    oled.t8 = 3;
    oled.len16 = sizeof(struct oled_dot);
    pDot = &(oled.udata.st_dot);
    pDot->offsetx = 0;
    pDot->offsety =0;
    pDot->w = 16;
    pDot->h = 16;
    pDot->style = 0;
    for(i=0;i<1024;i++) pDot->dot[i]=0xff;

    /* check command line arguments */
    if (argc < 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);
    if(argc>3)fontsize=atoi(argv[3]);
    else fontsize=16;

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);
#if 0
    /* get a message from the user */
    bzero(buf, BUFSIZE);
    printf("Please enter msg: ");
    fgets(buf, BUFSIZE, stdin);
#endif
    /* send the message to the server */
    serverlen = sizeof(serveraddr);
       oled.t8 = 2;
       oled.len16=0;
       n = sendto(sockfd, &oled, sizeof(struct udp_oled), 0, &serveraddr, serverlen);
    //p = gp16+(15*94*32);
    p = gp16+(15*94*32);
       oled.t8 = 0;
    num = (128/fontsize)*(64/fontsize);
    for(i=0;i<num;i++,p+=32){
	usleep(4000);
    //pDot->offsety = (i&0x7)<<4;
    //pDot->offsetx =((i>>3)&0x3)<<4;
    pSt0->offsety = (i%(128/fontsize))*fontsize;
    pSt0->offsetx =(i/(128/fontsize))*fontsize;
    pSt0->fontsize = fontsize;
    pSt0->style = 0;
       //for(j=0;j<32; j++) pDot->dot[j]=p[j];
	pSt0->str[0]=0xb0;
	pSt0->str[1]=0xa1+i;
	pSt0->str[2]=0x0;
	oled.len16=2+16;
       //n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
       n = sendto(sockfd, &oled, sizeof(struct udp_oled), 0, &serveraddr, serverlen);
       if (n < 0) 
          error("ERROR in sendto");
    }
#if 0 
    /* print the server's reply */
    n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
    if (n < 0) 
      error("ERROR in recvfrom");
    printf("Echo from server: %s", buf);
#endif
    free(gp12);
    free(gp16);
    free(gp24);

    return 0;
}
