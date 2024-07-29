#include "AndroidCsound.hpp"
#include <android/log.h>

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
    if (this->CreateGlobalVariable("::async::", sizeof(int)) == 0) {
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
    if (this->CreateGlobalVariable("::paused::", sizeof(int)) == 0) {
       int *p = ((int *)csoundQueryGlobalVariable(csound,"::paused::"));
       *p = 0;
    }
   }
    
  
};

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
