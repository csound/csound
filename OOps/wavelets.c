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

static uint32_t swtRunCounter = 0;

static int32_t swt_layout_equal(const SWTDAT *a, const SWTDAT *b) {
  if (a == NULL || b == NULL) return 0;
  return a->framesize == b->framesize && a->hopsize == b->hopsize &&
         a->levels == b->levels && a->capacity == b->capacity &&
         a->waveletFamily == b->waveletFamily &&
         a->waveletOrder == b->waveletOrder &&
         a->boundaryMode == b->boundaryMode &&
         a->windowPair == b->windowPair && a->olaMode == b->olaMode &&
         a->latency == b->latency;
}

int32_t swt_history_overrun(const SWTDAT *frame, uint64_t cursor) {
  uint64_t pending;
  if (frame == NULL || frame->framecount < cursor) return 1;
  pending = frame->framecount - cursor;
  if (frame->historyCount <= 0) return pending > 1;
  return pending > (uint64_t) frame->historyCount;
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

static void swt_make_high(const cs_float *low, int32_t length, cs_float *high) {
  int32_t k;
  for (k = 0; k < length; k++) {
    high[k] = ((k & 1) ? -1.0 : 1.0) * low[length - 1 - k];
  }
}

static void swt_make_windows(cs_float *windows, int32_t N, int32_t mode) {
  int32_t n;
  if (mode == SWT_WINDOW_SQRT_HANN) {
    for (n = 0; n < N; n++) {
      double w = 0.5 - 0.5 * cos(2.0 * SWT_PI * ((double) n + 0.5) /
                                 (double) N);
      windows[n] = (cs_float) sqrt(w);
    }
  } else {
    for (n = 0; n < N; n++) windows[n] = 1.0;
  }
}

int32_t swt_band_offset(const SWTDAT *frame, int32_t band) {
  return SwtDataBandOffset(frame, band);
}

int32_t swt_band_size(const SWTDAT *frame, int32_t band) {
  return SwtDataBandSize(frame, band);
}

PUBLIC int32_t csoundSwtDataFrameSize(const SWTDAT *swtdat) {
  return SwtDataFrameSize(swtdat);
}

PUBLIC int32_t csoundSwtDataHopSize(const SWTDAT *swtdat) {
  return SwtDataHopSize(swtdat);
}

PUBLIC int32_t csoundSwtDataLevels(const SWTDAT *swtdat) {
  return SwtDataLevels(swtdat);
}

PUBLIC int32_t csoundSwtDataWaveletFamily(const SWTDAT *swtdat) {
  return SwtDataWaveletFamily(swtdat);
}

PUBLIC int32_t csoundSwtDataWaveletOrder(const SWTDAT *swtdat) {
  return SwtDataWaveletOrder(swtdat);
}

PUBLIC int32_t csoundSwtDataBoundaryMode(const SWTDAT *swtdat) {
  return SwtDataBoundaryMode(swtdat);
}

PUBLIC int32_t csoundSwtDataWindowPair(const SWTDAT *swtdat) {
  return SwtDataWindowPair(swtdat);
}

PUBLIC int32_t csoundSwtDataOlaMode(const SWTDAT *swtdat) {
  return SwtDataOlaMode(swtdat);
}

PUBLIC int32_t csoundSwtDataLatency(const SWTDAT *swtdat) {
  return SwtDataLatency(swtdat);
}

PUBLIC int32_t csoundSwtDataCapacity(const SWTDAT *swtdat) {
  return SwtDataCapacity(swtdat);
}

PUBLIC double csoundSwtDataSampleRate(const SWTDAT *swtdat) {
  return SwtDataSampleRate(swtdat);
}

PUBLIC uint64_t csoundSwtDataFramecount(const SWTDAT *swtdat) {
  return SwtDataFramecount(swtdat);
}

PUBLIC uint64_t csoundSwtDataRevision(const SWTDAT *swtdat) {
  return SwtDataRevision(swtdat);
}

PUBLIC int32_t csoundSwtDataBandOffset(const SWTDAT *swtdat, int32_t band) {
  return SwtDataBandOffset(swtdat, band);
}

PUBLIC int32_t csoundSwtDataBandSize(const SWTDAT *swtdat, int32_t band) {
  return SwtDataBandSize(swtdat, band);
}

PUBLIC const cs_float *csoundGetSwtData(const SWTDAT *swtdat) {
  return GetSwtData(swtdat);
}

int32_t swt_forward(const cs_float *low, const cs_float *high, int32_t length,
                    int32_t N, int32_t J, cs_float *work, cs_float *tmp,
                    cs_float *frame) {
  const cs_float scale = (cs_float) (1.0 / SWT_SQRT2);
  int32_t j, n, k;

  if (low == NULL || high == NULL || work == NULL || tmp == NULL ||
      frame == NULL || length <= 0 || N <= 0 || J <= 0) {
    return NOTOK;
  }
  for (j = 1; j <= J; j++) {
    int64_t step64 = swt_step(j);
    int32_t step, band;
    cs_float *detail;
    if (step64 <= 0 || step64 > INT32_MAX) return NOTOK;
    step = (int32_t) step64;
    if ((int64_t) step * length > N) return NOTOK;
    band = J - j + 1;
    detail = frame + band * N;
    for (n = 0; n < N; n++) {
      cs_float a = 0.0;
      cs_float d = 0.0;
      for (k = 0; k < length; k++) {
        cs_float v = work[swt_mod(n - k * step, N)];
        a += low[k] * v;
        d += high[k] * v;
      }
      tmp[n] = scale * a;
      detail[n] = scale * d;
    }
    memcpy(work, tmp, (size_t) N * sizeof(cs_float));
  }
  memcpy(frame, work, (size_t) N * sizeof(cs_float));
  return OK;
}

int32_t swt_inverse(const cs_float *low, const cs_float *high, int32_t length,
                    int32_t N, int32_t J, const cs_float *frame, cs_float *work,
                    cs_float *tmp) {
  const cs_float scale = (cs_float) (1.0 / SWT_SQRT2);
  int32_t j, n, k;

  if (low == NULL || high == NULL || frame == NULL || work == NULL ||
      tmp == NULL || length <= 0 || N <= 0 || J <= 0) {
    return NOTOK;
  }
  memcpy(work, frame, (size_t) N * sizeof(cs_float));
  for (j = J; j >= 1; j--) {
    int64_t step64 = swt_step(j);
    int32_t step, band;
    const cs_float *detail;
    if (step64 <= 0 || step64 > INT32_MAX) return NOTOK;
    step = (int32_t) step64;
    if ((int64_t) step * length > N) return NOTOK;
    band = J - j + 1;
    detail = frame + band * N;
    for (n = 0; n < N; n++) {
      cs_float a = 0.0;
      cs_float d = 0.0;
      for (k = 0; k < length; k++) {
        int32_t idx = swt_mod(n + k * step, N);
        a += low[k] * work[idx];
        d += high[k] * detail[idx];
      }
      tmp[n] = scale * (a + d);
    }
    memcpy(work, tmp, (size_t) N * sizeof(cs_float));
  }
  return OK;
}

static int32_t swt_set_metadata(CSOUND *csound, SWTDAT *frame, int32_t N,
                                 int32_t H, int32_t J, int32_t family,
                                 int32_t order, int32_t window, int32_t ola,
                                 int32_t historyCount, int64_t startSample) {
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
  frame->generation = (uint64_t) ATOMIC_INCR(swtRunCounter);
  frame->startSample = startSample;
  frame->sampleRate = (double) csound->esr;
  return OK;
}

void swt_copy_frame(CSOUND *csound, SWTDAT *dest, const SWTDAT *src) {
  size_t bytes;
  size_t historyBytes;
  if (dest == NULL || src == NULL || dest == src) return;
  bytes = (size_t) src->capacity * sizeof(cs_float);
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
  const cs_float *low;

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
  ksmps = csound->ksmps > (int32_t) CS_KSMPS ? csound->ksmps :
          (int32_t) CS_KSMPS;
  frames = ((int64_t) ksmps + H - 1) / H +
           ((int64_t) N + H - 1) / H + 2;
  if (frames < 1 || frames > INT32_MAX) {
    csound->InitError(csound, Str("swtanal: frame history is too large\n"));
    return NOTOK;
  }
  historyCount = (int32_t) frames;
  if ((size_t) capacity > SIZE_MAX / (size_t) historyCount / sizeof(cs_float)) {
    csound->InitError(csound, Str("swtanal: frame history is too large\n"));
    return NOTOK;
  }
  if (swt_set_metadata(csound, p->out, N, H, J, family, order, window, ola,
                        historyCount,
                        (int64_t) csound->GetCurrentTimeSamples(csound) +
                            (int64_t) p->h.insdshead->ksmps_offset) != OK) {
    csound->InitError(csound, Str("swtanal: invalid SWT metadata\n"));
    return NOTOK;
  }
  p->N = N;
  p->H = H;
  p->J = J;
  p->filterLength = L;
  p->inputCount = 0;
  p->low = low;
  swt_alloc_aux(csound, (size_t) p->out->capacity * sizeof(cs_float),
                &p->out->frame);
  swt_alloc_aux(csound, (size_t) p->out->capacity *
                (size_t) historyCount * sizeof(cs_float),
                &p->out->history);
  csound->AuxAlloc(csound, (size_t) N * sizeof(cs_float), &p->input);
  csound->AuxAlloc(csound, (size_t) N * sizeof(cs_float), &p->scratch);
  csound->AuxAlloc(csound, (size_t) N * sizeof(cs_float), &p->scratch2);
  csound->AuxAlloc(csound, (size_t) N * sizeof(cs_float), &p->windows);
  csound->AuxAlloc(csound, (size_t) L * sizeof(cs_float), &p->highBuf);
  if (p->out->frame.auxp == NULL || p->out->history.auxp == NULL ||
      p->input.auxp == NULL || p->scratch.auxp == NULL ||
      p->scratch2.auxp == NULL || p->windows.auxp == NULL ||
      p->highBuf.auxp == NULL) {
    csound->InitError(csound, Str("swtanal: memory allocation failed\n"));
    return NOTOK;
  }
  swt_make_windows((cs_float *) p->windows.auxp, N, window);
  swt_make_high(low, L, (cs_float *) p->highBuf.auxp);
  p->high = (const cs_float *) p->highBuf.auxp;
  memset(p->input.auxp, 0, (size_t) N * sizeof(cs_float));
  memset(p->out->frame.auxp, 0, (size_t) p->out->capacity * sizeof(cs_float));
  memset(p->out->history.auxp, 0,
         (size_t) p->out->capacity * (size_t) historyCount * sizeof(cs_float));
  return OK;
}

int32_t swtanal(CSOUND *csound, SWTANAL *p) {
  uint32_t start = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS - early;
  uint32_t i;
  cs_float *input = (cs_float *) p->input.auxp;
  cs_float *work = (cs_float *) p->scratch.auxp;
  cs_float *tmp = (cs_float *) p->scratch2.auxp;
  cs_float *windows = (cs_float *) p->windows.auxp;
  cs_float *frame = (cs_float *) p->out->frame.auxp;
  cs_float *history = (cs_float *) p->out->history.auxp;
  int32_t writeIndex = p->inputCount % p->N;
  int32_t n;

  if (input == NULL || work == NULL || tmp == NULL || windows == NULL ||
      frame == NULL || history == NULL) {
    return NOTOK;
  }
  {
    int64_t blockStart =
        csound->GetCurrentTimeSamples(csound) - (int64_t) csound->ksmps;
    int64_t inputOffset = 0;
    if (csound->spin != NULL && p->h.insdshead->spin != NULL &&
        csound->inchnls > 0) {
      inputOffset = (int64_t) (p->h.insdshead->spin - csound->spin) /
                    (int64_t) csound->inchnls;
    }
    if (inputOffset < 0) inputOffset = 0;
    if (blockStart + inputOffset + (int64_t) start < p->out->startSample) {
      int64_t skip = p->out->startSample -
                     (blockStart + inputOffset + (int64_t) start);
      if (skip >= (int64_t) nsmps - (int64_t) start) return OK;
      start += (uint32_t) skip;
    }
  }
  for (i = start; i < nsmps; i++) {
    input[writeIndex] = (cs_float) p->ain[i];
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
             (size_t) p->out->capacity * sizeof(cs_float));
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
  size = SwtDataBandSize(p->in, band);
  if (size <= 0) {
    csound->InitError(csound, Str("swtget: invalid band selector\n"));
    return NOTOK;
  }
  p->bandValue = band;
  p->count = size;
  p->frameSize = p->in->framesize;
  p->levels = p->in->levels;
  p->capacity = p->in->capacity;
  if (tabinit(csound, p->out, size, p->h.insdshead) != OK) {
    csound->InitError(csound, Str("swtget: array allocation failed\n"));
    return NOTOK;
  }
  return OK;
}

int32_t swtget(CSOUND *csound, SWTGET *p) {
  int32_t offset = SwtDataBandOffset(p->in, p->bandValue);
  int32_t size = SwtDataBandSize(p->in, p->bandValue);
  int32_t i;
  const cs_float *data;
  if (offset < 0 || size != p->count || p->in->frame.auxp == NULL ||
      p->in->framesize != p->frameSize || p->in->levels != p->levels ||
      p->in->capacity != p->capacity) {
    return csound->PerfError(csound, &p->h,
                             Str("swtget: SWT frame layout changed\n"));
  }
  data = (const cs_float *) p->in->frame.auxp + offset;
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

static void swt_apply_band(cs_float *frame, int32_t offset, int32_t size,
                           const cs_float *values) {
  int32_t i;
  for (i = 0; i < size; i++) frame[offset + i] = (cs_float) values[i];
}

int32_t swtsetset(CSOUND *csound, SWTSET *p) {
  int32_t band = -1;
  size_t available;
  if (p == NULL || p->in == NULL || p->out == NULL) return NOTOK;
  if (p->band != NULL) band = swt_int_arg(*p->band);
  if (SwtDataBandSize(p->in, band) <= 0) {
    csound->InitError(csound, Str("swtset: invalid band selector\n"));
    return NOTOK;
  }
  p->bandValue = band;
  p->count = SwtDataBandSize(p->in, band);
  if (swt_band_array_length(p->bands, &available) != OK ||
      available < (size_t) p->count) {
    csound->InitError(csound, Str("swtset: input array is too small\n"));
    return NOTOK;
  }
  swt_copy_frame(csound, p->out, p->in);
  p->lastFrame = p->in->framecount;
  p->generation = p->in->generation;
  p->streamStart = p->in->startSample;
  return OK;
}

int32_t swtset(CSOUND *csound, SWTSET *p) {
  int32_t band = p->bandValue;
  int32_t offset = SwtDataBandOffset(p->out, band);
  int32_t size = SwtDataBandSize(p->out, band);
  cs_float *data = (cs_float *) p->out->frame.auxp;
  size_t bytes = (size_t) p->out->capacity * sizeof(cs_float);
  size_t available;
  if (!swt_layout_equal(p->in, p->out)) {
    return csound->PerfError(csound, &p->h,
                             Str("swtset: SWT frame layout changed\n"));
  }
  if (offset < 0 || size <= 0 || data == NULL ||
      swt_band_array_length(p->bands, &available) != OK ||
      available < (size_t) size) {
    return csound->PerfError(csound, &p->h,
                             Str("swtset: input array is too short\n"));
  }
  if (p->in->generation != p->generation ||
      p->in->startSample != p->streamStart ||
      p->in->framecount < p->lastFrame) {
    p->lastFrame = 0;
    p->generation = p->in->generation;
    p->streamStart = p->in->startSample;
    if (p->out != p->in) {
      p->out->framecount = 0;
      p->out->revision = p->in->revision;
      p->out->generation = p->in->generation;
      p->out->startSample = p->in->startSample;
      if (p->out->frame.auxp != NULL)
        memset(p->out->frame.auxp, 0, bytes);
      if (p->out->history.auxp != NULL)
        memset(p->out->history.auxp, 0,
               bytes * (size_t) p->out->historyCount);
    }
  }
  if (swt_history_overrun(p->in, p->lastFrame)) {
    return csound->PerfError(csound, &p->h,
                             Str("swtset: SWT history overrun\n"));
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
      cs_float *edited = (cs_float *) p->out->history.auxp +
                      outSlot * (size_t) p->out->capacity;
      memcpy(edited,
             (const cs_float *) p->in->history.auxp +
             inSlot * (size_t) p->in->capacity, bytes);
      swt_apply_band(edited, offset, size, p->bands->data);
      p->lastFrame++;
    }
    memcpy(data, (const cs_float *) p->in->frame.auxp, bytes);
    p->out->framecount = p->lastFrame;
  }
  swt_apply_band(data, offset, size, p->bands->data);
  if (p->out->history.auxp != NULL && p->out->historyCount > 0 &&
      p->out->framecount > 0) {
    size_t slot = (size_t) ((p->out->framecount - 1) %
                            (uint64_t) p->out->historyCount);
    memcpy((cs_float *) p->out->history.auxp + slot * (size_t) p->out->capacity,
           data, bytes);
  }
  p->out->revision++;
  return OK;
}

static double swt_ola_denominator(const cs_float *windows, int32_t N, int32_t H,
                                  int64_t q, const cs_float *denom) {
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
                                   const cs_float *source) {
  cs_float *recon = (cs_float *) p->recon.auxp;
  cs_float *work = (cs_float *) p->scratch.auxp;
  cs_float *tmp = (cs_float *) p->scratch2.auxp;
  cs_float *windows = (cs_float *) p->windows.auxp;
  cs_float *denom = (cs_float *) p->denom.auxp;
  cs_float *outbuf = (cs_float *) p->outbuf.auxp;
  int64_t frameOffset = (int64_t) p->nextFrame * (int64_t) p->H;
  int64_t offsetPlusLatency;
  int64_t start;
  int32_t n;
  if (frameOffset > INT64_MAX - (int64_t) p->latency) return NOTOK;
  offsetPlusLatency = frameOffset + (int64_t) p->latency;
  if (p->timeBase > INT64_MAX - offsetPlusLatency ||
      p->timeBase < INT64_MIN + offsetPlusLatency)
    return NOTOK;
  start = p->timeBase + offsetPlusLatency;

  if (recon == NULL || work == NULL || tmp == NULL || windows == NULL ||
      denom == NULL || outbuf == NULL) {
    return NOTOK;
  }
  if (swt_inverse(p->low, p->high, p->filterLength, p->N, p->J,
                  source, work, tmp) != OK) {
    return NOTOK;
  }
  memcpy(recon, work, (size_t) p->N * sizeof(cs_float));
  for (n = 0; n < p->N; n++) {
    int64_t pos = start + n;
    int64_t idx = swt_wrap64(pos, (int64_t) p->buflen);
    cs_float value = recon[n];
    if (p->olaMode == SWT_OLA_OVERLAP_ADD) {
      int64_t q = (int64_t) (p->nextFrame - p->firstFrame) * (int64_t) p->H +
                  (int64_t) n;
      double weightSum = swt_ola_denominator(windows, p->N, p->H, q, denom);
      value *= windows[n] / (cs_float) weightSum;
    }
    outbuf[idx] += value;
  }
  return OK;
}

int32_t swtsynthset(CSOUND *csound, SWTSYNTH *p) {
  int32_t N, H, J, L, historyCount, ksmps, capacity;
  int64_t buflen;
  cs_float *windows, *denom;
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
  p->generation = p->in->generation;
  p->streamStart = p->in->startSample;
  p->synthStart = (int64_t) csound->GetCurrentTimeSamples(csound) +
                  (int64_t) p->h.insdshead->ksmps_offset;
  p->timeBase = p->in->framecount == 0 ?
                p->streamStart - p->synthStart : 0;
  p->outpos = (int64_t) (p->nextFrame * (uint64_t) H);
  p->originOffset = p->outpos;
  ksmps = CS_KSMPS;
  buflen = (int64_t) 2 * N + H + 2 * (int64_t) ksmps + 8;
  if (buflen > INT32_MAX) {
    csound->InitError(csound, Str("swtsynth: buffer is too large\n"));
    return NOTOK;
  }
  p->buflen = (int32_t) buflen;
  csound->AuxAlloc(csound, (size_t) N * sizeof(cs_float), &p->recon);
  csound->AuxAlloc(csound, (size_t) N * sizeof(cs_float), &p->scratch);
  csound->AuxAlloc(csound, (size_t) N * sizeof(cs_float), &p->scratch2);
  csound->AuxAlloc(csound, (size_t) N * sizeof(cs_float), &p->windows);
  csound->AuxAlloc(csound, (size_t) H * sizeof(cs_float), &p->denom);
  csound->AuxAlloc(csound, (size_t) L * sizeof(cs_float), &p->highBuf);
  csound->AuxAlloc(csound, (size_t) p->buflen * sizeof(cs_float), &p->outbuf);
  if (p->recon.auxp == NULL || p->scratch.auxp == NULL ||
      p->scratch2.auxp == NULL || p->windows.auxp == NULL ||
      p->denom.auxp == NULL || p->highBuf.auxp == NULL ||
      p->outbuf.auxp == NULL) {
    csound->InitError(csound, Str("swtsynth: memory allocation failed\n"));
    return NOTOK;
  }
  windows = (cs_float *) p->windows.auxp;
  denom = (cs_float *) p->denom.auxp;
  swt_make_windows(windows, N, p->olaMode == SWT_OLA_OVERLAP_ADD ?
                   SWT_WINDOW_SQRT_HANN : SWT_WINDOW_NONE);
  swt_make_high(p->low, L, (cs_float *) p->highBuf.auxp);
  p->high = (const cs_float *) p->highBuf.auxp;
  for (i = 0; i < H; i++) {
    double sum = 0.0;
    int32_t n;
    if (p->olaMode == SWT_OLA_DIRECT) {
      denom[i] = 1.0;
      continue;
    }
    for (n = i; n < N; n += H) sum += (double) windows[n] * windows[n];
    denom[i] = (cs_float) (sum > 1e-12 ? sum : 1.0);
  }
  memset(p->outbuf.auxp, 0, (size_t) p->buflen * sizeof(cs_float));
  return OK;
}

int32_t swtsynth(CSOUND *csound, SWTSYNTH *p) {
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS - early;
  uint32_t i;
  int64_t clearFrom;
  int64_t clearEnd;
  int64_t pos;
  int64_t scheduled;
  cs_float *outbuf = (cs_float *) p->outbuf.auxp;
  cs_float *history = (cs_float *) p->in->history.auxp;
  cs_float *latest = (cs_float *) p->in->frame.auxp;

  if (outbuf == NULL || latest == NULL) return NOTOK;
  if (p->in->framesize != p->N || p->in->hopsize != p->H ||
      p->in->levels != p->J) {
    return NOTOK;
  }
  if (p->in->framecount > (uint64_t) (INT64_MAX / p->H)) {
    return csound->PerfError(csound, &p->h,
                             Str("swtsynth: frame count is too large\n"));
  }
  if (p->in->generation != p->generation ||
      p->in->startSample != p->streamStart ||
      p->in->framecount < p->nextFrame) {
    p->firstFrame = 0;
    p->nextFrame = 0;
    p->generation = p->in->generation;
    p->streamStart = p->in->startSample;
    p->timeBase = p->streamStart - p->synthStart + p->originOffset;
    clearFrom = p->timeBase > p->outpos ? p->timeBase : p->outpos;
    clearEnd = p->outpos + (int64_t) p->buflen;
    for (pos = clearFrom; pos < clearEnd; pos++) {
      outbuf[swt_wrap64(pos, (int64_t) p->buflen)] = 0.0;
    }
    while (p->nextFrame < p->in->framecount) {
      scheduled = p->timeBase +
                  (int64_t) (p->nextFrame * (uint64_t) p->H) +
                  (int64_t) p->latency;
      if (scheduled >= p->outpos) break;
      p->nextFrame++;
    }
    p->firstFrame = p->nextFrame;
  }
  if (swt_history_overrun(p->in, p->nextFrame)) {
    return csound->PerfError(csound, &p->h,
                             Str("swtsynth: SWT history overrun\n"));
  }
  for (i = 0; i < offset && i < (uint32_t) CS_KSMPS; i++) p->aout[i] = 0;
  for (i = nsmps; i < (uint32_t) CS_KSMPS; i++) p->aout[i] = 0;
  for (i = offset; i < nsmps; i++) {
    while (p->nextFrame < p->in->framecount) {
      uint64_t m = p->nextFrame;
      const cs_float *source;
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
      outbuf[idx] = 0.0;
    }
    p->outpos++;
  }
  return OK;
}
