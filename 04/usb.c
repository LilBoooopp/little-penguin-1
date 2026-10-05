// SPDX-License-Identifier: GPL-2.0
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/usb.h>
#include <linux/hid.h>

MODULE_AUTHOR("Charlie Bopp");
MODULE_DESCRIPTION("Loaded when a USB keyboard is plugged in.");
MODULE_LICENSE("GPL");
static const struct usb_device_id my_table[] = {
        { USB_INTERFACE_INFO(USB_INTERFACE_CLASS_HID,
                             USB_INTERFACE_SUBCLASS_BOOT,
                             USB_INTERFACE_PROTOCOL_KEYBOARD) },
        { }
};
MODULE_DEVICE_TABLE(usb, my_table);

static int __init main_init(void)
{
        pr_info("USB module running!\n");
        return 0;
}

static void __exit main_exit(void)
{
        pr_info("Cleaning up USB module.\n");
}

module_init(main_init);
module_exit(main_exit);
