#ifdef SWIG
%module csnd
#endif
#include "csound.hpp"
#include "csound_misc.h"
extern "C" long csoundGetKcounter(CSOUND *csound);
class PUBLIC AndroidCsound : public Csound {
  int asyncProcess;
  void initControls() {
    // set up pause controls
     if(csoundQueryGlobalVariable(csound,"::paused::") == NULL) {
      if (csoundCreateGlobalVariable(csound,"::paused::", sizeof(int)) == 0) {
        int *p = ((int *)csoundQueryGlobalVariable(csound,"::paused::"));
        *p = 0;
      }
    }
  }
 public:
  AndroidCsound(bool async=true) : Csound::Csound(){
    asyncProcess = async;
  }
  void setOpenSlCallbacks();
  void setAAudioCallbacks(); 
  int SetGlobalEnv(const char* name, const char* variable);
  unsigned long getStreamTime();
  void Pause(bool pause);
  long GetKcount(){ return csoundGetKcounter(csound); }
};
