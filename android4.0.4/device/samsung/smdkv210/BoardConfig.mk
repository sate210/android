#
# Copyright (C) 2011 The Android Open-Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# These two variables are set first, so they can be overridden
# by BoardConfigVendor.mk
BOARD_USES_GENERIC_AUDIO := false 
USE_CAMERA_STUB := false

TARGET_BOARD_PLATFORM := smdkv210


TARGET_NO_BOOTLOADER := true	#uses u-boot instead
TARGET_NO_KERNEL := true
TARGET_NO_RECOVERY := true
TARGET_NO_RADIOIMAGE := true
# TARGET_CPU_SMP := true
TARGET_ARCH_VARIANT := armv7-a-neon
ARCH_ARM_HAVE_TLS_REGISTER := true

BOARD_HAVE_BLUETOOTH := true

TARGET_BOOTLOADER_BOARD_NAME := smdkv210

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi

BOARD_EGL_CFG := device/samsung/smdkv210/egl.cfg

USE_OPENGL_RENDERER := true

BOARD_NAND_PAGE_SIZE := 2048
BOARD_NAND_SPARE_SIZE := 64

#TARGET_USERIMAGES_USE_EXT4 := true
#TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true
#BOARD_SYSTEMIMAGE_PARTITION_SIZE := 268435456
#BOARD_USERDATAIMAGE_PARTITION_SIZE := 536870912
#BOARD_FLASH_BLOCK_SIZE := 4096

#ifeq ($(BOARD_USES_GENERIC_AUDIO),false)
#BOARD_USES_ALSA_AUDIO := true
#BUILD_WITH_ALSA_UTILS := true
#BOARD_USES_CIRCULAR_BUF_AUDIO := true
#USE_ULP_AUDIO := true
#endif

ifeq ($(USE_CAMERA_STUB),false)
BOARD_CAMERA_LIBRARIES := libcamera
endif

S5P_BOARD_USES_HDMI := true
S5P_BOARD_USES_HDMI_SUBTITLES := true
ifeq ($(S5P_BOARD_USES_HDMI),true)
BOARD_HDMI_STD := STD_720P
S5P_BOARD_USES_HDMI_SUBTITLES := false
endif

DEFAULT_FB_NUM	:= 2


##########################the following ware added by fightger_cui#############################

# Video Devices
#BOARD_USES_OVERLAY := true
#BOARD_V4L2_DEVICE := /dev/video1
#BOARD_CAMERA_DEVICE := /dev/video0
#BOARD_SECOND_CAMERA_DEVICE := /dev/video2


WPA_SUPPLICANT_VERSION := VER_0_6_X
BOARD_WPA_SUPPLICANT_DRIVER := WEXT

WPA_BUILD_SUPPLICANT := true
CONFIG_CTRL_IFACE := y

#WIFI_BCM4329 := true
 
#for bcm4329
ifeq ($(WIFI_BCM4329),true)
BOARD_WLAN_DEVICE := bcm4329
WIFI_DRIVER_MODULE_PATH     := "/system/modules/bcm4329.ko"
WIFI_DRIVER_FW_STA_PATH     := "/vendor/firmware/fw_bcm4329.bin"
WIFI_DRIVER_FW_AP_PATH      := "/vendor/firmware/fw_bcm4329_apsta.bin"
WIFI_DRIVER_MODULE_NAME     :=  "bcm4329"
WIFI_DRIVER_MODULE_ARG      :=  "firmware_path=/vendor/firmware/fw_bcm4329.bin nvram_path=/vendor/firmware/nvram_net.txt"
endif

WIFI_SD8787 := true
#for marval 88W8787
ifeq ($(WIFI_SD8787),true)
BOARD_WLAN_DEVICE := sd8xxx
WIFI_DRIVER_MODULE_PATH     := "/system/modules/sd8xxx.ko"
WIFI_DRIVER_FW_STA_PATH     := "/etc/firmware/mrvl/sd8787_uapsta.bin"
WIFI_DRIVER_MODULE_NAME     :=  "sd8xxx"
endif

#FOR 3G MODULE
USB_3G := true
#MF210 := true
