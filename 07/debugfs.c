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

static struct dentry *debug_fs_create_dir("fortytwo", NULL);

static const char login[] = "cbopp\n";

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

static const struct file_operations jiffies_fops = {
        .owner = THIS_MODULE,
        .read = ,
        .write = ,
};

static const struct file_operations foo_fops = {
        .owner = THIS_MODULE,
        .read = ,
        .write = ,
};

static int __init fortytwo_init(void)
{
        int ret;

        if (ret) {
                pr_info("debugfs: debugfs_create_file - id error\n");
                return ret;
        }
        pr_info("debugfs: module running!\n");
        return 0;
}

static void __exit fortytwo_exit(void)
{
        debugfs_remove_recursive("fortytwo");
        pr_info("debugfs: Cleaning up debugfs module.\n");
}

module_init(fortytwo_init);
module_exit(fortytwo_exit);

