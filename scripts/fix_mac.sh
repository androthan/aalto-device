#!/system/bin/sh
if [ ! -f /data/misc/wifi/nvs_map.bin ]; then
	insmod /system/lib/modules/tiwlan_drv.ko
	/system/bin/tiwlan_loader -i /system/etc/wifi/tiwlan.ini -f /system/etc/wifi/firmware.bin
	/system/bin/tiwlan_plts -n
	rmmod tiwlan_drv
fi
