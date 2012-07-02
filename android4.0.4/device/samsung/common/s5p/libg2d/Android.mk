# Copyright (C) 2008 The Android Open Source Project
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

ifeq ($(filter-out smdkc110 smdkv210,$(TARGET_BOARD_PLATFORM)),)

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_C_INCLUDES += $(PRODUCT_COMMON_DIR)/include
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= SecG2d.cpp

ifeq ($(filter-out smdkc110 smdkv210,$(TARGET_BOARD_PLATFORM)),)
LOCAL_SRC_FILES += SecG2dC110.cpp
endif

ifeq ($(filter-out s5pc210,$(TARGET_BOARD_PLATFORM)),)
LOCAL_SRC_FILES += SecG2dC210.cpp
endif

LOCAL_SHARED_LIBRARIES:= liblog libutils libbinder

LOCAL_MODULE:= libg2d

LOCAL_PRELINK_MODULE := false

include $(BUILD_SHARED_LIBRARY)

endif
