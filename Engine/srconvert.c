/*
    srconvert.c

    Copyright (C) 2024 Victor Lazzarini

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "srconvert.h"

// sample rate conversion
// src_init() - allocate and initialise converters
// src_convert() - convert
// src_deinit() - deallocate converters
// Upwards: integer convertion ratios are used
// Input is read on first of N calls to src_convert(), N = ratio.
// Output is split into N blocks, taking N calls to drain.
// Downwards: an input of size N is taken and N/ratio
// samples are output.
// Basic linear converter
static SR_CONVERTER *src_linear_init(CSOUND *csound, int32_t mode,
                                     float ratio, CS_VARIABLE *var,
                                     INSDS *ip) {
  IGN(mode);
  int32_t n = 1,  i, size;
  const CS_TYPE *typ = var->varType;
  SR_CONVERTER *pp = (SR_CONVERTER *) csound->Calloc(csound,
                                                     sizeof(SR_CONVERTER));
  pp->var = var;
  pp->csound = csound;
  pp->ip = ip;
  if(typ == &CS_VAR_TYPE_ARRAY) {
    ARRAYDAT *array  = (ARRAYDAT *) (ip->lclbas + var->memBlockIndex);
    typ = var->subType;
    for(i = 0; i < array->dimensions; i++)
     n *= array->sizes[i];
  }

  if(typ->userDefinedType) { // UDTs currently unsupported
    csound->Free(csound, pp);
    csound->Message(csound, "Support for local sampling rate and "
                    "user-defined types not currently available\n");
    return NULL;
  }
  
  if(typ == &CS_VAR_TYPE_A ||
     typ == &CS_VAR_TYPE_K) {
    size = typ == &CS_VAR_TYPE_K ? 1 : ip->ksmps;
    pp->dat = (CVTDAT *) csound->Calloc(csound, sizeof(CVTDAT)*n);
    for(i = 0; i < n; i++) { // one cvt per array item or per asig/ksig var
      pp->dat[i].data = csound->Calloc(csound, sizeof(MYFLT));
      pp->dat[i].bufferin =
        csound->Calloc(csound, size*sizeof(MYFLT)*
                     (ratio > 1 ? ratio : 1./ratio));
    }
    pp->ncvt = n;
    pp->ratio = ratio;
    pp->size = size;
    pp->mode = 4;
  } else // bypass conversion
    pp->ncvt = 0;
  
  return pp;
}

static void src_linear_deinit(CSOUND *csound, SR_CONVERTER *pp) {
  printf("ncvts %d \n", pp->ncvt);
  for(int i = 0; i < pp->ncvt; i++) {
   csound->Free(csound, pp->dat[i].bufferin);
   //csound->Free(csound, pp->dat[i].data);
   }
  if(pp->ncvt > 0) csound->Free(csound, pp->dat);
  csound->Free(csound, pp);
}

static inline double mod1(double x){
  double r;
  r = x - MYFLT2LRND(x) ;
  if (r < 0.0) return r + 1.0 ;
  return r;
}

static
void src_linear_process(SR_CONVERTER *pp, MYFLT *in, MYFLT *out,
                        MYFLT **data, int32_t outsamps){

  int32_t outcnt, incnt;
  MYFLT start = *((MYFLT *) data), frac;
  MYFLT ratio = pp->ratio, fac = FL(0.0);
  for(incnt = 0, outcnt = 0; outcnt < outsamps; outcnt++) {
    out[outcnt] = start + fac*(in[incnt] - start);
    fac += 1./ratio;
    frac = mod1(fac);
    incnt += MYFLT2LRND(fac - frac);
    fac = frac;
    if(incnt >= 1) start = in[incnt-1];
  }
  *((MYFLT *) data) = in[incnt-1];
}

static
int32_t src_linear_convert(CSOUND *csound, SR_CONVERTER *pp,
                           MYFLT *argin, MYFLT *argout){
  IGN(csound);
  int32_t i = pp->ncvt;
  if(i > 0) { // convert
    for(int n = 0; n < i; n++) {
      int32_t size = pp->size, cnt = pp->dat[n].cnt;
      MYFLT ratio = pp->ratio;
      MYFLT *buff = (MYFLT *)(pp->dat[n].bufferin),
        *in = argin, *out = argout;
      const CS_TYPE *typ = pp->var->varType;

      if(typ == &CS_VAR_TYPE_ARRAY) {
        ARRAYDAT *arg = (ARRAYDAT *) argin;
        in = arg->data + n*size;
        arg = (ARRAYDAT *) argout;
        out = arg->data + n*size;
      }   
      
      if(ratio > 1) {
        if(!cnt) {
          src_linear_process(pp, in, buff,
                             (MYFLT **) &(pp->dat[n].data),
                             size*ratio);
        }
        memcpy(out,buff+cnt*size, sizeof(MYFLT)*size);
        cnt = cnt < ratio - 1 ? cnt + 1 : 0;
      } else {
        memcpy(buff+cnt*size,in,sizeof(MYFLT)*size);
        cnt = cnt < 1/ratio - 1 ? cnt + 1 : 0;
        if(!cnt) src_linear_process(pp,buff, out,
                                    (MYFLT **)&(pp->dat[n].data),
                                    size);
      }
      pp->dat[n].cnt = cnt;
    }
  } else // bypass
    pp->var->varType->copyValue(pp->csound,pp->var->varType,
                                argout, argin, pp->ip); 
  return 0;
}

#ifndef USE_SRC
// fallback to linear conversion
SR_CONVERTER *src_init(CSOUND *csound, int32_t mode,
                       float ratio, CS_VARIABLE *var, INSDS *ip) {
  return src_linear_init(csound, mode, ratio, var, ip);
}
int32_t src_convert(CSOUND *csound, SR_CONVERTER *pp,
                    MYFLT *in, MYFLT *out){
  return src_linear_convert(csound, pp, in, out);
}

void src_deinit(CSOUND *csound, SR_CONVERTER *pp) {
  src_linear_deinit(csound, pp);
}

#else // Use Secret Rabbit Code
#include <samplerate.h>
typedef struct {
  SRC_STATE* stat;
  SRC_DATA cvt;
} SRC;

/*  SRC modes
    SRC_SINC_BEST_QUALITY       = 0,
    SRC_SINC_MEDIUM_QUALITY     = 1,
    SRC_SINC_FASTEST            = 2,
    SRC_ZERO_ORDER_HOLD         = 3,
    SRC_LINEAR                  = 4
    NB - linear uses the code above, avoiding extra copying
    and implementing ksig conversion correctly
    (SRC linear converter has a bug for single-sample conversion)
*/
SR_CONVERTER *src_init(CSOUND *csound, int32_t mode,
                       float ratio, CS_VARIABLE *var,
                       INSDS *ip) {
  if(mode < 4) {
    int32_t err = 0;
    int32_t n = 1, size;
    int i;
    const CS_TYPE *typ = var->varType;
    SR_CONVERTER *pp = (SR_CONVERTER *)
      csound->Calloc(csound, sizeof(SR_CONVERTER));
  
    if(typ == &CS_VAR_TYPE_ARRAY) {
      ARRAYDAT *array  = (ARRAYDAT *) (ip->lclbas + var->memBlockIndex);
      typ = var->subType;
      for(i = 0; i < array->dimensions; i++)
         n *= array->sizes[i];
    }

    if(typ->userDefinedType) { // UDTs currently unsupported
      csound->Free(csound, pp);
      csound->Message(csound, "Support for local sampling rate and "
                               "user-defined types not currently available\n");
      return NULL;
    }
    
    if(typ == &CS_VAR_TYPE_A || typ == &CS_VAR_TYPE_K) {
      // src conversion
      size = typ == &CS_VAR_TYPE_K ? 1 : ip->ksmps;
      pp->dat = (CVTDAT *) csound->Calloc(csound, sizeof(CVTDAT)*n);
      pp->size = size;
      for(i = 0; i < n; i++) { // one cvt per array item or per asig/ksig var
        SRC_STATE* stat = src_new(mode > 0 ? mode : 0, 1, &err);
        if(!err) {
          SRC *p = (SRC *) csound->Calloc(csound, sizeof(SRC));
          p->stat = stat;
          p->cvt.src_ratio = ratio;
          if (ratio > 1) {
            p->cvt.input_frames = size;
            p->cvt.output_frames = size*ratio;
          }  else {
            p->cvt.input_frames = size/ratio;
            p->cvt.output_frames = size;
          }
          pp->dat[i].bufferin = (float *)
            csound->Calloc(csound, sizeof(float)*p->cvt.input_frames);
          p->cvt.data_in = pp->dat[i].bufferin;
          pp->dat[i].bufferout = (float *)
            csound->Calloc(csound, sizeof(float)*p->cvt.output_frames);
          p->cvt.data_out = pp->dat[i].bufferout;
          p->cvt.end_of_input = 0;
          pp->dat[i].data = (void *)  p;
          pp->dat[i].cnt = 0;
        }
        else {
          // init fail - free memory
          pp->ncvt = i; // set count to last allocated
          src_deinit(csound, pp); // call deinit to free all memory
          return NULL;
        } 
      }
      pp->ncvt = n;
    } else pp->ncvt = 0;
    pp->ratio = ratio;      
    pp->mode = mode;
    pp->var = var;
    pp->csound = csound;
    pp->ip = ip;           
    return pp;
  } else
    return src_linear_init(csound, mode, ratio, var, ip);
}

/* this routine on upsampling feeds a buffer, converts, then outputs it in blocks;
   on downsampling, it feeds a buffer, when full converts and outputs
*/
int32_t src_convert(CSOUND *csound, SR_CONVERTER *pp, MYFLT *argin, MYFLT *argout){
  int32_t k = pp->ncvt;
  if(k) {
    // src conversion
    if(pp->mode < 4){
      for(int n = 0; n < k; n++) {
        int32_t i, cnt = pp->dat[n].cnt, size = pp->size;
        float ratio = pp->ratio;
        MYFLT *in = argin, *out = argout;
        SRC *p = (SRC *) pp->dat[n].data;
        const CS_TYPE *typ = pp->var->varType;
        if(typ == &CS_VAR_TYPE_ARRAY) {
          ARRAYDAT *arg = (ARRAYDAT *) argin;
          in = arg->data + n*size;
          arg = (ARRAYDAT *) argout;
          out = arg->data + n*size;
        }   
        if(ratio > 1) {
          // oversample
          if(!cnt) {
            for(i = 0; i < size; i++) {
              pp->dat[n].bufferin[i] = in[i];
            }
            src_process(p->stat, &p->cvt);
          }
          for(i = 0; i < size; i++)
            out[i] = pp->dat[n].bufferout[i+size*cnt];
          cnt = cnt < ratio - 1 ? cnt + 1 : 0;
        } else {
          // undersample
          for(i = 0; i < size; i++)
            pp->dat[n].bufferin[i+size*cnt] = in[i];
          cnt = cnt < 1/ratio - 1 ? cnt + 1 : 0;
          if(!cnt) {
            src_process(p->stat, &p->cvt);
            for(i = 0; i < size; i++)
              out[i] = pp->dat[n].bufferout[i];
          }
        }
        pp->dat[n].cnt = cnt;
      }   
    } else // bypass
      return src_linear_convert(csound, pp, argin, argout);
  } else pp->var->varType->copyValue(pp->csound, pp->var->varType,
                                     argout, argin, pp->ip);  
  return 0;
}
void src_deinit(CSOUND *csound, SR_CONVERTER *pp) {
  if(pp->mode < 4) {
    for(int i = 0; i < pp->ncvt; i++) {
     SRC *p = (SRC *) pp->dat[i].data;
     src_delete(p->stat);
     csound->Free(csound, p);
     csound->Free(csound, pp->dat[i].bufferin);
     csound->Free(csound, pp->dat[i].bufferout);
    }
    if(pp->ncvt > 0) csound->Free(csound, pp->dat);
    csound->Free(csound, pp);
  }
  else src_linear_deinit(csound, pp);
}
#endif  // ifndef USE_SRC


