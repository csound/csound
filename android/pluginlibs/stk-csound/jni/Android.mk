LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

CSOUND_SRC_ROOT := ../../../..

LOCAL_MODULE   := libstk

LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../stk/include $(LOCAL_PATH)/../../stk/src $(LOCAL_PATH)/../../../../Engine $(LOCAL_PATH)/../../../../H $(LOCAL_PATH)/../../../../include $(LOCAL_PATH)/../../../.. $(LOCAL_PATH)/../../../ $(NDK_MODULE_PATH)/libsndfile-android/jni $(LOCAL_PATH)/../../../CsoundAndroid/jni
LOCAL_CFLAGS := -O3 -D__BUILDING_LIBCSOUND -DENABLE_NEW_PARSER -DLINUX -DHAVE_DIRENT_H -DHAVE_FCNTL_H -DHAVE_UNISTD_H -DHAVE_STDINT_H -DHAVE_SYS_TIME_H -DHAVE_SYS_TYPES_H -DHAVE_TERMIOS_H 
# Might need this: -DDEFAULT_RAWWAVE_PATH=${DEFAULT_STK_RAWWAVE_PATH}
LOCAL_CPPFLAGS :=$(LOCAL_CFLAGS)
LOCAL_CPPFLAGS += -std=c++11 -pthread -frtti -fexceptions
LOCAL_LDFLAGS += -Wl,--export-dynamic
###

# OMIT:
#            src/InetWvIn.cpp    src/InetWvOut.cpp
#            src/Mutex.cpp       src/RtAudio.cpp
#            src/RtMidi.cpp
#            src/RtWvIn.cpp      src/RtWvOut.cpp
#            src/Socket.cpp      src/TcpClient.cpp
#            src/TcpServer.cpp   src/Thread.cpp
#            src/UdpSocket.cpp)

LOCAL_SRC_FILES := $(CSOUND_SRC_ROOT)/Opcodes/stk/stkOpcodes.cpp \
    ../../stk/src/ADSR.cpp \
    ../../stk/src/Asymp.cpp \
    ../../stk/src/BandedWG.cpp \
    ../../stk/src/BeeThree.cpp \
    ../../stk/src/BiQuad.cpp \
    ../../stk/src/Blit.cpp \
    ../../stk/src/BlitSaw.cpp \
    ../../stk/src/BlitSquare.cpp \
    ../../stk/src/BlowBotl.cpp \
    ../../stk/src/BlowHole.cpp \
    ../../stk/src/Bowed.cpp \
    ../../stk/src/Brass.cpp \
    ../../stk/src/Chorus.cpp \
    ../../stk/src/Clarinet.cpp \
    ../../stk/src/DelayA.cpp \
    ../../stk/src/Delay.cpp \
    ../../stk/src/DelayL.cpp \
    ../../stk/src/Drummer.cpp \
    ../../stk/src/Echo.cpp \
    ../../stk/src/Envelope.cpp \
    ../../stk/src/FileLoop.cpp \
    ../../stk/src/FileRead.cpp \
    ../../stk/src/FileWrite.cpp \
    ../../stk/src/FileWvIn.cpp \
    ../../stk/src/FileWvOut.cpp \
    ../../stk/src/Fir.cpp \
    ../../stk/src/Flute.cpp \
    ../../stk/src/FM.cpp \
    ../../stk/src/FMVoices.cpp \
    ../../stk/src/FormSwep.cpp \
    ../../stk/src/FreeVerb.cpp \
    ../../stk/src/Granulate.cpp \
    ../../stk/src/Guitar.cpp \
    ../../stk/src/HevyMetl.cpp \
    ../../stk/src/Iir.cpp \
    ../../stk/src/JCRev.cpp \
    ../../stk/src/LentPitShift.cpp \
    ../../stk/src/Mandolin.cpp \
    ../../stk/src/Mesh2D.cpp \
    ../../stk/src/Messager.cpp \
    ../../stk/src/MidiFileIn.cpp \
    ../../stk/src/ModalBar.cpp \
    ../../stk/src/Modal.cpp \
    ../../stk/src/Modulate.cpp \
    ../../stk/src/Moog.cpp \
    ../../stk/src/Noise.cpp \
    ../../stk/src/NRev.cpp \
    ../../stk/src/OnePole.cpp \
    ../../stk/src/OneZero.cpp \
    ../../stk/src/PercFlut.cpp \
    ../../stk/src/Phonemes.cpp \
    ../../stk/src/PitShift.cpp \
    ../../stk/src/Plucked.cpp \
    ../../stk/src/PoleZero.cpp \
    ../../stk/src/PRCRev.cpp \
    ../../stk/src/Resonate.cpp \
    ../../stk/src/Rhodey.cpp \
    ../../stk/src/Sampler.cpp \
    ../../stk/src/Saxofony.cpp \
    ../../stk/src/Shakers.cpp \
    ../../stk/src/Simple.cpp \
    ../../stk/src/SineWave.cpp \
    ../../stk/src/SingWave.cpp \
    ../../stk/src/Sitar.cpp \
    ../../stk/src/Skini.cpp \
    ../../stk/src/Sphere.cpp \
    ../../stk/src/StifKarp.cpp \
    ../../stk/src/Stk.cpp \
    ../../stk/src/TapDelay.cpp \
    ../../stk/src/TubeBell.cpp \
    ../../stk/src/Twang.cpp \
    ../../stk/src/TwoPole.cpp \
    ../../stk/src/TwoZero.cpp \
    ../../stk/src/Voicer.cpp \
    ../../stk/src/VoicForm.cpp \
    ../../stk/src/Whistle.cpp \
    ../../stk/src/Wurley.cpp

include $(BUILD_SHARED_LIBRARY)

