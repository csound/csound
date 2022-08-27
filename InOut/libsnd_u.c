/*
    libsnd_u.c:

    Copyright (C) 2005 Barry Vercoe, John ffitch, Istvan Varga

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
#include "soundio.h"
#include <sndfile.h>

void rewriteheader(void *ofd)
{
    if (LIKELY(ofd != NULL))
      sf_command((SNDFILE *)ofd, SFC_UPDATE_HEADER_NOW, NULL, 0);
}

/* Stand-Alone sndgetset() */
/* used by SoundAnal progs */
/* Returns NULL on failure */

void *SAsndgetset(CSOUND *csound, char *infilnam, void *ap_,
                  MYFLT *abeg_time, MYFLT *ainput_dur, MYFLT *asr,
                  int channel)
{
    SOUNDIN **ap = (SOUNDIN**) ap_;
    SOUNDIN *p;
    SNDFILE *infile = NULL;

    csound->esr = FL(0.0);      /* set esr 0. with no orchestra   */
    *ap = p = (SOUNDIN*) csound->Calloc(csound, sizeof(SOUNDIN));
    strNcpy(p->sfname, infilnam, 512); //p->sfname[511] = '\0';
    if (UNLIKELY(channel < 1 && channel != ALLCHNLS)) {
      csound->Message(csound, Str("channel request %d illegal\n"), channel);
      csound->Free(csound, p);
      *ap = NULL;
      return NULL;
    }
    p->channel = channel;
    p->analonly = 1;
    p->sr = (int) (*asr + FL(0.5));
    p->skiptime = *abeg_time;
    if ((infile = sndgetset(csound, p)) == NULL)  /* open sndfil, do skiptime */
      return(NULL);
    if (UNLIKELY(p->framesrem < (int64_t) 0)) {
      csound->Warning(csound, Str("undetermined file length, "
                                  "will attempt requested duration"));
    }
    else {
      if (*ainput_dur <= FL(0.0)) {         /* 0 durtim, use to EOF */
        p->getframes = p->framesrem;
        *ainput_dur = (MYFLT) ((double) p->getframes / (double) p->sr);
      }
      /* else chk that input dur is within filetime rem */
      else {
        p->getframes = (int64_t) ((double) p->sr * (double) *ainput_dur + 0.5);
        if (UNLIKELY(p->getframes > p->framesrem)) {
          p->getframes = p->framesrem;
          csound->Warning(csound, Str("full requested duration not available"));
        }
      }
      csound->Message(csound, Str("analysing %ld sample frames (%3.1f secs)"),
                              (long) p->getframes, *ainput_dur);
      if (*abeg_time != FL(0.0))
        csound->Message(csound, Str(" from timepoint %3.1f\n"), *abeg_time);
      else
        csound->Message(csound, "\n");
    }
    return (void*) infile;
}

/* special handling of sound input to accomodate reads thru pipes & net
 * where nbytes rcvd can be < n requested
 *
 * extra arg passed for filetyp testing on POST-HEADER reads of audio samples
 */
static int sreadin(CSOUND *csound, SNDFILE *infd, MYFLT *inbuf,
                   int nsamples, SOUNDIN *p)
{
    /* return the number of samples read */
    int   n, ntot = 0;
    do {
      n = sf_read_MYFLT(infd, inbuf + ntot, nsamples - ntot);
      if (UNLIKELY(n < 0))
        csound->Die(csound, Str("soundfile read error"));
    } while (n > 0 && (ntot += n) < nsamples);
    if (p->audrem > (int64_t) 0) {
      if ((int64_t) ntot > p->audrem)   /*   chk haven't exceeded */
        ntot = (int) p->audrem;         /*   limit of audio data  */
      p->audrem -= (int64_t) ntot;
      return ntot;
    }
    return 0;
}

/* core of soundinset     */
/* called from sndinset, SAsndgetset, & gen01 */
/* Return NULL on failure */

void *sndgetset(CSOUND *csound, void *p_)
{
    SOUNDIN *p = (SOUNDIN*) p_;
    int     n;
    int     framesinbuf, skipframes;
    char    *sfname;
    SF_INFO sfinfo;

    sfname = &(p->sfname[0]);
    /* IV - Feb 26 2005: should initialise sfinfo structure */
    memset(&sfinfo, 0, sizeof(SF_INFO));
    sfinfo.format = (p->format<0 ?        /* store default sample format, */
                     ((int) FORMAT2SF(-p->format) | SF_FORMAT_RAW) : 0);
    sfinfo.channels = 1;                /* number of channels, */
    if (p->analonly)                    /* and sample rate */
      sfinfo.samplerate = (int) p->sr;
    else
      sfinfo.samplerate = (int) ((double) csound->esr + 0.5);
    if (sfinfo.samplerate < 1)
      sfinfo.samplerate = (int) ((double) DFLT_SR + 0.5);
    /* open with full dir paths */
    p->fd = csound->FileOpen2(csound, &(p->sinfd), CSFILE_SND_R,
                                     sfname, &sfinfo, "SFDIR;SSDIR",
                                     CSFTYPE_UNKNOWN_AUDIO, 0);
    if (UNLIKELY(p->fd == NULL)) {
      csound->ErrorMsg(csound, Str("soundin cannot open %s: %s"),
                       sfname, sf_strerror(NULL));
      goto err_return;
    }
    /* & record fullpath filnam */
    sfname = csound->GetFileName(p->fd);

    /* copy type from headata */
    p->format = SF2FORMAT(sfinfo.format);
    p->sampframsiz = (int) sfsampsize(sfinfo.format) * (int) sfinfo.channels;
    p->nchanls = sfinfo.channels;
    framesinbuf = (int) SNDINBUFSIZ / (int) p->nchanls;
    p->bufsmps = framesinbuf * p->nchanls;
    p->endfile = 0;
    p->filetyp = SF2TYPE(sfinfo.format);
    if (p->analonly) {                              /* anal: if sr param val */
      if (p->sr != 0 && p->sr != sfinfo.samplerate) {   /*   use it          */
        csound->Warning(csound, Str("-s %d overriding soundfile sr %d"),
                                (int) p->sr, (int) sfinfo.samplerate);
        sfinfo.samplerate = p->sr;
      }
    }
    else if (UNLIKELY(sfinfo.samplerate != (int) ((double) csound->esr + 0.5))) {
      csound->Warning(csound,                       /* non-anal:  cmp w. esr */
                      "%s sr = %d, orch sr = %7.1f",
                      sfname, (int) sfinfo.samplerate, csound->esr);
    }
    if (UNLIKELY(p->channel != ALLCHNLS && p->channel > sfinfo.channels)) {
      csound->ErrorMsg(csound, Str("error: req chan %d, file %s has only %d"),
                               (int) p->channel, sfname, (int) sfinfo.channels);
      goto err_return;
    }
    p->sr = (int) sfinfo.samplerate;
    if (csound->oparms_.msglevel & 3) {
      csound->Message(csound, Str("audio sr = %d, "), (int) p->sr);
      switch (p->nchanls) {
        case 1: csound->Message(csound, Str("monaural")); break;
        case 2: csound->Message(csound, Str("stereo"));   break;
        case 4: csound->Message(csound, Str("quad"));     break;
        case 6: csound->Message(csound, Str("hex"));      break;
        case 8: csound->Message(csound, Str("oct"));      break;
        default: csound->Message(csound, Str("%d-channels"), (int) p->nchanls);
      }
      if (p->nchanls > 1) {
        if (p->channel == ALLCHNLS)
          csound->Message(csound, Str(", reading %s channels"),
                                  (p->nchanls == 2 ? Str("both") : Str("all")));
        else
          csound->Message(csound, Str(", reading channel %d"),
                                  (int) p->channel);
      }
      csound->Message(csound, Str("\nopening %s infile %s\n"),
                              type2string(p->filetyp), sfname);
    }
    p->audrem = (int64_t) sfinfo.frames * (int64_t) sfinfo.channels;
    p->framesrem = (int64_t) sfinfo.frames;         /*   find frames rem */
    skipframes = (int) ((double) p->skiptime * (double) p->sr
                        + (p->skiptime >= FL(0.0) ? 0.5 : -0.5));
    if (skipframes < 0) {
      n = -skipframes;
      if (UNLIKELY(n > framesinbuf)) {
        csound->ErrorMsg(csound, Str("soundin: invalid skip time"));
        goto err_return;
      }
      n *= (int) sfinfo.channels;
      p->inbufp = &(p->inbuf[0]);
      p->bufend = p->inbufp;
      do {
        *(p->bufend++) = FL(0.0);
      } while (--n);
    }
    else if (skipframes < framesinbuf) {        /* if sound within 1st buf */
      n = sreadin(csound, p->sinfd, p->inbuf, p->bufsmps, p);
      p->bufend = &(p->inbuf[0]) + n;
      p->inbufp = &(p->inbuf[0]) + (skipframes * (int) sfinfo.channels);
      if (p->inbufp >= p->bufend) {
        p->inbufp = p->bufend;
        p->audrem = (int64_t) 0;
        p->endfile = 1;
      }
    }
    else if ((int64_t) skipframes >= p->framesrem) {
      n = framesinbuf * (int) sfinfo.channels;
      p->inbufp = &(p->inbuf[0]);
      p->bufend = p->inbufp;
      do {
        *(p->bufend++) = FL(0.0);
      } while (--n);
      p->audrem = (int64_t) 0;
      p->endfile = 1;
    }
    else {                                      /* for greater skiptime: */
      /* else seek to bndry */
      if (UNLIKELY(sf_seek(p->sinfd, (sf_count_t) skipframes, SEEK_SET) < 0)) {
        csound->ErrorMsg(csound, Str("soundin seek error"));
        goto err_return;
      }
      /* now rd fulbuf */
      if ((n = sreadin(csound, p->sinfd, p->inbuf, p->bufsmps, p)) == 0)
        p->endfile = 1;
      p->inbufp = p->inbuf;
      p->bufend = p->inbuf + n;
    }
    if (p->framesrem != (int64_t) -1)
      p->framesrem -= (int64_t) skipframes;     /* sampleframes to EOF   */

    return p->sinfd;                            /* return the active fd  */

 err_return:
    if (p->fd != NULL)
      csound->FileClose(csound, p->fd);
    p->sinfd = NULL;
    p->fd = NULL;
    return NULL;
}

/* a simplified soundin */

int getsndin(CSOUND *csound, void *fd_, MYFLT *fp, int nlocs, void *p_)
{
    SNDFILE *fd = (SNDFILE*) fd_;
    SOUNDIN *p = (SOUNDIN*) p_;
    int     i = 0, n;
    MYFLT   scalefac;

    if (p->format == AE_FLOAT || p->format == AE_DOUBLE) {
      if (p->filetyp == TYP_WAV || p->filetyp == TYP_AIFF ||
          p->filetyp == TYP_W64)
        scalefac = csound->e0dbfs;
      else
        scalefac = FL(1.0);
      if (p->do_floatscaling)
        scalefac *= p->fscalefac;
    }
    else
      scalefac = csound->e0dbfs;

    if (p->nchanls == 1 || p->channel == ALLCHNLS) {  /* MONO or ALLCHNLS */
      for ( ; i < nlocs; i++) {
        if (p->inbufp >= p->bufend) {
          if ((n = sreadin(csound, fd, p->inbuf, p->bufsmps, p)) <= 0)
            break;
          p->inbufp = p->inbuf;
          p->bufend = p->inbuf + n;
        }
        fp[i] = *p->inbufp++ * scalefac;
      }
    }
    else {                                /* MULTI-CHANNEL, SELECT ONE */
      int   chcnt;
      for ( ; i < nlocs; i++) {
        if (p->inbufp >= p->bufend) {
          if ((n = sreadin(csound, fd, p->inbuf, p->bufsmps, p)) <= 0)
            break;
          p->inbufp = p->inbuf;
          p->bufend = p->inbuf + n;
        }
        chcnt = 0;
        do {
          if (++chcnt == p->channel)
            fp[i] = *p->inbufp * scalefac;
          p->inbufp++;
        } while (chcnt < p->nchanls);
      }
    }

    n = i;
    memset(&(fp[i]), 0, (nlocs-i)*sizeof(MYFLT)); /* if incomplete PAD */
    /* for ( ; i < nlocs; i++)     /\* if incomplete *\/ */
    /*   fp[i] = FL(0.0);          /\*  pad with 0's *\/ */
    return n;
}

void dbfs_init(CSOUND *csound, MYFLT dbfs)
{
    csound->dbfs_to_float = FL(1.0) / dbfs;
    csound->e0dbfs = dbfs;
    /* probably want this message written just before note messages start... */
    /* VL: printing too early does not allow us to switch this off
       better print this when the engine is ready to run.
     */
    // csound->Message(csound, Str("0dBFS level = %.1f\n"), dbfs);

}

char *type2string(int x)
{
    switch (x) {
      case TYP_WAV:   return "WAV";
      case TYP_AIFF:  return "AIFF";
      case TYP_AU:    return "AU";
      case TYP_RAW:   return "RAW";
      case TYP_PAF:   return "PAF";
      case TYP_SVX:   return "SVX";
      case TYP_NIST:  return "NIST";
      case TYP_VOC:   return "VOC";
      case TYP_IRCAM: return "IRCAM";
      case TYP_W64:   return "W64";
      case TYP_MAT4:  return "MAT4";
      case TYP_MAT5:  return "MAT5";
      case TYP_PVF:   return "PVF";
      case TYP_XI:    return "XI";
      case TYP_HTK:   return "HTK";
      case TYP_SDS:   return "SDS";
      case TYP_SD2:   return "SD2";
      case TYP_FLAC:  return "FLAC";
      case TYP_CAF:   return "CAF";
      case TYP_WVE:   return "WVE";
      case TYP_OGG:   return "OGG";
      case TYP_MPC2K: return "MPC2K";
      case TYP_RF64:  return "RF64";
      case TYP_MPEG:  return "MPEG";
      default:        return Str("unknown");
    }
}

int sfsampsize(int type)
{
    switch (type & SF_FORMAT_SUBMASK) {
      case SF_FORMAT_PCM_16:  return 2;     /* Signed 16 bit data */
      case SF_FORMAT_PCM_32:  return 4;     /* Signed 32 bit data */
      case SF_FORMAT_FLOAT:   return 4;     /* 32 bit float data */
      case SF_FORMAT_PCM_24:  return 3;     /* Signed 24 bit data */
      case SF_FORMAT_DOUBLE:  return 8;     /* 64 bit float data */
    }
    return 1;
}

char *getstrformat(int format)  /* used here, and in sfheader.c */
{
    switch (format) {
      case  AE_UNCH:    return Str("unsigned bytes"); /* J. Mohr 1995 Oct 17 */
      case  AE_CHAR:    return Str("signed chars");
      case  AE_ALAW:    return Str("alaw bytes");
      case  AE_ULAW:    return Str("ulaw bytes");
      case  AE_SHORT:   return Str("shorts");
      case  AE_LONG:    return Str("longs");
      case  AE_FLOAT:   return Str("floats");
      case  AE_DOUBLE:  return Str("double floats");
      case  AE_24INT:   return Str("24bit ints");     /* RWD 5:2001 */
      case  AE_VORBIS:  return Str("vorbis encoding");
      case  AE_MPEG:    return Str("mpeg encoding");
    }
    return Str("unknown");
}

/* type should be one of Csound's TYP_XXX macros,
   encoding should be one of its AE_XXX macros. */
int type2csfiletype(int type, int encoding)
{
    switch (type) {
      case TYP_RAW:    return CSFTYPE_RAW_AUDIO;
      case TYP_IRCAM:  return CSFTYPE_IRCAM;
      case TYP_AIFF:
        switch (encoding) {
          case AE_CHAR:
          case AE_SHORT:
          case AE_24INT:
          case AE_LONG:
                       return CSFTYPE_AIFF;
          default:     return CSFTYPE_AIFC;
        }
      case TYP_WAV:    return CSFTYPE_WAVE;
      case TYP_AU:     return CSFTYPE_AU;
      case TYP_W64:    return CSFTYPE_W64;
      case TYP_WAVEX:  return CSFTYPE_WAVEX;
      case TYP_AVR:    return CSFTYPE_AVR;
      case TYP_HTK:    return CSFTYPE_HTK;
      case TYP_MAT4:   return CSFTYPE_MAT4;
      case TYP_MAT5:   return CSFTYPE_MAT5;
      case TYP_NIST:   return CSFTYPE_NIST;
      case TYP_PAF:    return CSFTYPE_PAF;
      case TYP_PVF:    return CSFTYPE_PVF;
      case TYP_SVX:    return CSFTYPE_SVX;
      case TYP_VOC:    return CSFTYPE_VOC;
      case TYP_XI:     return CSFTYPE_XI;
      case TYP_SDS:    return CSFTYPE_SDS;
      case TYP_SD2:    return CSFTYPE_SD2;
      case TYP_FLAC:   return CSFTYPE_FLAC;
      case TYP_CAF:    return CSFTYPE_CAF;
      case TYP_WVE:    return CSFTYPE_WVE;
      case TYP_OGG:    return CSFTYPE_OGG;
      case TYP_MPEG:   return CSFTYPE_MPEG;
      case TYP_MPC2K:  return CSFTYPE_MPC2K;
      case TYP_RF64:   return CSFTYPE_RF64;
      default:         return CSFTYPE_UNKNOWN_AUDIO;
    }
}

/* type should be one of libsndfile's format values. */
int sftype2csfiletype(int type)
{
    /* mask out the endian-ness bits */
    int typemod = type & (SF_FORMAT_TYPEMASK | SF_FORMAT_SUBMASK);
    return type2csfiletype(SF2TYPE(typemod), SF2FORMAT(typemod));
}
