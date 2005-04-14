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

#include "cs.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "soundio.h"
#include "dsputil.h"
#include "convolve.h"

static void takeFFT(SOUNDIN *inputSound, CVSTRUCT *outputCVH,
                     long Hlenpadded, SNDFILE *infd, int ofd);
static int quit(ENVIRON*,char *msg);
static int CVAlloc(CVSTRUCT**, long, int, MYFLT, int, int, long, int, int);
int csoundYield(void *csound);

#define SF_UNK_LEN      -1      /* code for sndfile len unkown  */

#define FIND(MSG)   if (*s == '\0')  \
                        if (!(--argc) || ((s = *++argv) && *s == '-'))  \
                            return quit(csound,MSG);

int cvanal(int argc, char **argv)
{
    ENVIRON *csound = &cenviron;
    CVSTRUCT *cvh;
    char    *infilnam, *outfilnam;
    SNDFILE *infd;
    int     ofd, err, channel = ALLCHNLS;
    SOUNDIN *p;  /* space allocated by SAsndgetset() */

    MYFLT   beg_time = FL(0.0), input_dur = FL(0.0), sr = FL(0.0);
    long    Estdatasiz,Hlen;
    long    Hlenpadded = 1;
    long    nb;

    /* must set this for 'standard' behaviour when analysing
       (assume re-entrant Csound) */
    dbfs_init(csound, DFLT_DBFS);

    if (!(--argc)) {
      return quit(csound,Str("insufficient arguments"));
    }
    do {
      char *s = *++argv;
      if (*s++ == '-')
        switch (*s++) {
        case 's':
          FIND(Str("no sampling rate"))
#ifdef USE_DOUBLE
          sscanf(s,"%lf",&sr);
#else
          sscanf(s,"%f",&sr);
#endif
          break;
        case 'c':
          FIND(Str("no channel"))
            sscanf(s,"%d",&channel);
          if ((channel < 1) || (channel > 4))
            return quit(csound,Str("channel must be in the range 1 to 4"));
          break;
        case 'b':
          FIND(Str("no begin time"))
#ifdef USE_DOUBLE
          sscanf(s,"%lf",&beg_time);
#else
          sscanf(s,"%f",&beg_time);
#endif
          break;
        case 'd':
          FIND(Str("no duration time"))
#ifdef USE_DOUBLE
          sscanf(s,"%lf",&input_dur);
#else
          sscanf(s,"%f",&input_dur);
#endif
          break;
        default:   return quit(csound,Str("unrecognised switch option"));
        }
      else break;
    } while (--argc);

    if (argc !=  2) return quit(csound,Str("illegal number of filenames"));
    infilnam = *argv++;
    outfilnam = *argv;

    if ((infd = SAsndgetset(csound, infilnam, &p,
                            &beg_time, &input_dur, &sr, channel)) < 0) {
      sprintf(errmsg,Str("error while opening %s"), retfilnam);
      return quit(csound,errmsg);
    }
    sr = (MYFLT)p->sr;

    Hlen = p->getframes;
    while (Hlenpadded < 2*Hlen-1)
      Hlenpadded <<= 1;

    Estdatasiz = (Hlenpadded + 2) * sizeof(MYFLT);
    if (channel == ALLCHNLS)
      Estdatasiz *= p->nchanls;

    /* alloc & fill CV hdrblk */
    if ((err = CVAlloc(&cvh, Estdatasiz, CVMYFLT, sr, p->nchanls,channel,
                       Hlen,CVRECT,4))) {
      csound->Message(csound, Str("cvanal: Error allocating header\n"));
      exit(1);
    }

    if ((ofd = openout(outfilnam, 1)) < 0)     /* open the output CV file */
      return quit(csound,Str("cannot create output file"));
                                               /* & wrt hdr into the file */
    if ((nb = write(ofd,(char *)cvh,(int)cvh->headBsize)) < cvh->headBsize)
      return quit(csound,Str("cannot write header"));

    takeFFT(p, cvh, Hlenpadded,infd, ofd);

/*      outputPVH->dataBsize = oframeAct * fftfrmBsiz; */
/*      PVCloseWrHdr(ftFile, outputPVH); */    /* Rewrite dataBsize, Close files */

    sf_close(infd);
    close(ofd);
    return 0;
}

static int quit(ENVIRON *csound,char *msg)
{
    csound->Message(csound,Str("cvanal error: %s\n"), msg);
    csound->Message(csound,Str("Usage: cvanal [-d<duration>] "
                         "[-c<channel>] [-b<begin time>] <input soundfile>"
                         " <output impulse response FFT file> \n"));
    return -1;
}

static void takeFFT(
    SOUNDIN     *p,
    CVSTRUCT    *cvh,
    long        Hlenpadded,
    SNDFILE     *infd,
    int         ofd)
{
    ENVIRON *csound = &cenviron;
    int     i, j, read_in;
    MYFLT   *inbuf,*outbuf;
    MYFLT   *fp1,*fp2;
    int     Hlen = (int) cvh->Hlen;
    int     nchanls;

    nchanls = cvh->channel != ALLCHNLS ? 1 : cvh->src_chnls;
    j = (int) (Hlen * nchanls);
    inbuf = fp1 = (MYFLT *) csound->Malloc(csound, j * sizeof(MYFLT));
    if ((read_in = csound->getsndin(csound, infd, inbuf, j, p)) < j)
      csound->Die(csound, Str("less sound than expected!"));

    /* normalize the samples read in. */
    for (i = read_in; i--; ) {
      *fp1++ *= csound->dbfs_to_float;
    }

    fp1 = inbuf;
    outbuf = fp2 = (MYFLT *)MakeBuf(Hlenpadded + 2);

    for (i = 0; i < nchanls; i++) {
      for (j = Hlen; j > 0; j--) {
        *fp2++ = *fp1;
        fp1 += nchanls;
      }
      fp1 = inbuf + i + 1;
      csound->RealFFT(csound, outbuf, (int) Hlenpadded);
      outbuf[Hlenpadded] = outbuf[1];
      outbuf[1] = outbuf[Hlenpadded + 1L] = FL(0.0);
      if (!csoundYield(csound)) exit(1);
      /* write straight out, just the indep vals */
      write(ofd, (char *)outbuf, cvh->dataBsize/nchanls);
      for (j = Hlenpadded - Hlen; j > 0; j--)
        *fp2++ = FL(0.0);
      fp2 = outbuf;
    } /* end i loop */
}

static int CVAlloc(
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
    long        hSize;

    hSize = sizeof(CVSTRUCT) + infoBsize - CVDFLTBYTS;
    if (( (*pphdr) = (CVSTRUCT *)malloc((size_t)hSize)) == NULL )
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

