/*
    hrtfopcodes.c: new HRTF opcodes

    Copyright (c) Brian Carty, 2010

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

#include <math.h>
/* definitions */
/* from mit */
#define minelev (-40)
#define elevincrement (10)

/* max delay for min phase: a time value:
   multiply by sr to get no of samples for memory allocation */
#define maxdeltime (0.0011)

/* additional definitions for woodworth models */
#define c (34400.0)

/* hrtf data sets were analysed for low frequency phase values, as it
   is the important part of the spectrum for phase based localisation
   cues. The values below were extracted and are used to scale the
   functional phase spectrum. radius: 8.8 cm see nonlinitd.cpp */
static const float nonlinitd[5] = {1.570024f, 1.378733f, 1.155164f, 1.101230f,1.0f};
static const float nonlinitd48k[5] =
  {1.549748f, 1.305457f, 1.124501f, 1.112852f,1.0f};
static const float nonlinitd96k[5] =
  {1.550297f, 1.305671f, 1.124456f, 1.112818f,1.0f};

/* number of measurements per elev: mit data const:read only, static:exists
   for whole process... */
static const int32_t elevationarray[14] =
  {56, 60, 72, 72, 72, 72, 72, 60, 56, 45, 36, 24, 12, 1 };

/* assumed mit hrtf data will be used here. Otherwise delay data would need
   to be extracted and replaced here... */
static const float minphasedels[368] =
{
  0.000000f, 0.000045f, 0.000091f, 0.000136f, 0.000159f, 0.000204f,
  0.000249f, 0.000272f, 0.000295f, 0.000317f, 0.000363f, 0.000385f,
  0.000272f, 0.000408f, 0.000454f, 0.000454f, 0.000408f, 0.000385f,
  0.000363f, 0.000317f, 0.000295f, 0.000295f, 0.000249f, 0.000204f,
  0.000159f, 0.000136f, 0.000091f, 0.000045f, 0.000000f, 0.000000f,
  0.000045f, 0.000091f, 0.000136f, 0.000181f, 0.000227f, 0.000249f,
  0.000272f, 0.000317f, 0.000363f, 0.000385f, 0.000454f, 0.000476f,
  0.000454f, 0.000522f, 0.000499f, 0.000499f, 0.000476f, 0.000454f,
  0.000408f, 0.000408f, 0.000385f, 0.000340f, 0.000295f, 0.000272f,
  0.000227f, 0.000181f, 0.000136f, 0.000091f, 0.000045f, 0.000000f,
  0.000000f, 0.000045f, 0.000091f, 0.000113f, 0.000159f, 0.000204f,
  0.000227f, 0.000272f, 0.000317f, 0.000317f, 0.000363f, 0.000408f,
  0.000363f, 0.000522f, 0.000476f, 0.000499f, 0.000590f, 0.000567f,
  0.000567f, 0.000544f, 0.000522f, 0.000499f, 0.000476f, 0.000454f,
  0.000431f, 0.000408f, 0.000385f, 0.000363f, 0.000317f, 0.000295f,
  0.000249f, 0.000204f, 0.000181f, 0.000136f, 0.000091f, 0.000045f,
  0.000000f, 0.000000f, 0.000045f, 0.000091f, 0.000113f, 0.000159f,
  0.000204f, 0.000249f, 0.000295f, 0.000317f, 0.000363f, 0.000340f,
  0.000385f, 0.000431f, 0.000476f, 0.000522f, 0.000544f, 0.000612f,
  0.000658f, 0.000658f, 0.000635f, 0.000658f, 0.000522f, 0.000499f,
  0.000476f, 0.000454f, 0.000408f, 0.000385f, 0.000363f, 0.000340f,
  0.000295f, 0.000272f, 0.000227f, 0.000181f, 0.000136f, 0.000091f,
  0.000045f, 0.000000f, 0.000000f, 0.000045f, 0.000091f, 0.000136f,
  0.000159f, 0.000204f, 0.000249f, 0.000295f, 0.000340f, 0.000385f,
  0.000431f, 0.000476f, 0.000522f, 0.000567f, 0.000522f, 0.000567f,
  0.000567f, 0.000635f, 0.000703f, 0.000748f, 0.000748f, 0.000726f,
  0.000703f, 0.000658f, 0.000454f, 0.000431f, 0.000385f, 0.000363f,
  0.000317f, 0.000295f, 0.000272f, 0.000227f, 0.000181f, 0.000136f,
  0.000091f, 0.000045f, 0.000000f, 0.000000f, 0.000045f, 0.000091f,
  0.000113f, 0.000159f, 0.000204f, 0.000249f, 0.000295f, 0.000340f,
  0.000385f, 0.000408f, 0.000454f, 0.000499f, 0.000544f, 0.000522f,
  0.000590f, 0.000590f, 0.000635f, 0.000658f, 0.000680f, 0.000658f,
  0.000544f, 0.000590f, 0.000567f, 0.000454f, 0.000431f, 0.000385f,
  0.000363f, 0.000317f, 0.000272f, 0.000272f, 0.000227f, 0.000181f,
  0.000136f, 0.000091f, 0.000045f, 0.000000f, 0.000000f, 0.000045f,
  0.000068f, 0.000113f, 0.000159f, 0.000204f, 0.000227f, 0.000272f,
  0.000317f, 0.000340f, 0.000385f, 0.000431f, 0.000454f, 0.000499f,
  0.000499f, 0.000544f, 0.000567f, 0.000590f, 0.000590f, 0.000590f,
  0.000590f, 0.000567f, 0.000567f, 0.000476f, 0.000454f, 0.000408f,
  0.000385f, 0.000340f, 0.000340f, 0.000295f, 0.000249f, 0.000204f,
  0.000159f, 0.000136f, 0.000091f, 0.000045f, 0.000000f, 0.000000f,
  0.000045f, 0.000091f, 0.000113f, 0.000159f, 0.000204f, 0.000249f,
  0.000295f, 0.000340f, 0.000363f, 0.000385f, 0.000431f, 0.000454f,
  0.000499f, 0.000522f, 0.000522f, 0.000522f, 0.000499f, 0.000476f,
  0.000454f, 0.000431f, 0.000385f, 0.000340f, 0.000317f, 0.000272f,
  0.000227f, 0.000181f, 0.000136f, 0.000091f, 0.000045f, 0.000000f,
  0.000000f, 0.000045f, 0.000091f, 0.000136f, 0.000159f, 0.000204f,
  0.000227f, 0.000249f, 0.000295f, 0.000340f, 0.000363f, 0.000385f,
  0.000408f, 0.000431f, 0.000431f, 0.000431f, 0.000431f, 0.000408f,
  0.000385f, 0.000363f, 0.000317f, 0.000317f, 0.000272f, 0.000227f,
  0.000181f, 0.000136f, 0.000091f, 0.000045f, 0.000000f, 0.000000f,
  0.000045f, 0.000091f, 0.000136f, 0.000181f, 0.000204f, 0.000227f,
  0.000272f, 0.000295f, 0.000317f, 0.000340f, 0.000340f, 0.000363f,
  0.000363f, 0.000340f, 0.000317f, 0.000295f, 0.000249f, 0.000204f,
  0.000159f, 0.000113f, 0.000068f, 0.000023f, 0.000000f, 0.000045f,
  0.000068f, 0.000113f, 0.000159f, 0.000181f, 0.000204f, 0.000227f,
  0.000249f, 0.000249f, 0.000249f, 0.000227f, 0.000227f, 0.000181f,
  0.000159f, 0.000113f, 0.000091f, 0.000045f, 0.000000f, 0.000000f,
  0.000045f, 0.000091f, 0.000136f, 0.000159f, 0.000181f, 0.000181f,
  0.000181f, 0.000159f, 0.000136f, 0.000091f, 0.000045f, 0.000000f,
  0.000000f, 0.000045f, 0.000068f, 0.000091f, 0.000068f, 0.000045f,
  0.000000f, 0.000000f
};


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

/* Csound hrtf magnitude interpolation, phase truncation object */

/* aleft,aright hrtfmove asrc, kaz, kel, ifilel->data, ifiler [, imode = 0,
   ifade = 8, sr = 44100]... */
/* imode: minphase/phase truncation, ifade: no of buffers per fade for
   phase trunc., sr can be 44.1/48/96k */

typedef struct
{
        OPDS  h;
        /* outputs and inputs */
        MYFLT *outsigl, *outsigr;
        MYFLT *in, *kangle, *kelev;
          STRINGDAT *ifilel, *ifiler;
        MYFLT *omode, *ofade, *osr;

        /* check if relative source has changed! */
        MYFLT anglev, elevv;

        float *fpbeginl,*fpbeginr;

        /* see definitions in INIT */
        int32_t irlength, irlengthpad, overlapsize;

        MYFLT sr;

        /* old indices for checking if changes occur in trajectory. */
        int32_t oldelevindex, oldangleindex;

        int32_t counter;

        /* initialfade used to avoid fade in of data...if not,'old' data
           faded out with zero hrtf,'new' data faded in. */
        int32_t cross,l,initialfade;

        /* user defined buffer size for fades. */
        int32_t fadebuffer, fade;

        /* flags for process type */
        int32_t minphase,phasetrunc;

        /* hrtf data padded */
        AUXCH hrtflpad,hrtfrpad;
        /* old data for fades */
        AUXCH oldhrtflpad,oldhrtfrpad;
        /* in and output buffers */
        AUXCH insig, outl, outr, outlold, outrold;

        /* memory local to perform method */
        /* insig fft */
        AUXCH complexinsig;
        /* hrtf buffers (rectangular complex form) */
        AUXCH hrtflfloat, hrtfrfloat;
        /* spectral data */
        AUXCH outspecl, outspecr, outspecoldl, outspecoldr;

        /* overlap data */
        AUXCH overlapl, overlapr;
        /* old overlap data for longer crossfades */
        AUXCH overlapoldl, overlapoldr;

        /* interpolation buffers */
        AUXCH lowl1, lowr1, lowl2, lowr2, highl1, highr1, highl2, highr2;
        /* current phase buffers */
        AUXCH currentphasel, currentphaser;

        /* min phase buffers */
        AUXCH logmagl,logmagr,xhatwinl,xhatwinr,expxhatwinl,expxhatwinr;
        /* min phase window: a static buffer */
        AUXCH win;
        MYFLT delayfloat;

        /* delay */
        AUXCH delmeml, delmemr;
        int32_t ptl, ptr, mdtl, mdtr;

        void *setup, *setup_pad, *isetup, *isetup_pad;
}
hrtfmove;

static int32_t hrtfmove_init(CSOUND *csound, hrtfmove *p)
{
    /* left and right data files: spectral mag, phase format. */
    MEMFIL *fpl = NULL,*fpr = NULL;
    int32_t i;
    char filel[MAXNAME],filer[MAXNAME];

    int32_t mode = (int32_t)*p->omode;
    int32_t fade = (int32_t)*p->ofade;
    MYFLT sr = *p->osr;

    MYFLT *win;

    /* time domain impulse length, padded, overlap add */
    int32_t irlength=0, irlengthpad=0, overlapsize=0;

    /* flag for process type: default phase trunc */
    if(mode == 1)
      {
        p->minphase = 1;
        p->phasetrunc = 0;
      }
    else
      {
        p->phasetrunc = 1;
        p->minphase = 0;
      }

    /* fade length: default 8, max 24, min 1 */
    if(fade < 1 || fade > 24)
      fade = 8;
    p->fade = fade;

    /* sr, default 44100 */
    //if(sr != 44100 && sr != 48000 && sr != 96000)
    //sr = 44100;

      if (sr==0) sr = CS_ESR;
      p->sr = sr;

      if (UNLIKELY(CS_ESR != sr))
      csound->Message(csound,
                      Str("\n\nWARNING!!:\nOrchestra SR not compatible"
                          " with HRTF processing SR of: %.0f\n\n"), sr);

    /* setup as per sr */
    if(sr == 44100 || sr == 48000)
      {
        irlength = 128;
        irlengthpad = 256;
        overlapsize = (irlength - 1);
      }
    else if(sr == 96000)
      {
        irlength = 256;
        irlengthpad = 512;
        overlapsize = (irlength - 1);
      }

    /* copy in string name */
    strncpy(filel, (char*) p->ifilel->data, MAXNAME-1); //filel[MAXNAME-1]='\0';
    strncpy(filer, (char*) p->ifiler->data, MAXNAME-1); //filel[MAXNAME-1]='\0';

    /* reading files, with byte swap */
    fpl = csound->LoadMemoryFile(csound, filel, CSFTYPE_FLOATS_BINARY,
                                   swap4bytes);
    if (UNLIKELY(fpl == NULL))
      return
        csound->InitError(csound,
                          "%s", Str("\n\n\nCannot load left data file, exiting\n\n"));

    fpr = csound->LoadMemoryFile(csound, filer, CSFTYPE_FLOATS_BINARY,
                                   swap4bytes);
    if (UNLIKELY(fpr == NULL))
      return
        csound->InitError(csound,
                          "%s", Str("\n\n\nCannot load right data file, exiting\n\n"));

    p->irlength = irlength;
    p->irlengthpad = irlengthpad;
    p->overlapsize = overlapsize;

    /* the amount of buffers to fade over. */
    p->fadebuffer = (int32_t)fade*irlength;

    /* file handles */
    p->fpbeginl = (float *) fpl->beginp;
    p->fpbeginr = (float *) fpr->beginp;

    /* common buffers (used by both min phase and phasetrunc) */
    if (!p->insig.auxp || p->insig.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->insig);
    if (!p->outl.auxp || p->outl.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->outl);
    if (!p->outr.auxp || p->outr.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->outr);
    if (!p->hrtflpad.auxp || p->hrtflpad.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->hrtflpad);
    if (!p->hrtfrpad.auxp || p->hrtfrpad.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->hrtfrpad);
    if (!p->complexinsig.auxp || p->complexinsig.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->complexinsig);
    if (!p->hrtflfloat.auxp || p->hrtflfloat.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->hrtflfloat);
    if (!p->hrtfrfloat.auxp || p->hrtfrfloat.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->hrtfrfloat);
    if (!p->outspecl.auxp || p->outspecl.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->outspecl);
    if (!p->outspecr.auxp || p->outspecr.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->outspecr);
    if (!p->overlapl.auxp || p->overlapl.size < overlapsize * sizeof(MYFLT))
      csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &p->overlapl);
    if (!p->overlapr.auxp || p->overlapr.size < overlapsize * sizeof(MYFLT))
      csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &p->overlapr);

    memset(p->insig.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->outl.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->outr.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->hrtflpad.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->hrtfrpad.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->complexinsig.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->hrtflfloat.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->hrtfrfloat.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->outspecl.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->outspecr.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->overlapl.auxp, 0, overlapsize * sizeof(MYFLT));
    memset(p->overlapr.auxp, 0, overlapsize * sizeof(MYFLT));

    /* interpolation values */
    if (!p->lowl1.auxp || p->lowl1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowl1);
    if (!p->lowr1.auxp || p->lowr1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowr1);
    if (!p->lowl2.auxp || p->lowl2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowl2);
    if (!p->lowr2.auxp || p->lowr2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowr2);
    if (!p->highl1.auxp || p->highl1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highl1);
    if (!p->highr1.auxp || p->highr1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highr1);
    if (!p->highl2.auxp || p->highl2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highl2);
    if (!p->highr2.auxp || p->highr2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highr2);
    if (!p->currentphasel.auxp || p->currentphasel.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->currentphasel);
    if (!p->currentphaser.auxp || p->currentphaser.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->currentphaser);

    memset(p->lowl1.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->lowr1.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->lowl2.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->lowr2.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->highl1.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->highl2.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->highr1.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->highr2.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->currentphasel.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->currentphaser.auxp, 0, irlength * sizeof(MYFLT));

    /* phase truncation buffers and variables */
    if (!p->oldhrtflpad.auxp || p->oldhrtflpad.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->oldhrtflpad);
    if (!p->oldhrtfrpad.auxp || p->oldhrtfrpad.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->oldhrtfrpad);
    if (!p->outlold.auxp || p->outlold.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->outlold);
    if (!p->outrold.auxp || p->outrold.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->outrold);
    if (!p->outspecoldl.auxp || p->outspecoldl.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->outspecoldl);
    if (!p->outspecoldr.auxp || p->outspecoldr.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->outspecoldr);
    if (!p->overlapoldl.auxp || p->overlapoldl.size < overlapsize * sizeof(MYFLT))
      csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &p->overlapoldl);
    if (!p->overlapoldr.auxp || p->overlapoldr.size < overlapsize * sizeof(MYFLT))
      csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &p->overlapoldr);

    memset(p->oldhrtflpad.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->oldhrtfrpad.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->outlold.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->outrold.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->outspecoldl.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->outspecoldr.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->overlapoldl.auxp, 0, overlapsize * sizeof(MYFLT));
    memset(p->overlapoldr.auxp, 0, overlapsize * sizeof(MYFLT));

    /* initialize counters and indices */
    p->counter = 0;
    p->cross = 0;
    p->l = 0;
    p->initialfade = 0;

    /* need to be a value that is not possible for first check to avoid
       phase not being read. */
    p->oldelevindex = -1;
    p->oldangleindex = -1;

    /* buffer declaration for min phase calculations */
    if (!p->logmagl.auxp || p->logmagl.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->logmagl);
    if (!p->logmagr.auxp || p->logmagr.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->logmagr);
    if (!p->xhatwinl.auxp || p->xhatwinl.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->xhatwinl);
    if (!p->xhatwinr.auxp || p->xhatwinr.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->xhatwinr);
    if (!p->expxhatwinl.auxp || p->expxhatwinl.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->expxhatwinl);
    if (!p->expxhatwinr.auxp || p->expxhatwinr.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->expxhatwinr);

    memset(p->logmagl.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->logmagr.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->xhatwinl.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->xhatwinr.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->expxhatwinl.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->expxhatwinr.auxp, 0, irlength * sizeof(MYFLT));

    /* delay buffers */
    if (!p->delmeml.auxp ||
        p->delmeml.size < (int32_t)(sr * maxdeltime) * sizeof(MYFLT))
      csound->AuxAlloc(csound,
                       (int32_t)(sr * maxdeltime) * sizeof(MYFLT), &p->delmeml);
    if (!p->delmemr.auxp ||
        p->delmemr.size < (int32_t)(sr * maxdeltime) * sizeof(MYFLT))
      csound->AuxAlloc(csound,
                       (int32_t)(sr * maxdeltime) * sizeof(MYFLT), &p->delmemr);

    memset(p->delmeml.auxp, 0, (int32_t)(sr * maxdeltime) * sizeof(MYFLT));
    memset(p->delmemr.auxp, 0, (int32_t)(sr * maxdeltime) * sizeof(MYFLT));

    if (!p->win.auxp || p->win.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->win);

    win = (MYFLT *)p->win.auxp;

    /* min phase win defined for irlength point impulse! */
    win[0] = FL(1.0);
    for(i = 1; i < (irlength / 2); i++)
      win[i] = FL(2.0);
    win[(irlength / 2)] = FL(1.0);
    for(i = ((irlength / 2) + 1); i < irlength; i++)
      win[i] = FL(0.0);

    p->mdtl = (int32_t)(FL(0.00095) * sr);
    p->mdtr = (int32_t)(FL(0.00095) * sr);
    p->delayfloat = FL(0.0);

    p->ptl = 0;
    p->ptr = 0;

    /* setup values used to check if src has moved, illegal values to
       start with to ensure first read */
    p->anglev = -1;
    p->elevv = -41;
    p->setup_pad = csound->RealFFTSetup(csound, p->irlengthpad, FFT_FWD);
    p->setup = csound->RealFFTSetup(csound, p->irlength, FFT_FWD);
    p->isetup_pad = csound->RealFFTSetup(csound, p->irlengthpad, FFT_INV);
    p->isetup = csound->RealFFTSetup(csound, p->irlength, FFT_INV);
    return OK;
}


static int32_t hrtfmove_process(CSOUND *csound, hrtfmove *p)
{
    /* local pointers to p */
    MYFLT *in = p->in;
    MYFLT *outsigl  = p->outsigl;
    MYFLT *outsigr = p->outsigr;

    /* common buffers and variables */
    MYFLT *insig = (MYFLT *)p->insig.auxp;
    MYFLT *outl = (MYFLT *)p->outl.auxp;
    MYFLT *outr = (MYFLT *)p->outr.auxp;

    MYFLT *hrtflpad = (MYFLT *)p->hrtflpad.auxp;
    MYFLT *hrtfrpad = (MYFLT *)p->hrtfrpad.auxp;

    MYFLT *complexinsig = (MYFLT *)p->complexinsig.auxp;
    MYFLT *hrtflfloat = (MYFLT *)p->hrtflfloat.auxp;
    MYFLT *hrtfrfloat = (MYFLT *)p->hrtfrfloat.auxp;
    MYFLT *outspecl = (MYFLT *)p->outspecl.auxp;
    MYFLT *outspecr = (MYFLT *)p->outspecr.auxp;

    MYFLT *overlapl = (MYFLT *)p->overlapl.auxp;
    MYFLT *overlapr = (MYFLT *)p->overlapr.auxp;

    MYFLT elev = *p->kelev;
    MYFLT angle = *p->kangle;

    int32_t counter = p->counter;

    /* pointers into HRTF files: floating point data (even in 64 bit csound) */
    float *fpindexl;
    float *fpindexr;

    int32_t i,elevindex, angleindex, skip = 0;

    int32_t minphase = p->minphase;
    int32_t phasetrunc = p->phasetrunc;

    MYFLT sr = p->sr;

    int32_t irlength = p->irlength;
    int32_t irlengthpad = p->irlengthpad;
    int32_t overlapsize = p->overlapsize;

    /* local variables, mainly used for simplification */
    MYFLT elevindexstore;
    MYFLT angleindexlowstore;
    MYFLT angleindexhighstore;

    /* interpolation values */
    MYFLT *lowl1 = (MYFLT *)p->lowl1.auxp;
    MYFLT *lowr1 = (MYFLT *)p->lowr1.auxp;
    MYFLT *lowl2 = (MYFLT *)p->lowl2.auxp;
    MYFLT *lowr2 = (MYFLT *)p->lowr2.auxp;
    MYFLT *highl1 = (MYFLT *)p->highl1.auxp;
    MYFLT *highr1 = (MYFLT *)p->highr1.auxp;
    MYFLT *highl2 = (MYFLT *)p->highl2.auxp;
    MYFLT *highr2 = (MYFLT *)p->highr2.auxp;
    MYFLT *currentphasel = (MYFLT *)p->currentphasel.auxp;
    MYFLT *currentphaser = (MYFLT *)p->currentphaser.auxp;

    /* local interpolation values */
    MYFLT elevindexhighper, angleindex2per, angleindex4per;
    int32_t elevindexlow, elevindexhigh, angleindex1, angleindex2,
      angleindex3, angleindex4;
    MYFLT magl,magr,phasel,phaser, magllow, magrlow, maglhigh, magrhigh;

    /* phase truncation buffers and variables */
    MYFLT *oldhrtflpad = (MYFLT *)p->oldhrtflpad.auxp;
    MYFLT *oldhrtfrpad = (MYFLT *)p->oldhrtfrpad.auxp;
    MYFLT *outlold = (MYFLT *)p->outlold.auxp;
    MYFLT *outrold = (MYFLT *)p->outrold.auxp;
    MYFLT *outspecoldl = (MYFLT *)p->outspecoldl.auxp;
    MYFLT *outspecoldr = (MYFLT *)p->outspecoldr.auxp;
    MYFLT *overlapoldl = (MYFLT *)p->overlapoldl.auxp;
    MYFLT *overlapoldr = (MYFLT *)p->overlapoldr.auxp;

    int32_t oldelevindex = p ->oldelevindex;
    int32_t oldangleindex = p ->oldangleindex;

    int32_t cross = p ->cross;
    int32_t l = p->l;
    int32_t initialfade = p->initialfade;

    int32_t crossfade;
    int32_t crossout;

    int32_t fade = p->fade;
    int32_t fadebuffer = p->fadebuffer;

    /* minimum phase buffers */
    MYFLT *logmagl = (MYFLT *)p->logmagl.auxp;
    MYFLT *logmagr = (MYFLT *)p->logmagr.auxp;
    MYFLT *xhatwinl = (MYFLT *)p->xhatwinl.auxp;
    MYFLT *xhatwinr = (MYFLT *)p->xhatwinr.auxp;
    MYFLT *expxhatwinl = (MYFLT *)p->expxhatwinl.auxp;
    MYFLT *expxhatwinr = (MYFLT *)p->expxhatwinr.auxp;

    /* min phase window */
    MYFLT *win = (MYFLT *)p->win.auxp;

    /* min phase delay variables */
    MYFLT *delmeml = (MYFLT *)p->delmeml.auxp;
    MYFLT *delmemr = (MYFLT *)p->delmemr.auxp;
    MYFLT delaylow1, delaylow2, delayhigh1, delayhigh2, delaylow, delayhigh;
    MYFLT delayfloat = p->delayfloat;
    int32_t ptl = p->ptl;
    int32_t ptr = p->ptr;
    int32_t mdtl = p->mdtl;
    int32_t mdtr = p->mdtr;
    int32_t posl, posr;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t j, nsmps = CS_KSMPS;
    MYFLT outvdl, outvdr, vdtl, vdtr, fracl, fracr, rpl, rpr;

    /* start indices at correct value (start of file)/ zero indices. */
    fpindexl = (float *) p->fpbeginl;
    fpindexr = (float *) p->fpbeginr;

    if (UNLIKELY(offset)) {
      memset(outsigl, '\0', offset*sizeof(MYFLT));
      memset(outsigr, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&outsigl[nsmps], '\0', early*sizeof(MYFLT));
      memset(&outsigr[nsmps], '\0', early*sizeof(MYFLT));
    }
    for(j = offset; j < nsmps; j++)
      {
        /* ins and outs */
        insig[counter] = in[j];

        outsigl[j] = outl[counter];
        outsigr[j] = outr[counter];

        counter++;

        if(phasetrunc)
          {
            /* used to ensure fade does not happen on first run */
            if(initialfade < (irlength + 2))
              initialfade++;
          }

        if(counter == irlength)
          {
            /* process a block */
            crossfade = 0;
            crossout = 0;

            if(elev > FL(90.0))
              elev = FL(90.0);
            if(elev < FL(-40.0))
              elev = FL(-40.0);

            while(angle < FL(0.0))
              angle += FL(360.0);
            while(angle >= FL(360.0))
              angle -= FL(360.0);

            /* only update if location changes! */
            if(angle != p->anglev || elev != p->elevv)
              {
                /* two nearest elev indices to avoid recalculating */
                elevindexstore = (elev - minelev) / elevincrement;
                elevindexlow = (int32_t)elevindexstore;

                if(elevindexlow < 13)
                  elevindexhigh = elevindexlow + 1;
                /* highest index reached */
                else
                  elevindexhigh = elevindexlow;

                /* get percentage value for interpolation */
                elevindexhighper = elevindexstore - elevindexlow;

                /* read using an index system based on number of points
                   measured per elevation at mit */
                /* lookup indices, used to check for crossfade */
                elevindex = (int32_t)(elevindexstore + FL(0.5));

                angleindex = (int32_t)(angle/(FL(360.0) / elevationarray[elevindex])
                                   + FL(0.5));
                angleindex = angleindex % elevationarray[elevindex];

                /* avoid recalculation */
                angleindexlowstore = angle / (FL(360.0) /
                                              elevationarray[elevindexlow]);
                angleindexhighstore = angle / (FL(360.) /
                                               elevationarray[elevindexhigh]);

                /* 4 closest indices, 2 low and 2 high */
                angleindex1 = (int32_t)angleindexlowstore;

                angleindex2 = angleindex1 + 1;
                angleindex2 = angleindex2 % elevationarray[elevindexlow];

                angleindex3 = (int32_t)angleindexhighstore;

                angleindex4 = angleindex3 + 1;
                angleindex4 = angleindex4 % elevationarray[elevindexhigh];

                /* angle percentages for interp */
                angleindex2per = angleindexlowstore - angleindex1;
                angleindex4per = angleindexhighstore - angleindex3;

                if(phasetrunc)
                  {
                    if(angleindex!=oldangleindex || elevindex!=oldelevindex)
                      {
                        /* store last point and turn crossfade on, provided that
                           initialfade value indicates first block processed! */
                        /* (otherwise,there will be a fade in at the start). */
                        if(initialfade>irlength)
                          {
                            /* post warning if fades ovelap */
                            if(cross)
                              {
                                csound->Message(csound,
                                                "%s", Str("\nWARNING: fades are "
                                                    "overlapping: this could lead"
                                                    " to noise: reduce fade size "
                                                    "or change trajectory\n\n"));
                                cross = 0;
                              }
                            /* reset l, use as index to fade */
                            l = 0;
                            crossfade = 1;
                            /* store old data */
                            for(i = 0; i < irlengthpad; i++)
                              {
                                oldhrtflpad[i] = hrtflpad[i];
                                oldhrtfrpad[i] = hrtfrpad[i];
                              }
                          }

                        /* store point for current phase as trajectory comes
                           closer to a new index */
                        skip = 0;
                        /* store current phase */
                        if(angleindex > elevationarray[elevindex] / 2)
                          {
                            for(i = 0; i < elevindex; i++)
                              skip +=((int32_t)(elevationarray[i] / 2)+1)*irlength;
                            for (i = 0;
                                 i < (elevationarray[elevindex] - angleindex);
                                 i++)
                              skip += irlength;
                            for(i = 0; i < irlength; i++)
                              {
                                currentphasel[i] = fpindexr[skip + i];
                                currentphaser[i] = fpindexl[skip + i];
                              }
                          }
                        else
                          {
                            for(i = 0; i < elevindex; i++)
                              skip +=((int32_t)(elevationarray[i] / 2)+1)*irlength;
                            for (i = 0; i < angleindex; i++)
                              skip += irlength;
                            for(i = 0; i < irlength; i++)
                              {
                                currentphasel[i] = fpindexl[skip+i];
                                currentphaser[i] = fpindexr[skip+i];
                              }
                          }
                      }
                  }
                /* for next check */
                p->oldelevindex = elevindex;
                p->oldangleindex = angleindex;

                /* read 4 nearest HRTFs */
                skip = 0;
                /* switch l and r */
                if(angleindex1 > elevationarray[elevindexlow] / 2)
                  {
                    for(i = 0; i < elevindexlow; i++)
                      skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0;
                         i < (elevationarray[elevindexlow] - angleindex1);
                         i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        lowl1[i] = fpindexr[skip+i];
                        lowr1[i] = fpindexl[skip+i];
                      }
                  }
                else
                  {
                    for(i = 0; i < elevindexlow; i++)
                      skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0; i < angleindex1; i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        lowl1[i] = fpindexl[skip+i];
                        lowr1[i] = fpindexr[skip+i];
                      }
                  }

                skip = 0;
                if(angleindex2 > elevationarray[elevindexlow] / 2)
                  {
                    for(i = 0; i < elevindexlow; i++)
                      skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0;
                         i < (elevationarray[elevindexlow] - angleindex2);
                         i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        lowl2[i] = fpindexr[skip+i];
                        lowr2[i] = fpindexl[skip+i];
                      }
                  }
                else
                  {
                    for(i = 0; i < elevindexlow; i++)
                      skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0; i < angleindex2; i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        lowl2[i] = fpindexl[skip+i];
                        lowr2[i] = fpindexr[skip+i];
                      }
                  }

                skip = 0;
                if(angleindex3 > elevationarray[elevindexhigh] / 2)
                  {
                    for(i = 0; i < elevindexhigh; i++)
                      skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0;
                         i < (elevationarray[elevindexhigh] - angleindex3);
                         i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        highl1[i] = fpindexr[skip+i];
                        highr1[i] = fpindexl[skip+i];
                      }
                  }
                else
                  {
                    for(i = 0; i < elevindexhigh; i++)
                      skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0; i < angleindex3; i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        highl1[i] = fpindexl[skip+i];
                        highr1[i] = fpindexr[skip+i];
                      }
                  }

                skip = 0;
                if(angleindex4 > elevationarray[elevindexhigh] / 2)
                  {
                    for(i = 0; i < elevindexhigh; i++)
                      skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0;
                         i < (elevationarray[elevindexhigh] - angleindex4);
                         i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        highl2[i] = fpindexr[skip+i];
                        highr2[i] = fpindexl[skip+i];
                      }
                  }
                else
                  {
                    for(i = 0; i < elevindexhigh; i++)
                      skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0; i < angleindex4; i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        highl2[i] = fpindexl[skip+i];
                        highr2[i] = fpindexr[skip+i];
                      }
                  }

                /* interpolation */
                /* 0 Hz and Nyq...absoulute values for mag */
                magllow = FABS(lowl1[0]) + (FABS(lowl2[0]) - FABS(lowl1[0])) *
                  angleindex2per;
                maglhigh = FABS(highl1[0]) + (FABS(highl2[0]) - FABS(highl1[0]))
                  * angleindex4per;
                magl = magllow + (maglhigh - magllow) * elevindexhighper;
                if(minphase)
                  {
                    logmagl[0] = LOG((magl == FL(0.0) ? FL(0.00000001) : magl));
                  }
                /* this is where real values of 0hz and nyq needed:
                   if neg real, 180 degree phase */
                /* if data was complex, mag interp would use fabs()
                   inherently, phase would be 0/pi */
                /* if pi, real is negative! */
                else
                  {
                    if(currentphasel[0] < FL(0.0))
                      hrtflfloat[0] = -magl;
                    else
                      hrtflfloat[0] = magl;
                  }

                magllow = FABS(lowl1[1]) + (FABS(lowl2[1]) - FABS(lowl1[1])) *
                  angleindex2per;
                maglhigh = FABS(highl1[1]) + (FABS(highl2[1]) - FABS(highl1[1])) *
                  angleindex4per;
                magl = magllow + (maglhigh-magllow) * elevindexhighper;
                if(minphase)
                  {
                    logmagl[1] = LOG(magl == FL(0.0) ? FL(0.00000001) : magl);
                  }
                else

                  {
                    if(currentphasel[1] < FL(0.0))
                      hrtflfloat[1] = -magl;
                    else
                      hrtflfloat[1] = magl;
                  }

                magrlow = FABS(lowr1[0]) + (FABS(lowr2[0]) - FABS(lowr1[0])) *
                  angleindex2per;
                magrhigh = FABS(highr1[0]) + (FABS(highr2[0]) - FABS(highr1[0])) *
                  angleindex4per;
                magr = magrlow + (magrhigh - magrlow) * elevindexhighper;
                if(minphase)
                  {
                    logmagr[0] = LOG(magr == FL(0.0) ? FL(0.00000001) : magr);
                  }
                else
                  {
                    if(currentphaser[0] < FL(0.0))
                      hrtfrfloat[0] = -magr;
                    else
                      hrtfrfloat[0] = magr;
                  }

                magrlow = FABS(lowr1[1]) + (FABS(lowr2[1]) - FABS(lowr1[1]))  *
                  angleindex2per;
                magrhigh = FABS(highr1[1]) + (FABS(highr2[1]) - FABS(highr1[1])) *
                  angleindex4per;
                magr = magrlow + (magrhigh - magrlow) * elevindexhighper;
                if(minphase)
                  {
                    logmagr[1] = LOG(magr == FL(0.0) ? FL(0.00000001) : magr);
                  }
                else
                  {
                    if(currentphaser[1] < FL(0.0))
                      hrtfrfloat[1] = -magr;
                    else
                      hrtfrfloat[1] = magr;
                  }

                /* remaining values */
                for(i = 2; i < irlength; i += 2)
                  {
                    /* interpolate high and low mags */
                    magllow = lowl1[i] + (lowl2[i] - lowl1[i]) * angleindex2per;
                    maglhigh = highl1[i] + (highl2[i] - highl1[i]) * angleindex4per;

                    magrlow = lowr1[i] + (lowr2[i] - lowr1[i]) * angleindex2per;
                    magrhigh = highr1[i] + (highr2[i] - highr1[i]) * angleindex4per;

                    /* interpolate high and low results */
                    magl = magllow + (maglhigh - magllow) * elevindexhighper;
                    magr = magrlow + (magrhigh - magrlow) * elevindexhighper;

                    if(phasetrunc)
                      {
                        /* use current phase, back to rectangular */
                        phasel = currentphasel[i + 1];
                        phaser = currentphaser[i + 1];

                        /* polar to rectangular */
                        hrtflfloat[i] = magl * COS(phasel);
                        hrtflfloat[i+1] = magl * SIN(phasel);

                        hrtfrfloat[i] = magr * COS(phaser);
                        hrtfrfloat[i+1] = magr * SIN(phaser);
                      }

                    if(minphase)
                      {
                        /* store log magnitudes, 0 phases for ifft, do not
                           allow log(0.0) */
                        logmagl[i] = LOG(magl == FL(0.0) ? FL(0.00000001) : magl);
                        logmagr[i] = LOG(magr == FL(0.0) ? FL(0.00000001) : magr);

                        logmagl[i + 1] = FL(0.0);
                        logmagr[i + 1] = FL(0.0);
                      }
                  }

                if(minphase)
                  {
                    /* ifft!...see Oppehneim and Schafer for min phase
                       process...based on real cepstrum method */
                    csound->RealFFT(csound, p->isetup, logmagl);
                    csound->RealFFT(csound,  p->isetup, logmagr);

                    /* window, note no need to scale on csound iffts... */
                    for(i = 0; i < irlength; i++)
                      {
                        xhatwinl[i] = logmagl[i] * win[i];
                        xhatwinr[i] = logmagr[i] * win[i];
                      }

                    /* fft */
                    csound->RealFFT(csound, p->setup, xhatwinl);
                    csound->RealFFT(csound, p->setup, xhatwinl);

                    /* exponential of result */
                    /* 0 hz and nyq purely real... */
                    expxhatwinl[0] = EXP(xhatwinl[0]);
                    expxhatwinl[1] = EXP(xhatwinl[1]);
                    expxhatwinr[0] = EXP(xhatwinr[0]);
                    expxhatwinr[1] = EXP(xhatwinr[1]);

                    /* exponential of real, cos/sin of imag */
                    for(i = 2; i < irlength; i += 2)
                      {
                        expxhatwinl[i] = EXP(xhatwinl[i]) * COS(xhatwinl[i + 1]);
                        expxhatwinl[i+1] = EXP(xhatwinl[i]) * SIN(xhatwinl[i + 1]);
                        expxhatwinr[i] = EXP(xhatwinr[i]) * COS(xhatwinr[i + 1]);
                        expxhatwinr[i+1] = EXP(xhatwinr[i]) * SIN(xhatwinr[i + 1]);
                      }

                    /* ifft for output buffers */
                    csound->RealFFT(csound,  p->isetup, expxhatwinl);
                    csound->RealFFT(csound,  p->isetup, expxhatwinr);

                    /* output */
                    for(i= 0; i < irlength; i++)
                      {
                        hrtflpad[i] = expxhatwinl[i];
                        hrtfrpad[i] = expxhatwinr[i];
                      }
                  }

                /* use current phase and interped mag directly */
                if(phasetrunc)
                  {
                    /* ifft */
                    csound->RealFFT(csound, p->setup, hrtflfloat);
                    csound->RealFFT(csound, p->setup, hrtfrfloat);

                    for (i = 0; i < irlength; i++)
                      {
                        /* scale and pad buffers with zeros to fftbuff */
                        hrtflpad[i] = hrtflfloat[i];
                        hrtfrpad[i] = hrtfrfloat[i];
                      }
                  }

                /* zero pad impulse */
                for(i = irlength; i < irlengthpad; i++)
                  {
                    hrtflpad[i] = FL(0.0);
                    hrtfrpad[i] = FL(0.0);
                  }

                /* back to freq domain */
                csound->RealFFT(csound,  p->setup_pad,hrtflpad);
                csound->RealFFT(csound,  p->setup_pad,hrtfrpad);

                if(minphase)
                  {
                    /* read delay data: 4 nearest points, as above */
                    /* point 1 */
                    skip = 0;
                    if(angleindex1 > elevationarray[elevindexlow] / 2)
                      {
                        for (i = 0; i < elevindexlow; i++)
                          skip += ((int32_t)(elevationarray[i] / 2) + 1);
                        for(i = 0;
                            i < (elevationarray[elevindexlow] - angleindex1);
                            i++)
                          skip++;
                        delaylow1 = minphasedels[skip];
                      }
                    else
                      {
                        for (i = 0; i < elevindexlow; i++)
                          skip += ((int32_t)(elevationarray[i] / 2) + 1);
                        for(i = 0; i < angleindex1; i++)
                          skip++;
                        delaylow1 = minphasedels[skip];
                      }

                    /* point 2 */
                    skip = 0;
                    if(angleindex2 > elevationarray[elevindexlow] / 2)
                      {
                        for (i = 0; i < elevindexlow; i++)
                          skip += ((int32_t)(elevationarray[i] / 2) + 1);
                        for(i = 0;
                            i < (elevationarray[elevindexlow] - angleindex2);
                            i++)
                          skip++;
                        delaylow2 = minphasedels[skip];
                      }
                    else
                      {
                        for (i = 0; i < elevindexlow; i++)
                          skip += ((int32_t)(elevationarray[i] / 2) + 1);
                        for (i = 0; i < angleindex2; i++)
                          skip++;
                        delaylow2 = minphasedels[skip];
                      }

                    /* point 3 */
                    skip = 0;
                    if(angleindex3 > elevationarray[elevindexhigh] / 2)
                      {
                        for (i = 0; i < elevindexhigh; i++)
                          skip += ((int32_t)(elevationarray[i] / 2) + 1);
                        for(i = 0;
                            i < (elevationarray[elevindexhigh] - angleindex3);
                            i++)
                          skip++;
                        delayhigh1  =minphasedels[skip];
                      }
                    else
                      {
                        for (i = 0; i < elevindexhigh; i++)
                          skip += ((int32_t)(elevationarray[i] / 2) + 1);
                        for (i = 0; i < angleindex3; i++)
                          skip++;
                        delayhigh1 = minphasedels[skip];
                      }

                    /* point 4 */
                    skip = 0;
                    if(angleindex4 > elevationarray[elevindexhigh] / 2)
                      {
                        for (i = 0; i < elevindexhigh; i++)
                          skip += ((int32_t)(elevationarray[i] / 2) + 1);
                        for(i = 0;
                            i < (elevationarray[elevindexhigh] - angleindex4);
                            i++)
                          skip++;
                        delayhigh2 = minphasedels[skip];
                      }
                    else
                      {
                        for (i = 0; i < elevindexhigh; i++)
                          skip += ((int32_t)(elevationarray[i] / 2) + 1);
                        for (i = 0; i < angleindex4; i++)
                          skip++;
                        delayhigh2 = minphasedels[skip];
                      }

                    /* delay interp */
                    delaylow = delaylow1 + ((delaylow2 - delaylow1) *
                                            angleindex2per);
                    delayhigh = delayhigh1 + ((delayhigh2 - delayhigh1) *
                                              angleindex4per);
                    delayfloat = delaylow + ((delayhigh - delaylow) *
                                             elevindexhighper);

                    p->delayfloat = delayfloat;
                  }
                /* end of angle/elev change process */
                p->elevv = elev;
                p->anglev = angle;
              }

            /* look after overlap add */
            for(i = 0; i < overlapsize ; i++)
              {
                overlapl[i] = outl[i + irlength];
                overlapr[i] = outr[i + irlength];
                /* look after fade */
                if(phasetrunc)
                  {
                    if(crossfade)
                      {
                        overlapoldl[i] = outl[i + irlength];
                        overlapoldr[i] = outr[i + irlength];
                      }
                    /* overlap will be previous fading out signal */
                    if(cross)
                      {
                        overlapoldl[i] = outlold[i + irlength];
                        overlapoldr[i] = outrold[i + irlength];
                      }
                  }
              }

            /* insert insig */
            for (i = 0; i < irlength; i++)
              complexinsig[i] = insig[i];

            for (i = irlength; i < irlengthpad; i++)
              complexinsig[i] = FL(0.0);

            csound->RealFFT(csound,p->setup_pad, complexinsig);

            /* complex mult function... */
            csound->RealFFTMult(csound, outspecl, hrtflpad, complexinsig,
                                irlengthpad, FL(1.0));
            csound->RealFFTMult(csound, outspecr, hrtfrpad, complexinsig,
                                irlengthpad, FL(1.0));

            /* convolution is the inverse FFT of above result */
            csound->RealFFT(csound,p->isetup_pad,outspecl);
            csound->RealFFT(csound,p->isetup_pad,outspecr);

            /* real values, scaled (by a little more than usual to ensure
               no clipping) sr related */
            for(i = 0; i < irlengthpad; i++)
              {
                outl[i] = outspecl[i] / (sr / FL(38000.0));
                outr[i] = outspecr[i] / (sr / FL(38000.0));
              }

            if(phasetrunc)
              {
                /* setup for fades */
                if(crossfade || cross)
                  {
                    crossout = 1;

                    csound->RealFFTMult(csound, outspecoldl, oldhrtflpad,
                                        complexinsig, irlengthpad, FL(1.0));
                    csound->RealFFTMult(csound, outspecoldr, oldhrtfrpad,
                                        complexinsig, irlengthpad, FL(1.0));

                    csound->RealFFT(csound,p->isetup_pad, outspecoldl);
                    csound->RealFFT(csound, p->isetup_pad,outspecoldr);

                    /* scaled */
                    for(i = 0; i < irlengthpad; i++)
                      {
                        outlold[i] = outspecoldl[i] / (sr / FL(38000.0));
                        outrold[i] = outspecoldr[i] / (sr / FL(38000.0));
                      }

                    cross++;
                    /* number of processing buffers in a fade */
                    cross = cross % fade;
                  }

                if(crossout)
                  {
                    /* do fade */
                    for(i = 0; i < irlength; i++)
                      {
                        outl[i] = ((outlold[i] +
                                    (i<overlapsize ? overlapoldl[i] : 0)) *
                                   (FL(1.0) - FL(l) / fadebuffer)) +
                          ((outl[i] + (i < overlapsize ? overlapl[i] : 0)) *
                           FL(l)/fadebuffer);
                        outr[i] = ((outrold[i] +
                                    (i<overlapsize ? overlapoldr[i] : 0)) *
                                   (FL(1.0) - FL(l) / fadebuffer)) +
                          ((outr[i] + (i < overlapsize ? overlapr[i] : 0)) *
                           FL(l)/fadebuffer);
                        l++;
                      }
                  }
                else
                  for(i = 0; i < irlength; i++) {
                    outl[i] = outl[i] + (i < overlapsize ? overlapl[i] : FL(0.0));
                    outr[i] = outr[i] + (i < overlapsize ? overlapr[i] : FL(0.0));
                  }
              }

            if(minphase)
              {
                /* use output direcly and add delay in time domain */
                for(i = 0; i < irlength; i++)
                  {
                    outl[i] = outl[i] + (i < overlapsize ? overlapl[i] : FL(0.0));
                    outr[i] = outr[i] + (i < overlapsize ? overlapr[i] : FL(0.0));
                  }

                if(angle > FL(180.0))
                  {
                    vdtr =  delayfloat * sr;
                    vdtl = FL(0.0);
                  }
                else
                  {
                    vdtr = FL(0.0);
                    vdtl = delayfloat * sr;
                  }

                /* delay right */
                if(vdtr > mdtr)
                  vdtr = FL(mdtr);
                for(i = 0; i < irlength; i++)
                  {
                    rpr = ptr - vdtr;
                    rpr = (rpr >= 0 ? (rpr < mdtr ? rpr : rpr - mdtr) : rpr + mdtr);
                    posr = (int32_t) rpr;
                    fracr = rpr - posr;
                    delmemr[ptr] = outr[i];
                    outvdr =  delmemr[posr] + fracr *
                      (delmemr[(posr + 1 < mdtr ? posr + 1 : 0)] - delmemr[posr]);
                    outr[i] = outvdr;
                    ptr = (ptr != mdtr - 1 ? ptr + 1 : 0);
                  }

                /* delay left */
                if(vdtl > mdtl)
                  vdtl = FL(mdtl);
                for(i = 0; i < irlength; i++)
                  {
                    rpl = ptl - vdtl;
                    rpl = (rpl >= 0 ? (rpl < mdtl ? rpl : rpl - mdtl) : rpl + mdtl);
                    posl = (int32_t) rpl;
                    fracl = rpl - (int32_t) posl;
                    delmeml[ptl] = outl[i];
                    outvdl =  delmeml[posl] + fracl *
                      (delmeml[(posl + 1 < mdtl ? posl + 1 : 0)] - delmeml[posl]);
                    outl[i] = outvdl;
                    ptl = (ptl != mdtl - 1 ? ptl + 1 : 0);
                  }

                p->ptl = ptl;
                p->ptr = ptr;
              }

            /* reset counter */
            counter = 0;
            if(phasetrunc)
              {
                /* update */
                p->cross = cross;
                p->l = l;
              }

          }       /* end of irlength == counter */

      }   /* end of ksmps audio loop */

    /* update */
    p->counter = counter;
    if(phasetrunc)
      p->initialfade = initialfade;

    return OK;
}

/* Csound hrtf magnitude interpolation, woodworth phase,
   static source: January 10 */
/* overlap add convolution */

/* aleft, aright hrtfstat ain, iang, iel, ifilel, ifiler->data [,iradius = 8.8,
   isr = 44100]...options of 48 and 96k sr */

/* see definitions above */

typedef struct
{
        OPDS  h;
        /* outputs and inputs */
        MYFLT *outsigl, *outsigr;
        MYFLT *in, *iangle, *ielev;
        STRINGDAT *ifilel, *ifiler;
        MYFLT *oradius, *osr;

        /*see definitions in INIT*/
        int32_t irlength, irlengthpad, overlapsize;
        MYFLT sroverN;

        int32_t counter;
        MYFLT sr;

        /* hrtf data padded */
        AUXCH hrtflpad,hrtfrpad;
        /* in and output buffers */
        AUXCH insig, outl, outr;

        /* memory local to perform method */
        /* insig fft */
        AUXCH complexinsig;
        /* hrtf buffers (rectangular complex form) */
        AUXCH hrtflfloat, hrtfrfloat;
        /* spectral data */
        AUXCH outspecl, outspecr;

        /* overlap data */
        AUXCH overlapl, overlapr;

        /* interpolation buffers */
        AUXCH lowl1, lowr1, lowl2, lowr2, highl1, highr1, highl2, highr2;

        /* buffers for impulse shift */
        AUXCH leftshiftbuffer, rightshiftbuffer;

  void *setup, *isetup, *isetup_pad, *setup_pad;
}
hrtfstat;

static int32_t hrtfstat_init(CSOUND *csound, hrtfstat *p)
{
    /* left and right data files: spectral mag, phase format. */
    MEMFIL *fpl = NULL, *fpr = NULL;
    char filel[MAXNAME], filer[MAXNAME];

    /* interpolation values */
    MYFLT *lowl1;
    MYFLT *lowr1;
    MYFLT *lowl2;
    MYFLT *lowr2;
    MYFLT *highl1;
    MYFLT *highr1;
    MYFLT *highl2;
    MYFLT *highr2;

    MYFLT *hrtflfloat;
    MYFLT *hrtfrfloat;

    MYFLT *hrtflpad;
    MYFLT *hrtfrpad;

    MYFLT elev = *p->ielev;
    MYFLT angle = *p->iangle;
    MYFLT r = *p->oradius;
    MYFLT sr = *p->osr;

        /* pointers into HRTF files */
    float *fpindexl=NULL;
    float *fpindexr=NULL;

    /* time domain impulse length, padded, overlap add */
    int32_t irlength=0, irlengthpad=0, overlapsize=0;

    int32_t i, skip = 0;

    /* local interpolation values */
    MYFLT elevindexhighper, angleindex2per, angleindex4per;
    int32_t elevindexlow, elevindexhigh, angleindex1, angleindex2,
      angleindex3, angleindex4;
    MYFLT magl, magr, phasel, phaser, magllow, magrlow, maglhigh, magrhigh;

    /* local variables, mainly used for simplification */
    MYFLT elevindexstore;
    MYFLT angleindexlowstore;
    MYFLT angleindexhighstore;

    /* woodworth values */
    MYFLT radianangle, radianelev, itd=0, itdww, freq;

    /* shift */
    int32_t shift;
    MYFLT *leftshiftbuffer;
    MYFLT *rightshiftbuffer;

    /* sr */
    if (sr == 0) sr = CS_ESR;
    if (sr != FL(44100.0) && sr != FL(48000.0) && sr != FL(96000.0))
      sr = FL(44100.0);
    p->sr = sr;

    if (UNLIKELY(CS_ESR != sr))
      csound->Message(csound,
                      Str("\n\nWARNING!!:\nOrchestra SR not compatible with "
                          "HRTF processing SR of: %.0f\n\n"), sr);

    /* setup as per sr */
    if(sr == 44100 || sr == 48000)
      {
        irlength = 128;
        irlengthpad = 256;
        overlapsize = (irlength - 1);
      }
    else if(sr == 96000)
      {
        irlength = 256;
        irlengthpad = 512;
        overlapsize = (irlength - 1);
      }

    /* copy in string name... */
    strncpy(filel, (char*) p->ifilel->data, MAXNAME-1); //filel[MAXNAME-1]='\0';
    strncpy(filer, (char*) p->ifiler->data, MAXNAME-1); //filel[MAXNAME-1]='\0';

    /* reading files, with byte swap */
    fpl = csound->LoadMemoryFile(csound, filel, CSFTYPE_FLOATS_BINARY,
                                   swap4bytes);
    if (UNLIKELY(fpl == NULL))
      return
        csound->InitError(csound,
                          "%s", Str("\n\n\nCannot load left data file, exiting\n\n"));

    fpr = csound->LoadMemoryFile(csound, filer, CSFTYPE_FLOATS_BINARY,
                                   swap4bytes);
    if (UNLIKELY(fpr == NULL))
      return
        csound->InitError(csound,
                          "%s", Str("\n\n\nCannot load right data file, exiting\n\n"));

    p->irlength = irlength;
    p->irlengthpad = irlengthpad;
    p->overlapsize = overlapsize;

    p->sroverN = sr/irlength;

    /* start indices at correct value (start of file)/ zero indices.
       (do not need to store here, as only accessing in INIT) */
    fpindexl = (float *) fpl->beginp;
    fpindexr = (float *) fpr->beginp;

    /* buffers */
    if (!p->insig.auxp || p->insig.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->insig);
    if (!p->outl.auxp || p->outl.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->outl);
    if (!p->outr.auxp || p->outr.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->outr);
    if (!p->hrtflpad.auxp || p->hrtflpad.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->hrtflpad);
    if (!p->hrtfrpad.auxp || p->hrtfrpad.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->hrtfrpad);
    if (!p->complexinsig.auxp || p->complexinsig.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p-> complexinsig);
    if (!p->hrtflfloat.auxp || p->hrtflfloat.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->hrtflfloat);
    if (!p->hrtfrfloat.auxp || p->hrtfrfloat.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->hrtfrfloat);
    if (!p->outspecl.auxp || p->outspecl.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->outspecl);
    if (!p->outspecr.auxp || p->outspecr.size < irlengthpad * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlengthpad*sizeof(MYFLT), &p->outspecr);
    if (!p->overlapl.auxp || p->overlapl.size < overlapsize * sizeof(MYFLT))
      csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &p->overlapl);
    if (!p->overlapr.auxp || p->overlapr.size < overlapsize * sizeof(MYFLT))
      csound->AuxAlloc(csound, overlapsize*sizeof(MYFLT), &p->overlapr);

    memset(p->insig.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->outl.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->outr.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->hrtflpad.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->hrtfrpad.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->complexinsig.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->hrtflfloat.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->hrtfrfloat.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->outspecl.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->outspecr.auxp, 0, irlengthpad * sizeof(MYFLT));
    memset(p->overlapl.auxp, 0, overlapsize * sizeof(MYFLT));
    memset(p->overlapr.auxp, 0, overlapsize * sizeof(MYFLT));

    /* interpolation values */
    if (!p->lowl1.auxp || p->lowl1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowl1);
    if (!p->lowr1.auxp || p->lowr1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowr1);
    if (!p->lowl2.auxp || p->lowl2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowl2);
    if (!p->lowr2.auxp || p->lowr2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowr2);
    if (!p->highl1.auxp || p->highl1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highl1);
    if (!p->highr1.auxp || p->highr1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highr1);
    if (!p->highl2.auxp || p->highl2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highl2);
    if (!p->highr2.auxp || p->highr2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highr2);

    /* best to zero, for future changes (filled in init) */
    memset(p->lowl1.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->lowr1.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->lowl2.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->lowr2.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->highl1.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->highl2.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->highr1.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->highr2.auxp, 0, irlength * sizeof(MYFLT));

    /* shift buffers */
    if (!p->leftshiftbuffer.auxp ||
        p->leftshiftbuffer.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->leftshiftbuffer);
    if (!p->rightshiftbuffer.auxp ||
        p->rightshiftbuffer.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength*sizeof(MYFLT), &p->rightshiftbuffer);

    memset(p->leftshiftbuffer.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->rightshiftbuffer.auxp, 0, irlength * sizeof(MYFLT));

    lowl1 = (MYFLT *)p->lowl1.auxp;
    lowr1 = (MYFLT *)p->lowr1.auxp;
    lowl2 = (MYFLT *)p->lowl2.auxp;
    lowr2 = (MYFLT *)p->lowr2.auxp;
    highl1 = (MYFLT *)p->highl1.auxp;
    highr1 = (MYFLT *)p->highr1.auxp;
    highl2 = (MYFLT *)p->highl2.auxp;
    highr2 = (MYFLT *)p->highr2.auxp;

    leftshiftbuffer = (MYFLT *)p->leftshiftbuffer.auxp;
    rightshiftbuffer = (MYFLT *)p->rightshiftbuffer.auxp;

    hrtflfloat = (MYFLT *)p->hrtflfloat.auxp;
    hrtfrfloat = (MYFLT *)p->hrtfrfloat.auxp;

    hrtflpad = (MYFLT *)p->hrtflpad.auxp;
    hrtfrpad = (MYFLT *)p->hrtfrpad.auxp;

    if(r <= 0 || r > 15)
      r = FL(8.8);

    if(elev > FL(90.0))
      elev = FL(90.0);
    if(elev < FL(-40.0))
      elev = FL(-40.0);

    while(angle < FL(0.0))
      angle += FL(360.0);
    while(angle >= FL(360.0))
      angle -= FL(360.0);

    /* two nearest elev indices to avoid recalculating */
    elevindexstore = (elev - minelev) / elevincrement;
    elevindexlow = (int32_t)elevindexstore;

    if(elevindexlow < 13)
      elevindexhigh = elevindexlow + 1;
    /* highest index reached */
    else
      elevindexhigh = elevindexlow;

    /* get percentage value for interpolation */
    elevindexhighper = elevindexstore - elevindexlow;

    /* avoid recalculation */
    angleindexlowstore = angle / (FL(360.0) / elevationarray[elevindexlow]);
    angleindexhighstore = angle / (FL(360.0) / elevationarray[elevindexhigh]);

    /* 4 closest indices, 2 low and 2 high */
    angleindex1 = (int32_t)angleindexlowstore;

    angleindex2 = angleindex1 + 1;
    angleindex2 = angleindex2 % elevationarray[elevindexlow];

    angleindex3 = (int32_t)angleindexhighstore;

    angleindex4 = angleindex3 + 1;
    angleindex4 = angleindex4 % elevationarray[elevindexhigh];

    /* angle percentages for interp */
    angleindex2per = angleindexlowstore - angleindex1;
    angleindex4per = angleindexhighstore - angleindex3;

    /* read 4 nearest HRTFs */
    skip = 0;
    /* switch l and r */
    if(angleindex1 > elevationarray[elevindexlow] / 2)
      {
        for(i = 0; i < elevindexlow; i++)
          skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
        for (i = 0; i < (elevationarray[elevindexlow] - angleindex1); i++)
          skip += irlength;
        for(i = 0; i < irlength; i++)
          {
            lowl1[i] = fpindexr[skip + i];
            lowr1[i] = fpindexl[skip + i];
          }
      }
    else
      {
        for(i = 0; i < elevindexlow; i++)
          skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
        for (i = 0; i < angleindex1; i++)
          skip += irlength;
        for(i = 0; i < irlength; i++)
          {
            lowl1[i] = fpindexl[skip + i];
            lowr1[i] = fpindexr[skip + i];
          }
      }

    skip = 0;
    if(angleindex2 > elevationarray[elevindexlow] / 2)
      {
        for(i = 0; i < elevindexlow; i++)
          skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
        for (i = 0; i < (elevationarray[elevindexlow] - angleindex2); i++)
          skip += irlength;
        for(i = 0; i < irlength; i++)
          {
            lowl2[i] = fpindexr[skip + i];
            lowr2[i] = fpindexl[skip + i];
          }
      }
    else
      {
        for(i = 0; i < elevindexlow; i++)
          skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
        for (i = 0; i < angleindex2; i++)
          skip += irlength;
        for(i = 0; i < irlength; i++)
          {
            lowl2[i] = fpindexl[skip + i];
            lowr2[i] = fpindexr[skip + i];
          }
      }

    skip = 0;
    if(angleindex3 > elevationarray[elevindexhigh] / 2)
      {
        for(i = 0; i < elevindexhigh; i++)
          skip += ((int32_t)(elevationarray[i] / 2) + 1) * irlength;
        for (i = 0; i < (elevationarray[elevindexhigh] - angleindex3); i++)
          skip += irlength;
        for(i = 0; i < irlength; i++)
          {
            highl1[i] = fpindexr[skip + i];
            highr1[i] = fpindexl[skip + i];
          }
      }
    else
      {
        for(i = 0; i < elevindexhigh; i++)
          skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
        for (i = 0; i < angleindex3; i++)
          skip += irlength;
        for(i = 0; i < irlength; i++)
          {
            highl1[i] = fpindexl[skip + i];
            highr1[i] = fpindexr[skip + i];
          }
      }

    skip = 0;
    if(angleindex4 > elevationarray[elevindexhigh] / 2)
      {
        for(i = 0; i < elevindexhigh; i++)
          skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
        for (i = 0; i < (elevationarray[elevindexhigh] - angleindex4); i++)
          skip += irlength;
        for(i = 0; i < irlength; i++)
          {
            highl2[i] = fpindexr[skip + i];
            highr2[i] = fpindexl[skip + i];
          }
      }
    else
      {
        for(i = 0; i < elevindexhigh; i++)
          skip +=((int32_t)(elevationarray[i] / 2) + 1) * irlength;
        for (i = 0; i < angleindex4; i++)
          skip += irlength;
        for(i = 0; i < irlength; i++)
          {
            highl2[i] = fpindexl[skip + i];
            highr2[i] = fpindexr[skip + i];
          }
      }

    /* woodworth process */
    /* ITD formula, check which ear is relevant to calculate angle from */
    if(angle > FL(180.0))
      radianangle = (angle - FL(180.0)) * PI_F / FL(180.0);
    else
      radianangle = angle * PI_F / FL(180.0);
    /* degrees to radians */
    radianelev = elev * PI_F / FL(180.0);

    /* get in correct range for formula */
    if(radianangle > PI_F / FL(2.0))
      radianangle = FL(PI) - radianangle;

    /* woodworth formula for itd */
    itdww = (radianangle + SIN(radianangle)) * r * COS(radianelev) / c;

    /* 0 Hz and Nyq... */
    /* these are real values...may be neg (implying phase of pi:
       in phase truncation), so need fabs... */
    magllow = FABS(lowl1[0]) + (FABS(lowl2[0]) - FABS(lowl1[0])) * angleindex2per;
    maglhigh = FABS(highl1[0]) + (FABS(highl2[0]) - FABS(highl1[0])) *
      angleindex4per;
    hrtflfloat[0] = magllow + (maglhigh - magllow) * elevindexhighper;

    magllow = FABS(lowl1[1]) + (FABS(lowl2[1]) - FABS(lowl1[1])) * angleindex2per;
    maglhigh = FABS(highl1[1]) + (FABS(highl2[1]) - FABS(highl1[1])) *
      angleindex4per;
    hrtflfloat[1] = magllow + (maglhigh - magllow) * elevindexhighper;

    magrlow = FABS(lowr1[0]) + (FABS(lowr2[0]) - FABS(lowr1[0])) * angleindex2per;
    magrhigh = FABS(highr1[0]) + (FABS(highr2[0]) - FABS(highr1[0])) *
      angleindex4per;
    hrtfrfloat[0] = magrlow + (magrhigh - magrlow) * elevindexhighper;

    magrlow = FABS(lowr1[1]) + (FABS(lowr2[1]) - FABS(lowr1[1])) * angleindex2per;
    magrhigh = FABS(highr1[1]) + (FABS(highr2[1]) - FABS(highr1[1])) *
      angleindex4per;
    hrtfrfloat[1] = magrlow + (magrhigh - magrlow) * elevindexhighper;

    /* magnitude interpolation */
    for(i = 2; i < irlength; i+=2)
      {
        /* interpolate high and low mags */
        magllow = lowl1[i] + (lowl2[i] - lowl1[i]) * angleindex2per;
        maglhigh = highl1[i]+(highl2[i] - highl1[i]) * angleindex4per;

        magrlow = lowr1[i] + (lowr2[i] - lowr1[i]) * angleindex2per;
        magrhigh = highr1[i] + (highr2[i] - highr1[i]) * angleindex4per;

        /* interpolate high and low results */
        magl = magllow + (maglhigh - magllow) * elevindexhighper;
        magr = magrlow + (magrhigh - magrlow) * elevindexhighper;

        freq = (i / 2) * p->sroverN;

        /* non linear itd...last value in array = 1.0, so back to itdww */
        if(p->sr == 96000)
          {
            if ((i / 2) < 6)
              itd = itdww * nonlinitd96k[(i / 2) - 1];
          }
        else if(p->sr == 48000)
          {
            if ((i / 2) < 6)
              itd = itdww * nonlinitd48k[(i / 2) - 1];
          }
        else if(p->sr == 44100)
          {
            if((i / 2) < 6)
              itd = itdww * nonlinitd[(i / 2) - 1];
          }

        if(angle > FL(180.))
          {
            phasel = TWOPI_F * freq * (itd / 2);
            phaser = TWOPI_F * freq * -(itd / 2);
          }
        else
          {
            phasel = TWOPI_F * freq * -(itd / 2);
            phaser = TWOPI_F * freq * (itd / 2);
          }

        /* polar to rectangular */
        hrtflfloat[i] = magl * COS(phasel);
        hrtflfloat[i+1] = magl * SIN(phasel);

        hrtfrfloat[i] = magr * COS(phaser);
        hrtfrfloat[i+1] = magr * SIN(phaser);
      }

    p->setup_pad = csound->RealFFTSetup(csound, p->irlengthpad, FFT_FWD);
    p->setup = csound->RealFFTSetup(csound, p->irlength, FFT_FWD);
    p->isetup_pad = csound->RealFFTSetup(csound, p->irlengthpad, FFT_INV);
    p->isetup = csound->RealFFTSetup(csound, p->irlength, FFT_INV);

    /* ifft */
    csound->RealFFT(csound,  p->isetup, hrtflfloat);
    csound->RealFFT(csound,  p->isetup, hrtfrfloat);

    for (i = 0; i < irlength; i++)
      {
        /* scale and pad buffers with zeros to fftbuff */
        leftshiftbuffer[i] = hrtflfloat[i];
        rightshiftbuffer[i] = hrtfrfloat[i];
    }

    /* shift for causality...impulse as is is centred around zero time lag...
       then phase added. */
    /* this step centres impulse around centre tap of filter (then phase
       moves it for correct itd...) */
    shift = irlength / 2;

    for(i = 0; i < irlength; i++)
      {
        hrtflpad[i] = leftshiftbuffer[shift];
        hrtfrpad[i] = rightshiftbuffer[shift];

        shift++;
        shift = shift % irlength;
    }

    /* zero pad impulse */
    for(i = irlength; i < irlengthpad; i++)
      {
        hrtflpad[i] = FL(0.0);
        hrtfrpad[i] = FL(0.0);
      }

    /* back to freq domain */
    csound->RealFFT(csound,  p->setup_pad, hrtflpad);
    csound->RealFFT(csound,  p->setup_pad, hrtfrpad);

        /* initialize counter */
    p->counter = 0;

    return OK;
}


static int32_t hrtfstat_process(CSOUND *csound, hrtfstat *p)
{
        /* local pointers to p */
    MYFLT *in = p->in;
    MYFLT *outsigl  = p->outsigl;
    MYFLT *outsigr = p->outsigr;

    /* common buffers and variables */
    MYFLT *insig = (MYFLT *)p->insig.auxp;
    MYFLT *outl = (MYFLT *)p->outl.auxp;
    MYFLT *outr = (MYFLT *)p->outr.auxp;

    MYFLT *hrtflpad = (MYFLT *)p->hrtflpad.auxp;
    MYFLT *hrtfrpad = (MYFLT *)p->hrtfrpad.auxp;

    MYFLT *complexinsig = (MYFLT *)p->complexinsig.auxp;
    MYFLT *outspecl = (MYFLT *)p->outspecl.auxp;
    MYFLT *outspecr = (MYFLT *)p->outspecr.auxp;

    MYFLT *overlapl = (MYFLT *)p->overlapl.auxp;
    MYFLT *overlapr = (MYFLT *)p->overlapr.auxp;

    int32_t counter = p->counter;
    int32_t i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t j, nsmps = CS_KSMPS;

    int32_t irlength = p->irlength;
    int32_t irlengthpad = p->irlengthpad;
    int32_t overlapsize = p->overlapsize;

    MYFLT sr = p->sr;

    if (UNLIKELY(offset)) {
      memset(outsigl, '\0', offset*sizeof(MYFLT));
      memset(outsigr, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&outsigl[nsmps], '\0', early*sizeof(MYFLT));
      memset(&outsigr[nsmps], '\0', early*sizeof(MYFLT));
    }
    for(j = offset; j < nsmps; j++)
      {
        /* ins and outs */
        insig[counter] = in[j];

        outsigl[j] = outl[counter];
        outsigr[j] = outr[counter];

        counter++;

        if(counter == irlength)
          {
            /* process a block */
            /* look after overlap add stuff */
            for(i = 0; i < overlapsize ; i++)
              {
                overlapl[i] = outl[i+irlength];
                overlapr[i] = outr[i+irlength];
              }

            /* insert insig for complex real,im fft, zero pad */
            for (i = 0; i <  irlength; i++)
              complexinsig[i] = insig[i];

            for (i = irlength; i <  irlengthpad; i++)
              complexinsig[i] = FL(0.0);

            csound->RealFFT(csound, p->setup_pad, complexinsig);

            /* complex multiplication */
            csound->RealFFTMult(csound, outspecl, hrtflpad, complexinsig,
                                irlengthpad, FL(1.0));
            csound->RealFFTMult(csound, outspecr, hrtfrpad, complexinsig,
                                irlengthpad, FL(1.0));

            /* convolution is the inverse FFT of above result */
            csound->RealFFT(csound,  p->isetup_pad,outspecl);
            csound->RealFFT(csound, p->isetup_pad, outspecr);

            /* scaled by a factor related to sr...? */
            for(i = 0; i < irlengthpad; i++)
              {
                outl[i] = outspecl[i] / (sr / FL(38000.0));
                outr[i] = outspecr[i] / (sr / FL(38000.0));
              }

            for(i = 0; i < irlength; i++)
              {
                outl[i] = outl[i] + (i < overlapsize ? overlapl[i] : FL(0.0));
                outr[i] = outr[i] + (i < overlapsize ? overlapr[i] : FL(0.0));
              }

            /* reset counter */
            counter = 0;

          }       /* end of irlength == counter */

      }   /* end of ksmps audio loop */

    /* update */
    p->counter = counter;

    return OK;
}

/* Csound hrtf magnitude interpolation, dynamic woodworth trajectory */
/* stft from fft.cpp in sndobj... */

/* stft based on sndobj implementation...some notes: */
/* using an overlapskip (same as m_counter) for in and out to control
   seperately for clarity... */

/* aleft, aright hrtfmove2 ain, kang, kel, ifilel, ifiler [, ioverlap = 4,
   iradius = 8.8, isr = 44100] */
/* ioverlap is stft overlap, iradius is head radius, sr can also be 48000
   and 96000 */

typedef struct
{
        OPDS  h;
        /* outputs and inputs */
        MYFLT *outsigl, *outsigr;
  MYFLT *in, *kangle, *kelev;
  STRINGDAT *ifilel, *ifiler;
MYFLT *ooverlap, *oradius, *osr;

        /* check if relative source has changed! */
        MYFLT anglev, elevv;

        /* see definitions in INIT */
        int32_t irlength;
        MYFLT sroverN;
        MYFLT sr;

        /* test inputs in init, get accepted value/default, and store in
           variables below. */
        int32_t overlap;
        MYFLT radius;

        int32_t hopsize;

        float *fpbeginl,*fpbeginr;

        /* to keep track of process */
        int32_t counter, t;

        /* in and output buffers */
        AUXCH inbuf;
        AUXCH outbufl, outbufr;

        /* memory local to perform method */
        /* insig fft */
        AUXCH complexinsig;
        /* hrtf buffers (rectangular complex form) */
        AUXCH hrtflfloat, hrtfrfloat;
        /* spectral data */
        AUXCH outspecl, outspecr;

        /* interpolation buffers */
        AUXCH lowl1,lowr1,lowl2,lowr2,highl1,highr1,highl2,highr2;

        /* stft window */
        AUXCH win;
        /* used for skipping into next stft array on way in and out */
        AUXCH overlapskipin, overlapskipout;
  void *setup, *isetup, *setup_pad, *isetup_pad;

}
hrtfmove2;

static int32_t hrtfmove2_init(CSOUND *csound, hrtfmove2 *p)
{
    /* left and right data files: spectral mag, phase format. */
    MEMFIL *fpl = NULL, *fpr = NULL;

    char filel[MAXNAME], filer[MAXNAME];

    /* time domain impulse length */
    int32_t irlength=0;

    /* stft window */
    MYFLT *win;
    /* overlap skip buffers */
    int32_t *overlapskipin, *overlapskipout;
    //MYFLT *inbuf;
    //MYFLT *outbufl, *outbufr;

    int32_t overlap = (int32_t)*p->ooverlap;
    MYFLT r = *p->oradius;
    MYFLT sr = *p->osr;

    int32_t i = 0;

    if (sr==0) sr = CS_ESR;
    if(sr != 44100 && sr != 48000 && sr != 96000)
      sr = 44100;
    p->sr = sr;

    if (UNLIKELY(CS_ESR != sr))
      csound->Message(csound,
                       Str("\n\nWARNING!!:\nOrchestra SR not compatible"
                          "with HRTF processing SR of: %.0f\n\n"), sr);

    /* setup as per sr */
    if(sr == 44100 || sr == 48000)
      irlength = 128;
    else if(sr == 96000)
      irlength = 256;

    /* copy in string name... */
    strncpy(filel, (char*) p->ifilel->data, MAXNAME-1); //filel[MAXNAME-1] = '\0';
    strncpy(filer, (char*) p->ifiler->data, MAXNAME-1); //filer[MAXNAME-1] = '\0';

    /* reading files, with byte swap */
    fpl = csound->LoadMemoryFile(csound, filel, CSFTYPE_FLOATS_BINARY,
                                   swap4bytes);
    if (UNLIKELY(fpl == NULL))
     return
        csound->InitError(csound,
                          "%s", Str("\n\n\nCannot load left data file, exiting\n\n"));

    fpr = csound->LoadMemoryFile(csound, filer, CSFTYPE_FLOATS_BINARY,
                                   swap4bytes);
    if (UNLIKELY(fpr == NULL))
      return
        csound->InitError(csound,
                          "%s", Str("\n\n\nCannot load right data file, exiting\n\n"));

    p->irlength = irlength;
    p->sroverN = sr / irlength;

    /* file handles */
    p->fpbeginl = (float *) fpl->beginp;
    p->fpbeginr = (float *) fpr->beginp;

    if(overlap != 2 && overlap != 4 && overlap != 8 && overlap != 16)
      overlap = 4;
    p->overlap = overlap;

    if(r <= 0 || r > 15)
      r = FL(8.8);
    p->radius = r;

    p->hopsize = (int32_t)(irlength / overlap);

    /* buffers */
    if (!p->inbuf.auxp || p->inbuf.size < (overlap * irlength) * sizeof(MYFLT))
      csound->AuxAlloc(csound, (overlap * irlength) * sizeof(MYFLT), &p->inbuf);
    /* 2d arrays in 1d! */
    if (!p->outbufl.auxp || p->outbufl.size < (overlap * irlength) * sizeof(MYFLT))
      csound->AuxAlloc(csound, (overlap * irlength) * sizeof(MYFLT), &p->outbufl);
    if (!p->outbufr.auxp || p->outbufr.size < (overlap * irlength) * sizeof(MYFLT))
      csound->AuxAlloc(csound, (overlap * irlength) * sizeof(MYFLT), &p->outbufr);
    if (!p->complexinsig.auxp || p->complexinsig.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p-> complexinsig);
    if (!p->hrtflfloat.auxp || p->hrtflfloat.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->hrtflfloat);
    if (!p->hrtfrfloat.auxp || p->hrtfrfloat.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->hrtfrfloat);
    if (!p->outspecl.auxp || p->outspecl.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->outspecl);
    if (!p->outspecr.auxp || p->outspecr.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->outspecr);

    memset(p->inbuf.auxp, 0, (overlap*irlength) * sizeof(MYFLT));
    memset(p->outbufl.auxp, 0, (overlap*irlength) * sizeof(MYFLT));
    memset(p->outbufr.auxp, 0, (overlap*irlength) * sizeof(MYFLT));
    memset(p->complexinsig.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->hrtflfloat.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->hrtfrfloat.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->outspecl.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->outspecr.auxp, 0, irlength * sizeof(MYFLT));

    /* interpolation values */
    if (!p->lowl1.auxp || p->lowl1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowl1);
    if (!p->lowr1.auxp || p->lowr1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowr1);
    if (!p->lowl2.auxp || p->lowl2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowl2);
    if (!p->lowr2.auxp || p->lowr2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->lowr2);
    if (!p->highl1.auxp || p->highl1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highl1);
    if (!p->highr1.auxp || p->highr1.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highr1);
    if (!p->highl2.auxp || p->highl2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highl2);
    if (!p->highr2.auxp || p->highr2.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->highr2);

    memset(p->lowl1.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->lowr1.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->lowl2.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->lowr2.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->highl1.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->highl2.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->highr1.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->highr2.auxp, 0, irlength * sizeof(MYFLT));

    if (!p->win.auxp || p->win.size < irlength * sizeof(MYFLT))
      csound->AuxAlloc(csound, irlength * sizeof(MYFLT), &p->win);
    if (!p->overlapskipin.auxp || p->overlapskipin.size < overlap * sizeof(int32_t))
      csound->AuxAlloc(csound, overlap * sizeof(int32_t), &p->overlapskipin);
    if (!p->overlapskipout.auxp ||
        p->overlapskipout.size < overlap * sizeof(int32_t))
      csound->AuxAlloc(csound, overlap * sizeof(int32_t), &p->overlapskipout);

    memset(p->win.auxp, 0, irlength * sizeof(MYFLT));
    memset(p->overlapskipin.auxp, 0, overlap * sizeof(int32_t));
    memset(p->overlapskipout.auxp, 0, overlap * sizeof(int32_t));

    win = (MYFLT *)p->win.auxp;
    overlapskipin = (int32_t *)p->overlapskipin.auxp;
    overlapskipout = (int32_t *)p->overlapskipout.auxp;

    /* window is Hanning */
    for(i = 0; i < irlength; i++)
      win[i] = FL(0.5) - (FL(0.5) * COS(i * TWOPI_F / (MYFLT)(irlength - 1)));

    for(i = 0; i < overlap; i++)
      {
        /* so, for example in overlap 4: will be 0, 32, 64, 96 if ir = 128 */
        overlapskipin[i] = p->hopsize * i;
        overlapskipout[i] = p->hopsize * i;
      }

    /* initialise */
    p->counter = 0;
    p->t = 0;

    /* setup values used to check if src has moved, illegal values to
       start with to ensure first read */
    p->anglev = -1;
    p->elevv = -41;

    p->setup = csound->RealFFTSetup(csound, p->irlength, FFT_FWD);
    p->isetup = csound->RealFFTSetup(csound, p->irlength, FFT_INV);

    return OK;
}


static int32_t hrtfmove2_process(CSOUND *csound, hrtfmove2 *p)
{
    /* local pointers to p */
    MYFLT *in = p->in;
    MYFLT *outsigl  = p->outsigl;
    MYFLT *outsigr = p->outsigr;

    /* common buffers and variables */
    MYFLT *inbuf = (MYFLT *)p->inbuf.auxp;

    MYFLT *outbufl = (MYFLT *)p->outbufl.auxp;
    MYFLT *outbufr = (MYFLT *)p->outbufr.auxp;

    MYFLT outsuml = FL(0.0), outsumr = FL(0.0);

    MYFLT *complexinsig = (MYFLT *)p->complexinsig.auxp;
    MYFLT *hrtflfloat = (MYFLT *)p->hrtflfloat.auxp;
    MYFLT *hrtfrfloat = (MYFLT *)p->hrtfrfloat.auxp;
    MYFLT *outspecl = (MYFLT *)p->outspecl.auxp;
    MYFLT *outspecr = (MYFLT *)p->outspecr.auxp;

    MYFLT elev = *p->kelev;
    MYFLT angle = *p->kangle;
    int32_t overlap = p->overlap;
    MYFLT r = p->radius;

    MYFLT sr = p->sr;
    MYFLT sroverN = p->sroverN;

    int32_t hopsize = p->hopsize;

    MYFLT *win = (MYFLT *)p->win.auxp;
    int32_t *overlapskipin = (int32_t *)p->overlapskipin.auxp;
    int32_t *overlapskipout = (int32_t *)p->overlapskipout.auxp;

    int32_t counter = p ->counter;
    int32_t t = p ->t;

        /* pointers into HRTF files */
    float *fpindexl;
    float *fpindexr;

    int32_t i, skip = 0;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t j, nsmps = CS_KSMPS;

    /* interpolation values */
    MYFLT *lowl1 = (MYFLT *)p->lowl1.auxp;
    MYFLT *lowr1 = (MYFLT *)p->lowr1.auxp;
    MYFLT *lowl2 = (MYFLT *)p->lowl2.auxp;
    MYFLT *lowr2 = (MYFLT *)p->lowr2.auxp;
    MYFLT *highl1 = (MYFLT *)p->highl1.auxp;
    MYFLT *highr1 = (MYFLT *)p->highr1.auxp;
    MYFLT *highl2 = (MYFLT *)p->highl2.auxp;
    MYFLT *highr2 = (MYFLT *)p->highr2.auxp;

    /* local interpolation values */
    MYFLT elevindexhighper, angleindex2per, angleindex4per;
    int32_t elevindexlow, elevindexhigh, angleindex1, angleindex2,
      angleindex3, angleindex4;
    MYFLT magl, magr, phasel, phaser, magllow, magrlow, maglhigh, magrhigh;

    /* woodworth values */
    MYFLT radianangle, radianelev, itd=0, itdww, freq;

    int32_t irlength = p->irlength;

    /* local variables, mainly used for simplification */
    MYFLT elevindexstore;
    MYFLT angleindexlowstore;
    MYFLT angleindexhighstore;


    /* start indices at correct value (start of file)/ zero indices. */
    fpindexl = (float *) p->fpbeginl;
    fpindexr = (float *) p->fpbeginr;

    if (UNLIKELY(offset)) {
      memset(outsigl, '\0', offset*sizeof(MYFLT));
      memset(outsigr, '\0', offset*sizeof(MYFLT));
    }
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&outsigl[nsmps], '\0', early*sizeof(MYFLT));
      memset(&outsigr[nsmps], '\0', early*sizeof(MYFLT));
    }
    /* ksmps loop */
    for(j = offset; j < nsmps; j++)
      {
        /* distribute the signal and apply the window */
        /* according to a time pointer (kept by overlapskip[n]) */
        for(i = 0; i < overlap; i++)
          {
            inbuf[(i * irlength) + overlapskipin[i]] = in[j] *
              win[overlapskipin[i]];
            overlapskipin[i]++;
          }

        counter++;

        if(counter == hopsize)
          {
            /* process a block */
            if(elev > FL(90.0))
              elev = FL(90.0);
            if(elev < FL(-40.0))
              elev = FL(-40.0);

            while(angle < FL(0.0))
              angle += FL(360.0);
            while(angle >= FL(360.0))
              angle -= FL(360.0);

            if(angle != p->anglev || elev != p->elevv)
              {
                /* two nearest elev indices to avoid recalculating */
                elevindexstore = (elev - minelev) / elevincrement;
                elevindexlow = (int32_t)elevindexstore;

                if(elevindexlow < 13)
                  elevindexhigh = elevindexlow + 1;
                /* highest index reached */
                else
                  elevindexhigh = elevindexlow;

                /* get percentage value for interpolation */
                elevindexhighper = elevindexstore - elevindexlow;

                /* avoid recalculation */
                angleindexlowstore = angle / (FL(360.0) /
                                              elevationarray[elevindexlow]);
                angleindexhighstore = angle / (FL(360.0) /
                                               elevationarray[elevindexhigh]);

                /* 4 closest indices, 2 low and 2 high */
                angleindex1 = (int32_t)angleindexlowstore;

                angleindex2 = angleindex1 + 1;
                angleindex2 = angleindex2 % elevationarray[elevindexlow];

                angleindex3 = (int32_t)angleindexhighstore;

                angleindex4 = angleindex3 + 1;
                angleindex4 = angleindex4 % elevationarray[elevindexhigh];

                /* angle percentages for interp */
                angleindex2per = angleindexlowstore - angleindex1;
                angleindex4per = angleindexhighstore - angleindex3;

                /* read 4 nearest HRTFs */
                skip = 0;
                /* switch l and r */
                if(angleindex1 > elevationarray[elevindexlow] / 2)
                  {
                    for(i = 0; i < elevindexlow; i++)
                      skip += ((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0;
                         i < (elevationarray[elevindexlow] - angleindex1);
                         i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        lowl1[i] = fpindexr[skip + i];
                        lowr1[i] = fpindexl[skip + i];
                      }
                  }
                else
                  {
                    for(i = 0; i < elevindexlow; i++)
                      skip += ((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0; i < angleindex1; i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        lowl1[i] = fpindexl[skip + i];
                        lowr1[i] = fpindexr[skip + i];
                      }
                  }

                skip = 0;
                if(angleindex2 > elevationarray[elevindexlow] / 2)
                  {
                    for(i = 0; i < elevindexlow; i++)
                      skip += ((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0;
                         i < (elevationarray[elevindexlow] - angleindex2);
                         i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        lowl2[i] = fpindexr[skip + i];
                        lowr2[i] = fpindexl[skip + i];
                      }
                  }
                else
                  {
                    for(i = 0; i < elevindexlow; i++)
                      skip += ((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0; i < angleindex2; i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        lowl2[i] = fpindexl[skip + i];
                        lowr2[i] = fpindexr[skip + i];
                      }
                  }

                skip = 0;
                if(angleindex3 > elevationarray[elevindexhigh] / 2)
                  {
                    for(i = 0; i < elevindexhigh; i++)
                      skip += ((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0;
                         i < (elevationarray[elevindexhigh] - angleindex3);
                         i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        highl1[i] = fpindexr[skip + i];
                        highr1[i] = fpindexl[skip + i];
                      }
                  }
                else
                  {
                    for(i = 0; i < elevindexhigh; i++)
                      skip += ((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0; i < angleindex3; i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        highl1[i] = fpindexl[skip + i];
                        highr1[i] = fpindexr[skip + i];
                      }
                  }

                skip = 0;
                if(angleindex4 > elevationarray[elevindexhigh] / 2)
                  {
                    for(i = 0; i < elevindexhigh; i++)
                      skip += ((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0;
                         i < (elevationarray[elevindexhigh] - angleindex4);
                         i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        highl2[i] = fpindexr[skip + i];
                        highr2[i] = fpindexl[skip + i];
                      }
                  }
                else
                  {
                    for(i = 0; i < elevindexhigh; i++)
                      skip += ((int32_t)(elevationarray[i] / 2) + 1) * irlength;
                    for (i = 0; i < angleindex4; i++)
                      skip += irlength;
                    for(i = 0; i < irlength; i++)
                      {
                        highl2[i] = fpindexl[skip + i];
                        highr2[i] = fpindexr[skip + i];
                      }
                  }

                /* woodworth process */
                /* ITD formula, check which ear is relevant to calculate
                   angle from */
                if(angle > FL(180.0))
                  radianangle = (angle - FL(180.0)) * PI_F / FL(180.0);
                else
                  radianangle = angle * PI_F / FL(180.0);  /* degrees to radians */
                radianelev = elev * PI_F / FL(180.0);

                /* get in correct range for formula */
                if(radianangle > (PI_F / FL(2.0)))
                  radianangle = FL(PI) - radianangle;

                /* woodworth formula for itd */
                itdww = (radianangle + SIN(radianangle)) * r * COS(radianelev) / c;

                /* 0 Hz and Nyq... */
                /* need fabs() here, to get mag, as 0hz and nyq stored as a
                   real value, to allow for possible negative, implying phase
                   of pi (in phase truncation) */
                magllow = FABS(lowl1[0]) + (FABS(lowl2[0]) - FABS(lowl1[0])) *
                  angleindex2per;
                maglhigh = FABS(highl1[0]) + (FABS(highl2[0]) - FABS(highl1[0])) *
                  angleindex4per;
                hrtflfloat[0] = magllow + (maglhigh - magllow) * elevindexhighper;

                magllow = FABS(lowl1[1]) + (FABS(lowl2[1]) - FABS(lowl1[1])) *
                  angleindex2per;
                maglhigh = FABS(highl1[1]) + (FABS(highl2[1]) - FABS(highl1[1])) *
                  angleindex4per;
                hrtflfloat[1] = magllow + (maglhigh - magllow) * elevindexhighper;

                magrlow = FABS(lowr1[0]) + (FABS(lowr2[0]) - FABS(lowr1[0])) *
                  angleindex2per;
                magrhigh = FABS(highr1[0]) + (FABS(highr2[0]) - FABS(highr1[0])) *
                  angleindex4per;
                hrtfrfloat[0] = magrlow + (magrhigh - magrlow) * elevindexhighper;

                magrlow = FABS(lowr1[1]) + (FABS(lowr2[1]) - FABS(lowr1[1])) *
                  angleindex2per;
                magrhigh = FABS(highr1[1]) + (FABS(highr2[1]) - FABS(highr1[1])) *
                  angleindex4per;
                hrtfrfloat[1] = magrlow + (magrhigh - magrlow) * elevindexhighper;

                /* magnitude interpolation */
                for(i = 2; i < irlength; i += 2)
                  {
                    /* interpolate high and low mags */
                    magllow = lowl1[i] + (lowl2[i] - lowl1[i]) * angleindex2per;
                    maglhigh = highl1[i] + (highl2[i] - highl1[i]) * angleindex4per;

                    magrlow = lowr1[i] + (lowr2[i] - lowr1[i]) * angleindex2per;
                    magrhigh = highr1[i] + (highr2[i] - highr1[i]) * angleindex4per;

                    /* interpolate high and low results */
                    magl = magllow + (maglhigh - magllow) * elevindexhighper;
                    magr = magrlow + (magrhigh - magrlow) * elevindexhighper;

                    freq = (i / 2) * sroverN;

                    /* non linear itd...last value in array = 1.0,
                       so back to itdww */
                    if(p->sr == 96000)
                      {
                        if ((i / 2) < 6)
                          itd = itdww * nonlinitd96k[(i / 2) - 1];
                      }
                    else if(p->sr == 48000)
                      {
                        if ((i / 2) < 6)
                          itd = itdww * nonlinitd48k[(i / 2) - 1];
                      }
                    else if(p->sr == 44100)
                      {
                        if((i / 2) < 6)
                          itd = itdww * nonlinitd[(i / 2) - 1];
                      }

                    if(angle > FL(180.))
                      {
                        phasel = TWOPI_F * freq * (itd / 2);
                        phaser = TWOPI_F * freq * -(itd / 2);}
                    else
                      {
                        phasel = TWOPI_F * freq * -(itd / 2);
                        phaser = TWOPI_F * freq * (itd / 2);
                      }

                    /* polar to rectangular */
                    hrtflfloat[i] = magl * COS(phasel);
                    hrtflfloat[i+1] = magl * SIN(phasel);

                    hrtfrfloat[i] = magr * COS(phaser);
                    hrtfrfloat[i+1] = magr * SIN(phaser);
                  }

                p->elevv = elev;
                p->anglev = angle;
              }

            /* t used to read inbuf...*/
            t--;
            if(t < 0)
              t = overlap - 1;

            /* insert insig for complex real, im fft */
            for(i = 0; i < irlength; i++)
              complexinsig[i] = inbuf[(t * irlength) + i];

            /* zero the current input sigframe time pointer */
            overlapskipin[t] = 0;

            csound->RealFFT(csound, p->setup, complexinsig);

            csound->RealFFTMult(csound, outspecl, hrtflfloat,
                                complexinsig, irlength, FL(1.0));
            csound->RealFFTMult(csound, outspecr, hrtfrfloat,
                                complexinsig, irlength, FL(1.0));

            /* convolution is the inverse FFT of above result */
            csound->RealFFT(csound, p->isetup, outspecl);
            csound->RealFFT(csound,  p->isetup, outspecr);

            /* need scaling based on overlap (more overlaps -> louder) and sr... */
            for(i = 0; i < irlength; i++)
              {
                outbufl[(t * irlength) + i] = outspecl[i] /
                  (overlap * FL(0.5) * (sr / FL(44100.0)));
                outbufr[(t * irlength) + i] = outspecr[i] /
                  (overlap * FL(0.5) * (sr / FL(44100.0)));
              }

          }       /* end of !counter % hopsize */

        /* output = sum of all relevant outputs: eg if overlap = 4 and
           counter = 0, */
        /* outsigl[j] = outbufl[0] + outbufl[128 + 96] +
           outbufl[256 + 64] + outbufl[384 + 32]; */
        /*        * * * * [ ]         + */
        /*          * * * [*]         + */
        /*            * * [*] *       + */
        /*              * [*] * *     = */
        /* stft! */

        outsuml = outsumr = FL(0.0);

        for(i = 0; i < (int32_t
                        )overlap; i++)
          {
            outsuml += outbufl[(i * irlength) +
                               overlapskipout[i]] * win[overlapskipout[i]];
            outsumr += outbufr[(i * irlength) +
                               overlapskipout[i]] * win[overlapskipout[i]];
            overlapskipout[i]++;
          }

        if(counter == hopsize)
          {
            /* zero output incrementation... */
            /* last buffer will have gone from 96 to 127...then 2nd last
               will have gone from 64 to 127... */
            overlapskipout[t] = 0;
            counter = 0;
          }

        outsigl[j] = outsuml;
        outsigr[j] = outsumr;

      }   /* end of ksmps audio loop */

    /* update */
    p->t = t;
    p->counter = counter;

    return OK;
}

/* see csound manual (extending csound) for details of below */
static OENTRY hrtfopcodes_localops[] =
{
 { "hrtfmove", sizeof(hrtfmove),0,  "aa", "akkSSooo",
    (SUBR)hrtfmove_init, (SUBR)hrtfmove_process },
 { "hrtfstat", sizeof(hrtfstat),0,  "aa", "aiiSSoo",
    (SUBR)hrtfstat_init, (SUBR)hrtfstat_process },
 { "hrtfmove2",  sizeof(hrtfmove2),0,  "aa", "akkSSooo",
    (SUBR)hrtfmove2_init, (SUBR)hrtfmove2_process }
};

LINKAGE_BUILTIN(hrtfopcodes_localops)
