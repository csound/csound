#ifndef OPCODE_BASE_H
#define OPCODE_BASE_H
#include "cs.h"
#include <cstdarg>

/**
* Template base class, or pseudo-virtual base class,
* for writing Csound opcodes in C++. 
* Derive opcode implementation classes like this: 
* 
* DerivedClass : public OpcodeBase<DerivedClass> 
* {
* public:
*     // All output fields must be declared first:
*     MYFLT *aret1;
*     // All input fields must be declared next:
*     MYFLT *iarg1;
*     MYFLT *karg2;
*     MYFLT *aarg3;
*     // All internal state variables must be declared after that:
*     size_t state1;
*     double state2;
*     MYFLT state3;
*     // Declare and implement only whichever of these are required:
*     void initialize();
*     void onKontrolSample();
*     void onAudioSample;
*     void deinitialize();
* };
*/
template<typename T> 
class OpcodeBase
{
public:
    int initialize()
    {
        return NOTOK;
    }
    static int initialize_(void *opcode)
    {
        return reinterpret_cast<T *>(opcode)->initialize();
    }
    int onKontrolSample()
    {
        return NOTOK;
    }
    static int onKontrolSample_(void *opcode)
    {
        return reinterpret_cast<T *>(opcode)->onKontrolSample();
    }
    int onAudioSample()
    {
        return NOTOK;
    }
    static int onAudioSample_(void *opcode)
    {
        return reinterpret_cast<T *>(opcode)->onKontrolSample();
    }
    int deinitialize()
    {
        return NOTOK;
    }
    static int deinitialize_(void *opcode)
    {
        return reinterpret_cast<T *>(opcode)->onKontrolSample();
    }
    ENVIRON *cs()
    {
        return h.insdshead->csound;
    }
    void log(const char *format,...)
    {
      va_list args;
      va_start(args, format);
      if(cs()) {
            cs()->MessageV(cs(), format, args);
      }
      else {
            vfprintf(stdout, format, args);
      }
      va_end(args);
    }
    void warn(const char *format,...)
    {
      va_list args;
      va_start(args, format);
      if(cs()) {
          if(cs()->GetMessageLevel(cs() & WARNMSG ||
             cs()->GetDebug(cs()) {
              cs()->MessageV(cs(), format, args);
          }
      }
      else {
            vfprintf(stdout, format, args);
      }
      va_end(args);
    }
    OPDS h;
};

#endif
