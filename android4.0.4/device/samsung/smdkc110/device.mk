# These is the hardware-specific overlay, which points to the location
# of hardware-specific resource overrides, typically the frameworks and
# application settings that are stored in resourced.
include $(LOCAL_PATH)/BoardConfig.mk

ifeq ($(BOARD_SDMMC_BSP),true)
source_init_rc_file := $(LOCAL_PATH)/init.smdkc110_sdmmc.rc
else
source_init_rc_file := $(LOCAL_PATH)/init.smdkc110.rc
endif

DEVICE_PACKAGE_OVERLAYS := device/samsung/smdkc110/overlay

PRODUCT_COMMON_DIR := device/samsung/common/s5p

PRODUCT_COPY_FILES := \
	$(source_init_rc_file):root/init.smdkc110.rc \
	device/samsung/smdkc110/init.smdkc110.usb.rc:root/init.smdkc110.usb.rc \
	device/samsung/smdkc110/ueventd.smdkc110.rc:root/ueventd.smdkc110.rc \
	device/samsung/smdkc110/s3c-keypad.kl:system/usr/keylayout/s3c-keypad.kl \
        device/samsung/smdkc110/s3c-keypad.kcm:system/usr/keychars/s3c-keypad.kcm \
        device/samsung/smdkc110/s3c_ts.idc:system/usr/idc/s3c_ts.idc \
	device/samsung/smdkc110/vold.fstab:system/etc/vold.fstab

PRODUCT_PROPERTY_OVERRIDES += \
	ro.sf.lcd_density=240 \
	ro.opengles.version=131072

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
        persist.sys.usb.config=mass_storage

PRODUCT_PROPERTY_OVERRIDES += \
        hwui.render_dirty_regions=false

PRODUCT_TAGS += dalvik.gc.type-precise

#audio
PRODUCT_PACKAGES += \
        audio_policy.smdkc110 \
        audio.primary.smdkc110 \
        audio.a2dp.default \
        lights.smdkc110 \
        gralloc.smdkc110 \
	hwcomposer.smdkc110 \
        libaudioutils

# These is the OpenMAX IL configuration files
PRODUCT_COPY_FILES += \
        $(PRODUCT_COMMON_DIR)/sec_mm/sec_omx/sec_omx_core/secomxregistry:system/etc/secomxregistry \
	$(PRODUCT_COMMON_DIR)/media_profiles.xml:system/etc/media_profiles.xml

#MFC Firmware
PRODUCT_COPY_FILES += \
        $(PRODUCT_COMMON_DIR)/samsung_mfc_fw.bin:system/vendor/firmware/samsung_mfc_fw.bin

# These are the OpenMAX IL modules
PRODUCT_PACKAGES += \
        libSEC_OMX_Core \
        libOMX.SEC.AVC.Decoder \
        libOMX.SEC.M4V.Decoder \
        libOMX.SEC.M4V.Encoder \
        libOMX.SEC.AVC.Encoder

# Include libstagefright module
PRODUCT_PACKAGES += \
        libstagefrighthw
# Camera
PRODUCT_PACKAGES += \
	camera.smdkc110

# Filesystem management tools
PRODUCT_PACKAGES += \
	make_ext4fs \
	setup_fs

$(call inherit-product, frameworks/base/build/phone-xhdpi-1024-dalvik-heap.mk)
$(call inherit-product-if-exists, vendor/samsung/smdkc110/device-vendor.mk)
