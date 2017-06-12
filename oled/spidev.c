#include <stdint.h>  
#include <unistd.h>  
#include <stdio.h>  
#include <stdlib.h>  
#include <getopt.h>  
#include <fcntl.h>  
#include <sys/ioctl.h>  
#include <linux/types.h>  
#include <linux/spi/spidev.h>
#include <sys/socket.h>
#include <netinet/in.h>  
#include <arpa/inet.h>  
#include <netdb.h>  
#include <string.h>
#include <stdbool.h>

#include "ft2build.h"

#include "ziku.h"
#include "gbk2unicode.h"

#include "mydot.h"

#include FT_FREETYPE_H

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define LowerColumn  0x02	
#define HighColumn   0x10	
#define Max_Column	128	
#define Max_Row		64

#define port 45454

#define DEBUG

struct st_mydot *gp_mydot;

unsigned char screen[64][128];

unsigned char teststring[]={0xb1, 0xa6, 0x61, 0x00, 0x61, 0x00, 0x61, 0x00, 0};
unsigned char testdata[]={0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80};

static void pabort(const char *s)
{
	perror(s);
	abort();
}

static const char *device = "/dev/spidev3.0";
static uint8_t mode;
static uint8_t bits = 9;
static uint32_t speed = 8000000;
static uint16_t delay;

static void write_command(int fd, uint8_t data)
{
	int ret;  
	uint8_t tx[] = {
		0xff, 0x00
	};  
	tx[0] = data;
	uint8_t rx[ARRAY_SIZE(tx)] = {0, };  
	struct spi_ioc_transfer tr = {  
		.tx_buf = (unsigned long)tx,  
		.rx_buf = (unsigned long)rx,  
		.len = ARRAY_SIZE(tx),  
		.delay_usecs = delay,  
		.speed_hz = speed,  
		.bits_per_word = bits,  
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);  
	if (ret < 1)  
		pabort("can't send spi message");
}  

static void write_data(int fd, uint8_t data)
{
	int ret;  
	uint8_t tx[] = {
		0xff, 0xff
	};  
	tx[0] = data;
	uint8_t rx[ARRAY_SIZE(tx)] = {0, };
	struct spi_ioc_transfer tr = {  
		.tx_buf = (unsigned long)tx,  
		.rx_buf = (unsigned long)rx,  
		.len = ARRAY_SIZE(tx),  
		.delay_usecs = delay,  
		.speed_hz = speed,  
		.bits_per_word = bits,  
	};

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);  
	if (ret < 1)  
		pabort("can't send spi message");  
}

void set_start_column(int fd, unsigned char x)
{
	if((x%16) >= 0x0d)
	{
		write_command(fd, HighColumn + (((x) >> 4) + 1));		// Set Higher Column Start Address for Page Addressing Mode
		write_command(fd, (LowerColumn + ((x) & 0x0f)) & 0x0f);	
	}
	else
	{
	  write_command(fd, LowerColumn + x%16);		// Set Lower Column Start Address for Page Addressing Mode
	  write_command(fd, HighColumn + x/16);	
	}
	//write_command(fd, LowerColumn + d%16);
	//write_command(fd, HighColumn + d/16);
}

void set_start_page(int fd, unsigned char d)
{
	write_command(fd, 0xb0 | d);
}

void set_contrast_control(int fd, unsigned char d)
{
	write_command(fd, 0x81);
	write_command(fd, d);
}

void set_segment_remap(int fd, unsigned char d)
{
	write_command(fd, 0xa0 | d);
}

void set_common_remap(int fd, unsigned char d)
{
	write_command(fd, 0xc0 | d);
}

void set_inverse_display(int fd, unsigned char d)
{
	write_command(fd, 0xa6 | d);
}

void set_display_on_off(int fd, unsigned char d)
{
	write_command(fd, 0xae | d);
}

void set_vcomh(int fd, unsigned char d)
{
	write_command(fd, 0xdb);
	write_command(fd, d);
}

void set_nop(int fd)
{
	write_command(fd, 0xe3);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show Character (5x7)
//
//    a: Database
//    b: Ascii
//    c: Start Page
//    d: Start Column
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void show_font57(int fd, unsigned char a, unsigned char b, unsigned char c, unsigned char d)
{
	unsigned char *Src_Pointer;
	unsigned char i=0;

	switch(a)
	{
		case 1:
			Src_Pointer=&Ascii_1[(b)][i];
			break;
		case 2:
			Src_Pointer=&Ascii_2[(b)][i];
			break;
	}
	set_start_page(fd, c);
	set_start_column(fd, d);
	for(i=0;i<5;i++)
	{
		write_data(fd, *Src_Pointer);
		Src_Pointer++;
	}
	write_data(fd, 0x00);
}

//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//  Show String
//
//    a: Database
//    b: Start Page
//    c: Start Column
//    * Must write "0" in the end...
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
void show_string(int fd, unsigned char a, unsigned char *Data_Pointer, unsigned char b, unsigned char c)
{
	unsigned char *Src_Pointer;

	Src_Pointer=Data_Pointer;
	show_font57(fd, 1,96,b,c);			// No-Break Space
						//   Must be written first before the string start...

	while(1)
	{
		show_font57(fd, a,(*Src_Pointer-32),b,c);
		Src_Pointer++;
		c+=6;
		if(*Src_Pointer == 0) break;
	}
}

int spi_init()
{
	int ret = 0;
	int fd;

	fd = open(device, O_RDWR);
	if (fd < 0)
		pabort("can't open device");

	/*
	* spi mode
	*/
	ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		pabort("can't set spi mode");

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
		pabort("can't get spi mode");

	/*
	* bits per word
	*/
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't set bits per word");

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1)
		pabort("can't get bits per word");

	/*
	* max speed hz
	*/
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't set max speed hz");

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1)
		pabort("can't get max speed hz");

	#ifdef DEBUG
	printf("spi mode: %d\n", mode);
	printf("bits per word: %d\n", bits);
	printf("max speed: %d Hz (%d KHz)\n", speed, speed/1000);
	#endif

	return fd;
}



void lcd_init(int fd)
{
	write_command(fd, 0xae);

	write_command(fd, 0xd5); 
	write_command(fd, 0x80);

	write_command(fd, 0xa8);
	write_command(fd, 0x3f);

	write_command(fd, 0xd3);
	write_command(fd, 0x00);

	write_command(fd, 0x40);

	write_command(fd, 0xad);
	write_command(fd, 0x8b);

	write_command(fd, 0x33);

	write_command(fd, 0xa1);

	write_command(fd, 0xc8);

	write_command(fd, 0xda);
	write_command(fd, 0x12);

	write_command(fd, 0x81);
	write_command(fd, 0x50);

	write_command(fd, 0xd9);
	write_command(fd, 0x22);

	write_command(fd, 0xdb);
	write_command(fd, 0x18);

	write_command(fd, 0xa4);

	write_command(fd, 0xa6); //0xa6 change color

	write_command(fd, 0xaf);
}

unsigned short g2u(unsigned char *data)
{
    unsigned int i;
    unsigned short g=0;
    g |= (data[0]<<8);
    g |= data[1];

	#ifdef DEBUG
    printf("find gbk:%x\n", g);
    #endif

    for (i=0; i<10589; i++)
    {
        if (g == g2umap[i][1])
        {
            return g2umap[i][0];
        }
    }
    return 0;
}

/*
	把缓存区某一块的点转换成字节，写入屏幕。
	xb:缓存区开始的横坐标
	yb:开始的纵坐标
	xe:结束的横坐标
	ye:结束的纵坐标
*/
int flash(int fd, unsigned char xb, unsigned char yb, unsigned char xe, unsigned char ye)
{
	unsigned char pb = xb / 8;
	unsigned char cb = yb;
	unsigned char pe = xe / 8;
	unsigned char ce = ye;
	unsigned char i,j,k;
	unsigned char temp;

	//检测坐标是否合法
	if (pe >= 8)
	{
		pe = 7;
	}
	if (ce >= 128)
	{
		ce = 127;
	}
	if (pb >= 8 | cb >= 128)
	{
		printf("out of screen\n");
		return 1;
	}
	//点转为字节
	for (i=pb; i<=pe; i++)
	{
		set_start_page(fd, i);
		set_start_column(fd, cb);
		for (j=cb; j<=ce; j++)
		{
			temp = 0x00;
			for (k=0; k<8; k++)
			{
				if (screen[i*8+k][j])
				{
					temp |= (0x01 << k);
				}
			}
			write_data(fd, temp);
		}
	}
}
/*
	缓冲区帅刷屏
	xb:缓存区开始的横坐标
	yb:开始的纵坐标
	xe:结束的横坐标
	ye:结束的纵坐标
	d :0或者1
*/
int clear(int fd, unsigned char xb, unsigned char yb, unsigned char xe, unsigned char ye, unsigned char d)
{
	unsigned char i,j;
	if(xb >= 64 | yb >= 128)
		return 1;
	if(xe >= 64)
		xe = 63;
	if(ye >= 128)
		ye = 127;
	for (i=xb; i<=xe; i++)
		for(j=yb; j<=ye; j++)
			screen[i][j] = d;
	flash(fd, xb, yb, xe, ye);
}

/*
	显示一个任意大小的字符
	font_size:字体大小
	gbk_code:48~57是数字, 65~90是大写字母,97~122小写字母
	x:开始横坐标
	y:开始纵坐标
	h:1.高亮 0.不高量
	_type:字体

	返回宽度，用作移动
*/
unsigned char hanzi(int fd, unsigned char *gbk_code, unsigned char x, unsigned char y, int font_size, unsigned char h)
{
    FT_Library library;
    FT_Face face;
    int error;
    int i, j, k, counter;
    int m=0, n=0;
    unsigned char temp;
    unsigned char dot=1, blk=0;
    FT_UInt char_index;
    unsigned char *type1="AGENCYR.TTF";
    unsigned char *type2="DENG.TTF";
    unsigned char mov_y;

	unsigned short uni_code;		

    if (h)
    {
    	dot = 0;
    	blk = 1;
    }

    error = FT_Init_FreeType(&library);
    if (error)
    {
        printf("can not init free type library!\n");
        return 0;
    }

    if (gbk_code[0] < 0x7f && gbk_code[0] != 0)
    {
    	uni_code = gbk_code[0];
    	error = FT_New_Face(library, type1, 0, &face);
    	mov_y = font_size/2;
    }
    else
    {
    	uni_code = g2u(gbk_code);
    	error = FT_New_Face(library, type2, 0, &face);
    	mov_y = font_size;
    }

	#ifdef DEBUG
	printf("uni_code: %x\n", uni_code);
	#endif

    if (error)
    {
        printf("create new face falied!\n");
        return 0;
    }

    error = FT_Set_Pixel_Sizes(face, 0, font_size);
    if (error)
    {
        printf("set font size error!\n");
        return 0;
    }

    char_index = FT_Get_Char_Index(face, uni_code );
    error = FT_Load_Glyph(face, char_index, FT_LOAD_DEFAULT);
    if (error)
    {
        printf("Load char error!\n");
        return 0;
    }

    if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
    {
        error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO);
        if (error)
        {
            printf("render char failed!\n");
            return 0;
        }
    }

    for (j = 0; j < (font_size * 26) / 32 - face->glyph->bitmap_top; j++)
    {
        for (i = 0; i < font_size; i++)
        {
        	screen[m+x][n+y] = blk;
        	n += 1;
            //printf(" ");
        }
        m += 1;
        n = 0;
        //printf("\n");
    }

    for (; j < face->glyph->bitmap.rows + (font_size * 26) / 32 - face->glyph->bitmap_top; j++)
    {
        for (i = 1; i <= face->glyph->bitmap_left; i++)
        {
        	screen[m+x][n+y] = blk;
        	n += 1;
            //printf(" ");
        }

        for (k = 0; k < face->glyph->bitmap.pitch; k++)
        {
            temp = face->glyph->bitmap.buffer[face->glyph->bitmap.pitch*(j + face->glyph->bitmap_top - (font_size * 26) / 32) + k];
            for (counter = 0; counter < 8; counter++)
            {
 if (temp & 0x80)
                {
                	screen[m+x][n+y] = dot;
                	n += 1;
                    //printf("*");
                }
                else
                {
                	screen[m+x][n+y] = blk;
                	n += 1;
                    //printf(" ");
                }
                temp <<= 1;
                i++;
                if (i > font_size)
                {
                    break;
                }
            }
        }

        for (; i <= font_size; i++)
        {
        // printf("|");

        }
        m += 1;
        n = 0;
        //printf("\n");
    }

    for (; j < font_size; j++)
    {
        for (i = 0; i < font_size; i++)
        {
        	screen[m+x][n+y] = blk;
        	n += 1;
            //printf(" ");
        }
        m += 1;
        n = 0;
        //printf("\n");
    }
    flash(fd, x, y, x+font_size, y+mov_y);
    return mov_y;
}

/*
	画点

	data:数据
	x:起始横坐标
	y:起始纵坐标
	len_x:横坐标长度
	len_y:纵坐标长度
	h:1高亮 0正常
*/

int draw_point33(int fd, unsigned char *data, unsigned char x, unsigned char y, unsigned char len_x, unsigned char len_y, unsigned char h)
{
	unsigned char i,j,k;
	unsigned char *data_p = data;
	unsigned char temp;

	for (i=0; i<len_x; ++i)
	{
		for (j=0; j<(((len_y-1)/8)+1); ++j)
		{
			temp = *data_p;
			for (k=0; k<8; ++k)
			{
				if(j*8+k >= len_y) break;
				screen[i+x][j*8+k+y] ^= 1;
			}
		data_p ++;
		}
	}
	flash(fd, x, y, x+len_x, y+len_y);
}
int draw_point(int fd, unsigned char *data, unsigned char x, unsigned char y, unsigned char len_x, unsigned char len_y, unsigned char h)
{
	unsigned char i,j,k;
	unsigned char dot, blk;
	unsigned char *data_p = data;
	unsigned char temp;

	if (h)
	{
		dot = 0;
		blk = 1;
	}
	else
	{
		dot = 1;
		blk = 0;
	}

	for (i=0; i<len_x; ++i)
	{
		for (j=0; j<(((len_y-1)/8)+1); ++j)
		{
			temp = *data_p;
			for (k=0; k<8; ++k)
			{
				if(j*8+k >= len_y)
					break;
				if(temp&(0x80>>k))
				{
					screen[i+x][j*8+k+y] = dot;
					#ifdef DEBUG
						printf("*");
					#endif
				}
				else
				{
					screen[i+x][j*8+k+y] = blk;
					#ifdef DEBUG
						printf("-");
					#endif
				}
			}
		data_p ++;
		}
		#ifdef DEBUG
			printf("\n");
		#endif
	}
	flash(fd, x, y, x+len_x, y+len_y);
}

/*
	打印一个GBK字符串
	date:gbk字符串
	x:开始横坐标
	y:开始纵坐标
	t:字符点阵大小
	h:1.高亮 0.不高量
	num:字符数量
*/

int screen_show(int fd, unsigned char *date, unsigned char x, unsigned char y, unsigned char t, unsigned char h, unsigned char num)
{
	unsigned char *date_p = date;
	unsigned char pos_x = x;
	unsigned char pos_y = y;
	unsigned char mov_y;

		#ifdef DEBUG
		printf("func screen_show     mov_y:%d    num:%d\n", mov_y,num);
		#endif
	while(num--)
	{
		if (*date < 0x7B && *date != 0)
			mov_y = t/2;
		else
			mov_y = t;

		#ifdef DEBUG
		printf("mov_y:%d    num:%d\n", mov_y,num);
		#endif

		if (pos_x+t > 64)
		{
			return 0; 
		}
		if (pos_y+mov_y > 128)
		{
			pos_x += t;
			if (pos_x+t > 64)
			{
				return 0; 
			}
			pos_y = 0;
		}

		mov_y = hanzi(fd, date_p, pos_x, pos_y, t, h);

		pos_y += mov_y;
		date_p += 1;
		if(t == mov_y)
		{
			num--;
			date_p += 1;
		}
	}
	return 0;
}
unsigned char* getpdot(int sn,int* ph)
{
	unsigned char *p=NULL;
	int i;
	*ph = -1;
	for(i=0;i<gp_mydot->num;i++){
		if(sn==gp_mydot->idx[i]){
			p=gp_mydot->dot + gp_mydot->offset[i];
			*ph = gp_mydot->wh[i];
			break;
		}
	}

	return p;
}
/*
	阻塞等待udp数据包
*/
int rec_loop(int fd)
{
	int sock, address_len;
	unsigned char i;
	unsigned char x, y, t, h, len, len_x, len_y;
    int wh;

    struct Rec_Date
    {
        unsigned char t1;
        unsigned char t2;
        unsigned char lenl;
        unsigned char lenh;

        unsigned char buf[2048];
    }rec_date, *rec_p;

    rec_p = &rec_date; 

	struct sockaddr_in address;

    bzero(&address,sizeof(address));  
    address.sin_family=AF_INET;  
    address.sin_addr.s_addr=htonl(INADDR_ANY);  
    address.sin_port=htons(port);  
    address_len=sizeof(address);  	

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    bind(sock,(struct sockaddr *)&address,sizeof(address));

    while(1)  
    {  
        recvfrom(sock,(unsigned char *)rec_p,sizeof(rec_date),0,\
        	(struct sockaddr *)&address,&address_len);  
        #ifdef DEBUG
        	printf("t1:%d\n", rec_date.t1);
        	printf("t2:%d\n", rec_date.t2);
        	printf("len_l%d\n", rec_date.lenl);
        	printf("len_h%d\n", rec_date.lenh);
        #endif

        //普通文本
        if (rec_date.t2 == 0x00)
        {
        	#ifdef DEBUG
        	printf("get document\n");
    		#endif
        	unsigned char *p = rec_date.buf;
        	unsigned int *int_p = (unsigned int *)p;

        	len = rec_date.lenl + rec_date.lenh*255;

    		#ifdef DEBUG
    		printf("len: %d\n", len);
    		#endif

        	if (len != 0)
        	{
	        	//x = ((*int_p)&0xff000000) >> 24;
	        	x = *int_p;
	    		int_p ++;
	        	#ifdef DEBUG
	    		printf("x: %d\n", x);
	    		#endif

	    		y = *int_p;
	    		int_p ++;
	        	#ifdef DEBUG
	    		printf("y: %d\n", y);
	    		#endif

	    		t = *int_p;
	    		int_p ++;
	        	#ifdef DEBUG
	    		printf("t: %d\n", t);
	    		#endif

	    		h = *int_p;
	    		int_p ++;
	        	#ifdef DEBUG
	    		printf("h: %d\n", h);
	    		#endif

	    		p += 16;
	    		rec_date.buf[len] = 0;
	    		screen_show(fd, p, x, y, t, h, (len-16));
        	}
        }

        //特殊文本
        if (rec_date.t2 == 0x01)
        {
        	unsigned char *p = rec_date.buf;
        	unsigned char *p1;
        	unsigned int *int_p = (unsigned int *)p;

        	len = rec_date.lenl + rec_date.lenh*255;

        	if (len != 0)
        	{
	        	//x = ((*int_p)&0xff000000) >> 24;
	        	x = *int_p;
	    		int_p ++;

	    		y = *int_p;
	    		int_p ++;

	        	len_x = *int_p;
	    		len_y = *int_p;
	    		int_p ++;

	    		h = *int_p;
	    		int_p ++;

			if(*int_p==33) draw_point33(fd, p1, x, y, 1, len_y, h);
			else{
	    		p1=getpdot(*int_p,&wh);
			if(wh>0) draw_point(fd, p1, x, y, wh, wh, h);
			}
        	}
	}

        //清空屏幕
        if (rec_date.t2 == 0x02)
        {
        	clear(fd, 0, 0, 64, 128, 0);
        	#ifdef DEBUG
        	printf("clear screen\n");
        	#endif
        }

        //点阵字符
        if (rec_date.t2 == 0x03)
        {
        	#ifdef DEBUG
        	printf("draw point\n");
    		#endif
        	unsigned char *p = rec_date.buf;
        	unsigned int *int_p = (unsigned int *)p;

        	len = rec_date.lenl + rec_date.lenh*255;

    		#ifdef DEBUG
    		printf("len: %d\n", len);
    		#endif

        	if (len != 0)
        	{
	        	//x = ((*int_p)&0xff000000) >> 24;
	        	x = *int_p;
	    		int_p ++;
	        	#ifdef DEBUG
	    		printf("x: %d\n", x);
	    		#endif

	    		y = *int_p;
	    		int_p ++;
	        	#ifdef DEBUG
	    		printf("y: %d\n", y);
	    		#endif

	        	len_x = *int_p;
	    		int_p ++;
	        	#ifdef DEBUG
	    		printf("len_x: %d\n", len_x);
	    		#endif

	    		len_y = *int_p;
	    		int_p ++;
	        	#ifdef DEBUG
	    		printf("len_y: %d\n", len_y);
	    		#endif

	    		h = *int_p;
	    		int_p ++;
	        	#ifdef DEBUG
	    		printf("h: %d\n", h);
	    		#endif

	    		p += 20;
	    		rec_date.buf[len] = 0;
	    		draw_point(fd, p, x, y, len_x, len_y, h);
        	}
        }


        //关闭屏幕
        if (rec_date.t2 == 0x04)
        {
        	write_command(fd, 0xae);
        	#ifdef DEBUG
        	printf("close screen\n");
        	#endif
        }

        //打开屏幕
        if (rec_date.t2 == 0x05)
        {
        	write_command(fd, 0xaf);
        	#ifdef DEBUG
        	printf("open screen\n");
        	#endif
        }
    }

    close(sock);  
    exit(0);  
  
    return (EXIT_SUCCESS);  
}

/*
	debug ,输出缓冲区64*128
*/
#ifdef DEBUG
int debug_show()
{
	unsigned char i,j;
	for (i=0; i<64; i++)
	{
		for (j=0; j<128; j++)
		{
			if (screen[i][j])
				printf("*");
			else
				printf(" ");
		}
		printf("\n");
	}
}
#endif



int main(int argc, char *argv[])
{
	int fd;
	int i;
	FILE *fp;
	char fn[100];

	if(argc>1){
		sprintf(fn,"%s",argv[1]);//		strcpy(argv[1],fn);
	}
	else sprintf(fn,"mydot.bin");

	gp_mydot = malloc(sizeof(struct st_mydot));
	memset(gp_mydot,0,sizeof(struct st_mydot));
	fp=fopen(fn,"rb");
	if(fp!=NULL){
		fread(gp_mydot,sizeof(struct st_mydot),1,fp);
		fclose(fp);
	}
	else printf(" file %s  not found\n",fn);

	fd = spi_init(); //init spi

	lcd_init(fd); //init oled

	clear(fd, 0, 0, 63, 127, 0); // clear screen buffer
	//clear(fd, 0, 0, 64, 128, 0);
	//debug_show();
	//screen_show(fd, teststring, 0, 0, 16, 0);
	//screen_show(fd, teststring, 0, 0, 30, 0);
	//screen_16(fd, teststring, 0, 0, 0);
	//write_command(fd, 0xae);
	unsigned char line1[] = {0xb5,0xc8,0xb4,0xfd,0xd6,0xd0};

	screen_show(fd, line1, 0, 0, 20, 0, 6);
 
	rec_loop(fd);
	//hanzi(fd,teststring, 0, 0, 50, 0);
	//hanzi(fd,teststring2, 0, 0+50, 50, 0);
	//hanzi(fd,teststring2, 0, 0+50+25, 50, 0);
	//draw_point(fd, testdata, 0, 0, 8, 15, 0);


	#ifdef DEBUG
	debug_show();
	#endif

	close(fd);

	free(gp_mydot);

	return 0;
}
