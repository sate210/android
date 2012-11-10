# These is the hardware-specific overlay, which points to the location
# of hardware-specific resource overrides, typically the frameworks and
# application settings that are stored in resourced.
DEVICE_PACKAGE_OVERLAYS := device/samsung/smdkv210/overlay

PRODUCT_COMMON_DIR := device/samsung/common/s5p

PRODUCT_COPY_FILES := \
	device/samsung/smdkv210/init.smdkv210.rc:root/init.smdkv210.rc \
	device/samsung/smdkv210/init.smdkv210.usb.rc:root/init.smdkv210.usb.rc \
	device/samsung/smdkv210/ueventd.smdkv210.rc:root/ueventd.smdkv210.rc \
	device/samsung/smdkv210/s3c-keypad.kl:system/usr/keylayout/s3c-keypad.kl \
  device/samsung/smdkv210/smdkv210_tp.idc:system/usr/idc/smdkv210_tp.idc \
	device/samsung/smdkv210/vold.fstab:system/etc/vold.fstab	\
	device/samsung/smdkv210/vold.conf:system/etc/vold.conf	
	
#    device/samsung/smdkv210/s3c-keypad.kcm:system/usr/keychars/s3c-keypad.kcm

PRODUCT_PROPERTY_OVERRIDES += \
	ro.sf.lcd_density=160
#	ro.opengles.version=131072

# Set default USB interface
PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
        persist.sys.usb.config=mass_storage

PRODUCT_PROPERTY_OVERRIDES += \
        hwui.render_dirty_regions=false

PRODUCT_TAGS += dalvik.gc.type-precise

PRODUCT_PACKAGES += \
	gralloc.smdkv210 

#audio
PRODUCT_PACKAGES += \
        audio_policy.smdkv210 \
        audio.primary.smdkv210 \
        audio.a2dp.default \
        lights.smdkv210 \
        hwcomposer.smdkv210 \
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
	camera.smdkv210

# Filesystem management tools
PRODUCT_PACKAGES += \
	make_ext4fs \
	setup_fs


#ifeq ($(WIFI_SD8787),true)
PRODUCT_COPY_FILES += \
  	$(LOCAL_PATH)/wifi/88W8787/sd8787_uapsta.bin:system/etc/firmware/mrvl/sd8787_uapsta.bin \
    $(LOCAL_PATH)/wifi/88W8787/sd8xxx.ko:system/modules/sd8xxx.ko \
    $(LOCAL_PATH)/wifi/88W8787/mlan.ko:system/modules/mlan.ko \
    $(LOCAL_PATH)/wifi/88W8787/bt8xxx.ko:system/modules/bt8xxx.ko \
    $(LOCAL_PATH)/wifi/88W8787/wpa_supplicant.conf:system/etc/wifi/wpa_supplicant.conf
#endif
#for app

	
# for tools
PRODUCT_COPY_FILES += \
	$(LOCAL_PATH)/tools/busybox:root/sbin/busybox 
#	$(LOCAL_PATH)/tools/picocom:root/sbin/picocom \
#	$(LOCAL_PATH)/tools/usb_modeswitch-1.1.9-arm-static:root/sbin/usb_modeswitch 

#for usb 3g
USB_3G := true
#MF210 := true

ifeq ($(USB_3G),true)
 PRODUCT_PROPERTY_OVERRIDES += ro.telephony.default_network=0
 $(call inherit-product, $(LOCAL_PATH)/3G/USB3G/copy_file.mk)
endif	

ifeq ($(MF210),true)
PRODUCT_COPY_FILES += \
  	$(LOCAL_PATH)/3G/ZTE/MF210/libreference-ril.so:system/lib/libreference-ril.so  \
  	$(LOCAL_PATH)/3G/ZTE/MF210/init.gprs-pppd:system/etc/init.gprs-pppd  
endif
#########################################
# These are the hardware-specific features
PRODUCT_COPY_FILES += \
	frameworks/base/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml \
	frameworks/base/data/etc/android.hardware.camera.flash-autofocus.xml:system/etc/permissions/android.hardware.camera.flash-autofocus.xml \
	frameworks/base/data/etc/android.hardware.camera.front.xml:system/etc/permissions/android.hardware.camera.front.xml \
	frameworks/base/data/etc/android.hardware.location.gps.xml:system/etc/permissions/android.hardware.location.gps.xml \
	frameworks/base/data/etc/android.hardware.wifi.xml:system/etc/permissions/android.hardware.wifi.xml \
	frameworks/base/data/etc/android.hardware.sensor.light.xml:system/etc/permissions/android.hardware.sensor.light.xml \
	frameworks/base/data/etc/android.hardware.sensor.gyroscope.xml:system/etc/permissions/android.hardware.sensor.gyroscope.xml \
	frameworks/base/data/etc/android.hardware.touchscreen.multitouch.jazzhand.xml:system/etc/permissions/android.hardware.touchscreen.multitouch.jazzhand.xml \
	frameworks/base/data/etc/android.software.sip.voip.xml:system/etc/permissions/android.software.sip.voip.xml \
	frameworks/base/data/etc/android.hardware.usb.accessory.xml:system/etc/permissions/android.hardware.usb.accessory.xml \
	packages/wallpapers/LivePicker/android.software.live_wallpaper.xml:system/etc/permissions/android.software.live_wallpaper.xml \
	frameworks/base/data/etc/android.hardware.usb.host.xml:system/etc/permissions/android.hardware.usb.host.xml 

#	
#	frameworks/base/data/etc/android.hardware.sensor.proximity.xml:system/etc/permissions/android.hardware.sensor.proximity.xml \	
#	frameworks/base/data/etc/android.hardware.sensor.barometer.xml:system/etc/permissions/android.hardware.sensor.barometer.xml \	
#frameworks/base/data/etc/android.hardware.wifi.direct.xml:system/etc/permissions/android.hardware.wifi.direct.xml 
#	frameworks/base/data/etc/android.hardware.nfc.xml:system/etc/permissions/android.hardware.nfc.xml 

BOARD_USES_HIGH_RESOLUTION_LCD := true

ifeq ($(BOARD_USES_HIGH_RESOLUTION_LCD),true)
PRODUCT_CHARACTERISTICS := tablet
PRODUCT_COPY_FILES += \
frameworks/base/data/etc/tablet_core_hardware.xml:system/etc/permissions/tablet_core_hardware.xml
$(call inherit-product, frameworks/base/build/tablet-dalvik-heap.mk) 
else 
PRODUCT_CHARACTERISTICS := phone
PRODUCT_COPY_FILES += \ 
frameworks/base/data/etc/handheld_core_hardware.xml:system/etc/permissions/handheld_core_hardware.xml 
$(call inherit-product, frameworks/base/build/phone-hdpi-512-dalvik-heap.mk)
PRODUCT_PROPERTY_OVERRIDES +=\ 
ro.sf.lcd_density=240     \
PRODUCT_AAPT_CONFIG := normal hdpi 
endif


# Set default USB interface
#	PRODUCT_DEFAULT_PROPERTY_OVERRIDES += \
#	persist.sys.usb.config=mtp
#########################################
$(call inherit-product, frameworks/base/build/phone-xhdpi-1024-dalvik-heap.mk)
$(call inherit-product-if-exists, vendor/samsung/smdkv210/device-vendor.mk)


