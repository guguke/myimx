/**
*  procfs2.c -  create a "file" in /proc
*
*/

#include <linux/module.h> /* Specifically, a module */
#include <linux/kernel.h> /* We're doing kernel work */
#include <linux/proc_fs.h>        /* Necessary because we use the proc fs */
#include <asm/uaccess.h>  /* for copy_from_user */
#include <linux/uidgid.h>

#define PROCFS_MAX_SIZE        1024
#define PROCFS_NAME            "myhpi"

//module_init(init_module);
//module_exit(cleanup_module);

static ssize_t procfile_read(struct file *,char *,size_t, loff_t*);
/**
* This structure hold information about the /proc file
*
*/
static struct proc_dir_entry *Our_Proc_File;
static struct file_operations hpi_file_ops={
	.owner = THIS_MODULE,
	.read = procfile_read,
};

/**
* The buffer used to store character for this module
*
*/
static char procfs_buffer[PROCFS_MAX_SIZE];

/**
* The size of the buffer
*
*/
static unsigned long procfs_buffer_size = 0;

/**
* This function is called then the /proc file is read
*
*/
static ssize_t
procfile_read(struct file *file, char *buffer, size_t length, loff_t *offset)
{
        int ret;
	static int finished=0;

        printk(KERN_INFO "procfile_read (/proc/%s) called\n", PROCFS_NAME);

        if (finished > 0) {
                /* we have finished to read, return 0 */
                ret  = 0;
		finished = 0;
        } else {
		finished = 1;
		ret = sprintf(buffer,"hello,world!\n");
                /* fill the buffer, return the buffer size */
                //ret = length;
        }

        return ret;
}

/**
* This function is called with the /proc file is written
*
*/
int procfile_write(struct file *file, const char *buffer, unsigned long count,
                  void *data)
{
        /* get buffer size */
        procfs_buffer_size = count;
        if (procfs_buffer_size > PROCFS_MAX_SIZE ) {
                procfs_buffer_size = PROCFS_MAX_SIZE;
        }

        /* write data to the buffer */
        if ( copy_from_user(procfs_buffer, buffer, procfs_buffer_size) ) {
                return -EFAULT;
        }

        return procfs_buffer_size;
}

/**
*This function is called when the module is loaded
*
*/
int __init init_module()
{
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
        return 0;      /* everything is ok */
}

/**
*This function is called when the module is unloaded
*
*/
void __exit cleanup_module()
{
        remove_proc_entry(PROCFS_NAME, NULL);
        printk(KERN_INFO "/proc/%s removed\n", PROCFS_NAME);
}
MODULE_LICENSE("GPL");


