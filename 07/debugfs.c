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

// static const struct file_operations foo_fops = {
//         .owner = THIS_MODULE,
//         .read = ,
//         .write = ,
// };

static int __init fortytwo_init(void)
{
        fortytwo_dir = debug_fs_create_dir("fortytwo", NULL);
        debugfs_create_file("id", 0666, fortytwo_dir, NULL, &id_fops);
        debugfs_create_file("jiffies", 0444, fortytwo_dir, NULL,
                            &jiffies_fops);
        pr_info("fortytwo: debugfs module loaded!\n");
        return 0;
}

static void __exit fortytwo_exit(void)
{
        debugfs_remove_recursive(fortytwo_dir);
        pr_info("fortytwo: debugfs module unloaded");
}

module_init(fortytwo_init);
module_exit(fortytwo_exit);

