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
#pragma once

/**
 * There is a conflict between the preprocessor definition "_CR" in the
 * standard C++ library and in Csound. To work around this, undefine "_CR" and
 * include ALL standard library include files BEFORE including ANY Csound
 * include files.
 */
#if defined(_CR)
#undef _CR
#endif

#include <algorithm>
#include <cstdarg>
#include <map>
#if !(defined(__wasi__))
#include <mutex>
#endif
#include <string>
#include <vector>
#include <interlocks.h>
#include <csdl.h>

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

namespace csound {

static bool diagnostics_enabled = false;

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
    uint32_t output_arg_count()
    {
        return (uint32_t)opds.optext->t.outArgCount;
    }
    uint32_t input_arg_count()
    {
        return (uint32_t)opds.optext->t.inArgCount;
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
            if(csound->GetMessageLevel(csound) & CS_WARNMSG) {
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
    uint32_t output_arg_count()
    {
        return (uint32_t)opds.optext->t.outArgCount;
    }
    uint32_t input_arg_count()
    {
        return (uint32_t)opds.optext->t.inArgCount;
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
            if(csound->GetMessageLevel(csound) & CS_WARNMSG) {
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

#if (__cplusplus >= 201103L) && !(defined(__wasi__))
#pragma message("Defining heap_object_manager_t.")

/**
 * The memory of non-POD C++ or C objects allocated on the heap by Csound
 * plugins is managed not by Csound, but by the plugin module. This class
 * performs that function.
 *
 * As long as all memory is allocated in or after csoundModuleCreate and
 * de-allocated in csoundModuleDestroy, all will be well. The danger however
 * is that memory leaks will occur if allocations are not matched with
 * de-allocations, or worse, that crashes will occur if any objects are
 * deleted twice or de-referenced after deletion.
 *
 * So, the plugin should manage a collection of all heap-allocated objects,
 * keyed first off the Csound instance, and second off an object handle; and
 * all such objects must be accessed only by handle, from this collection.
 *
 * In csoundModuleDestroy, all objects for the calling instance of Csound
 * should be de-allocated, and the sub-collection for that instance should
 * also be de-allocated.
 *
 * If then there are no more sub-collections for other instances of Csound,
 * the entire contents of the collection should be de-allocated.
 *
 * This should be done as a singleton, and in a thread-safe way.
 *
 * There might still be a problem if any of the heap-allocated objects,
 * themselves, have global static members, perhaps created by some external
 * dependency.
 *
 * Please note, objects O in the declaration below must not be std::shared_ptr
 * objects. Deletion of objects herein must be unconditional. Rather, the
 * objects passed are raw pointers to instances of class O.
 */
 template<typename O> class heap_object_manager_t {
    private:
        std::map<CSOUND *, std::vector<O*>> objects_;
        std::recursive_mutex mutex;
        heap_object_manager_t(){};
        ~heap_object_manager_t(){};
    public:
        static heap_object_manager_t &instance() {
            static heap_object_manager_t singleton;
            return singleton;
        }
        std::map<CSOUND *, std::vector<O*>> &objects() {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            return objects_;
        }
        /**
         * Returns a list of pointers to all objects allocated for this
         * instance of Csound.
         */
        std::vector<O*> &objects_for_csound(CSOUND *csound) {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            auto &objects_for_csound_ = objects()[csound];
            return objects_for_csound_;
        }
        /**
         * Returns the handle for the object; if the object has not yet been
         * stored, inserts it into the list of object pointers for this
         * instance of Csound; otherwise, returns the handle of the stored
         * object pointer.
         */
        int handle_for_object(CSOUND *csound, O *object) {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            auto &objects_for_csound_ = objects_for_csound(csound);
            auto iterator = std::find(objects_for_csound_.begin(), objects_for_csound_.end(), object);
            if (iterator == objects_for_csound_.end()) {
                int handle = objects_for_csound_.size();
                objects_for_csound_.push_back(object);
                 if (diagnostics_enabled) std::fprintf(stderr, "heap_object_manager_t::handle_for_object %p: new object handle: %d (of %ld)\n", object, handle, objects_for_csound_.size());
                return handle;
            } else {
                int handle = static_cast<int>(iterator - objects_for_csound_.begin());
                 if (diagnostics_enabled) std::fprintf(stderr, "heap_object_manager_t::handle_for_object: existing object handle: %d\n", handle);
                return handle;
            }
        }
        /**
         * Returns the object pointer for the handle;
         * if the object pointer has not been stored by
         * handle, returns a null pointer.
         */
        O *object_for_handle(CSOUND *csound, int handle) {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            auto &objects_for_csound_ = objects_for_csound(csound);
            if (handle >= objects_for_csound_.size()) {
                return nullptr;
            }
            O *object = objects_for_csound_[handle];
            if (diagnostics_enabled) std::fprintf(stderr, "heap_object_manager_t::object_for_handle: %p %d (of %ld)\n", object, handle, objects_for_csound_.size());
            return object;
        }
        /**
         * First destroys all objects created by the calling
         * instance of Csound, then destroys the list of
         * object pointers for this instance of Csound.
         */
        void module_destroy(CSOUND *csound) {
            std::lock_guard<std::recursive_mutex> lock(mutex);
            auto &objects_for_csound_ = objects_for_csound(csound);
            for (int i = 0, n = objects_for_csound_.size(); i < n; ++i) {
                delete objects_for_csound_[i];
                objects_for_csound_[i] = nullptr;
            }
            objects_for_csound_.clear();
            objects().erase(csound);
        }
};

#else
#pragma message("Not defining heap_manager_t.")
#endif

}
