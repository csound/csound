/* Scanned Synthesis Opcodes:
   scansyn.c, scansyn.csd, scansyn.h and related files
   are Copyright, 1999 by Interval Research.
   Coded by Paris Smaragdis
   From an algorithm by Bill Verplank, Max Mathews and Rob Shaw

   Permission to use, copy, or modify these programs and their documentation
   for educational and research purposes only and without fee is hereby
   granted, provided that this copyright and permission notice appear on all
   copies and supporting documentation. For any other uses of this software,
   in original or modified form, including but not limited to distribution in
   whole or in part, specific prior permission from Interval Research must be
   obtained. Interval Research makes no representations about the suitability
   of this software for any purpose. It is provided "as is" without express or
   implied warranty.
*/

/* Code fixes by John ffitch, March 2000 */
/*               Made interpolation selectable April 2000 */

#include "csdl.h"
#include "scansyn.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "cwindow.h"

/* #undef CS_KSMPS */
/* #define CS_KSMPS     (csound->GetKsmps(csound)) */

/* Order of interpolation of scanning */
/* Either 1, 2 (linear), 3 (cubic) or 4 (quadratic) */
/* #define OSCIL_INTERP 4 */
/* Not used as now variable */

/* Order of interpolation of updating */
/* Either 2 (linear) or 3 (cubic) */
#define PHASE_INTERP 3

/****************************************************************************
 *      Helper functions and macros for updater
 ***************************************************************************/

/*
 *      Wavetable init
 */
static int scsnu_initw(CSOUND *csound, PSCSNU *p)
{
    FUNC *fi = csound->FTnp2Find(csound,  p->i_init);
    if (UNLIKELY(fi == NULL)) {
      return csound->InitError(csound,
                               Str("scanu: Could not find ifnnit ftable"));
    }
    if (UNLIKELY((int)fi->flen != p->len))
      return csound->InitError(csound, Str("scanu: Init table has bad size"));
    memcpy(p->x0, fi->ftable, p->len*sizeof(MYFLT));
    memcpy(p->x1, fi->ftable, p->len*sizeof(MYFLT));
    memcpy(p->x2, fi->ftable, p->len*sizeof(MYFLT));
    return OK;
}

/*
 *      Hammer hit
 */
static int scsnu_hammer(CSOUND *csound, PSCSNU *p, MYFLT pos, MYFLT sgn)
{
    int i, i1, i2;
    FUNC *fi;
    MYFLT *f;
    MYFLT tab = FABS(*p->i_init);

    /* Get table */
    //if (UNLIKELY(tab<FL(0.0))) tab = -tab;   /* JPff fix here */
    if (UNLIKELY((fi = csound->FTnp2Find(csound, &tab)) == NULL)) {
      return csound->InitError(csound,
                               Str("scanu: Could not find ifninit ftable"));
    }

    /* Add hit */
    f = fi->ftable;
    i1 = (int)(p->len*pos-fi->flen/2);
    i2 = (int)(p->len*pos+fi->flen/2);
    for (i = i1 ; i < 0 ; i++) {
#ifdef XALL
      p->x2[p->len-i-1] += sgn * *f;
      p->x3[p->len-i-1] += sgn * *f;
#endif
      p->x1[p->len-i-1] += sgn * *f++;
    }
    for (; i < p->len && i < i2 ; i++) {
#ifdef XALL
      p->x2[i] += sgn * *f;
      p->x3[i] += sgn * *f;
#endif
      p->x1[i] += sgn * *f++;
    }
    for (; i < i2 ; i++) {
#ifdef XALL
      p->x2[i-p->len] += sgn * *f;
      p->x3[i-p->len] += sgn * *f;
#endif
      p->x1[i-p->len] += sgn * *f++;
    }
    return OK;
}

/******************************
 *      Linked list stuff
 ******************************/

struct scsn_elem {
    int                 id;
    PSCSNU              *p;
    struct scsn_elem    *next;
};

#if 0
/* remove from list */
static int listrm(CSOUND *csound, PSCSNU *p)
{
    SCANSYN_GLOBALS  *pp = p->pp;
    struct scsn_elem *q = NULL;
    struct scsn_elem *i = (struct scsn_elem *) pp->scsn_list;

    csound->Message(csound, "remove from scsn_list\n");
    while (1) {
      if (UNLIKELY(i == NULL)) {

        csound->ErrorMsg(csound,
                         Str("Eek ... scan synthesis id was not found"));
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
#if 0
    csound->RegisterDeinitCallback(csound, p, (int (*)(CSOUND*, void*)) listrm);
#endif
}

/* Return from list according to id */
static PSCSNU *listget(CSOUND *csound, int id)
{
    SCANSYN_GLOBALS  *pp;
    struct scsn_elem  *i;

    pp = scansyn_getGlobals(csound);
    i = (struct scsn_elem *) pp->scsn_list;
    if (UNLIKELY(i == NULL)) {
      csound->ErrorMsg(csound, Str("scans: No scan synthesis net specified"));
      return NULL;
    }
    while (1) {
      if (i->id == id)
        break;
      i = i->next;
      if (UNLIKELY(i == NULL)){
        csound->ErrorMsg(csound, Str("Eek ... scan synthesis id was not found"));
        return NULL;
      }
    }
    return i->p;
}

/****************************************************************************
 *      Functions for scsnu
 ***************************************************************************/

/*
 *      Setup the updater
 */
static int scsnu_init(CSOUND *csound, PSCSNU *p)
{
    /* Get parameter table pointers and check lengths */
    SCANSYN_GLOBALS *pp;
    FUNC    *f;
    unsigned int     len;

    /* Mass */
    if (UNLIKELY((f = csound->FTnp2Find(csound, p->i_m)) == NULL)) {
      return csound->InitError(csound,
                               Str("scanu: Could not find ifnmass table"));
    }
    len = p->len = f->flen;
    p->m = f->ftable;

    /* Centering */
    if (UNLIKELY((f = csound->FTnp2Find(csound, p->i_c)) == NULL)) {
      return csound->InitError(csound,
                               Str("scanu: Could not find ifncentr table"));
    }
    if (UNLIKELY(f->flen != len))
      return csound->InitError(csound, Str("scanu: Parameter tables should all "
                                           "have the same length"));
    p->c = f->ftable;

    /* Damping */
    if (UNLIKELY((f = csound->FTnp2Find(csound, p->i_d)) == NULL)) {
      return csound->InitError(csound,
                               Str("scanu: Could not find ifndamp table"));
    }
    if (UNLIKELY(f->flen != len))
      return csound->InitError(csound, Str("scanu: Parameter tables should all "
                                           "have the same length"));
    p->d = f->ftable;

    /* Spring stiffness */
    {
      unsigned int i, j;

      /* Get the table */
      if (UNLIKELY((f = csound->FTnp2Find(csound, p->i_f)) == NULL)) {
        return csound->InitError(csound,
                                 Str("scanu: Could not find ifnstiff table"));
      }

     /* Check that the size is good */
      if (UNLIKELY(f->flen < len*len)) {
        //printf("len = %d len*len = %d flen = %d\n", len, len*len, f->flen);
        return csound->InitError(csound, Str("scanu: Spring matrix is too small"));
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
    {
      unsigned int i;
      for (i = 0 ; i != len ; i++) {
        p->x0[i] = p->x1[i] = p->x2[i]= p->ext[i] = FL(0.0);
#if PHASE_INTERP == 3
        p->x3[i] = FL(0.0);
#endif
      }
    }

    /* ... according to scheme */
    if ((int)*p->i_init < 0) {
      int res;
      res = scsnu_hammer(csound, p, *p->i_l, FL(1.0));
      if (res != OK) return res;
      res = scsnu_hammer(csound, p, *p->i_r, -FL(1.0));
      if (res != OK) return res;
    }
    else {
      int res;
      if (*p->i_id<FL(0.0)) scsnu_hammer(csound, p, FL(0.5), FL(1.0));
      else if ((res=scsnu_initw(csound, p))!=OK) return res;
    }
    /* Velocity gets presidential treatment */
    {
      unsigned int i;
      FUNC *f = csound->FTnp2Find(csound, p->i_v);
      if (UNLIKELY(f == NULL)) {
        return csound->InitError(csound,
                                 Str("scanu: Could not find ifnvel table"));
      }
      if (UNLIKELY(f->flen != len)) {
        return csound->InitError(csound, Str("scanu: Parameter tables should "
                                             "all have the same length"));
      }
      for (i = 0 ; i != len ; i++)
        p->v[i] = f->ftable[i];
    }

    /* Cache update rate over to local structure */
    p->rate = *p->i_rate * csound->GetSr(csound);

      /* Initialize index */
    p->idx = 0;

    /* External force index */
    p->exti = 0;

    /* Setup display window */
    if (*p->i_disp) {
      p->win = csound->Calloc(csound, sizeof(WINDAT));
      csound->dispset(csound, (WINDAT*)p->win, p->x1, len,
                      Str("Mass displacement"), 0, Str("Scansynth window"));
    }

    pp = scansyn_getGlobals(csound);
    p->pp = pp;

    /* Make external force window if we haven't so far */
    if (pp->ewin == NULL) {
      unsigned int i;
      MYFLT arg =  PI_F/(len-1);
      pp->ewin = (MYFLT*) csound->Calloc(csound, len * sizeof(MYFLT));
      for (i = 0 ; i != len-1 ; i++)
        pp->ewin[i] = SQRT(SIN(arg*i));
      pp->ewin[i] = FL(0.0); /* You get NaN otherwise */
    }

    /* Throw data into list or use table */
    p->id = (int) *p->i_id;
    if (p->id < 0) {
      if (UNLIKELY(csound->GetTable(csound, &(p->out), -(p->id)) < (int)len)) {
        return csound->InitError(csound, Str("scanu: invalid id table"));
      }
    }
    else {
      listadd(pp, p);
    }
    return OK;
}

/*
 *      Performance function for updater
 */

#define dt  (FL(1.0))

static int scsnu_play(CSOUND *csound, PSCSNU *p)
{
    SCANSYN_GLOBALS *pp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int     len = p->len;

    pp = p->pp;
    if (UNLIKELY(pp == NULL)) goto err1;

    if (UNLIKELY(early)) nsmps -= early;
    for (n = offset ; n < nsmps ; n++) {

      /* Put audio input in external force */
      p->ext[p->exti] = p->a_ext[n];
      p->exti++;
      if (UNLIKELY(p->exti >= len))
        p->exti = 0;

      /* If it is time to calculate next phase, do it */
      if (p->idx >= p->rate) {
        int i, j;
        for (i = 0 ; i != len ; i++) {
          MYFLT a = FL(0.0);
                                /* Throw in audio drive */
          p->v[i] += p->ext[p->exti++] * pp->ewin[i];
          if (UNLIKELY(p->exti >= len))
            p->exti = 0;
                                /* And push feedback */
          scsnu_hammer(csound, p, *p->k_x, *p->k_y);
                                /* Estimate acceleration */
          for (j = 0 ; j != len ; j++)
            if (p->f[i*len+j])
              a += (p->x1[j] - p->x1[i]) * p->f[i*len+j] * *p->k_f;
          a += - p->x1[i] * p->c[i] * *p->k_c -
               (p->x2[i] - p->x1[i]) * p->d[i] * *p->k_d;
          a /= p->m[i] * *p->k_m;
                                /* From which we get velocity */
          p->v[i] += dt * a;
                                /* ... and future position */
          p->x0[i] += p->v[i] * dt;
        }
        /* Swap to get time order */
        for (i = 0 ; i != len ; i++) {
#if PHASE_INTERP == 3
          p->x3[i] = p->x2[i];
#endif
          p->x2[i] = p->x1[i];
          p->x1[i] = p->x0[i];
        }
        /* Reset index and display the state */
        p->idx = 0;
        if (*p->i_disp)
          csound->display(csound, p->win);
      }
      if (p->id<0) { /* Write to ftable */
        int i;
        MYFLT t = (MYFLT)p->idx / p->rate;
        for (i = 0 ; i != p->len ; i++) {
#if PHASE_INTERP == 3
          p->out[i] = p->x1[i] + t*(-p->x3[i]*FL(0.5) +
                                    t*(p->x3[i]*FL(0.5) - p->x1[i] +
                                       p->x2[i]*FL(0.5))
                                    + p->x2[i]*FL(0.5));
#else
          p->out[i] = p->x2[i] + (p->x1[i] - p->x2[i]) * t;
#endif
        }
      }
      /* Update counter */
      p->idx++;
    }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("scanu: not initialised"));
}

/****************************************************************************
 *      Functions for scsns
 ***************************************************************************/

/*
 *      Succesive phase interpolator
 */
#if PHASE_INTERP == 3
#define pinterp(ii, x) \
        (pp->x1[p->t[(int)ii]] + x*(-pp->x3[p->t[(int)ii]]*FL(0.5) + \
         x*(pp->x3[p->t[(int)ii]]*FL(0.5) - pp->x1[p->t[(int)ii]] + \
         pp->x2[p->t[(int)ii]]*FL(0.5)) + pp->x2[p->t[(int)ii]]*FL(0.5)))
#else
#define pinterp(ii, x) \
        (pp->x2[p->t[(int)ii]] + (pp->x1[p->t[(int)ii]] - \
         pp->x2[p->t[(int)ii]]) * x)
#endif

/*
 *      Init scaner
 */
static int scsns_init(CSOUND *csound, PSCSNS *p)
{
    int     i;
    int     oscil_interp = (int) *p->interp;
    FUNC    *t;

    /* Get corresponding update */
    p->p = listget(csound, (int) *p->i_id);

    /* Get trajectory matrix */
    t = csound->FTnp2Find(csound, p->i_trj);
    if (UNLIKELY(t == NULL)) {
      return csound->InitError(csound, Str("scans: Could not find "
                                           "the ifntraj table"));
    }
    if (oscil_interp<1 || oscil_interp>4) oscil_interp = 4;
    p->oscil_interp = oscil_interp;
    p->tlen = t->flen;
    /* Check that trajectory is within bounds */
    for (i = 0 ; i != p->tlen ; i++)
      if (UNLIKELY(t->ftable[i] < 0 || t->ftable[i] >= p->p->len))
        return csound->InitError(csound, Str("vermp: Trajectory table includes "
                                             "values out of range"));
    /* Allocate memory and pad to accomodate interpolation */
                              /* Note that the 4 here is a hack -- jpff */
    csound->AuxAlloc(csound, (p->tlen + 4)*sizeof(int32), &p->aux_t);
    p->t = (int32*)p->aux_t.auxp + (int)(oscil_interp-1)/2;
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
    p->fix = (MYFLT)p->tlen*(1.0/csound->GetSr(csound));
    return OK;
}

/*
 *      Performance function for scanner
 */
static int scsns_play(CSOUND *csound, PSCSNS *p)
{
    MYFLT phs = p->phs, inc = *p->k_freq * p->fix;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    MYFLT t = (MYFLT)p->p->idx/p->p->rate;
    MYFLT *out = p->a_out;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    switch (p->oscil_interp) {
    case 1:
      for (i = offset ; i < nsmps ; i++) {
      /* Do various interpolations to get output sample ... */
        PSCSNU *pp = p->p;
/*      MYFLT x = phs - (int)phs; */
        out[i] = *p->k_amp * (pinterp(phs, t));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        if (phs >= p->tlen)
          phs -= p->tlen;    /* Remember phase */
      }
      break;
    case 2:
      for (i = offset ; i < nsmps ; i++) {
      /* Do various interpolations to get output sample ... */
        PSCSNU *pp = p->p;
        MYFLT x = phs - (int)phs;
        MYFLT y1 = pinterp(phs  , t);
        MYFLT y2 = pinterp(phs+1, t);

        out[i] = *p->k_amp * (y1 + x*(y2 - y1));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        if (phs >= p->tlen)
          phs -= p->tlen;    /* Remember phase */
      }
      break;
    case 3:
      for (i = offset ; i < nsmps ; i++) {
      /* Do various interpolations to get output sample ... */
        PSCSNU *pp = p->p;
        MYFLT x = phs - (int)phs;
        MYFLT y1 = pinterp(phs-1, t);
        MYFLT y2 = pinterp(phs  , t);
        MYFLT y3 = pinterp(phs+1, t);

        out[i] = *p->k_amp *
          (y2 + x*(-y1*FL(0.5) + x*(y1*FL(0.5) - y2 + y3*FL(0.5)) + y3*FL(0.5)));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        if (phs >= p->tlen)
          phs -= p->tlen;    /* Remember phase */
      }
      break;
    case 4:
      for (i = offset ; i < nsmps ; i++) {
      /* Do various interpolations to get output sample ... */
        PSCSNU *pp = p->p;
        MYFLT x = phs - (int)phs;
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
        if (phs >= p->tlen)
          phs -= p->tlen;    /* Remember phase */
      }
      break;
    }
    p->phs = phs;
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "scanu", S(PSCSNU),TR, 5, "", "iiiiiiikkkkiikkaii",
  (SUBR)scsnu_init, NULL, (SUBR)scsnu_play },
{ "scans", S(PSCSNS),TR, 5, "a","kkiio", (SUBR)scsns_init, NULL, (SUBR)scsns_play}
};

static int scansyn_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

PUBLIC int csoundModuleCreate(CSOUND *csound)
{
    (void) csound;
    return 0;
}

PUBLIC int csoundModuleInit(CSOUND *csound)
{
    int   err = 0;

    err |= scansyn_init_(csound);
    err |= scansynx_init_(csound);

    return (err ? CSOUND_ERROR : CSOUND_SUCCESS);
}

PUBLIC int csoundModuleInfo(void)
{
    return ((CS_APIVERSION << 16) + (CS_APISUBVER << 8) + (int) sizeof(MYFLT));
}
