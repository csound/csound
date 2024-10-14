/*
    cvanal.c:

    Copyright (C) 1996 Greg Sullivan, John ffitch

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

/************************************************************************/
/*                                                                      */
/*  Performs a FFT on a time domain soundfile, and saves the result in  */
/*  a file.                                                             */
/*  Purpose is to create a frequency domain version of an impulse       */
/*  response, for the later use by the convolve operator.               */
/*  Greg Sullivan                                                       */
/************************************************************************/

#include "std_util.h"
#include "soundio.h"
#include "convolve.h"

static int32_t takeFFT(CSOUND *csound, SOUNDIN *inputSound, CVSTRUCT *outputCVH,
                       int64_t Hlenpadded, SNDFILE *infd, FILE *ofd, int32_t nf);
static int32_t quit(CSOUND*, char *msg);
static int32_t CVAlloc(CSOUND*, CVSTRUCT**, int64_t, int32_t, MYFLT,
                       int32_t, int32_t, int64_t, int32_t, int32_t);

#define SF_UNK_LEN      -1      /* code for sndfile len unkown  */

#define FIND(MSG)   if (*s == '\0')  \
                      if (UNLIKELY(!(--argc) || ((s = *++argv) && *s == '-')))    \
                        return quit(csound,MSG);

static int32_t cvanal(CSOUND *csound, int32_t argc, char **argv)
{
    CVSTRUCT *cvh;
    char    *infilnam, *outfilnam;
    SNDFILE *infd;
    FILE    *ofd;
    void    *ofd_handle;
    int32_t err, channel = ALLCHNLS;
    SOUNDIN *p;  /* space allocated by SAsndgetset() */
    MYFLT   beg_time = FL(0.0), input_dur = FL(0.0), sr = FL(0.0);
    int64_t Estdatasiz, Hlen;
    int64_t Hlenpadded = 1;
    char    err_msg[512];
    int32_t res;
    int32_t new_format = 0;

    /* csound->dbfs_to_float = csound->e0dbfs = FL(1.0); */
    if (UNLIKELY(!(--argc))) {
      return quit(csound,Str("insufficient arguments"));
    }
    do {
      char *s = *++argv;
      if (*s++ == '-')
        switch (*s++) {
        case 's':
          FIND(Str("no sampling rate"))
#ifdef USE_DOUBLE
          csound->Sscanf(s, "%lf", &sr);
#else
          csound->Sscanf(s, "%f", &sr);
#endif
          break;
        case 'c':
          FIND(Str("no channel"))
            csound->Sscanf(s, "%d", &channel);
          if (UNLIKELY((channel < 1) || (channel > 4)))
            return quit(csound, Str("channel must be in the range 1 to 4"));
          break;
        case 'b':
          FIND(Str("no begin time"))
#ifdef USE_DOUBLE
          csound->Sscanf(s, "%lf", &beg_time);
#else
          csound->Sscanf(s, "%f", &beg_time);
#endif
          break;
        case 'd':
          FIND(Str("no duration time"))
#ifdef USE_DOUBLE
          csound->Sscanf(s, "%lf", &input_dur);
#else
          csound->Sscanf(s, "%f", &input_dur);
#endif
          break;
        case 'X':
          new_format = 1;
          break;
        default:   return quit(csound, Str("unrecognised switch option"));
        }
      else break;
    } while (--argc);

    if (UNLIKELY(argc !=  2))
      return quit(csound, Str("illegal number of filenames"));
    infilnam = *argv++;
    outfilnam = *argv;

    if (UNLIKELY((infd = (csound->GetUtility(csound))->SndinGetSetSA(csound, infilnam, &p, &beg_time,
                                             &input_dur, &sr, channel)) == NULL)) {
      snprintf(err_msg, 512, Str("error while opening %s"), infilnam);
      return quit(csound, err_msg);
    }
    sr = (MYFLT) p->sr;

    Hlen = p->getframes;
    while (Hlenpadded < 2*Hlen-1)
      Hlenpadded <<= 1;

    Estdatasiz = (Hlenpadded + 2) * sizeof(MYFLT);
    if (channel == ALLCHNLS)
      Estdatasiz *= p->nchanls;

    /* alloc & fill CV hdrblk */
    if (UNLIKELY((err = CVAlloc(csound, &cvh, Estdatasiz, CVMYFLT, sr,
                                p->nchanls, channel, Hlen, CVRECT, 4)))) {
      csound->Message(csound, "%s", Str("cvanal: Error allocating header\n"));
      return -1;
    }
    if (new_format) {

      ofd_handle = csound->FileOpen(csound, &ofd, CSFILE_STD, outfilnam, "w",
                                     "SADIR", CSFTYPE_CVANAL, 0);
      if (UNLIKELY(ofd_handle == NULL)) {         /* open the output CV file */
        return quit(csound, Str("cannot create output file"));
      }                                          /* & wrt hdr into the file */
#if defined(USE_DOUBLE)
      fprintf(ofd, "CVANAL\n%d %d %d %.17lg %d %d %d %d\n",
              cvh->headBsize,              /* total number of bytes of data */
              cvh->dataBsize,              /* total number of bytes of data */
              cvh->dataFormat,             /* (int32_t) format specifier */
              (double)cvh->samplingRate,   /* of original sample */
              cvh->src_chnls,              /* no. of channels in source */
              cvh->channel,                /* requested channel(s) */
              cvh->Hlen,                   /* length of impulse reponse */
              cvh->Format);                /* (int32_t) how words are org'd in frm */
#else
      fprintf(ofd, "CVANAL\n%d %d %d %.9g %d %d %d %d\n",
              cvh->headBsize,              /* total number of bytes of data */
              cvh->dataBsize,              /* total number of bytes of data */
              cvh->dataFormat,             /* (int32_t) format specifier */
              (double)cvh->samplingRate,   /* of original sample */
              cvh->src_chnls,              /* no. of channels in source */
              cvh->channel,                /* requested channel(s) */
              cvh->Hlen,                   /* length of impulse reponse */
              cvh->Format);                /* (int32_t) how words are org'd in frm */
#endif
    }
    else {
      ofd_handle = csound->FileOpen(csound, &ofd, CSFILE_STD, outfilnam, "wb",
                                     "SFDIR", CSFTYPE_CVANAL, 0);
      if (UNLIKELY(ofd_handle == NULL)) {           /* open the output CV file */
        return quit(csound, Str("cannot create output file"));
      }                                           /* & wrt hdr into the file */
      if (UNLIKELY((int64_t) fwrite(cvh, 1, cvh->headBsize, ofd) < cvh->headBsize)) {
        return quit(csound, Str("cannot write header"));
      }
    }
    res = takeFFT(csound, p, cvh, Hlenpadded, infd, ofd, new_format);
    csound->Message(csound, "%s", Str("cvanal finished\n"));
    return (res != 0 ? -1 : 0);
}

static int32_t quit(CSOUND *csound, char *msg)
{
    csound->Message(csound, Str("cvanal error: %s\n"), msg);
    csound->Message(csound, "%s", Str("Usage: cvanal [-d<duration>] "
                            "[-c<channel>] [-b<begin time>] [-X] <input soundfile>"
                            " <output impulse response FFT file>\n"));
    return -1;
}

static int32_t takeFFT(CSOUND *csound, SOUNDIN *p, CVSTRUCT *cvh,
                   int64_t Hlenpadded, SNDFILE *infd, FILE *ofd, int32_t nf)
{
    int32_t i, j, read_in;
    MYFLT   *inbuf, *outbuf;
    MYFLT   *fp1, *fp2;
    int32_t Hlen = (int32_t) cvh->Hlen;
    int32_t nchanls;
    void *setup;

    nchanls = cvh->channel != ALLCHNLS ? 1 : cvh->src_chnls;
    j = (int32_t) (Hlen * nchanls);
    inbuf = fp1 = (MYFLT *) csound->Malloc(csound, j * sizeof(MYFLT));
    if (UNLIKELY((read_in = (csound->GetUtility(csound))->Sndin(csound, infd, inbuf, j, p)) < j)) {
      csound->Message(csound, "%s", Str("less sound than expected!\n"));
      return -1;
    }
    /* normalize the samples read in. */
    for (i = read_in; i--; ) {
      *fp1++ *= 1.0/csound->Get0dBFS(csound);
    }

    fp1 = inbuf;
    outbuf = fp2 = (MYFLT*) csound->Malloc(csound,
                                           sizeof(MYFLT) * (Hlenpadded + 2));
    /* for (i = 0; i < (Hlenpadded + 2); i++) */
    /*   outbuf[i] = FL(0.0); */
    memset(outbuf, 0, sizeof(MYFLT)*(Hlenpadded + 2));
    setup = csound->RealFFTSetup(csound, (int32_t)Hlenpadded, FFT_FWD);

    for (i = 0; i < nchanls; i++) {
      for (j = Hlen; j > 0; j--) {
        *fp2++ = *fp1;
        fp1 += nchanls;
      }
      fp1 = inbuf + i + 1;
      csound->RealFFT(csound, setup, outbuf);
      outbuf[Hlenpadded] = outbuf[1];
      outbuf[1] = outbuf[Hlenpadded + 1L] = FL(0.0);
      /* write straight out, just the indep vals */
      if (nf) {
        int32 i, l;
        l = (cvh->dataBsize/nchanls)/sizeof(MYFLT);
        for (i=0; i<l; i++) {
            fprintf(ofd, "%a\n", (double)outbuf[i]);
        }
      }
      else
        if (UNLIKELY(1!=fwrite(outbuf, cvh->dataBsize/nchanls, 1, ofd)))
          fprintf(stderr, "%s", Str("Write failure\n"));
      for (j = (int32_t) (Hlenpadded - Hlen); j > 0; j--)
        fp2[j] = FL(0.0);
      fp2 = outbuf;
    }
    return 0;
}

static int32_t CVAlloc(
    CSOUND      *csound,
    CVSTRUCT    **pphdr,        /* returns address of new block */
    int64_t     dataBsize,      /* desired bytesize of datablock */
    int32_t     dataFormat,     /* data format - PVMYFLT etc */
    MYFLT       srate,          /* sampling rate of original in Hz */
    int32_t     src_chnls,      /* number of channels in source */
    int32_t     channel,        /* requested channel(s) */
    int64_t     Hlen,           /* impulse response length */
    int32_t     Format,         /* format of frames: CVPOLAR, CVPVOC etc */
    int32_t     infoBsize)      /* bytes to allocate in info region */

    /* Allocate memory for a new CVSTRUCT+data block;
       fill in header according to passed in data.
       Returns CVE_MALLOC  (& **pphdr = NULL) if malloc fails
               CVE_OK      otherwise  */
{
    int64_t  hSize;

    hSize = sizeof(CVSTRUCT) + infoBsize - CVDFLTBYTS;
    if (( (*pphdr) = (CVSTRUCT *) csound->Malloc(csound, hSize)) == NULL )
      return(CVE_MALLOC);
    (*pphdr)->magic        = CVMAGIC;
    (*pphdr)->headBsize    = (int32_t) hSize;
    (*pphdr)->dataBsize    = (int32_t) dataBsize;
    (*pphdr)->dataFormat   = dataFormat;
    (*pphdr)->samplingRate = srate;
    (*pphdr)->src_chnls    = src_chnls;
    (*pphdr)->channel      = channel;
    (*pphdr)->Hlen         = (int32_t) Hlen;
    (*pphdr)->Format       = Format;
    /* leave info bytes undefined */
    return(CVE_OK);
}

/* module interface */

int32_t cvanal_init_(CSOUND *csound)
{
    int32_t retval = (csound->GetUtility(csound))->AddUtility(csound, "cvanal", cvanal);
    if (!retval) {
      retval =
        (csound->GetUtility(csound))->SetUtilityDescription(csound, "cvanal",
                                      Str("Soundfile analysis for convolve"));
    }
    return retval;
}

