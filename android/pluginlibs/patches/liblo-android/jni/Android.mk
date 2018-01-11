LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS := -DHAVE_CONFIG_H
LOCAL_C_INCLUDES := -I.. -I. -I../..

LOCAL_MODULE   := lo

LOCAL_SRC_FILES := address.c \
blob.c \
bundle.c \
message.c \
method.c \
pattern_match.c \
send.c \
server.c \
server_thread.c \
timetag.c \
version.c

include $(BUILD_STATIC_LIBRARY)

