/*
    pvadd.c:

    Copyright (C) 1998 Richard Karpen

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

/******************************************/
/* The applications in this file were     */
/* designed and coded by Richard Karpen   */
/* University of Washington, Seattle 1998 */
/******************************************/
/*    PVADD.C        */

#include "cs.h"
#include <math.h>
#include "dsputil.h"
#include "pvoc.h"
#include "pvocext.h"
#include "pvadd.h"
#include "oload.h"


/* This is used in pvadd instead of the Fetch() from dsputil.c */
void FetchInForAdd(MYFLT *inp, MYFLT *buf, long fsize,
                MYFLT  pos, int binoffset, int maxbin, int binincr)
{
    long    j;
    MYFLT   *frm0,*frm1;
    long    base;
    MYFLT   frac;

    base = (long)pos;
    frac = ((MYFLT)(pos - (MYFLT)base));
    /* & how close to get to next */
    frm0 = inp + ((long)fsize+2L)*base;
    frm1 = frm0 + ((long)fsize+2L);
    if (frac != FL(0.0)) {
      for (j = binoffset; j < maxbin; j+=binincr) {
        buf[2L*j] = frm0[2L*j] + frac*(frm1[2L*j]-frm0[2L*j]);
        buf[2L*j+1L] = frm0[2L*j+1L]
          + frac*(frm1[2L*j+1L]-frm0[2L*j+1L]);
      }
    }
    else {
      for (j = binoffset; j < maxbin; j+=binincr) {
        buf[2L*j] = frm0[2L*j];
        buf[2L*j+1] = frm0[2L*j+1L];
      }
    }
}

int pvaddset(ENVIRON *csound, PVADD *p)
{
    int      i, ibins;
    char     pvfilnam[MAXNAME];
    PVSTRUCT *pvh;
    int      frInc, chans, size;
    MEMFIL   *mfp, *ldmemfile(char *);
    FUNC     *ftp = NULL, *AmpGateFunc = NULL;
    MYFLT    *oscphase;
    long     memsize;

   if (*p->ifn > 0)
     if ((ftp = ftfind(csound, p->ifn)) == NULL)
       return NOTOK;
   p->ftp = ftp;

   if (*p->igatefun > 0)
     if ((AmpGateFunc = ftfind(csound, p->igatefun)) == NULL)
       return NOTOK;
    p->AmpGateFunc = AmpGateFunc;

    if (*p->ifilno == SSTRCOD) {                         /* if strg name given */
      extern char *unquote(char *name);
      if (p->STRARG == NULL) strcpy(pvfilnam,unquote(currevent->strarg));
      else strcpy(pvfilnam, unquote(p->STRARG));
    }
    else if ((long)*p->ifilno <= strsmax && strsets != NULL &&
             strsets[(long)*p->ifilno])
      strcpy(pvfilnam, strsets[(long)*p->ifilno]);
    else sprintf(pvfilnam,"pvoc.%d", (int)*p->ifilno);
    if ((mfp = p->mfp) == NULL || strcmp(mfp->filename, pvfilnam) != 0)
      if ( (mfp = ldmemfile(pvfilnam)) == NULL) {
        sprintf(errmsg,Str("PVADD cannot load %s"), pvfilnam);
        goto pverr;
      }

    pvh = (PVSTRUCT *)mfp->beginp;
    if (pvh->magic != PVMAGIC) {
      sprintf(errmsg,Str("%s not a PVOC file (magic %ld)"),
              pvfilnam, pvh->magic );
      goto pverr;
    }

    chans    = pvh->channels;
    p->frSiz = pvh->frameSize;
    p->frPtr = (MYFLT *) ((char *)pvh+pvh->headBsize);
    p->maxFr = -1 + ( pvh->dataBsize / (chans * (p->frSiz+2) * sizeof(MYFLT)));

    if (*p->imode == 1 || *p->imode == 2)
      memsize = (long)(MAXBINS+PVFFTSIZE+PVFFTSIZE +
                       ((p->frSiz+2L) * (p->maxFr+2)));
    else
      memsize = (long)(MAXBINS+PVFFTSIZE+PVFFTSIZE);

    if (p->auxch.auxp == NULL || memsize != p->mems) {
      MYFLT *fltp;
      auxalloc((memsize * sizeof(MYFLT)), &p->auxch);
      fltp = (MYFLT *) p->auxch.auxp;
      p->oscphase = fltp;      fltp += MAXBINS;
      p->buf = fltp;
      if (*p->imode == 1 || *p->imode == 2) {
        fltp += PVFFTSIZE * 2;
        p->pvcopy = fltp;
      }
    }
    p->mems=memsize;

    if ((p->asr = pvh->samplingRate) != esr &&
        (O.msglevel & WARNMSG)) { /* & chk the data */
      printf(Str("WARNING: %s''s srate = %8.0f, orch's srate = %8.0f\n"),
             pvfilnam, p->asr, esr);
    }
    if (pvh->dataFormat != PVMYFLT) {
      sprintf(errmsg,Str("unsupported PV data format %ld in %s"),
              pvh->dataFormat, pvfilnam);
      goto pverr;
    }
    if (p->frSiz > PVFRAMSIZE) {
      sprintf(errmsg,Str("PV frame %d bigger than %ld in %s"),
              p->frSiz, PVFRAMSIZE, pvfilnam);
      goto pverr;
    }
    if (p->frSiz < 128) {
      sprintf(errmsg,Str("PV frame %ld seems too small in %s"),
              p->frSiz, pvfilnam);
      goto pverr;
    }
    if (chans != 1) {
      sprintf(errmsg,Str("%d chans (not 1) in PVOC file %s"),
              chans, pvfilnam);
      goto pverr;
    }

    frInc    = pvh->frameIncr;
    p->frPrtim = esr/((MYFLT)frInc);
    /* factor by which to mulitply 'real' time index to get frame index */

    size = pvfrsiz(p);
    p->prFlg = 1;    /* true */


   if (*p->igatefun > 0)
     p->PvMaxAmp = PvocMaxAmp(p->frPtr,size, p->maxFr);

   if (*p->imode == 1 || *p->imode == 2) {
     SpectralExtract(p->frPtr, p->pvcopy, size, p->maxFr, (int)*p->imode, *p->ifreqlim);
     p->frPtr = (MYFLT *)p->pvcopy;
   }

    oscphase = p->oscphase;

    for (i=0; i < MAXBINS; i++)
      *oscphase++ = FL(0.0);

    ibins = (*p->ibins==0 ? size/2 : (int)*p->ibins);
    p->maxbin = ibins + (int)*p->ibinoffset;
    p->maxbin = (p->maxbin > size/2 ? size/2 : p->maxbin);
    /*  printf("maxbin=%d\n", p->maxbin); fflush(stdout); */

    return OK;

 pverr:
    return initerror(errmsg);
}

int pvadd(ENVIRON *csound, PVADD *p)
{
    MYFLT  *ar, *ftab;
    MYFLT frIndx;
    int    size = pvfrsiz(p);
    int i, binincr=(int)*p->ibinincr,  nsmps=ksmps;
    MYFLT amp, v1, fract, *oscphase;
    long phase, incr;
    FUNC *ftp;
    long lobits;
/*     int mode = (int)*p->imode; */


    if (p->auxch.auxp==NULL) {
      return perferror(Str("pvadd: not initialised"));
    }

    ftp = p->ftp;
    if (ftp==NULL) {
      return perferror(Str("pvadd: not initialised"));
    }

    if ((frIndx = *p->ktimpnt * p->frPrtim) < 0) {
      return perferror(Str("PVADD timpnt < 0"));
    }
    if (frIndx > p->maxFr) { /* not past last one */
      frIndx = (MYFLT)p->maxFr;
      if (p->prFlg) {
        p->prFlg = 0;   /* false */
        if (O.msglevel & WARNMSG)
          printf(Str("WARNING: PVADD ktimpnt truncated to last frame"));
      }
    }
    FetchInForAdd(p->frPtr,p->buf, size, frIndx,
                  (int)*p->ibinoffset, p->maxbin, binincr);

    if (*p->igatefun > 0)
      PvAmpGate(p->buf,p->maxbin*2, p->AmpGateFunc, p->PvMaxAmp);

    ar = p->rslt;
    for (i=0; i<nsmps; i++) *ar++ = FL(0.0);
    oscphase = p->oscphase;
    for (i = (int)*p->ibinoffset; i < p->maxbin; i+=binincr) {
      lobits = ftp->lobits;
      nsmps = ksmps;
      ar = p->rslt;
      phase = (long)*oscphase;
      if (p->buf[i*2+1] == 0 || p->buf[i*2+i] == p->asr*.5) {
        incr = 0;               /* Hope then does not matter */
        amp = FL(0.0);
      }
      else {
        incr = (long)(p->buf[i*2+1] * *p->kfmod * sicvt);
        amp = p->buf[i*2];
      }
      do {
        fract = PFRAC(phase);
        ftab = ftp->ftable + (phase >> lobits);
        v1 = *ftab++;
        *ar += (v1 + (*ftab - v1) * fract) * amp;
        ar++;
        phase += incr;
        phase &= PHMASK;
      } while (--nsmps);
      *oscphase = (MYFLT)phase;
      oscphase++;
    }
    return OK;
}




