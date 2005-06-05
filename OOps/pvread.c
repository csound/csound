/*
    pvread.c:

    Copyright (C) 1992, 2000 Richard Karpen, Richard Dobson

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

/**************************************************************/
/***********pvread ********************************************/
/*** By Richard Karpen - July-October 1992*********************/
/**************************************************************/
#include "cs.h"
#include "dsputil.h"
#include "pvread.h"
#include "soundio.h"
#include <math.h>
#include "oload.h"

/*RWD 10:9:2000 read pvocex file format */
#include "pvfileio.h"
static int pvocex_loadfile(ENVIRON *, const char *fname,PVREAD *p,MEMFIL **mfp);

#define WLN   1         /* time window is WLN*2*ksmps long */
#define OPWLEN (2*WLN*ksmps)    /* manifest used for final time wdw */

void FetchInOne(
    MYFLT   *inp,       /* pointer to input data */
    MYFLT   *buf,       /* where to put our nice mag/pha pairs */
    long    fsize,      /* frame size we're working with */
    MYFLT   pos,        /* fractional frame we want */
    long    mybin)
{
    MYFLT   *frame0;
    MYFLT   *frame1;
    long    base;
    MYFLT   frac;
    long    twmybin = mybin+mybin; /* Always used thus */

    /***** WITHOUT INFO ON WHERE LAST FRAME IS, MAY 'INTERP' BEYOND IT ****/
    base = (long)pos;               /* index of basis frame of interpolation */
    frac = ((MYFLT)(pos - (MYFLT)base));
    /* & how close to get to next */
    frame0 = inp + ((long)fsize+2L)*base;
    frame1 = frame0 + ((long)fsize+2L);         /* addresses of both frames */
    if (frac != 0.0) {          /* must have 2 cases to avoid poss seg vlns */
                                /* and failed computes, else may interp   */
                                /* bd valid data */
      buf[0] = frame0[twmybin] + frac*(frame1[twmybin]-frame0[twmybin]);
      buf[1L] = frame0[twmybin+1L] + frac*(frame1[twmybin+1L]-frame0[twmybin+1L]);
    }
    else {
        /* frac is 0.0 i.e. just copy the source frame */
      buf[0] = frame0[twmybin];
      buf[1L] = frame0[twmybin+1L];
    }
}

int pvreadset(ENVIRON *csound, PVREAD *p)
{
    char      pvfilnam[256];
    MEMFIL    *mfp;

    csound->strarg2name(csound, pvfilnam, p->ifilno, "pvoc.", p->XSTRCODE);
    if (pvocex_loadfile(csound, pvfilnam, p, &mfp) == OK) {     /* got it */
      p->mfp = mfp;
      return OK;
    }
    return NOTOK;
}

int pvread(ENVIRON *csound, PVREAD *p)
{
    MYFLT  frIndx;
    MYFLT  *buf = (MYFLT*)(p->fftBuf.auxp);
    int    size = pvfrsiz(p);

    if ((frIndx = *p->ktimpnt * p->frPrtim) < 0) {
      return csound->PerfError(csound, Str("PVOC timpnt < 0"));
    }
    if (frIndx > p->maxFr) {  /* not past last one */
      frIndx = (MYFLT)p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;   /* false */
        csound->Warning(csound, Str("PVOC ktimpnt truncated to last frame"));
      }
    }
    FetchInOne(p->frPtr,buf,size,frIndx, p->mybin);
    *p->kfreq = buf[1];
    *p->kamp = buf[0];
    return OK;
 }

/* PVOCEX read into memory
 * NB!!!! there are two separate structures to read into:
          PVOC, and PVREAD.  This is PVREAD version.
 * convert pvocex data into amp+freq format Csound expects. Later, we can load
 * the stipulated window too...
 * generic read error messages in pvfileio.c
 * NB no death here on read failure, only on memory failure */

static int pvocex_loadfile(ENVIRON *csound,
                           const char *fname,PVREAD *p,MEMFIL **mfp)
{
    PVOCDATA pvdata;
    WAVEFORMATEX fmt;
    MEMFIL  *mfil = NULL;
    int     i, j, rc = 0, pvx_id = -1;
    long    pvx_fftsize;
    long    mem_wanted = 0;
    long    totalframes, framelen;
    float   *memblock = NULL;
    float   cs_ampfac;                  /* needs to be overlapsamps */
    float   *pFrame;

    pvx_id = csound->PVOC_OpenFile(csound, fname, &pvdata, &fmt);
    if (pvx_id < 0) {
      csound->InitError(csound, Str("PVREAD cannot load %s"), fname);
      return NOTOK;
    }
    /* fft size must be <= PVFRAMSIZE (=8192) for Csound */
    pvx_fftsize = 2 * (pvdata.nAnalysisBins-1);
    framelen = 2 * pvdata.nAnalysisBins;
    if (pvx_fftsize > PVFRAMSIZE) {
      csound->InitError(csound,
                        "pvoc-ex file %s: FFT size %ld too large for Csound",
                        fname, pvx_fftsize);
      return NOTOK;
    }

    /* have to reject m/c files for now, until opcodes upgraded*/
    if (fmt.nChannels > 1) {
      csound->InitError(csound, Str("pvoc-ex file %s is not mono"), fname);
      return NOTOK;
    }

    /* also, accept only 32bit floats for now */
    if (pvdata.wWordFormat == PVOC_IEEE_FLOAT) {
      csound->InitError(csound, "pvoc-ex file %s is not 32bit floats", fname);
      return NOTOK;
    }

    /* FOR NOW, accept only PVOC_AMP_FREQ : later, we can convert */
    /* NB Csound knows no other: frameFormat is not read anywhere! */
    if (pvdata.wAnalFormat != PVOC_AMP_FREQ) {
      csound->InitError(csound, "pvoc-ex file %s not in AMP_FREQ format",fname);
      return NOTOK;
    }
    /* ignore the window spec until we can use it! */
    totalframes = csound->PVOC_FrameCount(csound, pvx_id);

    if (totalframes == 0) {
      csound->InitError(csound, Str("pvoc-ex file %s is empty!"), fname);
      return NOTOK;
    }
    if (!find_memfile(csound, fname, &mfil)) {
      mem_wanted = totalframes * 2 * pvdata.nAnalysisBins * sizeof(float);
      /* try for the big block first! */
      memblock = (float *) mmalloc(csound, mem_wanted);
      if (p->fftBuf.auxp  == NULL) /* Allocate space dynamically */
        csoundAuxAlloc(csound, sizeof(MYFLT)*PVFFTSIZE, &p->fftBuf);

      /* fill'er-up */
      /* need to loop, as we need to rescale amplitudes for Csound */
      /* still not sure this is right, yet...what about effect of  */
      /* double-window ? */
      cs_ampfac = (float ) (pvdata.dwOverlap * ((float)(framelen-2) /
                                                (float) pvdata.dwWinlen)) ;
      pFrame = memblock;
      for (i=0;i < totalframes;i++) {
        rc = csound->PVOC_GetFrames(csound,pvx_id,pFrame,1);
        if (rc != 1)
          break;        /* read error, but may still have something to use */
        /* scale amps */
        for (j=0;j < framelen; j+=2)
          pFrame[j] *= cs_ampfac;
        pFrame += framelen;
      }
      csound->PVOC_CloseFile(csound, pvx_id);
      if (rc < 0) {
        mfree(csound, memblock);
        mfree(csound, p->fftBuf.auxp);
        csound->InitError(csound, Str("error reading pvoc-ex file %s"), fname);
        return NOTOK;
      }
      if (i < totalframes) {
        mfree(csound, memblock);
        mfree(csound, p->fftBuf.auxp);
        csound->InitError(csound,
                          "error reading pvoc-ex file %s after %d frames\n",
                          fname, i);
        return NOTOK;
      }
    }
    else
      memblock = (float *) mfil->beginp;
    /* & chk the data */
    if ((p->asr = (MYFLT) fmt.nSamplesPerSec) != csound->esr) {
      csound->Warning(csound, Str("%s''s srate = %8.0f, orch's srate = %8.0f"),
                              fname, p->asr, csound->esr);
    }
    p->frSiz = pvx_fftsize;
    p->frPtr = (MYFLT *) memblock;
    p->baseFr = 0;  /* point to first data frame */
    p->maxFr = totalframes - 1;

    /* highest possible frame index */
    /* factor by which to mult expand phase diffs (ratio of samp spacings) */

    p->frPrtim = csound->esr/((MYFLT) pvdata.dwOverlap);
    /*size = pvfrsiz(p);*/              /* size used in def of OPWLEN ? */
    p->prFlg = 1;    /* true */
    p->mybin = (long)(*p->ibin-1.0f);

    /* Need to assign the MEMFIL to p->mfp in calling func */
    if (mfil == NULL) {
      mfil = (MEMFIL *)  mmalloc(csound, sizeof(MEMFIL));
      /* just hope the filename is short enough...! */
      mfil->next = NULL;
      mfil->filename[0] = '\0';
      strcpy(mfil->filename,fname);
      mfil->beginp = (char *) memblock;
      mfil->endp = mfil->beginp + mem_wanted;
      mfil->length = mem_wanted;
      /*from memfiles.c */
      csound->Message(csound, Str("file %s (%ld bytes) loaded into memory\n"),
                              fname, (long) mem_wanted);
      add_memfil(csound, mfil);
    }
    *mfp = mfil;
    return OK;
}

