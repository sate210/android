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
#BOARD_SDMMC_BSP := true
USE_CAMERA_STUB := false

TARGET_BOARD_PLATFORM := smdkc110
TARGET_NO_BOOTLOADER := true	#uses u-boot instead
TARGET_NO_KERNEL := true
TARGET_NO_RECOVERY := true
TARGET_NO_RADIOIMAGE := true
# TARGET_CPU_SMP := true
TARGET_ARCH_VARIANT := armv7-a-neon
ARCH_ARM_HAVE_TLS_REGISTER := true

BOARD_HAVE_BLUETOOTH := false

TARGET_BOOTLOADER_BOARD_NAME := smdkc110

TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi

BOARD_EGL_CFG := device/samsung/smdkc110/egl.cfg

USE_OPENGL_RENDERER := true

ifeq ($(BOARD_SDMMC_BSP),true)
TARGET_USERIMAGES_USE_EXT4 := true
TARGET_USERIMAGES_SPARSE_EXT_DISABLED := true
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 268435456
BOARD_USERDATAIMAGE_PARTITION_SIZE := 268435456
endif

BOARD_FLASH_BLOCK_SIZE := 4096
BOARD_NAND_PAGE_SIZE := 4096
BOARD_NAND_SPARE_SIZE := 128

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
ifeq ($(S5P_BOARD_USES_HDMI),true)
BOARD_HDMI_STD := STD_1080P
S5P_BOARD_USES_HDMI_SUBTITLES := false
endif

DEFAULT_FB_NUM	:= 0
