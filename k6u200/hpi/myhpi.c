/**
*  procfs2.c -  create a "file" in /proc
*
*/

#include <linux/module.h> /* Specifically, a module */
#include <linux/kernel.h> /* We're doing kernel work */
#include <linux/proc_fs.h>        /* Necessary because we use the proc fs */
#include <asm/uaccess.h>  /* for copy_from_user */
#include <linux/uidgid.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <asm/io.h>
#include <linux/ioport.h>

static int ghpiirq;
static int gNumInt=0;
static void __iomem *gio;
static struct completion hpi_done;

#define PROCFS_MAX_SIZE        1024
#define PROCFS_NAME            "myhpi"

//module_init(init_module);
//module_exit(cleanup_module);

static ssize_t procfile_read(struct file *,char *,size_t, loff_t*);
int procfile_write(struct file *, char *, size_t, loff_t *);
/**
* This structure hold information about the /proc file
*
*/
static struct proc_dir_entry *Our_Proc_File;
static struct file_operations hpi_file_ops={
	.owner = THIS_MODULE,
	.read = procfile_read,
	.write = procfile_write,
};

/**
* The buffer used to store character for this module
*
*/
static int gp32[PROCFS_MAX_SIZE];
static short *gp16=(short*)gp32;
static char *procfs_buffer=(char *)gp32;

/**
* The size of the buffer
*
*/
static unsigned long procfs_buffer_size = 0;

static irqreturn_t hpi_isr(int irq, void *dev_id)
{
	int i;
	short *p16=gp16+256;
	//void __iomem *io;
	gNumInt++;

	//io=ioremap(0x50000000,SZ_2K);
	writew(0x20,gio+0);
	writew(0x0,gio+8);
	writew(0x0,gio+0);
	writew(0x400,gio+8);
	for(i=0;i<256;i++){
		p16[i] = readw(gio+4);
	}

	complete(&hpi_done);

	return IRQ_HANDLED;
}
/**
* This function is called then the /proc file is read
*
*/
static ssize_t
procfile_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
        int ret;
	static int finished=0;
	char *p=procfs_buffer+512;

        //printk(KERN_INFO "procfile_read (/proc/%s) called  read:%d\n", PROCFS_NAME,length);

        if (finished > 0) {
                /* we have finished to read, return 0 */
                ret  = 0;
		finished = 0;
        } else {
		finished = 1;
                /* fill the buffer, return the buffer size */
		init_completion(&hpi_done);
		wait_for_completion_interruptible(&hpi_done);
		copy_to_user(buffer,p,length);
                ret = length;
        }

        return ret;
}

/**
* This function is called with the /proc file is written
*
*/
int procfile_write(struct file *file, char *buffer, size_t count,
                  loff_t *data)
{
	int i;

	for(i=0;i<256;i++) gp16[i]=0;
        /* get buffer size */
        procfs_buffer_size = count;
        if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
                procfs_buffer_size = PROCFS_MAX_SIZE;
        }

        /* write data to the buffer */
        if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
                return -EFAULT;
        }

	writew(0x20,gio+0);
	writew(0x0,gio+8);
	writew(0x0,gio+0);
	writew(0x200,gio+8);
	for(i=0;i<256;i++){
		writew(gp16[i],gio+4);// hpi.int , gpio3_io03 
	}
	writew(0x2,gio+0);

        return procfs_buffer_size;
}

/**
*This function is called when the module is loaded
*
*/
int __init init_module()
{
	int i;
	unsigned r;
	void __iomem *io;
	int ret;

	init_completion(&hpi_done);

	ghpiirq=gpio_to_irq(2*32+3);

        /* create the /proc file */
        Our_Proc_File = proc_create(PROCFS_NAME, S_IFREG | S_IRUGO, NULL, &hpi_file_ops);

        if (Our_Proc_File == NULL) {
                remove_proc_entry(PROCFS_NAME, NULL);
                printk(KERN_ALERT "Error: Could not initialize /proc/%s\n",
                        PROCFS_NAME);
                return -ENOMEM;
        }
	proc_set_user(Our_Proc_File,KUIDT_INIT(0),KGIDT_INIT(0));
	proc_set_size(Our_Proc_File,37);

        printk(KERN_INFO "/proc/%s created\n", PROCFS_NAME);
	io=ioremap(0x2000000,SZ_2M);
	r = readl(io+0x0c4080);
	writel(r|0x0c00,io+0x0c4080);
	for(i=0;i<2;i++) writel(4,io+0x0e01d8+(i<<2));
	for(i=0;i<2;i++) writel(4,io+0x0e01e8+(i<<2));
	for(i=0;i<16;i++) writel(4,io+0x0e0138+(i<<2));

	writel(5,io+0x0e0110);// hpi.int , gpio3_io03 

	writel(0x00610080,io+0x1b8000);
	writel(0x00610081,io+0x1b8000);
	iounmap(io);
	gio=ioremap(0x50000000,SZ_2M);

	ret = request_irq(ghpiirq, hpi_isr,
				 IRQF_TRIGGER_FALLING,//////////////////////////////////////////// | IRQF_TRIGGER_RISING,
				 //IRQF_TRIGGER_RISING,
				 "hpi.irq", io);

        return 0;      /* everything is ok */
}

/**
*This function is called when the module is unloaded
*
*/
void __exit cleanup_module()
{
	iounmap(gio);
        remove_proc_entry(PROCFS_NAME, NULL);
        printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
	free_irq(ghpiirq,ghpiirq);
}
MODULE_LICENSE("GPL");


