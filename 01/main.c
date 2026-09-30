// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>

MODULE_AUTHOR("Charlie Bopp");
MODULE_DESCRIPTION("Hello world");
MODULE_LICENSE("GPL");

static int __init main_init(void)
{
        pr_info("Hello world!\n");
        return 0;
}

static void __exit main_exit(void)
{
        pr_info("Cleaning up module.\n");
}

module_init(main_init);
module_exit(main_exit);
