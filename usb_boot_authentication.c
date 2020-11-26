#include <linux/init.h>
#include <linux/module.h>
#include <linux/usb.h>

#define UBA_MODULE_NAME        "usb_boot_authentication"
#define UBA_MODULE_DESCRIPTION "Hardware authentication for Linux using ordinary USB Flash Drives"
#define UBA_MODULE_AUTHOR      "Ahmed Kerimov <kerimov.dev@yandex.ru>"
#define UBA_MODULE_LICENSE     "GPL"

MODULE_DESCRIPTION(UBA_MODULE_DESCRIPTION);
MODULE_AUTHOR(UBA_MODULE_AUTHOR);
MODULE_LICENSE(UBA_MODULE_LICENSE);

static int uba_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	printk(KERN_INFO "%s: USB flash drive (%04X:%04X) plugged\n", UBA_MODULE_NAME, id->idVendor, id->idProduct);
	return 0;
}

static void uba_disconnect(struct usb_interface *interface)
{
	printk(KERN_INFO "%s: USB flash drive unplugged\n", UBA_MODULE_NAME);
}

#define UBA_VENDOR_ID  0x0781
#define UBA_PRODUCT_ID 0x5591

static struct usb_device_id uba_table[] = {
	{ USB_DEVICE(UBA_VENDOR_ID, UBA_PRODUCT_ID) },
	{ USB_DEVICE(0x093a, 0x2521) },
	{ },
};

MODULE_DEVICE_TABLE(usb, uba_table);

static struct usb_driver uba_driver = {
	.name = UBA_MODULE_NAME,
	.probe = uba_probe,
	.disconnect = uba_disconnect,
	.id_table = uba_table,
};

static int __init usb_boot_authentication_init(void)
{
	int rc;

	if ((rc = usb_register(&uba_driver)) < 0) {
		printk(KERN_ERR "%s: usb_register failed with code %d\n", UBA_MODULE_NAME, rc);
		return rc;
	}

	printk(KERN_INFO "%s: module loaded\n", UBA_MODULE_NAME);
	return 0;
}

static void __exit usb_boot_authentication_exit(void)
{
	usb_deregister(&uba_driver);
	printk(KERN_INFO "%s: module unloaded\n", UBA_MODULE_NAME);
}

module_init(usb_boot_authentication_init)
module_exit(usb_boot_authentication_exit)
