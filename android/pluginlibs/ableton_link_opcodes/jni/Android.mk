LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

CSOUND_SRC_ROOT := ../../../..
ABLETON_LINK_HOME :=/home/mkg/link

LOCAL_MODULE   := ableton_link_opcodes 

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../Engine $(LOCAL_PATH)/../../../../H $(LOCAL_PATH)/../../../../include $(LOCAL_PATH)/../../../.. $(LOCAL_PATH)/../../../ $(NDK_MODULE_PATH)/libsndfile-android/jni $(LOCAL_PATH)/../../../CsoundAndroid/jni $(ABLETON_LINK_HOME)/include $(ABLETON_LINK_HOME)/modules/asio-standalone/asio/include
LOCAL_CFLAGS := -O3 -D__BUILDING_LIBCSOUND -DENABLE_NEW_PARSER -DLINUX -DHAVE_DIRENT_H -DHAVE_FCNTL_H -DHAVE_UNISTD_H -DHAVE_STDINT_H -DHAVE_SYS_TIME_H -DHAVE_SYS_TYPES_H -DHAVE_TERMIOS_H -DLINK_PLATFORM_LINUX=1 -DANDROID=1
LOCAL_CPPFLAGS :=$(LOCAL_CFLAGS)
LOCAL_CPPFLAGS += -std=c++11 -pthread -frtti -fcxx-exceptions -Wno-multichar -fexceptions #-Werror
LOCAL_LDLIBS := -latomic
LOCAL_LDFLAGS += -Wl,--export-dynamic

LOCAL_SRC_FILES := $(CSOUND_SRC_ROOT)/Opcodes/ableton_link_opcodes.cpp

include $(BUILD_SHARED_LIBRARY)

