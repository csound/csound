/*
  plugin.h
  CPOF Csound Plugin Opcode Framework
  C++ plugin opcode interface

  (c) Victor Lazzarini, 2017

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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA

*/

#ifndef _PLUGIN_H_
#define _PLUGIN_H_
#include <algorithm>
#include <complex>
#include <csdl.h>
#include <pstream.h>

namespace csnd {

/** opcode threads: i-time, k-perf and/or a-perf
*/
enum thread { i = 1, k = 2, ik = 3, a = 4, ia = 5, ika = 7 };

/** fsig formats: phase vocoder, stft polar, stft complex, or
    sinusoidal tracks
*/
enum fsig_format { pvs = 0, polar, complex, tracks };

/** Csound Engine object.
 */
class Csound : CSOUND {
public:
  /** Utility classes with full access
   */
  template <typename T> friend class Vector;
  friend class Fsig;
  friend class Table;
  template <typename T> friend class AuxMem;

  /** init-time error message
   */
  int init_error(const std::string &s) {
    return InitError(this, "%s\n", s.c_str());
  }

  /** perf-time error message
   */
  int perf_error(const std::string &s, INSDS *inst) {
    return PerfError(this, inst, "%s\n", s.c_str());
  }

  /** warning message
   */
  void warning(const std::string &s) { Warning(this, "%s", s.c_str()); }

  /** console messages
   */
  void message(const std::string &s) { Message(this, "%s\n", s.c_str()); }

  /** system sampling rate
   */
  MYFLT sr() { return GetSr(this); }

  /** system max amp reference
   */
  MYFLT _0dbfs() { return Get0dBFS(this); }

  /** system A4 reference
   */
  MYFLT _A4() { return GetA4(this); }

  /** number of audio channels (out)
   */
  uint32_t nchnls() { return GetNchnls(this); }

  /** number of audio channels (in)
   */
  uint32_t nchnls_i() { return GetNchnls_i(this); }

  /** time count (samples)
   */
  int64_t current_time_samples() { return GetCurrentTimeSamples(this); }

  /** time count (seconds)
   */
  double current_time_seconds() {
    return GetCurrentTimeSamples(this) / GetSr(this);
  }

  /** midi channel number for this instrument
   */
  int midi_channel() { return GetMidiChannelNumber(this); }

  /** midi note number for this instrument
   */
  int midi_note_num() { return GetMidiNoteNumber(this); }

  /** midi note velocity for this instrument
   */
  int midi_note_vel() { return GetMidiVelocity(this); }

  /** midi aftertouch for this channel
   */
  MYFLT midi_chn_aftertouch() { return GetMidiChannel(this)->aftouch; }

  /** midi poly aftertouch for this channel
   */
  MYFLT midi_chn_polytouch(uint32_t note) {
    return GetMidiChannel(this)->polyaft[note];
  }

  /** midi ctl change for this channel
   */
  MYFLT midi_chn_ctl(uint32_t ctl) {
    return GetMidiChannel(this)->ctl_val[ctl];
  }

  /** midi pitchbend for this channel
   */
  MYFLT midi_chn_pitchbend() { return GetMidiChannel(this)->pchbend; }

  /** list of active instrument instances for this channel \n
      returns an INSDS array with 128 items, one per
      MIDI note number. Inactive instances are marked NULL.
   */
  const INSDS *midi_chn_list() {
    return (const INSDS *)GetMidiChannel(this)->kinsptr;
  }

  /** FFT setup: real-to-complex and complex-to-real \n
      direction: FFT_FWD or FFT_INV \n
      returns a handle to the FFT setup.
   */
  void *rfft_setup(uint32_t size, uint32_t direction) {
    return RealFFT2Setup(this, size, direction);
  }

  /** FFT operation, in-place, but also
      returning a pointer to std::complex<MYFLT>
      to the transformed data memory.
  */
  std::complex<MYFLT> *rfft(void *setup, MYFLT *data) {
    RealFFT2(this, setup, data);
    return reinterpret_cast<std::complex<MYFLT> *>(data);
  }
};

/** One-dimensional array container
    template class
 */
template <typename T> class Vector : ARRAYDAT {
public:
  /** Initialise the container
   */
  void init(Csound *csound, int size) {
    if (data == NULL || dimensions == 0 ||
        (dimensions == 1 && sizes[0] < size)) {
      size_t ss;
      if (data == NULL) {
        CS_VARIABLE *var = arrayType->createVariable(csound, NULL);
        arrayMemberSize = var->memBlockSize;
      }
      ss = arrayMemberSize * size;
      if (data == NULL)
        data = (MYFLT *)csound->Calloc(csound, ss);
      else
        data = (MYFLT *)csound->ReAlloc(csound, data, ss);
      dimensions = 1;
      sizes = (int *)csound->Malloc(csound, sizeof(int));
      sizes[0] = size;
    }
  }
  /** iterator type
   */
  typedef T *iterator;

  /** const_iterator type
  */
  typedef const T *const_iterator;

  /** vector beginning
   */
  iterator begin() { return (T *)data; }

  /** vector end
   */
  iterator end() { return (T *)data + sizes[0]; }

  /** array subscript access (write)
   */
  T &operator[](int n) { return ((T *)data)[n]; }

  /** array subscript access (read)
   */
  const T &operator[](int n) const { return ((T *)data)[n]; }

  /** array subscript access (read)
   */
  uint32_t len() { return sizes[0]; }

  /** array data
   */
  T *data_array() { return (T *)data; }
};

typedef std::complex<float> pvscmplx;
typedef std::complex<MYFLT> sldcmplx;

/** pvsbin translation class for fsig_format::pvs. \n
    Holds a phase vocoder bin, and
    also allows alternative access via a std::complex
    implicit or explicit cast.
 */
class pvsbin {
  float am;
  float fr;

public:
  /** constructor
   */
  pvsbin() : am(0.f), fr(0.f){};

  /** access amplitude
   */
  float amp() { return am; }

  /** access frequency
   */
  float freq() { return fr; }

  /** set amplitude
   */
  void amp(float a) { am = a; }

  /** set frequency
   */
  void freq(float f) { fr = f; }

  /** multiplication (unary)
   */
  const pvsbin &operator*=(const pvsbin &bin) {
    am *= bin.am;
    fr = bin.fr;
    return *this;
  }

  /** multiplication (binary)
   */
  pvsbin operator*(const pvsbin &a) {
    pvsbin res = *this;
    return (res *= a);
  }

  /** multiplication by MYFLT (unary)
   */
  const pvsbin &operator*=(MYFLT f) {
    am *= f;
    return *this;
  }

  /** multiplication by MYFLT (binary)
   */
  pvsbin operator*(MYFLT f) {
    pvsbin res = *this;
    return (res *= f);
  }

  /** cast to std::complex<float>&
   */
  operator pvscmplx &() {
    return (pvscmplx &)reinterpret_cast<float(&)[2]>(*this);
  }

  /** cast to std::complex<float>*
   */
  operator pvscmplx *() { return (pvscmplx *)reinterpret_cast<float *>(this); }
};

/** fsig container class
 */
class Fsig : PVSDAT {
public:
  /** initialise the container
   */
  void init(Csound *csound, int32_t n, int32_t h, int32_t w, int32_t t,
            int32_t f, int32_t nb = 0, int32_t sl = 0, uint32_t nsmps = 1) {
    N = n;
    overlap = h;
    winsize = w;
    wintype = t;
    format = f;
    NB = nb;
    sliding = sl;
    if (!sliding) {
      int bytes = (n + 2) * sizeof(float);
      if (frame.auxp == nullptr || frame.size < bytes) {
        csound->AuxAlloc(csound, bytes, &frame);
        std::fill((float *)frame.auxp, (float *)frame.auxp + n + 2, 0);
      }
    } else {
      int bytes = (n + 2) * sizeof(MYFLT) * nsmps;
      if (frame.auxp == NULL || frame.size < bytes)
        csound->AuxAlloc(csound, bytes, &frame);
    }
    framecount = 1;
  }
  void init(Csound *csound, const Fsig &f, uint32_t nsmps = 1) {
    init(csound, f.N, f.overlap, f.winsize, f.wintype, f.format, f.NB,
         f.sliding, nsmps);
  }

  /** iterator type
  */
  typedef pvsbin *iterator;

  /** const_iterator type
  */
  typedef const pvsbin *const_iterator;

  /** returns an iterator to the
      beginning of the frame
   */
  iterator begin() { return (pvsbin *)frame.auxp; }

  /** returns an iterator to the
       end of the frame
    */
  iterator end() { return (pvsbin *)frame.auxp + N / 2 + 1; }

  /** array subscript access operator (write)
   */
  pvsbin &operator[](int n) { return ((pvsbin *)frame.auxp)[n]; }

  /** array subscript access operator (read)
   */
  const pvsbin &operator[](int n) const { return ((pvsbin *)frame.auxp)[n]; }

  /** function table data pointer
   */
  pvsbin *data() const { return (pvsbin *)frame.auxp; }

  /** function table data pointer (sliding case)
   */
  sldcmplx *data_sliding() const { return (sldcmplx *)frame.auxp; }

  /** frame length
   */
  uint32_t len() { return N / 2 + 1; }

  /** get framecount
   */
  uint32_t count() const { return framecount; }

  /** set framecount
   */
  uint32_t count(uint32_t cnt) { return (framecount = cnt); }

  /** check for sliding mode
   */
  bool isSliding() { return (bool)sliding; }

  /** get fsig data format
   */
  int fsig_format() { return format; }
};

/** function table container class
 */
class Table : FUNC {

public:
  /** Initialise this object from an opcode
      argument arg */
  int init(Csound *csound, MYFLT *arg) {
    Table *f = (Table *)csound->FTnp2Find(csound, arg);
    if (f != nullptr) {
      std::copy(f, f + 1, this);
      return OK;
    }
    return NOTOK;
  }

  /** iterator type
  */
  typedef MYFLT *iterator;

  /** const_iterator type
  */
  typedef const MYFLT *const_iterator;

  /** returns an iterator to the
      beginning of the table
   */
  iterator begin() { return ftable; }

  /** returns an iterator to the
       end of the table
    */
  iterator end() { return ftable + flen; }

  /** array subscript access operator (write)
   */
  MYFLT &operator[](int n) { return ftable[n]; }

  /** array subscript access operator (read)
   */
  const MYFLT &operator[](int n) const { return ftable[n]; }

  /** function table data pointer
   */
  MYFLT *data() const { return ftable; }

  /** function table length
   */
  uint32_t len() { return flen; }
};

/** vector container template using Csound AuxAlloc
    mechanism for dynamic memory allocation
 */
template <typename T> class AuxMem : AUXCH {

public:
  /** allocate memory for the container
   */
  void allocate(Csound *csound, int n) {
    int bytes = n * sizeof(T);
    if (auxp == nullptr || size < bytes) {
      csound->AuxAlloc(csound, bytes, (AUXCH *)this);
      std::fill((T *)auxp, (T *)auxp + n, 0);
    }
  }

  /** iterator type
  */
  typedef T *iterator;

  /** const_iterator type
  */
  typedef const T *const_iterator;

  /** vector beginning
   */
  iterator begin() { return (T *)auxp; }

  /** vector end
   */
  iterator end() { return (T *)endp; }

  /** array subscript access (write)
   */
  T &operator[](int n) { return ((T *)auxp)[n]; }

  /** array subscript access (read)
   */
  const T &operator[](int n) const { return ((T *)auxp)[n]; }

  /** returns a pointer to the vector data
   */
  T *data() { return (T *)auxp; }

  /** returns the length of the vector
   */
  uint32_t len() { return size / sizeof(T); }
};

/** Parameters template class
 */
template <uint32_t N> class Params {
  MYFLT *ptrs[N];

public:
  /** parameter access via array subscript (write)
   */
  MYFLT &operator[](int n) { return *ptrs[n]; }

  /** parameter access via array subscript (read)
   */
  const MYFLT &operator[](int n) const { return *ptrs[n]; }

  /** parameter data (MYFLT pointer) at index n
   */
  MYFLT *data(int n) { return ptrs[n]; }

  /** parameter string data (STRINGDAT ref) at index n
   */
  STRINGDAT &str_data(int n) { return (STRINGDAT &)*ptrs[n]; }

  /** parameter fsig data (PVSDAT ref) at index n
   */
  Fsig &fsig_data(int n) { return (Fsig &)*ptrs[n]; }

  /** 1-D array data
   */
  template <typename T> Vector<T> &vector_data(int n) {
    return (Vector<T> &)*ptrs[n];
  }
};

/** Plugin template base class:
    N outputs and M inputs
 */
template <uint32_t N, uint32_t M> struct Plugin : OPDS {
  Params<N> outargs;
  Params<M> inargs;
  Csound *csound;
  uint32_t offset;
  uint32_t nsmps;

  /** i-time function placeholder
   */
  int init() {
    nsmps = insdshead->ksmps;
    offset = 0;
    return OK;
  }

  /** k-rate function placeholder
   */
  int kperf() { return OK; }

  /** a-rate function placeholder
   */
  int aperf() { return OK; }

  /** sample-accurate offset for
      a-rate opcodes; updates offset
      and nsmps
   */
  void sa_offset(MYFLT *v) {
    uint32_t early = insdshead->ksmps_no_end;
    nsmps = insdshead->ksmps - early;
    offset = insdshead->ksmps_offset;
    if (UNLIKELY(offset))
      std::fill(v, v + offset, 0);
    if (UNLIKELY(early))
      std::fill(v + nsmps, v + nsmps + early, 0);
  }
};

/** Fsig plugin template base class:
    N outputs and M inputs
 */
template <uint32_t N, uint32_t M> struct FPlugin : Plugin<N, M> {
  uint32_t framecount;
};

/** opcode thread function template (i-time)
*/
template <typename T> int init(CSOUND *csound, T *p) {
  p->csound = (Csound *)csound;
  return p->init();
}

/** opcode thread function template (k-rate)
*/
template <typename T> int kperf(CSOUND *csound, T *p) {
  p->csound = (Csound *)csound;
  return p->kperf();
}

/** opcode thread function template (a-rate)
*/
template <typename T> int aperf(CSOUND *csound, T *p) {
  p->csound = (Csound *)csound;
  return p->aperf();
}

/** plugin registration function template
 */
template <typename T>
int plugin(CSOUND *csound, const char *name, const char *oargs,
           const char *iargs, uint32_t thread, uint32_t flags = 0) {
  return csound->AppendOpcode(csound, (char *)name, sizeof(T), flags, thread,
                              (char *)oargs, (char *)iargs, (SUBR)init<T>,
                              (SUBR)kperf<T>, (SUBR)aperf<T>);
}

/** plugin registration function template
    for classes with self-defined opcode argument types
 */
template <typename T>
int plugin(CSOUND *csound, const char *name, uint32_t thread,
           uint32_t flags = 0) {
  return csound->AppendOpcode(csound, (char *)name, sizeof(T), 0, thread,
                              (char *)T::otypes, (char *)T::itypes,
                              (SUBR)init<T>, (SUBR)kperf<T>, (SUBR)aperf<T>);
}
}
#endif
