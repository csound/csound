/*
    pstream.c:

    Copyright (C) 2001 Richard Dobson

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

/* TODO: split into two files: pvsimp.c for just the CARL anal/synth code, under LGPL,
   pvsops.c for everythng else, under ????
*/

#include <math.h>
#include "cs.h"
#include "pstream.h"
#include "pvfileio.h"
#include "oload.h"

#ifdef _DEBUG
#include <assert.h>
#endif

#ifdef _WIN32
# define MY_INLINE __inline
# ifdef _MSC_VER
# pragma message ("pvsadsyn: Using inline oscil")
# endif
#else
#define MY_INLINE
#endif


/* TODO: a generic form of this function! Oh for C++... */
static int pvx_loadfile(const char *fname,PVSFREAD *p,MEMFIL **mfp);
extern int find_memfile(const char *fname,MEMFIL **pp_mfp);
extern void add_memfil(MEMFIL *mfp);


int fsigs_equal(const PVSDAT *f1, const PVSDAT *f2)
{
    if ((f1->overlap    == f2->overlap)
        && (f1->winsize == f2->winsize)
        && (f1->wintype == f2->wintype) /* harsh, maybe... */
        && (f1->N       == f2->N)
        && (f1->format  == f2->format)
        )
      return 1;
    return 0;

}

/* Pandora's box opcode, but we can make it at least plausible,
   by forbidding copy to different format. */

int fassign(ENVIRON *csound, FASSIGN *p)
{
    int i;
    long framesize;
    float *fout,*fsrc;
    if (!fsigs_equal(p->fout,p->fsrc))
      die(Str(X_1824,"fsig = : formats are different.\n"));
    fout = (float *) p->fout->frame.auxp;
    fsrc = (float *) p->fsrc->frame.auxp;

    framesize = p->fsrc->N + 2;
    if (p->fout->framecount == p->fsrc->framecount)     /* avoid duplicate copying */
      for (i=0;i < framesize;i++)
        *fout++ = *fsrc++;
    return OK;
}



/************* OSCBANK SYNTH ***********/


int pvadsynset(ENVIRON *csound, PVADS *p)
{
    /* get params from input fsig */
    /* we trust they are legit! */
    int i;
    long N = p->fsig->N;
    long noscs,n_oscs;
    long startbin,binoffset;
    MYFLT *p_x;

    p->overlap = p->fsig->overlap;
    /* a moot question, whether window params are relevant for adsyn?*/
    /* but for now, we validate all params in fsig */
    p->winsize = p->fsig->winsize;
    p->wintype = p->fsig->wintype;
    p->fftsize = N;
    noscs   = N/2 + 1;                     /* max possible */
    n_oscs = (long) *p->n_oscs;    /* user param*/
    if (n_oscs <=0)
      die(Str(X_1573,"pvadsyn: bad value for inoscs\n"));
    /* remove this when I know how to do it... */
    if (p->fsig->format != PVS_AMP_FREQ)
      die(Str(X_1574,"pvadsyn: format must be amp-freq (0).\n"));
    p->format = p->fsig->format;
    /* interesting question: could noscs be krate variable?
     * answer: yes, but adds complexity to oscbank, as have to set/reset each osc when started
     and stopped */

    /* check bin params */
    startbin = (long) *p->ibin;            /* default 0 */
    binoffset = (long) *p->ibinoffset; /* default 1 */
    if (startbin        < 0 || startbin > noscs)
      die(Str(X_1575,"pvsadsyn: ibin parameter out of range.\n"));
    if (startbin + n_oscs > noscs)
      die(Str(X_1576,"pvsadsyn: ibin + inoscs too large.\n"));
    /* calc final max bin target */
    p->maxosc = startbin + (n_oscs * binoffset);
    if (p->maxosc > noscs)
      die(Str(X_1577,"pvsadsyn: ibin + (inoscs * ibinoffset) too large."));

    p->outptr = 0;
    p->lastframe = 0;
/*     p->one_over_sr = (float)onedsr); */
/*     p->pi_over_sr = (float)(pidsr); */
    p->one_over_overlap = (float)(FL(1.0) / p->overlap);
    /* alloc for all oscs; in case we can do something with them dynamically, one day */
    if (p->a.auxp==NULL)
      auxalloc(noscs * sizeof(MYFLT),&p->a);
    if (p->x.auxp==NULL)
      auxalloc(noscs * sizeof(MYFLT),&p->x);
    if (p->y.auxp==NULL)
      auxalloc(noscs * sizeof(MYFLT),&p->y);
    if (p->amps.auxp==NULL)
      auxalloc(noscs * sizeof(MYFLT),&p->amps);
    if (p->lastamps.auxp==NULL)
      auxalloc(noscs * sizeof(MYFLT),&p->lastamps);
    if (p->freqs.auxp==NULL)
      auxalloc(noscs * sizeof(MYFLT),&p->freqs);
    if (p->outbuf.auxp==NULL)
      auxalloc(p->overlap * sizeof(MYFLT),&p->outbuf);
    /* initialize oscbank */
    p_x = (MYFLT *) p->x.auxp;
    for (i=0;i < noscs;i++)
      *p_x++ = FL(1.0);

    return OK;
}
/* c/o John Lazzaro, for SAOL, and many other sources */
static MY_INLINE MYFLT fastoscil(MYFLT *a,MYFLT *x,MYFLT *y)
{
    *x = *x - *a * *y;
    *y = *y + *a * *x;
    if (*y<-FL(1.0))*y=-FL(1.0);          /* expensive, but worth it for evenness?*/
    if (*y>FL(1.0)) *y = FL(1.0);
    return *y;
}


static void adsyn_frame(PVADS *p)
{
    int i,j;
    long startbin,lastbin,binoffset;
    MYFLT *outbuf = (MYFLT *) (p->outbuf.auxp);

    float *frame;        /* RWD MUST be 32bit */
    MYFLT *a,*x,*y;
    MYFLT *amps,*freqs,*lastamps;
    MYFLT ffac = *p->kfmod;
    MYFLT nyquist = esr * FL(0.5);
    /* we add to outbuf, so clear it first*/
    memset(p->outbuf.auxp,0,p->overlap * sizeof(MYFLT));

    frame     = (float *) p->fsig->frame.auxp;
    a         = (MYFLT *) p->a.auxp;
    x         = (MYFLT *) p->x.auxp;
    y         = (MYFLT *) p->y.auxp;
    amps      = (MYFLT *) p->amps.auxp;
    freqs     = (MYFLT *) p->freqs.auxp;
    lastamps  = (MYFLT *) p->lastamps.auxp;
    startbin  = (long)    *p->ibin;
    binoffset = (long)    *p->ibinoffset;
    lastbin   = p->maxosc;

    /*update amps, freqs*/
    for (i=startbin;i < lastbin;i+= binoffset) {
      amps[i] = frame[i*2];
      /* lazy: force all freqs positive! */
      freqs[i] = ffac * (MYFLT) (fabs)(frame[(i*2)+1]);
      /* kill stuff over Nyquist. Need to worry about vlf values? */
      if (freqs[i] > nyquist)
        amps[i] = FL(0.0);
      a[i] = (MYFLT )(2.0 * sin( freqs[i] * (double)pidsr));
    }

    /* we need to interp amplitude, but seems we can avoid doing freqs too, for pvoc
       so can use direct calc for speed.
       But large overlap size is not a good idea. */
    /* if compiler cannot inline fastoscil, would be worth doing so by hand here */
    for (i=startbin;i < lastbin;i+=binoffset) {
      MYFLT thisamp = lastamps[i];
      MYFLT delta_amp = (amps[i] - thisamp) * p->one_over_overlap;

      for (j=0;j < p->overlap;j++) {
        outbuf[j] += thisamp *  fastoscil(a+i,x+i,y+i);
        thisamp += delta_amp;
      }
      lastamps[i] = amps[i];
    }
}



static MYFLT adsyn_tick(PVADS *p)
{
    MYFLT *outbuf = (MYFLT *) (p->outbuf.auxp);

    if (p->outptr== p->fsig->overlap) {
      adsyn_frame(p);
      p->outptr = 0;
      p->lastframe = p->fsig->framecount;
    }
    return  outbuf[p->outptr++];
}


int pvadsyn(ENVIRON *csound, PVADS *p)
{
    int i;

    MYFLT *aout = p->aout;

    if (p->outbuf.auxp==NULL) {
      die(Str(X_1578,"pvsynth: Not initialised.\n"));
    }
    for (i=0;i < ksmps;i++)
      aout[i] = adsyn_tick(p);
    return OK;
}

/******* PVSCROSS ***********/


int pvscrosset(ENVIRON *csound, PVSCROSS *p)
{
    long N = p->fsrc->N;
    /* source fsig */
    p->overlap = p->fsrc->overlap;
    p->winsize = p->fsrc->winsize;
    p->wintype = p->fsrc->wintype;
    p->fftsize = N;
    p->format  = p->fsrc->format;

    /* make sure fdest is same format */
    if (!fsigs_equal(p->fsrc,p->fdest))
      die(Str(X_1579,"pvscross: source and dest signals must have same format\n"));

    /* setup output signal */
    if (p->fout->frame.auxp==NULL)
      auxalloc((N+2)*sizeof(float),&p->fout->frame);     /* RWD MUST be 32bit */
    p->fout->N =  N;
    p->fout->overlap = p->overlap;
    p->fout->winsize = p->winsize;
    p->fout->wintype = p->wintype;
    p->fout->format = p->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    return OK;
}



int pvscross(ENVIRON *csound, PVSCROSS *p)
{
    long i,N = p->fftsize;
    MYFLT amp1 = (MYFLT) fabs(*p->kamp1);
    MYFLT amp2 = (MYFLT) fabs(*p->kamp2);
    float *fsrc = (float *) p->fsrc->frame.auxp;          /* RWD all must be 32bit */
    float *fdest = (float *) p->fdest->frame.auxp;
    float *fout = (float *) p->fout->frame.auxp;

    if (fout==NULL)
      die(Str(X_1580,"pvscross: not initialised\n"));

    /* make sure sigs have same format as output */
    if (!fsigs_equal(p->fout,p->fsrc))
      die(Str(X_1581,"pvscross: mismatch in fsrc format\n"));
    if (!fsigs_equal(p->fout,p->fdest))
      die(Str(X_1582,"pvscross: mismatch in fdest format\n"));

    /* only process when a new frame is ready */
    if (p->lastframe < p->fsrc->framecount) {
#ifdef _DEBUG
      assert(p->fsrc->framecount==p->fdest->framecount);
#endif
      for (i=0;i < N+2;i+=2) {
        fout[i] = (float) ((fsrc[i] * amp1) + (fdest[i] * amp2));
        /* copy src freqs to output, unmodified */
        fout[i+1] = (float) fsrc[i+1];
      }
      p->fout->framecount = p->lastframe = p->fsrc->framecount;
    }
    return OK;
}

/******** PVSFREAD ************/

int pvsfreadset(ENVIRON *csound, PVSFREAD *p)
{
    int      i;
    unsigned long  N;
    char     pvfilnam[MAXNAME];
    MEMFIL   *mfp;
    float       *frptr,*memptr;          /* RWD pvocex format: MUST be 32bit */

    if (*p->ifilno == SSTRCOD) {                     /* if strg name given */
      if (p->STRARG!=NULL)
        strcpy(pvfilnam, p->STRARG);                 /*   use that         */
      else
        strcpy(pvfilnam, unquote(currevent->strarg));
    }
    else if ((long)*p->ifilno < strsmax && strsets != NULL &&
             strsets[(long)*p->ifilno])
      strcpy(pvfilnam, strsets[(long)*p->ifilno]);
    /* do people still use this system? */
    else sprintf(pvfilnam,"pvoc.%d", (int)*p->ifilno); /* else pvoc.filnum   */
    mfp = p->mfp;
    if ((mfp == NULL) || strcmp(mfp->filename, pvfilnam) != 0) {/* if file not already readin */
      if (!pvx_loadfile(pvfilnam,p,&mfp))
        /* or get pvsys error message ? */
        die(errmsg);
      p->mfp = mfp;
    }

    if (p->nframes <= 0)
      die(Str(X_1583,"pvsfread: file is empty!\n"));
    /* special case if only one frame - it is an impulse response */
    if (p->nframes == 1)
      die(Str(X_1584,"pvsfread: file has only one frame (= impulse response).\n"));
    if (p->overlap < ksmps)
      die(Str(X_1585,"pvsfread: analysis frame overlap must be >= ksmps\n"));
    p->blockalign = (p->fftsize+2) * p->chans;
    if ((*p->ichan) >= p->chans)
      die(Str(X_1586,"pvsfread: ichan value exceeds file channel count.\n"));
    if ((long) (*p->ichan) < 0)
      die(Str(X_1587,"pvsfread: ichan cannot be negative.\n"));

    N = p->fftsize;
    /* setup output signal */
    if (p->fout->frame.auxp==NULL)
      auxalloc((N+2)*sizeof(float),&p->fout->frame);
    /* init sig with first frame from file, regardless (always zero amps, but with bin freqs) */
    memptr = (float *) mfp->beginp;                              /* RWD MUST be 32bit */
    frptr = (float *) p->fout->frame.auxp;               /* RWD MUST be 32bit */
    for (i=0;i < p->fftsize+2; i++)
      *frptr++ =   *memptr++;
    p->membase += p->blockalign;        /* move to second frame in file, as startpoint*/
    p->chanoffset = (long) (*p->ichan) * (N+2);
    p->nframes--;
    p->fout->N           =  N;
    p->fout->overlap = p->overlap;
    p->fout->winsize = p->winsize;
    p->fout->wintype = p->wintype;
    p->fout->format  = p->format;
    p->fout->framecount = 1;
    p->lastframe = 0;
    return OK;
}


int pvsfread(ENVIRON *csound, PVSFREAD *p)
{
    int i,j;
    MYFLT pos = *p->kpos;
    MYFLT framepos,frac;
    long frame1pos,frame2pos,framesize,n_mcframes;

    float *pmem,*pframe1,*pframe2;               /* RWD all MUST be 32bit */
    float *fout = (float *) p->fout->frame.auxp;

    if (fout==NULL)
      die(Str(X_1588,"pvsfread: not initialised.\n"));

    pmem = (float *) p->membase;
    framesize = p->fftsize+2;
    n_mcframes = p->nframes / p->chans;

    if (p->ptr>=p->overlap) {
      if (pos < 0.0f)
        pos = 0.0f;     /* or report as error... */
      framepos = pos * p->arate;
      frame1pos = (long) framepos;

      if (frame1pos>= n_mcframes -1) {
        /* just return final frame */
        pframe1 = pmem + (p->blockalign * (n_mcframes -1)) + p->chanoffset;
        for (i=0;i < framesize;i++)
          *fout++ = *pframe1++;
      }
      else {
        /* gotta interpolate */
        /* any optimizations possible here?  Avoid v samll frac values? higher-order interp? */
        frame2pos = frame1pos+p->chans;
        frac = framepos - (MYFLT) frame1pos;
        pframe1 = pmem + (frame1pos * p->blockalign) + p->chanoffset;
        pframe2 = pframe1 + p->blockalign;
        for (i=0;i < framesize;i+=2) {
          MYFLT amp,freq;
          j = i+1;
          amp = pframe1[i] + frac * (pframe2[i] - pframe1[i]);
          freq = pframe1[j] + frac * (pframe2[j] - pframe1[j]);
          fout[i] = (float) amp;
          fout[j] = (float) freq;
        }
      }
      p->ptr -= p->overlap;
      p->fout->framecount++;
      p->lastframe = p->fout->framecount;
    }
    p->ptr += ksmps;


    return OK;
}

/************* PVSMASKA ****************/
int pvsmaskaset(ENVIRON *csound, PVSMASKA *p)
{
    int i;
    MYFLT *ftable;
    long N = p->fsrc->N;
    long nbins = N/2 + 1;
    /* source fsig */
    p->overlap = p->fsrc->overlap;
    p->winsize = p->fsrc->winsize;
    p->wintype = p->fsrc->wintype;
    p->format  = p->fsrc->format;
    p->fftsize = N;
    if (!(p->format==PVS_AMP_FREQ) || (p->format==PVS_AMP_PHASE))
      die(Str(X_1589,"pvsmaska: signal format must be amp-phase or amp-freq.\n"));
    /* setup output signal */
    if (p->fout->frame.auxp==NULL)
      auxalloc((N+2)*sizeof(float),&p->fout->frame);    /* RWD MUST be 32bit */
    p->fout->N =  N;
    p->fout->overlap = p->overlap;
    p->fout->winsize = p->winsize;
    p->fout->wintype = p->wintype;
    p->fout->format  = p->format;
    p->fout->framecount = 1;
    p->lastframe = 0;

    p->maskfunc = ftfind(p->h.insdshead->csound, p->ifn);
    if (p->maskfunc==NULL)
      return NOTOK;

    /* require at least size of nbins */
    if (p->maskfunc->flen + 1 <         nbins)
      die(Str(X_1590,"pvsmaska: ftable too small.\n"));

    /* clip any negative values in table */
    ftable = p->maskfunc->ftable;
    for (i=0;i < p->maskfunc->flen+1;i++)
      if (ftable[i] < FL(0.0)) ftable[i] = FL(0.0);

    p->nwarned = p->pwarned = 0;
    return OK;
}



int pvsmaska(ENVIRON *csound, PVSMASKA *p)
{
    int i;
    long flen, nbins;
    MYFLT *ftable;
    float *fout,*fsrc;                                          /* RWD MUST be 32bit */
    float margin,depth = (float)*p->kdepth;


    flen = p->maskfunc->flen + 1;
    ftable = p->maskfunc->ftable;
    fout = (float *) p->fout->frame.auxp;  /* RWD both MUST be 32bit */
    fsrc = (float *) p->fsrc->frame.auxp;

    if (fout==NULL)
      die(Str(X_1591,"pvsmaska: not initialised\n"));

    if (depth < FL(0.0)) {
      /* need the warning: but krate linseg can give below-zeroes incorrectly */
      if (!p->nwarned)  {
        if (O.msglevel & WARNMSG)
          printf(Str(X_1592,
                     "WARNING: pvsmaska: negative value for kdepth; clipped to zero.\n"));
        p->nwarned = 1;
      }
      depth = FL(0.0);
    }
    if (depth > 1.0f) {
      if (!p->pwarned)  {
        if (O.msglevel & WARNMSG)
          printf(Str(X_1593,"WARNING: pvsmaska: kdepth > 1: clipped.\n"));
        p->pwarned = 1;
      }
      depth = FL(1.0);
    }
    /* massage into internal mapping */
    /* user 0 --> my 1
       user 1 ---> my 0
       user 0.75 -->my 0.25
    */
    depth = (float)(FL(1.0) - depth);

    margin = (float)(FL(1.0) - depth);

    nbins = p->fftsize/2 + 1;
    /* only process when a new frame is ready */
    if (p->lastframe < p->fsrc->framecount) {
      MYFLT amp = 1.0f;
      int aindex, findex;
      for (i=0;i < nbins;i++) {
        amp = depth + margin * ftable[i];
        aindex = i*2;
        findex = aindex +1;
        fout[aindex] = (float)(fsrc[aindex] * amp);
        /* freqs are unmodified */
        fout[findex] = (float) fsrc[findex];
      }

      p->fout->framecount = p->lastframe = p->fsrc->framecount;
    }
    return OK;
}

/**************PVSFTW ******************/

int pvsftwset(ENVIRON *csound, PVSFTW *p)
{
    int i;
    MYFLT *ftablea,*ftablef;
    float *fsrc;                                                 /* RWD MUST be 32bit */
    long flena,flenf,nbins, N = p->fsrc->N;
    /* source fsig */
    p->overlap = p->fsrc->overlap;
    p->winsize = p->fsrc->winsize;
    p->wintype = p->fsrc->wintype;
    p->format  = p->fsrc->format;
    p->fftsize = N;
    p->outfna = p->outfnf = NULL;
    p->lastframe = 0;

    if (!(p->format==PVS_AMP_FREQ) || (p->format==PVS_AMP_PHASE))
      die(Str(X_1594,"pvsftw: signal format must be amp-phase or amp-freq.\n"));
    if (*p->ifna < 1.0f)
      die(Str(X_1595,"pvsftw: bad value for ifna.\n"));
    if (*p->ifnf < 0.0f)                /* 0 = notused */
      die(Str(X_1596,"pvsftw: bad value for ifnf.\n"));
    p->outfna = ftfind(p->h.insdshead->csound, p->ifna);
    if (p->outfna==NULL)
      return NOTOK;
    fsrc = (float *) p->fsrc->frame.auxp;                 /* RWD MUST be 32bit */
    /* init table, one presumes with zero amps */
    nbins = p->fftsize/2 + 1;
    flena = p->outfna->flen + 1;

    if (flena < nbins)
      die(Str(X_1597,"pvsftw: amps ftable too small.\n"));

    /* init tables */
    ftablea = p->outfna->ftable;
    for (i=0;i < nbins;i++)
      ftablea[i] = fsrc[i*2];

    /* freq table? */
    if ((long) *p->ifnf >= 1.0f) {
      p->outfnf = ftfind(p->h.insdshead->csound, p->ifnf);
      if (p->outfnf==NULL)
        return NOTOK;
      ftablef = p->outfnf->ftable;
      if (ftablef) {
        flenf = p->outfnf->flen+1;
        if (flenf < nbins)
          die(Str(X_1598,"pvsftw: freqs ftable too small.\n"));
        for (i=0;i < nbins;i++)
          ftablef[i] = fsrc[(i*2) + 1];
      }
    }

    return OK;
}

int pvsftw(ENVIRON *csound, PVSFTW *p)
{
    int i;
    long nbins;
    MYFLT *ftablea, *ftablef = NULL;
    float *fsrc;                /* RWD MUST be 32bit */


    ftablea = p->outfna->ftable;
    fsrc = (float *) p->fsrc->frame.auxp;

    if (fsrc==NULL)
      die(Str(X_1599,"pvsftw: not initialised\n"));
    if (ftablea==NULL)
      die(Str(X_1600,"pvsftw: no amps ftable!\n"));
    if (p->outfnf) {
      ftablef = p->outfnf->ftable;
      if (ftablef==NULL)
        die(Str(X_1601,"pvsftw: no freqs ftable!\n"));
    }
    nbins = p->fftsize/2 + 1;


    /* only write when a new frame is ready */
    if (p->lastframe < p->fsrc->framecount) {
      /* write amps */
      for (i=0;i < nbins;i++)
        ftablea[i] = fsrc[i*2];
      /* freqs */
      if (ftablef)
        for (i=0;i < nbins;i++)
          ftablef[i] = fsrc[i*2 + 1];

      p->lastframe = p->fsrc->framecount;
      /* tell everyone about it */
      *p->kflag = FL(1.0);
    }
    else
      *p->kflag = FL(0.0);
    return OK;
}


/************ PVSFTR ****************/

int pvsftrset(ENVIRON *csound, PVSFTR *p)
{
    int i;
    float *fdest;                         /* RWD MUST be 32bit */
    long flena,flenf,nbins, N = p->fdest->N;
    /* source fsig */
    p->overlap = p->fdest->overlap;
    p->winsize = p->fdest->winsize;
    p->wintype = p->fdest->wintype;
    p->format  = p->fdest->format;
    p->fftsize = N;
    p->infna = p->infnf = NULL;
    p->ftablea = p->ftablef = NULL;
    p->lastframe = 0;
    nbins = p->fftsize/2 + 1;

    if (!(p->format==PVS_AMP_FREQ) || (p->format==PVS_AMP_PHASE))
      die(Str(X_1602,"pvsftr: signal format must be amp-phase or amp-freq.\n"));
    /* ifn = 0 = notused */
    if (*p->ifna < 0.0f)
      die(Str(X_1603,"pvsftr: bad value for ifna.\n"));
    if (*p->ifnf < 0.0f)
      die(Str(X_1604,"pvsftr: bad value for ifnf.\n"));

    /* if ifna=0; no amps to read; one assumes the user ias changing freqs only,
       otherwise, there is no change!
    */
    if ((long) *p->ifna != 0) {
      p->infna = ftfind(p->h.insdshead->csound, p->ifna);
      if (p->infna==NULL)
        return NOTOK;
      p->ftablea = p->infna->ftable;
      flena = p->infna->flen + 1;
      if (flena < nbins)
        die(Str(X_1605,"pvsftr: amps ftable too small.\n"));
    }
    fdest = (float *) p->fdest->frame.auxp;                /* RWD MUST be 32bit */

    /*** setup first frame ?? */
    if (p->ftablea)
      for (i=0;i < nbins;i++)
        fdest[i*2] = (float) p->ftablea[i];

    /* freq table? */
    if ((long) *p->ifnf >= 1) {
      p->infnf = ftfind(p->h.insdshead->csound, p->ifnf);
      if (p->infnf==NULL)
        return NOTOK;
      p->ftablef = p->infnf->ftable;
      flenf = p->infnf->flen+1;
      if (flenf < nbins)
        die(Str(X_1606,"pvsftr: freqs ftable too small.\n"));
      for (i=0;i < nbins;i++)
        fdest[(i*2) + 1] = (float) p->ftablef[i];

    }

    return OK;
}

int pvsftr(ENVIRON *csound, PVSFTR *p)
{
    int i;
    long nbins;
    float *fdest;                       /*RWD MUST be 32bit */

    fdest = (float *) p->fdest->frame.auxp;
    if (fdest==NULL)
      die(Str(X_1607,"pvsftr: not initialised\n"));
    nbins = p->fftsize/2 + 1;

    /* only write when a new frame is ready */
    if (p->lastframe < p->fdest->framecount) {
      if (p->ftablea) {
        for (i=0;i < nbins;i++)
          fdest[i*2] = (float) p->ftablea[i];
      }
      if (p->ftablef) {
        for (i=0;i < nbins;i++)
          fdest[(i*2) + 1] = (float) p->ftablef[i];
      }
      p->lastframe = p->fdest->framecount;
    }
    return OK;
}





/************** PVSINFO ***********/

int pvsinfo(ENVIRON *csound, PVSINFO *p)
{
#ifdef _DEBUG
    /* init stage opcode : this should always be a proper fsig */
    assert(p->fsrc->frame.auxp != NULL);
#endif
    *p->ioverlap = (MYFLT) p->fsrc->overlap;
    *p->inumbins = (MYFLT) (p->fsrc->N / 2) + 1;
    *p->iwinsize = (MYFLT) p->fsrc->winsize;
    *p->iformat  = (MYFLT) p->fsrc->format;
    return OK;
}




/*  despite basic parity in analysis and synthesis, we still have to rescale the amplitudes
    by 32768 to fit Csound's notion of 0dBFS. Note we do NOT try to rescale to match the old
    .pv format.
*/

/* custom version of ldmemfile();
   enables pvfileio funcs to apply byte-reversal if needed.
   NB: filename size in MEMFIL struct was only 64; now 256...
*/

/* RWD NB PVOCEX format always 32bit, so no MYFLTs here! */

static int pvx_loadfile(const char *fname,PVSFREAD *p,MEMFIL **mfp)
{
    PVOCDATA pvdata;
    WAVEFORMATEX fmt;
    MEMFIL *mfil = NULL;
    int i,j,rc = 0,pvx_id = -1;
    long pvx_fftsize,pvx_winsize;
    long mem_wanted = 0;
    long totalframes,framelen;
    float *pFrame;
    float *memblock = NULL;
    pv_wtype wtype;

    pvx_id = pvoc_openfile(fname,&pvdata,&fmt);
    if (pvx_id < 0) {
      sprintf(errmsg,Str(X_1608,"unable to open pvocex file %s.\n"),fname);
      return 0;
    }
    /* fft size must be <= PVFRAMSIZE (=8192) for Csound */
    pvx_fftsize = 2 * (pvdata.nAnalysisBins-1);
    framelen = 2 * pvdata.nAnalysisBins;
    /* no need to impose Csound limit on fftsize here */
    pvx_winsize = pvdata.dwWinlen;


    /* also, accept only 32bit floats for now */
    if (pvdata.wWordFormat != PVOC_IEEE_FLOAT) {
      sprintf(errmsg,Str(X_1609,"pvoc-ex file %s is not 32bit floats\n"),fname);
      return 0;
    }

    /* FOR NOW, accept only PVOC_AMP_FREQ : later, we can convert */
    /* NB Csound knows no other: frameFormat is not read anywhere! */
    if (pvdata.wAnalFormat != PVOC_AMP_FREQ) {
      sprintf(errmsg,Str(X_1610,"pvoc-ex file %s not in AMP_FREQ format\n"),fname);
      return 0;
    }
    p->format = PVS_AMP_FREQ;
    /* ignore the window spec until we can use it! */
    totalframes = pvoc_framecount(pvx_id);
    if (totalframes == 0) {
      sprintf(errmsg,Str(X_1611,"pvoc-ex file %s is empty!\n"),fname);
      return 0;
    }

    if (!find_memfile(fname,&mfil)) {
      mem_wanted = totalframes * 2 * pvdata.nAnalysisBins * sizeof(float);
      /* try for the big block first! */

      memblock = (float *) mmalloc(mem_wanted);

      pFrame = memblock;
      /* despite using pvocex infile, and pvocex-style resynth, we ~still~
         have to rescale to Csound's internal range! This is because all pvocex
         calculations assume +-1 floatsam i/o. It seems preferable to do this here,
         rather than force the user to do so. Csound might change one day...*/

      for (i=0;i < totalframes;i++) {
        rc = pvoc_getframes(pvx_id,pFrame,1);
        if (rc != 1)
          break;                /* read error, but may still have something to use */
        /* scale amps to Csound range, to fit fsig */
        for (j=0;j < framelen; j+=2) {
          pFrame[j] *= (float)e0dbfs;
        }
#ifdef _DEBUG
        assert(pFrame[1] < 200.f);
#endif
        pFrame += framelen;
      }
      if (rc <0) {
        sprintf(errmsg,Str(X_1612,"error reading pvoc-ex file %s\n"),fname);
        mfree(memblock);
        return 0;
      }
      if (i < totalframes) {
        sprintf(errmsg,Str(X_1613,"error reading pvoc-ex file %s after %d frames\n"),fname,i);
        mfree(memblock);
        return 0;
      }
    }
    else
      memblock = (float *) mfil->beginp;

    pvoc_closefile(pvx_id);


    if ((p->arate = (MYFLT) fmt.nSamplesPerSec) != esr &&
        (O.msglevel & WARNMSG)) { /* & chk the data */
      printf(Str(X_63,"WARNING: %s''s srate = %8.0f, orch's srate = %8.0f\n"),
              fname, p->arate, esr);
    }
    p->fftsize  = pvx_fftsize;
    p->winsize  = pvx_winsize;
    p->membase  = memblock;
    p->overlap  = pvdata.dwOverlap;
    p->chans    = fmt.nChannels;
    p->nframes = (unsigned) totalframes;
    p->arate    = esr / (MYFLT) p->overlap;
    wtype = (pv_wtype) pvdata.wWindowType;
    switch (wtype) {
    case PVOC_DEFAULT:
    case PVOC_HAMMING:
      p->wintype = PVS_WIN_HAMMING;
      break;
    case PVOC_HANN:
      p->wintype = PVS_WIN_HANN;
      break;
    default:
      /* deal with all other possibilities later! */
      p->wintype = PVS_WIN_HAMMING;
      break;
    }


    /* Need to assign an MEMFIL to p->mfp */
    if (mfil==NULL) {
      mfil = (MEMFIL *)  mmalloc(sizeof(MEMFIL));
      /* just hope the filename is short enough...! */
      mfil->next = NULL;
      mfil->filename[0] = '\0';
      strcpy(mfil->filename,fname);
      mfil->beginp = (char *) memblock;
      mfil->endp = mfil->beginp + mem_wanted;
      mfil->length = mem_wanted;
      /*from memfiles.c */
      printf(Str(X_764,"file %s (%ld bytes) loaded into memory\n"), fname,mem_wanted);
      add_memfil(mfil);
    }

    *mfp = mfil;

    return 1;
}
