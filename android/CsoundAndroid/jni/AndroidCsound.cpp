#include "AndroidCsound.hpp"
#include <android/log.h>

extern "C" {
extern void androidrtclose_(CSOUND *csound);
extern void androidrtplay_(CSOUND *csound, const MYFLT *buffer, int nbytes);
extern int androidrtrecord_(CSOUND *csound, MYFLT *buffer, int nbytes);
extern int androidrecopen_(CSOUND *csound, const csRtAudioParams *parm);
extern int androidplayopen_(CSOUND *csound, const csRtAudioParams *parm);

static void androidMessageCallbac(CSOUND*, int attr, const char *format, va_list valist) {

char message[1024];

vsnprintf(message, 1024, format, valist);
__android_log_print(ANDROID_LOG_INFO,"AndroidCsound",message); 

}


}


int AndroidCsound::PreCompile()
{

__android_log_print(ANDROID_LOG_INFO,"AndroidCsound","PreCompile()"); 

     int retVal = Csound::PreCompile(); 
__android_log_print(ANDROID_LOG_INFO,"AndroidCsound","set callbacks"); 
      csoundSetPlayopenCallback(csound, androidplayopen_);
      csoundSetRecopenCallback(csound, androidrecopen_);
      csoundSetRtplayCallback(csound, androidrtplay_);
      csoundSetRtrecordCallback(csound, androidrtrecord_);
      csoundSetRtcloseCallback(csound, androidrtclose_);
      csoundSetMessageCallback(csound, androidMessageCallbac);
__android_log_print(ANDROID_LOG_INFO,"AndroidCsound","callbacks set"); 

    return retVal; 
};

void AndroidCsound::SetGlobalEnv(const char* name, const char* variable) {
    csoundSetGlobalEnv(name, variable);
}
