LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

OGG_ROOT := $(LOCAL_PATH)/../deps/libogg/include
VORBIS_ROOT := $(LOCAL_PATH)/../deps/libvorbis/include

LOCAL_CFLAGS := -DHAVE_EXTERNAL_LIBS
LOCAL_C_INCLUDES := $(OGG_ROOT) $(VORBIS_ROOT)

LOCAL_MODULE   := sndfile

OGG_SRC_FILES := ../deps/libogg/src/bitwise.c ../deps/libogg/src/framing.c
V_SRC := ../deps/libvorbis/src
VORBIS_SRC_FILES := $(V_SRC)/analysis.c	$(V_SRC)/floor0.c	$(V_SRC)/lsp.c		$(V_SRC)/res0.c	\
       $(V_SRC)/vorbisfile.c $(V_SRC)/bitrate.c	$(V_SRC)/floor1.c $(V_SRC)/mapping0.c	$(V_SRC)/sharedbook.c	\
       $(V_SRC)/window.c $(V_SRC)/block.c	$(V_SRC)/info.c	  $(V_SRC)/mdct.c	$(V_SRC)/smallft.c \
       $(V_SRC)/codebook.c	$(V_SRC)/lookup.c $(V_SRC)/psy.c  $(V_SRC)/synthesis.c \
       $(V_SRC)/envelope.c	$(V_SRC)/lpc.c	$(V_SRC)/registry.c $(V_SRC)/vorbisenc.c

LOCAL_SRC_FILES := mat5.c windows.c G72x/g723_24.c G72x/g72x.c \
       G72x/g723_40.c G72x/g721.c G72x/g723_16.c \
       float32.c chanmap.c test_endswap.c rf64.c sndfile.c htk.c dither.c \
       test_log_printf.c txw.c ms_adpcm.c ima_adpcm.c flac.c aiff.c \
       wav.c macbinary3.c mat4.c pcm.c caf.c \
       audio_detect.c id3.c alaw.c macos.c file_io.c broadcast.c double64.c \
       raw.c test_broadcast_var.c \
       g72x.c command.c chunk.c avr.c sd2.c voc.c test_audio_detect.c \
       mpc2k.c gsm610.c dwd.c \
       interleave.c common.c test_strncpy_crlf.c sds.c pvf.c paf.c au.c \
       test_float.c \
       vox_adpcm.c ulaw.c strings.c svx.c test_conversions.c rx2.c nist.c \
       GSM610/code.c GSM610/gsm_destroy.c \
       GSM610/gsm_decode.c GSM610/short_term.c GSM610/gsm_create.c \
       GSM610/decode.c GSM610/gsm_option.c \
       GSM610/long_term.c GSM610/table.c GSM610/rpe.c GSM610/preprocess.c \
       GSM610/gsm_encode.c GSM610/lpc.c \
       GSM610/add.c dwvw.c wav_w64.c wve.c ogg.c ogg_vorbis.c w64.c test_file_io.c\
       ircam.c xi.c ima_oki_adpcm.c \
       $(OGG_SRC_FILES) $(VORBIS_SRC_FILES)

LOCAL_LDLIBS += -llog -ldl
include $(BUILD_SHARED_LIBRARY)

