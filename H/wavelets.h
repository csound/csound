#ifndef CSOUND_WAVELETS_H
#define CSOUND_WAVELETS_H

#include "csoundCore.h"
#include "arrays.h"
#include "swtdat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  OPDS h;
  SWTDAT *out;
  cs_float *ain;
  cs_float *framesize;
  cs_float *hopsize;
  cs_float *family;
  cs_float *order;
  cs_float *levels;
  cs_float *boundary;
  cs_float *window;
  cs_float *ola;
  int32_t N;
  int32_t H;
  int32_t J;
  int32_t filterLength;
  int64_t inputCount;
  const float *low;
  const float *high;
  AUXCH input;
  AUXCH scratch;
  AUXCH scratch2;
  AUXCH windows;
  AUXCH highBuf;
} SWTANAL;

typedef struct {
  OPDS h;
  ARRAYDAT *out;
  SWTDAT *in;
  cs_float *band;
  int32_t count;
  int32_t bandValue;
} SWTGET;

typedef struct {
  OPDS h;
  SWTDAT *out;
  SWTDAT *in;
  ARRAYDAT *bands;
  cs_float *band;
  int32_t count;
  int32_t bandValue;
  uint64_t lastFrame;
} SWTSET;

typedef struct {
  OPDS h;
  cs_float *aout;
  SWTDAT *in;
  int32_t N;
  int32_t H;
  int32_t J;
  int32_t latency;
  int32_t capacity;
  int32_t filterLength;
  int32_t olaMode;
  int32_t buflen;
  uint64_t nextFrame;
  int64_t outpos;
  const float *low;
  const float *high;
  AUXCH recon;
  AUXCH scratch;
  AUXCH scratch2;
  AUXCH windows;
  AUXCH highBuf;
  AUXCH denom;
  AUXCH outbuf;
} SWTSYNTH;

int32_t swt_alloc_aux(CSOUND *csound, size_t nbytes, SWT_AUXCH *aux);
void swt_free_aux(CSOUND *csound, SWT_AUXCH *aux);
void swt_free_var_mem(void *csound, void *varMem);
void swt_copy_frame(CSOUND *csound, SWTDAT *dest, const SWTDAT *src);

const float *swt_get_coefficients(int32_t family, int32_t order,
                                  int32_t *length);
int32_t swt_band_offset(const SWTDAT *frame, int32_t band);
int32_t swt_band_size(const SWTDAT *frame, int32_t band);
int32_t swt_forward(const float *low, const float *high, int32_t length,
                    int32_t N, int32_t J, float *work, float *tmp,
                    float *frame);
int32_t swt_inverse(const float *low, const float *high, int32_t length,
                    int32_t N, int32_t J, const float *frame, float *work,
                    float *tmp);

int32_t swtanalset(CSOUND *csound, SWTANAL *p);
int32_t swtanal(CSOUND *csound, SWTANAL *p);
int32_t swtgetset(CSOUND *csound, SWTGET *p);
int32_t swtget(CSOUND *csound, SWTGET *p);
int32_t swtsetset(CSOUND *csound, SWTSET *p);
int32_t swtset(CSOUND *csound, SWTSET *p);
int32_t swtsynthset(CSOUND *csound, SWTSYNTH *p);
int32_t swtsynth(CSOUND *csound, SWTSYNTH *p);

#ifdef __cplusplus
}
#endif

#endif
