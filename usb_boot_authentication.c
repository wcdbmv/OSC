#include <linux/init.h>
#include <linux/module.h>

#define UBA_MODULE_NAME        "usb_boot_authentication"
#define UBA_MODULE_DESCRIPTION "Hardware authentication for Linux using ordinary USB Flash Drives"
#define UBA_MODULE_AUTHOR      "Ahmed Kerimov <kerimov.dev@yandex.ru>"
#define UBA_MODULE_LICENSE     "GPL"

MODULE_DESCRIPTION(UBA_MODULE_DESCRIPTION);
MODULE_AUTHOR(UBA_MODULE_AUTHOR);
MODULE_LICENSE(UBA_MODULE_LICENSE);

static int __init usb_boot_authentication_init(void)
{
	printk(KERN_INFO "%s: module loaded\n", UBA_MODULE_NAME);
	return 0;
}

static void __exit usb_boot_authentication_exit(void)
{
	printk(KERN_INFO "%s: module unloaded\n", UBA_MODULE_NAME);
}

module_init(usb_boot_authentication_init)
module_exit(usb_boot_authentication_exit)
