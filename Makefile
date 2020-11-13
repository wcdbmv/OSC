SHELL = /bin/bash

UBA_MODULE_NAME = usb_boot_authentication

obj-m += $(UBA_MODULE_NAME).o
UBA_LKM = $(UBA_MODULE_NAME).ko

KERNEL_DIR = /lib/modules/$(shell uname -r)
KERNEL_BUILD_DIR = $(KERNEL_DIR)/build
KERNEL_DRIVERS_UBA_DIR = $(KERNEL_DIR)/kernel/drivers/$(UBA_MODULE_NAME)


all default: modules
install: modules_install


modules modules_install help clean:
	make -C $(KERNEL_BUILD_DIR) M=$(shell pwd) $@


_insmod: modules $(KERNEL_DRIVERS_UBA_DIR)
ifneq ($(shell grep $(UBA_MODULE_NAME) /etc/modules),)
	$(error module already loaded)
endif

	cp $(UBA_LKM) $(KERNEL_DRIVERS_UBA_DIR)
	echo $(UBA_MODULE_NAME) >> /etc/modules
	depmod


_rmmod:
ifeq ($(shell grep $(UBA_MODULE_NAME) /etc/modules),)
	$(error module already unloaded)
endif

ifneq ($(shell lsmod | grep $(UBA_MODULE_NAME)),)
	rmmod $(UBA_LKM)
endif

	rm -f $(KERNEL_DRIVERS_UBA_DIR)/$(UBA_LKM)
	rmdir $(KERNEL_DRIVERS_UBA_DIR)
	sed --in-place '/$(UBA_MODULE_NAME)/d' /etc/modules
	depmod


$(KERNEL_DRIVERS_UBA_DIR):
	mkdir -p $@
