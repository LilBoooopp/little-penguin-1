#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>

MODULE_AUTHOR("Charlie Bopp");
MODULE_DESCRIPTION("Hello world");
MODULE_LICENSE("GPL");

static int __init init(void)
{
    pr_info("Hello world!");
    return (0);
}

static void __exit exit(void)
{
    pr_debug("Cleaning module\n");
}

module_init(init);
module_exit(exit);
