/*
    scansynx.c:

    Copyright (C) 1999, 2000 Interval Research, John ffitch

*/

/* *********************************** */
/* *********************************** */
/* EXPERIMENTAL VERSION -- John ffitch */
/* *********************************** */
/* *********************************** */

/*
   Original 5.520u 0.130s 0:07.61 74.2%     0+0k 0+0io 451pf+0w
   Best     5.800u 0.130s 0:07.70 77.0%     0+0k 0+0io 437pf+0w
            5.770u 0.090s 0:07.34 79.8%     0+0k 0+0io 309pf+0w
            5.430u 0.120s 0:07.03 78.9%     0+0k 0+0io 307pf+0w
            5.390u 0.080s 0:06.97 78.4%     0+0k 0+0io 307pf+0w
            5.370u 0.100s 0:06.97 78.4%     0+0k 0+0io 307pf+0w
            5.300u 0.170s 0:07.00 78.1%     0+0k 0+0io 307pf+0w
            5.140u 0.090s 0:06.71 77.9%     0+0k 0+0io 307pf+0w
            5.200u 0.070s 0:06.85 76.9%     0+0k 0+0io 307pf+0w
            5.160u 0.090s 0:06.62 79.3%     0+0k 0+0io 274pf+0w
            5.760u 0.070s 0:07.23 80.6%     0+0k 0+0io 272pf+0w (bitmap)

   Ideas:   Make the matrix into a bitmap
            Hashed sparse matrix representation
            */

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
#include <math.h>
#include "cwindow.h"

/* Order of interpolation of scanning */
/* Either 1, 2 (linear), 3 (cubic) or 4 (quadratic) */

/* Order of interpolation of updating */
/* Either 2 (linear) or 3 (cubic) */
#define PHASE_INTERP 3
#define XALL

/* static void unquote(char *dst, char *src) */
/* { */
/*     if (src[0] == '"') { */
/*       int len = (int) strlen(src) - 2; */
/*       strcpy(dst, src + 1); */
/*       if (len >= 0 && dst[len] == '"') */
/*         dst[len] = '\0'; */
/*     } */
/*     else */
/*       strcpy(dst, src); */
/* } */

/***************************************************************************
 *      Helper functions and macros for updater                            *
 ***************************************************************************/

/*
 *      Wavetable init
 */
static int scsnux_initw(CSOUND *csound, PSCSNUX *p)
{
    uint32_t len = p->len;
    FUNC *fi = csound->FTnp2Find(csound, p->i_init);
    if (UNLIKELY(fi == NULL)) {
      return csound->InitError(csound,
                               Str("scanux: Could not find ifnnit ftable"));
    }
    if (UNLIKELY(fi->flen != len))
      return csound->InitError(csound, Str("scanux: Init table has bad size"));
    /*
      memcpy is 20 times faster that loop!!
    for (i = 0 ; i != len ; i++) {
      p->x0[i] = fi->ftable[i];
      p->x1[i] = fi->ftable[i];
      p->x2[i] = fi->ftable[i];
    }
    */
    len *= sizeof(MYFLT);
    memcpy(p->x0, fi->ftable, len);
    memcpy(p->x1, fi->ftable, len);
    memcpy(p->x2, fi->ftable, len);
    return OK;
}

/*
 *      Hammer hit
 */
static int scsnux_hammer(CSOUND *csound, PSCSNUX *p, MYFLT pos, MYFLT sgn)
{
    int i, i1, i2;
    FUNC *fi;
    MYFLT *f;
    MYFLT tab = FABS(*p->i_init);
    int32 len  = p->len;

    /* Get table */
    //if (UNLIKELY(tab<FL(0.0))) tab = -tab;   /* JPff fix here */
    if (UNLIKELY((fi = csound->FTnp2Find(csound, &tab)) == NULL)) {
      return csound->InitError(csound,
                               Str("scanux: Could not find ifninit ftable"));
    }

    /* Add hit */
    f  = fi->ftable;
    i1 = (int)(len*pos - fi->flen/2);
    i2 = (int)(len*pos + fi->flen/2);
    //printf("tab=%f len=%d i1=%d i2=%d\n", tab, len, i1, i2);///
    for (i = i1 ; i < 0 ; i++) {
      //printf("0: writing index %d (%d)\n", len+i, i);
#ifdef XALL
      p->x2[len+i] += sgn * *f;
      p->x3[len+i] += sgn * *f;
#endif
      p->x1[len+i] += sgn * *f++;
    }
    for (; i < len && i < i2 ; i++) {
      //printf("1: writing index %d\n", i);
#ifdef XALL
      p->x2[i] += sgn * *f;
      p->x3[i] += sgn * *f;
#endif
      p->x1[i] += sgn * *f++;
    }
    for (; i < i2 ; i++) {
      //printf("2: writing index %d (%d)\n", i-len, i);
#ifdef XALL
      p->x2[i-len] += sgn * *f;
      p->x3[i-len] += sgn * *f;
#endif
      p->x1[i-len] += sgn * *f++;
    }
    return OK;
}

/******************************
 *      Linked list stuff
 ******************************/

struct scsnx_elem {
    int                 id;
    PSCSNUX             *p;
    struct scsnx_elem   *next;
};

/* add to list */
static void listadd(SCANSYN_GLOBALS *pp, PSCSNUX *p)
{
    CSOUND  *csound = pp->csound;
    struct scsnx_elem *i = (struct scsnx_elem *) pp->scsnx_list;

    for ( ; i != NULL; i = i->next) {
      if (i->id == p->id) {
        i->p = p;
        return;
      }
    }
    i = (struct scsnx_elem *) csound->Malloc(csound, sizeof(struct scsnx_elem));
    i->id = p->id;
    i->p = p;
    i->next = (struct scsnx_elem *) pp->scsnx_list;
    pp->scsnx_list = (void*) i;
}

/* Return from list according to id */
static CS_NOINLINE PSCSNUX *listget(CSOUND *csound, int id)
{
    SCANSYN_GLOBALS   *pp;
    struct scsnx_elem *i;

    pp = scansyn_getGlobals(csound);
    i = (struct scsnx_elem *) pp->scsnx_list;
    if (UNLIKELY(i == NULL)) {
      csound->InitError(csound, Str("xscans: No scan synthesis net specified"));
      return NULL;
    }
    while (1) {
      if (i->id == id)
        break;
      i = i->next;
      if (UNLIKELY(i == NULL)) {
        csound->InitError(csound, Str("Eek ... scan synthesis id was not found"));
        return NULL;
      }
    }
    return i->p;
}

/****************************************************************************
 *      Functions for scsnux
 ***************************************************************************/

#define BITS_PER_UNIT (32)
#define LOG_BITS_PER_UNIT (5)

/*
 *      Setup the updater
 */

static int scsnux_init_(CSOUND *csound, PSCSNUX *p, int istring)
{
    /* Get parameter table pointers and check lengths */
    SCANSYN_GLOBALS *pp;
    FUNC    *f;
    uint32_t len;
    uint32_t i;

    /* Mass */
    if (UNLIKELY((f = csound->FTnp2Find(csound, p->i_m)) == NULL)) {
      return csound->InitError(csound,
                               Str("scanux: Could not find ifnmass table"));
    }
    len = p->len = f->flen;
    p->m = f->ftable;

    /* Centering */
    if (UNLIKELY((f = csound->FTnp2Find(csound, p->i_c)) == NULL)) {
      return csound->InitError(csound,
                               Str("scanux: Could not find ifncentr table"));
    }
    if (UNLIKELY(f->flen != len))
      return csound->InitError(csound, Str("scanux: Parameter tables should all "
                                           "have the same length"));
    p->c = f->ftable;

    /* Damping */
    if (UNLIKELY((f = csound->FTnp2Find(csound, p->i_d)) == NULL)) {
      return csound->InitError(csound,
                               Str("scanux: Could not find ifndamp table"));
    }
    if (UNLIKELY(f->flen != len))
      return csound->InitError(csound, Str("scanux: Parameter tables should all "
                                           "have the same length"));
    p->d = f->ftable;

    /* Spring stiffness */
    if (!istring) {
      uint32_t j, ilen;

      /* Get the table */
      if (UNLIKELY((f = csound->FTnp2Find(csound, p->i_f)) == NULL)) {
        return csound->InitError(csound,
                                 Str("scanux: Could not find ifnstiff table"));
      }

     /* Check that the size is good */
      if (UNLIKELY(f->flen < len*len))
        return csound->InitError(csound,
                                 Str("scanux: Spring matrix is too small"));

      /* Setup an easier addressing scheme */
#ifdef USING_CHAR
      /* ***** EXPERIMENTAL ****************************************** */
      /* This version uses a binary char matrix to save space and time */
      csound->AuxAlloc(csound, len*len * sizeof(char), &p->aux_f);
      p->f = (char*)p->aux_f.auxp;
#else
      /* ***** EXPERIMENTAL ****************************************** */
      /* This version uses a binary bit matrix to save space and time */
      csound->AuxAlloc(csound, 1L+(len*len*sizeof(int32))/BITS_PER_UNIT, &p->aux_f);
      p->f = (uint32*)p->aux_f.auxp;
#endif
      for (i = 0, ilen = 0 ; i != len ; i++, ilen += len) {
        for (j = 0 ; j != len ; j++) {
#ifdef USING_CHAR
          p->f[ilen+j] = (f->ftable[ilen+j] != 0 ? 1 : 0);
          if (p->f[ilen+j])
            csound->Message(csound, "%.0f: %d %d\n", *p->i_f, i, j);
#else
          int wd = (ilen+j)>>LOG_BITS_PER_UNIT; /* dead reckonng would be faster */
          int bt = (ilen+j)&(BITS_PER_UNIT-1);
          csound->Message(csound,
                          "%.0f: %d %d -> wd%d/bt%d\n", *p->i_f, i, j, wd, bt);
          p->f[wd] |= (1<<bt);
#endif
        }
      }
    }
    else {                      /* New format matrix */
      char filnam[256];
      MEMFIL *mfp;
      strncpy(filnam, ((STRINGDAT *) p->i_f)->data, 255);filnam[255]='\0';
      /* readfile if reqd */
      if (UNLIKELY((mfp =
                    csound->ldmemfile2withCB(csound, filnam,
                                       CSFTYPE_XSCANU_MATRIX, NULL)) == NULL)) {
        return csound->InitError(csound, Str("SCANU cannot load %s"), filnam);
      }
      else {
#define MATRIXLF "<MATRIX>\n"
#define MATLENLF (sizeof(MATRIXLF)-1)
#define MATRIXCRLF "<MATRIX>\r\n"
#define MATLENCRLF (sizeof(MATRIXCRLF)-1)
#define NMATRIXLF "</MATRIX>\n"
#define NMATLENLF (sizeof(NMATRIXLF)-1)
#define NMATRIXCRLF "</MATRIX>\r\n"
#define NMATLENCRLF (sizeof(NMATRIXCRLF)-1)
        unsigned int j;
        char *pp = mfp->beginp;
        if ((i=strncmp(pp, MATRIXLF, MATLENLF))==0) {
          pp += MATLENLF;
        }
        else if ((i=strncmp(pp, MATRIXCRLF, MATLENCRLF))==0) {
          pp += MATLENCRLF;
        }
        else {
          csound->Message(csound, Str("%d: Looking for (%d) %s Found %.12s\n"),
                                 i, (int32) MATLENLF, MATRIXLF, pp);
         return csound->InitError(csound, Str("Not a valid matrix"));
       }
#ifdef USING_CHAR
        csound->AuxAlloc(csound, len*len * sizeof(char), &p->aux_f);
        p->f = (char*)p->aux_f.auxp;
#else
        csound->AuxAlloc(csound,
                         BITS_PER_UNIT+(len*len*sizeof(int32))/BITS_PER_UNIT,
                         &p->aux_f);
        p->f = (uint32*)p->aux_f.auxp;
#endif
        while (pp < mfp->endp) {
          if (strncmp(pp, NMATRIXLF, NMATLENLF)==0) break;
          if (strncmp(pp, NMATRIXCRLF, NMATLENCRLF)==0) break;
          if (1 != sscanf(pp, "%d", &i)) break;
          if (1 != sscanf(pp, "%d", &j)) break;
#ifdef USING_CHAR
          p->f[i*len+j] = 1;
#else
          if (LIKELY(i<len && j<len)) { /* Only if in range! */
            int wd = (i*len+j)>>LOG_BITS_PER_UNIT;
            int bt = (i*len+j)&(BITS_PER_UNIT-1);
            p->f[wd] |= (1<<bt);
          }
          else {
            csound->Message(csound, Str("(%d,%d) is out of range\n"), i, j);
          }
#endif
          while (*pp++ != '\n') ;
        }
      }
    }

/* Make buffers to hold data */
#if PHASE_INTERP == 3
    csound->AuxAlloc(csound, 6*len*sizeof(MYFLT), &p->aux_x);
#else
    csound->AuxAlloc(csound, 5*len*sizeof(MYFLT), &p->aux_x);
#endif
    p->x0  = (MYFLT*)p->aux_x.auxp;
    p->x1  = p->x0 + len;
    p->x2  = p->x1 + len;
    p->ext = p->x2 + len;
    p->v   = p->ext + len;
#if PHASE_INTERP == 3
    p->x3  = p->v + len;
#endif

    /* Initialize them ... */
/*     for (i = 0 ; i != len ; i++) { */
/*       p->x0[i] = p->x1[i] = p->x2[i]= p->ext[i] = FL(0.0); */
/* #if PHASE_INTERP == 3 */
/*       p->x3[i] = FL(0.0); */
/* #endif */
/*     } */
#if PHASE_INTERP == 3
    memset(p->x0, 0, 6*len*sizeof(MYFLT));
#else
    memset(p->x0, 0, 5*len*sizeof(MYFLT));
#endif

    /* ... according to scheme */
    if ((int)*p->i_init < 0) {
      int res;
      res = scsnux_hammer(csound, p, *p->i_l, FL(1.0));
      if (res != OK) return res;
      res = scsnux_hammer(csound, p, *p->i_r, -FL(1.0));
      if (res != OK) return res;
    }
    else if (*p->i_id<FL(0.0))
      scsnux_hammer(csound, p, FL(0.5), FL(1.0));
    else {
      int res = scsnux_initw(csound, p);
      if (res != OK) return res;
    }

    /* Velocity gets presidential treatment */
    {
      FUNC *f = csound->FTnp2Find(csound, p->i_v);
      if (UNLIKELY(f == NULL)) {
        return csound->InitError(csound,
                                 Str("scanux: Could not find ifnvel table"));
      }
      if (UNLIKELY(f->flen != len)) {
        return csound->InitError(csound, Str("scanux: Parameter tables should "
                                             "all have the same length"));
      }
      for (i = 0 ; i != len ; i++)
        p->v[i] = f->ftable[i];
    }
    /* Cache update rate over to local structure */
    p->rate = *p->i_rate * csound->GetSr(csound);

      /* Initialize index */
    p->idx  = 0;

    /* External force index */
    p->exti = 0;

    /* Setup display window */
    if (*p->i_disp) {
      p->win = csound->Calloc(csound, sizeof(WINDAT));
      csound->dispset(csound, (WINDAT*) p->win, p->x1, len,
                      Str("Mass displacement"), 0, Str("Scansynth window"));
    }

    pp = scansyn_getGlobals(csound);
    p->pp = pp;

    /* Make external force window if we haven't so far */
    if (pp->ewinx == NULL) {
      MYFLT arg =  PI_F/(MYFLT)(len-1);
      pp->ewinx = (MYFLT*) csound->Malloc(csound, len * sizeof(MYFLT));
      for (i = 0 ; i != len-1 ; i++)
        pp->ewinx[i] = SQRT(SIN(arg*i));
      pp->ewinx[i] = FL(0.0); /* You get NaN otherwise */
    }

    /* Throw data into list or use table */
    p->id = (int) *p->i_id;
    if (p->id < 0) {
      if (UNLIKELY(csound->GetTable(csound, &(p->out), -(p->id)) < (int)len)) {
        return csound->InitError(csound, Str("xscanu: invalid id table"));
      }
    }
    else {
      listadd(pp, p);
    }

    return OK;
}

static int scsnux_init(CSOUND *csound, PSCSNUX *p){
  return scsnux_init_(csound, p, 0);
}

static int scsnux_init_S(CSOUND *csound, PSCSNUX *p){
  return scsnux_init_(csound, p, 1);
}

/*
 *      Performance function for updater
 */

#define dt FL(1.0)

static int scsnux(CSOUND *csound, PSCSNUX *p)
{
    SCANSYN_GLOBALS *pp;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int     len = p->len;
    int32    exti = p->exti;
    int32    idx = p->idx;
    MYFLT   rate = p->rate;
    MYFLT   *out = p->out;

    pp = p->pp;
    if (UNLIKELY(pp == NULL)) goto err1;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n = offset ; n < nsmps ; n++) {

      /* Put audio input in external force */
      p->ext[exti] = p->a_ext[n];
      exti++;
      if (UNLIKELY(exti >= len)) exti = 0;

      /* If it is time to calculate next phase, do it */
      if (idx >= rate) {
        int i, j, cnt = 0;      /* cnt is i*len+j */
        for (i = 0 ; i != len ; i++) {
          MYFLT a = FL(0.0);
                                /* Throw in audio drive */
          p->v[i] += p->ext[exti++] * pp->ewinx[i];
          if (UNLIKELY(exti >= len)) exti = 0L;
                                /* And push feedback */
          scsnux_hammer(csound, p, *p->k_x, *p->k_y);
                                /* Estimate acceleration */
          for (j = 0 ; j != len ; j++) {
#ifdef USING_CHAR
            if (p->f[cnt])  /* if connection */
              a += (p->x1[j] - p->x1[i])/* * p->f[cnt] */ * *p->k_f;
#else
            int wd = (cnt)>>LOG_BITS_PER_UNIT;
            int bt = (cnt)&(BITS_PER_UNIT-1);
            if (p->f[wd]&(1<<bt))
              a += (p->x1[j] - p->x1[i]) * *p->k_f;
#endif
            cnt++;
          }
          a += - p->x1[i] * p->c[i] * *p->k_c -
               (p->x2[i] - p->x1[i]) * p->d[i] * *p->k_d;
          a /= p->m[i] * *p->k_m;
                                /* From which we get velocity */
          p->v[i] += /* dt * */ a; /* Integrate accel to velocity */
                                /* ... and again to position future position */
          p->x0[i] += p->v[i] /* * dt */;
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
        idx = 0;
        if (*p->i_disp)
          csound->display(csound, p->win);
      }
      if (p->id<0) { /* Write to ftable */
        unsigned int i;
        MYFLT t  = (MYFLT)idx / rate;
        for (i = 0 ; i != p->len ; i++) {
#if PHASE_INTERP == 3
          out[i] = p->x1[i] +
            t*(-p->x3[i]*FL(0.5) +
               t*(p->x3[i]*FL(0.5) - p->x1[i] + p->x2[i]*FL(0.5))
               + p->x2[i]*FL(0.5));
#else
          out[i] = p->x2[i] + (p->x1[i] - p->x2[i]) * t;
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
      return csound->PerfError(csound, p->h.insdshead,
                               Str("xscanu: not initialised"));
}

/****************************************************************************
 *      Functions for scsnsx
 ***************************************************************************/

/*
 *      Succesive phase interpolator
 */
#if PHASE_INTERP == 3
#define PINTERP(ii, x) \
        (pp->x1[p->t[ii]] + x*((pp->x2[p->t[ii]]-pp->x3[p->t[ii]])*FL(0.5) + \
         x*((pp->x3[p->t[ii]]+pp->x2[p->t[ii]])*FL(0.5) - pp->x1[p->t[ii]])))
#else
#define PINTERP(ii, x) \
        (pp->x2[p->t[ii]] + (pp->x1[p->t[ii]] - pp->x2[p->t[ii]]) * x)
#endif

/*
 *      Init scaner
 */
static int scsnsx_init(CSOUND *csound, PSCSNSX *p)
{
    /* Get corresponding update */
    p->p = listget(csound, (int)*p->i_id);

    /* Get trajectory matrix */
    {
      int i;
      int oscil_interp = (int)*p->interp;
      FUNC *t = csound->FTnp2Find(csound, p->i_trj);
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
          return csound->InitError(csound, Str("scsn: Trajectory table includes "
                                               "values out of range"));
      /* Allocate mem<ory and pad to accomodate interpolation */
                                /* Note that the 3 here is a hack -- jpff */
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
    }
    /* Reset oscillator phase */
    p->phs = FL(0.0);
    /* Oscillator ratio */
    p->fix = (MYFLT)p->tlen*(1.0/csound->GetSr(csound));
    return OK;
}

/*
 *      Performance function for scanner
 */
static int scsnsx(CSOUND *csound, PSCSNSX *p)
{
    MYFLT   *out = p->a_out;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, nsmps = CS_KSMPS;
    int32 tlen   = p->tlen;
    MYFLT phs   = p->phs, inc = *p->k_freq * p->fix;
    MYFLT t = (MYFLT)p->p->idx/p->p->rate;
    MYFLT amp = *p->k_amp;
    PSCSNUX *pp = p->p;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    switch (p->oscil_interp) {
    case 1:
      for (i = offset ; i < nsmps ; i++) {
      /* Do various interpolations to get output sample ... */
/*      MYFLT x     = phs - (int)phs; */
        int ph = (int)phs;
        out[i] = amp * (PINTERP(ph, t));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        if (UNLIKELY(phs >= tlen)) phs -= tlen;
      }
      break;
    case 2:
      for (i = offset ; i < nsmps ; i++) {
      /* Do various interpolations to get output sample ... */
        int ph = (int)phs;
        MYFLT x     = phs - ph;
        MYFLT y1    = PINTERP(ph  , t);
        MYFLT y2    = PINTERP(ph+1, t);
        out[i] = amp * (y1 + x*(y2 - y1));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        if (UNLIKELY(phs >= tlen)) phs -= tlen;
      }
      break;
    case 3:
      for (i = offset ; i < nsmps ; i++) {
      /* Do various interpolations to get output sample ... */
        int ph = (int)phs;
        MYFLT x     = phs - ph;
        MYFLT y1    = PINTERP(ph-1, t);
        MYFLT y2    = PINTERP(ph  , t);
        MYFLT y3    = PINTERP(ph+1, t);
        out[i] = amp *
          (y2 + x*(-y1*FL(0.5) + x*(y1*FL(0.5) - y2 + y3*FL(0.5)) + y3*FL(0.5)));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        if (UNLIKELY(phs >= tlen)) phs -= tlen;
      }
      break;
    case 4:
      for (i = offset ; i < nsmps ; i++) {
      /* Do various interpolations to get output sample ... */
        int ph = (int)phs;
        MYFLT x     = phs - ph;
        MYFLT y1    = PINTERP(ph-1, t);
        MYFLT y2    = PINTERP(ph  , t);
        MYFLT y3    = PINTERP(ph+1, t);
        MYFLT y4    = PINTERP(ph+2, t);
        out[i] = amp *
          (y2 + x*(-y1/FL(3.0) - y2*FL(0.5) + y3 +
                   x*(y1*FL(0.5) - y2 + y3*FL(0.5) +
                      x*((y2 - y3)*FL(0.5) + (y4-y1)/FL(6.0))) -
                   y4/FL(6.0)));
                /* Update oscillator phase and wrap around if needed */
        phs += inc;
        if (UNLIKELY(phs >= tlen)) phs -= tlen;
      }
      break;
    }
    p->phs = phs;    /* Remember phase */
    return OK;
}

static int scsnmapx_init(CSOUND *csound, PSCSNMAPX *p)
{
    /* Get corresponding update */
    p->p = listget(csound, (int)*p->i_id);
    return OK;
}

static int scsnmapx(CSOUND *csound, PSCSNMAPX *p)
{
    PSCSNUX *pp = p->p;
    *p->k_pos = *p->k_pamp * pp->x0[(int)(*p->k_which)];
    *p->k_vel = *p->k_vamp * pp->v[(int)(*p->k_which)];
    return OK;
}

static int scsnsmapx(CSOUND *csound, PSCSNMAPX *p)
{
    PSCSNUX *pp = p->p;
    pp->x0[(int)(*p->k_which)] = *p->k_pos/(*p->k_pamp);
    pp->v[(int)(*p->k_which)]  = *p->k_vel/(*p->k_vamp);
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "xscanu", S(PSCSNUX),TR, 5, "", "iiiiSiikkkkiikkaii", (SUBR)scsnux_init_S,
                                                    NULL,(SUBR)scsnux },
{ "xscanu", S(PSCSNUX),TR, 5, "", "iiiiiiikkkkiikkaii", (SUBR)scsnux_init,
                                                    NULL,(SUBR)scsnux },
{ "xscans", S(PSCSNSX),  TR, 5,  "a", "kkiio",         (SUBR)scsnsx_init,
                                                    NULL, (SUBR)scsnsx},
{ "xscanmap", S(PSCSNMAPX),TR, 3, "kk", "ikko",        (SUBR)scsnmapx_init,
                                                   (SUBR)scsnmapx,NULL },
{ "xscansmap", S(PSCSNMAPX),TR, 3,"",   "kkikko",      (SUBR)scsnmapx_init,
                                                   (SUBR)scsnsmapx,NULL }
};

int scansynx_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}
