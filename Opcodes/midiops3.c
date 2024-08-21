/*
    midiops3.c:

    Copyright (C) 1997 Gabriel Maldonado

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

/* sliders and other MIDI opcodes by Gabriel Maldonado */

#include "stdopcod.h"
#include "midiops.h"
#include "midiops3.h"
#include <math.h>

#define f7bit           (FL(127.0))
#define oneTOf7bit      (MYFLT)(1.0/127.0)
#define f14bit          (FL(16383.0))
#define oneTOf14bit     (MYFLT)(1.0/16383.0)
#define f21bit          (FL(2097151.0))
#define oneTOf21bit     (MYFLT)(1.0/2097151.0)

/* This set of macros is rather a cop-out! */
#define SLIDERI_INIT(p, n)                                        \
{                                                                 \
    unsigned char chan = p->slchan = (unsigned char)((*p->ichan)-1); \
    char sbuf[120];                                               \
    if (UNLIKELY(chan > 15)) {                                    \
      return csound->InitError(csound, Str("illegal channel"));   \
    }                                                             \
    {                                                             \
      MYFLT value;                                                \
      int32_t j = 0;                                                  \
      SLD *sld = p->s;                                            \
      unsigned char *slnum = p->slnum;                            \
      MYFLT *min = p->min, *max= p->max;                          \
      FUNC **ftp = p->ftp;                                        \
      MYFLT *chanblock = (MYFLT *) csound->m_chnbp[chan]->ctl_val;\
      while (j++ < n) {                                           \
      *slnum = (unsigned char) *sld->ictlno;                      \
      if (UNLIKELY(*slnum > 127)) {                               \
        snprintf(sbuf, 120,                                       \
                  Str("illegal control number at position n.%d"), j); \
          return csound->InitError(csound, "%s", sbuf);           \
        }                                                         \
      if (UNLIKELY((value=*sld->initvalue) < (*min=*sld->imin) || \
                   value > (*max=*sld->imax) )) {                 \
        snprintf(sbuf, 120,                                       \
                  Str("illegal initvalue at position n.%d"),      \
                  j);                                             \
          return csound->InitError(csound, "%s", sbuf);           \
        }                                                         \
        if (*sld->ifn > 0)   *ftp++ = csound->FTFind(csound, sld->ifn); \
        else                 *ftp++ = NULL;                       \
        value =  (*(sld++)->initvalue - *min) / (*max++ - *min);  \
        min++;                                                    \
        chanblock[*slnum++] =  (MYFLT)((int32_t)(value * f7bit + FL(0.5))); \
      }                                                           \
    }                                                             \
    return OK;                                                    \
}

#define SLIDER_INIT(p, n)                                         \
{                                                                 \
    MYFLT value;                                                  \
    int32_t j = 0;                                                    \
    FUNC **ftp = p->ftp-1;                                        \
    MYFLT *chanblock = (MYFLT *) csound->m_chnbp[p->slchan]->ctl_val; \
    unsigned char  *slnum = p->slnum;                             \
    MYFLT *min = p->min, *max = p->max;                           \
    MYFLT **result = p->r;                                        \
    while (j++ < n) {                                             \
      value = (MYFLT) (chanblock[*slnum++] * oneTOf7bit);         \
      if (*(++ftp))   /* if valid ftable,use value as index   */  \
        value = *((*ftp)->ftable + (int32)(value * (*ftp)->flen)); \
                                /* no interpolation */            \
      **result++ = value * (*max++ - *min) + *min;   /* scales the output */ \
      min++;;                                                     \
    }                                                             \
    return OK;                                                    \
}

/*--------------------------------------------------------*/

static int32_t slider_i8(CSOUND *csound, SLIDER8 *p)
{
    SLIDERI_INIT(p, 8);
}

static int32_t slider8(CSOUND *csound, SLIDER8 *p)
{
    SLIDER_INIT(p, 8);
}

static int32_t slider_i16(CSOUND *csound, SLIDER16 *p)
{
    SLIDERI_INIT(p, 16);
}

static int32_t slider16(CSOUND *csound, SLIDER16 *p)
{
    SLIDER_INIT(p, 16);
}

static int32_t slider_i32(CSOUND *csound, SLIDER32 *p)
{
    SLIDERI_INIT(p, 32);
}

static int32_t slider32(CSOUND *csound, SLIDER32 *p)
{
    SLIDER_INIT(p, 32);
}

static int32_t slider_i64(CSOUND *csound, SLIDER64 *p)
{
    SLIDERI_INIT(p, 64);
}

static int32_t slider64(CSOUND *csound, SLIDER64 *p)
{
    SLIDER_INIT(p, 64);
}

/*==============================*/
#define SLIDERIF(p, n)                                            \
{                                                                 \
    unsigned char chan = p->slchan = (unsigned char)((*p->ichan)-1); \
    char sbuf[120];                                               \
    if (UNLIKELY(chan  > 15))  {                                  \
      return csound->InitError(csound, Str("illegal channel"));   \
    }                                                             \
    {                                                             \
      MYFLT value = FL(0.0);                                      \
      int32_t j = 0;                                                  \
      SLDf *sld = p->s;                                           \
      unsigned char *slnum = p->slnum;                            \
      MYFLT *min = p->min, *max= p->max;                          \
      FUNC **ftp = p->ftp;                                        \
      MYFLT     b;                                                \
      MYFLT *yt1 = p->yt1, *c1=p->c1, *c2=p->c2;                  \
      MYFLT *chanblock = (MYFLT *) csound->m_chnbp[chan]->ctl_val;\
      while (j++ < 8) {                                           \
      *slnum = (unsigned char) *sld->ictlno;                      \
      if (UNLIKELY(*slnum > 127)) {                               \
        snprintf(sbuf, 120,                                       \
                  Str("illegal control number at position n.%d"), j); \
          return csound->InitError(csound, "%s", sbuf);           \
        }                                                         \
      if (UNLIKELY((value=*sld->initvalue) < (*min=*sld->imin) || \
                   value > (*max=*sld->imax) )) {                 \
        snprintf(sbuf, 120,                                       \
                  Str("illegal initvalue at position n.%d"), j);  \
          return csound->InitError(csound, "%s", sbuf);           \
        }                                                         \
        if (*sld->ifn > 0)   *ftp++ = csound->FTFind(csound, sld->ifn); \
        else                 *ftp++ = NULL;                       \
        value =  (*sld->initvalue - *min) / (*max++ - *min);      \
        min++;;                                                   \
        chanblock[*slnum++] =  (MYFLT)(int32_t)(value * f7bit + FL(0.5));\
                                                                  \
                /*----- init filtering coeffs*/                   \
        *yt1++ = FL(0.0);                                         \
        b = (MYFLT)(2.0 - cos((double)(*(sld++)->ihp              \
                                       * CS_TPIDSR           \
                                       * CS_KSMPS)));        \
        *c2 = (MYFLT)(b - sqrt((double)(b * b - FL(1.0))));       \
        *c1++ = FL(1.0) - *c2++;                                  \
      }                                                           \
    }                                                             \
    return OK;                                                    \
}

#define SLIDERF(p, n)                                             \
{                                                                 \
    MYFLT value;                                                  \
    int32_t j = 0;                                                    \
    FUNC **ftp = p->ftp-1;                                        \
    MYFLT *chanblock = (MYFLT *) csound->m_chnbp[p->slchan]->ctl_val; \
    unsigned char  *slnum = p->slnum;                             \
    MYFLT *min = p->min, *max = p->max;                           \
    MYFLT **result = p->r;                                        \
    MYFLT *yt1 = p->yt1, *c1=p->c1, *c2=p->c2;                    \
    while (j++ < n) {                                             \
      value = chanblock[*slnum++] * oneTOf7bit;                   \
      if (*(++ftp))    /* if valid ftable,use value as index   */ \
        value = *( (*ftp)->ftable + (int32)(value * (*ftp)->flen));\
      value = value * (*max++ - *min) + *min; /* scales the output */ \
      min++;                                                      \
      **result++ =                                                \
        *yt1 = *c1++ * value + *c2++ * *yt1; /* filters the output */ \
      yt1++;                                                      \
    }                                                             \
    return OK;                                                    \
}

static int32_t slider_i8f(CSOUND *csound, SLIDER8f *p)
{
    SLIDERIF(p, 8);
}

static int32_t slider8f(CSOUND *csound, SLIDER8f *p)
{
    SLIDERF(p, 8);
}

static int32_t slider_i16f(CSOUND *csound, SLIDER16f *p)
{
    SLIDERIF(p, 16);
}

static int32_t slider16f(CSOUND *csound, SLIDER16f *p)
{
    SLIDERF(p, 16);
}

static int32_t slider_i32f(CSOUND *csound, SLIDER32f *p)
{
    SLIDERIF(p, 32);
}

static int32_t slider32f(CSOUND *csound, SLIDER32f *p)
{
    SLIDERF(p, 32);
}

static int32_t slider_i64f(CSOUND *csound, SLIDER64f *p)
{
    SLIDERIF(p, 64);
}

static int32_t slider64f(CSOUND *csound, SLIDER64f *p)
{
    SLIDERF(p, 64);
}

/*===================================*/

#define ISLIDER(p, n)                                             \
{                                                                 \
    unsigned char chan= (unsigned char) ((*p->ichan)-1);          \
    char sbuf[120];                                               \
if (UNLIKELY(chan  > 15))  {                                      \
      return csound->InitError(csound, Str("illegal channel"));   \
    }                                                             \
    {                                                             \
      MYFLT value;                                                \
      int32_t j = 0;                                                  \
      ISLD *sld = p->s;                                           \
      unsigned char slnum;                                        \
      MYFLT *chanblock = (MYFLT *) csound->m_chnbp[chan]->ctl_val;\
      FUNC *ftp;                                                  \
      MYFLT **result = p->r;                                      \
                                                                  \
      while (j++ < n) {                                           \
        slnum=(unsigned char) *sld->ictlno;                       \
        if (UNLIKELY(slnum > 127)) {                              \
          snprintf(sbuf, 120, Str("illegal control number at position n.%d"), j); \
          return csound->InitError(csound, "%s", sbuf);                 \
        }                                                         \
        value = chanblock[slnum] * oneTOf7bit;                    \
        if (*sld->ifn > 0)  {                                     \
          ftp = csound->FTFind(csound, sld->ifn);              \
          value = *( ftp->ftable + (int32)(value * ftp->flen));   \
                                /* no interpolation */            \
        }                                                         \
        **result++ = value * (*sld->imax - *sld->imin) + *sld->imin; \
                                          /* scales the output */ \
        sld++;                                                    \
      }                                                           \
    }                                                             \
    return OK;                                                    \
}

static int32_t islider8(CSOUND *csound, ISLIDER8 *p)
{
    ISLIDER(p, 8);
}

static int32_t islider16(CSOUND *csound, ISLIDER16 *p)
{
    ISLIDER(p, 16);
}

static int32_t islider32(CSOUND *csound, ISLIDER32 *p)
{
    ISLIDER(p, 32);
}

static int32_t islider64(CSOUND *csound, ISLIDER64 *p)
{
    ISLIDER(p, 64);
}

/*-------------------------------*/

#define SLIDERI14(p, n)                                                \
{                                                                      \
    unsigned char chan= p->slchan = (unsigned char)((*p->ichan)-1);    \
    char sbuf[120];                                                    \
if (UNLIKELY(chan  > 15))  {                                           \
      return csound->InitError(csound, Str("illegal channel"));        \
    }                                                                  \
    {                                                                  \
      MYFLT value;                                                     \
      int32_t intvalue, j = 0;                                             \
      SLD14 *sld = p->s;                                               \
      unsigned char *slnum_msb = p->slnum_msb;                         \
      unsigned char *slnum_lsb = p->slnum_lsb;                         \
      MYFLT *min = p->min, *max= p->max;                               \
      FUNC **ftp = p->ftp;                                             \
      MYFLT *chanblock = (MYFLT *) csound->m_chnbp[chan]->ctl_val;     \
                                                                       \
      while (j++ < n) {                                                \
        *slnum_msb = (unsigned char)*sld->ictlno_msb;                  \
        if (UNLIKELY(*slnum_msb > 127)) {                              \
          snprintf(sbuf, 120,                                          \
                  Str("illegal msb control number at position n.%d"),  \
                  j);                                                  \
          return csound->InitError(csound, "%s", sbuf);                \
        }                                                              \
        *slnum_lsb = (unsigned char)*sld->ictlno_lsb;                  \
        if (UNLIKELY(*slnum_lsb > 127)) {                              \
          snprintf(sbuf, 120,                                          \
                  Str("illegal lsb control number at position n.%d"),  \
                  j);                                                  \
          return csound->InitError(csound, "%s", sbuf);                \
        }                                                              \
        if (UNLIKELY((value=*sld->initvalue) < (*min=*sld->imin) ||    \
                     value > (*max=*sld->imax) )) {                    \
          snprintf(sbuf, 120,                                          \
                  Str("illegal initvalue at position n.%d"), j);       \
          return csound->InitError(csound, "%s", sbuf);                \
        }                                                              \
        if (*sld->ifn > 0)   *ftp++ = csound->FTFind(csound, sld->ifn); \
        else                 *ftp++ = NULL;                            \
        intvalue = (int32_t) (((*(sld++)->initvalue - *min) / (*max++ - *min)) \
                          * f14bit+FL(0.5));                           \
        min++;                                                         \
        chanblock[*slnum_msb++] =  (MYFLT) (intvalue >> 7);            \
        chanblock[*slnum_lsb++] =  (MYFLT) (intvalue & 0x7f);          \
      }                                                                \
    }                                                                  \
    return OK;                                                         \
}

#define SLIDER14(p, n)                                                 \
{                                                                      \
    MYFLT value = FL(0.0);                                             \
    int32_t j = 0;                                                         \
    FUNC **ftp = p->ftp-1;                                             \
    MYFLT *chanblock = (MYFLT *) csound->m_chnbp[p->slchan]->ctl_val;  \
    unsigned char  *slnum_msb = p->slnum_msb;                          \
    unsigned char  *slnum_lsb = p->slnum_lsb;                          \
    MYFLT *min = p->min, *max = p->max;                                \
    MYFLT **result = p->r;                                             \
                                                                       \
    while (j++ < n) {                                                  \
      value = (MYFLT)((chanblock[*slnum_msb++]  * 128                  \
                       + chanblock[*slnum_lsb++]) * oneTOf14bit);      \
      if (*(++ftp)) {      /* if valid ftable,use value as index   */  \
        MYFLT phase = value * (*ftp)->flen;                            \
        MYFLT *base = (*ftp)->ftable + (int32)(phase);                 \
        value = *base + (*(base+1) - *base) * (phase - (int32) phase); \
      }                                                                \
      **result++ = value * (*max++ - *min) + *min; /* scales the output */ \
      min++;                                                           \
    }                                                                  \
    return OK;                                                         \
}

static int32_t slider_i16bit14(CSOUND *csound, SLIDER16BIT14 *p)
{
    SLIDERI14(p, 16);
}

static int32_t slider16bit14(CSOUND *csound, SLIDER16BIT14 *p)
{
    SLIDER14(p, 16);
}

static int32_t slider_i32bit14(CSOUND *csound, SLIDER32BIT14 *p)
{
    SLIDERI14(p, 32);
}

static int32_t slider32bit14(CSOUND *csound, SLIDER32BIT14 *p)
{
    SLIDER14(p, 32);
}

/*--------------------------------*/
#define ISLIDER14(p, n)                                                \
{                                                                      \
    unsigned char chan = (unsigned char)((*p->ichan)-1);               \
    char sbuf[120];                                                    \
if (UNLIKELY(chan  > 15))  {                                           \
      return csound->InitError(csound, Str("illegal channel"));        \
    }                                                                  \
    {                                                                  \
      MYFLT value;                                                     \
      int32_t j = 0;                                                       \
      ISLD14 *sld = p->s;                                              \
      unsigned char slnum_msb;                                         \
      unsigned char slnum_lsb;                                         \
      MYFLT *chanblock = (MYFLT *) csound->m_chnbp[chan]->ctl_val;     \
      MYFLT **result = p->r;                                           \
                                                                       \
      while (j++ < n) {                                                \
        slnum_msb=(unsigned char)*sld->ictlno_msb;                     \
        if (UNLIKELY(slnum_msb > 127)) {                               \
          snprintf(sbuf, 120,                                          \
                  Str("illegal msb control number at position n.%d"),  \
                  j);                                                  \
          return csound->InitError(csound, "%s", sbuf);                \
        }                                                              \
        slnum_lsb=(unsigned char)*sld->ictlno_lsb;                     \
        if (UNLIKELY(slnum_lsb > 127)) {                               \
          snprintf(sbuf, 120,                                          \
                  Str("illegal lsb control number at position n.%d"),  \
                  j);                                                  \
          return csound->InitError(csound, "%s", sbuf);                \
        }                                                              \
                                                                       \
        value = (MYFLT)((chanblock[slnum_msb]  * 128                   \
                         + chanblock[slnum_lsb]) * oneTOf14bit);       \
        if (*sld->ifn > 0) {    /* linear interpolation routine */     \
          FUNC *ftp= csound->FTFind(csound, sld->ifn);              \
          MYFLT phase = value * ftp->flen;                             \
          MYFLT *base = ftp->ftable + (int32)(phase);                  \
          value = *base + (*(base + 1) - *base) * (phase - (int32) phase); \
        }                                                              \
                                /* scales the output */                \
        **result++ = value * (*sld->imax - *sld->imin) + *sld->imin;   \
        sld++;                                                         \
      }                                                                \
    }                                                                  \
    return OK;                                                         \
}

static int32_t islider16bit14(CSOUND *csound, ISLIDER16BIT14 *p)
{
    ISLIDER14(p, 16);
}

static int32_t islider32bit14(CSOUND *csound, ISLIDER32BIT14 *p)
{
    ISLIDER14(p, 32);
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "slider8.k", S(SLIDER8), 0, "kkkkkkkk",  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                              "iiiiiiii", (SUBR)slider_i8, (SUBR)slider8, NULL },
{ "slider8f", S(SLIDER8f), 0, "kkkkkkkk","iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiii",
                                        (SUBR)slider_i8f, (SUBR)slider8f, NULL },
{ "slider8.i", S(SLIDER8), 0, "iiiiiiii", "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
                                          (SUBR)islider8, NULL, NULL },
{ "slider16.k", S(SLIDER16), 0, "kkkkkkkkkkkkkkkk",
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiii",
                                        (SUBR)slider_i16, (SUBR)slider16, NULL },
{ "slider16f", S(SLIDER16f), 0, "kkkkkkkkkkkkkkkk",
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiii",
                                        (SUBR)slider_i16f, (SUBR)slider16f, NULL },
{ "slider16.i", S(SLIDER16), 0, "iiiiiiiiiiiiiiii",
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
                                        (SUBR)islider16, NULL, NULL       },
{ "slider32.k", S(SLIDER32),  0, "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk",
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiii",
                                        (SUBR)slider_i32, (SUBR)slider32, NULL  },
{ "slider32f", S(SLIDER32f), 0, "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk",
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiii",
                                        (SUBR)slider_i32f, (SUBR)slider32f, NULL },
{ "slider32.i", S(SLIDER32), 0, "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii",
                                        (SUBR)islider32, NULL, NULL  },
{ "slider64.k", S(SLIDER64), 0, "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
                              "kkkkkkkkkkkkkkkkkkk",
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiii",
                                        (SUBR)slider_i64, (SUBR)slider64, NULL  },
{ "slider64f", S(SLIDER64f), 0, "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
                                "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk",
                                        "iiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii",
                                        (SUBR)slider_i64f, (SUBR)slider64f, NULL },
{ "slider64.i", S(SLIDER64), 0, "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiii",
                                        (SUBR)islider64, NULL, NULL  },
{ "s16b14.k", S(SLIDER16BIT14), 0, "kkkkkkkkkkkkkkkk",
                                   "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                   "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                   "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
                                 (SUBR)slider_i16bit14, (SUBR)slider16bit14, NULL},
{ "s32b14.k", S(SLIDER32BIT14), 0, "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk",
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
                                 (SUBR)slider_i32bit14, (SUBR)slider32bit14, NULL},
{ "s16b14.i", S(ISLIDER16BIT14), 0, "iiiiiiiiiiiiiiii",
                                        "iiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiii",
                                        (SUBR)islider16bit14, NULL, NULL  },
{ "s32b14.i", S(ISLIDER32BIT14), 0, "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                        "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
                                        (SUBR)islider32bit14, NULL, NULL  }
};

int32_t midiops3_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int32_t
                                  ) (sizeof(localops) / sizeof(OENTRY)));
}

