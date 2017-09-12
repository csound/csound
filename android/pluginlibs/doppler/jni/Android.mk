LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

CSOUND_SRC_ROOT := ../../../..

LOCAL_MODULE   := doppler

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../Engine $(LOCAL_PATH)/../../../../H $(LOCAL_PATH)/../../../../include $(LOCAL_PATH)/../../../.. $(LOCAL_PATH)/../../../ $(NDK_MODULE_PATH)/libsndfile-android/jni $(LOCAL_PATH)/../../../CsoundAndroid/jni
LOCAL_CFLAGS := -O3 -D__BUILDING_LIBCSOUND -DENABLE_NEW_PARSER -DLINUX -DHAVE_DIRENT_H -DHAVE_FCNTL_H -DHAVE_UNISTD_H -DHAVE_STDINT_H -DHAVE_SYS_TIME_H -DHAVE_SYS_TYPES_H -DHAVE_TERMIOS_H
LOCAL_CPPFLAGS :=$(LOCAL_CFLAGS)
LOCAL_CPPFLAGS += -std=c++11 -pthread -frtti -fexceptions
LOCAL_LDFLAGS += -Wl,--export-dynamic

LOCAL_SRC_FILES := $(CSOUND_SRC_ROOT)/Opcodes/doppler.cpp

include $(BUILD_SHARED_LIBRARY)

