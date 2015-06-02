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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
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

static int takeFFT(CSOUND *csound, SOUNDIN *inputSound, CVSTRUCT *outputCVH,
                   long Hlenpadded, SNDFILE *infd, FILE *ofd, int nf);
static int quit(CSOUND*, char *msg);
static int CVAlloc(CSOUND*, CVSTRUCT**, long, int, MYFLT,
                   int, int, long, int, int);

#define SF_UNK_LEN      -1      /* code for sndfile len unkown  */

#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || ((s = *++argv) && *s == '-'))  \
                            return quit(csound,MSG);

static int cvanal(CSOUND *csound, int argc, char **argv)
{
    CVSTRUCT *cvh;
    char    *infilnam, *outfilnam;
    SNDFILE *infd;
    FILE    *ofd;
    void    *ofd_handle;
    int     err, channel = ALLCHNLS;
    SOUNDIN *p;  /* space allocated by SAsndgetset() */
    MYFLT   beg_time = FL(0.0), input_dur = FL(0.0), sr = FL(0.0);
    long    Estdatasiz, Hlen;
    long    Hlenpadded = 1;
    char    err_msg[512];
    int     res;
    int new_format = 0;

    /* csound->dbfs_to_float = csound->e0dbfs = FL(1.0); */
    if (!(--argc)) {
      return quit(csound, Str("insufficient arguments"));
    }
    do {
      char *s = *++argv;
      if (*s++ == '-')
        switch (*s++) {
        case 's':
          FIND(Str("no sampling rate"))
#ifdef USE_DOUBLE
          csound->sscanf(s, "%lf", &sr);
#else
          csound->sscanf(s, "%f", &sr);
#endif
          break;
        case 'c':
          FIND(Str("no channel"))
            sscanf(s, "%d", &channel);
          if ((channel < 1) || (channel > 4))
            return quit(csound, Str("channel must be in the range 1 to 4"));
          break;
        case 'b':
          FIND(Str("no begin time"))
#ifdef USE_DOUBLE
          csound->sscanf(s, "%lf", &beg_time);
#else
          csound->sscanf(s, "%f", &beg_time);
#endif
          break;
        case 'd':
          FIND(Str("no duration time"))
#ifdef USE_DOUBLE
          csound->sscanf(s, "%lf", &input_dur);
#else
          csound->sscanf(s, "%f", &input_dur);
#endif
          break;
        case 'X':
          new_format = 1;
          break;
        default:   return quit(csound, Str("unrecognised switch option"));
        }
      else break;
    } while (--argc);

    if (argc !=  2) return quit(csound, Str("illegal number of filenames"));
    infilnam = *argv++;
    outfilnam = *argv;

    if ((infd = csound->SAsndgetset(csound, infilnam, &p, &beg_time,
                                    &input_dur, &sr, channel)) == NULL) {
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
    if ((err = CVAlloc(csound, &cvh, Estdatasiz, CVMYFLT, sr,
                       p->nchanls, channel, Hlen, CVRECT, 4))) {
      csound->Message(csound, Str("cvanal: Error allocating header\n"));
      return -1;
    }
    if (new_format) {
      ofd_handle = csound->FileOpen2(csound, &ofd, CSFILE_STD, outfilnam, "w",
                                     "SFDIR", CSFTYPE_CVANAL, 0);
      if (ofd_handle == NULL) {                   /* open the output CV file */
        return quit(csound, Str("cannot create output file"));
      }                                           /* & wrt hdr into the file */
      fprintf(ofd, "CVANAL\n%d %d %d %a %d %d %d %d\n",
              cvh->headBsize,              /* total number of bytes of data */
              cvh->dataBsize,              /* total number of bytes of data */
              cvh->dataFormat,             /* (int) format specifier */
              (double)cvh->samplingRate,   /* of original sample */
              cvh->src_chnls,              /* no. of channels in source */
              cvh->channel,                /* requested channel(s) */
              cvh->Hlen,                   /* length of impulse reponse */
              cvh->Format);                /* (int) how words are org'd in frm */
    }
    else {
      ofd_handle = csound->FileOpen2(csound, &ofd, CSFILE_STD, outfilnam, "wb",
                                     "SFDIR", CSFTYPE_CVANAL, 0);
      if (ofd_handle == NULL) {                   /* open the output CV file */
        return quit(csound, Str("cannot create output file"));
      }                                           /* & wrt hdr into the file */
      if ((long) fwrite(cvh, 1, cvh->headBsize, ofd) < cvh->headBsize) {
        return quit(csound, Str("cannot write header"));
      }
    }
    res = takeFFT(csound, p, cvh, Hlenpadded, infd, ofd, new_format);
    csound->Message(csound, Str("cvanal finished\n"));
    return (res != 0 ? -1 : 0);
}

static int quit(CSOUND *csound, char *msg)
{
    csound->Message(csound, Str("cvanal error: %s\n"), msg);
    csound->Message(csound, Str("Usage: cvanal [-d<duration>] "
                            "[-c<channel>] [-b<begin time>] [-X] <input soundfile>"
                            " <output impulse response FFT file> \n"));
    return -1;
}

static int takeFFT(CSOUND *csound, SOUNDIN *p, CVSTRUCT *cvh,
                   long Hlenpadded, SNDFILE *infd, FILE *ofd, int nf)
{
    int     i, j, read_in;
    MYFLT   *inbuf, *outbuf;
    MYFLT   *fp1, *fp2;
    int     Hlen = (int) cvh->Hlen;
    int     nchanls;

    nchanls = cvh->channel != ALLCHNLS ? 1 : cvh->src_chnls;
    j = (int) (Hlen * nchanls);
    inbuf = fp1 = (MYFLT *) csound->Malloc(csound, j * sizeof(MYFLT));
    if ((read_in = csound->getsndin(csound, infd, inbuf, j, p)) < j) {
      csound->Message(csound, Str("less sound than expected!\n"));
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

    for (i = 0; i < nchanls; i++) {
      for (j = Hlen; j > 0; j--) {
        *fp2++ = *fp1;
        fp1 += nchanls;
      }
      fp1 = inbuf + i + 1;
      csound->RealFFT(csound, outbuf, (int) Hlenpadded);
      outbuf[Hlenpadded] = outbuf[1];
      outbuf[1] = outbuf[Hlenpadded + 1L] = FL(0.0);
      /* write straight out, just the indep vals */
      if (nf) {
        int32 i, l;
        l = (cvh->dataBsize/nchanls)/sizeof(MYFLT);
        for (i=0; i<l; i++) fprintf(ofd, "%a\n", (double)outbuf[i]);
      }
      else
        if (UNLIKELY(1!=fwrite(outbuf, cvh->dataBsize/nchanls, 1, ofd)))
          fprintf(stderr, Str("Write failure\n"));
      for (j = Hlenpadded - Hlen; j > 0; j--)
        fp2[j] = FL(0.0);
      fp2 = outbuf;
    }
    return 0;
}

static int CVAlloc(
    CSOUND      *csound,
    CVSTRUCT    **pphdr,        /* returns address of new block */
    long        dataBsize,      /* desired bytesize of datablock */
    int         dataFormat,     /* data format - PVMYFLT etc */
    MYFLT       srate,          /* sampling rate of original in Hz */
    int         src_chnls,      /* number of channels in source */
    int         channel,        /* requested channel(s) */
    long        Hlen,           /* impulse response length */
    int         Format,         /* format of frames: CVPOLAR, CVPVOC etc */
    int         infoBsize)      /* bytes to allocate in info region */

    /* Allocate memory for a new CVSTRUCT+data block;
       fill in header according to passed in data.
       Returns CVE_MALLOC  (& **pphdr = NULL) if malloc fails
               CVE_OK      otherwise  */
{
    long  hSize;

    hSize = sizeof(CVSTRUCT) + infoBsize - CVDFLTBYTS;
    if (( (*pphdr) = (CVSTRUCT *) csound->Malloc(csound, hSize)) == NULL )
      return(CVE_MALLOC);
    (*pphdr)->magic = CVMAGIC;
    (*pphdr)->headBsize = hSize;
    (*pphdr)->dataBsize = dataBsize;
    (*pphdr)->dataFormat= dataFormat;
    (*pphdr)->samplingRate = srate;
    (*pphdr)->src_chnls  = src_chnls;
    (*pphdr)->channel  = channel;
    (*pphdr)->Hlen = Hlen;
    (*pphdr)->Format = Format;
    /* leave info bytes undefined */
    return(CVE_OK);
}

/* module interface */

int cvanal_init_(CSOUND *csound)
{
    int retval = csound->AddUtility(csound, "cvanal", cvanal);
    if (!retval) {
      retval =
        csound->SetUtilityDescription(csound, "cvanal",
                                      Str("Soundfile analysis for convolve"));
    }
    return retval;
}

