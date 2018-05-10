/*  Copyright (C) 2007 Gabriel Maldonado

  Csound is free software; you can redistribute it
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

//#include "csdl.h"
#include "csoundCore.h"
#include "interlocks.h"
#include <math.h>

#define f7bit       (FL(127.0))
#define oneTOf7bit  (MYFLT)(1.0/127.0)
#define f14bit      (FL(16383.0))
#define oneTOf14bit (MYFLT)(1.0/16383.0)
#define f21bit      (FL(2097151.0))
#define oneTOf21bit (MYFLT)(1.0/2097151.0)

typedef struct {
    MYFLT *ictlno, *imin, *imax, *initvalue, *ifn;
} SLD;

typedef struct {
    MYFLT *ictlno, *imin, *imax, *initvalue, *ifn, *ihp;
} SLDf;


/*--------------------------------------------------------*/

#define SLIDER_I_TABLE_INIT(p, n)                                       \
{                                                                       \
    unsigned char chan = p->slchan = (unsigned char)((*p->ichan)-1);    \
    char sbuf[120];                                                     \
    if (UNLIKELY(chan  > 15))  {                                        \
      return csound->InitError(csound, Str("illegal channel"));         \
    }                                                                   \
    else {                                                              \
        MYFLT value;                                                    \
        int32_t j = 0;                                                      \
        SLD *sld = p->s;                                                \
        unsigned char *slnum = p->slnum;                                \
        MYFLT *min = p->min, *max= p->max;                              \
        FUNC *outftp, **ftp = p->ftp;                                   \
        MYFLT *chanblock = (MYFLT *)  csound->m_chnbp[chan]->ctl_val;   \
                                                                        \
        if ((outftp = csound->FTnp2Find(csound, p->ioutfn)) != NULL)    \
          p->outTable = outftp->ftable;                                 \
        while (j < 8) {                                                 \
            int32_t t = (int32_t) *sld->ifn;                                    \
            *slnum = (unsigned char) *sld->ictlno;                      \
            value=*sld->initvalue;                                      \
                                                                        \
            if (UNLIKELY(*slnum > 127)) {                               \
              snprintf(sbuf, 120,                                       \
                        Str("illegal control number at position n.%d"), \
                        j);                                             \
                return csound->InitError(csound, "%s", sbuf);                 \
                break;                                                  \
            }                                                           \
            *min=*sld->imin;                                            \
            *max=*sld->imax;                                            \
            if (t !=0  && t != -1) {           /*table indexing */      \
              if (UNLIKELY(value >= 1 || value < 0)) {                  \
                snprintf(sbuf, 120,                                     \
                         Str("sliderXtable: illegal initvalue at "      \
                                  "position %d.  When using table "     \
                                  "indexing, the init range is 0 to 1"),\
                        j);                                             \
                return csound->InitError(csound, "%s", sbuf);           \
                break;                                                  \
              }                                                         \
            }                                                           \
            else if (UNLIKELY(value < *min || value > *max )) {         \
              snprintf(sbuf, 120,                                       \
                      Str("illegal initvalue at position n.%d"), j);    \
              return csound->InitError(csound, "%s", sbuf);             \
              break;                                                    \
            }                                                           \
                                                                        \
            switch (t) {                                                \
            case 0: /* LINEAR   */                                      \
              value =  (*sld->initvalue - *min) / (*max - *min);        \
              break;                                                    \
            case -1: /* EXPONENTIAL */                                  \
              if (UNLIKELY(*min == 0 || *max == 0)) {                   \
                return csound->InitError(csound,                        \
                                         Str("sliderXtable: zero is "   \
                                             "illegal in exponential "  \
                                             "operations"));            \
              }                                                         \
              {                                                         \
                MYFLT range = *max-*min;                                \
                MYFLT base;                                             \
                base= (MYFLT) pow(*max / *min, 1/range);                \
                value = (MYFLT) (log(value/ *min) / log(base)) ;        \
                value /= range;                                         \
              }                                                         \
              break;                                                    \
            default: /* TABLE */                                        \
              value = value; /* unchanged, value must be in the 0 to 1 range, */ \
              /*   representing the phase of the table            */    \
              if (*sld->ifn > 0)   *ftp = csound->FTnp2Find(csound, sld->ifn); \
            }                                                           \
            chanblock[*slnum++] =  (MYFLT)((int32_t)(value * f7bit + FL(0.5))); \
            min++; max++; ftp++; j++; sld++;                            \
        }                                                               \
    }                                                                   \
}                                                                       \
return OK;

#define SLIDER_TABLE_INIT(p, n)                                         \
{                                                                       \
    int32_t j = 0;                                                          \
    FUNC **ftp = p->ftp;                                                \
    MYFLT *chanblock = (MYFLT *) csound->m_chnbp[p->slchan]->ctl_val;   \
    unsigned char  *slnum = p->slnum;                                   \
    MYFLT *min = p->min, *max = p->max;                                 \
    MYFLT *outTable = p->outTable + (int32_t) *p->ioffset;                  \
    *p->ktrig = 0;                                                      \
    while (j < n) {                                                     \
      int32_t t = (int32_t) *(p->s[j].ifn);                                     \
      int32_t val = (int32_t) chanblock[*slnum++];                              \
      MYFLT value = (MYFLT) val / f7bit;                                \
      if (val != p->oldvalue[j] ) {                                     \
        MYFLT base, range = *max - *min;                                \
        *p->ktrig = 1;                                                  \
        p->oldvalue[j] = val;                                           \
        switch (t) {                                                    \
        case -1: /* EXPONENTIAL */                                      \
          base = (MYFLT) pow((*max / *min), 1/range);                   \
          value = *min * (MYFLT) pow(base, value * range);              \
          break;                                                        \
        case 0: /* LINEAR */                                            \
          value = value * range + *min;                                 \
          break;                                                        \
        default: /* TABLE */                                            \
          value = *((*ftp)->ftable + (int64_t)(value * (*ftp)->flen));     \
          value = value * range + *min;   /* scales the output */       \
          break;                                                        \
        }                                                               \
        *outTable = value;                                              \
      }                                                                 \
      min++; max++; j++; ftp++; outTable++;                             \
    }                                                                   \
}                                                                       \
    return OK;

/*--------------------------------------------------------*/

typedef struct {
    OPDS   h;
    MYFLT  *ktrig;      /* output */
    MYFLT  *ichan,  *ioutfn, *ioffset;              /* input */
    SLD    s[8];
    MYFLT  min[8], max[8], *outTable;
    unsigned char   slchan, slnum[8], oldvalue[8];
    FUNC   *ftp[8];
} SLIDER8t; /* GAB */


static int32_t sliderTable_i8(CSOUND *csound, SLIDER8t *p) /* GAB */
{
    SLIDER_I_TABLE_INIT(p,8);
}

static int32_t sliderTable8(CSOUND *csound, SLIDER8t *p) /* GAB */
{
    SLIDER_TABLE_INIT(p,8);

}
/*--------------------------------------------------------*/

typedef struct {
    OPDS   h;
    MYFLT  *ktrig;      /* output */
    MYFLT  *ichan,  *ioutfn, *ioffset;              /* input */
    SLD    s[16];
    MYFLT  min[16], max[16], *outTable;
    unsigned char   slchan, slnum[16], oldvalue[16];
    FUNC   *ftp[16];
} SLIDER16t; /* GAB */

static int32_t sliderTable_i16(CSOUND *csound, SLIDER16t *p) /* GAB */
{
    SLIDER_I_TABLE_INIT(p,16);
}

static int32_t sliderTable16(CSOUND *csound, SLIDER16t *p) /* GAB */
{
    SLIDER_TABLE_INIT(p,16);
}
/*--------------------------------------------------------*/

typedef struct {
    OPDS   h;
    MYFLT  *ktrig;      /* output */
    MYFLT  *ichan,  *ioutfn, *ioffset;              /* input */
    SLD    s[32];
    MYFLT  min[32], max[32], *outTable;
    unsigned char   slchan, slnum[32], oldvalue[32];
    FUNC   *ftp[32];
} SLIDER32t; /* GAB */


static int32_t sliderTable_i32(CSOUND *csound, SLIDER32t *p) /* GAB */
{
    SLIDER_I_TABLE_INIT(p,32);
}

static int32_t sliderTable32(CSOUND *csound, SLIDER32t *p) /* GAB */
{
    SLIDER_TABLE_INIT(p,32);
}
/*--------------------------------------------------------*/

typedef struct {
    OPDS   h;
    MYFLT  *ktrig;      /* output */
    MYFLT  *ichan,  *ioutfn, *ioffset;              /* input */
    SLD    s[64];
    MYFLT  min[64], max[64], *outTable;
    unsigned char   slchan, slnum[64], oldvalue[64];
    FUNC   *ftp[64];
} SLIDER64t; /* GAB */

static int32_t sliderTable_i64(CSOUND *csound, SLIDER64t *p) /* GAB */
{
    SLIDER_I_TABLE_INIT(p,64);
}

static int32_t sliderTable64(CSOUND *csound, SLIDER64t *p) /* GAB */
{
    SLIDER_TABLE_INIT(p,64);
}


/*--------------------------------------------------------*/


#define SLIDER_I_TABLEF_INIT(p,n)                                       \
  {                                                                     \
    unsigned char chan = p->slchan = (unsigned char)((*p->ichan)-1);    \
    char sbuf[120];                                                     \
    if (UNLIKELY(chan  > 15))  {                                        \
      return csound->InitError(csound, Str("illegal channel"));         \
    }                                                                   \
    {                                                                   \
      MYFLT value;                                                      \
      int32_t j = 0;                                                        \
      SLDf *sld = p->s;                                                 \
      unsigned char *slnum = p->slnum;                                  \
      MYFLT *min = p->min, *max= p->max;                                \
      FUNC *outftp, **ftp = p->ftp;                                     \
      MYFLT *chanblock = (MYFLT *) csound->m_chnbp[chan]->ctl_val;      \
                                                                        \
      MYFLT   b;                                                        \
      MYFLT *yt1 = p->yt1, *c1=p->c1, *c2=p->c2;                        \
                                                                        \
                                                                        \
      if((outftp = csound->FTnp2Find(csound, p->ioutfn)) != NULL)       \
        p->outTable = outftp->ftable;                                   \
      while (j < n) {                                                   \
        int32_t t = (int32_t) *sld->ifn;                                        \
        *slnum = (unsigned char) *sld->ictlno;                          \
        value=*sld->initvalue;                                          \
                                                                        \
        if (UNLIKELY(*slnum > 127)) {                                   \
          snprintf(sbuf, 120,                                           \
                  Str("illegal control number at position n.%d"), j);   \
          return csound->InitError(csound, "%s", sbuf);                 \
          break;                                                        \
        }                                                               \
        if (UNLIKELY(value < (*min=*sld->imin) ||                       \
                     value > (*max=*sld->imax) )) {                     \
          snprintf(sbuf, 120,                                           \
                  Str("illegal initvalue at position n.%d"), j);        \
          return csound->InitError(csound, "%s", sbuf);                 \
          break;                                                        \
        }                                                               \
                                                                        \
        switch (t) {                                                    \
        case 0: /* LINEAR */                                            \
          value =  (*sld->initvalue - *min) / (*max - *min);            \
          break;                                                        \
        case -1: /* EXPONENTIAL */                                      \
          if (*min == 0 || *max == 0) {                                 \
            return csound->InitError(csound,                            \
                                     Str("sliderXtable: zero is illegal"\
                                         " in exponential operations"));\
          }                                                             \
          {                                                             \
            MYFLT range = *max-*min;                                    \
            MYFLT base;                                                 \
            base= (MYFLT) pow(*max / *min, 1/range);                    \
            value = (MYFLT) (log(value/ *min) / log(base)) ;            \
            value /= range;                                             \
          }                                                             \
          break;                                                        \
        default: /* TABLE */                                            \
          value = value; /* unchanged, value must be in the 0 to 1 range, */ \
          /* representing the phase of the table */                     \
          if (*sld->ifn > 0)   *ftp = csound->FTnp2Find(csound, sld->ifn); \
          if (UNLIKELY(value >= 1 || value < 0)) {                      \
            snprintf(sbuf, 120,                                         \
                    Str("sliderXtable: illegal initvalue at "           \
                              "position %d. When using table indexing," \
                              " the init range is 0 to 1"), j);         \
            return csound->InitError(csound, "%s", sbuf);               \
          }                                                             \
        }                                                               \
        chanblock[*slnum++] =  (MYFLT)((int32_t)(value * f7bit + FL(0.5))); \
        /*----- init filtering coeffs*/                                 \
        *yt1++ = FL(0.0);                                               \
        b = (MYFLT)(2.0 - cos((double)(*(sld)->ihp *                    \
                              csound->tpidsr * CS_KSMPS)));             \
        *c2 = (MYFLT)(b - sqrt((double)(b * b - FL(1.0))));             \
        *c1++ = FL(1.0) - *c2++;                                        \
                                                                        \
        min++; max++; ftp++; j++; sld++;                                \
      }                                                                 \
    }                                                                   \
}                                                                       \
    return OK;


#define SLIDER_TABLEF_INIT(p,n)                                                 \
{                                                                               \
    MYFLT value, base;                                                          \
    int32_t j = 0;                                                              \
    FUNC **ftp = p->ftp;                                                        \
    MYFLT *chanblock = (MYFLT *) csound->m_chnbp[p->slchan]->ctl_val;           \
    unsigned char  *slnum = p->slnum;                                           \
    MYFLT *min = p->min, *max = p->max;                                         \
    MYFLT *outTable = p->outTable + (int32_t) *p->ioffset;                      \
    MYFLT *yt1 = p->yt1, *c1=p->c1, *c2=p->c2;                                  \
                                                                                \
    while (j < n) {                                                             \
        int32_t t = (int32_t) *(p->s[j].ifn);                                   \
        MYFLT range;                                                            \
        int32_t val = (int32_t) chanblock[*slnum++];                            \
        value = (MYFLT) val / f7bit;                                            \
        if (val != p->oldvalue[j] ) {                                           \
            *p->ktrig = 1;                                                      \
            p->oldvalue[j] = val;                                               \
        }                                                                       \
        switch (t) {                                                            \
        case -1: /* EXPONENTIAL */                                              \
            range = *max - *min;                                                \
            base = (MYFLT) pow((*max / *min), 1/range);                         \
            value = *min * (MYFLT) pow(base, value * range);                    \
            break;                                                              \
        case 0: /* LINEAR   */                                                  \
            value = value * (*max++ - *min) + *min;                             \
            break;                                                              \
        default: /* TABLE   */                                                  \
            value = *((*ftp)->ftable + (int64_t)(value * (*ftp)->flen));        \
            value = value * (*max - *min) + *min;   /* scales the output */     \
            break;                                                              \
        }                                                                       \
        *outTable++ =                                                           \
            *yt1 = *c1++ * value + *c2++ * *yt1; /* filters the output */       \
        yt1++; min++; max++; j++; ftp++;                                        \
    }                                                                           \
}                                                                               \
return OK;

/*--------------------------------------------------------*/

typedef struct {
    OPDS   h;
    MYFLT  *ktrig;                          /* output */
    MYFLT  *ichan,  *ioutfn, *ioffset;      /* input */
    SLDf   s[8];
    MYFLT  min[8], max[8], *outTable;
    unsigned char   slchan, slnum[8], oldvalue[8];
    FUNC   *ftp[8];
    MYFLT  c1[8], c2[8];
    MYFLT  yt1[8];
} SLIDER8tf;


static int32_t sliderTable_i8f(CSOUND *csound, SLIDER8tf *p)
{
    SLIDER_I_TABLEF_INIT(p,8)
}

static int32_t sliderTable8f(CSOUND *csound, SLIDER8tf *p)
{
    SLIDER_TABLEF_INIT(p,8)
}
/*--------------------------------------------------------*/


typedef struct {
    OPDS   h;
    MYFLT  *ktrig;                          /* output */
    MYFLT  *ichan,  *ioutfn, *ioffset;      /* input */
    SLDf   s[16];
    MYFLT  min[16], max[16], *outTable;
    unsigned char   slchan, slnum[16], oldvalue[16];
    FUNC   *ftp[16];
    MYFLT  c1[16], c2[16];
    MYFLT  yt1[6];
} SLIDER16tf;

static int32_t sliderTable_i16f(CSOUND *csound, SLIDER16tf *p)
{
    SLIDER_I_TABLEF_INIT(p,16)
}

static int32_t sliderTable16f(CSOUND *csound, SLIDER16tf *p)
{
    SLIDER_TABLEF_INIT(p,16)
}
/*--------------------------------------------------------*/

typedef struct {
    OPDS   h;
    MYFLT  *ktrig;                          /* output */
    MYFLT  *ichan,  *ioutfn, *ioffset;      /* input */
    SLDf   s[32];
    MYFLT  min[32], max[32], *outTable;
    unsigned char   slchan, slnum[32], oldvalue[32];
    FUNC   *ftp[32];
    MYFLT  c1[32], c2[32];
    MYFLT  yt1[32];
} SLIDER32tf;


static int32_t sliderTable_i32f(CSOUND *csound, SLIDER32tf *p)
{
    SLIDER_I_TABLEF_INIT(p,32)
}

static int32_t sliderTable32f(CSOUND *csound, SLIDER32tf *p)
{
    SLIDER_TABLEF_INIT(p,32)
}
/*--------------------------------------------------------*/

typedef struct {
    OPDS   h;
    MYFLT  *ktrig;                          /* output */
    MYFLT  *ichan,  *ioutfn, *ioffset;      /* input */
    SLDf   s[64];
    MYFLT  min[64], max[64], *outTable;
    unsigned char   slchan, slnum[64], oldvalue[64];
    FUNC   *ftp[64];
    MYFLT  c1[6], c2[64];
    MYFLT  yt1[64];
} SLIDER64tf;

static int32_t sliderTable_i64f(CSOUND *csound, SLIDER64tf *p)
{
    SLIDER_I_TABLEF_INIT(p,64)
}

static int32_t sliderTable64f(CSOUND *csound, SLIDER64tf *p)
{
    SLIDER_TABLEF_INIT(p,64)
}

/*--------------------------------------------------------*/
/*
    An opcode to expand the versatility of the KAWAI MM-16 midi mixer.
    This device doesn't allow to program the midi message associated
    to each slider. In fact it is only able to output the control 7 for
    each midi channel. With this opcode it is possible to assign each
    slider to the 16 output values.

     Syntax: k1,k2,....k16 sliderKawai \
           imin1, imax1, initvalue1, ifn1,
           imin2, imax2, initvalue2, ifn2,
           ..............................
           imin16, imax16, initvalue16, ifn16,
*/

typedef struct {
    MYFLT *imin, *imax, *initvalue, *ifn;
} SLD2;

typedef struct {
    OPDS   h;
    MYFLT  *r[16];             /* output */
    SLD2   s[16];
    MYFLT  min[16], max[16];
    FUNC   *ftp[16];
} SLIDERKAWAI;

static int32_t sliderKawai_i(CSOUND *csound, SLIDERKAWAI *p)
{
    char sbuf[120];
    int32_t n = 16;
    MYFLT value;
    int32_t j = 0;
    SLD2 *sld = p->s;
    MYFLT *min = p->min, *max= p->max;
    FUNC **ftp = p->ftp;
    do  {
      if (UNLIKELY((value=*sld->initvalue) < (*min=*sld->imin) ||
                   value > (*max=*sld->imax) )) {
        snprintf(sbuf, 120, Str("illegal initvalue at position n.%d"), j);
        return csound->InitError(csound, "%s", sbuf);
      }
      if (*sld->ifn > 0)   *ftp++ = csound->FTnp2Find(csound, sld->ifn);
      else                 *ftp++ = NULL;
      value =  (*(sld++)->initvalue - *min) / (*max++ - *min);
      min++;
      csound->m_chnbp[j]->ctl_val[7] = (MYFLT)((int32_t)(value * f7bit + FL(0.5)));
    } while (++j < n);
    return OK;
}

static int32_t sliderKawai(CSOUND *csound, SLIDERKAWAI *p)
{
    MYFLT value;
    int32_t j = 0, n=16;
    FUNC **ftp = p->ftp-1;
    MYFLT *min = p->min, *max = p->max;
    MYFLT **result = p->r;
    do {
      value = (MYFLT)  csound->m_chnbp[j]->ctl_val[7] * oneTOf7bit;
      if (*(++ftp))             /* if valid ftable,use value as index   */
        /* no interpolation */
        value = *((*ftp)->ftable + (int64_t)(value * (*ftp)->flen));
      **result++ = value * (*max++ - *min) + *min;   /* scales the output */
      min++;;
    } while (++j < n);
    return OK;
}

/*--------------------------------------------------------*/
#define TOOSMALL 0.0000000000000000000000001f  /* to avoid denormalization bug */

typedef struct {
    OPDS   h;
    MYFLT  *r, *ichan, *ictlno, *imin, *imax, *ifn, *icutoff;
    short flag;
    FUNC *ftp;
    int64_t   ctlno;

    MYFLT   c1;     /* Value to multiply with input value  */
    MYFLT   c2;     /* Value to multiply with previous state */
    MYFLT   yt1;        /* Previous state */
    MYFLT   prev;

} CTRL7a;

static int32_t ctrl7a_set(CSOUND *csound, CTRL7a *p)
{
    int32_t ctlno, chan;
    MYFLT   cutoff, b;


    if ((ctlno = (int32_t) *p->ictlno) < 0 || ctlno > 127)
      return csound->InitError(csound, Str("illegal controller number"));
    else if ((chan=(int32_t) *p->ichan-1) < 0 || chan > 15)
      return csound->InitError(csound, Str("illegal midi channel"));
    else p->ctlno = ctlno;

    if (*p->ifn > 0) {
      if (UNLIKELY(((p->ftp = csound->FTnp2Find(csound, p->ifn)) == NULL)))
        p->flag = 0;  /* invalid ftable */
      else p->flag= 1;
    }
    else p->flag= 0;

    p->yt1 = FL(0.0);
    if (*p->icutoff <= 0) cutoff = 5;
    else cutoff = *p->icutoff;

    b = FL(2.0) - COS(cutoff * csound->tpidsr * CS_KSMPS);
    p->c2 = b - SQRT(b * b - 1.0);
    p->c1 = FL(1.0) - p->c2;
    p->prev = 0;
    return OK;
}

static int32_t ctrl7a(CSOUND *csound, CTRL7a *p)
{
    MYFLT       *ar, val, incr;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT value =
      (MYFLT) (csound->m_chnbp[(int32_t) *p->ichan-1]->
               ctl_val[p->ctlno]*oneTOf7bit);
    if (p->flag)  {             /* if valid ftable,use value as index   */
                                /* no interpolation */
      value = *(p->ftp->ftable + (int64_t)(value*(p->ftp->flen-1)));
    }
    /* scales the output */
    value = value * (*p->imax - *p->imin) + *p->imin + TOOSMALL;
    value = p->yt1 = p->c1 * value + p->c2 * p->yt1;
    ar = p->r;
    val = p->prev;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    incr = (value - val) / (MYFLT)(nsmps-offset);
    for (n=offset; n<nsmps; n++) {
      ar[n] = val += incr;
    }
    p->prev = val;
    return OK;
}



#define S(x)    sizeof(x)

OENTRY sliderTable_localops[] = {
{ "slider8table", S(SLIDER8t), 0, 3, "k",  "iii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
  (SUBR)sliderTable_i8, (SUBR)sliderTable8, (SUBR)NULL },
{ "slider16table", S(SLIDER8t), 0, 3, "k", "iii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
  (SUBR)sliderTable_i16, (SUBR)sliderTable16, (SUBR)NULL },
{ "slider32table", S(SLIDER8t), 0, 3, "k", "iii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
  (SUBR)sliderTable_i32, (SUBR)sliderTable32, (SUBR)NULL },
{ "slider64table", S(SLIDER8t), 0, 3, "k", "iii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
  (SUBR)sliderTable_i64, (SUBR)sliderTable64, (SUBR)NULL },
{ "slider8tablef", S(SLIDER8tf), 0, 3, "k", "iii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
  (SUBR)sliderTable_i8f, (SUBR)sliderTable8f, (SUBR)NULL },
{ "slider16tablef",S(SLIDER16tf), 0, 3, "k", "iii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
  (SUBR)sliderTable_i16f, (SUBR)sliderTable16f, (SUBR)NULL },
{ "slider32tablef",S(SLIDER32tf), 0, 3, "k", "iii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
  (SUBR)sliderTable_i32f, (SUBR)sliderTable32f, (SUBR)NULL },
{ "slider64tablef",S(SLIDER64tf), 0, 3, "k", "iii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
  (SUBR)sliderTable_i64f, (SUBR)sliderTable64f, (SUBR)NULL },
{ "sliderKawai", S(SLIDERKAWAI),  0, 3, "kkkkkkkkkkkkkkkk",
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
  (SUBR)sliderKawai_i, (SUBR)sliderKawai, NULL },
{ "ctrl7.a", S(CTRL7a),  0, 3, "a",    "iikkoo",
  (SUBR) ctrl7a_set,    (SUBR) ctrl7a },
};

int32_t slidertable_init_(CSOUND *csound) {
    return
      csound->AppendOpcodes(csound, &(sliderTable_localops[0]),
                            (int32_t) (sizeof(sliderTable_localops)/sizeof(OENTRY)));
}
