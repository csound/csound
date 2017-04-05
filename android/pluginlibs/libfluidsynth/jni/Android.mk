LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LIBSNDFILE_SRC_DIR := $(NDK_MODULE_PATH)/libsndfile-android/jni/
LIBFLUIDSYNTH_SRC_DIR := $(NDK_MODULE_PATH)/fluidsynth-android/jni/

LOCAL_MODULE   := fluidOpcodes

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../H $(LOCAL_PATH)/../../../../include $(LOCAL_PATH)/../../../.. $(LOCAL_PATH)/../../../ $(LIBSNDFILE_SRC_DIR) $(LIBFLUIDSYNTH_SRC_DIR)/include $(LOCAL_PATH)/../../../../Opcodes/fluidOpcodes/ $(LOCAL_PATH)/../../../CsoundAndroid/jni 
LOCAL_CFLAGS := -O3 -D__BUILDING_LIBCSOUND -DENABLE_NEW_PARSER -DLINUX -DHAVE_DIRENT_H -DHAVE_FCNTL_H -DHAVE_UNISTD_H -DHAVE_STDINT_H -DHAVE_SYS_TIME_H -DHAVE_SYS_TYPES_H -DHAVE_TERMIOS_H 
LOCAL_CPPFLAGS :=$(LOCAL_CFLAGS)
LOCAL_CPPFLAGS += -std=c++11 -pthread -frtti -fexceptions
###

LOCAL_SRC_FILES := ../../../../Opcodes/fluidOpcodes/fluidOpcodes.cpp 

LOCAL_LDLIBS += -ldl

LOCAL_STATIC_LIBRARIES := fluidsynth-android 

include $(BUILD_SHARED_LIBRARY)

$(call import-module,fluidsynth-android/jni)
