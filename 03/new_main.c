// SPDX-License-Identifier: GPL-2.0
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("If long delay, tell userspace");
MODULE_AUTHOR("Charlie Bopp");

static int do_work(int *my_int)
{
        int x;
        int y = *my_int;
        int z;

        for (x = 0; x < y; ++x)
                udelay(10);

        /* That was a long sleep, tell userspace about it */
        if (y > 10)
                pr_info("We slept a long time!\n");

        z = x * y;
        return z;
}

static int __init my_init(void)
{
        int x = 10;

        x = do_work(&x);
        return 0;
}

static void __exit my_exit(void)
{
}

module_init(my_init);
module_exit(my_exit);
