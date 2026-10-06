// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/mutex.h>
#include <linux/jiffies.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/uaccess.h>

MODULE_AUTHOR("Charlie Bopp");
MODULE_DESCRIPTION("TBD");
MODULE_LICENSE("GPL");

static struct dentry *fortytwo_dir;
static const char login[] = "cbopp\n";
static char *foo_buf;
static size_t foo_len;
static DEFINE_MUTEX(foo_lock);

static ssize_t id_read(struct file *file, char __user *buf,
                             size_t count, loff_t *ppos)
{
        return simple_read_from_buffer(buf, count, ppos, login,
                                       sizeof(login) - 1);
}

static ssize_t id_write(struct file *file, const char __user *buf,
                              size_t count, loff_t *ppos)
{
        char tmp[sizeof(login) - 1];

        if (count != sizeof(login) - 1)
                return -EINVAL;
        if (copy_from_user(tmp, buf, count) != 0)
                return -EFAULT;
        if (memcmp(tmp, login, sizeof(login) - 1) != 0)
                return -EINVAL;
        return count;
}

static const struct file_operations id_fops = {
        .owner = THIS_MODULE,
        .read = id_read,
        .write = id_write,
};

static ssize_t jiffies_read(struct file *file, char __user *buf,
                             size_t count, loff_t *ppos)
{
        char tmp[32];
        int len;

        len = snprintf(tmp, sizeof(tmp), "%lu\n", jiffies);
        return simple_read_from_buffer(buf, count, ppos, tmp, len);
}

static const struct file_operations jiffies_fops = {
        .owner = THIS_MODULE,
        .read = jiffies_read,
};

static ssize_t foo_read(struct file *file, char __user *buf,
                        size_t count, loff_t *ppos)
{
        ssize_t ret;

        mutex_lock(&foo_lock);
        ret = simple_read_from_buffer(buf, count, ppos, foo_buf, foo_len);
        mutex_unlock(&foo_lock);
        return ret;
}

static ssize_t foo_write(struct file *file, const char __user *buf,
                         size_t count, loff_t *ppos)
{
        ssize_t ret;

        if (*ppos >= PAGE_SIZE || count > PAGE_SIZE - *ppos)
            return -ENOSPC;
        mutex_lock(&foo_lock);
        ret = simple_write_to_buffer(foo_buf, PAGE_SIZE, ppos, buf,
                                        count);
        if (ret > 0)
            foo_len = *ppos;
        mutex_unlock(&foo_lock);
        return ret;
}

static const struct file_operations foo_fops = {
        .owner = THIS_MODULE,
        .read = foo_read,
        .write = foo_write,
};

static int __init fortytwo_init(void)
{
        foo_buf = kzalloc(PAGE_SIZE, GFP_KERNEL);
        if (!foo_buf)
            return -ENOMEM;
        fortytwo_dir = debugfs_create_dir("fortytwo", NULL);
        debugfs_create_file("id", 0666, fortytwo_dir, NULL, &id_fops);
        debugfs_create_file("jiffies", 0444, fortytwo_dir, NULL,
                            &jiffies_fops);
        debugfs_create_file("foo", 0644, fortytwo_dir, NULL, &foo_fops);
        pr_info("fortytwo: debugfs module loaded!\n");
        return 0;
}

static void __exit fortytwo_exit(void)
{
        debugfs_remove_recursive(fortytwo_dir);
        kfree(foo_buf);
        pr_info("fortytwo: debugfs module unloaded\n");
}

module_init(fortytwo_init);
module_exit(fortytwo_exit);

