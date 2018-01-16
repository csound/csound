LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

CSOUND_SRC_ROOT := ../../../..

LOCAL_MODULE   := liboboe

LOCAL_C_INCLUDES := $(NDK_MODULE_PATH)/oboe/include $(NDK_MODULE_PATH)/oboe/src

LOCAL_SRC_FILES := $(NDK_MODULE_PATH)/oboe/src/aaudio/AAudioLoader.cpp \
        $(NDK_MODULE_PATH)/oboe/src/aaudio/AudioStreamAAudio.cpp \
        $(NDK_MODULE_PATH)/oboe/src/common/LatencyTuner.cpp \
        $(NDK_MODULE_PATH)/oboe/src/common/AudioStream.cpp \
        $(NDK_MODULE_PATH)/oboe/src/common/AudioStreamBuilder.cpp \
        $(NDK_MODULE_PATH)/oboe/src/common/Utilities.cpp \
        $(NDK_MODULE_PATH)/oboe/src/fifo/FifoBuffer.cpp \
        $(NDK_MODULE_PATH)/oboe/src/fifo/FifoController.cpp \
        $(NDK_MODULE_PATH)/oboe/src/fifo/FifoControllerBase.cpp \
        $(NDK_MODULE_PATH)/oboe/src/fifo/FifoControllerIndirect.cpp \
        $(NDK_MODULE_PATH)/oboe/src/opensles/AudioInputStreamOpenSLES.cpp \
        $(NDK_MODULE_PATH)/oboe/src/opensles/AudioOutputStreamOpenSLES.cpp \
        $(NDK_MODULE_PATH)/oboe/src/opensles/AudioStreamBuffered.cpp \
        $(NDK_MODULE_PATH)/oboe/src/opensles/AudioStreamOpenSLES.cpp \
        $(NDK_MODULE_PATH)/oboe/src/opensles/EngineOpenSLES.cpp \
        $(NDK_MODULE_PATH)/oboe/src/opensles/OpenSLESUtilities.cpp \
        $(NDK_MODULE_PATH)/oboe/src/opensles/OutputMixerOpenSLES.cpp
        
LOCAL_LDLIBS += -llog -lOpenSLES 

include $(BUILD_SHARED_LIBRARY)

