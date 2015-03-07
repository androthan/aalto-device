## Specify phone tech before including full_phone
$(call inherit-product, vendor/cm/config/gsm.mk)

# Release name
PRODUCT_RELEASE_NAME := YP-GS1

# Inherit some common CM stuff.
$(call inherit-product, vendor/cm/config/common_full_phone.mk)

# Inherit device configuration
$(call inherit-product, device/samsung/aalto/full_aalto.mk)

## Device identifier. This must come after all inclusions
PRODUCT_DEVICE := aalto
PRODUCT_NAME := cm_aalto
PRODUCT_BRAND := Samsung
PRODUCT_MODEL := YP-GS1
