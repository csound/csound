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
static void error(const char*, const char*);
static void pluckSetFilters(WGPLUCK*, MYFLT, MYFLT);
static MYFLT *pluckShape(WGPLUCK*);    /* pluck shape function */

/* ***** plucked string class member function definitions ***** */
static /* pluck::excite -- excitation function for plucked string */
int pluckExcite(WGPLUCK* plk)
{
    MYFLT *shape;
    int i;
    int size = plk->wg.upperRail.size;

    /* set the delay element to pick at */
    plk->pickSamp=(len_t)(size * *plk->pickPos);
    if (plk->pickSamp<1)
      plk->pickSamp = 1;

    /* set the bridge filter coefficients for the correct magnitude response */
    pluckSetFilters(plk,*plk->Aw0,*plk->AwPI);/*attenuation in dB at w0 and PI*/

    /* add the pick shape to the waveguide rails */
    shape = pluckShape(plk);    /* Efficiency loss here */

    /* add shape to lower rail */
    for (i=0;i<size;i++) {
      plk->wg.lowerRail.data[i] = shape[i]; /* Why add? Starts at zero anyway */
      plk->wg.upperRail.data[size-i-1] = shape[i];
    }

    /* flip shape and add to upper rail */
/*      pluckFlip(plk,shape); */
/*      for (i=0;i<size;i++) */
/*        plk->wg.upperRail.data[i] = shape[i]; */

    /* free the space used by the pluck shape */
    mfree((char*)shape);

    /* Reset the tuning and bridge filters */
    /*filterReset(&plk->wg.tnFIR);*/
    /*filterReset(&plk->bridge);*/

    /* set excitation flag */
    plk->wg.excited = 1;
    return OK;
}

/* ::pluck -- create the plucked-string instrument */
int pluckPluck(WGPLUCK* plk)
{
    /* ndelay = total required delay - 1.0 */
    len_t ndelay = (len_t) (esr_ / *plk->freq - FL(1.0));

#ifdef WG_VERBOSE
    printf("pluckPluck -- allocating memory...");
#endif

    /* Allocate auxillary memory or reallocate if size has changed */
    auxalloc((len_t)(ndelay/2)*sizeof(MYFLT), &plk->upperData);
    auxalloc((len_t)(ndelay/2)*sizeof(MYFLT), &plk->lowerData);
/*     auxalloc(3L*sizeof(MYFLT), &plk->bridgeCoeffs); */
/*     auxalloc(3L*sizeof(MYFLT), &plk->bridgeData); */

#ifdef WG_VERBOSE
    printf("done.\n");
#endif

    /* construct waveguide object */
#ifdef WG_VERBOSE
    printf("Constructing waveguide...");
#endif

    waveguideWaveguide((waveguide*)&plk->wg,             /* waveguide       */
                       (MYFLT)*plk->freq,                /* f0 frequency    */
                       (MYFLT*)plk->upperData.auxp,      /* upper rail data */
                       (MYFLT*)plk->lowerData.auxp);     /* lower rail data */
#ifdef WG_VERBOSE
    printf("done.\n");
#endif
    /* Allocate memory to bridge data and coeffs */
#ifdef WG_VERBOSE
    printf("Initializing bridge filters...");
#endif
/*     plk->bridge.coeffs=(MYFLT*)plk->bridgeCoeffs.auxp;  /\* bridge coeffs *\/ */
/*     plk->bridge.buffer.data=(MYFLT*)plk->bridgeData.auxp;/\* bridge data *\/ */
/*     filterFilter(&plk->bridge,3); /\* construct bridge filter object *\/ */
#ifdef WG_VERBOSE
    printf("done\n");
#endif
    /* Excite the string with the input parameters */
#ifdef WG_VERBOSE
    printf("Exciting the string...");
#endif
    pluckExcite(plk);
#ifdef WG_VERBOSE
    printf("done\n");
#endif
    return OK;
}


/* pluck::setFilters -- frequency dependent filter calculations */
static void pluckSetFilters(WGPLUCK* plk, MYFLT A_w0, MYFLT A_PI)
{
    /* Define the required magnitude response of H1 at w0 and PI */

    /* Constrain attenuation specification to dB per second */
    double NRecip = plk->wg.f0*onedsr; /*  N=t*esr/f0  */
    MYFLT H1_w0 = (MYFLT) pow(10.0,-(double)A_w0*0.05*NRecip);
    MYFLT H1_PI = (MYFLT) pow(10.0,-(double)A_PI*0.05*NRecip);
    {
      /* The tuning filter is allpass, so no dependency for H1 */
      /* therefore solve for the coefficients of the bridge filter directly */
      MYFLT cosw0 = (MYFLT)cos((double)plk->wg.w0);
      MYFLT a1=(H1_w0+cosw0*H1_PI)/(1+cosw0);
      MYFLT a0 = (a1 - H1_PI)*FL(0.5);
      /* apply constraints on coefficients (see Sullivan)*/
      if ((a0<FL(0.0))|| (a1<a0+a0)) {
        a0=FL(0.0);
        a1=H1_w0;
      }
      filter3Set(&plk->bridge,a0, a1);   /* set the new bridge coefficients */
    }
}

/* ::pluckShape -- the pluck function for a string */
static MYFLT *pluckShape(WGPLUCK* plk)
{
    MYFLT scale = *plk->amp;
    MYFLT  *shape;
    len_t len=plk->wg.lowerRail.size;
    len_t i;
    MYFLT M;

    /* This memory must be freed after use */
    shape = (MYFLT *) mmalloc(len*sizeof(MYFLT));
    if (!shape)
      error(Str(X_231,"Couldn't allocate for initial shape"),"<pluckShape>");

    scale = FL(0.5) * scale;      /* Scale was squared!! */
    for (i=0;i<plk->pickSamp;i++)
      shape[i] = scale*i / plk->pickSamp;

    M = (MYFLT)len - plk->pickSamp;
    for (i=0;i<M;i++)
      shape[plk->pickSamp+i] = scale - (i*scale/M);

    return shape;
}


/* ::getSamps -- the sample generating routine */
int pluckGetSamps(WGPLUCK* plk)
{
    MYFLT       yr0,yl0,yrM,ylM;        /* Key positions on the waveguide */
    MYFLT *ar = plk->out;    /* The sample output buffer */
    len_t M=plk->wg.upperRail.size; /* Length of the guide rail */
    len_t N=ksmps_;
/*    int i = 0; */
    MYFLT *fdbk = plk->afdbk;
    /* set the delay element to pickup at */
    len_t pickupSamp=(len_t)(M * *plk->pickupPos);
    if (pickupSamp<1) pickupSamp = 1;

    /* calculate N samples of the plucked string algorithm */
/*      if (plk->wg.excited) */
      do {
/*          void dumpRail(guideRail*, len_t); */
/*          dumpRail(&plk->wg.upperRail, M-1); */
/*          dumpRail(&plk->wg.lowerRail, M-1); */
        *ar++ = guideRailAccess(&plk->wg.upperRail,pickupSamp)
               +guideRailAccess(&plk->wg.lowerRail,M-pickupSamp);
        /*        sampBuf[i++] += *fdbk++; */
        yrM = guideRailAccess(&plk->wg.upperRail,M-1);/* wave into the nut */
        ylM = -yrM;                 /* reflect the incoming sample at the nut */

        yl0 = guideRailAccess(&plk->wg.lowerRail,0);  /* wave into bridge */
/*          printf("Removing %.2f (upper) and %.2f (lower)\n", yrM, yl0); */
        yr0 = -filter3FIR(&plk->bridge,yl0);   /* bridge reflection filter */
        yr0 = filterAllpass(&plk->wg,yr0);     /* allpass tuning filter */
        yr0 += *fdbk++;           /* Surely better to inject here */
        guideRailUpdate(&plk->wg.upperRail,yr0);    /* update the upper rail*/
        guideRailUpdate(&plk->wg.lowerRail,ylM);    /* update the lower rail*/
/*          printf("inserting %.2f (upper) and %.2f (lower)\n", yr0, ylM); */
      } while(--N);
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
void circularBufferCircularBuffer(circularBuffer* cb, len_t N)
{
    MYFLT *data = cb->data;
    if (!data)
      error(Str(X_194,"Buffer memory not allocated!"),
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
void guideRailGuideRail(guideRail* gr, len_t d)
{
    circularBufferCircularBuffer(gr,d); /* Guide rail is a circular buffer */
}


/* ::update -- waveguide rail insert and update routine */
void guideRailUpdate(guideRail *gr,MYFLT samp)
{
    *gr->pointer++ = samp;
    if (gr->pointer > gr->endPoint)
      gr->pointer = gr->data;
}

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

void dumpRail(guideRail* gr, len_t M)
{
    MYFLT *s = gr->pointer;
    while (M-- >= 0) {
      printf("%.2f ", *s);
      if (++s > gr->endPoint) s -= gr->size;
    }
    printf("\n\n");
}

#ifdef ORIGINAL
/* ***** class filter -- digital filter routines ****** */
/* ::filter -- constructor, assumes preallocated data and coeffs*/
void filterFilter(filter* filt, len_t n)
{
    if (!filt->coeffs)
      error(Str(X_223,"Coeffs not allocated!"),"<filter::filter>");

    /* Initialize circular buffer */
    circularBufferCircularBuffer(&filt->buffer,n);
}

/* ::set -- set the coefficients */
void filterSet(filter* filt, MYFLT *c)
{
    int i;

    if (!filt->buffer.inited)
      error(Str(X_277,"Filter not inited, cannot set"),"<filter::set>");

    for (i=0;i<filt->buffer.size;i++)
      filt->coeffs[i]=c[i];
#ifdef WG_VERBOSE
    for (i=0; i<filt->buffer.size; i++) printf("c[%d]=%f\n", i, c[i]);
    printf("Zeros at %f, %f\n",
           (-c[1]-sqrt(c[1]*c[1]-4.0*c[0]*c[0]))/(2.0*c[0]),
           (-c[1]+sqrt(c[1]*c[1]-4.0*c[0]*c[0]))/(2.0*c[0]));
#endif
}

/* ::FIR -- direct convolution filter routine */
 MYFLT filterFIR(filter* filt, MYFLT s)
{
    MYFLT *c = filt->coeffs;
    int i;

    /* y[n] = c1*x[n] + c2*x[n-1] + ... + cM*x[n-M+1] */
    circularBufferWrite(&filt->buffer,s);
    s = FL(0.0);
    for (i=0;i<filt->buffer.size;i++) {
      s += *c++ * circularBufferRead(&filt->buffer);
    }
    return s;
}
#endif

/* ***** class filter3 -- JPff ****** */

/* ::set -- set the coefficients */
void filter3Set(filter3* filt, MYFLT a0, MYFLT a1)
{
    filt->a0 = a0;
    filt->a1 = a1;
#ifdef WG_VERBOSE
    printf("c[0]=%f; c[1]=%f; c[2]=\n", a0, a1, a0);
    printf("Zeros at %f, %f\n",
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
void waveguideWaveguide(waveguide* wg,
                        MYFLT  freq,
                        MYFLT* upperData,
                        MYFLT* lowerData)
{
    MYFLT size, df;

    wg->excited = 0;
    wg->p       = FL(0.0); /* tuning filter state variable */
    wg->f0      = freq;
    wg->w0      = tpidsr*freq;

#ifdef WG_VERBOSE
    printf("f0=%f, w0=%f\n",wg->f0,wg->w0);
#endif

    /* Calculate the size of the delay lines and set them */
    /* Set pointers to appropriate positions in instrument memory */
    size = esr_ / freq - FL(1.0);

    /* construct the fractional part of the delay */
    df = (size - (len_t)size); /* fractional delay amount */
    if (df<EPSILON) {
      df   = FL(1.0)+EPSILON;
      size = size-FL(1.0);
    }
    wg->upperRail.data = upperData;
    wg->lowerRail.data = lowerData;
#ifdef WG_VERBOSE
    printf("size=%d+1, df=%f\n",(len_t)size,df);
#endif
    size = size*FL(0.5);
    guideRailGuideRail(&wg->upperRail,(len_t)size);
    guideRailGuideRail(&wg->lowerRail,(len_t)size);
    waveguideSetTuning(wg,df);
}

/* Set the allpass tuning filter coefficient */
void waveguideSetTuning(waveguide* wg, MYFLT df)
{
    MYFLT k=onedsr*wg->w0;

  /*c = (1.0-df)/(1.0+df);*/ /* Solve for coefficient from df */
    wg->c = -sinf((k-k*df)/FL(2.0))/sinf((k+k*df)/FL(2.0));

#ifdef WG_VERBOSE
    printf("tuning :c=%f\n",wg->c);
    fflush(stdout);
#endif
}

/* error -- report errors */
static void error(const char* a, const char* b)
{
    printf(Str(X_259,"Error:%s,%s\n"),a,b);
    longjmp(pcglob->exitjmp,1);
}

#define S       sizeof

static OENTRY localops[] = {
{ "wgpluck",S(WGPLUCK),5,"a","iikiiia",(SUBR)pluckPluck,NULL,(SUBR)pluckGetSamps},
};

LINKAGE
