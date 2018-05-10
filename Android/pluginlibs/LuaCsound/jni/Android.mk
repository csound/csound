# When porting to Android Studio NDK support see this: 
# https://www.twilio.com/blog/2016/03/building-native-android-libraries-with-the-latest-experimental-android-plugin.html
# This may be relevant:
# http://stackoverflow.com/questions/16673385/app-stl-gnustl-static-is-broken-in-android-ndk-8c-through-8e

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

CSOUND_SRC_ROOT := ../../../..

LOCAL_MODULE   := LuaCsound 

# LOCAL_CXX := "g++ -m32"
LOCAL_C_INCLUDES := $(NDK_MODULE_PATH)/luajit-2.0/src $(LOCAL_PATH)/../../../../Engine $(LOCAL_PATH)/../../../../H $(LOCAL_PATH)/../../../../include $(LOCAL_PATH)/../../../CsoundAndroid/jni $(LOCAL_PATH)/../../../.. $(LOCAL_PATH)/../../../ $(NDK_MODULE_PATH)/libsndfile-android/jni 
LOCAL_CFLAGS := -O3 -D__BUILDING_LIBCSOUND -DENABLE_NEW_PARSER -DLINUX -DHAVE_DIRENT_H -DHAVE_FCNTL_H -DHAVE_UNISTD_H -DHAVE_STDINT_H -DHAVE_SYS_TIME_H -DHAVE_SYS_TYPES_H -DHAVE_TERMIOS_H 
LOCAL_CPPFLAGS :=$(LOCAL_CFLAGS)
LOCAL_CPPFLAGS += -pthread -frtti -fexceptions -std=c++11 

LOCAL_LDLIBS += -ldl 

LOCAL_SRC_FILES := $(CSOUND_SRC_ROOT)/Opcodes/LuaCsound.cpp

LOCAL_STATIC_LIBRARIES += luajit

include $(BUILD_SHARED_LIBRARY)

$(call import-module,luajit-2.0/jni)
