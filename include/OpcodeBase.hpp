/*
    OpcodeBase.hpp:

    Copyright (C) 2005, 2009, 2017 by Istva Varga, Victor Lazzarini and
                                      Michael Gogins

    This file is part of Csound.

    The Csound Library is free software; you can redistribute it
    and/or modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    Csound is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with Csound; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#ifndef OPCODE_BASE_H
#define OPCODE_BASE_H

#include <interlocks.h>
#include <csdl.h>
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
 *     // If the opcode shares data protect it by creating one or more void
 *     // *mutex member pointers:
 *     void *mutex1;
 *     void *mutex2;
 *     // and create them usind csound->Create_Mutex() in csoundModuleCreate
 *     // and destroy them  using csound->DeleteMutex(mutex)
 *     // csoundModuleDestroy, and lock them using LockGuard guard(mutex).
 *     int init();
 *     int kontrol();
 *     int audio;
 *     int noteoff();
 *     void deinit();
 * };
 */

namespace csound
{

/**
 * Use this to guard against data races in opcode functions. The mutex should
 * be created by csound->Create_Mutex() in csoundModuleCreate(), and should
 * be destroyed by csound->DeleteMutex() in csoundModuleDestroy().
 *
 * If data is shared between opcode instances, the mutex should be global to
 * the opcode library; if data is shared between threads for a single instance,
 * the mutex should belong to the opcode instance.
 */
struct LockGuard {
    LockGuard(CSOUND *csound_, void *mutex_) : csound(csound_), mutex(mutex_)
    {
        csound->LockMutex(mutex);
    }
    ~LockGuard()
    {
        csound->UnlockMutex(mutex);
    }
    CSOUND *csound;
    void *mutex;
};

/**
 * Use this to store a pointer to a global heap-allocated object, e.g. one
 * used to manage state between opcode instances.
 */

template<typename T> int CreateGlobalPointer(CSOUND *csound, const char *name, T *pointer)
{
    T **pointer_to_pointer = 0;
    int result = csound->CreateGlobalVariable(csound, name, sizeof(pointer_to_pointer));
    pointer_to_pointer = static_cast<T **>(csound->QueryGlobalVariable(csound, name));
    *pointer_to_pointer = pointer;
    return result;
}

/**
 * Retrieve a pointer to a global heap-allocated object, e.g. one
 * used to manage state between opcode instances.
 */
template<typename T> T *QueryGlobalPointer(CSOUND *csound, const char *name, T*& pointer)
{
    T **pointer_to_pointer = static_cast<T **>(csound->QueryGlobalVariableNoCheck(csound, name));
    if (pointer_to_pointer != 0) {
        pointer = *pointer_to_pointer;
    } else {
        pointer = 0;
    }
    return pointer;
}

template<typename T>
class OpcodeBase
{
public:
    int init(CSOUND *csound)
    {
        return NOTOK;
    }
    static int init_(CSOUND *csound, void *opcode)
    {
        return reinterpret_cast<T *>(opcode)->init(csound);
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
    /**
      This is how to compute audio signals for normal opcodes:
      (1) Zero all frames from 0 up to but not including Offset.
      (2) Compute all frames from ksmps_offset up to but not including End.
      (3) Zero all frames from End up to but not including ksmps.
      Example from a C opcode:
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n = offset; n < nsmps; n++) {
        input1 = MYFLT2LRND(p->a[n]);
        p->r[n] = (MYFLT) (input1 >> input2);
      }
      So in C++ it should look like this (which is much easier to understand):
      int frameIndex = 0;
      for( ; frameIndex < kperiodOffset(); ++frameIndex) {
          asignal[frameIndex] = 0;
      }
      for( ; frameIndex < kperiodEnd(); ++frameIndex) {
          asignal[frameIndex] = compute();
      }
      for( ; frameIndex < ksmps(); ++frameIndex) {
          asignal[frameIndex] = 0;
      }
     */
    uint32_t kperiodOffset() const
    {
        return opds.insdshead->ksmps_offset;
    }
    uint32_t kperiodEnd() const
    {
        uint32_t end = opds.insdshead->ksmps_no_end;
        if (end) {
            return end;
        } else {
            return ksmps();
        }
    }
    uint32_t ksmps() const
    {
        return opds.insdshead->ksmps;
    }
    void log(CSOUND *csound, const char *format,...)
    {
        va_list args;
        va_start(args, format);
        if(csound) {
            csound->MessageV(csound, 0, format, args);
        } else {
            vfprintf(stdout, format, args);
        }
        va_end(args);
    }
    void warn(CSOUND *csound, const char *format,...)
    {
        if(csound) {
            if(csound->GetMessageLevel(csound) & WARNMSG) {
                va_list args;
                va_start(args, format);
                csound->MessageV(csound, CSOUNDMSG_WARNING, format, args);
                va_end(args);
            }
        } else {
            va_list args;
            va_start(args, format);
            vfprintf(stdout, format, args);
            va_end(args);
        }
    }
    OPDS opds;
};

template<typename T>
class OpcodeNoteoffBase
{
public:
    int init(CSOUND *csound)
    {
      IGN(csound);
        return NOTOK;
    }
    static int init_(CSOUND *csound, void *opcode)
    {
        if (!csound->GetReinitFlag(csound) && !csound->GetTieFlag(csound)) {
            csound->RegisterDeinitCallback(csound, opcode,
                                           &OpcodeNoteoffBase<T>::noteoff_);
        }
        return reinterpret_cast<T *>(opcode)->init(csound);
    }
    int kontrol(CSOUND *csound)
    {
        IGN(csound);
        return NOTOK;
    }
    static int kontrol_(CSOUND *csound, void *opcode)
    {
        return reinterpret_cast<T *>(opcode)->kontrol(csound);
    }
    int audio(CSOUND *csound)
    {
        IGN(csound);
        return NOTOK;
    }
    static int audio_(CSOUND *csound, void *opcode)
    {
        return reinterpret_cast<T *>(opcode)->audio(csound);
    }
    /**
      This is how to compute audio signals for normal opcodes:
      (1) Zero all frames from 0 up to but not including Offset.
      (2) Compute all frames from ksmps_offset up to but not including End.
      (3) Zero all frames from End up to but not including ksmps.
      Example from a C opcode:
      uint32_t offset = p->h.insdshead->ksmps_offset;
      uint32_t early  = p->h.insdshead->ksmps_no_end;
      uint32_t n, nsmps = CS_KSMPS;
      if (UNLIKELY(offset)) memset(p->r, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&p->r[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n = offset; n < nsmps; n++) {
        input1 = MYFLT2LRND(p->a[n]);
        p->r[n] = (MYFLT) (input1 >> input2);
      }
      So in C++ it should look like this (which is much easier to understand):
      int frameIndex = 0;
      for( ; frameIndex < kperiodOffset(); ++frameIndex) {
          asignal[frameIndex] = 0;
      }
      for( ; frameIndex < kperiodEnd(); ++frameIndex) {
          asignal[frameIndex] = compute();
      }
      for( ; frameIndex < ksmps(); ++frameIndex) {
          asignal[frameIndex] = 0;
      }
     */
    uint32_t kperiodOffset() const
    {
        return opds.insdshead->ksmps_offset;
    }
    uint32_t kperiodEnd() const
    {
        uint32_t end = opds.insdshead->ksmps_no_end;
        if (end) {
            return end;
        } else {
            return ksmps();
        }
    }
    uint32_t ksmps() const
    {
        return opds.insdshead->ksmps;
    }
    void log(CSOUND *csound, const char *format,...)
    {
        va_list args;
        va_start(args, format);
        if(csound) {
            csound->MessageV(csound, 0, format, args);
        } else {
            vfprintf(stdout, format, args);
        }
        va_end(args);
    }
    void warn(CSOUND *csound, const char *format,...)
    {
        if(csound) {
            if(csound->GetMessageLevel(csound) & WARNMSG) {
                va_list args;
                va_start(args, format);
                csound->MessageV(csound, CSOUNDMSG_WARNING, format, args);
                va_end(args);
            }
        } else {
            va_list args;
            va_start(args, format);
            vfprintf(stdout, format, args);
            va_end(args);
        }
    }
    int noteoff(CSOUND *csound)
    {
        return OK;
    }
    static int noteoff_(CSOUND *csound, void *opcode)
    {
        return reinterpret_cast<T *>(opcode)->noteoff(csound);
    }
    OPDS opds;
};

};

#endif
