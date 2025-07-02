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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "srconvert.h"
#include "csoundCore.h"

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

static SR_CONVERTER *src_linear_init(CSOUND *csound, int32_t mode, float ratio, int32_t size) {
  IGN(mode);
  SR_CONVERTER *pp = (SR_CONVERTER *) csound->Calloc(csound, sizeof(SR_CONVERTER));
  pp->data = csound->Calloc(csound, sizeof(MYFLT));
  pp->bufferin = csound->Calloc(csound, size*sizeof(MYFLT)*(ratio > 1 ? ratio : 1./ratio));
  pp->ratio = ratio;
  pp->size = size;
  pp->mode = 4;
  return pp;
}

static void src_linear_deinit(CSOUND *csound, SR_CONVERTER *pp) {
  csound->Free(csound, pp->bufferin);
  csound->Free(csound, pp->data);
  csound->Free(csound, pp);
}

static inline double mod1(double x){
  double r;
  r = x - MYFLT2LRND(x) ;
  if (r < 0.0) return r + 1.0 ;
  return r;
}

static
void src_linear_process(SR_CONVERTER *pp, MYFLT *in, MYFLT *out, int32_t outsamps){
  int32_t outcnt, incnt;
  MYFLT start = *((MYFLT *) pp->data), frac;
  MYFLT ratio = pp->ratio, fac = FL(0.0);
  for(incnt = 0, outcnt = 0; outcnt < outsamps; outcnt++) {
    out[outcnt] = start + fac*(in[incnt] - start);
    fac += 1./ratio;
    frac = mod1(fac);
    incnt += MYFLT2LRND(fac - frac);
    fac = frac;
    if(incnt >= 1) start = in[incnt-1];
  }
  *((MYFLT *) pp->data) = in[incnt-1];
}

static
int32_t src_linear_convert(CSOUND *csound, SR_CONVERTER *pp, MYFLT *in, MYFLT *out){
  IGN(csound);
  int32_t size = pp->size, cnt = pp->cnt;
  MYFLT ratio = pp->ratio;
  MYFLT *buff = (MYFLT *)(pp->bufferin);
  if(ratio > 1) {
    if(!cnt) {
      src_linear_process(pp, in, buff, size*ratio);
    }
    memcpy(out,buff+cnt*size, sizeof(MYFLT)*size);
    cnt = cnt < ratio - 1 ? cnt + 1 : 0;
  } else {
    memcpy(buff+cnt*size,in,sizeof(MYFLT)*size);
    cnt = cnt < 1/ratio - 1 ? cnt + 1 : 0;
    if(!cnt) src_linear_process(pp,buff, out, size);
  }
  pp->cnt = cnt;
  return 0;
}

#ifndef USE_SRC
// fallback to linear conversion
SR_CONVERTER *src_init(CSOUND *csound, int32_t mode,
                       float ratio, int32_t size) {
  return src_linear_init(csound, mode, ratio, size);
}
int32_t src_convert(CSOUND *csound, SR_CONVERTER *pp, MYFLT *in, MYFLT *out){
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
                       float ratio, int32_t size) {
  if(mode < 4) {
    int32_t err = 0;
    SRC_STATE* stat = src_new(mode > 0 ? mode : 0, 1, &err);
    if(!err) {
      SR_CONVERTER *pp = (SR_CONVERTER *)
        csound->Calloc(csound, sizeof(SR_CONVERTER));
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
      pp->bufferin = (float *)
        csound->Calloc(csound, sizeof(float)*p->cvt.input_frames);
      p->cvt.data_in = pp->bufferin;
      pp->bufferout = (float *)
        csound->Calloc(csound, sizeof(float)*p->cvt.output_frames);
      p->cvt.data_out = pp->bufferout;
      p->cvt.end_of_input = 0;
      pp->data = (void *)  p;
      pp->size = size;
      pp->ratio = ratio;
      pp->cnt = 0;
      pp->mode = mode;
      return pp;
    }
    else return NULL;
  } else
    return src_linear_init(csound, mode, ratio, size);
}

/* this routine on upsampling feeds a buffer, converts, then outputs it in blocks;
   on downsampling, it feeds a buffer, when full converts and outputs
*/
int32_t src_convert(CSOUND *csound, SR_CONVERTER *pp, MYFLT *in, MYFLT *out){
  if(pp->mode < 4){
    int32_t i, cnt = pp->cnt, size = pp->size;
    float ratio = pp->ratio;
    SRC *p = (SRC *) pp->data;
    if(ratio > 1) {
      // upsampling (udo input)
      if(!cnt) {
        for(i = 0; i < size; i++)
          pp->bufferin[i] = in[i];
        src_process(p->stat, &p->cvt);
      }
      for(i = 0; i < size; i++)
        out[i] = pp->bufferout[i+size*cnt];
      cnt = cnt < ratio - 1 ? cnt + 1 : 0;
    } else {
      // downsampling (udo output)
      for(i = 0; i < size; i++)
        pp->bufferin[i+size*cnt] = in[i];
      cnt = cnt < 1/ratio - 1 ? cnt + 1 : 0;
      if(!cnt) {
        src_process(p->stat, &p->cvt);
        for(i = 0; i < size; i++)
          out[i] = pp->bufferout[i];
      }
    }
    pp->cnt = cnt;
    return 0;
  } else
    return src_linear_convert(csound, pp, in, out);
  return 0;
}
void src_deinit(CSOUND *csound, SR_CONVERTER *pp) {
  if(pp->mode < 4) {
    SRC *p = (SRC *) pp->data;
    src_delete(p->stat);
    csound->Free(csound, p);
    csound->Free(csound, pp->bufferin);
    csound->Free(csound, pp->bufferout);
    csound->Free(csound, pp);
  }
  else src_linear_deinit(csound, pp);
}
#endif  // ifndef USE_SRC


