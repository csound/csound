#ifndef OPCODE_BASE_H
#define OPCODE_BASE_H
#ifndef MSVC
#include "csdl.h"
#include <cstdarg>
#else
#include <cmath>
#include "csdl.h"
#endif

/**
 * Template base class, or pseudo-virtual base class,
 * for writing Csound opcodes in C++.
 * Derive opcode implementation classes like this:
 *
 * DerivedClass : public OpcodeBase<DerivedClass>
 * {
 * public:
 *     // All output fields must be declared first as MYFLT *:
 *     MYFLT *aret1;
 *     // All input fields must be declared next as MYFLT *:
 *     MYFLT *iarg1;
 *     MYFLT *karg2;
 *     MYFLT *aarg3;
 *     // All internal state variables must be declared after that:
 *     size_t state1;
 *     double state2;
 *     MYFLT state3;
 *     // Declare and implement only whichever of these are required:
 *     void init();
 *     void kontrol();
 *     void audio;
 *     void noteoff();
 *     void deinit();
 * };
 */
template<typename T>
class OpcodeBase
{
public:
  int init(CSOUND *csound)
  {
    return NOTOK;
  }
  static int init_(CSOUND *csound, void *opcode_)
  {
    T *opcode = reinterpret_cast<T *>(opcode_);
    if (!csound->reinitflag && !csound->tieflag)
      csound->RegisterDeinitCallback(csound,
                                     &opcode->h, OpcodeBase<T>::noteoff_);
    return opcode->init(csound);
  }
  int kontrol(CSOUND *csound)
  {
    return NOTOK;
  }
  static int kontrol_(CSOUND *csound, void *opcode)
  {
    return reinterpret_cast<T *>(opcode)->kontrol(csound);
  }
  int audio(CSOUND *csound)
  {
    return NOTOK;
  }
  static int audio_(CSOUND *csound, void *opcode)
  {
    return reinterpret_cast<T *>(opcode)->audio(csound);
  }
  int noteoff(CSOUND *csound)
  {
    return OK;
  }
  static int noteoff_(CSOUND *csound, void *opcode)
  {
    return reinterpret_cast< OpcodeBase<T> *>(opcode)->noteoff(csound);
  }
  void log(CSOUND *csound, const char *format,...)
  {
    va_list args;
    va_start(args, format);
    if(csound) {
      csound->MessageV(csound, 0, format, args);
    }
    else {
      vfprintf(stdout, format, args);
    }
    va_end(args);
  }
  void warn(CSOUND *csound, const char *format,...)
  {
    va_list args;
    va_start(args, format);
    if(csound) {
      if(csound->GetMessageLevel(csound) & WARNMSG ||
         csound->GetDebug(csound)) {
        csound->MessageV(csound, CSOUNDMSG_WARNING, format, args);
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

