LOCAL_PATH := $(call my-dir) 

MASTER_BOOTSTRAP := $(PRODUCT_OUT)/bootstrap.cpio.gz
$(MASTER_BOOTSTRAP): $(INSTALLED_RAMDISK_TARGET) $(recovery_ramdisk) $(HOST_OUT)/bin/mkbootfs $(PRODUCT_OUT)/utilities/busybox
	$(call pretty,"Bootstrap: $@")
	cp -r device/samsung/aalto/bootstrap $(PRODUCT_OUT)
	cp -r device/samsung/aalto/bootstrap.sh $(PRODUCT_OUT)
	$(PRODUCT_OUT)/bootstrap.sh

INSTALLED_BOOTIMAGE_TARGET := $(PRODUCT_OUT)/boot.img
$(INSTALLED_BOOTIMAGE_TARGET): $(TARGET_PREBUILT_KERNEL) $(MASTER_BOOTSTRAP)
	$(call pretty,"Boot image: $@")
	$(HOST_OUT)/bin/mkbootimg --kernel $(TARGET_PREBUILT_KERNEL) --ramdisk $(MASTER_BOOTSTRAP) --cmdline "$(BOARD_KERNEL_CMDLINE)" --base $(BOARD_KERNEL_BASE) --pagesize $(BOARD_KERNEL_PAGESIZE) --ramdiskaddr 0x82800000 --output $@

$(INSTALLED_RECOVERYIMAGE_TARGET): $(INSTALLED_BOOTIMAGE_TARGET)
	$(ACP) $(INSTALLED_BOOTIMAGE_TARGET) $@
