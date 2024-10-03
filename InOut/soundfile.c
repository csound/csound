/*
    soundfile.c:

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/
#include "csoundCore.h"
#include "soundfile.h"

#if USE_LIBSNDFILE
const char *sflib_strerror(void *p){
  return sf_strerror((SNDFILE *)p); 
}

int32_t  sflib_set_string(void *sndfile, int32_t str_type, const char* str){
  return sf_set_string((SNDFILE *)sndfile, str_type, str) ;
}
  
int32_t sflib_command(void *handle, int32_t cmd, void *data, int32_t datasize)  {
  return sf_command((SNDFILE*) handle, cmd, data, datasize) ;
}

void *sflib_open_fd(int32_t fd, int32_t mode, SFLIB_INFO *sfinfo, int32_t close_desc) {
      SNDFILE *handle;
      SF_INFO info;
      if(mode == SFM_WRITE) {
        info.samplerate = sfinfo->samplerate;
        info.channels = sfinfo->channels;
        info.format = sfinfo->format;
      }
      handle = sf_open_fd(fd, mode, &info, close_desc);
      if(mode == SFM_READ) {
        sfinfo->samplerate = info.samplerate;
        sfinfo->channels = info.channels;
        sfinfo->format = info.format;
        sfinfo->frames  = info.frames;
      }
     return handle;
}

void *sflib_open(const char *path, int32_t mode, SFLIB_INFO *sfinfo){
      SNDFILE *handle;
      SF_INFO info;
      if(mode == SFM_WRITE) {
        info.samplerate = sfinfo->samplerate;
        info.channels = sfinfo->channels;
        info.format = sfinfo->format;
      }
      handle = sf_open(path, mode, &info);
      if(mode == SFM_READ) {
        sfinfo->samplerate = info.samplerate;
        sfinfo->channels = info.channels;
        sfinfo->format = info.format;
        sfinfo->frames  = info.frames;
      }
     return handle;
}

int32_t sflib_close(void *handle) {
    return sf_close((SNDFILE *) handle);
}

long sflib_seek(void *handle, long frames, int32_t whence) {
    return sf_seek((SNDFILE *) handle, frames, whence);
}

long sflib_read_float(void *handle, float *ptr, long items) {
    return sf_read_float((SNDFILE *) handle, ptr, items);
}

long sflib_write_float(void *handle, float *ptr, long items) {
    return sf_write_float((SNDFILE *) handle, ptr, items);
}

long sflib_read_double(void *handle, double *ptr, long items) {
    return sf_read_double((SNDFILE *) handle, ptr, items);
}

long sflib_write_double(void *handle, double *ptr, long items) {
    return sf_write_double((SNDFILE *) handle, ptr, items);
}

long sflib_readf_float(void *handle, float *ptr, long items) {
    return sf_readf_float((SNDFILE *) handle, ptr, items);
}

long sflib_writef_float(void *handle, float *ptr, long items) {
    return sf_writef_float((SNDFILE *) handle, ptr, items);
}

long sflib_readf_double(void *handle, double *ptr, long items) {
    return sf_readf_double((SNDFILE *) handle, ptr, items);
}

long sflib_writef_double(void *handle, double *ptr, long items) {
    return sf_writef_double((SNDFILE *) handle, ptr, items);
}

#else 
int32_t sflib_command(void *handle, int32_t cmd, void *data, int32_t datasize)  {
      return 0;
}

void *sflib_open_fd(int32_t fd, int32_t mode, SFLIB_INFO *sfinfo, int32_t close_desc) {
      return NULL;
}

void *sflib_open(const char *path, int32_t mode,  SFLIB_INFO *sfinfo){
      return NULL;
}

int32_t sflib_close(void *sndfile) {
    return 0;
}

long sflib_seek(void *handle, long frames, int32_t whence) {
    return 0;
}

long sflib_read_float(void *sndfile, float *ptr, long items) {
    return 0;
}

long sflib_readf_float(void *sndfile, float *ptr, long items) {
    return 0;
}

long sflib_read_double(void *handle, double *ptr, long items) {
    return 0;
}

long sflib_readf_double(void *handle, double *ptr, long items) {
    return 0;
}

long sflib_write_float(void *handle, float *ptr, long items) {
    return 0;
}

long sflib_write_double(void *handle, double *ptr, long items) {
    return 0;
}


long sflib_writef_float(void *handle, float *ptr, long items) {
    return 0;
}

long sflib_writef_double(void *handle, double *ptr, long items) {
    return 0;
}
int32_t  sflib_set_string(void *sndfile, int32_t str_type, const char* str){
  return 0;
}

const char *sflib_strerror(void *p){
  return NULL;
}
#endif

