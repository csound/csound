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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA

*/

#ifndef _PLUGIN_H_
#define _PLUGIN_H_
#include "csdl.h"
#include "pstream.h"
#include "arrays.h"
#include <array>
#include <algorithm>
#include <complex>
#include <cstring>
#include <iostream>

namespace csnd {

/* constants */
const double twopi = TWOPI;

/** opcode threads: i-time, k-perf and/or a-perf
*/
enum thread { i = 1, k = 2, ik = 3, a = 4, ia = 5 /*, ika = 3*/ };

/** fsig formats: phase vocoder, stft polar, stft complex, or
    sinusoidal tracks
*/
enum fsig_format { pvs = 0, polar, complex, tracks };

typedef CSOUND_FFT_SETUP *fftp;

/** Csound Engine object.
 */
class Csound : CSOUND {

  /** Utility classes
   */
  template <typename T> friend class Vector;
  friend class Fsig;
  friend class Table;
  template <typename T> friend class AuxMem;

  /**
    @private
    opcode function template (deinit-time)
   */
  template <typename T> static int deinit(CSOUND *csound, void *p) {
    return ((T *)p)->deinit();
  }

public:
  /** Host Data
   */
  void *host_data() { return GetHostData(this); }

  /** init-time error message
   */
  int init_error(const std::string &s) {
    return InitError(this, "%s\n", LocalizeString(s.c_str()));
  }

  /** perf-time error message
   */
  int perf_error(const std::string &s, OPDS *inst) {
    return PerfError(this, inst, "%s\n", LocalizeString(s.c_str()));
  }

  /** warning message
   */
  void warning(const std::string &s) {
    Warning(this, "%s", LocalizeString(s.c_str()));
  }

  /** console messages
   */
  void message(const std::string &s) {
    Message(this, "%s\n", LocalizeString(s.c_str()));
  }

  /** system sampling rate
   */
  MYFLT sr() { return GetSr(this); }

  /** system control rate
   */
  MYFLT kr() { return GetKr(this); }

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

  /** check for audio signal variable argument
   */
  bool is_asig(void *arg) {
    return !std::strcmp(GetTypeForArg(arg)->varTypeName, "a");
  }

  /** midi channel number for this instrument
   */
  int midi_channel(OPDS *p) { return GetMidiChannelNumber(p); }

  /** midi note number for this instrument
   */
  int midi_note_num(OPDS *p) { return GetMidiNoteNumber(p); }

  /** midi note velocity for this instrument
   */
  int midi_note_vel(OPDS *p) { return GetMidiVelocity(p); }

  /** midi aftertouch for this channel
   */
  MYFLT midi_chn_aftertouch(OPDS *p) {
    return GetMidiChannel(p)->aftouch; }

  /** midi poly aftertouch for this channel
   */
  MYFLT midi_chn_polytouch(OPDS *p, uint32_t note) {
    return GetMidiChannel(p)->polyaft[note];
  }

  /** midi ctl change for this channel
   */
  MYFLT midi_chn_ctl(OPDS *p, uint32_t ctl) {
    return GetMidiChannel(p)->ctl_val[ctl];
  }

  /** midi pitchbend for this channel
   */
  MYFLT midi_chn_pitchbend(OPDS *p) {
    return GetMidiChannel(p)->pchbend;
  }

  /** list of active instrument instances for this channel \n
      returns an INSDS array with 128 items, one per
      MIDI note number. Inactive instances are marked NULL.
   */
  const INSDS *midi_chn_list(OPDS *p) {
    return (const INSDS *) GetMidiChannel(p)->kinsptr;
  }

  /** deinit registration for a given plugin class
   */
  template <typename T> void plugin_deinit(T *p) {
    RegisterDeinitCallback(this, (void *)p, deinit<T>);
  }

  /** Csound memory allocation - malloc style
   */
  void *malloc(size_t size) { return Malloc(this, size); }

  /** Csound memory allocation - calloc style
   */
  void *calloc(size_t size) { return Calloc(this, size); }

  /** Csound memory re-allocation
   */
  void *realloc(void *p, size_t size) { return ReAlloc(this, p, size); }

  /** Csound string duplication
   */
  char *strdup(char *s) { return Strdup(this, s); }

  /** Csound memory de-allocation
   */
  void free(void *p) { Free(this, p); }

  /** FFT setup: real-to-complex and complex-to-real \n
      direction: FFT_FWD or FFT_INV \n
      returns a handle to the FFT setup.
   */
  fftp fft_setup(uint32_t size, uint32_t direction) {
    return (fftp)RealFFT2Setup(this, size, direction);
  }

  /** FFT operation, in-place, but also
      returning a pointer to std::complex<MYFLT>
      to the transformed data memory.
  */
  std::complex<MYFLT> *rfft(fftp setup, MYFLT *data) {
    if (!setup->p2) {
      if (setup->d == FFT_FWD)
        RealFFTnp2(this, data, setup->N);
      else
        InverseRealFFTnp2(this, data, setup->N);
    } else
      RealFFT2(this, setup, data);
    return reinterpret_cast<std::complex<MYFLT> *>(data);
  }

  /** FFT operation for complex data, in-place, but also
      returning a pointer to std::complex<MYFLT>
      to the transformed data memory.
  */
  std::complex<MYFLT> *fft(fftp setup, std::complex<MYFLT> *data) {
    MYFLT *fdata = reinterpret_cast<MYFLT *>(data);
    if (setup->d == FFT_FWD)
      ComplexFFT(this, fdata, setup->N);
    else
      ComplexFFT(this, fdata, setup->N);
    return reinterpret_cast<std::complex<MYFLT> *>(fdata);
  }

  /** Creates a global variable in the current Csound object
  */
  int create_global_variable(const char *name, size_t nbytes) {
    return CreateGlobalVariable(this, name, nbytes);
  }

  /** Retrieves a ptr for an existing named global variable
   */
  void *query_global_variable(const char* name) {
    return QueryGlobalVariable(this, name);
  }

  /** Destroy an existing named global variable
   */
  int destroy_global_variable(const char* name) {
     return DestroyGlobalVariable(this, name);
  }

  /** Access to the base CSOUND object
   */
  CSOUND *get_csound() {
    return this;
  }

  /** Sleep
   */
  void sleep(int ms) { Sleep(ms); }
};

/**
  Thread pure virtual base class
 */
class Thread {
  void *thread;
  static uintptr_t thrdRun(void *t) { return ((Thread *)t)->run(); }
  virtual uintptr_t run() = 0;

protected:
  Csound *csound;

public:
  Thread(Csound *cs) : csound(cs) {
    CSOUND *p = (CSOUND *)csound;
    thread = p->CreateThread(thrdRun, (void *)this);
  }

  uintptr_t join() {
    CSOUND *p = (CSOUND *)csound;
    return p->JoinThread(thread);
  }
  void *get_thread() { return thread; }
};

/** Class AudioSig wraps an audio signal
 */
class AudioSig {
  uint32_t early;
  uint32_t offset;
  uint32_t nsmps;
  MYFLT *sig;

public:
  /** Constructor takes the plugin object and the
      audio argument pointer, and a reset flag if
      we need to clear an output buffer
   */
  AudioSig(OPDS *p, MYFLT *s, bool res = false)
      : early(p->insdshead->ksmps_no_end), offset(p->insdshead->ksmps_offset),
        nsmps(p->insdshead->ksmps - p->insdshead->ksmps_no_end), sig(s) {
    if (res) {
      std::fill(sig, sig + p->insdshead->ksmps, 0);
    }
  };

  /** iterator type
  */
  typedef MYFLT *iterator;

  /** const_iterator type
  */
  typedef const MYFLT *const_iterator;

  /** vector beginning
   */
  iterator begin() { return sig + offset; }

  /** vector end
   */
  iterator end() { return sig + nsmps; }

  /** vector beginning
   */
  const_iterator begin() const { return sig + offset; }

  /** vector end
   */
  const_iterator end() const { return sig + nsmps; }

  /** vector beginning
   */
  const_iterator cbegin() const { return sig + offset; }

  /** vector end
   */
  const_iterator cend() const { return sig + nsmps; }

  /** array subscript access (write)
   */
  MYFLT &operator[](int n) { return sig[n]; }

  /** array subscript access (read)
   */
  const MYFLT &operator[](int n) const { return sig[n]; }

  /** get early exit sample position
   */
  uint32_t GetEarly() { return early; }

  /** get early exit sample offset
   */
  uint32_t GetOffset() { return offset; }

  /** get number of samples to process
  */
  uint32_t GetNsmps() { return nsmps; }
};

/** One-dimensional array container
    template class
 */
template <typename T> class Vector : ARRAYDAT {

public:
  /** Initialise the container
   */
  void init(Csound *csound, int size) {
    tabinit(csound, this, size);
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
  iterator end() { return (T *)((char *)data + sizes[0] * arrayMemberSize); }

  /** vector beginning
   */
  const_iterator cbegin() const { return (const T *)data; }

  /** vector end
   */
  const_iterator cend() const {
    return (const T *)((char *)data + sizes[0] * arrayMemberSize);
  }

  /** vector beginning
   */
  const_iterator begin() const { return (const T *)data; }

  /** vector end
   */
  const_iterator end() const {
    return (const T *)((char *)data + sizes[0] * arrayMemberSize);
  }

  /** array subscript access (write)
   */
  T &operator[](int n) { return ((T *)data)[n]; }

  /** array subscript access (read)
   */
  const T &operator[](int n) const { return ((T *)data)[n]; }

  /** array subscript access (read)
   */
  uint32_t len() { return sizes[0]; }

  /** element offset
   */
  uint32_t elem_offset() { return arrayMemberSize / sizeof(T); }

  /** array data
   */
  T *data_array() { return (T *)data; }
};

typedef Vector<MYFLT> myfltvec;
typedef std::complex<float> pvscmplx;
typedef std::complex<MYFLT> sldcmplx;

/** Pvbin holds one Phase Vocoder bin
 */
template <typename T> class Pvbin {
  T am;
  T fr;

public:
  /** constructor
   */
  Pvbin() : am((T)0), fr((T)0){};

  /** access amplitude
   */
  T amp() { return am; }

  /** access frequency
   */
  T freq() { return fr; }

  /** set amplitude
   */
  T amp(T a) { return (am = a); }

  /** set frequency
   */
  T freq(T f) { return (fr = f); }

  /** multiplication (unary)
   */
  const Pvbin &operator*=(const Pvbin &bin) {
    am *= bin.am;
    fr = bin.fr;
    return *this;
  }

  /** multiplication (binary)
   */
  Pvbin operator*(const Pvbin &a) {
    Pvbin res = *this;
    return (res *= a);
  }

  /** multiplication by MYFLT (unary)
   */
  const Pvbin &operator*=(MYFLT f) {
    am *= f;
    return *this;
  }

  /** multiplication by MYFLT (binary)
   */
  Pvbin operator*(MYFLT f) {
    Pvbin res = *this;
    return (res *= f);
  }

  /** cast to std::complex<T>&
   */
  operator pvscmplx &() { return (pvscmplx &)reinterpret_cast<T(&)[2]>(*this); }

  /** cast to std::complex<T>*
   */
  operator pvscmplx *() { return (pvscmplx *)reinterpret_cast<T *>(this); }
};

/** Phase Vocoder bin */
typedef Pvbin<float> pv_bin;

/** Sliding Phase Vocoder bin */
typedef Pvbin<MYFLT> spv_bin;

template <typename T> class Pvframe;

/** Phase Vocoder frame */
typedef Pvframe<pv_bin> pv_frame;

/** Sliding Phase Vocoder frame */
typedef Pvframe<spv_bin> spv_frame;

/** fsig base class, holds PVSDAT data
 */
class Fsig : protected PVSDAT {
public:
  /** initialise the object, allocating memory
      if necessary.
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
      size_t bytes = (n + 2) * sizeof(float);
      if (frame.auxp == nullptr || frame.size < bytes) {
        csound->AuxAlloc(csound, bytes, &frame);
        std::fill((float *)frame.auxp, (float *)frame.auxp + n + 2, 0);
      }
    } else {
      size_t bytes = (n + 2) * sizeof(MYFLT) * nsmps;
      if (frame.auxp == NULL || frame.size < bytes)
        csound->AuxAlloc(csound, bytes, &frame);
    }
    framecount = 1;
  }
  void init(Csound *csound, const Fsig &f, uint32_t nsmps = 1) {
    init(csound, f.N, f.overlap, f.winsize, f.wintype, f.format, f.NB,
         f.sliding, nsmps);
  }

  /** get the DFT size
   */
  uint32_t dft_size() { return N; }

  /** get the analysis hop size
   */
  uint32_t hop_size() { return overlap; }

  /** get the analysis window size
   */
  uint32_t win_size() { return winsize; }

  /** get the window type
   */
  int32_t win_type() { return wintype; }

  /** get the number of bins
   */
  uint32_t nbins() { return N / 2 + 1; }

  /** get the framecount
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

  /** get data frame as floats
   */
  float *data() { return (float *) frame.auxp; }

  /** convert to pv_frame ref
   */
  operator pv_frame &() { return reinterpret_cast<pv_frame &>(*this); }

#ifdef USE_DOUBLE
  /** convert to spv_frame ref
   */
  operator spv_frame &() { return reinterpret_cast<spv_frame &>(*this); }
#endif

};

/**  Container class for a Phase Vocoder
     analysis frame
*/
template <typename T> class Pvframe : public Fsig {

public:
  /** iterator type
  */
  typedef T *iterator;

  /** const_iterator type
  */
  typedef const T *const_iterator;

  /** returns an iterator to the
      beginning of the frame
   */
  iterator begin() { return (T *)frame.auxp; }

  /** returns an iterator to the
       end of the frame
    */
  iterator end() { return (T *)frame.auxp + N / 2 + 1; }

  /** returns a const iterator to the
      beginning of the frame
   */
  const_iterator begin() const { return (const T *)frame.auxp; }

  /** returns a const iterator to the
       end of the frame
    */
  const_iterator end() const { return (const T *)(frame.auxp + N / 2 + 1); }

  /** returns a const iterator to the
      beginning of the frame
   */
  const_iterator cbegin() const { return (const T *)frame.auxp; }

  /** returns a const iterator to the
       end of the frame
    */
  const_iterator cend() const { return (const T *)(frame.auxp + N / 2 + 1); }

  /** array subscript access operator (write)
   */
  T &operator[](int n) { return ((T *)frame.auxp)[n]; }

  /** array subscript access operator (read)
   */
  const T &operator[](int n) const { return ((T *)frame.auxp)[n]; }

  /** frame data pointer
   */
  T *data() const { return (T *)frame.auxp; }

  /** return the container length
   */
  uint32_t len() { return nbins(); }
};

/** function table container class
 */
class Table : FUNC {

public:
  /** Initialise this object from an opcode
      argument arg */
  int init(Csound *csound, MYFLT *arg) {
    Table *f = (Table *)csound->FTnp2Finde(csound, arg);
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

  /** returns a const iterator to the
      beginning of the table
   */
  const_iterator begin() const { return ftable; }

  /** returns a const iterator to the
       end of the table
    */
  const_iterator end() const { return ftable + flen; }

  /** returns a const iterator to the
     beginning of the table
  */
  const_iterator cbegin() const { return ftable; }

  /** returns a const iterator to the
       end of the table
    */
  const_iterator cend() const { return ftable + flen; }

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
    size_t bytes = n * sizeof(T);
    if (auxp == nullptr || size != bytes) {
      csound->AuxAlloc(csound, bytes, (AUXCH *)this);
      std::fill((char *)auxp, (char *)endp, 0);
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

  /** vector beginning (const iterator)
   */
  const_iterator begin() const { return (const T *)auxp; }

  /** vector end  (const iterator)
   */
  const_iterator end() const { return (const T *)endp; }

  /** vector beginning (const iterator)
   */
  const_iterator cbegin() const { return (const T *)auxp; }

  /** vector end  (const iterator)
   */
  const_iterator cend() const { return (const T *)endp; }

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
 template <std::size_t N> class Param {
  std::array<MYFLT *, N> ptrs;

public:
  /** parameter access via array subscript (write)
   */
  MYFLT &operator[](int n) { return *ptrs[n]; }

  /** parameter access via array subscript (read)
   */
  const MYFLT &operator[](int n) const { return *ptrs[n]; }

  /** iterator type
  */
  typedef MYFLT **iterator;

  /** const_iterator type
  */
  typedef const MYFLT **const_iterator;

  /** vector beginning
   */
  iterator begin() { return &ptrs[0]; }

  /** vector end
   */
  iterator end() { return  &ptrs[N]; }

  /** vector beginning
   */
  const_iterator begin() const { return (const MYFLT **)&ptrs[0]; }

  /** vector end
   */
  const_iterator end() const { return (const MYFLT **)&ptrs[N]; }

  /** vector beginning
   */
  const_iterator cbegin() const { return (const MYFLT **)&ptrs[0]; }

  /** vector end
   */
  const_iterator cend() const { return (const MYFLT **)&ptrs[N]; }

  /** parameter data (MYFLT pointer) at index n
   */
  MYFLT *operator()(int n) { return ptrs[n]; }

  /** @private:
       same as operator()
   */
  MYFLT *data(int n) { return ptrs[n]; }

  /** parameter string data (STRINGDAT ref) at index n
   */
  STRINGDAT &str_data(int n) { return (STRINGDAT &)*ptrs[n]; }

  /** parameter fsig data (Fsig ref) at index n
   */
  Fsig &fsig_data(int n) { return (Fsig &)*ptrs[n]; }

  /** 1-D array data as Vector template ref
   */
  template <typename T> Vector<T> &vector_data(int n) {
    return (Vector<T> &)*ptrs[n];
  }

  /** returns 1-D numeric array data
   */
  myfltvec &myfltvec_data(int n) { return (myfltvec &)*ptrs[n]; }

};

/** InPlug template base class:
    for 0 outputs and N inputs
    also for multiple outputs and/or inputs
 */
template <std::size_t N> struct InPlug : OPDS {
  /** arguments */
  Param<N> args;
  /** Csound engine */
  Csound *csound;
  /** sample-accurate offset */
  uint32_t offset;
  /** vector samples to process */
  uint32_t nsmps;

  /** i-time function placeholder
   */
  int init() { return OK; }

  /** k-rate function placeholder
   */
  int kperf() { return OK; }

  /** a-rate function placeholder
   */
  int aperf() { return OK; }

  /** @private
      sample-accurate offset for
      a-rate opcodes; updates offset
      and nsmps. Called implicitly by
      the aperf() method.
   */
  void sa_offset() {
    uint32_t early = insdshead->ksmps_no_end;
    nsmps = insdshead->ksmps - early;
    offset = insdshead->ksmps_offset;
  }

  /** @private
      set nsmps and offset value for kperf()
   */
  void nsmps_set() {
    nsmps = insdshead->ksmps - insdshead->ksmps_no_end;
    offset = insdshead->ksmps_offset;
  }

  /** returns the number of output arguments
      used in the case of variable output count
  */
  uint32_t out_count() { return (uint32_t)optext->t.outArgCount; }

  /** returns the number of input arguments
      used in the case of variable input count
  */
  uint32_t in_count() { return (uint32_t)optext->t.inArgCount; }

  /** local control rate
   */
  MYFLT kr() { return insdshead->ekr; }

  /** local ksmps
   */
  MYFLT ksmps() { return insdshead->ksmps; }

   /** sampling rate
   */
  MYFLT sr() { return csound->sr(); }

  /** midi channel number for this instrument
   */
  int midi_channel() { return ((CSOUND *)csound)->GetMidiChannelNumber(this); }

  /** midi note number for this instrument
   */
  int midi_note_num() { return ((CSOUND *)csound)->GetMidiNoteNumber(this); }

  /** midi note velocity for this instrument
   */
  int midi_note_vel() { return ((CSOUND *)csound)->GetMidiVelocity(this); }

  /** midi aftertouch for this channel
   */
  MYFLT midi_chn_aftertouch() {
    return ((CSOUND *)csound)->GetMidiChannel(this)->aftouch; }

  /** midi poly aftertouch for this channel
   */
  MYFLT midi_chn_polytouch(uint32_t note) {
    return ((CSOUND *)csound)->GetMidiChannel(this)->polyaft[note];
  }

  /** midi ctl change for this channel
   */
  MYFLT midi_chn_ctl(uint32_t ctl) {
    return ((CSOUND *)csound)->GetMidiChannel(this)->ctl_val[ctl];
  }

  /** midi pitchbend for this channel
   */
  MYFLT midi_chn_pitchbend() {
    return ((CSOUND *)csound)->GetMidiChannel(this)->pchbend; }

  /** list of active instrument instances for this channel \n
      returns an INSDS array with 128 items, one per
      MIDI note number. Inactive instances are marked NULL.
   */
  const INSDS *midi_chn_list() {
    return (const INSDS *) ((CSOUND *)csound)->GetMidiChannel(this)->kinsptr;
  }

 /** check if this opcode runs at init time
  */
  bool is_init() {
    return this->iopadr ? true : false;
  }

  /** check if this opcode runs at perf time
  */
  bool is_perf() {
      return this->opaddr ? true : false;
  }

};

/** Plugin template base class:
    for N outputs and M inputs, N > 0
 */
template <std::size_t N, std::size_t M> struct Plugin : OPDS {
  /** output arguments */
  Param<N> outargs;
  /** input arguments */
  Param<M> inargs;
  /** Csound engine */
  Csound *csound;
  /** sample-accurate offset */
  uint32_t offset;
  /** vector samples to process */
  uint32_t nsmps;

  /** i-time function placeholder
   */
  int init() { return OK; }

  /** k-rate function placeholder
   */
  int kperf() { return OK; }

  /** a-rate function placeholder
   */
  int aperf() { return OK; }

  /** @private
      sample-accurate offset for
      a-rate opcodes; updates offset
      and nsmps. Called implicitly by
      the aperf() method.
   */
  void sa_offset() {
    uint32_t early = insdshead->ksmps_no_end;
    nsmps = insdshead->ksmps - early;
    offset = insdshead->ksmps_offset;
    if (UNLIKELY(offset || early))
      for (auto &arg : outargs) {
        if (csound->is_asig(arg)) {
          std::fill(arg, arg + offset, 0);
          std::fill(arg + nsmps, arg + nsmps + early, 0);
        }
      }
  }

  /** @private
      set nsmps and offset value for kperf()
   */
  void nsmps_set() {
    nsmps = insdshead->ksmps - insdshead->ksmps_no_end;
    offset = insdshead->ksmps_offset;
  }

  /** returns the number of output arguments
      used in the case of variable output count
  */
  uint32_t out_count() { return (uint32_t)optext->t.outArgCount; }

  /** returns the number of input arguments
      used in the case of variable input count
  */
  uint32_t in_count() { return (uint32_t)optext->t.inArgCount; }

  /** local control rate
   */
  MYFLT kr() { return insdshead->ekr; }

 /** local ksmps
   */
  MYFLT ksmps() { return insdshead->ksmps; }

   /** sampling rate
   */
  MYFLT sr() { return csound->sr(); }

  /** midi channel number for this instrument
   */
  int midi_channel() { return ((CSOUND *)csound)->GetMidiChannelNumber(this); }

  /** midi note number for this instrument
   */
  int midi_note_num() { return ((CSOUND *)csound)->GetMidiNoteNumber(this); }

  /** midi note velocity for this instrument
   */
  int midi_note_vel() { return ((CSOUND *)csound)->GetMidiVelocity(this); }

  /** midi aftertouch for this channel
   */
  MYFLT midi_chn_aftertouch() {
    return ((CSOUND *)csound)->GetMidiChannel(this)->aftouch; }

  /** midi poly aftertouch for this channel
   */
  MYFLT midi_chn_polytouch(uint32_t note) {
    return ((CSOUND *)csound)->GetMidiChannel(this)->polyaft[note];
  }

  /** midi ctl change for this channel
   */
  MYFLT midi_chn_ctl(uint32_t ctl) {
    return ((CSOUND *)csound)->GetMidiChannel(this)->ctl_val[ctl];
  }

  /** midi pitchbend for this channel
   */
  MYFLT midi_chn_pitchbend() {
    return ((CSOUND *)csound)->GetMidiChannel(this)->pchbend; }

  /** list of active instrument instances for this channel \n
      returns an INSDS array with 128 items, one per
      MIDI note number. Inactive instances are marked NULL.
   */
  const INSDS *midi_chn_list() {
    return (const INSDS *) ((CSOUND *)csound)->GetMidiChannel(this)->kinsptr;
  }

  /** check if this opcode runs at init time
  */
  bool is_init() {
    return this->iopadr ? true : false;
  }

  /** check if this opcode runs at perf time
  */
  bool is_perf() {
      return this->opadr ? true : false;
  }

};


/** Fsig plugin template base class:
    for N outputs and M inputs
 */
template <std::size_t N, std::size_t M> struct FPlugin : Plugin<N, M> {
  /** current frame time index */
  uint32_t framecount;
};

/**
  @private
  opcode thread function template (i-time)
*/
template <typename T> int init(CSOUND *csound, T *p) {
  p->csound = (Csound *)csound;
  return p->init();
}

/**
   @private
   opcode thread function template (k-rate)
*/
template <typename T> int kperf(CSOUND *csound, T *p) {
  p->csound = (Csound *)csound;
  p->nsmps_set();
  return p->kperf();
}

/**
  @private
  opcode thread function template (a-rate)
*/
template <typename T> int aperf(CSOUND *csound, T *p) {
  p->csound = (Csound *)csound;
  p->sa_offset();
  return p->aperf();
}

/** plugin registration function template
 */
template <typename T>
int plugin(Csound *csound, const char *name, const char *oargs,
           const char *iargs, uint32_t thr, uint32_t flags = 0) {
  CSOUND *cs = (CSOUND *)csound;
  if(thr == thread::ia || thr == thread::a) {
  thr = thr == thread::ia ? 3 : 2;
  return cs->AppendOpcode(cs, (char *)name, sizeof(T), flags, thr,
                          (char *)oargs, (char *)iargs, (SUBR)init<T>,
                          (SUBR)aperf<T>, NULL);
  }
  else
  return cs->AppendOpcode(cs, (char *)name, sizeof(T), flags, thr,
                          (char *)oargs, (char *)iargs, (SUBR)init<T>,
                          (SUBR)kperf<T>, NULL);
}

/** plugin registration function template
    for classes with self-defined opcode argument types
 */
template <typename T>
int plugin(Csound *csound, const char *name, uint32_t thr,
           uint32_t flags = 0) {
  CSOUND *cs = (CSOUND *)csound;
  if(thr == thread::ia || thr == thread::a) {
  thr = thr == thread::ia ? 3 : 2;
  return cs->AppendOpcode(cs, (char *)name, sizeof(T), flags, thr,
                          (char *)T::otypes, (char *)T::itypes, (SUBR)init<T>,
                          (SUBR)aperf<T>, NULL);

  }
  else
  return cs->AppendOpcode(cs, (char *)name, sizeof(T), flags, thr,
                          (char *)T::otypes, (char *)T::itypes, (SUBR)init<T>,
                          (SUBR)kperf<T>, NULL);

}

/** utility constructor function template for member classes: \n
    takes the class and constructor types as arguments. \n
    Function takes the allocated memory pointer and constructor
    arguments.\n
 */
template <typename T, typename... Types> T *constr(T *p, Types... args) {
  return new (p) T(args...);
}

template <typename T> void destr(T *p) { p->T::~T(); }
}
#endif
