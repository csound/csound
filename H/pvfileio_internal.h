#pragma once

#include "csound.h"
#include "pvfileio.h"

extern const GUID KSDATAFORMAT_SUBTYPE_PVOC;

/* pvoc file handling functions */

const char *pvoc_errorstr(CSOUND *);
int32_t init_pvsys(CSOUND *);
int32_t pvoc_createfile(CSOUND *, const char *, uint32, uint32, uint32, uint32,
                        int32, int32_t, int32_t, float, float *, uint32);
int32_t pvoc_openfile(CSOUND *, const char *filename, void *data_, void *fmt_);
int32_t pvoc_closefile(CSOUND *, int32_t);
int32_t pvoc_putframes(CSOUND *, int32_t ofd, const float *frame,
                       int32 numframes);
int32_t pvoc_getframes(CSOUND *, int32_t ifd, float *frames, uint32 nframes);
int32_t pvoc_framecount(CSOUND *, int32_t ifd);
int32_t pvoc_fseek(CSOUND *, int32_t ifd, int32_t offset);
int32_t pvsys_release(CSOUND *);
