#ifdef SWIG
%module csnd
#endif

#include "csound.hpp"

class PUBLIC AndroidCsound : public Csound {
public:
virtual int PreCompile();
int SetGlobalEnv(const char* name, const char* variable);
};
