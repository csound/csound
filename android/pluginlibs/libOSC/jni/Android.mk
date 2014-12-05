LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LIBLO_SRC_DIR := $(NDK_MODULE_PATH)/liblo-android/jni

LOCAL_MODULE   := OSC

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../H $(LOCAL_PATH)/../../../../include $(LOCAL_PATH)/../../../.. $(LOCAL_PATH)/../../../ $(LIBLO_SRC_DIR) $(LOCAL_PATH)/../../../../Opcodes $(LOCAL_PATH)/../../../CsoundAndroid/jni 
LOCAL_CFLAGS := -O3 -D__BUILDING_LIBCSOUND -DENABLE_NEW_PARSER -DLINUX -DHAVE_DIRENT_H -DHAVE_FCNTL_H -DHAVE_UNISTD_H -DHAVE_STDINT_H -DHAVE_SYS_TIME_H -DHAVE_SYS_TYPES_H -DHAVE_TERMIOS_H 
LOCAL_CPPFLAGS :=$(LOCAL_CFLAGS)
LOCAL_CPPFLAGS += -pthread -frtti -fexceptions
###

LOCAL_SRC_FILES := ../../../../Opcodes/OSC.c 

LOCAL_LDLIBS += -ldl

LOCAL_STATIC_LIBRARIES := liblo
 
include $(BUILD_SHARED_LIBRARY)

$(call import-module,liblo-android/jni)
