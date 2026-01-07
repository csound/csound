/*
    soundfile.c: soundfile backend

    Copyright (C) 2021 V Lazzarini

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
#include "csoundCore.h"
#include "soundfile.h"

#if USE_LIBSNDFILE
static const char *sflib_strerror(void *p){
  return sf_strerror((SNDFILE *)p); 
}

static inline int32_t  sflib_set_string(void *sndfile, int32_t str_type, const char* str){
  return sf_set_string((SNDFILE *)sndfile, str_type, str) ;
}
  
static inline int32_t sflib_command(void *handle, int32_t cmd, void *data, int32_t datasize)  {
  return sf_command((SNDFILE*) handle, cmd, data, datasize) ;
}

static inline void *sflib_open_fd(int32_t fd, int32_t mode, SFLIB_INFO *sfinfo, int32_t close_desc) {
      SNDFILE *handle;
      SF_INFO info = {
        .samplerate = sfinfo->samplerate,
        .channels = sfinfo->channels,
        .format = sfinfo->format,
        .frames = sfinfo->frames,
      };
      handle = sf_open_fd(fd, mode, &info, close_desc);
      if(mode == SFM_READ) {
        sfinfo->samplerate = info.samplerate;
        sfinfo->channels = info.channels;
        sfinfo->format = info.format;
        sfinfo->frames  = info.frames;
      }
     return handle;
}

static inline void *sflib_open(const char *path, int32_t mode, SFLIB_INFO *sfinfo){
      SNDFILE *handle;
      SF_INFO info;
      info.samplerate = sfinfo->samplerate;
      info.channels = sfinfo->channels;
      info.format = sfinfo->format;
      handle = sf_open(path, mode, &info);
      if(mode == SFM_READ) {
        sfinfo->samplerate = info.samplerate;
        sfinfo->channels = info.channels;
        sfinfo->format = info.format;
        sfinfo->frames  = info.frames;
      }
     return handle;
}

static inline int32_t sflib_close(void *handle) {
    return sf_close((SNDFILE *) handle);
}

static inline long sflib_seek(void *handle, long frames, int32_t whence) {
    return sf_seek((SNDFILE *) handle, frames, whence);
}

#ifndef USE_DOUBLE
static inline long sflib_read_MYFLT(void *handle, float *ptr, long items) {
    return sf_read_float((SNDFILE *) handle, ptr, items);
}

static inline long sflib_write_MYFLT(void *handle, float *ptr, long items) {
    return sf_write_float((SNDFILE *) handle, ptr, items);
}

static inline long sflib_readf_MYFLT(void *handle, float *ptr, long items) {
    return sf_readf_float((SNDFILE *) handle, ptr, items);
}

static inline long sflib_writef_MYFLT(void *handle, float *ptr, long items) {
    return sf_writef_float((SNDFILE *) handle, ptr, items);
}

#else
static inline long sflib_read_MYFLT(void *handle, double *ptr, long items) {
    return sf_read_double((SNDFILE *) handle, ptr, items);
}

static inline long sflib_write_MYFLT(void *handle, double *ptr, long items) {
    return sf_write_double((SNDFILE *) handle, ptr, items);
}

static inline long sflib_readf_MYFLT(void *handle, double *ptr, long items) {
    return sf_readf_double((SNDFILE *) handle, ptr, items);
}

static inline long sflib_writef_MYFLT(void *handle, double *ptr, long items) {
    return sf_writef_double((SNDFILE *) handle, ptr, items);
}
#endif // !USE_DOUBLE

#else 
static int32_t sflib_command(void *handle, int32_t cmd, void *data, int32_t datasize)  {
      return 0;
}

static void *sflib_open_fd(int32_t fd, int32_t mode, SFLIB_INFO *sfinfo, int32_t close_desc) {
      return NULL;
}

static void *sflib_open(const char *path, int32_t mode,  SFLIB_INFO *sfinfo){
      return NULL;
}

static int32_t sflib_close(void *sndfile) {
    return 0;
}

static long sflib_seek(void *handle, long frames, int32_t whence) {
    return 0;
}

static long sflib_read_float(void *sndfile, float *ptr, long items) {
    return 0;
}

static long sflib_readf_float(void *sndfile, float *ptr, long items) {
    return 0;
}

static long sflib_read_double(void *handle, double *ptr, long items) {
    return 0;
}

static long sflib_readf_double(void *handle, double *ptr, long items) {
    return 0;
}

static long sflib_write_float(void *handle, float *ptr, long items) {
    return 0;
}

static long sflib_write_double(void *handle, double *ptr, long items) {
    return 0;
}


static long sflib_writef_float(void *handle, float *ptr, long items) {
    return 0;
}

static long sflib_writef_double(void *handle, double *ptr, long items) {
    return 0;
}
static int32_t  sflib_set_string(void *sndfile, int32_t str_type, const char* str){
  return 0;
}

static const char *sflib_strerror(void *p){
  return NULL;
}
#endif

int64_t csoundSndfileWrite(CSOUND *csound, void *h, MYFLT *p, int64_t frames) {
  IGN(csound);
  return sflib_writef_MYFLT(h, p, frames);
}

int64_t csoundSndfileRead(CSOUND *csound, void *h, MYFLT *p, int64_t frames) {
  IGN(csound);
  return sflib_readf_MYFLT(h, p, frames);
}

int64_t csoundSndfileWriteSamples(CSOUND *csound, void *h, MYFLT *p,
                                   int64_t samples) {
  IGN(csound);
  return sflib_write_MYFLT(h, p, samples);
}

int64_t csoundSndfileReadSamples(CSOUND *csound, void *h, MYFLT *p,
                                  int64_t samples) {
  IGN(csound);
  return sflib_read_MYFLT(h, p, samples);
}

int64_t csoundSndfileSeek(CSOUND *csound, void *h, int64_t frames,
                           int32_t whence) {
  IGN(csound);
  return sflib_seek(h, frames, whence);
}

void *csoundSndfileOpen(CSOUND *csound, const char *path, int32_t mode,
                         SFLIB_INFO *sfinfo) {
  IGN(csound);
  return sflib_open(path, mode, sfinfo);
}

void *csoundSndfileOpenFd(CSOUND *csound, int32_t fd, int32_t mode,
                           SFLIB_INFO *sfinfo, int32_t close_desc) {
  IGN(csound);
  return sflib_open_fd(fd, mode, sfinfo, close_desc);
}

int32_t csoundSndfileClose(CSOUND *csound, void *sndfile) {
  IGN(csound);
  return sflib_close(sndfile);
}

int32_t csoundSndfileSetString(CSOUND *csound, void *sndfile, int32_t str_type,
                                const char *str) {
  IGN(csound);
  return sflib_set_string(sndfile, str_type, str);
}

const char *csoundSndfileStrError(CSOUND *csound, void *sndfile) {
  IGN(csound);
  return sflib_strerror(sndfile);
}

int32_t csoundSndfileCommand(CSOUND *csound, void *handle, int32_t cmd,
                              void *data, int32_t datasize) {
  return sflib_command(handle, cmd, data, datasize);
}

// stubs

static int64_t csoundSndfileWrite_stub(CSOUND *csound, void *h, MYFLT *p, int64_t frames) {
  return 0;
}

static int64_t csoundSndfileRead_stub(CSOUND *csound, void *h, MYFLT *p, int64_t frames) {
  return 0;
}

static int64_t csoundSndfileWriteSamples_stub(CSOUND *csound, void *h, MYFLT *p,
                                   int64_t samples) {
  return 0;
}

static int64_t csoundSndfileReadSamples_stub(CSOUND *csound, void *h, MYFLT *p,
                                  int64_t samples) {
  return 0;
}

static int64_t csoundSndfileSeek_stub(CSOUND *csound, void *h, int64_t frames,
                           int32_t whence) {
  return 0;
}

static void *csoundSndfileOpen_stub(CSOUND *csound, const char *path, int32_t mode,
                         SFLIB_INFO *sfinfo) {
  return 0;
}

static void *csoundSndfileOpenFd_stub(CSOUND *csound, int32_t fd, int32_t mode,
                           SFLIB_INFO *sfinfo, int32_t close_desc) {
  return 0;
}

static int32_t csoundSndfileClose_stub(CSOUND *csound, void *sndfile) {
  return 0;
}

static int32_t csoundSndfileSetString_stub(CSOUND *csound, void *sndfile, int32_t str_type,
                                const char *str) {
  return 0;
}

static const char *csoundSndfileStrError_stub(CSOUND *csound, void *sndfile) {
  return NULL;
}

static int32_t csoundSndfileCommand_stub(CSOUND *csound, void *handle, int32_t cmd,
                              void *data, int32_t datasize) {
  return 0;
}

/** Sets the callbacks for sndfile IO
    NULL callbacks are replaced by stubs.
 */
void csoundSetSndfileCallbacks(CSOUND *csound, SNDFILE_CALLBACKS *p) {
  if (p == NULL) {
    csound->SndfileOpen = csoundSndfileOpen;
    csound->SndfileOpenFd = csoundSndfileOpenFd;
    csound->SndfileClose = csoundSndfileClose;
    csound->SndfileWrite = csoundSndfileWrite;
    csound->SndfileRead = csoundSndfileRead;
    csound->SndfileWriteSamples = csoundSndfileWriteSamples;
    csound->SndfileReadSamples = csoundSndfileReadSamples;
    csound->SndfileSeek = csoundSndfileSeek;
    csound->SndfileSetString = csoundSndfileSetString;
    csound->SndfileStrError = csoundSndfileStrError;
    csound->SndfileCommand = csoundSndfileCommand;
  } else {
    csound->SndfileOpen = p->SndfileOpen ? p->SndfileOpen : csoundSndfileOpen_stub;
    csound->SndfileOpenFd = p->SndfileOpenFd ? p->SndfileOpenFd : csoundSndfileOpenFd_stub;
    csound->SndfileClose = p->SndfileClose ? p->SndfileClose : csoundSndfileClose_stub;
    csound->SndfileWrite = p->SndfileWrite ? p->SndfileWrite : csoundSndfileWrite_stub;
    csound->SndfileRead = p->SndfileRead ? p->SndfileRead : csoundSndfileRead_stub;
    csound->SndfileWriteSamples =
        p->SndfileWriteSamples ? p->SndfileWriteSamples : csoundSndfileWriteSamples_stub;
    csound->SndfileReadSamples =
        p->SndfileReadSamples ? p->SndfileReadSamples : csoundSndfileReadSamples_stub;
    csound->SndfileSeek = p->SndfileSeek ? p->SndfileSeek : csoundSndfileSeek_stub;
    csound->SndfileSetString =
        p->SndfileSetString ? p->SndfileSetString : csoundSndfileSetString_stub;
    csound->SndfileStrError =
        p->SndfileStrError ? p->SndfileStrError : csoundSndfileStrError_stub;
    csound->SndfileCommand =
        p->SndfileCommand ? p->SndfileCommand : csoundSndfileCommand_stub;
  }
}

