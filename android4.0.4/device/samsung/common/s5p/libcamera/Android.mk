ifeq ($(filter-out smdkc110 smdkv210,$(TARGET_BOARD_PLATFORM)),)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

# HAL module implemenation stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw

LOCAL_C_INCLUDES += $(PRODUCT_COMMON_DIR)/include
LOCAL_C_INCLUDES += $(PRODUCT_COMMON_DIR)/libs3cjpeg

LOCAL_SRC_FILES:= \
	SecCamera.cpp SecCameraHWInterface.cpp

LOCAL_SHARED_LIBRARIES:= libutils libcutils libbinder liblog libcamera_client libhardware
LOCAL_SHARED_LIBRARIES+= libs3cjpeg


ifeq ($(BOARD_USES_HDMI),true)
LOCAL_CFLAGS += -DBOARD_USES_HDMI
endif

LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

endif
