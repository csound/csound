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


/* Order of interpolation of scanning */
/* Either 1, 2 (linear), 3 (cubic) or 4 (quadratic) */
/* #define OSCIL_INTERP 4 */
/* Not used as now variable */

/* Order of interpolation of updating */
/* Either 2 (linear) or 3 (cubic) */
#define PHASE_INTERP 3

/* Window to be applied at external force */
static MYFLT *ewin = NULL;


/****************************************************************************
 *      Helper functions and macros for updater
 ***************************************************************************/

/*
 *      Wavetable init
 */
static int scsnu_initw(ENVIRON *csound, PSCSNU *p)
{
    int i;
    FUNC *fi = ftfind(csound,  p->i_init);
    if (fi == NULL) {
      return initerror(Str("scanu: Could not find ifnnit ftable"));
    }
    if (fi->flen != p->len)
      die(Str("scanu: Init table has bad size"));
    for (i = 0 ; i != p->len ; i++)
      p->x1[i] = fi->ftable[i];
    return OK;
}

/*
 *      Hammer hit
 */
static int scsnu_hammer(ENVIRON *csound, PSCSNU *p, MYFLT pos, MYFLT sgn)
{
    int i, i1, i2;
    FUNC *fi;
    MYFLT *f;
    MYFLT tab = *p->i_init;

    /* Get table */
    if (tab<FL(0.0)) tab = -tab;   /* JPff fix here */
    if ((fi = ftfind(csound, &tab)) == NULL) {
      return initerror(Str("scanu: Could not find ifninit ftable"));
    }

    /* Add hit */
    f = fi->ftable;
    i1 = (int)(p->len*pos-fi->flen/2);
    i2 = (int)(p->len*pos+fi->flen/2);
    for (i = i1 ; i < 0 ; i++) {
      p->x1[p->len-i-1] += sgn * *f++;
#ifdef XALL
      p->x2[p->len-i-1] += sgn * *f++;
      p->x3[p->len-i-1] += sgn * *f++;
#endif
    }
    for (; i < p->len && i < i2 ; i++) {
      p->x1[i] += sgn * *f++;
#ifdef XALL
      p->x2[i] += sgn * *f++;
      p->x3[i] += sgn * *f++;
#endif
    }
    for (; i < i2 ; i++) {
      p->x1[i-p->len] += sgn * *f++;
#ifdef XALL
      p->x2[i-p->len] += sgn * *f++;
      p->x3[i-p->len] += sgn * *f++;
#endif
    }
    return OK;
}



/******************************
 *      Linked list stuff
 ******************************/

struct scsn_elem {
  int                   id;
    PSCSNU              *p;
    struct scsn_elem    *next;
};

struct scsn_elem scsn_list = {0, NULL, NULL};

/* add to list */
void listadd(PSCSNU *p)
{
/*     int cnt = 0; */
    struct scsn_elem *i = &scsn_list;
    while(i->next != NULL) {
      if (i->id==p->id) {
/*         printf("Reusing id %d\n", i->id); */
        i->p = p;               /* Reuse the space */
        return;
      }
      i = i->next;
/*       cnt++; */
    }
    i->next = (struct scsn_elem *)calloc(1, sizeof(struct scsn_elem));
    i->p = p;
    i->id = p->id;
    i->next->next = NULL;       /* ??? was i->next = NULL which is clearly wrong! */
/*     printf("scsn_list of length %d\n", cnt+1); */
}

#ifdef never
/* This code is not used as csoudn has no dealloc method */
/* remove from list */
void listrm(PSCSNU *p)
{
    struct scsn_elem *q;
    struct scsn_elem *i = &scsn_list;
    printf("remove from scsn_listd\n");
    while (i->next->p != p) {
      i = i->next;
      if (i == NULL)
        die(Str("Eek ... scan synthesis id was not found"));
    }
    q = i->next->next;
    free(i->next);
    i->next = q;
}
#endif

/* Return from list according to id */
PSCSNU *listget(ENVIRON *csound, int id)
{
    struct scsn_elem *i = &scsn_list;
    if (i->p == NULL) {
        csound->initerror_(csound->LocalizeString("scans: No scan synthesis net specified"));
        longjmp(csound->exitjmp_,1);
    }
    while (i->id != id) {
      i = i->next;
      if (i == NULL)
        csound->die_(csound->LocalizeString("Eek ... scan synthesis id was not found"));
    }
    return i->p;
}



/****************************************************************************
 *      Functions for scsnu
 ***************************************************************************/

/*
 *      Setup the updater
 */
int scsnu_init(ENVIRON *csound, PSCSNU *p)
{
    /* Get parameter table pointers and check lengths */
    FUNC *f;
    int len;

    /* Mass */
    if ((f = ftfind(csound, p->i_m)) == NULL) {
      return initerror(Str("scanu: Could not find ifnmass table"));
    }
    len = p->len = f->flen;
    p->m = f->ftable;

    /* Centering */
    if ((f = ftfind(csound, p->i_c)) == NULL) {
      return initerror(Str("scanu: Could not find ifncentr table"));
    }
    if (f->flen != len)
      die(Str(
              "scanu: Parameter tables should all have the same length"));
    p->c = f->ftable;

    /* Damping */
    if ((f = ftfind(csound, p->i_d)) == NULL) {
      return initerror(Str("scanu: Could not find ifndamp table"));
    }
    if (f->flen != len)
      die(Str("scanu: Parameter tables should all have the same length"));
    p->d = f->ftable;

    /* Spring stiffness */
    {
      int i, j;

      /* Get the table */
      if ((f = ftfind(csound, p->i_f)) == NULL) {
        return initerror(Str("scanu: Could not find ifnstiff table"));
      }

     /* Check that the size is good */
      if (f->flen < len*len)
        die(Str("scanu: Spring matrix is too small"));

      /* Setup an easier addressing scheme */
      auxalloc(len*len * sizeof(MYFLT), &p->aux_f);
      p->f = (MYFLT*)p->aux_f.auxp;
      for (i = 0 ; i != len ; i++) {
        for (j = 0 ; j != len ; j++)
          p->f[i*len+j] = f->ftable[i*len+j];
      }
    }

/* Make buffers to hold data */
#if PHASE_INTERP == 3
    auxalloc(6*len*sizeof(MYFLT), &p->aux_x);
#else
    auxalloc(5*len*sizeof(MYFLT), &p->aux_x);
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
      int i;
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
      int i;
      FUNC *f = ftfind(csound, p->i_v);
      if (f == NULL) {
        return initerror(Str("scanu: Could not find ifnvel table"));
      }
      if (f->flen != len) {
        return initerror(Str(
                "scanu: Parameter tables should all have the same length"));
      }
      for (i = 0 ; i != len ; i++)
        p->v[i] = f->ftable[i];
    }

    /* Cache update rate over to local structure */
    p->rate = *p->i_rate * esr;

      /* Initialize index */
    p->idx = 0;

    /* External force index */
    p->exti = 0;

    /* Setup display window */
    if (*p->i_disp) {
      p->win = calloc(1, sizeof(WINDAT));
      dispset((WINDAT*)p->win, p->x1, len,
              Str("Mass displaycement"), 0,
              Str("Scansynth window"));
    }

    /* Make external force window if we haven't so far */
    if (ewin == NULL) {
      int i;
      MYFLT arg =  PI_F/(len-1);
      ewin = (MYFLT *)calloc(len, sizeof(MYFLT));
      for (i = 0 ; i != len-1 ; i++)
        ewin[i] = (MYFLT)sqrt(sin((double)arg*i));
      ewin[i] = FL(0.0); /* You get NaN otherwise */
    }

    /* Throw data into list or use table */
    if (*p->i_id < FL(0.0)) {
      MYFLT id = - *p->i_id;
      FUNC *f = ftfind(csound, &id);
      if (f == NULL) {
        return initerror(Str("scanu: Could not find (id) table"));
      }
      p->out = f->ftable;
      p->id = (int)*p->i_id;

    }
    else {
      p->id = (int)*p->i_id;
      listadd(p);
    }
    return OK;
}



/*
 *      Performance function for updater
 */

static MYFLT dt = FL(1.0);

int scsnu_play(ENVIRON *csound, PSCSNU *p)
{
    int n;
    int len = p->len;

    for (n = 0 ; n != ksmps ; n++) {

      /* Put audio input in external force */
      p->ext[p->exti] = p->a_ext[n];
      p->exti++;
      if (p->exti >= len)
        p->exti = 0;

      /* If it is time to calculate next phase, do it */
      if (p->idx >= p->rate) {
        int i, j;
        for (i = 0 ; i != len ; i++) {
          MYFLT a = FL(0.0);
                                /* Throw in audio drive */
          p->v[i] += p->ext[p->exti++] * ewin[i];
          if (p->exti >= len)
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
          display(p->win);
      }
      if (p->id<0) { /* Write to ftable */
        int i;
        MYFLT t = (MYFLT)p->idx / p->rate;
        for (i = 0 ; i != p->len ; i++) {
#if PHASE_INTERP == 3
          p->out[i] = p->x1[i] + t*(-p->x3[i]*FL(0.5) +
                                    t*(p->x3[i]*FL(0.5) - p->x1[i] + p->x2[i]*FL(0.5))
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
}



/****************************************************************************
 *      Functions for scsns
 ***************************************************************************/


/*
 *      Succesive phase interpolator
 */
#if PHASE_INTERP == 3
#define pinterp(ii, x) \
        (pp->x1[p->t[(int)ii]] + x*(-pp->x3[p->t[(int)ii]]*FL(0.5) + x*(pp->x3[p->t[(int)ii]]*FL(0.5) - pp->x1[p->t[(int)ii]] + pp->x2[p->t[(int)ii]]*FL(0.5)) + pp->x2[p->t[(int)ii]]*FL(0.5)))
#else
#define pinterp(ii, x) \
        (pp->x2[p->t[(int)ii]] + (pp->x1[p->t[(int)ii]] - pp->x2[p->t[(int)ii]]) * x)
#endif



/*
 *      Init scaner
 */
int scsns_init(ENVIRON *csound, PSCSNS *p)
{
    /* Get corresponding update */
    p->p = listget(csound, (int)*p->i_id);

    /* Get trajectory matrix */
    {
      int i;
      int oscil_interp = (int)*p->interp;
      FUNC *t = ftfind(csound, p->i_trj);
      if (t == NULL) {
        return initerror(Str("scans: Could not find the ifntraj table"));
      }
      if (oscil_interp<1 || oscil_interp>4) oscil_interp = 4;
      p->oscil_interp = oscil_interp;
      p->tlen = t->flen;
      /* Check that trajectory is within bounds */
      for (i = 0 ; i != p->tlen ; i++)
        if (t->ftable[i] < 0 || t->ftable[i] >= p->p->len)
          die(Str("vermp: Trajectory table includes values out of range"));
      /* Allocate memory and pad to accomodate interpolation */
                                /* Note that the 3 here is a hack -- jpff */
      auxalloc((p->tlen+3/*oscil_interp*/-1)*sizeof(long), &p->aux_t);
      p->t = (long*)p->aux_t.auxp + (int)(oscil_interp-1)/2;
      /* Fill 'er up */
      for (i = 0 ; i != p->tlen ; i++)
        p->t[i] = (long)t->ftable[i];
      /* Do wraparounds */
      for (i = 1 ; i <= (oscil_interp-1)/2 ; i++)
        p->t[-i] = p->t[i];
      for (i = 0 ; i <= oscil_interp/2 ; i++)
        p->t[p->tlen+1] = p->t[i];
    }
    /* Reset oscillator phase */
    p->phs = FL(0.0);
    /* Oscillator ratio */
    p->fix = (MYFLT)p->tlen*onedsr;
    return OK;
}



/*
 *      Performance function for scanner
 */
int scsns_play(ENVIRON *csound, PSCSNS *p)
{
    int i;
    MYFLT phs = p->phs, inc = *p->k_freq * p->fix;
    MYFLT t = (MYFLT)p->p->idx/p->p->rate;

    switch (p->oscil_interp) {
    case 1:
      for (i = 0 ; i != ksmps ; i++) {
      /* Do various interpolations to get output sample ... */
        PSCSNU *pp = p->p;
/*        MYFLT x = phs - (int)phs; */
        p->a_out[i] = *p->k_amp * (pinterp(phs, t));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        if (phs >= p->tlen)
          phs -= p->tlen;    /* Remember phase */
      }
      break;
    case 2:
      for (i = 0 ; i != ksmps ; i++) {
      /* Do various interpolations to get output sample ... */
        PSCSNU *pp = p->p;
        MYFLT x = phs - (int)phs;
        MYFLT y1 = pinterp(phs  , t);
        MYFLT y2 = pinterp(phs+1, t);

        p->a_out[i] = *p->k_amp * (y1 + x*(-y1 + y2));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        if (phs >= p->tlen)
          phs -= p->tlen;    /* Remember phase */
      }
      break;
    case 3:
      for (i = 0 ; i != ksmps ; i++) {
      /* Do various interpolations to get output sample ... */
        PSCSNU *pp = p->p;
        MYFLT x = phs - (int)phs;
        MYFLT y1 = pinterp(phs-1, t);
        MYFLT y2 = pinterp(phs  , t);
        MYFLT y3 = pinterp(phs+1, t);

        p->a_out[i] = *p->k_amp *
          (y2 + x*(-y1*FL(0.5) + x*(y1*FL(0.5) - y2 + y3*FL(0.5)) + y3*FL(0.5)));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        if (phs >= p->tlen)
          phs -= p->tlen;    /* Remember phase */
      }
      break;
    case 4:
      for (i = 0 ; i != ksmps ; i++) {
      /* Do various interpolations to get output sample ... */
        PSCSNU *pp = p->p;
        MYFLT x = phs - (int)phs;
        MYFLT y1 = pinterp(phs-1, t);
        MYFLT y2 = pinterp(phs  , t);
        MYFLT y3 = pinterp(phs+1, t);
        MYFLT y4 = pinterp(phs+2, t);

        p->a_out[i] = *p->k_amp *
          (y2 + x*(-y1/FL(3.0) - y2*FL(0.5) + y3 +
                   x*(y1*FL(0.5) - y2 + y3*FL(0.5) +
                      x*(-y1/FL(6.0) + y2*FL(0.5) - y3*FL(0.5) + y4/FL(6.0))) - y4/FL(6.0)));
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

#define S       sizeof

static OENTRY localops[] = {
{ "scanu", S(PSCSNU),5, "", "iiiiiiikkkkiikkaii",
  (SUBR)scsnu_init, NULL, (SUBR)scsnu_play },
{ "scans", S(PSCSNS),5, "a","kkiio", (SUBR)scsns_init, NULL, (SUBR)scsns_play}
};

LINKAGE
