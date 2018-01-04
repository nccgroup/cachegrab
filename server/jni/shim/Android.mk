LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS += -Wall -I$(LOCAL_PATH)/../server
LOCAL_MODULE := cachegrab_shim
LOCAL_SRC_FILES := shim.c utils.c
LOCAL_LDLIBS := -L. -llog -fPIE

include $(BUILD_SHARED_LIBRARY)
