#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define MAJOR_NUMBER 61

/* forward declaration */
int onebyte_open(struct inode *inode, struct file *filep);
int onebyte_release(struct inode *inode, struct file *filep);
ssize_t onebyte_read(struct file * filep, char *buf, size_t count, loff_t *f_pos);
ssize_t onebyte_write(struct file * filep, const char *buf, size_t count, loff_t *f_pos);
static void onebyte_exit(void);
loff_t onebyte_lseek (struct file * filep, loff_t f_pos, int whence);

/* definition of file_operation structure */
struct file_operations onebyte_fops = {
	llseek:  onebyte_lseek,
	read:	onebyte_read,
	write:	onebyte_write,
	open:	onebyte_open,
	release: onebyte_release
};

char *fourmegabyte_data = NULL;

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
			newpos = 4*1024*1024 + off;
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
