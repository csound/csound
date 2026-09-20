/*
  fout.c:

  Copyright (C) 1999 Gabriel Maldonado, John ffitch, Matt Ingalls
  (C) 2005, 2006 Istvan Varga, Victor Lazzarini

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

/* Opcodes By Gabriel Maldonado, 1999 */
/* Code modified by JPff to remove fixed size arrays, allow
   AIFF and WAV, and close files neatly.  Also bugs fixed */

#include "stdopcod.h"
#include "fout.h"
#include "soundio.h"
#include <ctype.h>
#include <limits.h>

/* remove a file reference, optionally closing the file */
static CS_NOINLINE int32_t fout_deinit(CSOUND *csound, FOUT_FILE *p)
{
  if(p->need_deinit) {
    p->need_deinit = 0;
    struct fileinTag  *pp;
    STDOPCOD_GLOBALS *ppp = (STDOPCOD_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                                            "STDOPC_GLOBALS");

    p->sf = (SNDFILE*) NULL;
    p->f = (FILE*) NULL;
    if (p->idx) {
      pp = &(ppp->file_opened[p->idx - 1]);
      p->idx = 0;
      if (pp->refCount) {
        pp->refCount--;
        /* The high bit records a pending ficlose, not another user. */
        if ((pp->refCount & 0x7fffffffU) == 0) {
          pp->file = (SNDFILE*) NULL;
          pp->raw = (FILE*) NULL;
          csound->Free(csound, pp->name);
          pp->name = (char*) NULL;
          pp->do_scale = 0;
          pp->refCount = 0U;

          if (pp->fd != NULL) {
            if (((csound->GetOParms(csound))->msglevel & 7) == 7)
              csound->Message(csound, Str("Closing file '%s'...\n"),
                              csound->GetFileName(pp->fd));
            /* Do not wait for asynchronous file-worker borrowers from a
               realtime deinit pass. Unborrowed files still close here. */
            csound->FileClose(csound, pp->fd, CSFILE_CLOSE_DEFER);
            pp->fd = NULL;
          }
        }
      }
    }
  }
  return OK;
}

static CS_NOINLINE FOUT_FILE *fout_open_file(CSOUND *csound, FOUT_FILE *p, void *fp,
                                             int32_t fileType, MYFLT *iFile,
                                             int32_t isString,
                                             void *fileParams, int32_t forceSync,
                                             int32_t *handle)
{
  STDOPCOD_GLOBALS  *pp =  (STDOPCOD_GLOBALS*)
    csound->QueryGlobalVariable(csound,"STDOPC_GLOBALS");
  char              *name;
  int32_t               idx, csFileType;
  const OPARMS *oparms = csound->GetOParms(csound);

  if (handle != NULL) *handle = -1;

  if (p != (FOUT_FILE*) NULL) p->async = 0;
  if (fp != NULL) {
    if (fileType == CSFILE_STD)
      *((FILE**) fp) = (FILE*) NULL;
    else
      *((SNDFILE**) fp) = (SNDFILE*) NULL;
  }
  /* if the opcode already uses a file, remove old reference first */
  if (p != (FOUT_FILE*) NULL) {
    if (p->idx) {
      p->need_deinit = 1;
      fout_deinit(csound, (void*) p);
    }
    p->need_deinit = 1;
  }
  /* get file name, */
  if (isString) name = csound->Strdup(csound, ((STRINGDAT *)iFile)->data);
  else if (IsStringCode(*iFile))
    name = csound->Strdup(csound, csound->GetArgString(csound, *iFile));
  /* else csound->strarg2name(csound, NULL, iFile, "fout.", 0);*/
  else {
    /* or handle to previously opened file */
    idx = (int32_t) MYFLT2LRND(*iFile);
    if (UNLIKELY(idx < 0 || idx > pp->file_num ||
                 (fileType == CSFILE_STD && pp->file_opened[idx].raw == NULL) ||
                 (fileType != CSFILE_STD && pp->file_opened[idx].file == NULL))) {
      csound->InitError(csound, "%s", Str("invalid file handle"));
      return NULL;
    }
    goto returnHandle;
  }
  /* check for a valid name */
  if (UNLIKELY(name == NULL || name[0] == '\0')) {
    csound->Free(csound, name);
    csound->InitError(csound, "%s", Str("invalid file name"));
    return NULL;
  }
  /* is this file already open ? */
  if (fileType == CSFILE_STD) {
    for (idx = 0; idx <= pp->file_num; idx++) {
      if (pp->file_opened[idx].raw != (FILE*) NULL &&
          strcmp(pp->file_opened[idx].name, name) == 0) {
        csound->Free(csound, name);
        goto returnHandle;
      }
    }
  }
  else {
    for (idx = 0; idx <= pp->file_num; idx++) {
      if (pp->file_opened[idx].file != (SNDFILE*) NULL &&
          strcmp(pp->file_opened[idx].name, name) == 0) {
        csound->Free(csound, name);
        goto returnHandle;
      }
    }
  }
  /* allocate new file handle, or use an already existing unused one */
  for (idx = 0; idx <= pp->file_num; idx++) {
    if (pp->file_opened[idx].fd == NULL)
      break;
  }
  if (idx > pp->file_num) {
    if (idx >= pp->file_max) {
      struct fileinTag  *tmp;
      /* Expand by 4 each time */
      pp->file_max = (idx | 3) + 1;
      tmp = (struct fileinTag *)
        csound->ReAlloc(csound, pp->file_opened,
                        sizeof(struct fileinTag) * pp->file_max);
      pp->file_opened = tmp;
      memset(&(tmp[pp->file_num + 1]), 0,
             sizeof(struct fileinTag) * (pp->file_max - (pp->file_num + 1)));
    }
    pp->file_num = idx;
  }
  memset(&pp->file_opened[idx], 0, sizeof(struct fileinTag));
  if (fileType == CSFILE_STD) {
    FILE    *f;
    void    *fd;
    char    *filemode = (char*)fileParams;
    
    if ((strcmp(filemode, "rb") == 0 || (strcmp(filemode, "wb") == 0)))
      csFileType = CSFTYPE_OTHER_BINARY;
    else  csFileType = CSFTYPE_OTHER_TEXT;
    fd = csound->FileOpen(csound, &f, fileType, name, fileParams, "",
                           csFileType, 0);
    if (UNLIKELY(fd == NULL)) {
      csound->InitError(csound, Str("error opening file '%s'"), name);
      csound->Free(csound, name);
      return NULL;
    }
    /* Set buffering only on a new stream, before any reads or writes. */
    if (csFileType == CSFTYPE_OTHER_BINARY)
      setbuf(f, NULL);
    pp->file_opened[idx].raw = f;
    pp->file_opened[idx].fd = fd;
  }
  else {
    SNDFILE *sf;
    void    *fd;
    int32_t     do_scale = 0;

    if (fileType == CSFILE_SND_W) {
      do_scale = ((SFLIB_INFO*) fileParams)->format;
      csFileType = csound->SndfileType2CsfileType(do_scale);
      if (oparms->realtime == 0 || forceSync == 1) {
        fd = csound->FileOpen(csound, &sf, fileType, name, fileParams,
                               "SFDIR", csFileType, 0);
        p->async = 0;
      }
      else {
        p->fd = fd = csound->FileOpenAsync(csound, &sf, fileType,
                                           name, fileParams,
                                           "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO,
                                           (int32_t) p->bufsize,  0);
        p->async = 1;
      }
      p->nchnls = ((SFLIB_INFO*) fileParams)->channels;
    }
    else {
      if (oparms->realtime == 0 || forceSync == 1) {
        fd = csound->FileOpen(csound, &sf, fileType, name, fileParams,
                               "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
        p->async = 0;

      } else {
        p->fd = fd = csound->FileOpenAsync(csound, &sf, fileType,
                                           name, fileParams,
                                           "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO,
                                           (int32_t) p->bufsize,  0);
        p->async = 1;
      }
      p->nchnls = ((SFLIB_INFO*) fileParams)->channels;
      do_scale = ((SFLIB_INFO*) fileParams)->format;
    }
    do_scale = (SF2TYPE(do_scale) == TYP_RAW ? 0 : 1);
    if (UNLIKELY(fd == NULL)) {
      csound->InitError(csound, Str("error opening sound file '%s'"), name);
      csound->Free(csound, name);
      return NULL;
    }
    if (!do_scale) {
#ifdef USE_DOUBLE
      csound->SndfileCommand(csound,sf, SFC_SET_NORM_DOUBLE, NULL, SFLIB_FALSE);
#else
      csound->SndfileCommand(csound,sf, SFC_SET_NORM_FLOAT, NULL, SFLIB_FALSE);
#endif
    }
    pp->file_opened[idx].file = sf;
    pp->file_opened[idx].fd = fd;
    pp->file_opened[idx].do_scale = do_scale;
    pp->file_opened[idx].nchnls = ((SFLIB_INFO*) fileParams)->channels;
  }
  /* store file information */
  pp->file_opened[idx].name = name;

 returnHandle:
  if (UNLIKELY(fileType == CSFILE_SND_W &&
               pp->file_opened[idx].nchnls !=
                   ((SFLIB_INFO*)fileParams)->channels)) {
    csound->InitError(csound, "%s",
                     Str("fout: file channel count does not match input"));
    return NULL;
  }
  /* return 'idx' as file handle */
  if (handle != NULL) *handle = idx;
  if (fp != NULL) {
    if (fileType == CSFILE_STD)
      *((FILE**) fp) = pp->file_opened[idx].raw;
    else
      *((SNDFILE**) fp) = pp->file_opened[idx].file;
  }
  if (p != (FOUT_FILE*) NULL) {
    if (fileType == CSFILE_STD) {
      p->sf = (SNDFILE*) NULL;
      p->f = pp->file_opened[idx].raw;
    }
    else {
      p->sf = pp->file_opened[idx].file;
      p->nchnls = pp->file_opened[idx].nchnls;
      p->f = (FILE*) NULL;
    }
    p->idx = idx + 1;
    pp->file_opened[idx].refCount++;
  }
  return p;
}

static int32_t outfile(CSOUND *csound, OUTFILE *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, j, k, nsmps = CS_KSMPS;
  uint32_t nargs = p->nargs;
  MYFLT *buf = (MYFLT *) p->buf.auxp;

  if (UNLIKELY(early)) nsmps -= early;
  if (p->f.sf == NULL) {
    if (p->f.f != NULL) { /* VL: make sure there is an open file */
      FILE  *fp = p->f.f;
      for (k = offset; k < nsmps; k++) {
        for (j = 0; j < nargs; j++)
          fprintf(fp, "%g ", p->argums[j][k]);
        fprintf(fp, "\n");
      }
    }
  }
  else {
    for (j = offset, k = p->buf_pos; j < nsmps; j++)
      for (i = 0; i < nargs; i++)
        buf[k++] = p->argums[i][j] * p->scaleFac;
    p->buf_pos = k;
    if (p->buf_pos >= p->guard_pos) {
      if (p->f.async==1)
        csound->WriteAsync(csound, p->f.fd, buf, p->buf_pos);
      else //csound->SndfileWriteSamples(csound, p->f.sf, buf, p->buf_pos);
        csound->SndfileWrite(csound, p->f.sf, buf, p->buf_pos/nargs); // in frames
      p->buf_pos = 0;
    }

  }
  return OK;
}

static int32_t outfile_array(CSOUND *csound, OUTFILEA *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t i, j, k, nsmps = CS_KSMPS;
  uint32_t nargs = p->f.nchnls;
  MYFLT *buf = (MYFLT *) p->buf.auxp;
  MYFLT *data = p->tabin->data;
  size_t stride = p->tabin->arrayMemberSize / sizeof(MYFLT);

  if (UNLIKELY(p->tabin->dimensions != 1 ||
               p->tabin->sizes[0] != (int32_t)nargs))
    return csound->PerfError(csound, &p->h,
                            "%s", Str("fout: array channel count changed"));

  if (UNLIKELY(early)) nsmps -= early;
  if (p->f.sf == NULL) {
    if (p->f.f != NULL) { /* VL: make sure there is an open file */
      FILE  *fp = p->f.f;
      for (k = offset; k < nsmps; k++) {
        for (j = 0; j < nargs; j++)
          fprintf(fp, "%g ", data[j*stride+k]);
        fprintf(fp, "\n");
      }
    }
  }
  else {
    for (j = offset, k = p->buf_pos; j < nsmps; j++)
      for (i = 0; i < nargs; i++)
        buf[k++] = data[i*stride+j] * p->scaleFac;
    p->buf_pos = k;
    if (p->buf_pos >= p->guard_pos) {
      if (p->f.async==1)
        csound->WriteAsync(csound, p->f.fd, buf, p->buf_pos);
      else
        //csound->SndfileWriteSamples(csound, p->f.sf, buf, p->buf_pos);
        csound->SndfileWrite(csound, p->f.sf, (MYFLT *) buf, p->buf_pos/nargs); // in frames
      p->buf_pos = 0;
    }

  }
  return OK;
}

static const int32_t fout_format_table[51] = {
  /* 0 - 9 */
  (AE_FLOAT | TYP2SF(TYP_RAW)), (AE_SHORT | TYP2SF(TYP_RAW)),
  AE_SHORT, AE_ULAW, AE_SHORT, AE_LONG,
  AE_FLOAT, AE_UNCH, AE_24INT, AE_DOUBLE,
  /* 10 - 19 */
  TYP2SF(TYP_WAV), (AE_CHAR | TYP2SF(TYP_WAV)),
  (AE_ALAW | TYP2SF(TYP_WAV)), (AE_ULAW | TYP2SF(TYP_WAV)),
  (AE_SHORT | TYP2SF(TYP_WAV)), (AE_LONG | TYP2SF(TYP_WAV)),
  (AE_FLOAT | TYP2SF(TYP_WAV)), (AE_UNCH | TYP2SF(TYP_WAV)),
  (AE_24INT | TYP2SF(TYP_WAV)), (AE_DOUBLE | TYP2SF(TYP_WAV)),
  /* 20 - 29 */
  TYP2SF(TYP_AIFF), (AE_CHAR | TYP2SF(TYP_AIFF)),
  (AE_ALAW | TYP2SF(TYP_AIFF)), (AE_ULAW | TYP2SF(TYP_AIFF)),
  (AE_SHORT | TYP2SF(TYP_AIFF)), (AE_LONG | TYP2SF(TYP_AIFF)),
  (AE_FLOAT | TYP2SF(TYP_AIFF)), (AE_UNCH | TYP2SF(TYP_AIFF)),
  (AE_24INT | TYP2SF(TYP_AIFF)), (AE_DOUBLE | TYP2SF(TYP_AIFF)),
  /* 30 - 39 */
  TYP2SF(TYP_RAW), (AE_CHAR | TYP2SF(TYP_RAW)),
  (AE_ALAW | TYP2SF(TYP_RAW)), (AE_ULAW | TYP2SF(TYP_RAW)),
  (AE_SHORT | TYP2SF(TYP_RAW)), (AE_LONG | TYP2SF(TYP_RAW)),
  (AE_FLOAT | TYP2SF(TYP_RAW)), (AE_UNCH | TYP2SF(TYP_RAW)),
  (AE_24INT | TYP2SF(TYP_RAW)), (AE_DOUBLE | TYP2SF(TYP_RAW)),
  /* 40 - 49 */
  TYP2SF(TYP_IRCAM), (AE_CHAR | TYP2SF(TYP_IRCAM)),
  (AE_ALAW | TYP2SF(TYP_IRCAM)), (AE_ULAW | TYP2SF(TYP_IRCAM)),
  (AE_SHORT | TYP2SF(TYP_IRCAM)), (AE_LONG | TYP2SF(TYP_IRCAM)),
  (AE_FLOAT | TYP2SF(TYP_IRCAM)), (AE_UNCH | TYP2SF(TYP_IRCAM)),
  (AE_24INT | TYP2SF(TYP_IRCAM)), (AE_DOUBLE | TYP2SF(TYP_IRCAM)),
  /* 50 */
  (TYP2SF(TYP_OGG) | AE_VORBIS)
};

static int32_t fout_flush_callback(CSOUND *csound, void *p_)
{
  OUTFILE            *p = (OUTFILE*) p_;

  if (p->f.sf != NULL && p->buf_pos > 0) {
    if (p->f.async == 1)
      csound->WriteAsync(csound, p->f.fd, (MYFLT *) p->buf.auxp, p->buf_pos);
    else
      // csound->SndfileWriteSamples(csound, p->f.sf, (MYFLT *) p->buf.auxp, p->buf_pos);
      csound->SndfileWrite(csound, p->f.sf, (MYFLT *) p->buf.auxp, p->buf_pos/p->nargs); // in frames
  }
  return fout_deinit(csound, &(p->f));
}

static int32_t fouta_flush_callback(CSOUND *csound, void *p_)
{
  OUTFILEA           *p = (OUTFILEA*) p_;

  if (p->f.sf != NULL && p->buf_pos > 0) {
    if (p->f.async == 1)
      csound->WriteAsync(csound, p->f.fd, (MYFLT *) p->buf.auxp, p->buf_pos);
    else
      // csound->SndfileWriteSamples(csound, p->f.sf, (MYFLT *) p->buf.auxp, p->buf_pos);
      csound->SndfileWrite(csound, p->f.sf, (MYFLT *) p->buf.auxp, p->buf_pos/p->f.nchnls); // in frames
  }
  return fout_deinit(csound, &(p->f));
}

static int32_t outfile_set_S(CSOUND *csound, OUTFILE *p)
{
  SFLIB_INFO sfinfo;
  int32_t     format_, n, buf_reqd;
  int32_t istring = 1;
  STDOPCOD_GLOBALS *pp = (STDOPCOD_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                                         "STDOPC_GLOBALS");
  const OPARMS *oparms = csound->GetOParms(csound);
  
  memset(&sfinfo, 0, sizeof(SFLIB_INFO));
  format_ = (int32_t) MYFLT2LRND(*p->iflag);
  if (format_ >= 51)
    sfinfo.format = AE_SHORT | TYP2SF(TYP_RAW);
  else if (format_ < 0) {
    sfinfo.format = FORMAT2SF(oparms->outformat);
    sfinfo.format |= TYPE2SF(oparms->filetyp);
  }
  else sfinfo.format = fout_format_table[format_];
  if (!SF2FORMAT(sfinfo.format))
    sfinfo.format |= FORMAT2SF(oparms->outformat);
  if (!SF2TYPE(sfinfo.format))
    sfinfo.format |= TYPE2SF(oparms->filetyp);
  sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_ESR);
  p->nargs = p->INOCOUNT - 2;
  p->buf_pos = 0;

  if (CS_KSMPS >= 512)
    p->guard_pos = CS_KSMPS * p->nargs;
  else
    p->guard_pos = 512* p->nargs;
  if (CS_KSMPS >= 512)
    buf_reqd = CS_KSMPS *  p->nargs;
  else
    buf_reqd = (1 + (int32_t)(512 / CS_KSMPS)) * CS_KSMPS *  p->nargs;
  if (p->buf.auxp == NULL || p->buf.size < buf_reqd*sizeof(MYFLT)) {
    csound->AuxAlloc(csound, sizeof(MYFLT)*buf_reqd, &p->buf);
  }
  p->f.bufsize =  (int32_t) p->buf.size;
  sfinfo.channels = p->nargs;
  if(fout_open_file(csound, &(p->f), NULL, CSFILE_SND_W,
                    p->fname, istring, &sfinfo, 0, NULL) != NULL)
    n = p->f.idx - 1;
  else return NOTOK;

  if (pp->file_opened[n].do_scale)
    p->scaleFac = (FL(1.)/csound->Get0dBFS(csound));
  else
    p->scaleFac = FL(1.0);
  return OK;
}

static int32_t outfile_set_A(CSOUND *csound, OUTFILEA *p)
{
  SFLIB_INFO sfinfo;
  int32_t     format_, n, len;
  size_t buf_reqd, frames;
  const OPARMS *oparms = csound->GetOParms(csound);
  STDOPCOD_GLOBALS *pp = (STDOPCOD_GLOBALS*) csound->QueryGlobalVariable(csound,
                                                                         "STDOPC_GLOBALS");
  
  p->buf_pos = 0;
  if (UNLIKELY(p->tabin->dimensions != 1 || p->tabin->sizes == NULL ||
               p->tabin->sizes[0] <= 0))
    return csound->InitError(csound, "%s",
                            Str("fout: expected a nonempty one-dimensional array"));
  len = p->tabin->sizes[0];
  frames = CS_KSMPS >= 512 ? CS_KSMPS : (1 + 512 / CS_KSMPS) * CS_KSMPS;
  /* The asynchronous file queue also needs four times this byte count. */
  if (UNLIKELY((size_t)len > INT32_MAX / 4 / sizeof(MYFLT) / frames))
    return csound->InitError(csound, "%s", Str("fout: too many channels"));
  memset(&sfinfo, 0, sizeof(SFLIB_INFO));
  format_ = (int32_t) MYFLT2LRND(*p->iflag);
  if (format_ >=  51)
    sfinfo.format = AE_SHORT | TYP2SF(TYP_RAW);
  else if (format_ < 0) {
    sfinfo.format = FORMAT2SF(oparms->outformat);
    sfinfo.format |= TYPE2SF(oparms->filetyp);
  }
  else
    sfinfo.format = fout_format_table[format_];
  if (!SF2FORMAT(sfinfo.format))
    sfinfo.format |= FORMAT2SF(oparms->outformat);
  if (!SF2TYPE(sfinfo.format))
    sfinfo.format |= TYPE2SF(oparms->filetyp);
  sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_ESR);
  p->guard_pos = (CS_KSMPS >= 512 ? CS_KSMPS : 512) * len;
  buf_reqd = frames * len;
  if (p->buf.auxp == NULL || p->buf.size < buf_reqd*sizeof(MYFLT)) {
    csound->AuxAlloc(csound, sizeof(MYFLT)*buf_reqd, &p->buf);
  }
  p->f.bufsize = (int32_t)  p->buf.size;
  sfinfo.channels = len;
  if(fout_open_file(csound, &(p->f), NULL, CSFILE_SND_W,
                    p->fname, 1, &sfinfo, 0, NULL) != NULL){
    n = p->f.idx - 1;
  }
  else return NOTOK;

  if (pp->file_opened[n].do_scale)
    p->scaleFac = (FL(1.)/csound->Get0dBFS(csound));
  else
    p->scaleFac = FL(1.0);
  return OK;
}


static int32_t koutfile(CSOUND *csound, KOUTFILE *p)
{
  int32_t   i, k;
  int32_t nargs = p->nargs;
  MYFLT *buf = (MYFLT *) p->buf.auxp;

  for (i = 0, k = p->buf_pos; i < nargs; i++)
    buf[k++] = p->argums[i][0] * p->scaleFac;
  p->buf_pos = k;
  if (p->buf_pos >= p->guard_pos) {
    if (p->f.async==1)
      csound->WriteAsync(csound, p->f.fd, buf, p->buf_pos);
    else //csound->SndfileWriteSamples(csound, p->f.sf, buf, p->buf_pos);
      csound->SndfileWrite(csound, p->f.sf, (MYFLT *) buf, p->buf_pos/nargs); // in frames
    p->buf_pos = 0;
  }
  return OK;
}

int32_t koutfile_deinit(CSOUND *csound, KOUTFILE *p) {
  if (p->f.sf != NULL && p->buf_pos > 0) {
    if (p->f.async == 1)
      csound->WriteAsync(csound, p->f.fd, (MYFLT *)p->buf.auxp, p->buf_pos);
    else
      csound->SndfileWrite(csound, p->f.sf, (MYFLT *)p->buf.auxp,
                          p->buf_pos / p->nargs);
  }
  return fout_deinit(csound, &(p->f));
}

static int32_t koutfile_set_(CSOUND *csound, KOUTFILE *p, int32_t istring)
{
  SFLIB_INFO sfinfo;
  int32_t     format_, n, buf_reqd;

  memset(&sfinfo, 0, sizeof(SFLIB_INFO));
  p->nargs = p->INOCOUNT - 2;
  p->buf_pos = 0;

  if (CS_KSMPS >= 512)
    p->guard_pos = CS_KSMPS * p->nargs;
  else
    p->guard_pos = 512 * p->nargs;

  sfinfo.channels = p->nargs;
  sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_EKR);
  format_ = (int32_t) MYFLT2LRND(*p->iflag);
  if ((uint32_t) format_ >= 10ul)
    sfinfo.format = AE_SHORT | TYP2SF(TYP_RAW);
  else
    sfinfo.format = fout_format_table[format_] | TYP2SF(TYP_RAW);

  if (CS_KSMPS >= 512)
    buf_reqd = CS_KSMPS *  p->nargs;
  else
    buf_reqd = (1 + (int32_t)(512 / CS_KSMPS)) * CS_KSMPS * p->nargs;
  if (p->buf.auxp == NULL || p->buf.size < buf_reqd*sizeof(MYFLT)) {
    csound->AuxAlloc(csound, sizeof(MYFLT)*buf_reqd, &p->buf);
  }
  p->f.bufsize =(int32_t)  p->buf.size;
  if(fout_open_file(csound, &(p->f), NULL, CSFILE_SND_W,
                    p->fname, istring, &sfinfo, 0, NULL) != NULL)
    n = p->f.idx - 1;
  else return NOTOK;

  if (((STDOPCOD_GLOBALS*) csound->QueryGlobalVariable(csound,"STDOPC_GLOBALS"))->file_opened[n].do_scale)
    p->scaleFac = (FL(1.)/csound->Get0dBFS(csound));
  else
    p->scaleFac = FL(1.0);
  return OK;
}

static int32_t koutfile_set(CSOUND *csound, KOUTFILE *p){
  return koutfile_set_(csound,p,0);
}

static int32_t koutfile_set_S(CSOUND *csound, KOUTFILE *p){
  return koutfile_set_(csound,p,1);
}

/* syntax:
   ihandle fiopen "filename" [, iascii]
*/
/* open a file and return its handle  */
/* the handle is simply a stack index */

static int32_t fiopen_(CSOUND *csound, FIOPEN *p, int32_t istring)
{
  char    *omodes[] = {"w", "r", "wb", "rb"};
  int32_t     idx = (int32_t) MYFLT2LRND(*p->iascii), handle;
    
  if (idx < 0 || idx > 3)
    idx = 0;
  /* A fiopen handle lasts until ficlose or the end of performance; it is
     not a borrower tied to the lifetime of this opcode instance. */
  fout_open_file(csound, NULL, NULL, CSFILE_STD,
                 p->fname, istring, omodes[idx], 1, &handle);
  if (handle < 0)
    return NOTOK;
      
  *p->ihandle = (MYFLT) handle;

  return OK;
}

static int32_t fiopen(CSOUND *csound, FIOPEN *p){
  return fiopen_(csound, p, 0);
}

static int32_t fiopen_S(CSOUND *csound, FIOPEN *p){
  return fiopen_(csound, p, 1);
}

static int32_t ficlose_opcode_(CSOUND *csound, FICLOSE *p, int32_t istring)
{

  STDOPCOD_GLOBALS  *pp = (STDOPCOD_GLOBALS*) csound->QueryGlobalVariable(csound,"STDOPC_GLOBALS");
  int32_t               idx = -1;

  if (istring || IsStringCode(*(p->iFile))) {
    char    *fname = NULL;
    if (istring) fname = csound->Strdup(csound, ((STRINGDAT *)p->iFile)->data);
    else if (IsStringCode(*(p->iFile)))
      fname = csound->Strdup(csound, csound->GetArgString(csound, *p->iFile));
    if (UNLIKELY(fname == NULL || fname[0] == (char) 0)) {
      if (fname != NULL) csound->Free(csound, fname);
      return csound->InitError(csound, "%s", Str("invalid file name"));
    }
    for (idx = 0; idx <= pp->file_num; idx++) {
      if (pp->file_opened[idx].fd != NULL &&
          pp->file_opened[idx].name != (char*) NULL &&
          strcmp(fname, pp->file_opened[idx].name) == 0)
        break;
    }
    if (UNLIKELY(idx > pp->file_num)) {
      csound->Warning(csound, Str("cannot close '%s': "
                                  "not found in list of open files"), fname);
      csound->Free(csound, fname);
      return OK;
    }
    csound->Free(csound, fname);
  }
  else {
    idx = (int32_t) MYFLT2LRND(*(p->iFile));
    if (UNLIKELY(idx < 0 || idx > pp->file_num ||
                 pp->file_opened[idx].fd == NULL)) {
      csound->Warning(csound,
                      Str("cannot close file #%d: not a valid handle"), idx);
      return OK;
    }
  }
  if (pp->file_opened[idx].refCount) {
    if (UNLIKELY(!(pp->file_opened[idx].refCount & 0x80000000U))) {
      pp->file_opened[idx].refCount |= 0x80000000U;
      csound->Warning(csound, Str("file #%d (%s) is in use, will be closed "
                                  "when released"),
                      idx, pp->file_opened[idx].name);
    }
  }
  else {
    FOUT_FILE tmp;
    pp->file_opened[idx].refCount = 1;
    memset(&tmp, 0, sizeof(FOUT_FILE));
    tmp.h.insdshead = p->h.insdshead;
    tmp.idx = idx + 1;
    tmp.need_deinit = 1;
    fout_deinit(csound, (void*) &tmp);
  }

  return OK;
}

static int32_t ficlose_opcode(CSOUND *csound, FICLOSE *p){
  return ficlose_opcode_(csound,p,0);
}

static int32_t ficlose_opcode_S(CSOUND *csound, FICLOSE *p){
  return ficlose_opcode_(csound,p,1);
}

/* syntax:
   fouti  ihandle, iascii, iflag, iarg1 [, iarg2, ...., iargN]
*/
static int32_t ioutfile_set(CSOUND *csound, IOUTFILE *p)
{
  STDOPCOD_GLOBALS  *pp = (STDOPCOD_GLOBALS*) csound->QueryGlobalVariable(csound,"STDOPC_GLOBALS");
  MYFLT   **args = p->argums;
  FILE    *rfil;
  uint32_t j;
  int32_t     n = (int32_t) MYFLT2LRND(*p->ihandle);
  if (UNLIKELY(n < 0 || n > pp->file_num))
    return csound->InitError(csound, "%s", Str("fouti: invalid file handle"));
  rfil = pp->file_opened[n].raw;
  if (UNLIKELY(rfil == NULL))
    return csound->InitError(csound, "%s", Str("fouti: invalid file handle"));
  if (*p->iascii == 0) { /* ascii format */
    switch ((int32_t) MYFLT2LRND(*p->iflag)) {
    case 1:
      {     /* with prefix (i-statement, p1, p2 and p3) */
        int32_t     p1 = (int32_t) p->h.insdshead->p1.value;
        double  p2 = (double) CS_KCNT * CS_ONEDKR;
        double  p3 = p->h.insdshead->p3.value;
        if (p3 > FL(0.0))
          fprintf(rfil, "i %i %f %f ", p1, p2, p3);
        else
          fprintf(rfil, "i %i %f . ", p1, p2);
      }
      break;
    case 2: /* with prefix (start at 0 time) */
      if (pp->fout_kreset == 0)
        pp->fout_kreset = CS_KCNT;
      {
        int32_t p1 = (int32_t) p->h.insdshead->p1.value;
        double p2 = (double) (CS_KCNT - pp->fout_kreset)
          * CS_ONEDKR;
        double p3 = p->h.insdshead->p3.value;
        if (p3 > FL(0.0))
          fprintf(rfil, "i %i %f %f ", p1, p2, p3);
        else
          fprintf(rfil, "i %i %f . ", p1, p2);
      }
      break;
    case 3: /* reset */
      pp->fout_kreset = 0;
      return OK;
    }
    for (j = 0; j < p->INOCOUNT - 3; j++) {
      fprintf(rfil, " %f", (double) *args[j]);
    }
    putc('\n', rfil);
  }
  else { /* binary format */
    for (j = 0; j < p->INOCOUNT - 3; j++) {
      if (UNLIKELY(1!=fwrite(args[j], sizeof(MYFLT), 1, rfil))) return NOTOK;
    }
  }
  return OK;
}



static int32_t ioutfile_set_r(CSOUND *csound, IOUTFILE_R *p)
{
  STDOPCOD_GLOBALS  *pp = (STDOPCOD_GLOBALS*) csound->QueryGlobalVariable(csound,"STDOPC_GLOBALS");
  if (p->h.insdshead->xtratim < 1)
    p->h.insdshead->xtratim = 1;
  p->counter =  CS_KCNT;
  p->done = 1;
  if (*p->iflag == 2 && pp->fout_kreset == 0)
    pp->fout_kreset = CS_KCNT;
  return OK;
}

static int32_t ioutfile_r(CSOUND *csound, IOUTFILE_R *p)
{
  STDOPCOD_GLOBALS  *pp;
  MYFLT **args;
  FILE  *rfil;
  uint32_t   j;
  int32_t n;

  if (!p->h.insdshead->relesing || !p->done)
    return OK;

  pp = (STDOPCOD_GLOBALS*) csound->QueryGlobalVariable(csound,"STDOPC_GLOBALS");
  args = p->argums;
  n = (int32_t) MYFLT2LRND(*p->ihandle);
  if (UNLIKELY(n < 0 || n > pp->file_num))
    return csound->InitError(csound, "%s", Str("fouti: invalid file handle"));
  rfil = pp->file_opened[n].raw;
  if (UNLIKELY(rfil == NULL))
    return csound->InitError(csound, "%s", Str("fouti: invalid file handle"));
  if (*p->iascii == 0) { /* ascii format */
    switch ((int32_t) MYFLT2LRND(*p->iflag)) {
    case 1:
      {     /* whith prefix (i-statement, p1, p2 and p3) */
        int32_t p1 = (int32_t) p->h.insdshead->p1.value;
        double p2 = p->counter * CS_ONEDKR;
        double p3 = (double) (CS_KCNT - p->counter)
          * CS_ONEDKR;
        fprintf(rfil, "i %i %f %f ", p1, p2, p3);
      }
      break;
    case 2: /* with prefix (start at 0 time) */
      {
        int32_t p1 = (int32_t) p->h.insdshead->p1.value;
        double p2 = (p->counter - pp->fout_kreset) * CS_ONEDKR;
        double p3 = (double) (CS_KCNT - p->counter)
          * CS_ONEDKR;
        fprintf(rfil, "i %i %f %f ", p1, p2, p3);
      }
      break;
    case 3: /* reset */
      pp->fout_kreset = 0;
      return OK;
    }
    for (j = 0; j < p->INOCOUNT - 3; j++) {
      fprintf(rfil, " %f", (double) *args[j]);
    }
    putc('\n', rfil);
  }
  else { /* binary format */
    for (j = 0; j < p->INOCOUNT - 3; j++) {
      if (UNLIKELY(1!=fwrite(args[j], sizeof(MYFLT), 1, rfil))) return NOTOK;
    }
  }
  p->done = 0;

  return OK;
}

int32_t infile_deinit(CSOUND *csound, INFILE *p) {
  return fout_deinit(csound, &(p->f));
}

static int32_t infile_set_(CSOUND *csound, INFILE *p, int32_t istring)
{
  SFLIB_INFO sfinfo;
  int32_t     n;
  size_t      buf_reqd;
  p->nargs = p->INOCOUNT - 3;
  if (UNLIKELY(!(*p->iskpfrms >= FL(0.0) &&
                 (double)*p->iskpfrms <= INT32_MAX)))
    return csound->InitError(csound, "%s", Str("invalid frame skip"));
  p->currpos = MYFLT2LRND(*p->iskpfrms);
  p->flag = 1;
  memset(&sfinfo, 0, sizeof(SFLIB_INFO));
  sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_ESR);
  if ((int32_t) MYFLT2LRND(*p->iflag) == -2)
    sfinfo.format = FORMAT2SF(AE_FLOAT) | TYPE2SF(TYP_RAW);
  else if ((int32_t) MYFLT2LRND(*p->iflag) == -1)
    sfinfo.format = FORMAT2SF(AE_SHORT) | TYPE2SF(TYP_RAW);
  else
    sfinfo.format = 0;
  sfinfo.channels = p->INOCOUNT - 3;
  if (CS_KSMPS >= 512)
    p->frames = CS_KSMPS;
  else
    p->frames = (int32_t)(512 / CS_KSMPS) * CS_KSMPS;
  if (UNLIKELY(sfinfo.channels <= 0 ||
               sfinfo.channels > INT32_MAX / 4 / sizeof(MYFLT) / p->frames))
    return csound->InitError(csound, "%s", Str("invalid file channel count"));
  /* Keep the asynchronous queue's existing headroom. */
  p->f.bufsize = p->frames * sfinfo.channels * sizeof(MYFLT);
  if(fout_open_file(csound, &(p->f), NULL, CSFILE_SND_R,
                    p->fname, istring, &sfinfo, 0, NULL) != NULL) {
    n = p->f.idx - 1;
  }
  else return NOTOK;

  if (UNLIKELY(p->f.nchnls != p->nargs)) {
    fout_deinit(csound, &p->f);
    return csound->InitError(csound, "%s",
                             Str("file channels do not match input arguments"));
  }
  buf_reqd = (size_t)p->frames * p->f.nchnls * sizeof(MYFLT);
  if (p->buf.auxp == NULL || p->buf.size < buf_reqd)
    csound->AuxAlloc(csound, buf_reqd, &p->buf);

  if (((STDOPCOD_GLOBALS*)
       csound->QueryGlobalVariable(csound,"STDOPC_GLOBALS"))
      ->file_opened[n].do_scale)
    p->scaleFac = csound->Get0dBFS(csound);
  else
    p->scaleFac = FL(1.0);
    
  p->guard_pos = p->buf_pos = p->remain = 0;

  if (p->f.async == 1 &&
      csound->FSeekAsync(csound, p->f.fd, (int32_t)p->currpos, SEEK_SET) < 0)
    p->flag = 0;

  return OK;
}

static int32_t infile_set(CSOUND *csound, INFILE *p){
  return infile_set_(csound,p,0);
}

static int32_t infile_set_S(CSOUND *csound, INFILE *p){
  return infile_set_(csound,p,1);
}

#include "arrays.h"
int32_t infilea_deinit(CSOUND *csound, INFILEA *p) {
  return fout_deinit(csound, &(p->f));
}
static int32_t infile_set_A(CSOUND *csound, INFILEA *p)
{

  SFLIB_INFO sfinfo;
  int32_t     n;
  size_t      buf_reqd;
  if (UNLIKELY(!(*p->iskpfrms >= FL(0.0) &&
                 (double)*p->iskpfrms <= INT32_MAX)))
    return csound->InitError(csound, "%s", Str("invalid frame skip"));
  p->currpos = MYFLT2LRND(*p->iskpfrms);
  p->flag = 1;
  memset(&sfinfo, 0, sizeof(SFLIB_INFO));
  sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_ESR);
  if ((int32_t) MYFLT2LRND(*p->iflag) == -2)
    sfinfo.format = FORMAT2SF(AE_FLOAT) | TYPE2SF(TYP_RAW);
  else if ((int32_t) MYFLT2LRND(*p->iflag) == -1)
    sfinfo.format = FORMAT2SF(AE_SHORT) | TYPE2SF(TYP_RAW);
  else
    sfinfo.format = 0;
  sfinfo.channels = (p->tabout->dimensions == 1 && p->tabout->sizes != NULL
                     ? p->tabout->sizes[0] : 1);
  if (CS_KSMPS >= 512)
    p->frames = CS_KSMPS;
  else
    p->frames = (int32_t)(512 / CS_KSMPS) * CS_KSMPS;
  if (UNLIKELY(sfinfo.channels <= 0 ||
               sfinfo.channels > INT32_MAX / 4 / sizeof(MYFLT) / p->frames))
    return csound->InitError(csound, "%s", Str("invalid file channel count"));
  /* Keep the asynchronous queue's existing headroom. */
  p->f.bufsize = p->frames * sfinfo.channels * sizeof(MYFLT);
  if(fout_open_file(csound, &(p->f), NULL, CSFILE_SND_R,
                    p->fname, 1, &sfinfo, 0, NULL) != NULL) {
    n = p->f.idx - 1;
  }
  else return NOTOK;

  p->chn = p->f.nchnls;
  if (UNLIKELY(p->f.nchnls <= 0 ||
               p->f.nchnls > INT32_MAX / p->frames)) {
    fout_deinit(csound, &p->f);
    return csound->InitError(csound, "%s", Str("invalid file channel count"));
  }
  buf_reqd = (size_t)p->frames * p->f.nchnls * sizeof(MYFLT);
  if (p->buf.auxp == NULL || p->buf.size < buf_reqd)
    csound->AuxAlloc(csound, buf_reqd, &p->buf);

  if (((STDOPCOD_GLOBALS*)
       csound->QueryGlobalVariable(csound,"STDOPC_GLOBALS"))
      ->file_opened[n].do_scale)
    p->scaleFac = csound->Get0dBFS(csound);
  else
    p->scaleFac = FL(1.0);

  p->guard_pos = p->buf_pos = p->remain = 0;
  if (p->f.async == 1 &&
      csound->FSeekAsync(csound, p->f.fd, (int32_t)p->currpos, SEEK_SET) < 0)
    p->flag = 0;

  if (UNLIKELY(tabinit(csound, p->tabout, p->chn,
                       p->h.insdshead) != OK)) {
    fout_deinit(csound, &p->f);
    return csound_array_init_resize_error(csound);
  }
  return OK;
}

/* Refill only between copy runs. SndfileRead returns frames, ReadAsync
   returns samples. Keep any incomplete asynchronous frame for the next read. */
#define INFILE_REFILL(p, buf) do {                                      \
    if ((p)->f.async == 0) {                                            \
      int64_t got = csound->SndfileSeek(csound, (p)->f.sf,                \
                                       (p)->currpos, SEEK_SET);         \
      if (got >= 0)                                                    \
        got = csound->SndfileRead(csound, (p)->f.sf, (buf), (p)->frames);  \
      (p)->remain = got > 0 ? (uint32_t)got : 0;                         \
      (p)->currpos += (p)->remain;                                      \
      (p)->guard_pos = (p)->remain * (p)->f.nchnls;                       \
      if ((p)->remain == 0) (p)->flag = 0;                               \
    } else {                                                           \
      int32_t tail = (p)->guard_pos - (p)->buf_pos;                       \
      for (int32_t t = 0; t < tail; t++)                                \
        (buf)[t] = (buf)[(p)->buf_pos + t];                              \
      (p)->guard_pos = tail + csound->ReadAsync(csound, (p)->f.fd,        \
                           (buf) + tail, (p)->frames * (p)->f.nchnls     \
                                         - tail);                      \
      (p)->remain = (p)->guard_pos / (p)->f.nchnls;                       \
    }                                                                  \
    (p)->buf_pos = 0;                                                   \
  } while (0)

static int32_t infile_act(CSOUND *csound, INFILE *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS - early;
  uint32_t i, j = offset, channels = p->nargs;
  MYFLT *buf = (MYFLT *)p->buf.auxp;

  for (i = 0; i < channels; i++) {
    if (UNLIKELY(offset))
      memset(&p->argums[i][0], 0, offset * sizeof(MYFLT));
    if (UNLIKELY(early))
      memset(&p->argums[i][nsmps], 0, early * sizeof(MYFLT));
  }
  while (j < nsmps && p->flag) {
    uint32_t count, stop, k;
    if (p->remain == 0)
      INFILE_REFILL(p, buf);
    if (p->remain == 0)
      break;
    count = nsmps - j;
    if (count > p->remain)
      count = p->remain;
    stop = j + count;
    k = p->buf_pos;
    for (; j < stop; j++)
      for (i = 0; i < channels; i++)
        p->argums[i][j] = buf[k++] * p->scaleFac;
    p->buf_pos = k;
    p->remain -= count;
  }
  for (i = 0; i < channels; i++)
    if (j < nsmps)
      memset(&p->argums[i][j], 0, (nsmps - j) * sizeof(MYFLT));
  return OK;
}

static int32_t infile_arr(CSOUND *csound, INFILEA *p)
{
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  uint32_t nsmps = CS_KSMPS - early;
  uint32_t i, j = offset, channels = p->chn;
  MYFLT *buf = (MYFLT *)p->buf.auxp;
  MYFLT *data = p->tabout->data;
  size_t stride = p->tabout->arrayMemberSize / sizeof(MYFLT);

  for (i = 0; i < channels; i++) {
    if (UNLIKELY(offset))
      memset(&data[i * stride + 0], 0, offset * sizeof(MYFLT));
    if (UNLIKELY(early))
      memset(&data[i * stride + nsmps], 0, early * sizeof(MYFLT));
  }
  while (j < nsmps && p->flag) {
    uint32_t count, stop, k;
    if (p->remain == 0)
      INFILE_REFILL(p, buf);
    if (p->remain == 0)
      break;
    count = nsmps - j;
    if (count > p->remain)
      count = p->remain;
    stop = j + count;
    k = p->buf_pos;
    for (; j < stop; j++)
      for (i = 0; i < channels; i++)
        data[i * stride + j] = buf[k++] * p->scaleFac;
    p->buf_pos = k;
    p->remain -= count;
  }
  for (i = 0; i < channels; i++)
    if (j < nsmps)
      memset(&data[i * stride + j], 0, (nsmps - j) * sizeof(MYFLT));
  return OK;
}

/* ---------------------------- */
int32_t kinfile_deinit(CSOUND *csound, KINFILE *p) {
  return fout_deinit(csound, &(p->f));
}

static int32_t kinfile_set_(CSOUND *csound, KINFILE *p, int32_t istring)
{
  SFLIB_INFO sfinfo;
  int32_t     n;
  size_t      buf_reqd;

  memset(&sfinfo, 0, sizeof(SFLIB_INFO));
  sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_EKR);
  if ((int32_t) MYFLT2LRND(*p->iflag) == -2)
    sfinfo.format = FORMAT2SF(AE_FLOAT) | TYPE2SF(TYP_RAW);
  else if ((int32_t) MYFLT2LRND(*p->iflag) == -1)
    sfinfo.format = FORMAT2SF(AE_SHORT) | TYPE2SF(TYP_RAW);
  else
    sfinfo.format = 0;
  sfinfo.channels = p->INOCOUNT - 3;

  p->nargs = p->INOCOUNT - 3;
  if (UNLIKELY(!(*p->iskpfrms >= FL(0.0) &&
                 (double)*p->iskpfrms <= INT32_MAX)))
    return csound->InitError(csound, "%s", Str("invalid frame skip"));
  p->currpos = MYFLT2LRND(*p->iskpfrms);
  p->flag = 1;

  if (CS_KSMPS >= 512)
    p->frames = CS_KSMPS;
  else
    p->frames = (int32_t)(512 / CS_KSMPS) * CS_KSMPS;

  if (UNLIKELY(sfinfo.channels <= 0 ||
               sfinfo.channels > INT32_MAX / 4 / sizeof(MYFLT) / p->frames))
    return csound->InitError(csound, "%s", Str("invalid file channel count"));
  /* Keep the asynchronous queue's existing headroom. */
  p->f.bufsize = p->frames * sfinfo.channels * sizeof(MYFLT);

  if(fout_open_file(csound, &(p->f), NULL, CSFILE_SND_R,
                    p->fname, istring, &sfinfo, 0, NULL) != NULL) {
    n = p->f.idx - 1;
  }
  else return NOTOK;

  if (UNLIKELY(p->f.nchnls != p->nargs)) {
    fout_deinit(csound, &p->f);
    return csound->InitError(csound, "%s",
                             Str("file channels do not match input arguments"));
  }
  buf_reqd = (size_t)p->frames * p->f.nchnls * sizeof(MYFLT);
  if (p->buf.auxp == NULL || p->buf.size < buf_reqd)
    csound->AuxAlloc(csound, buf_reqd, &p->buf);


  if (((STDOPCOD_GLOBALS*)
       csound->QueryGlobalVariable(csound,"STDOPC_GLOBALS"))
      ->file_opened[n].do_scale)
    p->scaleFac = csound->Get0dBFS(csound);
  else
    p->scaleFac = FL(1.0);

  p->guard_pos = p->buf_pos = p->remain = 0;

  if (p->f.async == 1 &&
      csound->FSeekAsync(csound, p->f.fd, (int32_t)p->currpos, SEEK_SET) < 0)
    p->flag = 0;

  return OK;
}

static int32_t kinfile_set(CSOUND *csound, KINFILE *p){
  return kinfile_set_(csound,p,0);
}

static int32_t kinfile_set_S(CSOUND *csound, KINFILE *p){
  return kinfile_set_(csound,p,1);
}


static int32_t kinfile(CSOUND *csound, KINFILE *p)
{
  int32_t i;
  MYFLT *buf = (MYFLT *)p->buf.auxp;

  if (p->flag && p->remain == 0)
    INFILE_REFILL(p, buf);
  if (p->remain > 0) {
    for (i = 0; i < p->nargs; i++)
      *p->argums[i] = buf[p->buf_pos++] * p->scaleFac;
    p->remain--;
  } else {
    for (i = 0; i < p->nargs; i++)
      *p->argums[i] = FL(0.0);
  }
  return OK;
}

#undef INFILE_REFILL

/* Return one for a value, zero for EOF, or an init error. */
static int32_t fini_read_text(CSOUND *csound, FILE *fp, MYFLT *value)
{
  char token[256], *start, *end;
  size_t len;
  int32_t c;
  double number;

  do {
    do {
      c = getc(fp);
    } while (c != EOF && isspace(c));
    if (c == EOF)
      return 0;
    len = 0;
    do {
      if (len == sizeof(token) - 1)
        return csound->InitError(csound, Str("fini: numeric token too long"));
      token[len++] = (char)c;
      c = getc(fp);
    } while (c != EOF && !isspace(c));
    token[len] = '\0';
    /* Accept the i-statement prefix written by fouti/foutir. */
  } while (strcmp(token, "i") == 0);
  start = token;
  if (token[0] == 'i' && isdigit((unsigned char)token[1]))
    start++;
  number = csound->Strtod(start, &end);
  if (end == start || *end != '\0')
    return csound->InitError(csound, Str("fini: invalid numeric data"));
  *value = (MYFLT)number;
  return 1;
}

static int32_t i_infile_(CSOUND *csound, I_INFILE *p, int32_t istring)
{
  int32_t j, nargs = p->INOCOUNT - 3, format;
  FILE *fp = NULL;
  MYFLT **args = p->argums;
  const char *omodes[] = {"r", "r", "rb"};
  double skip = floor((double)*p->iskpfrms);

  if (*p->iflag != FL(0.0) && *p->iflag != FL(1.0) &&
      *p->iflag != FL(2.0))
    return csound->InitError(csound, Str("fini: format must be 0, 1 or 2"));
  if (!(skip >= 0.0 && skip <= INT32_MAX))
    return csound->InitError(csound, Str("fini: invalid skip frame count"));
  format = (int32_t)*p->iflag;
  /* The stream is shared across init calls; it has no per-note borrower. */
  fout_open_file(csound, NULL, &fp, CSFILE_STD, p->fname, istring,
                 (void*)omodes[format], 0, NULL);
  if (fp == NULL)
    return NOTOK;

  for (j = 0; j < nargs; j++)
    *args[j] = FL(0.0);

  /* Zero keeps the shared stream position. A positive skip selects a
     starting frame relative to the beginning of the file. */
  if (skip > 0.0) {
    if (format == 2) {
      uint64_t bytes = (uint64_t)skip * (uint64_t)nargs * sizeof(float);
      if (bytes > LONG_MAX || fseek(fp, (long)bytes, SEEK_SET) != 0)
        return csound->InitError(csound, Str("fini: cannot seek to frame"));
    }
    else {
      int64_t count = (int64_t)skip * nargs;
      MYFLT ignored;
      if (fseek(fp, 0, SEEK_SET) != 0)
        return csound->InitError(csound, Str("fini: cannot seek to frame"));
      while (count-- > 0) {
        int32_t status = fini_read_text(csound, fp, &ignored);
        if (status < 0)
          return NOTOK;
        if (status == 0)
          break;
      }
    }
  }

  for (j = 0; j < nargs; j++) {
    if (format == 2) {
      float value;
      if (fread(&value, sizeof(value), 1, fp) != 1)
        break;
      *args[j] = (MYFLT)value;
    }
    else {
      int32_t status = fini_read_text(csound, fp, args[j]);
      if (status == 0 && format == 0 && !ferror(fp)) {
        if (fseek(fp, 0, SEEK_SET) != 0)
          return csound->InitError(csound, Str("fini: cannot rewind file"));
        /* Retry once: empty input must not loop forever. */
        status = fini_read_text(csound, fp, args[j]);
      }
      if (status < 0)
        return NOTOK;
      if (status == 0)
        break;
    }
  }
  if (ferror(fp))
    return csound->InitError(csound, Str("fini: file read failed"));
  return OK;
}

static int32_t i_infile(CSOUND *csound, I_INFILE *p){
  return i_infile_(csound,p,0);
}

static int32_t i_infile_S(CSOUND *csound, I_INFILE *p){
  return i_infile_(csound,p,1);
}

static int32_t incr(CSOUND *csound, INCR *p)
{
  IGN(csound);
  MYFLT *avar = p->avar, *aincr = p->aincr;
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early  = p->h.insdshead->ksmps_no_end;
  uint32_t n, nsmps = CS_KSMPS;

  if (UNLIKELY(early)) nsmps -= early;
  for (n = offset; n < nsmps; n++)
    avar[n] += aincr[n];
  return OK;
}

static int32_t clear(CSOUND *csound, CLEARS *p)
{
  IGN(csound);
  uint32_t   nsmps = CS_KSMPS, j;

  for (j = 0; j < p->INOCOUNT; j++) {
    memset(p->argums[j], 0, sizeof(MYFLT)*nsmps);
  }
  return OK;
}

int32_t fprintf_deinit(CSOUND *csound, FPRINTF *p) {
  return fout_deinit(csound, &(p->f));
}

static int32_t fprintf_set_(CSOUND *csound, FPRINTF *p, int32_t istring)
{
  char    *sarg = (char*) p->fmt->data;
  char    *sdest = p->txtstring;
  FOUT_FILE* pp;

  if (p->h.perf != (SUBR) NULL) {     /* fprintks */
    pp = fout_open_file(csound, &(p->f), NULL, CSFILE_STD,
                        p->fname, istring, "w", 1, NULL);
    if (UNLIKELY(pp == NULL)) return NOTOK;
  } else {                             /* fprints */
    fout_open_file(csound, (FOUT_FILE*) NULL, &(p->f.f), CSFILE_STD,
                   p->fname, istring, "w", 1, NULL);
    if (UNLIKELY(p->f.f  == NULL)) return NOTOK;
  }
  /* Expand the legacy escape codes, leaving room for the terminator. */
  while (*sarg) {
    char temp = sarg[0];
    char tempn = sarg[1];
    size_t needed = ((temp == '~' && tempn != '~') ||
                     (temp == '%' && tempn == '%')) ? 2 : 1;
    if ((size_t)(sdest - p->txtstring) + needed >= sizeof(p->txtstring))
      return csound->InitError(csound,
                              Str("expanded format exceeds 8192 characters"));
    /* Look for a single caret and insert an escape char.  */
    if ((temp  == '^') && (tempn != '^')) {
      *sdest++ = 0x1B; /* ESC */
    }
    /* Look for a double caret and insert a single caret
       - stepping forward one */
    else if ((temp  == '^') && (tempn == '^')) {
      *sdest++ = '^';
      sarg++;
    }
    /* Look for a single tilde and insert an escape followed by a '['.
     * ESC[ is the escape sequence for ANSI consoles */
    else if ((temp  == '~') && (tempn != '~')) {
      *sdest++ = 0x1B; /* ESC */
      *sdest++ = '[';
    }
    /* Look for a double tilde and insert a tilde caret
       - stepping forward one. */
    else if ((temp  == '~') && (tempn == '~')) {
      *sdest++ = '~';
      sarg++;
    }
    /* Look for \n, \N etc */
    else if (temp == '\\' && tempn != '\0') {
      switch (tempn) {
      case 'r': case 'R':
        *sdest++ = '\r';
        sarg++;
        break;
      case 'n': case 'N':
        *sdest++ = '\n';
        sarg++;
        break;
      case 't': case 'T':
        *sdest++ = '\t';
        sarg++;
        break;
      case 'a': case 'A':
        *sdest++ = '\a';
        sarg++;
        break;
      case 'b': case 'B':
        *sdest++ = '\b';
        sarg++;
        break;
      case '\\':
        *sdest++ = '\\';
        sarg++;
        break;
      default:
        *sdest++ = tempn;
        sarg++;
        break;
      }
    }
    else if (temp == '%') {
      /* an extra option to specify tab and return as %t and %r */
      switch (tempn) {
      case '%':
        *sdest++ = '%';
        *sdest++ = '%';
        sarg++;
        break;
      case 'r': case 'R':
        *sdest++ = '\r';
        sarg++;
        break;
      case 'n': case 'N':
        *sdest++ = '\n';
        sarg++;
        break;
      case 't': case 'T':
        *sdest++ = '\t';
        sarg++;
        break;
      case '!':       /* and a ';' */
        *sdest++ = ';';
        sarg++;
        break;
      default:
        *sdest++ = temp;
        break;
      }
    }
    else {
      /* If none of these match, then copy the character directly
       * and try again.      */
      *sdest++ = temp;
    }
    /* Increment pointer and process next character until end of string.  */
    sarg++;
  }
  *sdest = '\0';
  return OK;
}

static int32_t fprintf_set(CSOUND *csound, FPRINTF *p){
  return fprintf_set_(csound,p,0);
}

static int32_t fprintf_set_S(CSOUND *csound, FPRINTF *p){
  return fprintf_set_(csound,p,1);
}
/* Format one conversion at a time, checking its argument and output size. */
static const char *fprints_format(CSOUND *csound, FPRINTF *p,
                                 char *out, size_t size)
{
  const char *fmt = p->txtstring;
  size_t used = 0;
  int32_t arg = 0;
  char spec[sizeof(p->txtstring)];

  out[0] = '\0';
  while (*fmt) {
    const char *start;
    MYFLT *value;
    int32_t length = 0, n;
    char conversion;

    if (*fmt != '%' || fmt[1] == '%') {
      if (used == size - 1)
        return Str("formatted output exceeds 8192 characters");
      out[used++] = *fmt++;
      if (fmt[-1] == '%')
        fmt++;
      out[used] = '\0';
      continue;
    }
    start = fmt++;
    while (*fmt && strchr("-+ #0", *fmt))
      fmt++;
    while (isdigit((unsigned char)*fmt))
      fmt++;
    if (*fmt == '.') {
      fmt++;
      while (isdigit((unsigned char)*fmt))
        fmt++;
    }
    if (*fmt == 'h') {
      length = -1;
      if (*++fmt == 'h') {
        length = -2;
        fmt++;
      }
    }
    else if (*fmt == 'l') {
      length = 1;
      if (*++fmt == 'l') {
        length = 2;
        fmt++;
      }
    }
    conversion = *fmt;
    if (!conversion || !strchr("diouxXcseEfFgGaA", conversion))
      return Str("invalid format conversion");
    if ((strchr("cs", conversion) && length != 0) ||
        (strchr("eEfFgGaA", conversion) && length != 0 && length != 1))
      return Str("invalid format length modifier");
    fmt++;
    memcpy(spec, start, (size_t)(fmt - start));
    spec[fmt - start] = '\0';

    if (arg >= p->INOCOUNT - 2)
      return Str("insufficient arguments for format");
    value = p->argums[arg++];
    if (conversion == 's') {
      if (!IS_STR_ARG(value))
        return Str("string argument required for %s");
      n = snprintf(out + used, size - used, spec, ((STRINGDAT*)value)->data);
    }
    else {
      if (IS_STR_ARG(value) || IS_ASIG_ARG(value))
        return Str("scalar numeric argument required for format");
      if (strchr("dic", conversion)) {
        if (length == 2)
          n = snprintf(out + used, size - used, spec,
                       (long long)MYFLT2LRND(*value));
        else if (length == 1)
          n = snprintf(out + used, size - used, spec,
                       (long)MYFLT2LRND(*value));
        else
          n = snprintf(out + used, size - used, spec,
                       (int)MYFLT2LRND(*value));
      }
      else if (strchr("ouxX", conversion)) {
        if (length == 2)
          n = snprintf(out + used, size - used, spec,
                       (unsigned long long)MYFLT2LRND(*value));
        else if (length == 1)
          n = snprintf(out + used, size - used, spec,
                       (unsigned long)MYFLT2LRND(*value));
        else
          n = snprintf(out + used, size - used, spec,
                       (unsigned int)MYFLT2LRND(*value));
      }
      else
        n = snprintf(out + used, size - used, spec, (double)*value);
    }
    if (n < 0)
      return Str("format conversion failed");
    if ((size_t)n >= size - used)
      return Str("formatted output exceeds 8192 characters");
    used += (size_t)n;
  }
  return NULL;
}

static int32_t fprintf_output(CSOUND *csound, FPRINTF *p, int32_t init)
{
  char string[sizeof(p->txtstring)];
  const char *error = fprints_format(csound, p, string, sizeof(string));

  if (error == NULL &&
      (fprintf(p->f.f, "%s", string) < 0 || fflush(p->f.f) != 0))
    error = Str("file write failed");
  if (error != NULL) {
    if (init)
      return csound->InitError(csound, "%s", error);
    return csound->PerfError(csound, &p->h, "%s", error);
  }
  return OK;
}

static int32_t fprintf_k(CSOUND *csound, FPRINTF *p)
{
  return fprintf_output(csound, p, 0);
}

/* i-rate fprints */
static int32_t fprintf_i(CSOUND *csound, FPRINTF *p)
{
  if (UNLIKELY(fprintf_set(csound, p) != OK))
    return NOTOK;
  return fprintf_output(csound, p, 1);
}

static int32_t fprintf_i_S(CSOUND *csound, FPRINTF *p)
{
  if (UNLIKELY(fprintf_set_S(csound, p) != OK))
    return NOTOK;
  return fprintf_output(csound, p, 1);
}

#define S(x)    sizeof(x)
static OENTRY localops[] = {
  {"fprints",    S(FPRINTF),      0,  "",     "SSN",
   (SUBR) fprintf_i_S, (SUBR) NULL,(SUBR) fprintf_deinit, NULL, },
  {"fprints.i",    S(FPRINTF),      0,  "",     "iSN",
   (SUBR) fprintf_i, (SUBR) NULL,(SUBR) fprintf_deinit, NULL},
  { "fprintks",   S(FPRINTF),    WR,  "",     "SSN",
    (SUBR) fprintf_set_S,     (SUBR) fprintf_k,   (SUBR) fprintf_deinit, NULL,},
  { "fprintks.i",   S(FPRINTF),    WR,  "",     "iSN",
    (SUBR) fprintf_set,     (SUBR) fprintf_k,   (SUBR) fprintf_deinit, NULL},
  { "vincr",      S(INCR),       WI,   "",     "aa",
    (SUBR) NULL,            (SUBR) incr, NULL         },
  { "clear",      S(CLEARS),      WI,   "",     "y",
    (SUBR) NULL,            (SUBR) clear, NULL},
  { "fout",       S(OUTFILE),     0,  "",     "Siy",
    (SUBR) outfile_set_S,     (SUBR) outfile, (SUBR) fout_flush_callback},
  { "fout.A",     S(OUTFILEA),    0,  "",     "Sia[]",
    (SUBR) outfile_set_A,     (SUBR) outfile_array, (SUBR) fouta_flush_callback},
  { "foutk",      S(KOUTFILE),    0,  "",     "Siz",
    (SUBR) koutfile_set_S,    (SUBR) koutfile,    (SUBR) koutfile_deinit, NULL },
  { "foutk.i",      S(KOUTFILE),    0,  "",     "iiz",
    (SUBR) koutfile_set,    (SUBR) koutfile,    (SUBR) koutfile_deinit, NULL },
  { "fouti",      S(IOUTFILE),    0,  "",     "iiim",
    (SUBR) ioutfile_set,    (SUBR) NULL,        (SUBR) NULL, NULL         },
  { "foutir",     S(IOUTFILE_R),  0,  "",     "iiim",
    (SUBR) ioutfile_set_r,  (SUBR) ioutfile_r,  (SUBR) NULL, NULL},
  { "fiopen",     S(FIOPEN),      0,  "i",    "Si",
    (SUBR) fiopen_S,          (SUBR) NULL,        (SUBR) NULL, NULL},
  { "fiopen.i",     S(FIOPEN),      0,  "i",    "ii",
    (SUBR) fiopen,          (SUBR) NULL,        (SUBR) NULL, NULL},
  { "ficlose",    S(FICLOSE),     0,  "",     "S",
    (SUBR) ficlose_opcode_S,  (SUBR) NULL,        (SUBR) NULL, NULL},
  { "ficlose.S",  S(FICLOSE),     0,  "",     "i",
    (SUBR) ficlose_opcode,  (SUBR) NULL,        (SUBR) NULL, NULL },
  CSOUND_DEPRECATED_OPCODE("fin", "diskin2", LEGACY, "Adapt the file-reading arguments and output assignment; not a drop-in rename.")
  { "fin.a",      S(INFILE),     WI|_QQ,  "",      "Siiy",
    (SUBR) infile_set_S,    (SUBR) infile_act, (SUBR) infile_deinit},
  { "fin.A",      S(INFILEA),    WI|_QQ,  "",     "Siia[]",
    (SUBR) infile_set_A,    (SUBR) infile_arr, (SUBR) infilea_deinit},
  { "fin.i",      S(INFILE),     WI|_QQ,  "",     "iiiy",
    (SUBR) infile_set,      (SUBR) infile_act, (SUBR) infile_deinit},
  { "fink",       S(KINFILE),    WI,  "",     "Siiz",
    (SUBR) kinfile_set_S,     (SUBR) kinfile,     (SUBR) kinfile_deinit, NULL},
  { "fink.i",       S(KINFILE),  WI,  "",     "iiiz",
    (SUBR) kinfile_set,     (SUBR) kinfile,     (SUBR)  kinfile_deinit, NULL},
  { "fini",       S(I_INFILE),   WI,  "",     "Siim",
    (SUBR) i_infile_S,        (SUBR) NULL,        (SUBR) NULL, NULL },
  { "fini.i",       S(I_INFILE), WI,  "",     "iiim",
    (SUBR) i_infile,        (SUBR) NULL,        (SUBR) NULL, NULL}
};

int32_t fout_init_(CSOUND *csound)
{
  return csound->AppendOpcodes(csound, &(localops[0]),
                               (int32_t) (sizeof(localops) / sizeof(OENTRY)));
}
