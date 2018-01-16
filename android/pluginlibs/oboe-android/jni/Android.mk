LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

CSOUND_SRC_ROOT := ../../../..

LOCAL_MODULE   := liboboe

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../oboe/include $(LOCAL_PATH)/../../oboe/src

LOCAL_SRC_FILES := ../../oboe/src/aaudio/AAudioLoader.cpp \
        ../../oboe/src/aaudio/AudioStreamAAudio.cpp \
        ../../oboe/src/common/LatencyTuner.cpp \
        ../../oboe/src/common/AudioStream.cpp \
        ../../oboe/src/common/AudioStreamBuilder.cpp \
        ../../oboe/src/common/Utilities.cpp \
        ../../oboe/src/fifo/FifoBuffer.cpp \
        ../../oboe/src/fifo/FifoController.cpp \
        ../../oboe/src/fifo/FifoControllerBase.cpp \
        ../../oboe/src/fifo/FifoControllerIndirect.cpp \
        ../../oboe/src/opensles/AudioInputStreamOpenSLES.cpp \
        ../../oboe/src/opensles/AudioOutputStreamOpenSLES.cpp \
        ../../oboe/src/opensles/AudioStreamBuffered.cpp \
        ../../oboe/src/opensles/AudioStreamOpenSLES.cpp \
        ../../oboe/src/opensles/EngineOpenSLES.cpp \
        ../../oboe/src/opensles/OpenSLESUtilities.cpp \
        ../../oboe/src/opensles/OutputMixerOpenSLES.cpp
        
LOCAL_LDLIBS += -llog -lOpenSLES 

include $(BUILD_SHARED_LIBRARY)

