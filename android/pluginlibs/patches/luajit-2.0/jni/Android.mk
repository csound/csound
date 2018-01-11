LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := luajit
LOCAL_SRC_FILES := local/libs/$(TARGET_ARCH_ABI)/libluajit-5.1.a
include $(PREBUILT_STATIC_LIBRARY)
