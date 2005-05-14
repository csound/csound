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
#include "pvoc.h"
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
    MYFLT   *frm0,*frm1;
    long    base;
    MYFLT   frac;
    long    twmybin = mybin+mybin; /* Always used thus */

    /***** WITHOUT INFO ON WHERE LAST FRAME IS, MAY 'INTERP' BEYOND IT ****/
    base = (long)pos;               /* index of basis frame of interpolation */
    frac = ((MYFLT)(pos - (MYFLT)base));
    /* & how close to get to next */
    frm0 = inp + ((long)fsize+2L)*base;
    frm1 = frm0 + ((long)fsize+2L);         /* addresses of both frames */
    if (frac != 0.0) {          /* must have 2 cases to avoid poss seg vlns */
                                /* and failed computes, else may interp   */
                                /* bd valid data */
      buf[0] = frm0[twmybin] + frac*(frm1[twmybin]-frm0[twmybin]);
      buf[1L] = frm0[twmybin+1L] + frac*(frm1[twmybin+1L]-frm0[twmybin+1L]);
    }
    else {
        /* frac is 0.0 i.e. just copy the source frame */
      buf[0] = frm0[twmybin];
      buf[1L] = frm0[twmybin+1L];
    }
}


int pvreadset(ENVIRON *csound, PVREAD *p)
{
    char     pvfilnam[64];
    MEMFIL   *mfp;
    PVSTRUCT *pvh;
    int     frInc, chans, size; /* THESE SHOULD BE SAVED IN PVOC STRUCT */

    csound->strarg2name(csound, pvfilnam, p->ifilno, "pvoc.", p->XSTRCODE);
    if ((mfp = p->mfp) == NULL ||
        strcmp(mfp->filename, pvfilnam) != 0) { /* if file not already readin */
      /* RWD: try for pvocex first */
      if (pvocex_loadfile(csound, pvfilnam,p,&mfp))     {    /* got it */
        p->mfp = mfp;
        return OK;
      }
      if ( (mfp = ldmemfile(csound, pvfilnam)) == NULL) {
        sprintf(csound->errmsg, Str("PVREAD cannot load %s"), pvfilnam);
        goto pverr;
      }
    }
    else
      /* need to check format again here */
      if (pvocex_loadfile(csound, pvfilnam,p,&mfp)) {
        /* handles all config settings */
        p->mfp = mfp;
        return OK;
      }
    /* only old-format stuff from here */
    if (p->fftBuf.auxp  == NULL) /* Allocate space dynamically */
      csound->AuxAlloc(csound, sizeof(MYFLT)*PVFFTSIZE, &p->fftBuf);
    pvh = (PVSTRUCT *)mfp->beginp;
    if (pvh->magic != PVMAGIC) {
      sprintf(csound->errmsg, Str("%s not a PVOC file (magic %ld)"),
                              pvfilnam, pvh->magic );
      goto pverr;
    }
    p->frSiz = pvh->frameSize;
    frInc    = pvh->frameIncr;
    chans    = pvh->channels;
    if ((p->asr = pvh->samplingRate) != csound->esr &&
        (csound->oparms->msglevel & WARNMSG)) { /* & chk the data */
      csound->Message(csound, Str("WARNING: %s''s srate = %8.0f, orch's srate = %8.0f\n"),
             pvfilnam, p->asr, csound->esr);
    }
    if (pvh->dataFormat != PVMYFLT) {
      sprintf(csound->errmsg, Str("unsupported PVOC data format %ld in %s"),
                              pvh->dataFormat, pvfilnam);
      goto pverr;
    }
    if (p->frSiz > PVFRAMSIZE) {
      sprintf(csound->errmsg, Str("PVOC frame %d bigger than %ld in %s"),
                              p->frSiz, PVFRAMSIZE, pvfilnam);
      goto pverr;
    }
    if (p->frSiz < 128) {
      sprintf(csound->errmsg, Str("PVOC frame %ld seems too small in %s"),
                              p->frSiz, pvfilnam);
      goto pverr;
    }
    if (chans != 1) {
      sprintf(csound->errmsg, Str("%d chans (not 1) in PVOC file %s"),
                              chans, pvfilnam);
      goto pverr;
    }
    /* Check that pv->frSiz is a power of two too ? */
    p->frPtr = (MYFLT *) ((char *)pvh+pvh->headBsize);
    p->baseFr = 0;  /* point to first data frame */
    p->maxFr = -1 + (pvh->dataBsize / (chans * (p->frSiz+2) * sizeof(MYFLT)));
    /* highest possible frame index */
    /* factor by which to mult expand phase diffs (ratio of samp spacings) */
    p->frPrtim = csound->esr/((MYFLT)frInc);
    size = pvfrsiz(p);          /* size used in def of OPWLEN ? */
    p->prFlg = 1;    /* true */
    p->mybin = (long)(*p->ibin-FL(1.0));

    return OK;
 pverr:
    return csound->InitError(csound, csound->errmsg);
}

int pvread(ENVIRON *csound, PVREAD *p)
{
    MYFLT  frIndx;
    MYFLT  *buf = (MYFLT*)(p->fftBuf.auxp);
/*           int      asize = pvdasiz(p); */ /* fix */
    int    size = pvfrsiz(p);

/*     if (pdebug) { csound->Message(csound, "<%7.4f>",*p->ktimpnt); fflush(stdout); } */

    if ((frIndx = *p->ktimpnt * p->frPrtim) < 0) {
      return csound->PerfError(csound, Str("PVOC timpnt < 0"));
    }
    if (frIndx > p->maxFr) {  /* not past last one */
      frIndx = (MYFLT)p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;   /* false */
        if (csound->oparms->msglevel & WARNMSG)
          csound->Message(csound, Str("WARNING: PVOC ktimpnt truncated to last frame"));
      }
    }
    FetchInOne(p->frPtr,buf,size,frIndx, p->mybin);
    *p->kfreq = buf[1];
    *p->kamp = buf[0];
    return OK;
 }


/* PVOCEX read into memory */
/* NB!!!! there are two separate structures to read into:
        PVOC, and PVREAD.  This is PVREAD version.
 */
/* convert pvocex data into amp+freq format Csound expects. Later, we can load
 * the stipulated window too... */
/* generic read error messages in pvfileio.c */
/* NB no death here on read failure, only on memory failure */

static int pvocex_loadfile(ENVIRON *csound,
                           const char *fname,PVREAD *p,MEMFIL **mfp)
{
    PVOCDATA pvdata;
    WAVEFORMATEX fmt;
    MEMFIL *mfil = NULL;
    int i,j,rc=0,pvx_id = -1;
    long pvx_fftsize;
    long mem_wanted = 0;
    long totalframes,framelen;
    float *memblock = NULL;
    float cs_ampfac;               /* needs to be overlapsamps */
    float *pFrame;

    pvx_id = pvoc_openfile(csound,fname,&pvdata,&fmt);
    if (pvx_id < 0)
      return 0;

    /* fft size must be <= PVFRAMSIZE (=8192) for Csound */
    pvx_fftsize = 2 * (pvdata.nAnalysisBins-1);
    framelen = 2 * pvdata.nAnalysisBins;
    if (pvx_fftsize > PVFRAMSIZE) {
      sprintf(csound->errmsg,
              "pvoc-ex file %s: FFT size %ld too large for Csound\n",
              fname, pvx_fftsize);
      return 0;
    }

    /* have to reject m/c files for now, until opcodes upgraded*/
    if (fmt.nChannels > 1) {
      sprintf(csound->errmsg, "pvoc-ex file %s is not mono\n", fname);
      return 0;
    }

    /* also, accept only 32bit floats for now */
    if (pvdata.wWordFormat == PVOC_IEEE_FLOAT) {
      sprintf(csound->errmsg, "pvoc-ex file %s is not 32bit floats\n", fname);
      return 0;
    }

    /* FOR NOW, accept only PVOC_AMP_FREQ : later, we can convert */
    /* NB Csound knows no other: frameFormat is not read anywhere! */
    if (pvdata.wAnalFormat != PVOC_AMP_FREQ) {
      sprintf(csound->errmsg, "pvoc-ex file %s not in AMP_FREQ format\n",fname);
      return 0;
    }
    /* ignore the window spec until we can use it! */
    totalframes = pvoc_framecount(pvx_id);

    if (totalframes == 0) {
      sprintf(csound->errmsg, "pvoc-ex file %s is empty!\n", fname);
      return 0;
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
        rc = pvoc_getframes(pvx_id,pFrame,1);
        if (rc != 1)
          break;        /* read error, but may still have something to use */
        /* scale amps */
        for (j=0;j < framelen; j+=2)
          pFrame[j] *= cs_ampfac;
        pFrame += framelen;
      }
      pvoc_closefile(csound,pvx_id);
      if (rc <0) {
        sprintf(csound->errmsg, "error reading pvoc-ex file %s\n", fname);
        mfree(csound, memblock);
        mfree(csound, p->fftBuf.auxp);
        return 0;
      }
      if (i < totalframes) {
        sprintf(csound->errmsg,
                "error reading pvoc-ex file %s after %d frames\n",
                fname, i);
        mfree(csound, memblock);
        mfree(csound, p->fftBuf.auxp);
        return 0;
      }
    }
    else
      memblock = (float *) mfil->beginp;

    if ((p->asr = (MYFLT) fmt.nSamplesPerSec) != csound->esr &&
        (csound->oparms->msglevel & WARNMSG)) { /* & chk the data */
      csound->Message(csound, Str("WARNING: %s''s srate = %8.0f, orch's srate = %8.0f\n"),
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
    if (mfil==NULL) {
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
             fname,mem_wanted);
      add_memfil(csound, mfil);
    }
    *mfp = mfil;
    return 1;
}

