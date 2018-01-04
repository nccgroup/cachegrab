LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libmongoose
LOCAL_SRC_FILES := mongoose.c

include $(BUILD_STATIC_LIBRARY)
