#include <linux/module.h>	/* for modules */
#include <linux/fs.h>		/* file_operations */
#include <linux/uaccess.h>	/* copy_(to,from)_user */
#include <linux/init.h>		/* module_init, module_exit */
#include <linux/slab.h>		/* kmalloc */
#include <linux/cdev.h>		/* cdev utilities */
#include <linux/semaphore.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Jordan Insinger");
MODULE_DESCRIPTION("ASP Assignment 4 - Character Device Driver");

#define MYDEV_NAME "asp_cdrv"
#define ASP_IOC_MAGIC 'k'
#define ASP_CLEAR_BUF _IO(ASP_IOC_MAGIC, 0)

struct asp_cdev {
	struct cdev cdev;
	int devNo;
	char* ramdisk;
    size_t buffer_size;
	struct semaphore sem;
};

// module parameters
static int majorno = 500;
static int minorno = 0;
static int size = 16; // pages in ramdisk
static int num_devices = 3;

module_param(majorno, int, S_IRUGO);
module_param(size, int, S_IRUGO);
module_param(num_devices, int, S_IRUGO);

static struct asp_cdev* devices;
static struct class* asp_class; // for creating device nodes

static const struct file_operations asp_fops = {
	.owner = THIS_MODULE,
	.open = asp_open,
	.read = asp_read,
	.write = asp_write,
	.release = asp_release,
    .unlocked_ioctl = asp_ioctl,
    .llseek = asp_llseek,
};

static int asp_open(struct inode *inode, struct file* filp);
static ssize_t asp_read(struct file* filp, char __user*buf, size_t count, loff_t* f_pos);
static ssize_t asp_write(struct file* filp, const char __user* buf, size_t count, loff_t* f_pos);
static int asp_release(struct inode *inode, struct file* filp);
static int asp_setup_cdev(struct asp_cdev* dev, int index);
static long asp_ioctl(struct file* filp, unsigned int cmd, unsigned long arg);
static loff_t asp_llseek(struct file* filp, loff_t off, int whence);
static void __exit asp_exit(void);
static int __init asp_init(void);

static int asp_open(struct inode* inode, struct file* filp) {

	struct asp_cdev* dev;

    // initialize device
	dev = container_of(inode->i_cdev, struct asp_cdev, cdev);

    // set private data region
	filp->private_data = dev;

	pr_info("asp_driver: opened cdev%d\n", dev->devNo);

	return 0;
}

static ssize_t asp_read(struct file* filp, char __user* buf, size_t count, loff_t *f_pos) {
    struct asp_cdev* dev = filp->private_data;
    ssize_t ret_val = 0;
    
    // down on semaphore and clamp bytes to read
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;
    if (*f_pos >= dev->buffer_size)
        goto out;
    if (*f_pos + count > dev->buffer_size)
        count = dev->buffer_size - *f_pos;
        
    // copy bytes to user buf
    if (copy_to_user(buf, dev->ramdisk + *f_pos, count)) {
        ret_val = -EFAULT;
        goto out;
    }

    // move f_pos
    *f_pos += count;
    ret_val = count;

out:
    up(&dev->sem);
    return ret_val;

}

static ssize_t asp_write(struct file* filp, const char __user* buf, size_t count, loff_t* f_pos) {
    struct asp_cdev* dev = filp->private_data;
    ssize_t ret_val = 0;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    // if writing past buffer, reallocate
    if (*f_pos + count > dev->buffer_size) {
        ssize_t new_buffer_size = *f_pos + count;
        char* new_buf = krealloc(dev->ramdisk, new_buffer_size, GFP_KERNEL);

        if (new_buf == NULL)
            goto out;

        dev->ramdisk = new_buf;
        memset(dev->ramdisk + dev->buffer_size, 0, new_buffer_size - dev->buffer_size);
        dev->buffer_size = new_buffer_size;
    }

    if (copy_from_user(dev->ramdisk + *f_pos, buf, count))
    {
        ret_val = -EFAULT;
        goto out;
    }

    *f_pos += count;
    ret_val = count;

out:
    up(&dev->sem);
    return ret_val;
}

static int asp_release(struct inode *inode, struct file *filp) {
	return 0;
}

static loff_t asp_llseek(struct file* filp, loff_t off, int whence) {
    struct asp_cdev* dev = filp->private_data;
    loff_t new_pos;

    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;

    switch (whence) {
        case SEEK_SET:
            new_pos = off;
            break;
        case SEEK_CUR:
            new_pos = filp->f_pos + off;
            break;
        case SEEK_END:
            new_pos = dev->buffer_size + off; 
            break;
        default:
            up(&dev->sem);
            return -EINVAL;
    }
    
    if (new_pos < 0) {
        up(&dev->sem);
        return -EINVAL;
    }

    // if seeking past buffer, reallocate
    if (new_pos > dev->buffer_size) {
        char* new_buf = krealloc(dev->ramdisk, new_pos, GFP_KERNEL);

        if (new_buf == NULL) {
            up(&dev->sem);
            return -ENOMEM;
        }

        dev->ramdisk = new_buf;
        memset(dev->ramdisk + dev->buffer_size, 0, new_pos - dev->buffer_size);
        dev->buffer_size = new_pos;
    }

    filp->f_pos = new_pos;
    up(&dev->sem);
    return new_pos;
}

static long asp_ioctl(struct file* filp, unsigned int cmd, unsigned long arg) {

    struct asp_cdev* dev = filp->private_data;

    switch (cmd) {
        case ASP_CLEAR_BUF: 
            if (down_interruptible(&dev->sem))
                return -ERESTARTSYS;

            memset(dev->ramdisk, 0, dev->buffer_size);
            filp->f_pos = 0;
            up(&dev->sem);

            pr_info("asp_driver: cdev%d buffer cleared", dev->devNo);
            return 0;

        default: 
            return -ENOTTY;
    }
}

static void __exit asp_exit(void) {
    dev_t devno = MKDEV(majorno, minorno);

    if (devices) {
        for (int i = 0; i < num_devices; i++) {
            device_destroy(asp_class, MKDEV(majorno, i));
            kfree(devices[i].ramdisk);
            devices[i].ramdisk = NULL;
            cdev_del(&devices[i].cdev);
        }
        kfree(devices);
    }

    if (asp_class)
        class_destroy(asp_class);

    unregister_chrdev_region(devno, num_devices);
    pr_info("module unloaded");
}

static int __init asp_init(void) {

    int result, i;
    dev_t dev_id = 0;

    if (majorno == 500)
        pr_info("asp_driver: initializing with default majorno: 500");
    else
        pr_info("asp_driver: initializing with dynamic majorno: %d\n", majorno);

    dev_id = MKDEV(majorno, minorno);
	result = register_chrdev_region(dev_id, num_devices, MYDEV_NAME);
    
    if (result) {
        pr_err("asp_driver: register_chrdev_region failed: (err %d)", result);
        return result;
    }

    asp_class = class_create(THIS_MODULE, "asp_class");
    
    // allocate devices
    devices = kcalloc(num_devices, sizeof(struct asp_cdev), GFP_KERNEL);

    if (!devices) {
        pr_err("asp_driver: kcalloc failed for devices");
        result = -ENOMEM;
        goto fail;
    }

    // initialize devices
    for (i = 0; i < num_devices; i++) {
        asp_setup_cdev(&devices[i], i);
    }

    pr_info("asp_driver: successfully loaded");
    return 0;

fail:
    kfree(devices);
    class_destroy(asp_class);
    unregister_chrdev_region(dev_id, num_devices);
    return result;
}

static int asp_setup_cdev(struct asp_cdev* dev, int index) {
    int err; 
    dev_t dev_id = MKDEV(majorno, index); 

    // allocate ramdisk
    dev->ramdisk = kzalloc(size * PAGE_SIZE, GFP_KERNEL);
    dev->buffer_size = size * PAGE_SIZE;
    
    // setup semaphore - may need to change initial val for semaphores
    sema_init(&dev->sem, 1);
    
    // set devNo
    dev->devNo = index;

    cdev_init(&dev->cdev, &asp_fops);
    dev->cdev.owner = THIS_MODULE;
    err = cdev_add(&dev->cdev, dev_id, 1);

    if (err) {
        pr_err("Error %d while trying to add cdev %d", err, index);
        kfree(dev->ramdisk);
        dev->ramdisk = NULL;
        return err;
    }

    /* Create the /dev/mycdevX node with udev */
    if (IS_ERR(device_create(asp_class, NULL, dev_id, NULL,
                             "mycdev%d", index))) {
        pr_err("asp_driver: device_create failed for mycdev%d\n", index);
        cdev_del(&dev->cdev);
        kfree(dev->ramdisk);
        dev->ramdisk = NULL;
        return -ENODEV;
    }

    return 0;
        
}

module_init(asp_init);
module_exit(asp_exit);
