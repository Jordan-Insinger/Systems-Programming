#include <linux/module.h>	/* for modules */
#include <linux/fs.h>		/* file_operations */
#include <linux/uaccess.h>	/* copy_(to,from)_user */
#include <linux/init.h>		/* module_init, module_exit */
#include <linux/slab.h>		/* kmalloc */
#include <linux/cdev.h>		/* cdev utilities */

MODULE_AUTHOR("Jordan Insinger");
MODULE_DESCRIPTION("ASP Assignment 4 - Character Device Driver");

#define MYDEV_NAME="asp_cdrv"

// module parameters
static int majorno = 500;
static int minorno = 0;
static int num_devices = 3;

module_param(majorno, int, S_IRUGO);
module_param(minorno, int, S_IRUGO);
module_param(num_devices, int, S_IRUGO);

struct asp_cdev {
	stuct cdev dev;
	char* ramdisk;
	struct semaphore sem;
	int devNo;
};

static const struct file_operations asp_fops = {
	.owner = THIS_MODULE,
	.open = asp_open,
	.read = asp_read,
	.write = asp_write,
	.release = asp_release,
};

static int asp_open(stuct inode *inode, struct file *filp) {

	struct asp_cdev *dev;

	dev = container_of(inode->i_cdev, struct asp_cdev, dev);
	filp->private_data = dev;

	pr_info("asp_driver: opened cdev%d\n", dev->devno);

	return 0;
}

static ssize_t asp_read(struct file* filp, char __user*buf, size_t count, loff_t *f_pos) {
	/* read, return bytes read?*/
	val = -1;
	return val;
}

static ssize_t asp_write(struct file* filp, const char __user* buf, size_t count, loff_t* f_pos) {
	/* write, return bytes written? */
	val = -1;	
	return val;
}

static int release(stuct inode *inode, struct file *filp) {
	/* do something here? */
	return 0;
}


static int __init asp_init(void) {
	ramdisk = kmalloc(ramdisk_size, GFP_KERNEL);
	first = MKDEV(majorno, minorno);
	register_chrdev_region(first, count, MYDEV_NAME);

}


