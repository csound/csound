/* Scanned Synthesis Opcodes:
   Copyright, 1999 Paris Smaragdis
   An extended system from an algorithm by Bill Verplank, Max Mathews and Rob Shaw

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

/* Code fixes by John ffitch, March 2000 */
/*               Made interpolation selectable April 2000 */
/* Minor code optimisations April 2021 by JPff */

#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif

#include "scansyn.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cwindow.h"
#include "arrays.h"

/* #undef CS_KSMPS */
/* #define CS_KSMPS     (csound->GetKsmps(csound)) */

/* Order of interpolation of scanning */
/* Either 1, 2 (linear), 3 (cubic) or 4 (quadratic) */
/* #define OSCIL_INTERP 4 */
/* Not used as now variable */

/* Order of interpolation of updating */
/* Either 2 (linear) or 3 (cubic) */
//#define PHASE_INTERP 3
//#define XALL

/****************************************************************************
 *      Helper functions and macros for updater
 ***************************************************************************/

/*
 *      Wavetable init
 */
static int32_t scsnu_initw(CSOUND *csound, PSCSNU *p)
{
    int32_t len = p->len*sizeof(MYFLT);
    FUNC *fi = csound->FTFind(csound,  p->i_init);
    if (UNLIKELY(fi == NULL)) {
      return csound->InitError(csound,
                               "%s", Str("scanu: Could not find ifnnit ftable"));
    }
    if (UNLIKELY((int32_t)fi->flen != p->len))
      return csound->InitError(csound, "%s", Str("scanu: Init table has bad size"));
    p->fi = fi;
    memcpy(p->x0, fi->ftable, len);
    memcpy(p->x1, fi->ftable, len);
    memcpy(p->x2, fi->ftable, len);
    return OK;
}

/*
 *      Hammer hit
 */

static int32_t scsnu_hammer(CSOUND *csound, PSCSNU *p, MYFLT pos, MYFLT wgt)
{
    int32_t i, i1, i2;
    FUNC *fi = p->fi;
    MYFLT *f;
    MYFLT tab = FABS(*p->i_init);
    MYFLT *x1 = p->x1;
#ifdef XALL
    MYFLT *x3 = p->x3, *x2 = p->x2;
#endif
    int32_t len = p->len;
    if (pos<FL(0.0)) pos = FL(0.0);
    if (pos>FL(1.0)) pos = FL(1.0);

    /* Get table */
    //if (UNLIKELY(tab<FL(0.0))) tab = -tab;   /* JPff fix here */
    if (fi == NULL)
    if (UNLIKELY((fi = csound->FTFind(csound, &tab)) == NULL)) {
      return csound->InitError(csound,
                               "%s", Str("scanu: Could not find ifninit ftable"));
    }
    p->fi = fi;
    /* Add hit */
    f = fi->ftable;
    i1 = (int32_t)(len*pos-fi->flen/2);
    i2 = (int32_t)(len*pos+fi->flen/2);
    for (i = i1 ; i < 0 ; i++) {
#ifdef XALL
      x2[len+i] += wgt * *f;
      x3[len+i] += wgt * *f;
#endif
      x1[len+i] += wgt * *f++;
    }
    for (; i < p->len && i < i2 ; i++) {
#ifdef XALL
      x2[i] += wgt * *f;
      x3[i] += wgt * *f;
#endif
      x1[i] += wgt * *f++;
    }
    for (; i < i2 ; i++) {
#ifdef XALL
      x2[i-len] += wgt * *f;
      x3[i-len] += wgt * *f;
#endif
      x1[i-len] += wgt * *f++;
    }
    return OK;
}

/******************************
 *      Linked list stuff
 ******************************/

struct scsn_elem {
    int32_t             id;
    PSCSNU              *p;
    struct scsn_elem    *next;
};

#if 0
/* remove from list */
static int32_t listrm(CSOUND *csound, PSCSNU *p)
{
    SCANSYN_GLOBALS  *pp = p->pp;
    struct scsn_elem *q = NULL;
    struct scsn_elem *i = (struct scsn_elem *) pp->scsn_list;

    csound->Message(csound, "remove from scsn_list\n");
    while (1) {
      if (UNLIKELY(i == NULL)) {

        csound->ErrorMsg(csound,
                         "%s", Str("Eek ... scan synthesis id was not found"));
        return NOTOK;
      }
      if (i->p == p)
        break;
      q = i;
      i = i->next;
    }
    if (q != NULL)
      q->next = i->next;
    else
      pp->scsn_list = (void*) i->next;
    csound->Free(csound, i);
    return OK;
}
#endif

/* add to list */
static void listadd(SCANSYN_GLOBALS *pp, PSCSNU *p)
{
    CSOUND  *csound = pp->csound;
    struct scsn_elem *i = (struct scsn_elem *) pp->scsn_list;

    for ( ; i != NULL; i = i->next) {
      if (i->id == p->id) {
        i->p = p;
        return;
      }
    }
    i = (struct scsn_elem *) csound->Calloc(csound, sizeof(struct scsn_elem));
    i->id = p->id;
    i->p = p;
    i->next = (struct scsn_elem *) pp->scsn_list;
    pp->scsn_list = (void*) i;
}

/* Return from list according to id */
static PSCSNU *listget(CSOUND *csound, int32_t id)
{
    SCANSYN_GLOBALS  *pp;
    struct scsn_elem  *i;

    pp = scansyn_getGlobals(csound);
    i = (struct scsn_elem *) pp->scsn_list;
    if (UNLIKELY(i == NULL)) {
      csound->ErrorMsg(csound, "%s", Str("scans: No scan synthesis net specified"));
      return NULL;
    }
    while (1) {
      if (i->id == id)
        break;
      i = i->next;
      if (UNLIKELY(i == NULL)){
        csound->ErrorMsg(csound, "%s",
                         Str("Eek ... scan synthesis id was not found"));
        return NULL;
      }
    }
    return i->p;
}

/****************************************************************************
 *      Functions for scsnu
 ***************************************************************************/

/* *      Setup the updater
 */
static int32_t scsnu_init(CSOUND *csound, PSCSNU *p)
{
    /* Get parameter table pointers and check lengths */
    SCANSYN_GLOBALS *pp;
    FUNC    *f;
    uint32_t len;
    //printf("**** p, i_f i_m = %p %g %g %g\n", p, p->i_f, p->i_m, p->i_c);
    
    /* Mass */
    if (UNLIKELY((f = csound->FTFind(csound, p->i_m)) == NULL)) {
      return csound->InitError(csound,
                               "%s", Str("scanu: Could not find ifnmass table"));
    }
    len = p->len = f->flen;
    p->m = f->ftable;
    // printf("**** p, i_f i_m = %p %g %g %g\n", p, p->i_f, p->i_m, p->i_c);

    /* Centering */
    if (UNLIKELY((f = csound->FTFind(csound, p->i_c)) == NULL)) {
      return csound->InitError(csound,
                               "%s", Str("scanu: Could not find ifncentr table"));
    }
    if (UNLIKELY(f->flen != len))
      return csound->InitError(csound, "%s",
                               Str("scanu: Parameter tables should all "
                                   "have the same length"));
    p->c = f->ftable;
    //printf("**** p, i_f i_m = %p %g %g %g\n", p, p->i_f, p->i_m, p->i_c);

    /* Damping */
    if (UNLIKELY((f = csound->FTFind(csound, p->i_d)) == NULL)) {
      return csound->InitError(csound,
                               "%s", Str("scanu: Could not find ifndamp table"));
    }
    if (UNLIKELY(f->flen != len))
      return csound->InitError(csound, "%s",
                               Str("scanu: Parameter tables should all "
                                   "have the same length"));
    p->d = f->ftable;
    // printf("**** p, i_f i_m = %p %g %g %g\n", p, p->i_f, p->i_m, p->i_c);

    /* Spring stiffness */
    {
      uint32_t i, j;

      // printf("**** p, i_f i_m = %p %g %g %g\n", p, p->i_f, p->i_m, p->i_c);
      /* Get the table */
      if (UNLIKELY((f = csound->FTFind(csound, p->i_f)) == NULL)) {
        return csound->InitError(csound,
                                 "%s %p", Str("scanu: Could not find ifnmatrix table"), p->i_f);
      }
      //printf("**** p->i_f i_m = %p %g %g %g\n",p, p->i_f, p->i_m, p->i_c);

     /* Check that the size is good */
      if (UNLIKELY(f->flen < len*len)) {
        // printf("len = %d len*len = %d flen = %d\n", len, len*len, f->flen);
        return csound->InitError(csound, "%s",
                                 Str("scanu: Spring matrix is too small"));
      }

      /* Setup an easier addressing scheme */
      csound->AuxAlloc(csound, len*len * sizeof(MYFLT), &p->aux_f);
      p->f = (MYFLT*)p->aux_f.auxp;
      for (i = 0 ; i != len ; i++) {
        for (j = 0 ; j != len ; j++)
          p->f[i*len+j] = f->ftable[i*len+j];
      }
    }

/* Make buffers to hold data */
#if PHASE_INTERP == 3
    csound->AuxAlloc(csound, 6*len*sizeof(MYFLT), &p->aux_x);
#else
    csound->AuxAlloc(csound, 5*len*sizeof(MYFLT), &p->aux_x);
#endif
    p->x0 = (MYFLT*)p->aux_x.auxp;
    p->x1 = p->x0 + len;
    p->x2 = p->x1 + len;
    p->ext = p->x2 + len;
    p->v = p->ext + len;
#if PHASE_INTERP == 3
    p->x3 = p->v + len;
#endif

    /* Initialize them ... */
    /* This relies on contiguous allocation of these vectors
       but as they are allocated via AuxAlloc they are zeroed anyway!  */
    //memset(p->x0, '\0', 4*len+sizeof(MYFlT));
#if PHASE_INTERP == 3
    //memset(p->x3, '\0', len+sizeof(MYFlT));
#endif
    /* Setup display window */
    if (*p->i_disp) {
      p->win = csound->Calloc(csound, sizeof(WINDAT));
      csound->SetDisplay(csound, (WINDAT*)p->win, p->x1, len,
                      Str("Mass displacement"), 0, Str("Scansynth window"));
    }

    p->fi = NULL;
    MYFLT temp;
    /* ... according to scheme */
    if (MODF(*p->i_init, &temp)) {
      // random fill
      int32_t i;
      MYFLT *x1 = p->x1;
      for (i=0; i<p->len; i++)
        x1[i] = temp*(MYFLT)(rand()-(RAND_MAX/2))/(RAND_MAX/2);
    }
    else if ((int32_t)*p->i_init < 0) {
      if (p->revised) {
        int32_t i;
        MYFLT *x1 = p->x1;
#ifdef XALL
        MYFLT *x3 = p->x3, *x2 = p->x2;
#endif
        int32_t len = p->len;
        int32_t l = (int32_t)(*p->i_l*p->len), r = (int32_t)(*p->i_r*p->len);
        if (l<r) {
          MYFLT slope = FL(1.0)/l;
          for (i = 0; i<=l; i++)
            x1[i] = i*slope;
          slope = (MYFLT)2.0/(l-r);
          for (i=l+1; i<=r; i++)
            x1[i] = (MYFLT)(l+r)/(r-l) + i*slope;
          slope = FL(1.0)/(len-r);
          for (i=r+1; i<len; i++)
            x1[i] = -(MYFLT)len/(len-r) +i*slope;
        }
        else if (r<l) {
        MYFLT slope = -FL(1.0)/r;
        for (i = 0; i<=r; i++)
          x1[i] = i*slope;
        slope = (MYFLT)2.0/(l-r);
        for (i=r+1; i<=l; i++)
          x1[i] = (MYFLT)(l+r)/(r-l) + i*slope;
        slope = -(MYFLT)FL(1.0)/(len-l);
        for (i=l+1; i<len; i++)
          x1[i] = (MYFLT)len/(len-l) +i*slope;
        }
        else { //Only one up pluck
          MYFLT slope = FL(1.0)/l;
          for (i = 0; i<=l; i++)
            x1[i] = i*slope;
          slope = -FL(1.0)/(len-l);
        for (i=l+1; i<len; i++)
          x1[i] = (MYFLT)len/(len-l) + slope*i;
        }
      }
      else {
        int32_t res = scsnu_hammer(csound, p, *p->i_l, FL(1.0));
        if (res != OK) return res;
        res = scsnu_hammer(csound, p, *p->i_r, -FL(1.0));
        if (res != OK) return res;
      }
    }
    else {
      int32_t res;
      if (*p->i_id<=FL(0.0)) scsnu_hammer(csound, p, FL(0.5), FL(1.0));
      else if ((res=scsnu_initw(csound, p))!=OK) return res;
    }
    if (*p->i_disp)
      csound->Display(csound, p->win); /* *********************** */

    /* Velocity gets presidential treatment */
    {
      uint32_t i;
      FUNC *f = csound->FTFind(csound, p->i_v);
      if (UNLIKELY(f == NULL)) {
        return csound->InitError(csound,
                                 "%s", Str("scanu: Could not find ifndisplace table"));
      }
      if (UNLIKELY(f->flen != len)) {
        return csound->InitError(csound, "%s",
                                 Str("scanu: Parameter tables should "
                                     "all have the same length"));
      }
      for (i = 0 ; i != len ; i++)
        p->v[i] = f->ftable[i];
    }

    /* Cache update rate over to local structure */
    if (*p->i_rate<=FL(0.0) || *p->i_rate>FL(1.0)) { //suspect value
      csound->Message(csound, "%s", Str("i_rate parameter out of range\n"));
      p->rate = 0;
    }
    else
      p->rate = (int32_t)(*p->i_rate * CS_ESR);

      /* Initialize index */
    p->idx = 0;

    /* External force index */
    p->exti = 0;

    //csound->Display(csound, p->win); /* ********************** */

    pp = scansyn_getGlobals(csound);
    p->pp = pp;

    /* Make external force window if we haven't so far */
    if (pp->ewin == NULL) {
      uint32_t i;
      MYFLT arg =  PI_F/(len-1);
      pp->ewin = (MYFLT*) csound->Calloc(csound, len * sizeof(MYFLT));
      for (i = 0 ; i != len-1 ; i++)
        pp->ewin[i] = SQRT(SIN(arg*i));
      pp->ewin[i] = FL(0.0); /* You get NaN otherwise */
    }

    /* Throw data into list or use table */
    p->id = (int32_t) *p->i_id;
    if (p->id < 0) {
      FUNC *ftp = csound->FTFind(csound, p->i_id);
      if (UNLIKELY(ftp == NULL)) {
        return csound->InitError(csound, "%s", Str("scanu: invalid id table"));
      }
      p->out = ftp->ftable;
    }
    else {
      listadd(pp, p);
    }
    return OK;
}

int32_t scsnu_init1(CSOUND *csound, PSCSNU *p)
{
    p->revised = 0;
    return scsnu_init(csound, p);
}
int32_t scsnu_init2(CSOUND *csound, PSCSNU *p)
{
    p->revised = 1;
    return scsnu_init(csound, p);
}


/*
 *      Performance function for updater
 */

#define dt  (FL(1.0))

static int32_t scsnu_play(CSOUND *csound, PSCSNU *p)
{
    SCANSYN_GLOBALS *pp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t     len = p->len;
    int32_t     idx = p->idx;
    int32_t     rate = p->rate;
    MYFLT       *out = p->out;
    int32_t     exti = p->exti;
    MYFLT       *x0 = p->x0;
    MYFLT       *x1 = p->x1;
    MYFLT       *x2 = p->x2;
#if PHASE_INTERP == 3
    MYFLT       *x3 = p->x3;
#endif
    MYFLT       *v = p->v;

    pp = p->pp;
    if (UNLIKELY(pp == NULL)) goto err1;

    //if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      //memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset ; n < nsmps ; n++) {

      /* Put audio input in external force */
      p->ext[exti] = p->a_ext[n];
      exti++;
      if (UNLIKELY(exti >= len))
        exti = 0;

      /* If it is time to calculate next phase, do it */
      if (UNLIKELY(idx >= rate)) {
        int32_t i, j;
        scsnu_hammer(csound, p, *p->k_x, *p->k_y);
        if (*p->i_disp)
          csound->Display(csound, p->win); /* *********************** */
        for (i = 0 ; i != len ; i++) {
          MYFLT a = FL(0.0);
                                /* Throw in audio drive */

          v[i] += p->ext[exti++] * pp->ewin[i];
          if (UNLIKELY(exti >= len))
            exti = 0;
                                /* And push feedback */
          //scsnu_hammer(csound, p, *p->k_x, *p->k_y);
          //if (*p->i_disp)
          //  csound->Display(csound, p->win); /* *********************** */
                                /* Estimate acceleration */
          if (p->revised) {
            MYFLT kf = *p->k_f;
            for (j = 0 ; j != len ; j++) {
              MYFLT weight = p->f[i*len+j];
              if (weight!=FL(0.0))
                a += (x1[j] - x1[i]) /(weight*kf);
            }
            a += - x1[i] * p->c[i] * *p->k_c -
               FABS(x2[i] - x1[i]) * p->d[i] * *p->k_d;
          }
          else {
            MYFLT kf = *p->k_f;
            for (j = 0 ; j != len ; j++) {
              MYFLT weight = p->f[i*len+j];
              if (weight!=FL(0.0))
                a += (x1[j] - x1[i]) * weight * kf;
            }
            a += - x1[i] * p->c[i] * *p->k_c -
              (x2[i] - x1[i]) * p->d[i] * *p->k_d;
          }
          a /= p->m[i] * *p->k_m;
          /* From which we get velocity */
          v[i] += dt * a;
          /* ... and future position */
          x0[i] += v[i] * dt;
        }
        /* Swap to get time order */
        {
          MYFLT* tmp= x2;
#if PHASE_INTERP == 3
          tmp = x3;
          p->x3 = x3 = x2;
#endif
          p->x2 = x2 = x1;
          p->x1 = x1 = x0;
          p->x0 = x0 = tmp;
          memcpy(x0, x1, len*sizeof(MYFLT));
        }
        /* Reset index and display the state */
        idx = 0;
        if (*p->i_disp)
          csound->Display(csound, p->win);
      }
      if (p->id<0) { /* Write to ftable */
        int32_t i;
        MYFLT t = (MYFLT)idx / rate;
        for (i = 0 ; i != p->len ; i++) {
#if PHASE_INTERP == 3
          out[i] = x1[i] + t*(-x3[i]*FL(0.5) +
                              t*(x3[i]*FL(0.5) - x1[i] +
                                 x2[i]*FL(0.5))
                              + x2[i]*FL(0.5));
#else
          out[i] = x2[i] + (x1[i] - x2[i]) * t;
#endif
        }
      }
      /* Update counter */
      idx++;
    }
    p->idx = idx;
    p->exti = exti;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("scanu: not initialised"));
}

/****************************************************************************
 *      Functions for scsns
 ***************************************************************************/

/*
 *      Succesive phase interpolator
 */
#if PHASE_INTERP == 3
#define pinterp(ii, x) \
        (pp->x1[p->t[(int32_t)ii]] + x*(-pp->x3[p->t[(int32_t)ii]]*FL(0.5) + \
         x*(pp->x3[p->t[(int32_t)ii]]*FL(0.5) - pp->x1[p->t[(int32_t)ii]] + \
         pp->x2[p->t[(int32_t)ii]]*FL(0.5)) + pp->x2[p->t[(int32_t)ii]]*FL(0.5)))
#else
#define pinterp(ii, x) \
        (pp->x2[p->t[(int32_t)ii]] + (pp->x1[p->t[(int32_t)ii]] - \
         pp->x2[p->t[(int32_t)ii]]) * x)
#endif

/*
 *      Init scaner
 */
static int32_t scsns_init(CSOUND *csound, PSCSNS *p)
{
    int32_t     i;
    int32_t     oscil_interp = (int32_t) *p->interp;
    FUNC    *t;

    /* Get corresponding update */
    p->p = listget(csound, (int32_t) *p->i_id);

    /* Get trajectory matrix */
    t = csound->FTFind(csound, p->i_trj);
    if (UNLIKELY(t == NULL)) {
      return csound->InitError(csound, "%s", Str("scans: Could not find "
                                          "the ifntraj table"));
    }
    if (oscil_interp<1 || oscil_interp>4) oscil_interp = 4;
    p->oscil_interp = oscil_interp;
    p->tlen = t->flen;
    /* Check that trajectory is within bounds */
    for (i = 0 ; i != p->tlen ; i++)
      if (UNLIKELY(t->ftable[i] < 0 || t->ftable[i] >= p->p->len))
        return csound->InitError(csound, "%s",
                                 Str("vermp: Trajectory table includes "
                                     "values out of range"));
    /* Allocate memory and pad to accomodate interpolation */
                              /* Note that the 4 here is a hack -- jpff */
    csound->AuxAlloc(csound, (p->tlen + 4)*sizeof(int32_t), &p->aux_t);
    p->t = (int32_t*)p->aux_t.auxp + (int32_t)(oscil_interp-1)/2;
    /* Fill 'er up */
    for (i = 0 ; i != p->tlen ; i++)
      p->t[i] = (int32)t->ftable[i];
    /* Do wraparounds */
    for (i = 1 ; i <= (oscil_interp-1)/2 ; i++)
      p->t[-i] = p->t[i];
    for (i = 0 ; i <= oscil_interp/2 ; i++)
      p->t[p->tlen+i] = p->t[i];
    /* Reset oscillator phase */
    p->phs = FL(0.0);
    /* Oscillator ratio */
    p->fix = (MYFLT)p->tlen*(1.0/CS_ESR);
    return OK;
}

/*
 *      Performance function for scanner
 */
static int32_t scsns_play(CSOUND *csound, PSCSNS *p)
{
    IGN(csound);
    MYFLT phs = p->phs, inc = *p->k_freq * p->fix;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    MYFLT t = (MYFLT)p->p->idx/p->p->rate;
    MYFLT *out = p->a_out;
    PSCSNU *pp = p->p;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    switch (p->oscil_interp) {
    case 1:
      for (i = offset ; i < nsmps ; i++) {
      /* Do various interpolations to get output sample ... */
/*      MYFLT x = phs - (int32_t)phs; */
        out[i] = *p->k_amp * (pinterp(phs, t));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        while (UNLIKELY(phs >= p->tlen))
          phs -= p->tlen;    /* Remember phase */
        while (UNLIKELY(phs < 0))
          phs += p->tlen;    /* Remember phase */
      }
      break;
    case 2:
      for (i = offset ; i < nsmps ; i++) {
      /* Do various interpolations to get output sample ... */
        MYFLT x = phs - (int32_t)phs;
        MYFLT y1 = pinterp(phs  , t);
        MYFLT y2 = pinterp(phs+1, t);

        out[i] = *p->k_amp * (y1 + x*(y2 - y1));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        while (UNLIKELY(phs >= p->tlen))
          phs -= p->tlen;    /* Remember phase */
        while (UNLIKELY(phs < 0))
          phs += p->tlen;    /* Remember phase */
      }
      break;
    case 3:
      for (i = offset ; i < nsmps ; i++) {
      /* Do various interpolations to get output sample ... */
        /* VL -- what happens if phs is 0? */
        MYFLT x = phs - (int32_t)phs;
        MYFLT y1 = pinterp(phs-1, t);
        MYFLT y2 = pinterp(phs  , t);
        MYFLT y3 = pinterp(phs+1, t);

        out[i] = *p->k_amp *
          (y2 + x*(-y1*FL(0.5) + x*(y1*FL(0.5) - y2 + y3*FL(0.5)) + y3*FL(0.5)));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        while (UNLIKELY(phs >= p->tlen))
          phs -= p->tlen;    /* Remember phase */
        while (UNLIKELY(phs < 0))
          phs += p->tlen;    /* Remember phase */
      }
      break;
    case 4:
      for (i = offset ; i < nsmps ; i++) {
      /* Do various interpolations to get output sample ... */
        /* VL -- what happens if phs is 0? */
        MYFLT x = phs - (int32_t)phs;
        MYFLT y1 = pinterp(phs-1, t);
        MYFLT y2 = pinterp(phs  , t);
        MYFLT y3 = pinterp(phs+1, t);
        MYFLT y4 = pinterp(phs+2, t);

        out[i] = *p->k_amp *
          (y2 + x*(-y1/FL(3.0) - y2*FL(0.5) + y3 +
                   x*(y1*FL(0.5) - y2 + y3*FL(0.5) +
                      x*(-y1/FL(6.0) + y2*FL(0.5) - y3*FL(0.5) +
                         y4/FL(6.0))) - y4/FL(6.0)));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        while (UNLIKELY(phs >= p->tlen))
          phs -= p->tlen;    /* Remember phase */
        while (UNLIKELY(phs < 0))
          phs += p->tlen;    /* Remember phase */
      }
      break;
    }
    p->phs = phs;
    return OK;
}

//======================================
static int32_t scsnmap_init(CSOUND *csound, PSCSNMAP *p)
{
    /* Get corresponding update */
    p->p = listget(csound, (int32_t)*p->i_id);
    if (p->p == NULL) return NOTOK;
    return OK;
}

static int32_t scsnmap(CSOUND *csound, PSCSNMAP *p)
{
    IGN(csound);
    PSCSNU *pp = p->p;
    int32 which = (int32)*p->k_which;
    if (which>=pp->len|| which<0)
      return csound->PerfError(csound, &(p->h),
                               Str("scan map %d out of range [0,%d]\n"),
                               which, pp->len);
    *p->k_pos = *p->k_pamp * pp->x0[which];
    *p->k_vel = *p->k_vamp * pp->v[which];
    return OK;
}

static int32_t scsnsmap(CSOUND *csound, PSCSNMAP *p)
{
    IGN(csound);
    PSCSNU *pp = p->p;
    int32 which = (int32)*p->k_which;
    if (which>=pp->len|| which<0)
      return csound->PerfError(csound, &(p->h),
                               Str("scan map %d out of range [0,%d]\n"),
                               which, pp->len);
 
    pp->x0[which] = *p->k_pos/(*p->k_pamp);
    pp->v[which]  = *p->k_vel/(*p->k_vamp);
    return OK;
}

//scsnmap to a vector

static int32_t scsnmapV_init(CSOUND *csound, PSCSNMAPV *p)
{
    /* Get corresponding update */
    p->p = listget(csound, (int32_t)*p->i_id);
    if (p->p == NULL) return NOTOK;
    tabinit(csound, p->k_pos,(p->p)->len, &(p->h));
    tabinit(csound, p->k_vel,(p->p)->len, &(p->h));
    return OK;
}

static int32_t scsnmapV(CSOUND *csound, PSCSNMAPV *p)
{
    IGN(csound);
    PSCSNU *pp = p->p;
    int32 len = pp->len;
    MYFLT pa = *p->k_pamp, va = *p->k_vamp;
    int32_t i;
    for (i=0; i<len; i++) {
      p->k_pos->data[i] = pp->x0[i]*pa;
      p->k_vel->data[i] = pp->v[i]*va;
    }
    //memcpy(p->k_pos->data, pp->x0, len);
    //memcpy(p->k_vel->data, pp->v, len);
    return OK;
}

//========================================================

#define S(x)    sizeof(x)

static OENTRY localops[] =
  {
    { "scanu", S(PSCSNU),TR,  "", "iiiiiiikkkkiikkaii",
     (SUBR)scsnu_init1, (SUBR)scsnu_play},
   { "scanu2", S(PSCSNU),TR,  "", "iiiiiiikkkkiikkaii",
     (SUBR)scsnu_init2, (SUBR)scsnu_play },
   { "scans", S(PSCSNS),TR,  "a","kkiio", (SUBR)scsns_init, (SUBR)scsns_play},
   { "scanmap", S(PSCSNMAP),TR,  "kk", "ikko",        (SUBR)scsnmap_init,
     (SUBR)scsnmap,NULL },
   { "scanmap.A", S(PSCSNMAPV),0,  "k[]k[]", "iPP",        (SUBR)scsnmapV_init,
     (SUBR)scsnmapV,NULL },
   { "scansmap", S(PSCSNMAP),TR, "",   "kkikko",      (SUBR)scsnmap_init,
     (SUBR)scsnsmap,NULL }

};

int32_t scansyn_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t) (sizeof(localops) / sizeof(OENTRY)));
}

#ifdef BUILD_PLUGINS
PUBLIC int32_t csoundModuleCreate(CSOUND *csound)
{
    (void) csound;
    return 0;
}

PUBLIC int32_t csoundModuleInit(CSOUND *csound)
{
    int32_t   err = 0;

    err |= scansyn_init_(csound);
    err |= scansynx_init_(csound);

    return (err ? CSOUND_ERROR : CSOUND_SUCCESS);
}

PUBLIC int32_t csoundModuleInfo(void)
{
    return ((CS_VERSION << 16) + (CS_SUBVER << 8) + (int32_t
                                                           ) sizeof(MYFLT));
}
#endif
