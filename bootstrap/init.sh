#!/stage1/busybox sh
export PATH=/stage1

# Bootstrap for Samsung YP-GS1
# Trial Boot: /system - /recovery - /lpm
#
busybox cd /
busybox date >>boot.txt
exec >>boot.txt 2>&1
busybox rm -fr init
busybox mkdir -p /proc
busybox mkdir -p /sys
busybox mount -t proc proc /proc
busybox mount -t sysfs sysfs /sys

# Android ramdisk
RAMDISK=ramdisk-system.cpio.gz

# Recovery boot
if busybox grep -q bootmode=2 /proc/cmdline || busybox grep -q androidboot.mode=reboot_recovery /proc/cmdline ; then
    RAMDISK=ramdisk-recovery.cpio.gz
fi

busybox gunzip -c ${RAMDISK} | busybox cpio -i

# Low power mode
if busybox grep -q bootmode=5 /proc/cmdline || busybox grep -q androidboot.mode=usb_charger /proc/cmdline ; then
    busybox cp lpm.rc init.rc
	busybox rm init.aalto.rc
fi

busybox umount /sys
busybox umount /proc
busybox date >>boot.txt

busybox rm -fr ramdisk-system.cpio.gz
busybox rm -fr ramdisk-recovery.cpio.gz

busybox rm -fr /stage1 /dev/*

exec /init
