// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

MODULE_AUTHOR("Charlie Bopp");
MODULE_DESCRIPTION("Misc device /dev/fortytwo returning the student login");
MODULE_LICENSE("GPL");

static const char login[] = "cbopp\n";

static ssize_t fortytwo_read(struct file *file, char __user *buf,
                             size_t count, loff_t *ppos)
{
        return simple_read_from_buffer(buf, count, ppos, login,
                                       sizeof(login) - 1);
}

static ssize_t fortytwo_write(struct file *file, const char __user *buf,
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

static const struct file_operations fortytwo_fops = {
                .owner = THIS_MODULE,
                .read = fortytwo_read,
                .write = fortytwo_write,
};

static struct miscdevice fortytwo_dev = {
        .minor = MISC_DYNAMIC_MINOR,
        .name = "fortytwo",
        .fops = &fortytwo_fops,
};



static int __init fortytwo_init(void)
{
        int ret;

        ret = misc_register(&fortytwo_dev);
        if (ret)
                return ret;
        pr_info("fortytwo: device registered\n");
        return 0;
}

static void __exit fortytwo_exit(void)
{
        misc_deregister(&fortytwo_dev);
        pr_info("Cleaning up misc device module.\n");
}

module_init(fortytwo_init);
module_exit(fortytwo_exit);
