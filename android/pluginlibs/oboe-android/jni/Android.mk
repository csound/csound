LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := oboe
# liboboe.a is here:
# ./pluginlibs/android-audio-high-performance/oboe/hello-oboe/.externalNativeBuild/cmake/release
# This Android.mk file for liboboe.a is here:
# ./pluginlibs/oboe-android/jni
# Therefore:
LOCAL_SRC_FILES := $(LOCAL_PATH)/../../android-audio-high-performance/oboe/hello-oboe/.externalNativeBuild/cmake/release/$(TARGET_ARCH_ABI)/oboe/liboboe.a
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_PATH)/../../android-audio-high-performance/oboe/lib-oboe/include
include $(PREBUILT_STATIC_LIBRARY)
