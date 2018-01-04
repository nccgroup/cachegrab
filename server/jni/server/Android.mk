LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := cachegrab_server
LOCAL_SRC_FILES := server.c server_scope.c server_probe.c server_capture.c
LOCAL_SRC_FILES += scope.c capture.c capture_data.c
LOCAL_SRC_FILES += thread_utils.c thread_scope.c thread_target.c thread_stall.c
LOCAL_CFLAGS := -Wall
LOCAL_CFLAGS += -I$(LOCAL_PATH)/../libpng -I$(LOCAL_PATH)/../mongoose
LOCAL_LDLIBS := -lz
LOCAL_STATIC_LIBRARIES := libpng libmongoose

include $(BUILD_EXECUTABLE)
