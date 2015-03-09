#ifdef SWIG
%module csnd
#endif

#include "csound.hpp"

class PUBLIC AndroidCsound : public Csound {
  int asyncProcess;
public:
    AndroidCsound(bool async=true) : Csound::Csound(){
      asyncProcess = async;
    }
    void setOpenSlCallbacks();
    int SetGlobalEnv(const char* name, const char* variable);
    unsigned long getStreamTime();
    void Pause(bool pause);
};
