/*
   AndroidCsound.cpp
   Android Csound class

   Copyright (C) 2011-2025 Steven Yi, Victor Lazzarini.

   This file is part of Csound.

   The Csound Library is free software; you can redistribute it
   and/or modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   Csound is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with Csound; if not, write to the Free Software
   Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA

*/

#include "AndroidCsound.hpp"
#include "text.h"
#include <android/log.h>
#include <jni.h>

static JavaVM* g_vm;  

extern "C" {
extern int androidplayopen_(CSOUND *csound, const csRtAudioParams *parm);
extern int androidrecopen_(CSOUND *csound, const csRtAudioParams *parm);
extern void androidrtplay_(CSOUND *csound, const cs_float *buffer, int nbytes);
extern int androidrtrecord_(CSOUND *csound, cs_float *buffer, int nbytes);
extern void androidrtclose_(CSOUND *csound);

static void androidMessageCallback(CSOUND*, int attr, const char *format, va_list valist) {
    char message[1024];
    vsnprintf(message, 1024, format, valist);
    __android_log_print(ANDROID_LOG_INFO,"AndroidCsound","%s", message); 
}
}

#if !defined(__BUILDING_LIBCSOUND) 
#define __BUILDING_LIBCSOUND
#endif

#include <pthread.h>
void AndroidCsound::setOpenSlCallbacks() {
   initControls();
   __android_log_print(ANDROID_LOG_INFO,"AndroidCsound","setOpenSlCallbacks"); 
   if(csoundQueryGlobalVariable(csound,"::async::") == NULL) 
     if (csoundCreateGlobalVariable(csound,"::async::", sizeof(int)) == 0) {
      int *p = ((int *)csoundQueryGlobalVariable(csound,"::async::"));
       *p = asyncProcess;
    __android_log_print(ANDROID_LOG_INFO,"AndroidCsound","==set callbacks");
    csoundSetPlayopenCallback(csound, androidplayopen_);
    csoundSetRecopenCallback(csound, androidrecopen_);
    csoundSetRtplayCallback(csound, androidrtplay_);
    csoundSetRtrecordCallback(csound, androidrtrecord_);
    csoundSetRtcloseCallback(csound, androidrtclose_);
    csoundSetMessageCallback(csound, androidMessageCallback);
      __android_log_print(ANDROID_LOG_INFO,"AndroidCsound","==callbacks set"); 
    }
};


extern "C" void aaudio_setup(CSOUND *csound);
void AndroidCsound::setAAudioCallbacks() {
  initControls();
  aaudio_setup(csound);
}


int AndroidCsound::SetGlobalEnv(const char* name, const char* variable) {
    return csoundSetGlobalEnv(name, variable);
}

void AndroidCsound::Pause(bool pause){
   int *p = ((int *)csoundQueryGlobalVariable(csound,"::paused::"));
   if(p) *p = pause ?  1  : 0;
   else csoundMessage(csound, Str("pause control not set up\n"));
}

unsigned long AndroidCsound::getStreamTime(){
  return *((__uint64_t*) csoundQueryGlobalVariable(csound,"::streamtime::"));
}


/* The CSOUND * a SWIG type wrapper, SWIGTYPE_p_CSOUND_, holds */
static CSOUND *wrapped_csound(JNIEnv* env, jobject csound) {
  if (csound == NULL)
    return NULL;
  jclass wrapper = env->GetObjectClass(csound);
  jfieldID ptr = env->GetFieldID(wrapper, "swigCPtr", "J");
  env->DeleteLocalRef(wrapper);
  if (ptr == NULL) {      /* NoSuchFieldError: R8 or ProGuard renamed it */
    env->ExceptionClear();
    __android_log_print(ANDROID_LOG_ERROR, "AndroidCsound",
                        "MIDI is not set up: SWIGTYPE_p_CSOUND_.swigCPtr is missing, "
                        "keep it in the R8 or ProGuard rules");
    return NULL;
  }
  return (CSOUND *) (intptr_t) env->GetLongField(csound, ptr);
}

extern "C" void android_midi_init(CSOUND *csound, JNIEnv* env, jobject obj_in, jobject obj_out);
extern "C" JNIEXPORT void JNICALL
Java_com_csounds_CsoundObj_setMidiDevices(JNIEnv* env, jobject, jobject csound, jobject in, jobject out) {
  CSOUND *cs = wrapped_csound(env, csound);
  if (cs != NULL)
    android_midi_init(cs, env, in, out);
}

extern "C" void android_midi_deinit(CSOUND *csound);

extern "C" JNIEXPORT void JNICALL
Java_com_csounds_CsoundObj_deinitMidiDevices(JNIEnv* env, jobject, jobject csound) {
  CSOUND *cs = wrapped_csound(env, csound);
  if (cs != NULL)
    android_midi_deinit(cs);
}
