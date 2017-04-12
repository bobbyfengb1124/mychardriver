#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/ioctl.h>

#define MAJOR_NUMBER 61
#define SCULL_IOC_MAGIC 'k'
#define SCULL_HELLO _IO(SCULL_IOC_MAGIC, 1)
#define SCULL_FROM_USER _IOR(SCULL_IOC_MAGIC, 2, char *)
#define SCULL_TO_USER _IOW(SCULL_IOC_MAGIC, 3, char *)
#define SCULL_WR_USER _IOW(SCULL_IOC_MAGIC, 4, char *)
#define SCULL_IOC_MAXNR 14

/* forward declaration */
int onebyte_open(struct inode *inode, struct file *filep);
int onebyte_release(struct inode *inode, struct file *filep);
ssize_t onebyte_read(struct file * filep, char *buf, size_t count, loff_t *f_pos);
ssize_t onebyte_write(struct file * filep, const char *buf, size_t count, loff_t *f_pos);
static void onebyte_exit(void);
loff_t onebyte_lseek (struct file * filep, loff_t f_pos, int whence);
long onebyte_ioctl(struct file *filep, unsigned int cmd, unsigned long arg);

/* definition of file_operation structure */
struct file_operations onebyte_fops = {
	llseek:  onebyte_lseek,
	read:	onebyte_read,
	write:	onebyte_write,
	open:	onebyte_open,
	unlocked_ioctl:  onebyte_ioctl,
	release: onebyte_release
};

char *fourmegabyte_data = NULL;
char dev_msg[16] = "READ";
char user_msg[16] = "";

long onebyte_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	int err=0;
	int retval = 0;

	/*
	* extract the type and number bitfields, and don'd
	* decode wrong cmds: return ENOTTY(inappropriate ioctl) before access_ok()
	*/

	if (_IOC_TYPE(cmd) != SCULL_IOC_MAGIC) return -ENOTTY;
	if (_IOC_NR(cmd) > SCULL_IOC_MAXNR) return -ENOTTY;

	/*
	* the direction is a bitmask, and VERIFY_WRITE catches R/W
	* transfer. 'Type' is user-oriented, while
	* access_ok is kernel-oriented, so the concept of "read" and 
	* "write" is reversed
	*/
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERYFY_WRITE, (void __user*) arg, _IOC_SIZE(cmd));
	
	if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERYFY_WRITE, (void __user*) arg, _IOC_SIZE(cmd));

	if (err) return -EFAULT;
	switch(cmd) {
		case SCULL_HELLO:
			printk(KERN_WARNING "HELLO\n");
			break;
		case SCULL_FROM_USER:
			copy_from_user(user_msg, arg, 16);
			retval = strlen(user_msg);
			printk(KERN_WARNING "msg from user: %s\n", user_msg);
			break;
		case SCULL_TO_USER:
			retval = copy_to_user(arg, dev_msg, 16);
			break;
		default: /* redundant, as cmd was checked against MAXNR */
			return -ENOTTY;
	}

	return retval;
}

loff_t onebyte_lseek (struct file * filep, loff_t off, int whence)
{
	loff_t newpos;
	switch(whence) {
		case 0: /* SEEK_SET */
			newpos = off;
			break;
		case 1: /* SEEK_CUR */
			newpos = filep->f_pos + off;
			break;
		case 2: /* SEEK_CUR */
			newpos = strlen(fourmegabyte_data) + off;
			break;

		default: /* can't happen */
			return -EINVAL;
	}
	if (newpos < 0) return -EINVAL;
	filep->f_pos = newpos;
	return newpos;
}
int onebyte_open(struct inode *inode, struct file *filep)
{
	return 0; // always successful
}

int onebyte_release(struct inode *inode, struct file *filep)
{
	return 0; // always successful
}

ssize_t onebyte_read(struct file * filep, char *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "Read DevOne: Data %s \n", fourmegabyte_data+*f_pos);
	printk(KERN_INFO "Read DevOne: Data Length %ld \n", count);
	if (*f_pos <= 4*1024*1024) {
		if (*f_pos + count > 4*1024*1024) {
			count = ((4*1024*1024) - *f_pos);
		}
		copy_to_user(buf, fourmegabyte_data+*f_pos, count);
		*f_pos+=count;
		return count;
	} else {
		return 0;
	}
}

ssize_t onebyte_write(struct file * filep, const char *buf, size_t count, loff_t *f_pos)
{
	printk(KERN_INFO "Write DevOne: Current Writing Position: %lld \n", *f_pos);
	if (*f_pos <= 4*1024*1024) {
		if (*f_pos + count > 4*1024*1024) {
			count = ((4*1024*1024) - *f_pos);
		}
		copy_from_user(fourmegabyte_data+*f_pos, buf, count);
		*f_pos+=count;
		printk(KERN_INFO "%ld has been written to /dev/one.\n", count);
		return count;
	}
	else {
		return -ENOSPC;
	}
}

static int onebyte_init(void)
{
	int result;
	// register the device
	result = register_chrdev(MAJOR_NUMBER, "onebyte", &onebyte_fops);
	if (result < 0) {
		return result;
	}

	// allocate one byte of memory for storage
	// kmalloc is just like malloc, the second parameter is 
	// the type of memory to be allocated.
	// To release the memory allocated by kmalloc, use kfree.
	fourmegabyte_data = kmalloc((4*1024*1024), GFP_KERNEL);
	if (!fourmegabyte_data) {
		onebyte_exit();
		// cannot allocate memory
		// return no memory error, negative signify a 
		// failure
		return -ENOMEM;
	}
	memset(fourmegabyte_data, 0, (4*1024*1024));
	// initialize the value to be X
	sprintf(fourmegabyte_data, "X");
//	*fourmegabyte_data = "X\0";
	printk(KERN_ALERT "This is a onebyte device module\n");
	return 0;
}

static void onebyte_exit(void)
{
	// if the pointer is pointing to something
	if (fourmegabyte_data) {
		// free the memory and assign the pointer to NULL
		kfree(fourmegabyte_data);
		fourmegabyte_data = NULL;
	}
	// unregister the device
	unregister_chrdev(MAJOR_NUMBER, "onebyte");
	printk(KERN_ALERT "Onebyte device module is unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(onebyte_init);
module_exit(onebyte_exit);
