#include "AndroidCsound.hpp"
#include <android/log.h>

static JavaVM* g_vm;  

extern "C" {
extern int androidplayopen_(CSOUND *csound, const csRtAudioParams *parm);
extern int androidrecopen_(CSOUND *csound, const csRtAudioParams *parm);
extern void androidrtplay_(CSOUND *csound, const MYFLT *buffer, int nbytes);
extern int androidrtrecord_(CSOUND *csound, MYFLT *buffer, int nbytes);
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

#include <csoundCore.h>
#include <pthread.h>
void AndroidCsound::setOpenSlCallbacks() {

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

   if(csoundQueryGlobalVariable(csound,"::paused::") == NULL) {
     if (csoundCreateGlobalVariable(csound,"::paused::", sizeof(int)) == 0) {
       int *p = ((int *)csoundQueryGlobalVariable(csound,"::paused::"));
       *p = 0;
    }
   }
    
  
};

extern "C" void android_midi_init(CSOUND *csound, JNIEnv* env, jobject obj_in, jobject obj_out);

void AndroidCsound::setMidiCallbacks(jobject midi_in, jobject midi_out) {
  JNIEnv *env;
  g_vm->GetEnv((void **) &env, JNI_VERSION_1_6);
  android_midi_init(csound, env, midi_in, midi_out);
}


int AndroidCsound::SetGlobalEnv(const char* name, const char* variable) {
    return csoundSetGlobalEnv(name, variable);
}

void AndroidCsound::Pause(bool pause){
   int *p = ((int *)csoundQueryGlobalVariable(csound,"::paused::"));
   *p = pause ?  1  : 0;
}

unsigned long AndroidCsound::getStreamTime(){
  return *((__uint64_t*) csoundQueryGlobalVariable(csound,"::streamtime::"));
}

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
  g_vm = vm;
  return 0;
}
