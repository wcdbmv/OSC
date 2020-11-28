#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/reboot.h>
#include <linux/sched/signal.h>
#include <linux/usb.h>

#define UBA_MODULE_NAME        "usb_boot_authentication"
#define UBA_MODULE_DESCRIPTION "Hardware authentication for Linux using ordinary USB Flash Drives"
#define UBA_MODULE_AUTHOR      "Ahmed Kerimov <kerimov.dev@yandex.ru>"
#define UBA_MODULE_LICENSE     "GPL"

MODULE_DESCRIPTION(UBA_MODULE_DESCRIPTION);
MODULE_AUTHOR(UBA_MODULE_AUTHOR);
MODULE_LICENSE(UBA_MODULE_LICENSE);

static struct task_struct *agetty_stop_thread;

static int uba_timer_thread(void *data)
{
	int i;

	for (i = 30; !kthread_should_stop(); --i) {
		printk(KERN_NOTICE "%s: %02d secs to shutdown\n", UBA_MODULE_NAME, i);
		printk(KERN_NOTICE "%s: plug in usb key to start up\n", UBA_MODULE_NAME);

		ssleep(1);
		if (i <= 0) {
			kernel_power_off();
		}
	}

	printk(KERN_NOTICE "%s: uba_timer_thread stopped\n", UBA_MODULE_NAME);

	return 0;
}

static int uba_agetty_stop_thread(void * data)
{
	bool timer_started;
	struct task_struct *task, *timer_thread;

	timer_started = false;

	while (!kthread_should_stop()) {
		for_each_process(task) {
			if (strcmp(task->comm, "agetty") == 0) {
				task->state = TASK_UNINTERRUPTIBLE;
				if (!timer_started) {
					timer_thread = kthread_run(uba_timer_thread, NULL, "uba_timer_thread");
					timer_started = true;
				}
			}
		}
		ssleep(1);
	}

	printk(KERN_NOTICE "%s: uba_agetty_stop_thread stopped\n", UBA_MODULE_NAME);

	kthread_stop(timer_thread);

	return 0;
}

#define UBA_VENDOR_ID  0x0781
#define UBA_PRODUCT_ID 0x5591
#define UBA_SERIAL "4C530001301105102492"

static int uba_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_device *dev;
	struct task_struct *task;

	dev = interface_to_usbdev(interface);

	printk(KERN_NOTICE "%s: USB flash drive (0x%04X:0x%04X) [Serial=%s] plugged\n", UBA_MODULE_NAME, id->idVendor, id->idProduct, dev->serial);
	if (strcmp(dev->serial, UBA_SERIAL)) {
		printk(KERN_NOTICE "%s: The usb serial does not match\n", UBA_MODULE_NAME);
	}

	kthread_stop(agetty_stop_thread);

	for_each_process(task) {
		if (strcmp(task->comm, "agetty") == 0) {
			task->state = TASK_INTERRUPTIBLE;
		}
	}

	return 0;
}

static void uba_disconnect(struct usb_interface *interface)
{
	printk(KERN_NOTICE "%s: USB flash drive unplugged\n", UBA_MODULE_NAME);
}

static struct usb_device_id uba_table[] = {
	{ USB_DEVICE(UBA_VENDOR_ID, UBA_PRODUCT_ID) },
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

	if (IS_ERR(agetty_stop_thread = kthread_run(uba_agetty_stop_thread, NULL, "uba_agetty_stop_thread"))) {
		rc = (int) PTR_ERR(agetty_stop_thread);
		printk(KERN_ERR "%s: kthread_run failed with code %d\n", UBA_MODULE_NAME, rc);
		return rc;
	}

	if ((rc = usb_register(&uba_driver)) < 0) {
		printk(KERN_ERR "%s: usb_register failed with code %d\n", UBA_MODULE_NAME, rc);
		return rc;
	}

	printk(KERN_DEBUG "%s: module loaded\n", UBA_MODULE_NAME);
	return 0;
}

static void __exit usb_boot_authentication_exit(void)
{
	usb_deregister(&uba_driver);
	printk(KERN_DEBUG "%s: module unloaded\n", UBA_MODULE_NAME);
}

module_init(usb_boot_authentication_init)
module_exit(usb_boot_authentication_exit)
