LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE   := fluidsynth-static
LOCAL_C_INCLUDES := $(LOCAL_PATH)/include

LOCAL_CFLAGS := -DHAVE_PTHREAD_H -DHAVE_STDLIB_H -DHAVE_STDIO_H -DHAVE_MATH_H -DHAVE_STRING_H -DHAVE_STDARG_H -DHAVE_SYS_SOCKET_H -DHAVE_NETINET_IN_H -DHAVE_ARPA_INET_H -DHAVE_NETINET_TCP_H -DHAVE_UNISTD_H -DHAVE_ERRNO_H -DHAVE_FCNTL_H -DVERSION=1.0.9
LOCAL_CPPFLAGS :=$(LOCAL_CFLAGS)

LOCAL_SRC_FILES := fluid_adriver.c		fluid_dll.c		fluid_list.c		fluid_seq.c \
fluid_mdriver.c		fluid_seqbind.c 	fluid_aufile.c		fluid_cmd.c\
fluid_dsp_float.c	fluid_midi.c		fluid_settings.c \
fluid_chan.c		fluid_midi_router.c	fluid_sndmgr.c \
fluid_chorus.c		fluid_event.c		fluid_synth.c \
fluid_gen.c		    fluid_mod.c		    fluid_sys.c \
fluid_conv.c		fluid_hash.c		fluid_tuning.c \
fluid_voice.c  		fluid_io.c \
fluid_dart.c		fluid_ramsfont.c	\
fluid_defsfont.c	fluid_rev.c
 
LOCAL_LDLIBS += -llog -ldl

include $(BUILD_STATIC_LIBRARY)

#include $(BUILD_SHARED_LIBRARY)

