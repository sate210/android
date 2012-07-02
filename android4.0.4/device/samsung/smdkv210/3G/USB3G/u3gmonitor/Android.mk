ifeq ($(USB_3G),true)

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := u3gmonitor

LOCAL_MODULE_TAGS := eng 

LOCAL_SRC_FILES := main.c UEventFramework.c 

LOCAL_SHARED_LIBRARIES := libcutils

include $(BUILD_EXECUTABLE)

endif

