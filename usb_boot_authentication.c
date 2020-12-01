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

static bool program_has_terminated;
static struct task_struct *suspend_agetty_process;

static bool uba_set_task_state(const char *comm, long state)
{
	struct task_struct *task;

	for_each_process(task) {
		if (!strcmp(task->comm, comm)) {
			task->state = state;
			return true;
		}
	}

	return false;
}

static void uba_stop_thread(struct task_struct **thread)
{
	kthread_stop(*thread);
	*thread = NULL;
}

static int uba_suspend_agetty_process(void *data)
{
	int i;

	while (!uba_set_task_state("agetty", TASK_UNINTERRUPTIBLE) && !kthread_should_stop()) {
		ssleep(1);
	}

	for (i = 30; i >= 0 && !kthread_should_stop(); --i) {
		printk(KERN_NOTICE "%s: %02d secs to shutdown\n", UBA_MODULE_NAME, i);
		printk(KERN_NOTICE "%s: plug in the USB key to start\n", UBA_MODULE_NAME);
		ssleep(1);
	}
	if (i < 0) {
		kernel_power_off();
	}

	printk(KERN_DEBUG "%s: uba_suspend_agetty_process stopped\n", UBA_MODULE_NAME);
	return 0;
}

#define UBA_VENDOR_ID  0x0781
#define UBA_PRODUCT_ID 0x5591
#define UBA_SERIAL "4C530001301105102492"

static int uba_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
	struct usb_device *dev;

	dev = interface_to_usbdev(interface);
	printk(KERN_NOTICE "%s: USB flash drive plugged\n", UBA_MODULE_NAME);
	printk(KERN_NOTICE "%s: VID: 0x%04X, PID: 0x%04X, Serial: %s\n", UBA_MODULE_NAME, id->idVendor, id->idProduct, dev->serial);

	if (!program_has_terminated) {
		if (strcmp(dev->serial, UBA_SERIAL)) {
			printk(KERN_NOTICE "%s: The serial of the USB flash drive doesn't match\n", UBA_MODULE_NAME);
			return -1;
		}

		uba_stop_thread(&suspend_agetty_process);
		uba_set_task_state("agetty", TASK_INTERRUPTIBLE);

		printk(KERN_NOTICE "%s: USB boot authentication completed successfully\n", UBA_MODULE_NAME);
		program_has_terminated = true;
	}

	return 0;
}

static void uba_disconnect(struct usb_interface *interface)
{
	printk(KERN_DEBUG "%s: USB flash drive unplugged\n", UBA_MODULE_NAME);
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

	program_has_terminated = false;

	if ((rc = usb_register(&uba_driver)) < 0) {
		printk(KERN_ERR "%s: usb_register failed with code %d\n", UBA_MODULE_NAME, rc);
		return rc;
	}

	if (IS_ERR(suspend_agetty_process = kthread_run(uba_suspend_agetty_process, NULL, "uba_suspend_agetty_process"))) {
		rc = (int) PTR_ERR(suspend_agetty_process);
		printk(KERN_ERR "%s: kthread_run failed with code %d\n", UBA_MODULE_NAME, rc);
		return rc;
	}

	printk(KERN_DEBUG "%s: module loaded\n", UBA_MODULE_NAME);
	return 0;
}

static void __exit usb_boot_authentication_exit(void)
{
	if (suspend_agetty_process) {
		uba_stop_thread(&suspend_agetty_process);
	}

	usb_deregister(&uba_driver);

	printk(KERN_DEBUG "%s: module unloaded\n", UBA_MODULE_NAME);
}

module_init(usb_boot_authentication_init)
module_exit(usb_boot_authentication_exit)
