/*
  Coypright (c) 2010 Brian Carty
  PhD Code August 2010
  binaural reverb: early reflections

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

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"

#ifdef __FAST_MATH__
#undef __FAST_MATH__
#endif

#define SQUARE(X) ((X)*(X))

/* definitions, from mit */
#define minelev (-40)
#define elevincrement (10)

static const int32_t elevationarray[14] =
  {56, 60, 72, 72, 72, 72, 72, 60, 56, 45, 36, 24, 12, 1 };

/* for ppc byte switch */
#ifdef WORDS_BIGENDIAN
static int32_t swap4bytes(CSOUND* csound, MEMFIL* mfp)
{
    char c1, c2, c3, c4;
    char *p = mfp->beginp;
    int32_t  size = mfp->length;

    while (size >= 4)
      {
        c1 = p[0]; c2 = p[1]; c3 = p[2]; c4 = p[3];
        p[0] = c4; p[1] = c3; p[2] = c2; p[3] = c1;
        size -= 4; p +=4;
      }

    return OK;
}
#else
static int32_t (*swap4bytes)(CSOUND*, MEMFIL*) = NULL;
#endif

/* low pass filter for overall surface shape */
MYFLT filter(MYFLT* sig, MYFLT highcoeff, MYFLT lowcoeff,
             MYFLT *del, int32_t vecsize, MYFLT sr)
{
    MYFLT costh, coef;
    int32_t i;

    /* setup filter */
    MYFLT T = FL(1.0) / sr;
    MYFLT twopioversr = FL(2.0) * PI_F * T;
    MYFLT freq;
    MYFLT check;
    MYFLT scale, nyqresponse, irttwo, highresponse, lowresponse, cosw,
      a, b, c, x, y;

    irttwo = FL(1.0) / ROOT2;

    /* simple filter deals with difference in low and high */
    highresponse = FL(1.0) - highcoeff;
    lowresponse = FL(1.0) - lowcoeff;
    /* scale factor: walls assumed to be low pass */
    scale = lowresponse;
    nyqresponse = highresponse + lowcoeff;
    /* should always be lowpass! */
    if (nyqresponse > irttwo)
      nyqresponse = irttwo;

    /* calculate cutoff, according to nyqresponse */
    /* w = twopioversr * f (= sr / (MYFLT)2.0)
       (w = pi in the case of nyq...2pi/sr * sr/2) */
    /* cosw = (MYFLT)cos(w);... = -1 in case of nyq */
    cosw = -FL(1.0);

    a = c = SQUARE(nyqresponse) - FL(1.0);
    b = (FL(2.0) * cosw * SQUARE(nyqresponse)) - FL(2.0);
    /* '+' and '-' sqrt in quadratic equation give equal results in this scenario:
       working backwards to find cutoff freq of simple tone filter! */
    x = (-b + (SQRT(SQUARE(b) - FL(4.0) * a * c))) / (FL(2.0) * a);
    y = (-SQUARE(x) - FL(1.0)) / (FL(2.0) * x);

    check = FL(2.0) - y;
    /* check for legal acos arg */
    if (check < -FL(1.0))
      check = -FL(1.0);
    freq = FL(acos(check));
    freq /= twopioversr;

    /* filter */
    costh = FL(2.0) - COS(freq * twopioversr);
    coef = SQRT(costh * costh - FL(1.0)) - costh;

    for(i = 0; i < vecsize; i++)
      {
        /* filter */
        sig[i] = (sig[i] * (FL(1) + coef) - *del * coef);
        /* scale */
        sig[i] *= scale;
        /* store */
        *del = sig[i];
      }

    return *sig;
}

/* band pass for surface detail, from csound eqfil */
MYFLT band(MYFLT* sig, MYFLT cfreq, MYFLT bw, MYFLT g, MYFLT *del,
           int32_t vecsize, MYFLT sr)
{
    MYFLT T = FL(1.0) / sr;
    MYFLT pioversr = FL(PI) * T;
    MYFLT a = (COS(cfreq * pioversr * FL(2.0)));
    MYFLT b = (TAN(bw * pioversr));
    MYFLT c = (FL(1.0) - b) / (FL(1.0) + b);
    MYFLT w, y;
    int32_t i;

    for(i = 0; i < vecsize; i++)
      {
        w = sig[i] + a * (FL(1.0) + c) * del[0] - c * del[1];
        y = w * c - a * (FL(1.0) + c) * del[0] + del[1];
        sig[i] = FL(0.5) * (y + sig[i] + g * (sig[i] - y));
        del[1] = del[0];
        del[0] = w;
      }

    return *sig;
}

typedef struct
{
  OPDS  h;
  /* out l/r, low rt60, high rt60, amp, delay for latediffuse */
  MYFLT *outsigl, *outsigr, *irt60low, *irt60high, *imfp;
  /* input, source, listener, hrtf files, default room, [fadelength,
     sr, order, threed, headrot, roomsize, wall high and low absorb
     coeffs, gain for 3 band pass, same for floor and ceiling] */
  MYFLT *in, *srcx, *srcy, *srcz, *lstnrx, *lstnry, *lstnrz;
  STRINGDAT *ifilel, *ifiler;
  MYFLT  *idefroom, *ofade, *osr, *porder, *othreed, *Oheadrot,
    *ormx, *ormy, *ormz, *owlh, *owll, *owlg1, *owlg2, *owlg3, *oflh,
    *ofll, *oflg1, *oflg2, *oflg3,*oclh, *ocll, *oclg1, *oclg2, *oclg3;

  /* check if relative source has changed, to avoid recalculations */
  MYFLT srcxv, srcyv, srczv, lstnrxv, lstnryv, lstnrzv;
  MYFLT srcxk, srcyk, srczk, lstnrxk, lstnryk, lstnrzk;
  MYFLT rotatev;

  /* processing buffer sizes, depends on sr */
  int32_t irlength, irlengthpad, overlapsize;
  MYFLT sr;
  int32_t counter;

  /* crossfade preparation and checks */
  int32_t fade, fadebuffer;
  int32_t initialfade;

  /* interpolation buffer declaration */
  AUXCH lowl1, lowr1, lowl2, lowr2;
  AUXCH highl1, highr1, highl2, highr2;
  AUXCH hrtflinterp, hrtfrinterp, hrtflpad, hrtfrpad;
  AUXCH hrtflpadold, hrtfrpadold;

  /* convolution and in/output buffers */
  AUXCH inbuf,inbufpad;
  AUXCH outlspec, outrspec;
  AUXCH outlspecold, outrspecold;
  AUXCH overlapl, overlapr;
  AUXCH overlaplold, overlaprold;

  /* no of impulses based on order */
  int32_t impulses, order;
  int32_t M;
  /* 3d check*/
  int32_t threed;

  /* speed of sound*/
  MYFLT c;

  /* Image Model*/
  MYFLT rmx, rmy, rmz;
  int32_t maxdelsamps;

  /* for each reflection*/
  AUXCH hrtflpadspec, hrtfrpadspec, hrtflpadspecold, hrtfrpadspecold;
  AUXCH outl, outr, outlold, outrold;
  AUXCH currentphasel, currentphaser;
  AUXCH dell, delr;
  AUXCH tempsrcx, tempsrcy, tempsrcz;
  AUXCH dist;
  AUXCH dtime;
  AUXCH amp;

  /* temp o/p buffers */
  AUXCH predell, predelr;

  /* processing values that need to be kept for each reflection*/
  /* dynamic values based on no. fo impulses*/
  AUXCH oldelevindex, oldangleindex;
  AUXCH cross, l, delp, skipdel;
  AUXCH vdt;

  /* wall details */
  MYFLT wallcoeflow, wallcoefhigh, wallg1, wallg2, wallg3;
  MYFLT floorcoeflow, floorcoefhigh, floorg1, floorg2, floorg3;
  MYFLT ceilingcoeflow, ceilingcoefhigh, ceilingg1, ceilingg2, ceilingg3;
  /* wall filter q*/
  MYFLT q;

  /* file pointers*/
  float *fpbeginl, *fpbeginr;
  void *setup, *isetup, *isetup_pad;

} early;

static int32_t early_init(CSOUND *csound, early *p)
{
    /* iterator */
    int32_t i;

    /* left and right data files: spectral mag, phase format.*/
    MEMFIL *fpl=NULL, *fpr=NULL;
    char filel[MAXNAME],filer[MAXNAME];

    /* processing sizes*/
    int32_t irlength = 0, irlengthpad = 0, overlapsize = 0;

    /* walls: surface area*/
    MYFLT wallS1, wallS2, cfS;

    /* rt60 */
    MYFLT Salphalow, Salphahigh;
    MYFLT vol;

    /* room */
    MYFLT rmx, rmy, rmz;
    /* default room */
    int32_t defroom;

    /* dynamic values, based on number of impulses...*/
    int32_t *oldelevindex;
    int32_t *oldangleindex;
    int32_t *skipdel;

    /* order calculation */
    int32_t impulses = 1;
    int32_t temp = 2;

    /* defs for delay lines */
    MYFLT maxdist = FL(0.0);
    MYFLT maxdtime;
    int32_t     maxdelsamps;

    /* late tail */
    MYFLT meanfreepath;
    MYFLT surfacearea = FL(0.0);

    /* setup defaults for optional parameters */
    int32_t fade = (int32_t)*p->ofade;
    MYFLT sr = *p->osr;
    int32_t threed = (int32_t)*p->othreed;
    int32_t order = (int32_t)*p->porder;

    /* fade length: default 8, max 24, min 1 (fade is a local variable)*/
    if (fade < 1 || fade > 24)
      fade = 8;
    p->fade = fade;

    /* threed defaults to 2d! */
    if (threed < 0 || threed > 1)
      threed = 0;
    p->threed = threed;

    /* order: max 4, default 1 */
    if (order < 0 || order > 4)
      order = 1;
    p->order = order;

    /* sr, defualt 44100 */
    if (sr != 44100 && sr != 48000 && sr != 96000)
      sr = 44100;
    p->sr = sr;

    if (UNLIKELY(CS_ESR != sr))
      csound->Message(csound,
                      Str("\n\nWARNING!!:\nOrchestra SR not compatible "
                          "with HRTF processing SR of: %.0f\n\n"), sr);

    /* setup as per sr */
    if (sr == 44100 || sr == 48000) {
      irlength = 128;
      irlengthpad = 256;
      overlapsize = (irlength - 1);
    }
    else if (sr == 96000) {
      irlength = 256;
      irlengthpad = 512;
      overlapsize = (irlength - 1);
    }

    /* copy in string name...*/
    strncpy(filel, (char*) p->ifilel->data, MAXNAME-1); //filel[MAXNAME-1]='\0';
    strncpy(filer, (char*) p->ifiler->data, MAXNAME-1); //filer[MAXNAME-1]='\0';

    /* reading files, with byte swap */
    fpl = csound->LoadMemoryFile(csound, filel, CSFTYPE_FLOATS_BINARY,
                                   swap4bytes);
    if (UNLIKELY(fpl == NULL))
      return
        csound->InitError(csound, "%s",
                          Str("\n\n\nCannot load left data file, exiting\n\n"));

    fpr = csound->LoadMemoryFile(csound, filer, CSFTYPE_FLOATS_BINARY,
                                   swap4bytes);
    if (UNLIKELY(fpr == NULL))
      return
        csound->InitError(csound,  "%s",
                          Str("\n\n\nCannot load right data file, exiting\n\n"));

    /* file handles */
    p->fpbeginl = (float *) fpl->beginp;
    p->fpbeginr = (float *) fpr->beginp;

    /* setup structure values */
    p->irlength = irlength;
    p->irlengthpad = irlengthpad;
    p->overlapsize = overlapsize;
    p->c = 344.0;

    /* zero structure values */
    p->counter = 0;
    p->initialfade = 0;
    p->M = 0;

    /* the amount of buffers to fade over */
    p->fadebuffer = (int32_t)fade * irlength;

    defroom = (int32_t)*p->idefroom;
    /* 3 default rooms allowed*/
    if (defroom > 3)
      defroom = 1;

    /* setup wall coeffs: walls: plasterboard, ceiling: painted plaster,
       floor: carpet
       if any default room is chosen, default parameters for walls/ceiling/floor */
    if (defroom) {
      p->wallcoefhigh = FL(0.3);
      p->wallcoeflow = FL(0.1);
      p->wallg1 = FL(0.75);
      p->wallg2 = FL(0.95);
      p->wallg3 = FL(0.9);
      p->floorcoefhigh = FL(0.6);
      p->floorcoeflow = FL(0.1);
      p->floorg1 = FL(0.95);
      p->floorg2 = FL(0.6);
      p->floorg3 = FL(0.35);
      p->ceilingcoefhigh = FL(0.2);
      p->ceilingcoeflow = FL(0.1);
      p->ceilingg1 = FL(1.0);
      p->ceilingg2 = FL(1.0);
      p->ceilingg3 = FL(1.0);
    }
    /* otherwise use values, if valid */
    else {
      p->wallcoefhigh = (*p->owlh > FL(0.0) && *p->owlh < FL(1.0)) ?
        *p->owlh : FL(0.3);
      p->wallcoeflow = (*p->owll > FL(0.0) && *p->owll < FL(1.0)) ?
        *p->owll : FL(0.1);
      p->wallg1 = (*p->owlg1 > FL(0.0) && *p->owlg1 < FL(10.0)) ?
        *p->owlg1 : FL(0.75);
      p->wallg2 = (*p->owlg2 > FL(0.0) && *p->owlg2 < FL(10.0)) ?
        *p->owlg2 : FL(0.95);
      p->wallg3 = (*p->owlg3 > FL(0.0) && *p->owlg3 < FL(10.0)) ?
        *p->owlg3 : FL(0.9);
      p->floorcoefhigh = (*p->oflh > FL(0.0) && *p->oflh < FL(1.0)) ?
        *p->oflh : FL(0.6);
      p->floorcoeflow = (*p->ofll > FL(0.0) && *p->ofll < FL(1.0)) ?
        *p->ofll : FL(0.1);
      p->floorg1 = (*p->oflg1 > FL(0.0) && *p->oflg1 < FL(10.0)) ?
        *p->oflg1 : FL(0.95);
      p->floorg2 = (*p->oflg2 > FL(0.0) && *p->oflg2 < FL(10.0)) ?
        *p->oflg2 : FL(0.6);
      p->floorg3 = (*p->oflg3 > FL(0.0) && *p->oflg3 < FL(10.0)) ?
        *p->oflg3 : FL(0.35);
      p->ceilingcoefhigh = (*p->oclh > FL(0.0) && *p->oclh < FL(1.0)) ?
        *p->oclh : FL(0.2);
      p->ceilingcoeflow = (*p->ocll > FL(0.0) && *p->ocll < FL(1.0)) ?
        *p->ocll : FL(0.1);
      p->ceilingg1 = (*p->oclg1 > FL(0.0) && *p->oclg1 < FL(10.0)) ?
        *p->oclg1 : FL(1.0);
      p->ceilingg2 = (*p->oclg2 > FL(0.0) && *p->oclg2 < FL(10.0)) ?
        *p->oclg2 : FL(1.0);
      p->ceilingg3 = (*p->oclg3 > FL(0.0) && *p->oclg3 < FL(10.0)) ?
        *p->oclg3 : FL(1.0);
    }

    /* medium room */
    if (defroom == 1) {
      rmx = 10;
      rmy = 10;
      rmz = 3;
    }
    /* small */
    else if (defroom == 2) {
      rmx = 3;
      rmy = 4;
      rmz = 3;
    }
    /* large */
    else if (defroom == 3) {
      rmx = 20;
      rmy = 25;
      rmz = 7;
    }

    /* read values if they exist, use medium if not valid (must be at
       least a 2*2*2 room! */
    else {
      rmx = *p->ormx >= FL(2.0) ? *p->ormx : 10;
      rmy = *p->ormy >= FL(2.0) ? *p->ormy : 10;
      rmz = *p->ormz >= FL(2.0) ? *p->ormz : 4;
    }

    /* store */
    p->rmx = rmx;
    p->rmy = rmy;
    p->rmz = rmz;

    /* how many sources? */
    if (threed)
      {
        for(i = 1; i <= order; i++)
          {
            impulses += (4 * i);
            if (i <= (order - 1))
              /* sources = 2d impulses for order, plus 2 * each
                 preceding no of impulses
                 eg order 2: 2d = 1 + 4 + 8 = 13, 3d + 2*5 + 2 = 25*/
              temp += 2*impulses;
            else
              impulses = impulses + temp;
          }
      }
    else
      {
        for(i = 1; i <= order; i++)
          /* there will be 4 * order additional impulses for each order*/
          impulses += (4*i);
      }
    p->impulses =  impulses;

    /* allocate memory, reuse if possible: interpolation buffers */
    if (!p->lowl1.auxp || p->lowl1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowl1);
    else
      memset(p->lowl1.auxp, 0, irlength * sizeof(MYFLT));

    if (!p->lowr1.auxp || p->lowr1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowr1);
    else
      memset(p->lowr1.auxp, 0, irlength * sizeof(MYFLT));

    if (!p->lowl2.auxp || p->lowl2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowl2);
    else
      memset(p->lowl2.auxp, 0, irlength * sizeof(MYFLT));

    if (!p->lowr2.auxp || p->lowr2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowr2);
    else
      memset(p->lowr2.auxp, 0, irlength * sizeof(MYFLT));

    if (!p->highl1.auxp || p->highl1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highl1);
    else
      memset(p->highl1.auxp, 0, irlength * sizeof(MYFLT));

    if (!p->highr1.auxp || p->highr1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highr1);
    else
      memset(p->highr1.auxp, 0, irlength * sizeof(MYFLT));

    if (!p->highl2.auxp || p->highl2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highl2);
    else
      memset(p->highl2.auxp, 0, irlength * sizeof(MYFLT));

    if (!p->highr2.auxp || p->highr2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highr2);
    else
      memset(p->highr2.auxp, 0, irlength * sizeof(MYFLT));

    if (!p->hrtflinterp.auxp || p->hrtflinterp.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->hrtflinterp);
    else
      memset(p->hrtflinterp.auxp, 0, irlength * sizeof(MYFLT));

    if (!p->hrtfrinterp.auxp || p->hrtfrinterp.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->hrtfrinterp);
    else
      memset(p->hrtfrinterp.auxp, 0, irlength * sizeof(MYFLT));

    /* hrtf processing buffers */
    if (!p->hrtflpad.auxp || p->hrtflpad.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->hrtflpad);
    if (!p->hrtfrpad.auxp || p->hrtfrpad.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->hrtfrpad);
    if (!p->hrtflpadold.auxp || p->hrtflpadold.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->hrtflpadold);
    if (!p->hrtfrpadold.auxp || p->hrtfrpadold.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->hrtfrpadold);

    memset(p->hrtflpad.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->hrtfrpad.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->hrtflpadold.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->hrtfrpadold.auxp, 0, irlengthpad * sizeof(MYFLT));

    /* convolution & processing */
    if (!p->inbuf.auxp || p->inbuf.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->inbuf);
    if (!p->inbufpad.auxp || p->inbufpad.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p-> inbufpad);

    memset(p->inbuf.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->inbufpad.auxp, 0, irlengthpad * sizeof(MYFLT));

    if (!p->outlspec.auxp || p->outlspec.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->outlspec);
    if (!p->outrspec.auxp || p->outrspec.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->outrspec);

    memset(p->outlspec.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->outrspec.auxp, 0, irlengthpad * sizeof(MYFLT));

    if (!p->outlspecold.auxp || p->outlspecold.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->outlspecold);
    if (!p->outrspecold.auxp || p->outrspecold.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * sizeof(MYFLT), &p->outrspecold);

    memset(p->outlspecold.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->outrspecold.auxp, 0, irlengthpad * sizeof(MYFLT));

    if (!p->overlapl.auxp || p->overlapl.size < overlapsize * sizeof(MYFLT))
      csound->AuxAlloc(csound, overlapsize * sizeof(MYFLT), &p->overlapl);
    if (!p->overlapr.auxp || p->overlapr.size < overlapsize * sizeof(MYFLT))
      csound->AuxAlloc(csound, overlapsize * sizeof(MYFLT), &p->overlapr);

    memset(p->overlapl.auxp, 0, overlapsize * sizeof(MYFLT));
    memset(p->overlapr.auxp, 0, overlapsize * sizeof(MYFLT));

    if (!p->overlaplold.auxp || p->overlaplold.size < overlapsize * sizeof(MYFLT))
      csound->AuxAlloc(csound, overlapsize * sizeof(MYFLT), &p->overlaplold);
    if (!p->overlaprold.auxp || p->overlaprold.size < overlapsize * sizeof(MYFLT))
      csound->AuxAlloc(csound, overlapsize * sizeof(MYFLT), &p->overlaprold);

    memset(p->overlaplold.auxp, 0, overlapsize * sizeof(MYFLT));
    memset(p->overlaprold.auxp, 0, overlapsize * sizeof(MYFLT));

    /* dynamic values, based on no. of impulses*/
    if (!p->predell.auxp || p->predell.size < irlength * impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * impulses * sizeof(MYFLT), &p->predell);
    if (!p->predelr.auxp || p->predelr.size < irlength * impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * impulses * sizeof(MYFLT), &p->predelr);

    memset(p->predell.auxp, 0, irlength * impulses * sizeof(MYFLT));
    memset(p->predelr.auxp, 0, irlength * impulses * sizeof(MYFLT));

    if (!p->cross.auxp || p->cross.size < impulses * sizeof(int32_t))
      csound->AuxAlloc(csound, impulses * sizeof(int32_t), &p->cross);
    if (!p->l.auxp || p->l.size < impulses * sizeof(int32_t))
      csound->AuxAlloc(csound, impulses * sizeof(int32_t), &p->l);
    if (!p->delp.auxp || p->delp.size < impulses * sizeof(int32_t))
      csound->AuxAlloc(csound, impulses * sizeof(int32_t), &p->delp);
    if (!p->skipdel.auxp || p->skipdel.size < impulses * sizeof(int32_t))
      csound->AuxAlloc(csound, impulses * sizeof(int32_t), &p->skipdel);
    if (!p->vdt.auxp || p->vdt.size < impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, impulses * sizeof(MYFLT), &p->vdt);

    memset(p->cross.auxp, 0, impulses * sizeof(int32_t));
    memset(p->l.auxp, 0, impulses * sizeof(int32_t));
    memset(p->delp.auxp, 0, impulses * sizeof(int32_t));
    memset(p->vdt.auxp, 0, impulses * sizeof(MYFLT));
    /* skipdel looked after below */

    /* values distinct to each reflection*/
    if (!p->tempsrcx.auxp || p->tempsrcx.size < impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, impulses * sizeof(MYFLT), &p->tempsrcx);

    if (!p->tempsrcy.auxp || p->tempsrcy.size < impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, impulses * sizeof(MYFLT), &p->tempsrcy);

    if (!p->tempsrcz.auxp || p->tempsrcz.size < impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, impulses * sizeof(MYFLT), &p->tempsrcz);
    if (!p->dist.auxp || p->dist.size < impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, impulses * sizeof(MYFLT), &p->dist);
    if (!p->dtime.auxp || p->dtime.size < impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, impulses * sizeof(MYFLT), &p->dtime);
    if (!p->amp.auxp || p->amp.size < impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, impulses * sizeof(MYFLT), &p->amp);

    memset(p->tempsrcx.auxp, 0, impulses * sizeof(MYFLT));
    memset(p->tempsrcy.auxp, 0, impulses * sizeof(MYFLT));
    memset(p->tempsrcz.auxp, 0, impulses * sizeof(MYFLT));
    memset(p->dist.auxp, 0, impulses * sizeof(MYFLT));
    memset(p->dtime.auxp, 0, impulses * sizeof(MYFLT));
    memset(p->amp.auxp, 0, impulses * sizeof(MYFLT));

    if (!p->oldelevindex.auxp || p->oldelevindex.size < impulses * sizeof(int32_t))
      csound->AuxAlloc(csound, impulses * sizeof(int32_t), &p->oldelevindex);

    if (!p->oldangleindex.auxp || p->oldangleindex.size < impulses * sizeof(int32_t))
      csound->AuxAlloc(csound, impulses * sizeof(int32_t), &p->oldangleindex);

    /* no need to zero above, as filled below...*/

    /* -1 for first check */
    oldelevindex = (int32_t *)p->oldelevindex.auxp;
    oldangleindex = (int32_t *)p->oldangleindex.auxp;

    for(i = 0; i < impulses; i++)
      oldelevindex[i] = oldangleindex[i] = -1;

    /* more values distinct to each reflection */
    if (!p->hrtflpadspec.auxp ||
        p->hrtflpadspec.size < irlengthpad * impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound,
                       irlengthpad * impulses * sizeof(MYFLT), &p->hrtflpadspec);
    if (!p->hrtfrpadspec.auxp ||
        p->hrtfrpadspec.size < irlengthpad * impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound,
                       irlengthpad * impulses * sizeof(MYFLT), &p->hrtfrpadspec);

    memset(p->hrtflpadspec.auxp, 0, irlengthpad * impulses * sizeof(MYFLT));
    memset(p->hrtfrpadspec.auxp, 0, irlengthpad * impulses * sizeof(MYFLT));

    if (!p->hrtflpadspecold.auxp ||
        p->hrtflpadspecold.size < irlengthpad * impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound,
                       irlengthpad * impulses * sizeof(MYFLT), &p->hrtflpadspecold);
    if (!p->hrtfrpadspecold.auxp ||
        p->hrtfrpadspecold.size < irlengthpad * impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound,
                       irlengthpad * impulses * sizeof(MYFLT), &p->hrtfrpadspecold);

    memset(p->hrtflpadspecold.auxp, 0, irlengthpad * impulses * sizeof(MYFLT));
    memset(p->hrtfrpadspecold.auxp, 0, irlengthpad * impulses * sizeof(MYFLT));

    if (!p->outl.auxp || p->outl.size < irlengthpad * impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * impulses * sizeof(MYFLT), &p->outl);
    if (!p->outr.auxp || p->outr.size < irlengthpad * impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * impulses * sizeof(MYFLT), &p->outr);

    memset(p->outl.auxp, 0, irlengthpad * impulses * sizeof(MYFLT));
    memset(p->outr.auxp, 0, irlengthpad * impulses * sizeof(MYFLT));

    if (!p->outlold.auxp ||
        p->outlold.size < irlengthpad * impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * impulses * sizeof(MYFLT), &p->outlold);
    if (!p->outrold.auxp ||
        p->outrold.size < irlengthpad * impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad * impulses * sizeof(MYFLT), &p->outrold);

    memset(p->outlold.auxp, 0, irlengthpad * impulses * sizeof(MYFLT));
    memset(p->outrold.auxp, 0, irlengthpad * impulses * sizeof(MYFLT));

    if (!p->currentphasel.auxp ||
        p->currentphasel.size < irlength * impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound,
                       irlength * impulses * sizeof(MYFLT), &p->currentphasel);
    if (!p->currentphaser.auxp ||
        p->currentphaser.size < irlength * impulses * sizeof(MYFLT))
      csound->AuxAlloc(csound,
                       irlength * impulses * sizeof(MYFLT), &p->currentphaser);


    memset(p->currentphasel.auxp, 0, irlength * impulses * sizeof(MYFLT));
    memset(p->currentphaser.auxp, 0, irlength * impulses * sizeof(MYFLT));

    /* setup rt60 calcs...*/
    /* rectangular room: surface area of opposite walls, and floor/ceiling */
    wallS1 = rmy * rmz;
    wallS2 = rmx * rmz;
    cfS = rmx * rmy;

    /* volume and surface areas, for rt60 calc, for high and low frequencies */
    vol = rmx * rmy * rmz;

    /* add all surfaces, 2 of each wall in shoebox geometry */
    Salphalow = wallS1 * (LOG(FL(1.0) - p->wallcoeflow)) * FL(2.0);
    Salphalow += wallS2 * (LOG(FL(1.0) - p->wallcoeflow)) * FL(2.0);
    Salphalow += cfS * (LOG(FL(1.0) - p->floorcoeflow));
    Salphalow += cfS * (LOG(FL(1.0) - p->ceilingcoeflow));
    Salphahigh = wallS1 * (LOG(FL(1.0) - p->wallcoefhigh)) * FL(2.0);
    Salphahigh += wallS2 * (LOG(FL(1.0) - p->wallcoefhigh)) * FL(2.0);
    Salphahigh += cfS * (LOG(FL(1.0) - p->floorcoefhigh));
    Salphahigh += cfS * (LOG(FL(1.0) - p->ceilingcoefhigh));

    /* wall filter quality factor (4 octaves for required spread!: .2666667)
       (2 octaves = .6667 implies 125 - 500, cf 250, 500 - 2k, cf 1k,
       2000 - 8000, cf 4k)
       (4 octaves = = .2666667 implies 62.5 - 1000, cf 250, 250 - 4000, cf 1k,
       1000 - 16k, cf 4k */
    p->q = FL(0.2666667);

    *p->irt60low = (FL(-0.161) * vol)/Salphalow;
    *p->irt60high = (FL(-0.161) * vol)/Salphahigh;

    /* calculate max delay according to max dist from order */
    /* use hypotenuse rule to get max dist */
    /* could calculate per order, but designed for low order use */
    maxdist = HYPOT(rmx, rmy);
    //maxdist = (SQRT(SQUARE(rmx) + SQUARE(rmy)));
    if (threed)
      maxdist = HYPOT(maxdist, rmz);//maxdist = (SQRT(SQUARE(maxdist)+SQUARE(rmz)));
    maxdist = maxdist * (order + FL(1.0));

    maxdtime = maxdist / p->c;
    maxdelsamps = (int32_t)(maxdtime * sr);
    p->maxdelsamps = maxdelsamps;

    surfacearea = FL(2.0) * wallS1 + FL(2.0) * wallS2 + FL(2.0) * cfS;

    meanfreepath = FL(4.0) * vol / (surfacearea * p->c);

    /* set output...*/
    *p->imfp = meanfreepath;

    /* allocate delay memory for each impulse */
    if (!p->dell.auxp || p->dell.size < maxdelsamps * sizeof(MYFLT) * impulses)
      csound->AuxAlloc(csound, maxdelsamps * sizeof(MYFLT) * impulses, &p->dell);
    if (!p->delr.auxp || p->delr.size < maxdelsamps * sizeof(MYFLT) * impulses)
      csound->AuxAlloc(csound, maxdelsamps * sizeof(MYFLT) * impulses, &p->delr);

    memset(p->dell.auxp, 0, maxdelsamps * impulses * sizeof(MYFLT));
    memset(p->delr.auxp, 0, maxdelsamps * impulses * sizeof(MYFLT));

    /* amount to skip in to each del line */
    skipdel = (int32_t *)p->skipdel.auxp;

    for(i = 0; i < impulses; i++)
      skipdel[i] = i * maxdelsamps;

    /* setup values used to check if relative source position has changed,
       start at illegal value to ensure first process read */
    p->srcxv = FL(-1.0);
    p->srcyv = FL(-1.0);
    p->srczv = FL(-1.0);
    p->lstnrxv = FL(-1.0);
    p->lstnryv = FL(-1.0);
    p->lstnrzv = FL(-1.0);
    p->srcxk = FL(-1.0);
    p->srcyk = FL(-1.0);
    p->srczk = FL(-1.0);
    p->lstnrxk = FL(-1.0);
    p->lstnryk = FL(-1.0);
    p->lstnrzk = FL(-1.0);

    p->rotatev = FL(0.0);
    p->setup = csound->RealFFTSetup(csound, p->irlengthpad, FFT_FWD);
    p->isetup = csound->RealFFTSetup(csound, p->irlength, FFT_INV);
    p->isetup_pad = csound->RealFFTSetup(csound, p->irlengthpad, FFT_INV);
    return OK;
}

static int32_t early_process(CSOUND *csound, early *p)
{
    /* iterators */
    int32_t i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t j, nsmps = CS_KSMPS;

    /* local pointers to p */
    MYFLT *in = p->in;
    MYFLT *outsigl  = p->outsigl;
    MYFLT *outsigr = p->outsigr;

    int32_t irlength = p->irlength;
    int32_t irlengthpad = p->irlengthpad;
    int32_t overlapsize = p->overlapsize;

    int32_t counter = p->counter;

    /* convolution buffers */
    MYFLT *lowl1 = (MYFLT *)p->lowl1.auxp;
    MYFLT *lowr1 = (MYFLT *)p->lowr1.auxp;
    MYFLT *lowl2 = (MYFLT *)p->lowl2.auxp;
    MYFLT *lowr2 = (MYFLT *)p->lowr2.auxp;
    MYFLT *highl1 = (MYFLT *)p->highl1.auxp;
    MYFLT *highr1 = (MYFLT *)p->highr1.auxp;
    MYFLT *highl2 = (MYFLT *)p->highl2.auxp;
    MYFLT *highr2 = (MYFLT *)p->highr2.auxp;
    MYFLT *hrtflinterp = (MYFLT *)p->hrtflinterp.auxp;
    MYFLT *hrtfrinterp = (MYFLT *)p->hrtfrinterp.auxp;

    /* hrtf processing buffers */
    MYFLT *hrtflpad = (MYFLT *)p->hrtflpad.auxp;
    MYFLT *hrtfrpad = (MYFLT *)p->hrtfrpad.auxp;
    MYFLT *hrtflpadold = (MYFLT *)p->hrtflpadold.auxp;
    MYFLT *hrtfrpadold = (MYFLT *)p->hrtfrpadold.auxp;

    /* pointers into HRTF files: floating point data(even in 64 bit csound) */
    float *fpindexl = (float *)p->fpbeginl;
    float *fpindexr = (float *)p->fpbeginr;

    /* local copies */
    MYFLT srcx = *p->srcx;
    MYFLT srcy = *p->srcy;
    MYFLT srcz = *p->srcz;
    MYFLT lstnrx = *p->lstnrx;
    MYFLT lstnry = *p->lstnry;
    MYFLT lstnrz = *p->lstnrz;
    MYFLT rotate = *p->Oheadrot;

    MYFLT sr = p->sr;

    /* local variables, mainly used for simplification */
    MYFLT elevindexstore;
    MYFLT angleindexlowstore;
    MYFLT angleindexhighstore;

    /* for reading */
    MYFLT angle, elev;
    int32_t elevindex;
    int32_t angleindex;
    int32_t skip = 0;

    /* crossfade preparation and checks */
    int32_t fade = p->fade;
    int32_t fadebuffer = p->fadebuffer;
    int32_t initialfade = p->initialfade;
    int32_t crossfade;
    int32_t crossout;

    /* interpolation variable declaration: local */
    int32_t elevindexlow, elevindexhigh, angleindex1, angleindex2,
      angleindex3, angleindex4;
    MYFLT elevindexhighper, angleindex2per, angleindex4per;
    MYFLT magllow, magrlow, maglhigh, magrhigh, magl, magr, phasel, phaser;

    /* convolution and in/output buffers */
    MYFLT *inbuf = (MYFLT *)p->inbuf.auxp;
    MYFLT *inbufpad = (MYFLT *)p->inbufpad.auxp;
    MYFLT *outlspec = (MYFLT *)p->outlspec.auxp;
    MYFLT *outrspec = (MYFLT *)p->outrspec.auxp;
    MYFLT *outlspecold = (MYFLT *)p->outlspecold.auxp;
    MYFLT *outrspecold = (MYFLT *)p->outrspecold.auxp;
    MYFLT *overlapl = (MYFLT *)p->overlapl.auxp;
    MYFLT *overlapr = (MYFLT *)p->overlapr.auxp;
    MYFLT *overlaplold = (MYFLT *)p->overlaplold.auxp;
    MYFLT *overlaprold = (MYFLT *)p->overlaprold.auxp;
    MYFLT *predell = (MYFLT *)p->predell.auxp;
    MYFLT *predelr = (MYFLT *)p->predelr.auxp;

    MYFLT outltot, outrtot;

    /* distinct to each reflection */
    MYFLT *hrtflpadspec = (MYFLT *)p->hrtflpadspec.auxp;
    MYFLT *hrtfrpadspec = (MYFLT *)p->hrtfrpadspec.auxp;
    MYFLT *hrtflpadspecold = (MYFLT *)p->hrtflpadspecold.auxp;
    MYFLT *hrtfrpadspecold = (MYFLT *)p->hrtfrpadspecold.auxp;
    MYFLT *outl = (MYFLT *)p->outl.auxp;
    MYFLT *outr = (MYFLT *)p->outr.auxp;
    MYFLT *outlold = (MYFLT *)p->outlold.auxp;
    MYFLT *outrold = (MYFLT *)p->outrold.auxp;
    MYFLT *currentphasel = (MYFLT *)p->currentphasel.auxp;
    MYFLT *currentphaser = (MYFLT *)p->currentphaser.auxp;
    MYFLT *dell = (MYFLT *)p->dell.auxp;
    MYFLT *delr = (MYFLT *)p->delr.auxp;

    /* as above */
    int32_t *oldelevindex = (int32_t *)p->oldelevindex.auxp;
    int32_t *oldangleindex = (int32_t *)p->oldangleindex.auxp;
    int32_t *cross = (int32_t *)p->cross.auxp;
    int32_t *l = (int32_t *)p->l.auxp;
    int32_t *delp = (int32_t *)p->delp.auxp;
    int32_t *skipdel = (int32_t *)p->skipdel.auxp;
    MYFLT *vdt = (MYFLT *)p->vdt.auxp;
    MYFLT *dist = (MYFLT *)p->dist.auxp;
    MYFLT *dtime = (MYFLT *)p->dtime.auxp;
    MYFLT *amp = (MYFLT *)p->amp.auxp;
    MYFLT *tempsrcx = (MYFLT *)p->tempsrcx.auxp;
    MYFLT *tempsrcy = (MYFLT *)p->tempsrcy.auxp;
    MYFLT *tempsrcz = (MYFLT *)p->tempsrcz.auxp;
    MYFLT tempdist;

    /* from structure */
    int32_t impulses = p->impulses;
    int32_t order = p->order;
    int32_t M = p->M;
    int32_t threed = p->threed;

    /* used in vdel */
    int32_t maxdelsamps = p->maxdelsamps;
    MYFLT c = p->c;
    int32_t pos;
    MYFLT rp, frac;

    /* room size */
    MYFLT rmx = p->rmx;
    MYFLT rmy = p->rmy;
    MYFLT rmz = p->rmz;

    /* xc = x coordinate, etc...*/
    int32_t xc, yc, zc, lowz, highz;

    /* to simplify formulae, local */
    MYFLT formx, formy, formz;
    int32_t formxpow, formypow, formzpow;

    int32_t wallreflections, floorreflections=0, ceilingreflections=0;
    MYFLT delsinglel, delsingler;
    MYFLT deldoublel[2], deldoubler[2];

    /* temp variables, for efficiency */
    MYFLT tempx, tempy;

    /* angle / elev calc of source location */
    MYFLT newpntx, newpnty, newpntz;
    MYFLT ab,ac,bc;
    MYFLT coselev;

    /* processing size! */
    //n = CS_KSMPS;

    /* check for legal src/lstnr locations */
    /* restricted to being inside room! */
    if (srcx > (rmx - FL(0.1)))
      srcx = rmx - FL(0.1);
    if (srcx < FL(0.1))
      srcx = FL(0.1);
    if (srcy > (rmy - FL(0.1)))
      srcy = rmy - FL(0.1);
    if (srcy < FL(0.1))
      srcy = FL(0.1);
    if (srcz > (rmz - FL(0.1)))
      srcz = rmz - FL(0.1);
    if (srcz < FL(0.1))
      srcz = FL(0.1);
    if (lstnrx > (rmx - FL(0.1)))
      lstnrx = rmx - FL(0.1);
    if (lstnrx < FL(0.1))
      lstnrx = FL(0.1);
    if (lstnry > (rmy - FL(0.1)))
      lstnry = rmy - FL(0.1);
    if (lstnry < FL(0.1))
      lstnry = FL(0.1);
    if (lstnrz > (rmz - FL(0.1)))
      lstnrz = rmz - FL(0.1);
    if (lstnrz < FL(0.1))
      lstnrz = FL(0.1);

    /* k rate computations: sources, distances, delays, amps
       for each image source. */
    /* need minus values for formula... */

    /* only update if relative source updates! improves speed in
       static sources by a factor of 2-3! */
    if (srcx != p->srcxk || srcy != p->srcyk || srcz != p->srczk ||
        lstnrx != p->lstnrxk || lstnry != p->lstnryk || lstnrz != p->lstnrzk) {
      p->srcxk = srcx;
      p->srcyk = srcy;
      p->srczk = srcz;
      p->lstnrxk = lstnrx;
      p->lstnryk = lstnry;
      p->lstnrzk = lstnrz;

      for (xc = -order; xc <= order; xc++) {
        for (yc = abs(xc) - order; yc <= order - abs(xc); yc++) {
          /* only scroll through z plane if 3d required...*/
          if (threed) {
            lowz = abs(yc) - (order - abs(xc));
            highz = (order - abs(xc)) - abs(yc);
          }
          else {
            lowz = 0;
            highz = 0;
          }
          for (zc = lowz; zc <= highz; zc++) {
            /* to avoid recalculation, especially at audio rate
               for delay, later on */
            formxpow = (int32_t)pow(-1.0, xc);
            formypow = (int32_t)pow(-1.0, yc);
            formzpow = (int32_t)pow(-1.0, zc);
            formx = (xc + (1 - formxpow)/2) * rmx;
            formy = (yc + (1 - formypow)/2) * rmy;
            formz = (zc + (1 - formzpow)/2) * rmz;

            /* image */
            tempsrcx[M] = formxpow * srcx + formx;
            tempsrcy[M] = formypow * srcy + formy;
            tempsrcz[M] = formzpow * srcz + formz;

            /* Calculate delay here using source and listener location */
            dist[M] = (SQRT(SQUARE(tempsrcx[M] - lstnrx) +
                            SQUARE(tempsrcy[M] - lstnry) +
                            SQUARE(tempsrcz[M] - lstnrz)));

            /* in seconds... */
            dtime[M] = dist[M] / c;

            /* furthest allowable distance....max amp = 1. */
            tempdist = (dist[M] < FL(0.45) ? FL(0.45) : dist[M]);

            /* high amp value may cause clipping on early
               reflections...reduce overall amp if so...*/
            /* SPL inverse distance law */
            amp[M] = FL(0.45) / tempdist;

            /* vdels for distance processing: */
            vdt[M] = dtime[M] * sr;
            if (vdt[M] > maxdelsamps)
              vdt[M] = (MYFLT)maxdelsamps;

            M++;
            M = M % impulses;
          }
        }
      }
    }

    /* a rate... */
    if (UNLIKELY(offset)) {
      memset(outsigl, '\0', offset*sizeof(MYFLT));
      memset(outsigr, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&outsigl[nsmps], '\0', early*sizeof(MYFLT));
      memset(&outsigr[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (j=offset;j<nsmps;j++) {
      /* input */
      inbuf[counter] = in[j];

      /* output */
      outltot = 0.0;
      outrtot = 0.0;

      /* for each reflection */
      for (M = 0; M < impulses; M++) {
        /* a rate vdel: */
        rp = delp[M] - vdt[M];
        rp = (rp >= 0 ? (rp < maxdelsamps ? rp : rp - maxdelsamps) :
              rp + maxdelsamps);
        frac = rp - (int32_t)rp;
        /* shift into correct part of buffer */
        pos = (int32_t)rp + skipdel[M];
        /* write to l and r del lines */
        dell[delp[M] + skipdel[M]] = predell[counter + M * irlength] * amp[M];
        delr[delp[M] + skipdel[M]] = predelr[counter + M * irlength] * amp[M];
        /* read, at variable interpolated speed */
        outltot += dell[pos] +
          frac*(dell[(pos + 1 < (maxdelsamps + skipdel[M]) ?
                      pos + 1 : skipdel[M])] - dell[pos]);
        outrtot += delr[pos] +
          frac*(delr[(pos + 1 < (maxdelsamps + skipdel[M]) ?
                      pos + 1 : skipdel[M])] - delr[pos]);
        delp[M] = (delp[M] != maxdelsamps - 1 ? delp[M] + 1 : 0);

        outsigl[j] = outltot;
        outsigr[j] = outrtot;
      }
      counter++;

      /* used to ensure fade does not happen on first run */
      if (initialfade < (irlength + 2))
        initialfade++;

      /* 'hrtf buffer' rate */
      if (counter == irlength) {
        /* reset */
        M = 0;
        /* run according to formula */
        for (xc = -order; xc <= order; xc++) {
          for (yc = abs(xc) - order; yc <= order - abs(xc); yc++) {
            /* only scroll through z plane if 3d required... */
            if (threed) {
              lowz = abs(yc) - (order - abs(xc));
              highz = (order - abs(xc)) - abs(yc);
            }
            else {
              lowz = 0;
              highz = 0;
            }
            for (zc = lowz; zc <= highz; zc++) {
              /* zero */
              crossout = 0;
              crossfade = 0;

              /* avoid unnecessary processing if relative
                 source location has not changed */
              if (srcx != p->srcxv || srcy != p->srcyv ||
                  srcz != p->srczv || lstnrx != p->lstnrxv ||
                  lstnry != p->lstnryv || lstnrz != p->lstnrzv ||
                  rotate != p->rotatev) {
                /* if first process complete (128 samps in) and
                   source is too close to listener: warning, do not
                   process duda and martens range dependence
                   jasa 98: 5 times radius: near field...hrtf
                   changes! */
                if (dist[M] < FL(0.45) && initialfade > irlength)
                  ;       /* do not process... */
                else {
                  /* to avoid case where atan2 is invalid */
                  tempx = tempsrcx[M] - lstnrx;
                  tempy = tempsrcy[M] - lstnry;
                  if (tempx == 0 && tempy == 0)
                    angle = 0;
                  else {
                    /* - to invert anticlockwise to clockwise */
                    angle = (-(ATAN2(tempy, tempx)) * FL(180.0) / PI_F);
                    /* add 90 to go from y axis (front) */
                    angle = angle + FL(90.0);
                  }

                  /* xy point will be same as source, z same as
                     listener: a workable triangle */
                  newpntx = tempsrcx[M];
                  newpnty = tempsrcy[M];
                  newpntz = lstnrz;

                  /* ab: source to listener, ac: source to new point
                     under/over source, bc listener to new point */
                  /* a = source, b = listener, c = new point */
                  ab = (SQRT(SQUARE(tempsrcx[M] - lstnrx) +
                             SQUARE(tempsrcy[M] - lstnry) +
                             SQUARE(tempsrcz[M] - lstnrz)));
                  ac = (SQRT(SQUARE(tempsrcx[M] - newpntx) +
                             SQUARE(tempsrcy[M] - newpnty) +
                             SQUARE(tempsrcz[M] - newpntz)));
                  bc = (SQRT(SQUARE(lstnrx - newpntx) +
                             SQUARE(lstnry - newpnty) +
                             SQUARE(lstnrz - newpntz)));

                  /* elev: when bc == 0 -> source + listener at
                     same x,y point (may happen in first run,
                     checked after that) angle = 0, elev = 0 if
                     at same point,
                     or source may be directly above/below */
                  if (bc == FL(0.0)) {
                    /* source at listener */
                    if (ac == FL(0.0))
                      elev = FL(0.0);
                    /* source above listener */
                    else
                      elev = FL(90.0);
                  }
                  else {
                    /* cosine rule */
                    coselev = ((SQUARE(bc) +
                                SQUARE(ab) -
                                SQUARE(ac)) / (2.0 * ab * bc));
                    /* VL: need to clamp it to avoid NaN */
                      elev = (ACOS(coselev > 1.0 ?
                                   1.0 : (coselev < -1.0 ? -1.0 :
                                          coselev))* 180.0 / PI_F);
                  }

                  /* if z coefficient of source < listener:
                     source below listener...*/
                  if (tempsrcz[M] < lstnrz) elev *= -1;

                  if (elev > FL(90.0))
                    elev = FL(90.0);
                  if (elev < FL(-40.0))
                    elev = FL(-40.0);

                  /* two nearest elev indices
                     to avoid recalculating */
                  elevindexstore = (elev - minelev) / elevincrement;
                  elevindexlow = (int32_t)elevindexstore;

                  if (elevindexlow < 13)
                    elevindexhigh = elevindexlow + 1;
                  /* highest index reached */
                  else
                    elevindexhigh = elevindexlow;

                  /* get percentage value for interpolation */
                  elevindexhighper = elevindexstore - elevindexlow;

                  /* head rotation */
                  angle -= rotate;

                  while(angle < FL(0.0))
                    angle += FL(360.0);
                  while(angle >= FL(360.0))
                    angle -= FL(360.0);

                  /* as above,lookup index, used to check
                     for crossfade */
                  elevindex = (int32_t)(elevindexstore + FL(0.5));

                  angleindex = (int32_t)(angle /
                                         (FL(360.0) /
                                      elevationarray[elevindex]) +
                                         FL(0.5));
                  angleindex = angleindex % elevationarray[elevindex];

                  /* avoid recalculation */
                  angleindexlowstore = angle /
                    (FL(360.0) /
                     elevationarray[elevindexlow]);
                  angleindexhighstore = angle /
                    (FL(360.0) / elevationarray[elevindexhigh]);

                  /* 4 closest indices, 2 low and 2 high */
                  angleindex1 = (int32_t)angleindexlowstore;

                  angleindex2 = angleindex1 + 1;
                  angleindex2 = angleindex2 %
                    elevationarray[elevindexlow];

                  angleindex3 = (int32_t)angleindexhighstore;

                  angleindex4 = angleindex3 + 1;
                  angleindex4 = angleindex4 %
                    elevationarray[elevindexhigh];

                  /* angle percentages for interp */
                  angleindex2per = angleindexlowstore - angleindex1;
                  angleindex4per = angleindexhighstore - angleindex3;

                  /* crossfade happens if index changes:nearest
                     measurement changes, 1st step: store old
                     values */
                  if (oldelevindex[M] != elevindex ||
                      oldangleindex[M] != angleindex) {
                    if (initialfade > irlength) {
                      /* warning on overlapping fades */
                      if (cross[M]) {
                        csound->Message(csound, Str("\nWARNING: fades are "
                                   "overlapping: this could "
                                   "lead to noise: reduce "
                                   "fade size or change "
                                   "trajectory, fade = %d, cross=%d\n\n"), fade, cross[M]);
                        cross[M] = 0;
                      }
                      /* reset l */
                      l[M] = 0;
                      crossfade = 1;
                      for (i = 0; i < irlengthpad; i++) {
                        hrtflpadspecold[irlengthpad * M + i] =
                          hrtflpadspec[irlengthpad * M + i];
                        hrtfrpadspecold[irlengthpad * M + i] =
                          hrtfrpadspec[irlengthpad * M + i];
                      }
                    }

                    skip = 0;
                    /* store current phase */
                    if (angleindex > elevationarray[elevindex] / 2) {
                      for (i = 0; i < elevindex; i ++)
                        skip +=((int32_t)(elevationarray[i] / 2)
                                + 1) * irlength;
                      for (i = 0;
                           i < (elevationarray[elevindex] -
                                angleindex);
                           i++)
                        skip += irlength;
                      for (i = 0; i < irlength; i++) {
                        currentphasel[irlength * M + i] =
                          fpindexr[skip + i];
                        currentphaser[irlength * M + i] =
                          fpindexl[skip + i];
                      }
                    }
                    else {
                      for (i = 0; i < elevindex; i ++)
                        skip +=((int32_t)(elevationarray[i] / 2)
                                + 1) * irlength;
                      for (i = 0; i < angleindex; i++)
                        skip += irlength;
                      for (i = 0; i < irlength; i++) {
                        currentphasel[irlength * M + i] =
                          fpindexl[skip+i];
                        currentphaser[irlength * M + i] =
                          fpindexr[skip+i];
                      }
                    }
                  }

                  /* for next check */
                  oldelevindex[M] = elevindex;
                  oldangleindex[M] = angleindex;

                  /* read 4 nearest HRTFs */
                  /* switch l and r */
                  skip = 0;
                  if (angleindex1 > elevationarray[elevindexlow] / 2) {
                    for (i = 0; i < elevindexlow; i ++)
                      skip +=((int32_t)(elevationarray[i] / 2)
                              + 1) * irlength;
                    for (i = 0;
                         i < (elevationarray[elevindexlow] -
                              angleindex1);
                         i++)
                      skip += irlength;
                    for (i = 0; i < irlength; i++) {
                      lowl1[i] = fpindexr[skip+i];
                      lowr1[i] = fpindexl[skip+i];
                    }
                  }
                  else {
                    for (i = 0; i < elevindexlow; i ++)
                      skip +=((int32_t)(elevationarray[i] / 2)
                              + 1) * irlength;
                    for (i = 0; i < angleindex1; i++)
                      skip += irlength;
                    for (i = 0; i < irlength; i++) {
                      lowl1[i] = fpindexl[skip+i];
                      lowr1[i] = fpindexr[skip+i];
                    }
                  }

                  skip = 0;
                  if (angleindex2 > elevationarray[elevindexlow] / 2) {
                    for (i = 0; i < elevindexlow; i ++)
                      skip +=((int32_t)(elevationarray[i] / 2)
                              + 1) * irlength;
                    for (i = 0;
                         i < (elevationarray[elevindexlow] -
                              angleindex2);
                         i++)
                      skip += irlength;
                    for (i = 0; i < irlength; i++) {
                      lowl2[i] = fpindexr[skip+i];
                      lowr2[i] = fpindexl[skip+i];
                    }
                  }
                  else {
                    for (i = 0; i < elevindexlow; i ++)
                      skip +=((int32_t)(elevationarray[i] / 2)
                              + 1) * irlength;
                    for (i = 0; i < angleindex2; i++)
                      skip += irlength;
                    for (i = 0; i < irlength; i++) {
                      lowl2[i] = fpindexl[skip+i];
                      lowr2[i] = fpindexr[skip+i];
                    }
                  }

                  skip = 0;
                  if (angleindex3 > elevationarray[elevindexhigh] / 2) {
                    for (i = 0; i < elevindexhigh; i ++)
                      skip +=((int32_t)(elevationarray[i] / 2)
                              + 1) * irlength;
                    for (i = 0;
                         i < (elevationarray[elevindexhigh] -
                              angleindex3);
                         i++)
                      skip += irlength;
                    for (i = 0; i < irlength; i++) {
                      highl1[i] = fpindexr[skip+i];
                      highr1[i] = fpindexl[skip+i];
                    }
                  }
                  else {
                    for (i = 0; i < elevindexhigh; i ++)
                      skip +=((int32_t)(elevationarray[i] / 2)
                              + 1) * irlength;
                    for (i = 0; i < angleindex3; i++)
                      skip += irlength;
                    for (i = 0; i < irlength; i++) {
                      highl1[i] = fpindexl[skip+i];
                      highr1[i] = fpindexr[skip+i];
                    }
                  }

                  skip = 0;
                  if (angleindex4 > elevationarray[elevindexhigh] / 2) {
                    for (i = 0; i < elevindexhigh; i ++)
                      skip +=((int32_t)(elevationarray[i] / 2)
                              + 1) * irlength;
                    for (i = 0;
                         i < (elevationarray[elevindexhigh] -
                              angleindex4);
                         i++)
                      skip += irlength;
                    for (i = 0; i < irlength; i++) {
                      highl2[i] = fpindexr[skip+i];
                      highr2[i] = fpindexl[skip+i];
                    }
                  }
                  else {
                    for (i = 0; i < elevindexhigh; i ++)
                      skip +=((int32_t)(elevationarray[i] / 2)
                              + 1) * irlength;
                    for (i = 0; i < angleindex4; i++)
                      skip += irlength;
                    for (i = 0; i < irlength; i++) {
                      highl2[i] = fpindexl[skip+i];
                      highr2[i] = fpindexr[skip+i];
                    }
                  }

                  /* magnitude interpolation */
                  /* 0hz and Nyq */
                  magllow = (FABS(lowl1[0])) +
                    ((FABS(lowl2[0]) - FABS(lowl1[0]))) *
                    angleindex2per;
                  maglhigh = (FABS(highl1[0])) +
                    ((FABS(highl2[0]) - FABS(highl1[0]))) *
                    angleindex4per;
                  magrlow = (FABS(lowr1[0])) +
                    ((FABS(lowr2[0]) - FABS(lowr1[0]))) *
                    angleindex2per;
                  magrhigh = (FABS(highr1[0])) +
                    ((FABS(highr2[0]) - FABS(highr1[0]))) *
                    angleindex4per;
                  magl = magllow + (maglhigh - magllow) *
                    elevindexhighper;
                  magr = magrlow + (magrhigh - magrlow) *
                    elevindexhighper;
                  if (currentphasel[M * irlength] < FL(0.0))
                    hrtflinterp[0] = - magl;
                  else
                    hrtflinterp[0] = magl;
                  if (currentphaser[M * irlength] < FL(0.0))
                    hrtfrinterp[0] = - magr;
                  else
                    hrtfrinterp[0] = magr;

                  magllow = (FABS(lowl1[1])) +
                    ((FABS(lowl2[1]) - FABS(lowl1[1]))) *
                    angleindex2per;
                  maglhigh = (FABS(highl1[1]))
                    + ((FABS(highl2[1]) - FABS(highl1[1]))) *
                    angleindex4per;
                  magrlow = (FABS(lowr1[1])) +
                    ((FABS(lowr2[1]) - FABS(lowr1[1]))) *
                    angleindex2per;
                  magrhigh = (FABS(highr1[1])) +
                    ((FABS(highr2[1]) - FABS(highr1[1]))) *
                    angleindex4per;
                  magl = magllow + (maglhigh - magllow) *
                    elevindexhighper;
                  magr = magrlow + (magrhigh - magrlow) *
                    elevindexhighper;
                  if (currentphasel[M * irlength + 1] < FL(0.0))
                    hrtflinterp[1] = - magl;
                  else
                    hrtflinterp[1] = magl;
                  if (currentphaser[M * irlength + 1] < FL(0.0))
                    hrtfrinterp[1] = - magr;
                  else
                    hrtfrinterp[1] = magr;

                  /* other values are complex, in fftw format */
                  for (i = 2; i < irlength; i += 2) {
                    /* interpolate high and low magnitudes */
                    magllow = lowl1[i] + (lowl2[i] - lowl1[i]) *
                      angleindex2per;
                    maglhigh = highl1[i] + (highl2[i] - highl1[i]) *
                      angleindex4per;

                    magrlow = lowr1[i] + (lowr2[i] - lowr1[i]) *
                      angleindex2per;
                    magrhigh = highr1[i] + (highr2[i] - highr1[i]) *
                      angleindex4per;

                    /* interpolate high and low results,
                       use current phase */
                    magl = magllow +  (maglhigh - magllow) *
                      elevindexhighper;
                    phasel = currentphasel[M * irlength + i + 1];

                    /* polar to rectangular */
                    hrtflinterp[i] = magl * COS(phasel);
                    hrtflinterp[i + 1] = magl * SIN(phasel);

                    magr = magrlow + (magrhigh - magrlow) *
                      elevindexhighper;
                    phaser = currentphaser[M * irlength + i + 1];

                    hrtfrinterp[i] = magr * COS(phaser);
                    hrtfrinterp[i + 1] = magr * SIN(phaser);
                  }

                  csound->RealFFT(csound, p->isetup, hrtflinterp);
                  csound->RealFFT(csound, p->isetup,hrtfrinterp);

                  /* wall filters... */
                  /* all 4 walls are the same! (trivial to
                     make them different...) */
                  /* x axis, wall1 (left) */
                  wallreflections =
                    (int32_t)abs((int32_t)(xc * .5 - .25 +
                                   (0.25 * pow(-1.0, xc))));
                  /* wall2, x (right) */
                  wallreflections +=
                    (int32_t)abs((int32_t)(xc * .5 + .25 -
                                   (0.25 * pow(-1.0, xc))));
                  /* yaxis, wall3 (bottom) */
                  wallreflections +=
                    (int32_t)abs((int32_t)(yc * .5 - .25 +
                                   (0.25 * pow(-1.0, yc))));
                  /* yaxis, wall 4 (top) */
                  wallreflections +=
                    (int32_t)abs((int32_t)(yc * .5 + .25 -
                                   (0.25 * pow(-1.0, yc))));
                  if (threed) {
                    /* floor (negative z) */
                    floorreflections =
                      (int32_t)abs((int32_t)(zc * .5 - .25 +
                                     (0.25 * pow(-1.0, zc))));
                    /* ceiling (positive z) */
                    ceilingreflections =
                      (int32_t)abs((int32_t)(zc * .5 + .25
                                     - (0.25 * pow(-1.0, zc))));
                  }

                  /* fixed parameters on bands etc (to limit no of
                     inputs), but these could trivially be variable */
                  /* note: delay values can be reused: zeroed every
                     time as only used in
                     processing hrtf, once every irlength, so not
                     used continuously...*/
                  /* if processing was continuous, would need
                     separate mem for each filter, store for
                     next run etc...*/
                  for (i = 0; i < wallreflections; i++) {
                    delsinglel = delsingler = FL(0.0);
                    filter(hrtflinterp, p->wallcoefhigh,
                           p->wallcoeflow, &delsinglel,
                           irlength, sr);
                    filter(hrtfrinterp, p->wallcoefhigh,
                           p->wallcoeflow, &delsingler,
                           irlength, sr);
                    deldoublel[0] = deldoublel[1] =
                      deldoubler[0] = deldoubler[1] = 0.0;
                    band(hrtflinterp, FL(250.0), FL(250.0) / p->q,
                         p->wallg1, deldoublel, irlength, sr);
                    band(hrtfrinterp, FL(250.0), FL(250.0) / p->q,
                         p->wallg1, deldoubler, irlength, sr);
                    deldoublel[0] = deldoublel[1] =
                      deldoubler[0] = deldoubler[1] = 0.0;
                    band(hrtflinterp, FL(1000.0),
                         FL(1000.0) / p->q, p->wallg2,
                         deldoublel, irlength, sr);
                    band(hrtfrinterp, FL(1000.0),
                         FL(1000.0) / p->q, p->wallg2,
                         deldoubler, irlength, sr);
                    deldoublel[0] = deldoublel[1] =
                      deldoubler[0] = deldoubler[1] = 0.0;
                    band(hrtflinterp, FL(4000.0),
                         FL(4000.0) / p->q, p->wallg3,
                         deldoublel, irlength, sr);
                    band(hrtfrinterp, FL(4000.0),
                         FL(4000.0) / p->q, p->wallg3,
                         deldoubler, irlength, sr);
                  }
                  if (threed) {
                    for (i = 0; i < floorreflections; i++) {
                      delsinglel = delsingler = FL(0.0);
                      filter(hrtflinterp, p->floorcoefhigh,
                             p->floorcoeflow, &delsinglel,
                             irlength, sr);
                      filter(hrtfrinterp, p->floorcoefhigh, p->floorcoeflow,
                             &delsingler, irlength, sr);
                      deldoublel[0] = deldoublel[1] = deldoubler[0] =
                        deldoubler[1] = 0.0;
                      band(hrtflinterp, FL(250.0), FL(250.0) / p->q, p->floorg1,
                           deldoublel, irlength, sr);
                      band(hrtfrinterp, FL(250.0), FL(250.0) / p->q, p->floorg1,
                           deldoubler, irlength, sr);
                      deldoublel[0] = deldoublel[1] = deldoubler[0] =
                        deldoubler[1] = 0.0;
                      band(hrtflinterp, FL(1000.0), FL(1000.0) / p->q, p->floorg2,
                           deldoublel, irlength, sr);
                      band(hrtfrinterp, FL(1000.0), FL(1000.0) / p->q, p->floorg2,
                           deldoubler, irlength, sr);
                      deldoublel[0] = deldoublel[1] = deldoubler[0] =
                        deldoubler[1] = 0.0;
                      band(hrtflinterp, FL(4000.0), FL(4000.0) / p->q, p->floorg3,
                           deldoublel, irlength, sr);
                      band(hrtfrinterp, FL(4000.0), FL(4000.0) / p->q, p->floorg3,
                           deldoubler, irlength, sr);
                    }
                    for (i = 0; i < ceilingreflections; i++) {
                      delsinglel = delsingler = FL(0.0);
                      filter(hrtflinterp, p->ceilingcoefhigh, p->ceilingcoeflow,
                             &delsinglel, irlength, sr);
                      filter(hrtfrinterp, p->ceilingcoefhigh, p->ceilingcoeflow,
                             &delsingler, irlength, sr);
                      deldoublel[0] = deldoublel[1] = deldoubler[0] =
                        deldoubler[1] = 0.0;
                      band(hrtflinterp, FL(250.0), FL(250.0) / p->q, p->ceilingg1,
                           deldoublel, irlength, sr);
                      band(hrtfrinterp, FL(250.0), FL(250.0) / p->q, p->ceilingg1,
                           deldoubler, irlength, sr);
                      deldoublel[0] = deldoublel[1] = deldoubler[0] =
                        deldoubler[1] = 0.0;
                      band(hrtflinterp, FL(1000.0), FL(1000.0) / p->q,
                           p->ceilingg2, deldoublel, irlength, sr);
                      band(hrtfrinterp, FL(1000.0), FL(1000.0) / p->q,
                           p->ceilingg2, deldoubler, irlength, sr);
                      deldoublel[0] = deldoublel[1] = deldoubler[0] =
                        deldoubler[1] = 0.0;
                      band(hrtflinterp, FL(4000.0), FL(4000.0) / p->q,
                           p->ceilingg3, deldoublel, irlength, sr);
                      band(hrtfrinterp, FL(4000.0), FL(4000.0) / p->q,
                           p->ceilingg3, deldoubler, irlength, sr);
                    }
                  }

                  for (i = 0; i < irlength; i++) {
                    hrtflpad[i] = hrtflinterp[i];
                    hrtfrpad[i] = hrtfrinterp[i];
                  }

                  for (i = irlength; i < irlengthpad; i++) {
                    hrtflpad[i] = FL(0.0);
                    hrtfrpad[i] = FL(0.0);
                  }

                  /* back to freq domain */
                  csound->RealFFT(csound, p->setup, hrtflpad);
                  csound->RealFFT(csound, p->setup, hrtfrpad);

                  /* store */
                  for (i = 0; i < irlengthpad; i++) {
                    hrtflpadspec[M * irlengthpad + i] = hrtflpad[i];
                    hrtfrpadspec[M * irlengthpad + i] = hrtfrpad[i];
                  }
                }
              }       /* end of source / listener relative
                         change process */

              /* look after overlap add */
              for (i = 0; i < overlapsize ; i++) {
                overlapl[i] = outl[M * irlengthpad + i + irlength];
                overlapr[i] = outr[M * irlengthpad + i + irlength];
                if (crossfade) {
                  overlaplold[i] =
                    outl[M * irlengthpad + i + irlength];
                  overlaprold[i] =
                    outr[M * irlengthpad + i + irlength];
                }
                /* overlap will be previous fading out signal */
                if (cross[M]) {
                  overlaplold[i] =
                    outlold[M * irlengthpad + i + irlength];
                  overlaprold[i] =
                    outrold[M * irlengthpad + i + irlength];
                }
              }

              /* insert insig */
              for (i = 0; i <  irlength; i++)
                inbufpad[i] = inbuf[i];

              for (i = irlength; i <  irlengthpad; i++)
                inbufpad[i] = FL(0.0);

              csound->RealFFT(csound, p->setup, inbufpad);

              for (i = 0; i < irlengthpad; i ++) {
                hrtflpad[i] = hrtflpadspec[M * irlengthpad + i];
                hrtfrpad[i] = hrtfrpadspec[M * irlengthpad + i];
              }

              /* convolution: spectral multiplication */
              csound->RealFFTMult(csound, outlspec, hrtflpad,
                                  inbufpad, irlengthpad, FL(1.0));
              csound->RealFFTMult(csound, outrspec, hrtfrpad,
                                  inbufpad, irlengthpad, FL(1.0));

              csound->RealFFT(csound, p->isetup_pad, outlspec);
              csound->RealFFT(csound, p->isetup_pad, outrspec);

              /* scale */
              for (i = 0; i < irlengthpad; i++) {
                outlspec[i] = outlspec[i]/(sr/FL(38000.0));
                outrspec[i] = outrspec[i]/(sr/FL(38000.0));
              }

              /* store */
              for (i = 0; i < irlengthpad; i++) {
                outl[M * irlengthpad + i] = outlspec[i];
                outr[M * irlengthpad + i] = outrspec[i];
              }

              /* setup for fades */
              if (crossfade || cross[M]) {
                crossout = 1;

                /* need to put these values into a buffer for processing */
                for (i = 0; i < irlengthpad; i++) {
                  hrtflpadold[i] =
                    hrtflpadspecold[M * irlengthpad + i];
                  hrtfrpadold[i] =
                    hrtfrpadspecold[M * irlengthpad + i];
                }

                /* convolution */
                csound->RealFFTMult(csound, outlspecold, hrtflpadold,
                                    inbufpad, irlengthpad, FL(1.0));
                csound->RealFFTMult(csound, outrspecold, hrtfrpadold,
                                    inbufpad, irlengthpad, FL(1.0));

                /* ifft, back to time domain */
                csound->RealFFT(csound, p->isetup_pad, outlspecold);
                csound->RealFFT(csound, p->isetup_pad, outrspecold);

                /* scale */
                for (i = 0; i < irlengthpad; i++) {
                  outlspecold[i] = outlspecold[i]/(sr/FL(38000.0));
                  outrspecold[i] = outrspecold[i]/(sr/FL(38000.0));
                }

                /* o/p real values */
                for (i = 0; i < irlengthpad; i++) {
                  outlold[M * irlengthpad + i] = outlspecold[i];
                  outrold[M * irlengthpad + i] = outrspecold[i];
                }

                cross[M]++;
                cross[M] %= fade;
              }

              if (crossout)
                for (i = 0; i < irlength; i++) {
                  predell[i + M * irlength] =
                    (outlspecold[i] +
                     (i < overlapsize ? overlaplold[i] : FL(0.0))) *
                    FL(1. - (FL(l[M]) / fadebuffer)) +
                    (outlspec[i] +
                     (i < overlapsize ? overlapl[i] : FL(0.0))) *
                    (FL(l[M]) / fadebuffer);
                  predelr[i + M * irlength] =
                    (outrspecold[i] +
                     (i < overlapsize ? overlaprold[i] : FL(0.0))) *
                    FL(1. - (FL(l[M]) / fadebuffer)) +
                    (outrspec[i] +
                     (i < overlapsize ? overlapr[i] : FL(0.0))) *
                    (FL(l[M]) / fadebuffer);
                  l[M]++;
                }
              else
                for (i = 0; i < irlength; i++) {
                  predell[i + M * irlength] =
                    outlspec[i] +
                    (i < overlapsize ? overlapl[i] : FL(0.0));
                  predelr[i + M * irlength] =
                    outrspec[i] +
                    (i < overlapsize ? overlapr[i] : FL(0.0));
                }

              M++;
              M = M % impulses;

            } /* z */
          } /* y */
        } /* x */

        counter = 0;
        /* need to store these values here, as storing them after check
           would not allow each impulse to be processed! */
        p->srcxv = srcx;
        p->srcyv = srcy;
        p->srczv = srcz;
        p->lstnrxv = lstnrx;
        p->lstnryv = lstnry;
        p->lstnrzv = lstnrz;
        p->rotatev = rotate;

      }       /* end of counter == irlength */

      /* update */
      p->counter = counter;
      p->initialfade = initialfade;

    } /* end of ksmps loop */

    return OK;
}

static OENTRY hrtfearly_localops[] =
  {
    {
     "hrtfearly",   sizeof(early), 0, "aaiii",
                                         "axxxxxxSSioopoOoooooooooooooooooo",
      (SUBR)early_init, (SUBR)early_process
    }
  };

LINKAGE_BUILTIN(hrtfearly_localops)
