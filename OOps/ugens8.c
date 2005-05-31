/*
    ugens8.c:

    Copyright (C) 1991, 1998, 2000 Dan Ellis, Richard Karpen, Richard Dobson

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

#include "cs.h"   /*      UGENS8.C        */
#include <math.h>
#include "dsputil.h"
#include "pvoc.h"
#include "pvocext.h"
#include "ugens8.h"
#include "soundio.h"
#include "oload.h"

/*RWD 10:9:2000 read pvocex file format */
#include "pvfileio.h"
static int pvx_loadfile(ENVIRON *, const char *, PVOC *, MEMFIL **);
static int byterev_pvoc(MEMFIL *mfil);
extern void bytrev4(char *buf, int nbytes);
/********************************************/
/* Originated by Dan Ellis, MIT             */
/* Spectral Extraction and Amplitude Gating */
/* added by Richard Karpen, University      */
/* of Washington, Seattle 1998              */
/********************************************/

#define WLN   1                 /* time window is WLN*2*ksmps long  */

#define OPWLEN (2*WLN*csound->ksmps)    /* manifest used for final time wdw */

int pvset(ENVIRON *csound, PVOC *p)
{
    int      i;
    long     memsize;
    char     pvfilnam[MAXNAME];
    MEMFIL   *mfp;
    PVSTRUCT *pvh = NULL;
    int      chans = 1, size; /* THESE SHOULD BE SAVED IN PVOC STRUCT */
    FUNC     *AmpGateFunc = NULL;
    int      old_format = 1;

    csound->strarg2name(csound, pvfilnam, p->ifilno, "pvoc.", p->XSTRCODE);

    if ((mfp = p->mfp) == NULL ||
        strcmp(mfp->filename, pvfilnam) != 0) { /* if file not already readin */
      /*RWD: TRY PVOCEX FILE FIRST... */
      if (pvx_loadfile(csound, pvfilnam, p, &mfp) == OK) {
        old_format = 0;
        p->mfp = mfp;
      }
      else
        if ((mfp = ldmemfile(csound, pvfilnam)) == NULL) {
          sprintf(csound->errmsg,Str("PVOC cannot load %s"), pvfilnam);
          goto pverr;
      }
    }
    else
      /* need to check format again here */
      if (pvx_loadfile(csound, pvfilnam, p, &mfp) == OK) {
        old_format = 0;
        p->mfp = mfp;
      }
    if (old_format) {
      pvh = (PVSTRUCT *)mfp->beginp;
      if (pvh->magic != PVMAGIC) {
        /* try byte-reversal to read Mac on PC and vv*/
        if (byterev_pvoc(mfp) != OK) {
          sprintf(csound->errmsg,Str("%s not a PVOC file (magic %ld)"),
                  pvfilnam, pvh->magic );
          goto pverr;
        }
        else
          csound->Message(csound, Str("applied byte-reversal\n"));
      }
      chans    = pvh->channels;
      p->frSiz = pvh->frameSize;
      p->frPtr = (MYFLT *) ((char *)pvh+pvh->headBsize);
      p->baseFr = 0;  /* point to first data frame */
      p->maxFr = (pvh->dataBsize / (chans * (p->frSiz+2) * sizeof(MYFLT))) - 1;
    }

    if (*p->imode == 1 || *p->imode == 2)
      memsize = (long)(PVDATASIZE + PVFFTSIZE*3 + PVWINLEN +
                       ((p->frSiz+2L) * (p->maxFr+2)));
    else
      memsize = (long)(PVDATASIZE + PVFFTSIZE*3 + PVWINLEN);

    if (p->auxch.auxp == NULL || memsize != p->mems) {
      register MYFLT *fltp;
      csound->AuxAlloc(csound, (memsize * sizeof(MYFLT)), &p->auxch);
      fltp = (MYFLT *) p->auxch.auxp;
      p->lastPhase = fltp;   fltp += PVDATASIZE;    /* and insert addresses */
      p->fftBuf = fltp;      fltp += PVFFTSIZE;
      p->dsBuf = fltp;       fltp += PVFFTSIZE;
      p->outBuf = fltp;      fltp += PVFFTSIZE;
      p->window = fltp;
      if (*p->imode == 1 || *p->imode == 2) {
        fltp += PVWINLEN;
        p->pvcopy = fltp;
      }
    }
    p->mems=memsize;
    if (old_format) {
      if ((p->asr = pvh->samplingRate) != csound->esr &&
          (csound->oparms->msglevel & WARNMSG)) { /* & chk the data */
        csound->Message(csound,
                        Str("WARNING: %s''s srate = %8.0f, "
                            "orch's srate = %8.0f\n"),
                        pvfilnam, p->asr, csound->esr);
      }
      if (pvh->dataFormat != PVMYFLT) {
        sprintf(csound->errmsg,Str("unsupported PVOC data format %ld in %s"),
                pvh->dataFormat, pvfilnam);
        goto pverr;
      }
      if (p->frSiz > PVFRAMSIZE) {
        sprintf(csound->errmsg, Str("PVOC frame %ld bigger than %ld in %s"),
                                p->frSiz, (long) PVFRAMSIZE, pvfilnam);
        goto pverr;
      }
      if (p->frSiz < 128) {
        sprintf(csound->errmsg,Str("PVOC frame %ld seems too small in %s"),
                p->frSiz, pvfilnam);
        goto pverr;
      }
      if (chans != 1) {
        sprintf(csound->errmsg,Str("%d chans (not 1) in PVOC file %s"),
                chans, pvfilnam);
        goto pverr;
      }
      p->frInc    = pvh->frameIncr;
    }

    p->frPktim = ((MYFLT)csound->ksmps)/((MYFLT) p->frInc);
    /* factor by which to mult expand phase diffs (ratio of samp spacings) */
    p->frPrtim = csound->esr/((MYFLT) p->frInc);
    /* factor by which to mulitply 'real' time index to get frame index */
    size = pvfrsiz(p);          /* size used in def of OPWLEN ? */
    p->scale = csound->e0dbfs*FL(2.0)*((MYFLT)csound->ksmps)/((MYFLT)OPWLEN);
    p->scale *= csound->GetInverseRealFFTScale(csound, (int) size);
    /* 2*incr/OPWLEN scales down for win ovlp, windo'd 1ce (but 2ce?) */
    /* 1/frSiz is the required scale down before (i)FFT */
    p->prFlg = 1;    /* true */
    p->opBpos = 0;
    p->lastPex = FL(1.0);     /* needs to know last pitchexp to update phase */
    /* Set up time window */
    for (i=0; i < pvdasiz(p); ++i) {  /* or maybe pvdasiz(p) */
     /* p->window[i] = (0.54-0.46*cos(TWOPI*(MYFLT)i/(MYFLT)(pvfrsiz(p)))); */
      p->lastPhase[i] = FL(0.0);
    }
    if ((OPWLEN/2 + 1)>PVWINLEN ) {
      sprintf(csound->errmsg,
              Str("ksmps of %d needs wdw of %d, max is %d for pv %s\n"),
              csound->ksmps, (OPWLEN/2 + 1), PVWINLEN, pvfilnam);
      goto pverr;
    }

    if (*p->igatefun > 0)
      if ((AmpGateFunc = csound->FTFind(csound, p->igatefun)) == NULL)
        return NOTOK;
    p->AmpGateFunc = AmpGateFunc;

    if (*p->igatefun > 0)
      p->PvMaxAmp = PvocMaxAmp(p->frPtr, size, p->maxFr);

    if (*p->imode == 1 || *p->imode == 2) {
      SpectralExtract(p->frPtr, p->pvcopy, size, p->maxFr,
                      (int)*p->imode, *p->ifreqlim);
      p->frPtr = p->pvcopy;
    }

    for (i=0; i < OPWLEN/2+1; ++i)    /* time window is OPWLEN long */
      p->window[i] = (FL(0.54)-FL(0.46)*
                      (MYFLT)cos(TWOPI*(double)i/(double)OPWLEN));
    /* NB : HAMMING */
    for (i=0; i< pvfrsiz(p); ++i)
      p->outBuf[i] = FL(0.0);
    MakeSinc( /* p->sncTab */ );        /* sinctab is same for all instances */
    return OK;

 pverr:
    return csound->InitError(csound, csound->errmsg);
}

int pvoc(ENVIRON *csound, PVOC *p)
{
    MYFLT  *ar = p->rslt;
    MYFLT  frIndx;
    MYFLT  *buf = p->fftBuf;
    MYFLT  *buf2 = p->dsBuf;
    int    asize = pvdasiz(p);  /* new */
    int    size = pvfrsiz(p);
    int    buf2Size, outlen;
    int    circBufSize = PVFFTSIZE;
    int    specwp = (int)*p->ispecwp;   /* spectral warping flag */
    MYFLT  pex;

    if (p->auxch.auxp==NULL) {
      return csound->PerfError(csound, Str("pvoc: not initialised"));
    }
    pex = *p->kfmod;
    outlen = (int)(((MYFLT)size)/pex);
    /* use outlen to check window/krate/transpose combinations */
    if (outlen>PVFFTSIZE) {  /* Maximum transposition down is one octave */
                             /* ..so we won't run into buf2Size problems */
      return csound->PerfError(csound, Str("PVOC transpose too low"));
    }
    if (outlen<2*csound->ksmps) {    /* minimum post-squeeze windowlength */
      return csound->PerfError(csound, Str("PVOC transpose too high"));
    }
    buf2Size = OPWLEN;       /* always window to same length after DS */
    if ((frIndx = *p->ktimpnt * p->frPrtim) < 0) {
      return csound->PerfError(csound, Str("PVOC timpnt < 0"));
    }
    if (frIndx > p->maxFr) {  /* not past last one */
      frIndx = (MYFLT)p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;   /* false */
        if (csound->oparms->msglevel & WARNMSG)
          csound->Message(csound,
                          Str("WARNING: PVOC ktimpnt truncated to last frame\n"));
      }
    }
    FetchIn(p->frPtr,buf,size,frIndx);

    if (*p->igatefun > 0)
      PvAmpGate(buf,size, p->AmpGateFunc, p->PvMaxAmp);

    FrqToPhase(buf, asize, pex*(MYFLT)csound->ksmps, p->asr,
               FL(0.5) * ( (pex / p->lastPex) - FL(1.0)) );
    /* Offset the phase to align centres of stretched windows, not starts */
    RewrapPhase(buf, asize, p->lastPhase);

    if (specwp>0)
      /* RWD: THIS CAUSED MASSIVE MEMORY ERROR, BUT DOESN'T WORK ANYWAY */
      PreWarpSpec(buf, asize, pex);

    Polar2Rect(buf, asize);
    /* kill spurious imag at dc & fs/2 */
    buf[1] = buf[size]; buf[size] = buf[size + 1] = FL(0.0);
    csound->InverseRealFFT(csound, buf, (int) size);
    if (pex != FL(1.0))
      UDSample(buf,(FL(0.5)*((MYFLT)size - pex*(MYFLT)buf2Size))/*a*/,
               buf2, size, buf2Size, pex);
    else
      CopySamps(buf+(int)(FL(0.5)*((MYFLT)size - pex*(MYFLT)buf2Size))/*a*/,
                buf2,buf2Size);
    ApplyHalfWin(buf2, p->window, buf2Size);
    addToCircBuf(buf2, p->outBuf, p->opBpos, csound->ksmps, circBufSize);
    writeClrFromCircBuf(p->outBuf, ar, p->opBpos, csound->ksmps, circBufSize);
    p->opBpos += csound->ksmps;
    if (p->opBpos > circBufSize)     p->opBpos -= circBufSize;
    addToCircBuf(buf2+csound->ksmps,p->outBuf,
                 p->opBpos,buf2Size-csound->ksmps,circBufSize);
    p->lastPex = pex;        /* needs to know last pitchexp to update phase */
    {
      int i;
      for (i = 0; i < csound->ksmps; i++)
        p->rslt[i] *= p->scale;
    }
    return OK;
}

/* RWD 8:2001: custom version of ldmemfile();
   enables pvfileio funcs to apply byte-reversal if needed.

  this version applies scaling to match  existing  pvanal format
 */
static int pvx_loadfile(ENVIRON *csound,
                        const char *fname, PVOC *p, MEMFIL **mfp)
{
    PVOCDATA pvdata;
    WAVEFORMATEX fmt;
    MEMFIL *mfil = NULL;
    int i,j,rc = 0,pvx_id = -1;
    long pvx_fftsize;
    long mem_wanted = 0;
    long totalframes,framelen;
    float *memblock = NULL;
    float cs_ampfac;               /* needs to be overlapsamps */
    float *pFrame;

    pvx_id = pvoc_openfile(csound,fname,&pvdata,&fmt);
    if (pvx_id < 0)
      return NOTOK;

    /* fft size must be <= PVFRAMSIZE (=8192) for Csound */
    pvx_fftsize = 2 * (pvdata.nAnalysisBins-1);
    framelen = 2 * pvdata.nAnalysisBins;
    if (pvx_fftsize > PVFRAMSIZE) {
      sprintf(csound->errmsg,
              Str("pvoc-ex file %s: FFT size %d too large for Csound\n"),
              fname, (int) pvx_fftsize);
      return NOTOK;
    }

    /* have to reject m/c files for now, until opcodes upgraded*/
    if (fmt.nChannels > 1) {
      sprintf(csound->errmsg,Str("pvoc-ex file %s is not mono\n"),fname);
      return NOTOK;
    }

    /* also, accept only 32bit floats for now */
    if (pvdata.wWordFormat != PVOC_IEEE_FLOAT) {
      sprintf(csound->errmsg,Str("pvoc-ex file %s is not 32bit floats\n"),fname);
      return NOTOK;
    }

    /* FOR NOW, accept only PVOC_AMP_FREQ : later, we can convert */
    /* NB Csound knows no other: frameFormat is not read anywhere! */
    if (pvdata.wAnalFormat != PVOC_AMP_FREQ) {
      sprintf(csound->errmsg,
              Str("pvoc-ex file %s not in AMP_FREQ format\n"),fname);
      return NOTOK;
    }
    /* ignore the window spec until we can use it! */
    totalframes = pvoc_framecount(pvx_id);

    if (totalframes == 0) {
      sprintf(csound->errmsg,Str("pvoc-ex file %s is empty!\n"),fname);
      return NOTOK;
    }
    if (!find_memfile(csound, fname, &mfil)) {
      /* get the memory and load */
      mem_wanted = totalframes * 2 * pvdata.nAnalysisBins * sizeof(float);
      /* try for the big block first! */
      memblock = (float *) mmalloc(csound, mem_wanted);
      /* fill'er-up */
      /* need to loop, as we need to rescale amplitudes for Csound */
      /* still not sure this is right, yet... */
      /* what about effect of double-window ? */
      cs_ampfac = (float ) (pvdata.dwOverlap *
                            ((float)(framelen-2) /(float) pvdata.dwWinlen)) ;

      pFrame = memblock;
      for (i=0;i < totalframes;i++) {
        rc = pvoc_getframes(pvx_id,pFrame,1);
        if (rc != 1)
          break;         /* read error, but may still have something to use */
        /* scale amps */
        for (j=0;j < framelen; j+=2)
          pFrame[j] *= cs_ampfac;
        pFrame += framelen;
      }
      /* so far, not been able to provoke entry to these blocks! */
      if (rc <0) {
        sprintf(csound->errmsg,Str("error reading pvoc-ex file %s\n"),fname);
        mfree(csound, memblock);
        return NOTOK;
      }
      if (i < totalframes) {
        /* if rc==0, there may be an error in the file header */
        sprintf(csound->errmsg,
                Str("error reading pvoc-ex file %s after %d frames\n"),
                fname,i);
        /* be strict. */
        mfree(csound, memblock);
        return NOTOK;
      }
    }
    else
      memblock = (float *) mfil->beginp;
    pvoc_closefile(csound,pvx_id);
    if ((p->asr = (MYFLT) fmt.nSamplesPerSec) != csound->esr &&
        (csound->oparms->msglevel & WARNMSG)) { /* & chk the data */
      csound->Message(csound,
                      Str("WARNING: %s''s srate = %8.0f, orch's srate = %8.0f\n"),
                      fname, p->asr, csound->esr);
    }
    p->frSiz    = pvx_fftsize;
    p->frPtr    = (MYFLT *) memblock;
    p->baseFr   = 0;  /* point to first data frame */
    p->maxFr    = totalframes - 1;
    p->frInc    = pvdata.dwOverlap;
    p->chans    = fmt.nChannels;

    /* highest possible frame index */
    /* factor by which to mult expand phase diffs (ratio of samp spacings) */
    p->frPrtim = csound->esr/((MYFLT) pvdata.dwOverlap);

    /* Need to assign an MEMFIL to p->mfp */
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
    return OK;
}

static int byterev_pvoc(MEMFIL *mfil)
{
    int i;
    PVSTRUCT *pvh;
    long lval, *lptr;
    long nfields,nwords,extra = 0;

    nfields = sizeof(PVSTRUCT) - PVDFLTBYTS;
    nfields /= sizeof(long);

    pvh = (PVSTRUCT *) mfil->beginp;
    lptr = (long *)    mfil->beginp;
    lval = *lptr;
    bytrev4((char *) &lval,4);  /* get magic */
    if (lval != PVMAGIC)
      return NOTOK;         /* can't be byte-reversed pvoc file! */

    /* got here, so byterev whole file*/
    /* can autoreverse most of the header, but need to trap any extra space */
    for (i=0; i < nfields; i++) {
      bytrev4((char *) lptr,4);
      lptr++;
    }

    /* any extra bytes? we just hope if there is, */
    /* the data is still lword-aligned! */
    extra = sizeof(PVSTRUCT) - pvh->headBsize;
    if (extra > 0) {
      if (extra%sizeof(long) != 0) {
/*      printf(Str("pvoc file has bad data alignment\n"));  */
        return NOTOK;
      }
      lptr += (extra / sizeof(long));
    }
    nwords = pvh->dataBsize / sizeof(float);
    /* and reverse rest of data */
    for (i=0; i < pvh->dataBsize /(int)sizeof(float); i++) {
      bytrev4((char *)lptr,4);
      lptr++;
    }
    return OK;
}

