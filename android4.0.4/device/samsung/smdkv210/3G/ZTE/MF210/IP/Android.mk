#ifeq ($(MF210),true)

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_SRC_FILES := ip-down-ppp0.c
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := ip-down-ppp0
LOCAL_SHARED_LIBRARIES := libcutils libc
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/ppp
include $(BUILD_EXECUTABLE)


include $(CLEAR_VARS)
LOCAL_SRC_FILES := ip-up-ppp0.c
LOCAL_MODULE_TAGS := eng
LOCAL_MODULE := ip-up-ppp0
LOCAL_SHARED_LIBRARIES := libcutils libc
LOCAL_MODULE_PATH := $(TARGET_OUT_ETC)/ppp
include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))

#endif