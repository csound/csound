#ifdef SWIG
%module csnd
#endif

#include "csound.hpp"

class PUBLIC AndroidCsound : public Csound {
public:
    void setOpenSlCallbacks();
    int SetGlobalEnv(const char* name, const char* variable);
};
