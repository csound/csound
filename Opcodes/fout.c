/*
    fout.c:

    Copyright (C) 1999 Gabriel Maldonado, John ffitch, Matt Ingalls
        (C) 2005, 2006 Istvan Varga

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

/* Opcodes By Gabriel Maldonado, 1999 */
/* Code modified by JPff to remove fixed size arrays, allow
   AIFF and WAV, and close files neatly.  Also bugs fixed */

#include "stdopcod.h"
#include <sndfile.h>
#include "fout.h"
#include "soundio.h"
#include <ctype.h>

/* remove a file reference, optionally closing the file */

static CS_NOINLINE int32_t fout_deinit_callback(CSOUND *csound, void *p_)
{
    FOUT_FILE         *p = (FOUT_FILE*) p_;
    struct fileinTag  *pp;
    p->sf = (SNDFILE*) NULL;
    p->f = (FILE*) NULL;
    if (p->idx) {
      pp = &(((STDOPCOD_GLOBALS*) csound->stdOp_Env)->file_opened[p->idx - 1]);
      p->idx = 0;
      if (pp->refCount) {
        pp->refCount--;
        /* VL 29/08/07: files were not being closed properly,
           changed check to 0 */
        if (pp->refCount == 0/*0x80000000U*/) {
          pp->file = (SNDFILE*) NULL;
          pp->raw = (FILE*) NULL;
          csound->Free(csound, pp->name);
          pp->name = (char*) NULL;
          pp->do_scale = 0;
          pp->refCount = 0U;

          if (pp->fd != NULL) {
            if ((csound->oparms->msglevel & 7) == 7)
              csound->Message(csound, Str("Closing file '%s'...\n"),
                                      csound->GetFileName(pp->fd));
            csound->FileClose(csound, pp->fd);
            pp->fd = NULL;
          }
        }
      }
    }

    return OK;
}

static CS_NOINLINE int32_t fout_open_file(CSOUND *csound, FOUT_FILE *p, void *fp,
                                          int32_t fileType, MYFLT *iFile,
                                          int32_t isString,
                                          void *fileParams, int32_t forceSync)
{
    STDOPCOD_GLOBALS  *pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    char              *name;
    int32_t               idx, csFileType, need_deinit = 0;

    if (p != (FOUT_FILE*) NULL) p->async = 0;
    if (fp != NULL) {
      if (fileType == CSFILE_STD)
        *((FILE**) fp) = (FILE*) NULL;
      else
        *((SNDFILE**) fp) = (SNDFILE*) NULL;
    }
    /* if the opcode already uses a file, remove old reference first */
    if (p != (FOUT_FILE*) NULL) {
      if (p->idx)
        fout_deinit_callback(csound, (void*) p);
      else
        need_deinit = 1;
    }
    /* get file name, */
    if (isString) name = cs_strdup(csound, ((STRINGDAT *)iFile)->data);
    else if (csound->ISSTRCOD(*iFile))
      name = cs_strdup(csound, get_arg_string(csound, *iFile));
    /* else csound->strarg2name(csound, NULL, iFile, "fout.", 0);*/
    else {
      /* or handle to previously opened file */
      idx = (int32_t) MYFLT2LRND(*iFile);
      if (UNLIKELY(idx < 0 || idx > pp->file_num ||
                   (fileType == CSFILE_STD && pp->file_opened[idx].raw == NULL) ||
                   (fileType != CSFILE_STD && pp->file_opened[idx].file == NULL))) {
        return csound->InitError(csound, Str("invalid file handle"));
      }
      goto returnHandle;
    }
    /* check for a valid name */
    if (UNLIKELY(name == NULL || name[0] == '\0')) {
      csound->Free(csound, name);
      return csound->InitError(csound, Str("invalid file name"));
    }
    /* is this file already open ? */
    if (fileType == CSFILE_STD) {
      for (idx = 0; idx <= pp->file_num; idx++) {
        if (pp->file_opened[idx].raw != (FILE*) NULL &&
            strcmp(pp->file_opened[idx].name, name) == 0)
          goto returnHandle;
      }
    }
    else {
      for (idx = 0; idx <= pp->file_num; idx++) {
        if (pp->file_opened[idx].file != (SNDFILE*) NULL &&
            strcmp(pp->file_opened[idx].name, name) == 0)
          goto returnHandle;
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
    /* pp->file_opened[idx].file = (SNDFILE*) NULL; */
    /* pp->file_opened[idx].raw = (FILE*) NULL; */
    /* pp->file_opened[idx].fd = NULL; */
    /* pp->file_opened[idx].name = (char*) NULL; */
    /* pp->file_opened[idx].do_scale = 0; */
    /* pp->file_opened[idx].refCount = 0U; */
    /* attempt to open file */
    if (fileType == CSFILE_STD) {
      FILE    *f;
      void    *fd;
      char    *filemode = (char*)fileParams;

      /* akozar: csFileType cannot be as specific as I'd like since it is not
         possible to know the real file type until this handle is used */
      if ((strcmp(filemode, "rb") == 0 || (strcmp(filemode, "wb") == 0)))
            csFileType = CSFTYPE_OTHER_BINARY;
      else  csFileType = CSFTYPE_OTHER_TEXT;
      fd = csound->FileOpen2(csound, &f, fileType, name, fileParams, "",
                               csFileType, 0);
      if (UNLIKELY(fd == NULL)) {
        csound->InitError(csound, Str("error opening file '%s'"), name);
        csound->Free(csound, name);
        return NOTOK;
      }
      /* setvbuf(f, (char *) NULL, _IOLBF, 0); */ /* Ensure line buffering */
      pp->file_opened[idx].raw = f;
      pp->file_opened[idx].fd = fd;
    }
    else {
      SNDFILE *sf;
      void    *fd;
      //int32_t     buf_reqd;
      int32_t     do_scale = 0;

      if (fileType == CSFILE_SND_W) {
        do_scale = ((SF_INFO*) fileParams)->format;
        csFileType = csound->sftype2csfiletype(do_scale);
        if (csound->oparms->realtime == 0 || forceSync == 1) {
          fd = csound->FileOpen2(csound, &sf, fileType, name, fileParams,
                                "SFDIR", csFileType, 0);
          p->async = 0;
        }
        else {
          p->fd = fd = csound->FileOpenAsync(csound, &sf, fileType,
                                             name, fileParams,
                                             "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO,
                                             p->bufsize,  0);
          p->async = 1;
        }
        p->nchnls = ((SF_INFO*) fileParams)->channels;
      }
      else {
        if (csound->oparms->realtime == 0 || forceSync == 1) {
          fd = csound->FileOpen2(csound, &sf, fileType, name, fileParams,
                                 "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
          p->async = 0;

        } else {
          p->fd = fd = csound->FileOpenAsync(csound, &sf, fileType,
                                             name, fileParams,
                                             "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO,
                                             p->bufsize,  0);
          p->async = 1;
        }
        p->nchnls = ((SF_INFO*) fileParams)->channels;
        do_scale = ((SF_INFO*) fileParams)->format;
      }
      do_scale = (SF2TYPE(do_scale) == TYP_RAW ? 0 : 1);
      if (UNLIKELY(fd == NULL)) {
        csound->InitError(csound, Str("error opening sound file '%s'"), name);
        csound->Free(csound, name);
        return NOTOK;
      }
      if (!do_scale) {
#ifdef USE_DOUBLE
        sf_command(sf, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE);
#else
        sf_command(sf, SFC_SET_NORM_FLOAT, NULL, SF_FALSE);
#endif
      }
      /* if (CS_KSMPS >= 512)
        buf_reqd = CS_KSMPS * ((SF_INFO*) fileParams)->channels;
      else
        buf_reqd = (1 + (int32_t)(512 / CS_KSMPS)) * CS_KSMPS
                   * ((SF_INFO*) fileParams)->channels;
      if (UNLIKELY(buf_reqd > pp->buf_size)) {
        pp->buf_size = buf_reqd;
        pp->buf = (MYFLT*) csound->ReAlloc(csound, pp->buf, sizeof(MYFLT)
                                                            * buf_reqd);
      }
      */ /* VL - now using per-instance buffer */
      pp->file_opened[idx].file = sf;
      pp->file_opened[idx].fd = fd;
      pp->file_opened[idx].do_scale = do_scale;
    }
    /* store file information */
    pp->file_opened[idx].name = name;

 returnHandle:
    /* return 'idx' as file handle */
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
        p->f = (FILE*) NULL;
      }
      p->idx = idx + 1;
      pp->file_opened[idx].refCount++;
      if (need_deinit) {
        p->h.insdshead = csound->ids->insdshead;
        /* FIXME: should check for error here */
        csound->RegisterDeinitCallback(csound, p, fout_deinit_callback);
      }
    }
    return idx;
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

        //#ifndef USE_DOUBLE
        //sf_write_float(p->f.sf, buf, p->buf_pos);
        //#else
        //sf_write_double(p->f.sf, buf, p->buf_pos);
        //#endif
        if (p->f.async==1)
          csound->WriteAsync(csound, p->f.fd, buf, p->buf_pos);
        else sf_write_MYFLT(p->f.sf, buf, p->buf_pos);
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
    uint32_t nargs = p->tabin->sizes[0];
    MYFLT *buf = (MYFLT *) p->buf.auxp;
    MYFLT *data = p->tabin->data;

    if (UNLIKELY(early)) nsmps -= early;
    if (p->f.sf == NULL) {
      if (p->f.f != NULL) { /* VL: make sure there is an open file */
        FILE  *fp = p->f.f;
        for (k = offset; k < nsmps; k++) {
          for (j = 0; j < nargs; j++)
            fprintf(fp, "%g ", data[j*CS_KSMPS+k]);
          fprintf(fp, "\n");
        }
      }
    }
    else {
      for (j = offset, k = p->buf_pos; j < nsmps; j++)
        for (i = 0; i < nargs; i++)
          buf[k++] = data[i*CS_KSMPS+j] * p->scaleFac;
      p->buf_pos = k;
      if (p->buf_pos >= p->guard_pos) {

        //#ifndef USE_DOUBLE
        //sf_write_float(p->f.sf, buf, p->buf_pos);
        //#else
        //sf_write_double(p->f.sf, buf, p->buf_pos);
        //#endif
        if (p->f.async==1)
          csound->WriteAsync(csound, p->f.fd, buf, p->buf_pos);
        else
          sf_write_MYFLT(p->f.sf, buf, p->buf_pos);
        p->buf_pos = 0;
       }

    }
    return OK;
}

static const int32_t fout_format_table[51] = {
    /* 0 - 9 */
    (SF_FORMAT_FLOAT | SF_FORMAT_RAW), (SF_FORMAT_PCM_16 | SF_FORMAT_RAW),
    SF_FORMAT_PCM_16, SF_FORMAT_ULAW, SF_FORMAT_PCM_16, SF_FORMAT_PCM_32,
    SF_FORMAT_FLOAT, SF_FORMAT_PCM_U8, SF_FORMAT_PCM_24, SF_FORMAT_DOUBLE,
    /* 10 - 19 */
    SF_FORMAT_WAV, (SF_FORMAT_PCM_S8 | SF_FORMAT_WAV),
    (SF_FORMAT_ALAW | SF_FORMAT_WAV), (SF_FORMAT_ULAW | SF_FORMAT_WAV),
    (SF_FORMAT_PCM_16 | SF_FORMAT_WAV), (SF_FORMAT_PCM_32 | SF_FORMAT_WAV),
    (SF_FORMAT_FLOAT | SF_FORMAT_WAV), (SF_FORMAT_PCM_U8 | SF_FORMAT_WAV),
    (SF_FORMAT_PCM_24 | SF_FORMAT_WAV), (SF_FORMAT_DOUBLE | SF_FORMAT_WAV),
    /* 20 - 29 */
    SF_FORMAT_AIFF, (SF_FORMAT_PCM_S8 | SF_FORMAT_AIFF),
    (SF_FORMAT_ALAW | SF_FORMAT_AIFF), (SF_FORMAT_ULAW | SF_FORMAT_AIFF),
    (SF_FORMAT_PCM_16 | SF_FORMAT_AIFF), (SF_FORMAT_PCM_32 | SF_FORMAT_AIFF),
    (SF_FORMAT_FLOAT | SF_FORMAT_AIFF), (SF_FORMAT_PCM_U8 | SF_FORMAT_AIFF),
    (SF_FORMAT_PCM_24 | SF_FORMAT_AIFF), (SF_FORMAT_DOUBLE | SF_FORMAT_AIFF),
    /* 30 - 39 */
    SF_FORMAT_RAW, (SF_FORMAT_PCM_S8 | SF_FORMAT_RAW),
    (SF_FORMAT_ALAW | SF_FORMAT_RAW), (SF_FORMAT_ULAW | SF_FORMAT_RAW),
    (SF_FORMAT_PCM_16 | SF_FORMAT_RAW), (SF_FORMAT_PCM_32 | SF_FORMAT_RAW),
    (SF_FORMAT_FLOAT | SF_FORMAT_RAW), (SF_FORMAT_PCM_U8 | SF_FORMAT_RAW),
    (SF_FORMAT_PCM_24 | SF_FORMAT_RAW), (SF_FORMAT_DOUBLE | SF_FORMAT_RAW),
    /* 40 - 49 */
    SF_FORMAT_IRCAM, (SF_FORMAT_PCM_S8 | SF_FORMAT_IRCAM),
    (SF_FORMAT_ALAW | SF_FORMAT_IRCAM), (SF_FORMAT_ULAW | SF_FORMAT_IRCAM),
    (SF_FORMAT_PCM_16 | SF_FORMAT_IRCAM), (SF_FORMAT_PCM_32 | SF_FORMAT_IRCAM),
    (SF_FORMAT_FLOAT | SF_FORMAT_IRCAM), (SF_FORMAT_PCM_U8 | SF_FORMAT_IRCAM),
    (SF_FORMAT_PCM_24 | SF_FORMAT_IRCAM), (SF_FORMAT_DOUBLE | SF_FORMAT_IRCAM),
    /* 50 */
    (SF_FORMAT_OGG | SF_FORMAT_VORBIS)
};

static int32_t fout_flush_callback(CSOUND *csound, void *p_)
{
    OUTFILE            *p = (OUTFILE*) p_;

    if (p->f.sf != NULL && p->buf_pos > 0) {
      //#ifndef USE_DOUBLE
      //sf_write_float(p->f.sf, (float*) p->buf.auxp, p->buf_pos);
      //#else
      //sf_write_double(p->f.sf, (double*) p->buf.auxp, p->buf_pos);
      //#endif
      if (p->f.async == 1)
        csound->WriteAsync(csound, p->f.fd, (MYFLT *) p->buf.auxp, p->buf_pos);
      else
        sf_write_MYFLT(p->f.sf, (MYFLT *) p->buf.auxp, p->buf_pos);
    }
    return OK;
}

static int32_t fouta_flush_callback(CSOUND *csound, void *p_)
{
    OUTFILEA           *p = (OUTFILEA*) p_;

    if (p->f.sf != NULL && p->buf_pos > 0) {
      //#ifndef USE_DOUBLE
      //sf_write_float(p->f.sf, (float*) p->buf.auxp, p->buf_pos);
      //#else
      //sf_write_double(p->f.sf, (double*) p->buf.auxp, p->buf_pos);
      //#endif
      if (p->f.async == 1)
        csound->WriteAsync(csound, p->f.fd, (MYFLT *) p->buf.auxp, p->buf_pos);
      else
        sf_write_MYFLT(p->f.sf, (MYFLT *) p->buf.auxp, p->buf_pos);
    }
    return OK;
}

static int32_t outfile_set_S(CSOUND *csound, OUTFILE *p/*, int32_t istring*/)
{
    SF_INFO sfinfo;
    int32_t     format_, n, buf_reqd;
    int32_t istring = 1;

    memset(&sfinfo, 0, sizeof(SF_INFO));
    format_ = (int32_t) MYFLT2LRND(*p->iflag);
    if (format_ >= 51)
      sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
    else if (format_ < 0) {
      sfinfo.format = FORMAT2SF(csound->oparms->outformat);
      sfinfo.format |= TYPE2SF(csound->oparms->filetyp);
    }
    else sfinfo.format = fout_format_table[format_];
    if (!SF2FORMAT(sfinfo.format))
      sfinfo.format |= FORMAT2SF(csound->oparms->outformat);
    if (!SF2TYPE(sfinfo.format))
      sfinfo.format |= TYPE2SF(csound->oparms->filetyp);
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
    p->f.bufsize =  p->buf.size;
    sfinfo.channels = p->nargs;
    n = fout_open_file(csound, &(p->f), NULL, CSFILE_SND_W,
                       p->fname, istring, &sfinfo, 0);
    if (UNLIKELY(n < 0))
      return NOTOK;

    if (((STDOPCOD_GLOBALS*) csound->stdOp_Env)->file_opened[n].do_scale)
      p->scaleFac = csound->dbfs_to_float;
    else
      p->scaleFac = FL(1.0);

    csound->RegisterDeinitCallback(csound, p, fout_flush_callback);
    return OK;
}

/* static int32_t outfile_set(CSOUND *csound, OUTFILE *p){ */
/*   return outfile_set_(csound,p,0); */
/* } */

/* static int32_t outfile_set_S(CSOUND *csound, OUTFILE *p){ */
/*   return outfile_set_(csound,p,1); */
/* } */

static int32_t outfile_set_A(CSOUND *csound, OUTFILEA *p)
{
    SF_INFO sfinfo;
    int32_t     format_, n, buf_reqd;
    int32_t len = p->tabin->sizes[0];

    memset(&sfinfo, 0, sizeof(SF_INFO));
    format_ = (int32_t) MYFLT2LRND(*p->iflag);
     if (format_ >=  51)
      sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
    else if (format_ < 0) {
      sfinfo.format = FORMAT2SF(csound->oparms->outformat);
      sfinfo.format |= TYPE2SF(csound->oparms->filetyp);
    }
    else
      sfinfo.format = fout_format_table[format_];
    if (!SF2FORMAT(sfinfo.format))
      sfinfo.format |= FORMAT2SF(csound->oparms->outformat);
    if (!SF2TYPE(sfinfo.format))
      sfinfo.format |= TYPE2SF(csound->oparms->filetyp);
    sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_ESR);
    p->buf_pos = 0;

    if (CS_KSMPS >= 512)
      buf_reqd = p->guard_pos = CS_KSMPS * len;
    else {
      p->guard_pos = 512 * len;
      buf_reqd = (1 + (int32_t)(512 / CS_KSMPS)) * p->guard_pos;
      // Or......
      //buf_reqd = (1 + (int32_t)(512 / CS_KSMPS)) * CS_KSMPS * len;
    }
    if (p->buf.auxp == NULL || p->buf.size < buf_reqd*sizeof(MYFLT)) {
      csound->AuxAlloc(csound, sizeof(MYFLT)*buf_reqd, &p->buf);
    }
    p->f.bufsize =  p->buf.size;
    sfinfo.channels = len;
    n = fout_open_file(csound, &(p->f), NULL, CSFILE_SND_W,
                       p->fname, 1, &sfinfo, 0);
    if (UNLIKELY(n < 0))
      return NOTOK;

    if (((STDOPCOD_GLOBALS*) csound->stdOp_Env)->file_opened[n].do_scale)
      p->scaleFac = csound->dbfs_to_float;
    else
      p->scaleFac = FL(1.0);

    csound->RegisterDeinitCallback(csound, p, fouta_flush_callback);
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
        //#ifndef USE_DOUBLE
        //sf_write_float(p->f.sf, buf, p->buf_pos);
        //#else
        //sf_write_double(p->f.sf, buf, p->buf_pos);
        //#endif
      if (p->f.async==1)
        csound->WriteAsync(csound, p->f.fd, buf, p->buf_pos);
      else sf_write_MYFLT(p->f.sf, buf, p->buf_pos);
      p->buf_pos = 0;
    }
    return OK;
}

static int32_t koutfile_set_(CSOUND *csound, KOUTFILE *p, int32_t istring)
{
    SF_INFO sfinfo;
    int32_t     format_, n, buf_reqd;

    memset(&sfinfo, 0, sizeof(SF_INFO));
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
      sfinfo.format = SF_FORMAT_PCM_16 | SF_FORMAT_RAW;
    else
      sfinfo.format = fout_format_table[format_] | SF_FORMAT_RAW;

    if (CS_KSMPS >= 512)
      buf_reqd = CS_KSMPS *  p->nargs;
    else
      buf_reqd = (1 + (int32_t)(512 / CS_KSMPS)) * CS_KSMPS * p->nargs;
    if (p->buf.auxp == NULL || p->buf.size < buf_reqd*sizeof(MYFLT)) {
      csound->AuxAlloc(csound, sizeof(MYFLT)*buf_reqd, &p->buf);
    }
    p->f.bufsize = p->buf.size;
    n = fout_open_file(csound, &(p->f), NULL, CSFILE_SND_W,
                       p->fname, istring, &sfinfo, 0);
    if (UNLIKELY(n < 0))
      return NOTOK;

    if (((STDOPCOD_GLOBALS*) csound->stdOp_Env)->file_opened[n].do_scale)
      p->scaleFac = csound->dbfs_to_float;
    else
      p->scaleFac = FL(1.0);

    csound->RegisterDeinitCallback(csound, p, fout_flush_callback);
    return OK;
}

static int32_t koutfile_set(CSOUND *csound, KOUTFILE *p){
  return koutfile_set_(csound,p,0);
}

static int32_t koutfile_set_S(CSOUND *csound, KOUTFILE *p){
  return koutfile_set_(csound,p,1);
}


/*--------------*/

/* syntax:
        ihandle fiopen "filename" [, iascii]
*/

/* open a file and return its handle  */
/* the handle is simply a stack index */

static int32_t fiopen_(CSOUND *csound, FIOPEN *p, int32_t istring)
{
    char    *omodes[] = {"w", "r", "wb", "rb"};
    FILE    *rfp = (FILE*) NULL;
    int32_t     idx = (int32_t) MYFLT2LRND(*p->iascii), n;

    if (idx < 0 || idx > 3)
      idx = 0;
    n = fout_open_file(csound, (FOUT_FILE*) NULL, &rfp, CSFILE_STD,
                       p->fname, istring, omodes[idx], 1);
    if (UNLIKELY(n < 0))
      return NOTOK;
    if (idx > 1)
      setbuf(rfp, NULL);
    *p->ihandle = (MYFLT) n;

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
    STDOPCOD_GLOBALS  *pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    int32_t               idx = -1;

    if (istring || csound->ISSTRCOD(*(p->iFile))) {
      char    *fname = NULL;
      if (istring) fname = cs_strdup(csound, ((STRINGDAT *)p->iFile)->data);
      else if (csound->ISSTRCOD(*(p->iFile)))
        fname = cs_strdup(csound, get_arg_string(csound, *p->iFile));
      if (UNLIKELY(fname == NULL || fname[0] == (char) 0)) {
        if (fname != NULL) csound->Free(csound, fname);
        return csound->InitError(csound, Str("invalid file name"));
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
          /*ref count was set to 0x80000001U, but it needs to be 1 */
      memset(&tmp, 0, sizeof(FOUT_FILE));
      tmp.h.insdshead = p->h.insdshead;
      tmp.idx = idx + 1;
      fout_deinit_callback(csound, (void*) &tmp);
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
    STDOPCOD_GLOBALS  *pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    MYFLT   **args = p->argums;
    FILE    *rfil;
    uint32_t j;
    int32_t     n = (int32_t) MYFLT2LRND(*p->ihandle);

    if (UNLIKELY(n < 0 || n > pp->file_num))
      return csound->InitError(csound, Str("fouti: invalid file handle"));
    rfil = pp->file_opened[n].raw;
    if (UNLIKELY(rfil == NULL))
      return csound->InitError(csound, Str("fouti: invalid file handle"));
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
    STDOPCOD_GLOBALS  *pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
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

    pp = (STDOPCOD_GLOBALS*) csound->stdOp_Env;
    args = p->argums;
    n = (int32_t) MYFLT2LRND(*p->ihandle);
    if (UNLIKELY(n < 0 || n > pp->file_num))
      return csound->InitError(csound, Str("fouti: invalid file handle"));
    rfil = pp->file_opened[n].raw;
    if (UNLIKELY(rfil == NULL))
      return csound->InitError(csound, Str("fouti: invalid file handle"));
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

/*----------------------------------*/

static int32_t infile_set_(CSOUND *csound, INFILE *p, int32_t istring)
{
    SF_INFO sfinfo;
    int32_t     n, buf_reqd;
    p->nargs = p->INOCOUNT - 3;
    p->currpos = MYFLT2LRND(*p->iskpfrms);
    p->flag = 1;
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_ESR);
    /* Following code is seriously broken*/
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

    if (CS_KSMPS >= 512)
      buf_reqd = CS_KSMPS * sfinfo.channels;
    else
      buf_reqd = (1 + (int32_t)(512 / CS_KSMPS)) * CS_KSMPS * p->nargs;
    if (p->buf.auxp == NULL || p->buf.size < buf_reqd*sizeof(MYFLT)) {
      csound->AuxAlloc(csound, sizeof(MYFLT)*buf_reqd, &p->buf);
    }
    p->f.bufsize =  p->buf.size;
    n = fout_open_file(csound, &(p->f), NULL, CSFILE_SND_R,
                       p->fname, istring, &sfinfo, 0);
    if (UNLIKELY(n < 0))
      return NOTOK;

    if (((STDOPCOD_GLOBALS*) csound->stdOp_Env)->file_opened[n].do_scale)
      p->scaleFac = csound->e0dbfs;
    else
      p->scaleFac = FL(1.0);



    p->guard_pos = p->frames * p->nargs;
    p->buf_pos = p->guard_pos;

    if (p->f.async == 1)
    csound->FSeekAsync(csound,p->f.fd, p->currpos*p->f.nchnls, SEEK_SET);

    return OK;
}

static int32_t infile_set(CSOUND *csound, INFILE *p){
    return infile_set_(csound,p,0);
}

static int32_t infile_set_S(CSOUND *csound, INFILE *p){
    return infile_set_(csound,p,1);
}

#include "arrays.h"

static int32_t infile_set_A(CSOUND *csound, INFILEA *p)
{
    SF_INFO sfinfo;
    int32_t     n, buf_reqd;
    p->currpos = MYFLT2LRND(*p->iskpfrms);
    p->flag = 1;
    memset(&sfinfo, 0, sizeof(SF_INFO));
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
    p->chn = sfinfo.channels;
    if (CS_KSMPS >= 512)
      buf_reqd = CS_KSMPS * sfinfo.channels;
    else
      buf_reqd = (1 + (int32_t)(512 / CS_KSMPS)) * CS_KSMPS * sfinfo.channels;
    if (p->buf.auxp == NULL || p->buf.size < buf_reqd*sizeof(MYFLT)) {
      csound->AuxAlloc(csound, sizeof(MYFLT)*buf_reqd, &p->buf);
    }
    p->f.bufsize =  p->buf.size;
    n = fout_open_file(csound, &(p->f), NULL, CSFILE_SND_R,
                       p->fname, 1, &sfinfo, 0);
    if (UNLIKELY(n < 0))
      return NOTOK;

    if (((STDOPCOD_GLOBALS*) csound->stdOp_Env)->file_opened[n].do_scale)
      p->scaleFac = csound->e0dbfs;
    else
      p->scaleFac = FL(1.0);

    p->guard_pos = p->frames * p->chn;
    p->buf_pos = p->guard_pos;

    if (p->f.async == 1)
    csound->FSeekAsync(csound,p->f.fd, p->currpos*p->f.nchnls, SEEK_SET);

    tabinit(csound, p->tabout, p->chn);
    return OK;
}

static int32_t infile_act(CSOUND *csound, INFILE *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, k, j = offset;
    uint32_t nsmps = CS_KSMPS, ksmps, nargs = p->nargs;
    MYFLT *buf = (MYFLT *) p->buf.auxp;

    ksmps = nsmps;
    if (UNLIKELY(offset))
      for (i = 0; i < nargs; i++)
        memset(p->argums[i], '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      for (i = 0; i < nargs; i++)
        memset(&p->argums[i][nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->flag) {
      if (p->buf_pos >= p->guard_pos) {
        if (UNLIKELY(p->f.async == 0)) {
          sf_seek(p->f.sf, p->currpos*p->f.nchnls, SEEK_SET);
          p->remain = (uint32_t) sf_read_MYFLT(p->f.sf, (MYFLT*) buf,
                                               p->frames*p->f.nchnls);
          p->remain /= p->f.nchnls;
        } else {
          p->remain = csoundReadAsync(csound,p->f.fd,(MYFLT *)buf,
                                      p->frames*p->f.nchnls);
          p->remain /= p->f.nchnls;
        }
        p->currpos += p->frames;
        p->buf_pos = 0;
      }
      if (p->remain < nsmps)
        nsmps = p->remain;
      for (k = (uint32_t)p->buf_pos; j < nsmps; j++)
        for (i = 0; i < nargs; i++)
          p->argums[i][j] = buf[k++] * p->scaleFac;
      p->buf_pos = k;
      p->remain -= ksmps;
      if (p->remain <= 0 && p->buf_pos < p->guard_pos) {
        p->flag = 0;
        for (; j < ksmps; j++)
          for (i = 0; i < nargs; i++)
            p->argums[i][j] = FL(0.0);
      }
      return OK;
    }
    for ( ; j < ksmps; j++)
      for (i = 0; i < nargs; i++)
        p->argums[i][j] = FL(0.0);

    return OK;
}

static int32_t infile_arr(CSOUND *csound, INFILEA *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, k, j = offset;
    uint32_t nsmps = CS_KSMPS, ksmps, chn = p->chn;
    MYFLT *buf = (MYFLT *) p->buf.auxp;
    MYFLT *data = p->tabout->data;

    ksmps = nsmps;
    if (UNLIKELY(offset))
      for (i = 0; i < chn; i++)
        memset(&data[i*chn], '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      for (i = 0; i < chn; i++)
        memset(&data[i*chn+nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->flag) {
      if (p->buf_pos >= p->guard_pos) {
        if (UNLIKELY(p->f.async == 0)) {
          sf_seek(p->f.sf, p->currpos*p->f.nchnls, SEEK_SET);
          p->remain = (uint32_t) sf_read_MYFLT(p->f.sf, (MYFLT*) buf,
                                               p->frames*p->f.nchnls);
          p->remain /= p->f.nchnls;
        } else {
          p->remain = csoundReadAsync(csound,p->f.fd,(MYFLT *)buf,
                                      p->frames*p->f.nchnls);
          p->remain /= p->f.nchnls;
        }
        p->currpos += p->frames;
        p->buf_pos = 0;
      }
      if (p->remain < nsmps)
        nsmps = p->remain;
      for (k = (uint32_t)p->buf_pos; j < nsmps; j++)
        for (i = 0; i < chn; i++)
          data[i*chn+j] = buf[k++] * p->scaleFac;
      p->buf_pos = k;
      p->remain -= ksmps;
      if (p->remain <= 0 && p->buf_pos < p->guard_pos) {
        p->flag = 0;
        for (; j < ksmps; j++)
          for (i = 0; i < chn; i++)
            data[i*chn+j] = FL(0.0);
      }
      return OK;
    }
    for ( ; j < ksmps; j++)
      for (i = 0; i < chn; i++)
        data[i*chn+j] = FL(0.0);

    return OK;
}

/* ---------------------------- */

static int32_t kinfile_set_(CSOUND *csound, KINFILE *p, int32_t istring)
{
    SF_INFO sfinfo;
    int32_t     n, buf_reqd;

    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.samplerate = (int32_t) MYFLT2LRND(CS_EKR);
    if ((int32_t) MYFLT2LRND(*p->iflag) == -2)
      sfinfo.format = FORMAT2SF(AE_FLOAT) | TYPE2SF(TYP_RAW);
    else if ((int32_t) MYFLT2LRND(*p->iflag) == -1)
      sfinfo.format = FORMAT2SF(AE_SHORT) | TYPE2SF(TYP_RAW);
    else
      sfinfo.format = 0;
    sfinfo.channels = p->INOCOUNT - 3;

    p->nargs = p->INOCOUNT - 3;
    p->currpos = MYFLT2LRND(*p->iskpfrms);
    p->flag = 1;

    if (CS_KSMPS >= 512)
      p->frames = CS_KSMPS;
    else
      p->frames = (int32_t)(512 / CS_KSMPS) * CS_KSMPS;

    if (CS_KSMPS >= 512)
      buf_reqd = CS_KSMPS *   sfinfo.channels;
    else
      buf_reqd = (1 + (int32_t)(512 / CS_KSMPS)) * CS_KSMPS * p->nargs;
    if (p->buf.auxp == NULL || p->buf.size < buf_reqd*sizeof(MYFLT)) {
      csound->AuxAlloc(csound, sizeof(MYFLT)*buf_reqd, &p->buf);
    }
    p->f.bufsize =  p->buf.size;
    n = fout_open_file(csound, &(p->f), NULL, CSFILE_SND_R,
                       p->fname, istring, &sfinfo, 0);
    if (UNLIKELY(n < 0))
      return NOTOK;

    if (((STDOPCOD_GLOBALS*) csound->stdOp_Env)->file_opened[n].do_scale)
      p->scaleFac = csound->e0dbfs;
    else
      p->scaleFac = FL(1.0);

    p->guard_pos = p->frames * p->nargs;
    p->buf_pos = p->guard_pos;

    if (p->f.async == 1)
    csound->FSeekAsync(csound,p->f.fd, p->currpos*p->f.nchnls, SEEK_SET);

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
    int32_t   i, k;
    int32_t nargs = p->nargs;
    MYFLT *buf = (MYFLT *) p->buf.auxp;

    if (p->flag) {
      if (p->buf_pos >= p->guard_pos) {
        if (UNLIKELY(p->f.async == 0)) {
          sf_seek(p->f.sf, p->currpos*p->f.nchnls, SEEK_SET);
          p->remain = (uint32_t) sf_read_MYFLT(p->f.sf, (MYFLT*) buf,
                                               p->frames*p->f.nchnls);
          p->remain /= p->f.nchnls;
        } else {
          p->remain = csoundReadAsync(csound,p->f.fd,(MYFLT *)buf,
                                      p->frames*p->f.nchnls);
          p->remain /= p->f.nchnls;
        }
        p->currpos += p->frames;
        p->buf_pos = 0;
      }
      if (p->remain > 0) {
        for (i = 0, k = p->buf_pos; i < nargs; i++)
          p->argums[i][0] = buf[k++] * p->scaleFac;
        p->buf_pos = k;
        p->remain--;
        return OK;
      }
      p->flag = 0;
    }
    for (i = 0; i < nargs; i++)
      p->argums[i][0] = FL(0.0);
    return OK;
}

static int32_t i_infile_(CSOUND *csound, I_INFILE *p, int32_t istring)
{
    int32_t     j, n, nargs;
    FILE    *fp = NULL;
    MYFLT   **args = p->argums;
    char    *omodes[] = {"r", "r", "rb"};
    int32_t     idx = (int32_t) MYFLT2LRND(*p->iflag);

    if (UNLIKELY(idx < 0 || idx > 2))
      idx = 0;
    n = fout_open_file(csound, (FOUT_FILE*) NULL, &fp, CSFILE_STD,
                       p->fname, istring, omodes[idx], 0);
    if (UNLIKELY(n < 0))
      return NOTOK;

    nargs = p->INOCOUNT - 3;
    switch ((int32_t) MYFLT2LRND(*p->iflag)) {
    case 0: /* ascii file with loop */
      {
        char  cf[64], *cfp;
        int32_t   cc;
      newcycle:
        for (j = 0; j < nargs; j++) {
          cfp = cf;
          while ((*cfp = cc = getc(fp)) == 'i' || isspace(*cfp));
          if (cc == EOF) {
            fseek(fp, 0, SEEK_SET);
            goto newcycle;
          }
          while (isdigit(*cfp) || *cfp == '.' || *cfp == '+' || *cfp == '-') {
            *(++cfp) = (char)(cc = getc(fp));
          }
          *++cfp = '\0';        /* Must terminate string */
          *(args[j]) = (MYFLT) atof(cf);
          if (cc == EOF) {
            fseek(fp, 0, SEEK_SET);
            break;
          }
        }
      }
      break;
    case 1: /* ascii file without loop */
      {
        char  cf[64], *cfp;
        int32_t   cc;
        for (j = 0; j < nargs; j++) {
          cfp = cf;
          while ((*cfp = cc = getc(fp)) == 'i' || isspace(*cfp));
          if (cc == EOF) {
            *(args[j]) = FL(0.0);
            break;
          }
          while (isdigit(*cfp) || *cfp == '.' || *cfp == '+' || *cfp == '-') {
            *(++cfp) = cc = getc(fp);
          }
          *++cfp = '\0';        /* Must terminate */
          *(args[j]) = (MYFLT) atof (cf);
          if (cc == EOF) {
            *(args[j]) = FL(0.0);
            break;
          }
        }
      }
      break;
    case 2: /* binary floats without loop */
      if (fseek(fp, p->currpos * sizeof(float) * nargs, SEEK_SET)<0) return NOTOK;
      p->currpos++;
      for (j = 0; j < nargs; j++) {
        if (1 == fread(args[j], sizeof(float), 1, fp));
        else {
          p->flag = 0;
          *(args[j]) = FL(0.0);
        }
      }
      break;
    }
    return OK;
}

static int32_t i_infile(CSOUND *csound, I_INFILE *p){
    return i_infile_(csound,p,0);
}

static int32_t i_infile_S(CSOUND *csound, I_INFILE *p){
    return i_infile_(csound,p,1);
}


/*---------------------------*/

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

/*---------------------------------*/
/* formatted output to a text file */

static int32_t fprintf_set_(CSOUND *csound, FPRINTF *p, int32_t istring)
{
    int32_t     n;
    char    *sarg = (char*) p->fmt->data;
    char    *sdest = p->txtstring;

    memset(p->txtstring, 0, 8192); /* Nasty to have exposed constant in code */

    if (p->h.opadr != (SUBR) NULL)      /* fprintks */
      n = fout_open_file(csound, &(p->f), NULL, CSFILE_STD,
                         p->fname, istring, "w", 1);
    else                                /* fprints */
      n = fout_open_file(csound, (FOUT_FILE*) NULL, &(p->f.f), CSFILE_STD,
                         p->fname, istring, "w", 1);
    if (UNLIKELY(n < 0))
      return NOTOK;
    //setvbuf(p->f.f, (char*)NULL, _IOLBF, BUFSIZ); /* Seems a good option */
    /* Copy the string to the storage place in PRINTKS.
     *
     * We will look out for certain special codes and write special
     * bytes directly to the string.
     *
     * There is probably a more elegant way of doing this, then using
     * the look flag.  I could use goto - but I would rather not.      */
    /* This is really a if then else if...
     * construct and is currently grotty -- JPff */
    do {
      char temp  = *sarg++;
      char tempn = *sarg--;
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
      else if (temp == '\\') {
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
    } while (*++sarg != 0);
    return OK;
}

static int32_t fprintf_set(CSOUND *csound, FPRINTF *p){
    return fprintf_set_(csound,p,0);
}

static int32_t fprintf_set_S(CSOUND *csound, FPRINTF *p){
    return fprintf_set_(csound,p,1);
}



/* perform a sprintf-style format -- matt ingalls */
void sprints1(char *outstring,  char *fmt, MYFLT **kvals, int32 numVals)
{
    char strseg[8192];
    int32_t len = 8192;
    int32_t i = 0, j = 0;
    char *segwaiting = 0;
    while (*fmt) {
      if (*fmt == '%') {
        /* if already a segment waiting, then lets print it */
        if (segwaiting) {
          strseg[i] = '\0';
          switch (*segwaiting) {
          case '%':
            strNcpy(outstring, "%%", len);
            j--;
            break;
          case 'd':
          case 'i':
          case 'o':
          case 'x':
          case 'X':
          case 'u':
          case 'c':
            snprintf(outstring, len, strseg, (int32_t) MYFLT2LRND(*kvals[j]));
            break;
          case 'h':
            snprintf(outstring, len, strseg, (int16) MYFLT2LRND(*kvals[j]));
            break;
          case 'l':
            snprintf(outstring, len, strseg, (int32) MYFLT2LRND(*kvals[j]));
            break;
          case 's':
            snprintf(outstring, len, strseg, ((STRINGDAT*)(kvals[j]))->data);
            break;
          default:
            snprintf(outstring, len, strseg, *kvals[j]);
            break;
          }
          len -= strlen(outstring);
          outstring += strlen(outstring);
          i = 0;
          segwaiting = 0;

          /* prevent potential problems */
          /* if user didnt give enough input params */
          if (j < numVals-1)
            j++;
        }
        /* copy the '%' */
        strseg[i++] = *fmt++;

        /* find the format code */
        segwaiting = fmt;
        while (*segwaiting && !isalpha(*segwaiting) && !(*segwaiting=='%'))
          segwaiting++;
      }
      else
        strseg[i++] = *fmt++;
    }

    if (i) {
      strseg[i] = '\0';
      if (segwaiting) {
        switch (*segwaiting) {
          case '%':
            strNcpy(outstring, "%%", len);
            j--;
            break;
        case 'd':
        case 'i':
        case 'o':
        case 'x':
        case 'X':
        case 'u':
        case 'c':
          snprintf(outstring, len, strseg, (int32_t) MYFLT2LRND(*kvals[j]));
          break;
        case 'h':
          snprintf(outstring, len, strseg, (int16) MYFLT2LRND(*kvals[j]));
          break;
        case 'l':
          snprintf(outstring, len, strseg, (int64_t) MYFLT2LRND(*kvals[j]));
          break;
        case 's':
            snprintf(outstring, len, strseg, ((STRINGDAT*)(kvals[j]))->data);
            break;

        default:
          snprintf(outstring, len, strseg, *kvals[j]);
          break;
        }
      }
      else
       snprintf(outstring, len, "%s", strseg);
    }

}

static int32_t fprintf_k(CSOUND *csound, FPRINTF *p)
{
    char    string[8192];

    (void) csound;
    sprints1(string, p->txtstring, p->argums, p->INOCOUNT - 2);
    fprintf(p->f.f, "%s", string);
    fflush(p->f.f);
    return OK;
}

/* i-rate fprints */
static int32_t fprintf_i(CSOUND *csound, FPRINTF *p)
{
    char    string[8192];

    if (UNLIKELY(fprintf_set(csound, p) != OK))
      return NOTOK;
    sprints1(string, p->txtstring, p->argums, p->INOCOUNT - 2);
    fprintf(p->f.f,"%s", string);
    fflush(p->f.f);
    return OK;
}

static int32_t fprintf_i_S(CSOUND *csound, FPRINTF *p)
{
    char    string[8192];

    if (UNLIKELY(fprintf_set_S(csound, p) != OK))
      return NOTOK;
    sprints1(string, p->txtstring, p->argums, p->INOCOUNT - 2);
    fprintf(p->f.f, "%s", string);
    fflush(p->f.f);
    return OK;
}

#define S(x)    sizeof(x)
static OENTRY localops[] = {

    {"fprints",    S(FPRINTF),      0, 1,  "",     "SSN",
        (SUBR) fprintf_i_S, (SUBR) NULL,(SUBR) NULL, NULL, },
    {"fprints.i",    S(FPRINTF),      0, 1,  "",     "iSN",
        (SUBR) fprintf_i, (SUBR) NULL,(SUBR) NULL, NULL},
    { "fprintks",   S(FPRINTF),    WR, 3,  "",     "SSN",
        (SUBR) fprintf_set_S,     (SUBR) fprintf_k,   (SUBR) NULL, NULL,},
    { "fprintks.i",   S(FPRINTF),    WR, 3,  "",     "iSN",
        (SUBR) fprintf_set,     (SUBR) fprintf_k,   (SUBR) NULL, NULL},
     { "vincr",      S(INCR),       WI, 2,  "",     "aa",
        (SUBR) NULL,            (SUBR) incr, NULL         },
    { "clear",      S(CLEARS),      WI, 2,  "",     "y",
        (SUBR) NULL,            (SUBR) clear, NULL},
    { "fout",       S(OUTFILE),     0, 3,  "",     "Siy",
        (SUBR) outfile_set_S,     (SUBR) outfile, NULL},
    { "fout.A",     S(OUTFILEA),    0, 3,  "",     "Sia[]",
        (SUBR) outfile_set_A,     (SUBR) outfile_array, NULL},
    /* { "fout.i",       S(OUTFILE),     0, 3,  "",     "iiy", */
    /*     (SUBR) outfile_set,     (SUBR) outfile, NULL}, */
    { "foutk",      S(KOUTFILE),    0, 3,  "",     "Siz",
        (SUBR) koutfile_set_S,    (SUBR) koutfile,    (SUBR) NULL, NULL },
    { "foutk.i",      S(KOUTFILE),    0, 3,  "",     "iiz",
        (SUBR) koutfile_set,    (SUBR) koutfile,    (SUBR) NULL, NULL },
    { "fouti",      S(IOUTFILE),    0, 1,  "",     "iiim",
        (SUBR) ioutfile_set,    (SUBR) NULL,        (SUBR) NULL, NULL         },
    { "foutir",     S(IOUTFILE_R),  0, 3,  "",     "iiim",
        (SUBR) ioutfile_set_r,  (SUBR) ioutfile_r,  (SUBR) NULL, NULL},
    { "fiopen",     S(FIOPEN),      0, 1,  "i",    "Si",
        (SUBR) fiopen_S,          (SUBR) NULL,        (SUBR) NULL, NULL},
    { "fiopen.i",     S(FIOPEN),      0, 1,  "i",    "ii",
        (SUBR) fiopen,          (SUBR) NULL,        (SUBR) NULL, NULL},
    { "ficlose",    S(FICLOSE),     0, 1,  "",     "S",
        (SUBR) ficlose_opcode_S,  (SUBR) NULL,        (SUBR) NULL, NULL},
    { "ficlose.S",  S(FICLOSE),     0, 1,  "",     "i",
        (SUBR) ficlose_opcode,  (SUBR) NULL,        (SUBR) NULL, NULL },
    { "fin.a",      S(INFILE),     WI|_QQ, 3,  "",      "Siiy",
        (SUBR) infile_set_S,    (SUBR) infile_act, NULL},
    { "fin.A",      S(INFILEA),    WI, 3,  "",     "Siia[]",
        (SUBR) infile_set_A,    (SUBR) infile_arr, NULL},
    { "fin.i",      S(INFILE),     WI|_QQ, 3,  "",     "iiiy",
        (SUBR) infile_set,      (SUBR) infile_act, NULL},
    { "fink",       S(KINFILE),    WI, 3,  "",     "Siiz",
        (SUBR) kinfile_set_S,     (SUBR) kinfile,     (SUBR) NULL, NULL},
    { "fink.i",       S(KINFILE),  WI, 3,  "",     "iiiz",
        (SUBR) kinfile_set,     (SUBR) kinfile,     (SUBR) NULL, NULL},
    { "fini",       S(I_INFILE),   WI, 1,  "",     "Siim",
      (SUBR) i_infile_S,        (SUBR) NULL,        (SUBR) NULL, NULL },
    { "fini.i",       S(I_INFILE), WI, 1,  "",     "iiim",
        (SUBR) i_infile,        (SUBR) NULL,        (SUBR) NULL, NULL}
};

int32_t fout_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t) (sizeof(localops) / sizeof(OENTRY)));
}
