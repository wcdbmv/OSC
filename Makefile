SHELL = /bin/bash

UBA_MODULE_NAME = usb_boot_authentication

obj-m += $(UBA_MODULE_NAME).o
UBA_LKM = $(UBA_MODULE_NAME).ko

KERNEL_DIR = /lib/modules/$(shell uname -r)
KERNEL_BUILD_DIR = $(KERNEL_DIR)/build
KERNEL_DRIVERS_UBA_DIR = $(KERNEL_DIR)/kernel/drivers/$(UBA_MODULE_NAME)

GRUB_CONFIG = /etc/default/grub
GRUB_CONFIG_BACKUP = $(GRUB_CONFIG).backup


all default: modules
install: modules_install


modules modules_install help clean:
	make -C $(KERNEL_BUILD_DIR) M=$(shell pwd) $@


__insmod: modules $(KERNEL_DRIVERS_UBA_DIR)
ifneq ($(shell grep $(UBA_MODULE_NAME) /etc/modules),)
	$(error module already loaded)
endif

	cp $(UBA_LKM) $(KERNEL_DRIVERS_UBA_DIR)
	echo $(UBA_MODULE_NAME) >> /etc/modules
	depmod


__rmmod:
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


__boot_console: $(GRUB_CONFIG_BACKUP)
	sed --in-place 's/^GRUB_CMDLINE_LINUX_DEFAULT=\(.*\)$$/#GRUB_CMDLINE_LINUX_DEFAULT=\1/' $(GRUB_CONFIG)
	sed --in-place 's/^GRUB_CMDLINE_LINUX=.*$$/GRUB_CMDLINE_LINUX=text/' $(GRUB_CONFIG)
	sed --in-place 's/^#GRUB_TERMINAL=.*$$/GRUB_TERMINAL=console/' $(GRUB_CONFIG)
	update-grub
	systemctl set-default multi-user.target


__boot_gui:
	mv $(GRUB_CONFIG_BACKUP) $(GRUB_CONFIG)
	update-grub
	systemctl set-default graphical.target


$(KERNEL_DRIVERS_UBA_DIR):
	mkdir -p $@


$(GRUB_CONFIG_BACKUP):
	cp --no-clobber $(GRUB_CONFIG) $@
