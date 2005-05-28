#ifndef OPCODE_BASE_H
#define OPCODE_BASE_H
#include "csoundCore.h"
#include <cstdarg>

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
  int init(ENVIRON *csound)
  {
    return NOTOK;
  }
  static int init_(void *csound_, void *opcode_)
  {
    ENVIRON *csound = reinterpret_cast<ENVIRON *>(csound_);
    T *opcode = reinterpret_cast<T *>(opcode_);
    csound->RegisterDeinitCallback(csound, &opcode->h, OpcodeBase<T>::noteoff_);
    return opcode->init(csound);
  }
  int kontrol(ENVIRON *csound)
  {
    return NOTOK;
  }
  static int kontrol_(void *csound, void *opcode)
  {
    return reinterpret_cast<T *>(opcode)->kontrol(reinterpret_cast<ENVIRON *>(csound));
  }
  int audio(ENVIRON *csound)
  {
    return NOTOK;
  }
  static int audio_(void *csound, void *opcode)
  {
    return reinterpret_cast<T *>(opcode)->audio(reinterpret_cast<ENVIRON *>(csound));
  }
  int noteoff(ENVIRON *csound)
  {
    return OK;
  }
  static int noteoff_(void *csound, void *opcode)
  {
    return reinterpret_cast< OpcodeBase<T> *>(opcode)->noteoff(reinterpret_cast<ENVIRON *>(csound));
  }
  void log(ENVIRON *csound, const char *format,...)
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
  void warn(ENVIRON *csound, const char *format,...)
  {
    va_list args;
    va_start(args, format);
    if(csound) {
      if(csound->GetMessageLevel(csound) & WARNMSG ||
	 csound->GetDebug(csound)) {
	csound->MessageV(csound, 0, format, args);
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
