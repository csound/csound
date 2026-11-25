#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "wavelets.h"

#define SWT_PI 3.14159265358979323846
#define SWT_SQRT2 1.41421356237309504880

static int32_t swt_int_arg(cs_float value) {
  double v = (double) value;
  if (v != v || v < -2147483647.0 || v > 2147483647.0) return 0;
  return (int32_t) v;
}

static int32_t swt_mod(int32_t n, int32_t N) {
  n %= N;
  if (n < 0) n += N;
  return n;
}

static int64_t swt_wrap64(int64_t n, int64_t N) {
  n %= N;
  if (n < 0) n += N;
  return n;
}

void swt_free_aux(CSOUND *csound, SWT_AUXCH *aux) {
  if (aux == NULL) return;
  if (aux->auxp != NULL) csound->Free(csound, aux->auxp);
  aux->nxtchp = NULL;
  aux->size = 0;
  aux->auxp = NULL;
  aux->endp = NULL;
}

int32_t swt_alloc_aux(CSOUND *csound, size_t nbytes, SWT_AUXCH *aux) {
  if (aux == NULL || nbytes == 0) return NOTOK;
  if (aux->auxp != NULL) {
    if (aux->size == nbytes) {
      memset(aux->auxp, 0, nbytes);
      return OK;
    }
    swt_free_aux(csound, aux);
  }
  aux->auxp = csound->Calloc(csound, nbytes);
  if (aux->auxp == NULL) {
    aux->nxtchp = NULL;
    aux->size = 0;
    aux->endp = NULL;
    return NOTOK;
  }
  aux->nxtchp = NULL;
  aux->size = nbytes;
  aux->endp = (char *) aux->auxp + nbytes;
  return OK;
}

void swt_free_var_mem(void *csnd, void *varMem) {
  CSOUND *csound = (CSOUND *) csnd;
  SWTDAT *frame = (SWTDAT *) varMem;
  if (csound == NULL || frame == NULL) return;
  swt_free_aux(csound, &frame->frame);
  swt_free_aux(csound, &frame->history);
}

static int64_t swt_step(int32_t level) {
  int64_t step = 1;
  int32_t i;
  for (i = 1; i < level; i++) {
    if (step > INT64_MAX / 2) return 0;
    step *= 2;
  }
  return step;
}

static int32_t swt_max_levels(int32_t N, int32_t L) {
  int64_t step = 1;
  int32_t levels = 0;
  while (step > 0 && step <= N && step * L <= N && step < N) {
    levels++;
    if (step > INT64_MAX / 2) break;
    step *= 2;
  }
  return levels;
}

static void swt_make_high(const float *low, int32_t length, float *high) {
  int32_t k;
  for (k = 0; k < length; k++) {
    high[k] = ((k & 1) ? -1.0f : 1.0f) * low[length - 1 - k];
  }
}

static void swt_make_windows(float *windows, int32_t N, int32_t mode) {
  int32_t n;
  if (mode == SWT_WINDOW_SQRT_HANN) {
    for (n = 0; n < N; n++) {
      double w = 0.5 - 0.5 * cos(2.0 * SWT_PI * ((double) n + 0.5) /
                                 (double) N);
      windows[n] = (float) sqrt(w);
    }
  } else {
    for (n = 0; n < N; n++) windows[n] = 1.0f;
  }
}

int32_t swt_band_offset(const SWTDAT *frame, int32_t band) {
  if (frame == NULL) return -1;
  if (band < 0) return 0;
  if (band > frame->levels) return -1;
  return band * frame->framesize;
}

int32_t swt_band_size(const SWTDAT *frame, int32_t band) {
  if (frame == NULL) return 0;
  if (band < 0) return frame->capacity;
  if (band > frame->levels) return 0;
  return frame->framesize;
}

PUBLIC int32_t csoundSwtDataFrameSize(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->framesize : 0;
}

PUBLIC int32_t csoundSwtDataHopSize(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->hopsize : 0;
}

PUBLIC int32_t csoundSwtDataLevels(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->levels : 0;
}

PUBLIC int32_t csoundSwtDataWaveletFamily(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->waveletFamily : 0;
}

PUBLIC int32_t csoundSwtDataWaveletOrder(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->waveletOrder : 0;
}

PUBLIC int32_t csoundSwtDataBoundaryMode(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->boundaryMode : 0;
}

PUBLIC int32_t csoundSwtDataWindowPair(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->windowPair : 0;
}

PUBLIC int32_t csoundSwtDataOlaMode(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->olaMode : 0;
}

PUBLIC int32_t csoundSwtDataLatency(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->latency : 0;
}

PUBLIC int32_t csoundSwtDataCapacity(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->capacity : 0;
}

PUBLIC double csoundSwtDataSampleRate(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->sampleRate : 0.0;
}

PUBLIC uint64_t csoundSwtDataFramecount(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->framecount : 0;
}

PUBLIC uint64_t csoundSwtDataRevision(const SWTDAT *swtdat) {
  return swtdat != NULL ? swtdat->revision : 0;
}

PUBLIC int32_t csoundSwtDataBandOffset(const SWTDAT *swtdat, int32_t band) {
  return swt_band_offset(swtdat, band);
}

PUBLIC int32_t csoundSwtDataBandSize(const SWTDAT *swtdat, int32_t band) {
  return swt_band_size(swtdat, band);
}

PUBLIC const float *csoundGetSwtData(const SWTDAT *swtdat) {
  return swtdat != NULL && swtdat->frame.auxp != NULL
             ? (const float *) swtdat->frame.auxp : NULL;
}

int32_t swt_forward(const float *low, const float *high, int32_t length,
                    int32_t N, int32_t J, float *work, float *tmp,
                    float *frame) {
  const float scale = (float) (1.0 / SWT_SQRT2);
  int32_t j, n, k;

  if (low == NULL || high == NULL || work == NULL || tmp == NULL ||
      frame == NULL || length <= 0 || N <= 0 || J <= 0) {
    return NOTOK;
  }
  for (j = 1; j <= J; j++) {
    int64_t step64 = swt_step(j);
    int32_t step, band;
    float *detail;
    if (step64 <= 0 || step64 > INT32_MAX) return NOTOK;
    step = (int32_t) step64;
    if ((int64_t) step * length > N) return NOTOK;
    band = J - j + 1;
    detail = frame + band * N;
    for (n = 0; n < N; n++) {
      float a = 0.0f;
      float d = 0.0f;
      for (k = 0; k < length; k++) {
        float v = work[swt_mod(n - k * step, N)];
        a += low[k] * v;
        d += high[k] * v;
      }
      tmp[n] = scale * a;
      detail[n] = scale * d;
    }
    memcpy(work, tmp, (size_t) N * sizeof(float));
  }
  memcpy(frame, work, (size_t) N * sizeof(float));
  return OK;
}

int32_t swt_inverse(const float *low, const float *high, int32_t length,
                    int32_t N, int32_t J, const float *frame, float *work,
                    float *tmp) {
  const float scale = (float) (1.0 / SWT_SQRT2);
  int32_t j, n, k;

  if (low == NULL || high == NULL || frame == NULL || work == NULL ||
      tmp == NULL || length <= 0 || N <= 0 || J <= 0) {
    return NOTOK;
  }
  memcpy(work, frame, (size_t) N * sizeof(float));
  for (j = J; j >= 1; j--) {
    int64_t step64 = swt_step(j);
    int32_t step, band;
    const float *detail;
    if (step64 <= 0 || step64 > INT32_MAX) return NOTOK;
    step = (int32_t) step64;
    if ((int64_t) step * length > N) return NOTOK;
    band = J - j + 1;
    detail = frame + band * N;
    for (n = 0; n < N; n++) {
      float a = 0.0f;
      float d = 0.0f;
      for (k = 0; k < length; k++) {
        int32_t idx = swt_mod(n + k * step, N);
        a += low[k] * work[idx];
        d += high[k] * detail[idx];
      }
      tmp[n] = scale * (a + d);
    }
    memcpy(work, tmp, (size_t) N * sizeof(float));
  }
  return OK;
}

static int32_t swt_set_metadata(CSOUND *csound, SWTDAT *frame, int32_t N,
                                int32_t H, int32_t J, int32_t family,
                                int32_t order, int32_t window, int32_t ola,
                                int32_t historyCount) {
  if (frame == NULL || N < 2 || H < 1 || H > N || J < 1 || historyCount < 1) {
    return NOTOK;
  }
  if ((int64_t) (J + 1) * N > INT32_MAX ||
      (int64_t) historyCount * (J + 1) * N > INT32_MAX / 4) {
    return NOTOK;
  }
  frame->framesize = N;
  frame->hopsize = H;
  frame->levels = J;
  frame->waveletFamily = family;
  frame->waveletOrder = order;
  frame->boundaryMode = SWT_BOUNDARY_PERIODIC;
  frame->windowPair = window;
  frame->olaMode = ola;
  frame->latency = N;
  frame->capacity = (J + 1) * N;
  frame->historyCount = historyCount;
  frame->framecount = 0;
  frame->revision = 0;
  frame->sampleRate = (double) csound->esr;
  return OK;
}

void swt_copy_frame(CSOUND *csound, SWTDAT *dest, const SWTDAT *src) {
  size_t bytes;
  size_t historyBytes;
  if (dest == NULL || src == NULL || dest == src) return;
  bytes = (size_t) src->capacity * sizeof(float);
  historyBytes = bytes * (size_t) src->historyCount;
  memcpy(dest, src, offsetof(SWTDAT, frame));
  if (bytes > 0 && (dest->frame.auxp == NULL || dest->frame.size < bytes)) {
    swt_alloc_aux(csound, bytes, &dest->frame);
  }
  if (historyBytes > 0 &&
      (dest->history.auxp == NULL || dest->history.size < historyBytes)) {
    swt_alloc_aux(csound, historyBytes, &dest->history);
  }
  if (bytes > 0 && src->frame.auxp != NULL && dest->frame.auxp != NULL) {
    memcpy(dest->frame.auxp, src->frame.auxp, bytes);
  }
  if (historyBytes > 0 && src->history.auxp != NULL &&
      dest->history.auxp != NULL) {
    memcpy(dest->history.auxp, src->history.auxp, historyBytes);
  }
}

int32_t swtanalset(CSOUND *csound, SWTANAL *p) {
  int32_t L = 0, N, H, J, family, order, boundary, window, ola;
  int32_t maxLevels, historyCount, ksmps;
  int64_t frames, capacity;
  const float *low;

  if (p == NULL || p->out == NULL) return NOTOK;
  N = swt_int_arg(*p->framesize);
  H = swt_int_arg(*p->hopsize);
  family = swt_int_arg(*p->family);
  order = swt_int_arg(*p->order);
  J = p->levels != NULL ? swt_int_arg(*p->levels) : -1;
  boundary = p->boundary != NULL ? swt_int_arg(*p->boundary) : 0;
  window = p->window != NULL ? swt_int_arg(*p->window) : 1;
  ola = p->ola != NULL ? swt_int_arg(*p->ola) : 0;
  if (N < 2 || H < 1 || H > N) {
    csound->InitError(csound, Str("swtanal: invalid frame size or hop size\n"));
    return NOTOK;
  }
  if (boundary != SWT_BOUNDARY_PERIODIC) {
    csound->InitError(csound, Str("swtanal: only periodic boundary is supported\n"));
    return NOTOK;
  }
  if (window != SWT_WINDOW_NONE && window != SWT_WINDOW_SQRT_HANN) {
    csound->InitError(csound, Str("swtanal: invalid window\n"));
    return NOTOK;
  }
  if (ola != SWT_OLA_AUTO && ola != SWT_OLA_DIRECT &&
      ola != SWT_OLA_OVERLAP_ADD) {
    csound->InitError(csound, Str("swtanal: invalid overlap-add mode\n"));
    return NOTOK;
  }
  low = swt_get_coefficients(family, order, &L);
  if (low == NULL || L < 1 || L > N) {
    csound->InitError(csound, Str("swtanal: invalid wavelet family or order\n"));
    return NOTOK;
  }
  maxLevels = swt_max_levels(N, L);
  if (maxLevels < 1) {
    csound->InitError(csound, Str("swtanal: frame size is too small for wavelet\n"));
    return NOTOK;
  }
  if (J < 0 || J > maxLevels) J = maxLevels;
  if (J == 0) J = 1;
  capacity = (int64_t) (J + 1) * N;
  if (capacity > INT32_MAX) {
    csound->InitError(csound, Str("swtanal: frame capacity is too large\n"));
    return NOTOK;
  }
  if (window == SWT_WINDOW_NONE && H < N) {
    csound->InitError(csound, Str("swtanal: H < N requires a window\n"));
    return NOTOK;
  }
  if (ola == SWT_OLA_AUTO) ola = H == N ? SWT_OLA_DIRECT : SWT_OLA_OVERLAP_ADD;
  if (ola == SWT_OLA_DIRECT && H != N) {
    csound->InitError(csound, Str("swtanal: direct mode requires H == N\n"));
    return NOTOK;
  }
  if (ola == SWT_OLA_OVERLAP_ADD && window == SWT_WINDOW_NONE) {
    csound->InitError(csound, Str("swtanal: overlap-add requires a window\n"));
    return NOTOK;
  }
  if (ola == SWT_OLA_OVERLAP_ADD && H == N) {
    csound->InitError(csound, Str("swtanal: overlap-add requires H < N\n"));
    return NOTOK;
  }
  if (ola == SWT_OLA_DIRECT) window = SWT_WINDOW_NONE;
  ksmps = CS_KSMPS;
  frames = ((int64_t) ksmps + H - 1) / H +
           ((int64_t) N + H - 1) / H + 2;
  if (frames < 1 || frames > INT32_MAX) {
    csound->InitError(csound, Str("swtanal: frame history is too large\n"));
    return NOTOK;
  }
  historyCount = (int32_t) frames;
  if ((size_t) capacity > SIZE_MAX / (size_t) historyCount / sizeof(float)) {
    csound->InitError(csound, Str("swtanal: frame history is too large\n"));
    return NOTOK;
  }
  if (swt_set_metadata(csound, p->out, N, H, J, family, order, window, ola,
                       historyCount) != OK) {
    csound->InitError(csound, Str("swtanal: invalid SWT metadata\n"));
    return NOTOK;
  }
  p->N = N;
  p->H = H;
  p->J = J;
  p->filterLength = L;
  p->inputCount = 0;
  p->low = low;
  swt_alloc_aux(csound, (size_t) p->out->capacity * sizeof(float),
                &p->out->frame);
  swt_alloc_aux(csound, (size_t) p->out->capacity *
                (size_t) historyCount * sizeof(float),
                &p->out->history);
  csound->AuxAlloc(csound, (size_t) N * sizeof(float), &p->input);
  csound->AuxAlloc(csound, (size_t) N * sizeof(float), &p->scratch);
  csound->AuxAlloc(csound, (size_t) N * sizeof(float), &p->scratch2);
  csound->AuxAlloc(csound, (size_t) N * sizeof(float), &p->windows);
  csound->AuxAlloc(csound, (size_t) L * sizeof(float), &p->highBuf);
  if (p->out->frame.auxp == NULL || p->out->history.auxp == NULL ||
      p->input.auxp == NULL || p->scratch.auxp == NULL ||
      p->scratch2.auxp == NULL || p->windows.auxp == NULL ||
      p->highBuf.auxp == NULL) {
    csound->InitError(csound, Str("swtanal: memory allocation failed\n"));
    return NOTOK;
  }
  swt_make_windows((float *) p->windows.auxp, N, window);
  swt_make_high(low, L, (float *) p->highBuf.auxp);
  p->high = (const float *) p->highBuf.auxp;
  memset(p->input.auxp, 0, (size_t) N * sizeof(float));
  memset(p->out->frame.auxp, 0, (size_t) p->out->capacity * sizeof(float));
  memset(p->out->history.auxp, 0,
         (size_t) p->out->capacity * (size_t) historyCount * sizeof(float));
  return OK;
}

int32_t swtanal(CSOUND *csound, SWTANAL *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS - early;
  uint32_t i;
  float *input = (float *) p->input.auxp;
  float *work = (float *) p->scratch.auxp;
  float *tmp = (float *) p->scratch2.auxp;
  float *windows = (float *) p->windows.auxp;
  float *frame = (float *) p->out->frame.auxp;
  float *history = (float *) p->out->history.auxp;
  int32_t writeIndex = p->inputCount % p->N;
  int32_t n;

  if (input == NULL || work == NULL || tmp == NULL || windows == NULL ||
      frame == NULL || history == NULL) {
    return NOTOK;
  }
  for (i = offset; i < nsmps; i++) {
    input[writeIndex] = (float) p->ain[i];
    writeIndex = (writeIndex + 1) % p->N;
    p->inputCount++;
    if (p->inputCount >= p->N && (p->inputCount - p->N) % p->H == 0) {
      int32_t slot;
      for (n = 0; n < p->N; n++) {
        int32_t idx = (writeIndex + n) % p->N;
        work[n] = input[idx] * windows[n];
      }
      if (swt_forward(p->low, p->high, p->filterLength, p->N, p->J,
                      work, tmp, frame) != OK) {
        return NOTOK;
      }
      slot = (int32_t) (p->out->framecount % (uint64_t) p->out->historyCount);
      memcpy(history + (size_t) slot * (size_t) p->out->capacity, frame,
             (size_t) p->out->capacity * sizeof(float));
      p->out->framecount++;
      p->out->revision++;
    }
  }
  return OK;
}

int32_t swtgetset(CSOUND *csound, SWTGET *p) {
  int32_t band = -1;
  int32_t size;
  if (p == NULL || p->in == NULL) return NOTOK;
  if (p->band != NULL) band = swt_int_arg(*p->band);
  size = swt_band_size(p->in, band);
  if (size <= 0) {
    csound->InitError(csound, Str("swtget: invalid band selector\n"));
    return NOTOK;
  }
  p->bandValue = band;
  p->count = size;
  if (tabinit(csound, p->out, size, p->h.insdshead) != OK) {
    csound->InitError(csound, Str("swtget: array allocation failed\n"));
    return NOTOK;
  }
  return OK;
}

int32_t swtget(CSOUND *csound, SWTGET *p) {
  int32_t offset = swt_band_offset(p->in, p->bandValue);
  int32_t i;
  const float *data;
  if (offset < 0 || p->in->frame.auxp == NULL) return NOTOK;
  data = (const float *) p->in->frame.auxp + offset;
  if (p->out->data == NULL) return NOTOK;
  for (i = 0; i < p->count; i++) p->out->data[i] = (cs_float) data[i];
  return OK;
}

static int32_t swt_band_array_length(const ARRAYDAT *array, size_t *length) {
  if (array == NULL || array->data == NULL ||
      array->arrayMemberSize < (int32_t) sizeof(cs_float)) {
    return NOTOK;
  }
  return csound_array_member_count(array, length);
}

static void swt_apply_band(float *frame, int32_t offset, int32_t size,
                           const cs_float *values) {
  int32_t i;
  for (i = 0; i < size; i++) frame[offset + i] = (float) values[i];
}

int32_t swtsetset(CSOUND *csound, SWTSET *p) {
  int32_t band = -1;
  size_t available;
  if (p == NULL || p->in == NULL || p->out == NULL) return NOTOK;
  if (p->band != NULL) band = swt_int_arg(*p->band);
  if (swt_band_size(p->in, band) <= 0) {
    csound->InitError(csound, Str("swtset: invalid band selector\n"));
    return NOTOK;
  }
  p->bandValue = band;
  p->count = swt_band_size(p->in, band);
  if (swt_band_array_length(p->bands, &available) != OK ||
      available < (size_t) p->count) {
    csound->InitError(csound, Str("swtset: input array is too small\n"));
    return NOTOK;
  }
  swt_copy_frame(csound, p->out, p->in);
  p->lastFrame = p->in->framecount;
  return OK;
}

int32_t swtset(CSOUND *csound, SWTSET *p) {
  int32_t band = p->bandValue;
  int32_t offset = swt_band_offset(p->out, band);
  int32_t size = swt_band_size(p->out, band);
  float *data = (float *) p->out->frame.auxp;
  size_t bytes = (size_t) p->out->capacity * sizeof(float);
  size_t available;
  if (offset < 0 || size <= 0 || data == NULL ||
      swt_band_array_length(p->bands, &available) != OK ||
      available < (size_t) size) {
    return csound->PerfError(csound, &p->h,
                             Str("swtset: input array is too short\n"));
  }
  if (p->in->framecount > p->lastFrame &&
      p->in->capacity == p->out->capacity &&
      p->in->frame.auxp != NULL && p->in->history.auxp != NULL &&
      p->out->history.auxp != NULL &&
      p->in->historyCount > 0 && p->out->historyCount > 0) {
    while (p->lastFrame < p->in->framecount) {
      size_t outSlot = (size_t) (p->lastFrame %
                                (uint64_t) p->out->historyCount);
      size_t inSlot = (size_t) (p->lastFrame %
                               (uint64_t) p->in->historyCount);
      float *edited = (float *) p->out->history.auxp +
                      outSlot * (size_t) p->out->capacity;
      memcpy(edited,
             (const float *) p->in->history.auxp +
             inSlot * (size_t) p->in->capacity, bytes);
      swt_apply_band(edited, offset, size, p->bands->data);
      p->lastFrame++;
    }
    memcpy(data, (const float *) p->in->frame.auxp, bytes);
    p->out->framecount = p->lastFrame;
  }
  swt_apply_band(data, offset, size, p->bands->data);
  if (p->out->history.auxp != NULL && p->out->historyCount > 0 &&
      p->out->framecount > 0) {
    size_t slot = (size_t) ((p->out->framecount - 1) %
                            (uint64_t) p->out->historyCount);
    memcpy((float *) p->out->history.auxp + slot * (size_t) p->out->capacity,
           data, bytes);
  }
  p->out->revision++;
  return OK;
}

static double swt_ola_denominator(const float *windows, int32_t N, int32_t H,
                                  int64_t q, const float *denom) {
  int64_t k;
  double sum = 0.0;
  if (q >= (int64_t) N - 1) {
    return denom[(int32_t) (q % (int64_t) H)];
  }
  for (k = q / (int64_t) H; k >= 0 && q - k * (int64_t) H < (int64_t) N;
       k--) {
    int32_t n = (int32_t) (q - k * (int64_t) H);
    sum += (double) windows[n] * windows[n];
  }
  return sum > 1e-12 ? sum : 1.0;
}

static int32_t swt_synth_add_frame(CSOUND *csound, SWTSYNTH *p,
                                   const float *source) {
  float *recon = (float *) p->recon.auxp;
  float *work = (float *) p->scratch.auxp;
  float *tmp = (float *) p->scratch2.auxp;
  float *windows = (float *) p->windows.auxp;
  float *denom = (float *) p->denom.auxp;
  float *outbuf = (float *) p->outbuf.auxp;
  int64_t start = (int64_t) p->nextFrame * (int64_t) p->H +
                  (int64_t) p->latency;
  int32_t n;

  if (recon == NULL || work == NULL || tmp == NULL || windows == NULL ||
      denom == NULL || outbuf == NULL) {
    return NOTOK;
  }
  if (swt_inverse(p->low, p->high, p->filterLength, p->N, p->J,
                  source, work, tmp) != OK) {
    return NOTOK;
  }
  memcpy(recon, work, (size_t) p->N * sizeof(float));
  for (n = 0; n < p->N; n++) {
    int64_t pos = start + n;
    int64_t idx = swt_wrap64(pos, (int64_t) p->buflen);
    float value = recon[n];
    if (p->olaMode == SWT_OLA_OVERLAP_ADD) {
      int64_t q = (int64_t) (p->nextFrame - p->firstFrame) * (int64_t) p->H +
                  (int64_t) n;
      double weightSum = swt_ola_denominator(windows, p->N, p->H, q, denom);
      value *= windows[n] / (float) weightSum;
    }
    outbuf[idx] += value;
  }
  return OK;
}

int32_t swtsynthset(CSOUND *csound, SWTSYNTH *p) {
  int32_t N, H, J, L, historyCount, ksmps, capacity;
  int64_t buflen;
  float *windows, *denom;
  int32_t i;

  if (p == NULL || p->in == NULL) return NOTOK;
  N = p->in->framesize;
  H = p->in->hopsize;
  J = p->in->levels;
  historyCount = p->in->historyCount;
  capacity = p->in->capacity;
  if (N < 2 || H < 1 || H > N || J < 1 || historyCount < 0 ||
      capacity != (J + 1) * N || p->in->frame.auxp == NULL) {
    csound->InitError(csound, Str("swtsynth: uninitialised SWT frame\n"));
    return NOTOK;
  }
  p->low = swt_get_coefficients(p->in->waveletFamily,
                                p->in->waveletOrder, &L);
  if (p->low == NULL || L < 1 || L > N) {
    csound->InitError(csound, Str("swtsynth: invalid wavelet metadata\n"));
    return NOTOK;
  }
  p->N = N;
  p->H = H;
  p->J = J;
  p->latency = p->in->latency > 0 ? p->in->latency : N;
  p->capacity = capacity;
  p->filterLength = L;
  if (p->in->olaMode < SWT_OLA_AUTO ||
      p->in->olaMode > SWT_OLA_OVERLAP_ADD) {
    csound->InitError(csound, Str("swtsynth: invalid overlap-add mode\n"));
    return NOTOK;
  }
  p->olaMode = p->in->olaMode == SWT_OLA_DIRECT ?
               SWT_OLA_DIRECT : SWT_OLA_OVERLAP_ADD;
  if (p->in->framecount > (uint64_t) (INT64_MAX / H)) {
    csound->InitError(csound, Str("swtsynth: frame count is too large\n"));
    return NOTOK;
  }
  p->firstFrame = p->in->framecount;
  p->nextFrame = p->firstFrame;
  p->outpos = (int64_t) (p->nextFrame * (uint64_t) H);
  ksmps = CS_KSMPS;
  buflen = (int64_t) 2 * N + H + 2 * (int64_t) ksmps + 8;
  if (buflen > INT32_MAX) {
    csound->InitError(csound, Str("swtsynth: buffer is too large\n"));
    return NOTOK;
  }
  p->buflen = (int32_t) buflen;
  csound->AuxAlloc(csound, (size_t) N * sizeof(float), &p->recon);
  csound->AuxAlloc(csound, (size_t) N * sizeof(float), &p->scratch);
  csound->AuxAlloc(csound, (size_t) N * sizeof(float), &p->scratch2);
  csound->AuxAlloc(csound, (size_t) N * sizeof(float), &p->windows);
  csound->AuxAlloc(csound, (size_t) H * sizeof(float), &p->denom);
  csound->AuxAlloc(csound, (size_t) L * sizeof(float), &p->highBuf);
  csound->AuxAlloc(csound, (size_t) p->buflen * sizeof(float), &p->outbuf);
  if (p->recon.auxp == NULL || p->scratch.auxp == NULL ||
      p->scratch2.auxp == NULL || p->windows.auxp == NULL ||
      p->denom.auxp == NULL || p->highBuf.auxp == NULL ||
      p->outbuf.auxp == NULL) {
    csound->InitError(csound, Str("swtsynth: memory allocation failed\n"));
    return NOTOK;
  }
  windows = (float *) p->windows.auxp;
  denom = (float *) p->denom.auxp;
  swt_make_windows(windows, N, p->olaMode == SWT_OLA_OVERLAP_ADD ?
                   SWT_WINDOW_SQRT_HANN : SWT_WINDOW_NONE);
  swt_make_high(p->low, L, (float *) p->highBuf.auxp);
  p->high = (const float *) p->highBuf.auxp;
  for (i = 0; i < H; i++) {
    double sum = 0.0;
    int32_t n;
    if (p->olaMode == SWT_OLA_DIRECT) {
      denom[i] = 1.0f;
      continue;
    }
    for (n = i; n < N; n += H) sum += (double) windows[n] * windows[n];
    denom[i] = (float) (sum > 1e-12 ? sum : 1.0);
  }
  memset(p->outbuf.auxp, 0, (size_t) p->buflen * sizeof(float));
  return OK;
}

int32_t swtsynth(CSOUND *csound, SWTSYNTH *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS - early;
  uint32_t i;
  float *outbuf = (float *) p->outbuf.auxp;
  float *history = (float *) p->in->history.auxp;
  float *latest = (float *) p->in->frame.auxp;

  if (outbuf == NULL || latest == NULL) return NOTOK;
  if (p->in->framesize != p->N || p->in->hopsize != p->H ||
      p->in->levels != p->J) {
    return NOTOK;
  }
  for (i = 0; i < offset && i < (uint32_t) CS_KSMPS; i++) p->aout[i] = 0;
  for (i = nsmps; i < (uint32_t) CS_KSMPS; i++) p->aout[i] = 0;
  for (i = offset; i < nsmps; i++) {
    while (p->nextFrame < p->in->framecount) {
      uint64_t m = p->nextFrame;
      const float *source;
      if (history != NULL && p->in->historyCount > 0) {
        size_t slot = (size_t) (m % (uint64_t) p->in->historyCount);
        source = history + slot * (size_t) p->capacity;
      } else {
        source = latest;
      }
      if (swt_synth_add_frame(csound, p, source) != OK) break;
      p->nextFrame++;
    }
    {
      int64_t idx = swt_wrap64(p->outpos, (int64_t) p->buflen);
      p->aout[i] = (cs_float) outbuf[idx];
      outbuf[idx] = 0.0f;
    }
    p->outpos++;
  }
  return OK;
}
