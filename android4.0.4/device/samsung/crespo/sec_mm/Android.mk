ifeq ($(filter-out s5pc110,$(TARGET_BOARD_PLATFORM)),)
WITH_SEC_OMX := true

ifeq ($(WITH_SEC_OMX), true)
  include $(all-subdir-makefiles)
endif
endif
