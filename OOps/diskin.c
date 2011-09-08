/*
    diskin.c:

    Copyright (C) 1998, 2001 matt ingalls, Richard Dobson, John ffitch
              (C) 2005 Istvan Varga

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/*               ugen developed by matt ingalls, ...            */
/*               replaced with new implementation based on      */
/*               diskin2 by Istvan Varga                        */
/*                                                              */
/*      diskin  -       a new soundin that shifts pitch         */
/*              based on the old soundin code                   */
/*              Adjusted to include sndfile library             */

#include "csoundCore.h"
#include "soundio.h"
#include "diskin.h"
#include <math.h>

static CS_NOINLINE void diskin_read_buffer(SOUNDINEW *p, int bufReadPos)
{
    int32 nsmps, bufsmps;
    int   i;

    /* calculate new buffer frame start position */
    p->bufStartPos = p->bufStartPos + (int32) bufReadPos;
    p->bufStartPos &= (~((int32) (p->bufSize - 1)));
    bufsmps = (int32) ((p->bufSize + 1) * p->nChannels);
    i = 0;
    if (p->bufStartPos >= 0L) {
      /* number of sample frames to read */
      nsmps = p->fileLength - p->bufStartPos;
      if (nsmps > 0) {         /* if there is anything to read: */
        nsmps *= (int32) p->nChannels;
        if (nsmps > bufsmps)
          nsmps = bufsmps;
        sf_seek(p->sf, (sf_count_t) p->bufStartPos, SEEK_SET);
        i = (int) sf_read_float(p->sf, &(p->buf[0]), (sf_count_t) nsmps);
        if (UNLIKELY(i < 0))  /* error ? */
          i = 0;    /* clear entire buffer to zero */
      }
    }
    /* fill rest of buffer with zero samples */
    memset(p->buf+i, 0, sizeof(float)*(bufsmps-i));
    /* while (i < bufsmps) */
    /*   p->buf[i++] = 0.0f; */
}

/* Mix one sample frame from input file at location 'pos' to outputs    */
/* of opcode 'p', at sample index 'n' (0 <= n < ksmps), with amplitude  */
/* scale 'scl'.                                                         */

static inline void diskin_get_sample(SOUNDINEW *p, int32 fPos, int n, MYFLT scl)
{
    int  bufPos, i;

    if (p->wrapMode) {
      if (UNLIKELY(fPos >= p->fileLength))
        fPos -= p->fileLength;
      else if (UNLIKELY(fPos < 0L))
        fPos += p->fileLength;
    }
    bufPos = (int) (fPos - p->bufStartPos);
    if (UNLIKELY(bufPos < 0 || bufPos > p->bufSize)) {
      /* not in current buffer frame, need to read file */
      diskin_read_buffer(p, bufPos);
      /* recalculate buffer position */
      bufPos = (int) (fPos - p->bufStartPos);
    }
    /* copy all channels from buffer */
    if (p->nChannels == 1) {
      p->aOut[0][n] += scl * (MYFLT) p->buf[bufPos];
    }
    else if (p->nChannels == 2) {
      bufPos += bufPos;
      p->aOut[0][n] += scl * (MYFLT) p->buf[bufPos];
      p->aOut[1][n] += scl * (MYFLT) p->buf[bufPos + 1];
    }
    else {
      bufPos *= p->nChannels;
      i = 0;
      /* p->aOut[i++][n] += scl * (MYFLT) p->buf[bufPos++]; */
      /* p->aOut[i++][n] += scl * (MYFLT) p->buf[bufPos++]; */
      do {
        p->aOut[i++][n] += scl * (MYFLT) p->buf[bufPos++];
      } while (i < p->nChannels);
    }
}

/* calculate buffer size in sample frames */

static int diskin_calc_buffer_size(SOUNDINEW *p, int n_monoSamps)
{
    int i, nFrames;

    /* default to 4096 mono samples if zero or negative */
    if (n_monoSamps <= 0)
      n_monoSamps = 4096;
    /* convert mono samples -> sample frames */
    i = n_monoSamps / p->nChannels;
    /* buffer size must be an integer power of two, so round up */
    nFrames = 1;
    while (nFrames < i)
      nFrames <<= 1;
    /* limit to sane range */
    if (nFrames < 128)
      nFrames = 128;
    else if (nFrames > 1048576)
      nFrames = 1048576;

    return nFrames;
}

static const int diskin_format_table[11] = {
    0,
    0,
    SF_FORMAT_RAW | SF_FORMAT_PCM_S8,
    SF_FORMAT_RAW | SF_FORMAT_ALAW,
    SF_FORMAT_RAW | SF_FORMAT_ULAW,
    SF_FORMAT_RAW | SF_FORMAT_PCM_16,
    SF_FORMAT_RAW | SF_FORMAT_PCM_32,
    SF_FORMAT_RAW | SF_FORMAT_FLOAT,
    SF_FORMAT_RAW | SF_FORMAT_PCM_U8,
    SF_FORMAT_RAW | SF_FORMAT_PCM_24,
    SF_FORMAT_RAW | SF_FORMAT_DOUBLE
};

/* init routine for diskin */

int newsndinset(CSOUND *csound, SOUNDINEW *p)
{
    double  pos;
    char    name[1024];
    void    *fd;
    SF_INFO sfinfo;
    int     n, bsize = (int) *p->ibufsize;

    /* check number of channels */
    p->nChannels = (int) (p->OUTOCOUNT);
    if (UNLIKELY(p->nChannels < 1 || p->nChannels > DISKIN2_MAXCHN)) {
      return csound->InitError(csound, Str("diskin: invalid number of channels"));
    }
    /* if already open, close old file first */
    if (p->fdch.fd != NULL) {
      /* skip initialisation if requested */
      if (*(p->iSkipInit) != FL(0.0))
        return OK;
      fdclose(csound, &(p->fdch));
    }
    /* set default format parameters */
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.samplerate = (int) (csound->esr + FL(0.5));
    sfinfo.channels = p->nChannels;
    /* check for user specified sample format */
    n = (int) (*(p->iSampleFormat) + FL(2.5)) - 1;
    if (n == 1) {
      sfinfo.format = SF_FORMAT_RAW
                      | (int) FORMAT2SF(csound->oparms_.outformat);
    }
    else {
      if (UNLIKELY(n < 0 || n > 10))
        return csound->InitError(csound, Str("diskin: unknown sample format"));
      sfinfo.format = diskin_format_table[n];
    }
    /* open file */
    /* FIXME: name can overflow with very long string */
    csound->strarg2name(csound, name, p->iFileCode, "soundin.", p->XSTRCODE);
    fd = csound->FileOpen2(csound, &(p->sf), CSFILE_SND_R, name, &sfinfo,
                            "SFDIR;SSDIR", CSFTYPE_UNKNOWN_AUDIO, 0);
    if (UNLIKELY(fd == NULL)) {
      return
        csound->InitError(csound, Str("diskin: %s: failed to open file"), name);
    }
    /* record file handle so that it will be closed at note-off */
    memset(&(p->fdch), 0, sizeof(FDCH));
    p->fdch.fd = fd;
    fdrecord(csound, &(p->fdch));
    /* print file information */
    csound->Warning(csound, Str("diskin: opened '%s':\n"
                                "        %d Hz, %d channel(s), "
                                "%ld sample frames\n"),
                    csound->GetFileName(fd),
                    (int) sfinfo.samplerate, (int) sfinfo.channels,
                    (int32) sfinfo.frames);
    /* check number of channels in file (must equal the number of outargs) */
    if (UNLIKELY(sfinfo.channels != p->nChannels &&
                 (csound->oparms_.msglevel & WARNMSG) != 0)) {
      return csound->InitError(csound,
                               Str("diskin: number of output args "
                                   "inconsistent with number of file channels"));
    }
    /* skip initialisation if requested */
    if (p->initDone && *(p->iSkipInit) != FL(0.0))
      return OK;
    /* set file parameters from header info */
    p->fileLength = (int32) sfinfo.frames;
    if ((int) (csound->esr + FL(0.5)) != sfinfo.samplerate) {
      csound->Warning(csound, Str("diskin: warning: file sample rate (%d) "
                                  "!= orchestra sr (%d)\n"),
                      sfinfo.samplerate, (int) (csound->esr + FL(0.5)));
    }
    /* apply dBFS scale unless reading "raw" float file */
    if ((SF2FORMAT(sfinfo.format) == AE_FLOAT ||
         SF2FORMAT(sfinfo.format) == AE_DOUBLE) &&
        !(SF2TYPE(sfinfo.format) == TYP_WAV ||
          SF2TYPE(sfinfo.format) == TYP_AIFF ||
          SF2TYPE(sfinfo.format) == TYP_W64))
      p->scaleFac = FL(1.0);
    else
      p->scaleFac = csound->e0dbfs;
    /* wrap mode */
    p->wrapMode = (*(p->iWrapMode) == FL(0.0) ? 0 : 1);
    if (p->fileLength < 1L)
      p->wrapMode = 0;
    /* initialise read position */
    pos = (double)*(p->iSkipTime) * (double)sfinfo.samplerate;
    if (UNLIKELY(pos > (double)p->fileLength)) {
      csound->Warning(csound, Str("skip time larger than audio data, "
                                  "substituting zero."));
      pos = 0.0;
    }
    else if (UNLIKELY(pos < 0.0)) {
      csound->Warning(csound, Str("negative skip time, substituting zero."));
      pos = 0.0;
    }
    pos = (pos + 0.5) * (double)POS_FRAC_SCALE;
    p->pos_frac = (int64_t)pos & (~((int64_t)POS_FRAC_MASK));
    p->pos_frac_inc = (int64_t)0;
    p->prv_kTranspose = FL(0.0);
    /* initialise buffer */
    p->bufSize = diskin_calc_buffer_size(p, (bsize ? bsize : 4096));
    csound->Warning(csound, Str("bufsize %d\n"), p->bufSize);
    p->bufStartPos = -((int32)(p->bufSize << 1));

    if (p->auxch.auxp == NULL ||
       p->auxch.size < 2*p->bufSize*sizeof(MYFLT)*p->nChannels)
      csound->AuxAlloc(csound,2*sizeof(MYFLT)*p->bufSize*p->nChannels, &p->auxch);
    p->buf = (float *) p->auxch.auxp;

    /* done initialisation */
    p->initDone = -1;

    return OK;
}

static inline void diskin_file_pos_inc(SOUNDINEW *p, int32 *ndx)
{
    p->pos_frac += p->pos_frac_inc;
    *ndx = (int32) (p->pos_frac >> POS_FRAC_SHIFT);
    if (p->wrapMode) {
      if (*ndx >= p->fileLength) {
        *ndx -= p->fileLength;
        p->pos_frac -= ((int64_t) p->fileLength << POS_FRAC_SHIFT);
      }
      else if (*ndx < 0L) {
        *ndx += p->fileLength;
        p->pos_frac += ((int64_t) p->fileLength << POS_FRAC_SHIFT);
      }
    }
}

/* a-rate routine for soundinew */

int soundinew(CSOUND *csound, SOUNDINEW *p)
{
    MYFLT   a0, a1;
    int32   ndx;
    int     nn, chn;

    if (p->initDone <= 0) {
      if (UNLIKELY(!p->initDone))
        return csound->PerfError(csound, Str("diskin: not initialised"));
      p->initDone = 1;
      /* if no skip time, and playing backwards: start from end of file */
      if (p->pos_frac <= (int64_t)0 && *(p->kTranspose) < FL(0.0)) {
        p->pos_frac = (int64_t)(((double)p->fileLength + 0.5)
                                 * (double)POS_FRAC_SCALE);
        p->pos_frac = p->pos_frac & (~((int64_t)POS_FRAC_MASK));
      }
    }
    if (*(p->kTranspose) != p->prv_kTranspose) {
      double  f;
      p->prv_kTranspose = *(p->kTranspose);
      f = (double)p->prv_kTranspose * (double)POS_FRAC_SCALE;
#ifdef HAVE_C99
      p->pos_frac_inc = (int64_t) llrint(f);
#else
      p->pos_frac_inc = (int64_t) (f + (f < 0.0 ? -0.5 : 0.5));
#endif
    }
    /* clear outputs to zero first */
    for (chn = 0; chn < p->nChannels; chn++)
      for (nn = 0; nn < csound->ksmps; nn++)
        p->aOut[chn][nn] = FL(0.0);
    /* file read position */
    ndx = (int32) (p->pos_frac >> POS_FRAC_SHIFT);
    /* ---- linear interpolation ---- */
    for (nn = 0; nn < csound->ksmps; nn++) {
      a1 = (MYFLT) ((int) (p->pos_frac & (int64_t) POS_FRAC_MASK))
           * (FL(1.0) / (MYFLT) POS_FRAC_SCALE) * p->scaleFac;
      a0 = p->scaleFac - a1;
      diskin_get_sample(p, ndx, nn, a0);
      ndx++;
      diskin_get_sample(p, ndx, nn, a1);
      /* update file position */
      diskin_file_pos_inc(p, &ndx);
    }

    return OK;
}

static int soundout_deinit(CSOUND *csound, void *pp)
{
    char    *opname = csound->GetOpcodeName(pp);
    SNDCOM  *p;

    if (strcmp(opname, "soundouts") == 0)
      p = &(((SNDOUTS*) pp)->c);
    else
      p = &(((SNDOUT*) pp)->c);

    if (p->fd != NULL) {
      /* flush buffer */
      MYFLT *p0 = (MYFLT*) &(p->outbuf[0]);
      MYFLT *p1 = (MYFLT*) p->outbufp;
      if (p1 > p0) {
        sf_write_MYFLT(p->sf, p0, (sf_count_t) ((MYFLT*) p1 - (MYFLT*) p0));
        p->outbufp = (MYFLT*) &(p->outbuf[0]);
      }
      /* close file */
      csound->FileClose(csound, p->fd);
      p->sf = (SNDFILE*) NULL;
      p->fd = NULL;
    }

    return OK;
}

/* RWD:DBFS: NB: thse funcs all supposed to write to a 'raw' file, so
   what will people want for 0dbfs handling? really need to update
   opcode with more options. */

/* init routine for instr soundout  */

int sndo1set(CSOUND *csound, void *pp)
{
    char    *sfname, *opname, sndoutname[256];
    SNDCOM  *p;
    MYFLT   *ifilcod, *iformat;
    int     filetyp = TYP_RAW, format = csound->oparms_.outformat, nchns = 1;
    SF_INFO sfinfo;

    opname = csound->GetOpcodeName(pp);
    csound->Warning(csound, Str("%s is deprecated; use fout instead\n"),
                      opname);
    if (strcmp(opname, "soundouts") == 0) {
      p = &(((SNDOUTS*) pp)->c);
      ifilcod = ((SNDOUTS*) pp)->ifilcod;
      iformat = ((SNDOUTS*) pp)->iformat;
      nchns++;
    }
    else {
      p = &(((SNDOUT*) pp)->c);
      ifilcod = ((SNDOUT*) pp)->ifilcod;
      iformat = ((SNDOUT*) pp)->iformat;
    }

    if (p->fd != NULL)                  /* if file already open, */
      return OK;                        /* return now            */

    csound->RegisterDeinitCallback(csound, pp, soundout_deinit);

    csound->strarg2name(csound, sndoutname, ifilcod, "soundout.",
                                ((OPDS*) pp)->optext->t.xincod_str);
    sfname = sndoutname;
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.frames = -1;
    sfinfo.samplerate = (int) (csound->esr + FL(0.5));
    sfinfo.channels = nchns;
    switch ((int) (*iformat + FL(0.5))) {
      case 1: format = AE_CHAR; break;
      case 4: format = AE_SHORT; break;
      case 5: format = AE_LONG; break;
      case 6: format = AE_FLOAT;
      case 0: break;
      default:
        return csound->InitError(csound, Str("%s: invalid sample format: %d"),
                                 opname, (int) (*iformat + FL(0.5)));
    }
    sfinfo.format = TYPE2SF(filetyp) | FORMAT2SF(format);
    p->fd = csound->FileOpen2(csound, &(p->sf), CSFILE_SND_W, sfname, &sfinfo,
                                "SFDIR", type2csfiletype(filetyp, format), 0);
    if (p->fd == NULL) {
      return csound->InitError(csound, Str("%s cannot open %s"), opname, sfname);
    }
    sfname = csound->GetFileName(p->fd);
    if (format != AE_FLOAT)
      sf_command(p->sf, SFC_SET_CLIPPING, NULL, SF_TRUE);
    else
      sf_command(p->sf, SFC_SET_CLIPPING, NULL, SF_FALSE);
#ifdef USE_DOUBLE
    sf_command(p->sf, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE);
#else
    sf_command(p->sf, SFC_SET_NORM_FLOAT, NULL, SF_FALSE);
#endif
    csound->Warning(csound, Str("%s: opening RAW outfile %s\n"),
                      opname, sfname);
    p->outbufp = p->outbuf;                 /* fix - isro 20-11-96 */
    p->bufend = p->outbuf + SNDOUTSMPS;     /* fix - isro 20-11-96 */

    return OK;
}

int soundout(CSOUND *csound, SNDOUT *p)
{
    int nn, nsamps = csound->ksmps;

    if (UNLIKELY(p->c.sf == NULL))
      return csound->PerfError(csound, Str("soundout: not initialised"));
    for (nn = 0; nn < nsamps; nn++) {
      if (UNLIKELY(p->c.outbufp >= p->c.bufend)) {
        sf_write_MYFLT(p->c.sf, p->c.outbuf, p->c.bufend - p->c.outbuf);
        p->c.outbufp = p->c.outbuf;
      }
      *(p->c.outbufp++) = p->asig[nn];
    }

    return OK;
}

int soundouts(CSOUND *csound, SNDOUTS *p)
{
    int nn, nsamps = csound->ksmps;

    if (UNLIKELY(p->c.sf == NULL))
      return csound->PerfError(csound, Str("soundouts: not initialised"));
    for (nn = 0; nn < nsamps; nn++) {
      if (UNLIKELY(p->c.outbufp >= p->c.bufend)) {
        sf_write_MYFLT(p->c.sf, p->c.outbuf, p->c.bufend - p->c.outbuf);
        p->c.outbufp = p->c.outbuf;
      }
      *(p->c.outbufp++) = p->asig1[nn];
      *(p->c.outbufp++) = p->asig2[nn];
    }

    return OK;
}

