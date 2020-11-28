SHELL = /bin/bash

UBA_MODULE_NAME = usb_boot_authentication

obj-m += $(UBA_MODULE_NAME).o
UBA_LKM = $(UBA_MODULE_NAME).ko

KERNEL_DIR = /lib/modules/$(shell uname -r)
KERNEL_BUILD_DIR = $(KERNEL_DIR)/build
KERNEL_DRIVERS_UBA_DIR = $(KERNEL_DIR)/kernel/drivers/$(UBA_MODULE_NAME)

GRUB_CONFIG = /etc/default/grub
GRUB_CONFIG_BACKUP = $(GRUB_CONFIG).backup

RSYSLOG_CONFIG = /etc/rsyslog.d/10-usb-boot-authentication.conf


.PHONY: all default install modules modules_install help clean \
	enable_at_boot \
	disable_at_boot \
	boot_in_console_mode \
	boot_in_gui_mode \
	enable_printing_kernel_journal_on_tty \
	disable_printing_kernel_journal_on_tty \
	enable_rc_local \
	disable_rc_local


all default: modules
install: modules_install


modules modules_install help clean:
	make -C $(KERNEL_BUILD_DIR) M=$(shell pwd) $@


enable_at_boot: modules $(KERNEL_DRIVERS_UBA_DIR)
ifneq ($(shell grep $(UBA_MODULE_NAME) /etc/modules),)
	$(error module already loaded)
endif

	cp $(UBA_LKM) $(KERNEL_DRIVERS_UBA_DIR)
	echo $(UBA_MODULE_NAME) >> /etc/modules
	depmod


disable_at_boot:
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


boot_in_console_mode: $(GRUB_CONFIG_BACKUP)
	sed --in-place 's/^GRUB_CMDLINE_LINUX_DEFAULT=\(.*\)$$/#GRUB_CMDLINE_LINUX_DEFAULT=\1/' $(GRUB_CONFIG)
	sed --in-place 's/^GRUB_CMDLINE_LINUX=.*$$/GRUB_CMDLINE_LINUX=text/' $(GRUB_CONFIG)
	sed --in-place 's/^#GRUB_TERMINAL=.*$$/GRUB_TERMINAL=console/' $(GRUB_CONFIG)
	update-grub
	systemctl set-default multi-user.target


boot_in_gui_mode:
	mv $(GRUB_CONFIG_BACKUP) $(GRUB_CONFIG)
	update-grub
	systemctl set-default graphical.target


enable_printing_kernel_journal_on_tty:
	echo 'kern.* /dev/tty1' > $(RSYSLOG_CONFIG)


disable_printing_kernel_journal_on_tty:
	rm -f $(RSYSLOG_CONFIG)


enable_rc_local: config/rc-local.service
	echo -e '#!/bin/bash\n\nsleep 31' > /etc/rc.local
	cp $< /etc/systemd/system/
	systemctl enable rc-local


disable_rc_local:
	rm -f /etc/rc.local /etc/systemd/system/rc-local.service
	systemctl disable rc-local


$(KERNEL_DRIVERS_UBA_DIR):
	mkdir -p $@


$(GRUB_CONFIG_BACKUP):
	cp --no-clobber $(GRUB_CONFIG) $@
