/**
  plugin.h
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
#include <csdl.h>
#include <algorithm>

namespace csnd {

/** opcode thread function template 
*/
template <typename T> int init(CSOUND *csound, T *p) {
  p->csound = csound;
  return p->init();
}
template <typename T> int kperf(CSOUND *csound, T *p) {
  p->csound = csound;
  return p->kperf();
}
template <typename T> int aperf(CSOUND *csound, T *p) {
  p->csound = csound;
  return p->aperf();
}

/** plugin registration function template 
 */
template <typename T>
int plugin(CSOUND *csound, const char *name, int thread, const char *oargs,
          const char *iargs) {
  return csound->AppendOpcode(csound, (char *)name, sizeof(T), 0, thread,
                       (char *)oargs, (char *)iargs, (SUBR)init<T>,
                       (SUBR)kperf<T>, (SUBR)aperf<T>);
}

/** function table container class 
 */
class Table : FUNC {
  
public:
  
  /** Initialise this object from an opcode
      argument arg */
  int init(CSOUND *csound, MYFLT *arg) {
    Table *f = (Table *) csound->FTnp2Find(csound,arg);
    if(f != nullptr) {
     std::copy(f,f+1,this);
     return OK;
    } return NOTOK;
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
  const MYFLT &operator[](int n) const  { return ftable[n]; }

  /** function table data pointer
   */
  MYFLT *data() const { return ftable; }

  /** function table length
   */
  uint32_t len() { return flen;}
  
};

/** vector container template using Csound AuxAlloc
    mechanism for dynamic memory allocation
 */
template<typename T>
class AuxMem : AUXCH {
  
public:
  /** allocate memory for the container 
   */
  void allocate(CSOUND *csound, int n) {
    int bytes = n*sizeof(T);
    if(auxp == nullptr || size < bytes) {
      csound->AuxAlloc(csound,bytes,(AUXCH *) this);
      std::fill((T *)auxp,(T *)auxp+n,0);
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
  iterator begin() { return (T *) auxp; }

  /** vector end
   */
  iterator end() { return (T *) auxp + len(); }

  /** array subscript access (write)
   */
  T &operator[](int n) { return ((T*)auxp)[n]; }

  /** array subscript access (read)
   */
  const T &operator[](int n) const  { return ((T*)auxp)[n]; }

  /** returns a pointer to the vector data
   */
  T *data() { return (T *) auxp;}

  /** returns the length of the vector
   */
  uint32_t len() { return size/sizeof(T); }
  
};

/** Parameters template class
 */
template<uint32_t N>
class Params {
  MYFLT *ptrs[N];
public:

  /** parameter access via array subscript (write)
   */
  MYFLT &operator[](int n) { return *ptrs[n]; }

  /** parameter access via array subscript (read)
   */
  const MYFLT &operator[](int n) const  { return *ptrs[n]; }

  /** parameter data (raw pointer) at index n
   */
  MYFLT *data(int n) const { return ptrs[n]; }
};

/** Plugin template base class: 
    N outputs and M inputs
 */
template<uint32_t N,uint32_t M>
struct Plugin : OPDS {
  Params<N> outargs;
  Params<M> inargs;
  CSOUND *csound;
  uint32_t offset;
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

  /** sample-accurate offset for
      a-rate opcodes; updates offset
      and nsmps 
   */
  void sa_offset(MYFLT *v) {
    uint32_t early  = insdshead->ksmps_no_end;
    nsmps = insdshead->ksmps - early;
    offset = insdshead->ksmps_offset;
    if(UNLIKELY(offset))
       std::fill(v,v+offset,0);
    if(UNLIKELY(early))
       std::fill(v+nsmps,v+nsmps+early,0);
  }
};
 
}
 
#endif
