# Samsung Galaxy Player/S WiFi 3.6 (YP-GS1)
# Ported by Androthan, androthan<at>gmail<dot>com, 2015
# Based on https://github.com/dhiru1602/android_device_samsung_galaxysl/tree/jellybean by dhiru1602 (THANKS!)
#
# Device identification table
#

# Specify phone tech before including full_phone
$(call inherit-product, vendor/cm/config/gsm.mk)

# Release name
PRODUCT_RELEASE_NAME := YP-GS1

# Common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Device configuration
$(call inherit-product, device/samsung/aalto/full_aalto.mk)

# Device identity
PRODUCT_DEVICE := aalto
PRODUCT_NAME := cm_aalto
PRODUCT_BRAND := Samsung
PRODUCT_MODEL := YP-GS1
