#ifndef CSOUND_SWTDAT_H
#define CSOUND_SWTDAT_H

#include <stddef.h>
#include <stdint.h>
#include "csound.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  SWT_FAMILY_HAAR = 0,
  SWT_FAMILY_DB = 1,
  SWT_FAMILY_SYMLET = 2,
  SWT_FAMILY_COIFLET = 3
} SWT_WAVELET_FAMILY;

typedef enum {
  SWT_BOUNDARY_PERIODIC = 0
} SWT_BOUNDARY_MODE;

typedef enum {
  SWT_WINDOW_NONE = 0,
  SWT_WINDOW_SQRT_HANN = 1
} SWT_WINDOW_PAIR;

typedef enum {
  SWT_OLA_AUTO = 0,
  SWT_OLA_DIRECT = 1,
  SWT_OLA_OVERLAP_ADD = 2
} SWT_OLA_MODE;

typedef struct swt_auxch {
  struct swt_auxch *nxtchp;
  size_t size;
  void *auxp;
  void *endp;
} SWT_AUXCH;

struct swtdat {
  int32_t framesize;
  int32_t hopsize;
  int32_t levels;
  int32_t waveletFamily;
  int32_t waveletOrder;
  int32_t boundaryMode;
  int32_t windowPair;
  int32_t olaMode;
  int32_t latency;
  int32_t capacity;
  int32_t historyCount;
  uint32_t framecount;
  uint32_t revision;
  double sampleRate;
  SWT_AUXCH frame;
  SWT_AUXCH history;
};

PUBLIC int32_t csoundSwtDataFrameSize(const SWTDAT *swtdat);
PUBLIC int32_t csoundSwtDataHopSize(const SWTDAT *swtdat);
PUBLIC int32_t csoundSwtDataLevels(const SWTDAT *swtdat);
PUBLIC int32_t csoundSwtDataWaveletFamily(const SWTDAT *swtdat);
PUBLIC int32_t csoundSwtDataWaveletOrder(const SWTDAT *swtdat);
PUBLIC int32_t csoundSwtDataBoundaryMode(const SWTDAT *swtdat);
PUBLIC int32_t csoundSwtDataWindowPair(const SWTDAT *swtdat);
PUBLIC int32_t csoundSwtDataOlaMode(const SWTDAT *swtdat);
PUBLIC int32_t csoundSwtDataLatency(const SWTDAT *swtdat);
PUBLIC int32_t csoundSwtDataCapacity(const SWTDAT *swtdat);
PUBLIC double csoundSwtDataSampleRate(const SWTDAT *swtdat);
PUBLIC uint32_t csoundSwtDataFramecount(const SWTDAT *swtdat);
PUBLIC uint32_t csoundSwtDataRevision(const SWTDAT *swtdat);
PUBLIC int32_t csoundSwtDataBandOffset(const SWTDAT *swtdat, int32_t band);
PUBLIC int32_t csoundSwtDataBandSize(const SWTDAT *swtdat, int32_t band);
PUBLIC const float *csoundGetSwtData(const SWTDAT *swtdat);

#ifdef __cplusplus
}
#endif

#endif
