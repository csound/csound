/*
    pluck.c:

    Copyright (C) 1994, 2000 Michael A. Casey, John ffitch

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


/* pluck.c -- plucked string class definitions */

/*
 * Code conversion from C++ to C (October 1994)
 * Author: Michael A. Casey MIT Media Labs
 * Language: C
 * Copyright (c) 1994 MIT Media Lab, All Rights Reserved
 * Soem modificatiosn John ffitch, 2000, simplifying code
 */

#include "csdl.h"
#include "wavegde.h"
#include "pluck.h"

/* external prototypes */
static void error(ENVIRON*, const char*, const char*);
static void pluckSetFilters(ENVIRON*, WGPLUCK*, MYFLT, MYFLT);
static MYFLT *pluckShape(ENVIRON*, WGPLUCK*);    /* pluck shape function */

/* ***** plucked string class member function definitions ***** */
static /* pluck::excite -- excitation function for plucked string */
int pluckExcite(ENVIRON *csound, WGPLUCK* p)
{
    MYFLT *shape;
    int i;
    int size = p->wg.upperRail.size;

    /* set the delay element to pick at */
    p->pickSamp=(len_t)(size * *p->pickPos);
    if (p->pickSamp<1)
      p->pickSamp = 1;

    /* set the bridge filter coefficients for the correct magnitude response */
    pluckSetFilters(csound, p,*p->Aw0,*p->AwPI);/*attenuation in dB at w0 and PI*/

    /* add the pick shape to the waveguide rails */
    shape = pluckShape(csound,p);    /* Efficiency loss here */

    /* add shape to lower rail */
    for (i=0;i<size;i++) {
      p->wg.lowerRail.data[i] = shape[i]; /* Why add? Starts at zero anyway */
      p->wg.upperRail.data[size-i-1] = shape[i];
    }

    /* flip shape and add to upper rail */
/*      pluckFlip(p,shape); */
/*      for (i=0;i<size;i++) */
/*        p->wg.upperRail.data[i] = shape[i]; */

    /* free the space used by the pluck shape */
    csound->Free(csound, (char*)shape);

    /* Reset the tuning and bridge filters */
    /*filterReset(&p->wg.tnFIR);*/
    /*filterReset(&p->bridge);*/

    /* set excitation flag */
    p->wg.excited = 1;
    return OK;
}

/* ::pluck -- create the plucked-string instrument */
int pluckPluck(ENVIRON *csound, WGPLUCK* p)
{
    /* ndelay = total required delay - 1.0 */
    len_t ndelay = (len_t) (csound->esr / *p->freq - FL(1.0));

#ifdef WG_VERBOSE
    csound->Message(csound, "pluckPluck -- allocating memory...");
#endif

    /* Allocate auxillary memory or reallocate if size has changed */
    csound->AuxAlloc(csound, (len_t)(ndelay/2)*sizeof(MYFLT), &p->upperData);
    csound->AuxAlloc(csound, (len_t)(ndelay/2)*sizeof(MYFLT), &p->lowerData);
/*     csound->AuxAlloc(csound, 3L*sizeof(MYFLT), &p->bridgeCoeffs); */
/*     csound->AuxAlloc(csound, 3L*sizeof(MYFLT), &p->bridgeData); */

#ifdef WG_VERBOSE
    csound->Message(csound, "done.\n");
#endif

    /* construct waveguide object */
#ifdef WG_VERBOSE
    csound->Message(csound, "Constructing waveguide...");
#endif

    waveguideWaveguide(csound,
                       (waveguide*)&p->wg,             /* waveguide       */
                       (MYFLT)*p->freq,                /* f0 frequency    */
                       (MYFLT*)p->upperData.auxp,      /* upper rail data */
                       (MYFLT*)p->lowerData.auxp);     /* lower rail data */
#ifdef WG_VERBOSE
    csound->Message(csound, "done.\n");
#endif
    /* Allocate memory to bridge data and coeffs */
#ifdef WG_VERBOSE
    csound->Message(csound, "Initializing bridge filters...");
#endif
/*     p->bridge.coeffs=(MYFLT*)p->bridgeCoeffs.auxp;  /\* bridge coeffs *\/ */
/*     p->bridge.buffer.data=(MYFLT*)p->bridgeData.auxp;/\* bridge data *\/ */
/*     filterFilter(&p->bridge,3); /\* construct bridge filter object *\/ */
#ifdef WG_VERBOSE
    csound->Message(csound, "done\n");
#endif
    /* Excite the string with the input parameters */
#ifdef WG_VERBOSE
    csound->Message(csound, "Exciting the string...");
#endif
    pluckExcite(csound,p);
#ifdef WG_VERBOSE
    csound->Message(csound, "done\n");
#endif
    return OK;
}


/* pluck::setFilters -- frequency dependent filter calculations */
static void pluckSetFilters(ENVIRON *csound, WGPLUCK* p, MYFLT A_w0, MYFLT A_PI)
{
    /* Define the required magnitude response of H1 at w0 and PI */

    /* Constrain attenuation specification to dB per second */
    double NRecip = p->wg.f0*onedsr; /*  N=t*csound->esr/f0  */
    MYFLT H1_w0 = (MYFLT) pow(10.0,-(double)A_w0*0.05*NRecip);
    MYFLT H1_PI = (MYFLT) pow(10.0,-(double)A_PI*0.05*NRecip);
    {
      /* The tuning filter is allpass, so no dependency for H1 */
      /* therefore solve for the coefficients of the bridge filter directly */
      MYFLT cosw0 = (MYFLT)cos((double)p->wg.w0);
      MYFLT a1=(H1_w0+cosw0*H1_PI)/(1+cosw0);
      MYFLT a0 = (a1 - H1_PI)*FL(0.5);
      /* apply constraints on coefficients (see Sullivan)*/
      if ((a0<FL(0.0))|| (a1<a0+a0)) {
        a0=FL(0.0);
        a1=H1_w0;
      }
      filter3Set(&p->bridge,a0, a1);   /* set the new bridge coefficients */
    }
}

/* ::pluckShape -- the pluck function for a string */
static MYFLT *pluckShape(ENVIRON *csound, WGPLUCK* p)
{
    MYFLT scale = *p->amp;
    MYFLT  *shape;
    len_t len=p->wg.lowerRail.size;
    len_t i;
    MYFLT M;

    /* This memory must be freed after use */
    shape = (MYFLT *) csound->Malloc(csound, len*sizeof(MYFLT));
    if (!shape)
      error(csound,
            Str("Couldn't allocate for initial shape"),"<pluckShape>");

    scale = FL(0.5) * scale;      /* Scale was squared!! */
    for (i=0;i<p->pickSamp;i++)
      shape[i] = scale*i / p->pickSamp;

    M = (MYFLT)len - p->pickSamp;
    for (i=0;i<M;i++)
      shape[p->pickSamp+i] = scale - (i*scale/M);

    return shape;
}

/* ::update -- waveguide rail insert and update routine */
inline void guideRailUpdate(guideRail *gr,MYFLT samp)
{
    *gr->pointer++ = samp;
    if (gr->pointer > gr->endPoint)
      gr->pointer = gr->data;
}

/* ::getSamps -- the sample generating routine */
int pluckGetSamps(ENVIRON *csound, WGPLUCK* p)
{
    MYFLT       yr0,yl0,yrM,ylM;        /* Key positions on the waveguide */
    MYFLT *ar = p->out;    /* The sample output buffer */
    len_t M=p->wg.upperRail.size; /* Length of the guide rail */
    len_t n,nsmps=csound->ksmps;
/*    int i = 0; */
    MYFLT *fdbk = p->afdbk;
    /* set the delay element to pickup at */
    len_t pickupSamp=(len_t)(M * *p->pickupPos);
    if (pickupSamp<1) pickupSamp = 1;

      for (n=0;n<nsmps;n++) {
        ar[n] = guideRailAccess(&p->wg.upperRail,pickupSamp)
               +guideRailAccess(&p->wg.lowerRail,M-pickupSamp);
        yrM = guideRailAccess(&p->wg.upperRail,M-1);/* wave into the nut */
        ylM = -yrM;                 /* reflect the incoming sample at the nut */

        yl0 = guideRailAccess(&p->wg.lowerRail,0);  /* wave into bridge */
        yr0 = -filter3FIR(&p->bridge,yl0);   /* bridge reflection filter */
        yr0 = filterAllpass(&p->wg,yr0);     /* allpass tuning filter */
        yr0 += *fdbk++;           /* Surely better to inject here */
        guideRailUpdate(&p->wg.upperRail,yr0);    /* update the upper rail*/
        guideRailUpdate(&p->wg.lowerRail,ylM);    /* update the lower rail*/
      }
      return OK;
}

/*
 * Code conversion from C++ to C (October 1994)
 * Author: Michael A. Casey MIT Media Labs
 * Language: C
 * Copyright (c) 1994 MIT Media Lab, All Rights Reserved
 */

#define EPSILON (FL(0.25))      /* threshold for small tuning values */
/* prototypes */


/***** circularBuffer class member function definitions *****/

/* ::circularBuffer -- constructor for circular buffer class
 * This routine assumes that the DATA pointer has already been
 * allocated by the calling routine.
 */
void circularBufferCircularBuffer(ENVIRON *csound, circularBuffer* cb, len_t N)
{
    MYFLT *data = cb->data;
    if (!data)
      error(csound, csound->LocalizeString("Buffer memory not allocated!"),
                    "<circularBuffer::circularBuffer>");

  /* Initialize pointers and variables */
    cb->size            = N;
    cb->inited          = 1;
    cb->pointer         = data;
    cb->endPoint        = data+cb->size-1;
    cb->insertionPoint  = data;
    cb->extractionPoint = data;
}



/* ::write -- insert new value in the buffer, update insertion pointer */
void circularBufferWrite(circularBuffer* cb, MYFLT val)
{
    /* update the extraction point */
    cb->extractionPoint = cb->insertionPoint;

    /* place data at the insertion point */
    *cb->insertionPoint-- = val;

    /* update the insertion point */
    if (cb->insertionPoint<cb->data)
      cb->insertionPoint = cb->endPoint;
}



/* ::read -- extract the value at the extraction point */
MYFLT circularBufferRead(circularBuffer* cb)
{
    MYFLT val;
    /* Read the value at the extraction point */
    val = *cb->extractionPoint++;

    /* Update the extraction point */
    if (cb->extractionPoint>cb->endPoint)
      cb->extractionPoint=cb->data;

    return val;
}


/* ***** class guideRail -- waveguide rail derived class ***** */
/* Guide rail is a circular buffer */
#define guideRailGuideRail(csound,gr,d) circularBufferCircularBuffer(csound, gr,d)


/* ::access -- waveguide rail access routine */
MYFLT guideRailAccess(guideRail* gr, len_t pos)
{
    MYFLT *s = gr->pointer - pos;
    while(s < gr->data)
      s += gr->size;
    while(s > gr->endPoint)
      s -= gr->size;
    return *s;
}

/* unused */
#if 0
void dumpRail(ENVIRON *csound, guideRail* gr, len_t M)
{
    MYFLT *s = gr->pointer;
    while (M-- >= 0) {
      csound->Message(csound, "%.2f ", *s);
      if (++s > gr->endPoint) s -= gr->size;
    }
    csound->Message(csound, "\n\n");
}
#endif

/* ***** class filter3 -- JPff ****** */

/* ::set -- set the coefficients */
void filter3Set(filter3* filt, MYFLT a0, MYFLT a1)
{
    filt->a0 = a0;
    filt->a1 = a1;
    filt->x1 = filt->x2 = FL(0.0);
#ifdef WG_VERBOSE
    csound->Message(csound, "c[0]=%f; c[1]=%f; c[2]=\n", a0, a1, a0);
    csound->Message(csound, "Zeros at %f, %f\n",
                            (-a1-sqrt(a1*a1-4.0*a0*a0))/(2.0*a0),
                            (-a1+sqrt(a1*a1-4.0*a0*a0))/(2.0*a0));
#endif
}


/* ::FIR -- direct convolution filter routine */
 MYFLT filter3FIR(filter3* filt, MYFLT s)
{
    /* y[n] = c1*x[n] + c2*x[n-1] + ... + cM*x[n-M+1] */
    MYFLT ans = filt->a0 * (s+filt->x2) + filt->a1 * filt->x1;
    filt->x2 = filt->x1;
    filt->x1 = s;
    return ans;
}

/* ::allpass -- accurate 1st-order allpass filter routine */
/*   c = allpass filter coefficient, input sample */
MYFLT filterAllpass(waveguide* wg,MYFLT s)
{
    /* p[n] = x[n] + gp[n-1], y[n] = p[n-1] - gp[n] */
    MYFLT q = s + wg->c*wg->p;
    s = - wg->c * q + wg->p;
    wg->p = q;
    return s;
}
/* q = 0.000000 wg->c = -0.047619 wg->p = 0.000000 s = 0.000000 s1 = nan */

/* ***** Waveguide base-class member definitions ***** */

/* ::waveguide -- constructor
 * sets delay lengths and filter responses for frequency
 * total delay length = (SR/f0)
 * also sets tuning filter for fractional delay for exact tuning
 */
void waveguideWaveguide(ENVIRON *csound,
                        waveguide* wg,
                        MYFLT  freq,
                        MYFLT* upperData,
                        MYFLT* lowerData)
{
    MYFLT size, df;

    wg->excited = 0;
    wg->p       = FL(0.0); /* tuning filter state variable */
    wg->f0      = freq;
    wg->w0      = csound->tpidsr_*freq;

#ifdef WG_VERBOSE
    csound->Message(csound, "f0=%f, w0=%f\n", wg->f0, wg->w0);
#endif

    /* Calculate the size of the delay lines and set them */
    /* Set pointers to appropriate positions in instrument memory */
    size = csound->esr / freq - FL(1.0);

    /* construct the fractional part of the delay */
    df = (size - (len_t)size); /* fractional delay amount */
    if (df<EPSILON) {
      df   = FL(1.0)+EPSILON;
      size = size-FL(1.0);
    }
    wg->upperRail.data = upperData;
    wg->lowerRail.data = lowerData;
#ifdef WG_VERBOSE
    csound->Message(csound, "size=%d+1, df=%f\n", (len_t) size, df);
#endif
    size = size*FL(0.5);
    guideRailGuideRail(csound, &wg->upperRail,(len_t)size);
    guideRailGuideRail(csound, &wg->lowerRail,(len_t)size);
    waveguideSetTuning(csound, wg,df);
}

/* Set the allpass tuning filter coefficient */
void waveguideSetTuning(ENVIRON *csound, waveguide* wg, MYFLT df)
{
    MYFLT k=csound->onedsr_*wg->w0;

  /*c = (1.0-df)/(1.0+df);*/ /* Solve for coefficient from df */
    wg->c = -sinf((k-k*df)/FL(2.0))/sinf((k+k*df)/FL(2.0));

#ifdef WG_VERBOSE
    csound->Message(csound, "tuning :c=%f\n", wg->c);
#endif
}

/* error -- report errors */
static void error(ENVIRON *csound, const char* a, const char* b)
{
    csound->Die(csound, Str("Error: %s, %s"), a, b);
}

#define S       sizeof

static OENTRY localops[] = {
{ "wgpluck",S(WGPLUCK),5,"a","iikiiia",(SUBR)pluckPluck,NULL,(SUBR)pluckGetSamps},
};

LINKAGE

