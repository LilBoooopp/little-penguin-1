// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Louis Solofrizzo <louis@ne02ptzero.me>");
MODULE_DESCRIPTION("Takes an input on write, prints in reverse on read");

static ssize_t myfd_read(struct file *fp, char __user *user,
		size_t size, loff_t *offs);
static ssize_t myfd_write(struct file *fp, const char __user *user,
		size_t size, loff_t *offs);

static const struct file_operations myfd_fops = {
	.owner = THIS_MODULE,
	.read = myfd_read,
	.write = myfd_write,
};

static struct miscdevice myfd_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "reverse",
	.fops = &myfd_fops,
};

static char str[PAGE_SIZE];
static char *tmp;
static DEFINE_MUTEX(myfd_lock);

static int __init myfd_init(void)
{
	int retval;

	tmp = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!tmp)
		return -ENOMEM;
	retval = misc_register(&myfd_device);
	if (retval)
		kfree(tmp);
	return retval;
}

static void __exit myfd_cleanup(void)
{
	misc_deregister(&myfd_device);
	kfree(tmp);
}

static ssize_t myfd_read(struct file *fp, char __user *user, size_t size,
		loff_t *offs)
{
	ssize_t res;
	size_t len;
	size_t i;
	bool nl;
	int t;

	mutex_lock(&myfd_lock);
	len = strlen(str);
	nl = len > 0 && str[len - 1] == '\n';
	if (nl)
		len--;
	for (t = (int)len - 1, i = 0; t >= 0; t--, i++)
		tmp[i] = str[t];
	if (nl)
		tmp[i++] = '\n';
	res = simple_read_from_buffer(user, size, offs, tmp, i);
	mutex_unlock(&myfd_lock);
	return res;
}

static ssize_t myfd_write(struct file *fp, const char __user *user, size_t size,
		loff_t *offs)
{
	ssize_t res;

	if (*offs >= sizeof(str) - 1 || size > sizeof(str) - 1 - *offs)
		return -ENOSPC;

	mutex_lock(&myfd_lock);
	res = simple_write_to_buffer(str, sizeof(str) - 1, offs, user,
					size);
	if (res > 0)
		str[*offs] = '\0';
	mutex_unlock(&myfd_lock);
	return res;
}

module_init(myfd_init);
module_exit(myfd_cleanup);

