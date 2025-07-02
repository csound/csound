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
#include "csoundCore.h"
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

 int32_t slider_i8(CSOUND *csound, SLIDER8 *p)
{
    SLIDERI_INIT(p, 8);
}

 int32_t slider8(CSOUND *csound, SLIDER8 *p)
{
    SLIDER_INIT(p, 8);
}

 int32_t slider_i16(CSOUND *csound, SLIDER16 *p)
{
    SLIDERI_INIT(p, 16);
}

 int32_t slider16(CSOUND *csound, SLIDER16 *p)
{
    SLIDER_INIT(p, 16);
}

 int32_t slider_i32(CSOUND *csound, SLIDER32 *p)
{
    SLIDERI_INIT(p, 32);
}

 int32_t slider32(CSOUND *csound, SLIDER32 *p)
{
    SLIDER_INIT(p, 32);
}

 int32_t slider_i64(CSOUND *csound, SLIDER64 *p)
{
    SLIDERI_INIT(p, 64);
}

 int32_t slider64(CSOUND *csound, SLIDER64 *p)
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

 int32_t slider_i8f(CSOUND *csound, SLIDER8f *p)
{
    SLIDERIF(p, 8);
}

 int32_t slider8f(CSOUND *csound, SLIDER8f *p)
{
    SLIDERF(p, 8);
}

 int32_t slider_i16f(CSOUND *csound, SLIDER16f *p)
{
    SLIDERIF(p, 16);
}

 int32_t slider16f(CSOUND *csound, SLIDER16f *p)
{
    SLIDERF(p, 16);
}

 int32_t slider_i32f(CSOUND *csound, SLIDER32f *p)
{
    SLIDERIF(p, 32);
}

 int32_t slider32f(CSOUND *csound, SLIDER32f *p)
{
    SLIDERF(p, 32);
}

 int32_t slider_i64f(CSOUND *csound, SLIDER64f *p)
{
    SLIDERIF(p, 64);
}

 int32_t slider64f(CSOUND *csound, SLIDER64f *p)
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

 int32_t islider8(CSOUND *csound, ISLIDER8 *p)
{
    ISLIDER(p, 8);
}

 int32_t islider16(CSOUND *csound, ISLIDER16 *p)
{
    ISLIDER(p, 16);
}

 int32_t islider32(CSOUND *csound, ISLIDER32 *p)
{
    ISLIDER(p, 32);
}

 int32_t islider64(CSOUND *csound, ISLIDER64 *p)
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

 int32_t slider_i16bit14(CSOUND *csound, SLIDER16BIT14 *p)
{
    SLIDERI14(p, 16);
}

 int32_t slider16bit14(CSOUND *csound, SLIDER16BIT14 *p)
{
    SLIDER14(p, 16);
}

 int32_t slider_i32bit14(CSOUND *csound, SLIDER32BIT14 *p)
{
    SLIDERI14(p, 32);
}

 int32_t slider32bit14(CSOUND *csound, SLIDER32BIT14 *p)
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

 int32_t islider16bit14(CSOUND *csound, ISLIDER16BIT14 *p)
{
    ISLIDER14(p, 16);
}

 int32_t islider32bit14(CSOUND *csound, ISLIDER32BIT14 *p)
{
    ISLIDER14(p, 32);
}

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
        if ((outftp = csound->FTFind(csound, p->ioutfn)) != NULL)    \
          p->outTable = outftp->ftable;                                 \
        while (j < n) {                                                 \
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
              if (*sld->ifn > 0)   *ftp = csound->FTFind(csound, sld->ifn); \
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



int32_t sliderTable_i8(CSOUND *csound, SLIDER8t *p) /* GAB */
{
    SLIDER_I_TABLE_INIT(p,8);
}

int32_t sliderTable8(CSOUND *csound, SLIDER8t *p) /* GAB */
{
    SLIDER_TABLE_INIT(p,8);

}
/*--------------------------------------------------------*/



 int32_t sliderTable_i16(CSOUND *csound, SLIDER16t *p) /* GAB */
{
    SLIDER_I_TABLE_INIT(p,16);
}

 int32_t sliderTable16(CSOUND *csound, SLIDER16t *p) /* GAB */
{
    SLIDER_TABLE_INIT(p,16);
}
/*--------------------------------------------------------*/



 int32_t sliderTable_i32(CSOUND *csound, SLIDER32t *p) /* GAB */
{
    SLIDER_I_TABLE_INIT(p,32);
}

 int32_t sliderTable32(CSOUND *csound, SLIDER32t *p) /* GAB */
{
    SLIDER_TABLE_INIT(p,32);
}
/*--------------------------------------------------------*/



 int32_t sliderTable_i64(CSOUND *csound, SLIDER64t *p) /* GAB */
{
    SLIDER_I_TABLE_INIT(p,64);
}

 int32_t sliderTable64(CSOUND *csound, SLIDER64t *p) /* GAB */
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
      if((outftp = csound->FTFind(csound, p->ioutfn)) != NULL)       \
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
          if (*sld->ifn > 0)   *ftp = csound->FTFind(csound, sld->ifn); \
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
                              CS_TPIDSR * CS_KSMPS)));             \
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



 int32_t sliderTable_i8f(CSOUND *csound, SLIDER8tf *p)
{
    SLIDER_I_TABLEF_INIT(p,8)
}

 int32_t sliderTable8f(CSOUND *csound, SLIDER8tf *p)
{
    SLIDER_TABLEF_INIT(p,8)
}
/*--------------------------------------------------------*/



 int32_t sliderTable_i16f(CSOUND *csound, SLIDER16tf *p)
{
    SLIDER_I_TABLEF_INIT(p,16)
}

 int32_t sliderTable16f(CSOUND *csound, SLIDER16tf *p)
{
    SLIDER_TABLEF_INIT(p,16)
}
/*--------------------------------------------------------*/


 int32_t sliderTable_i32f(CSOUND *csound, SLIDER32tf *p)
{
    SLIDER_I_TABLEF_INIT(p,32)
}

 int32_t sliderTable32f(CSOUND *csound, SLIDER32tf *p)
{
    SLIDER_TABLEF_INIT(p,32)
}
/*--------------------------------------------------------*/


 int32_t sliderTable_i64f(CSOUND *csound, SLIDER64tf *p)
{
    SLIDER_I_TABLEF_INIT(p,64)
}

 int32_t sliderTable64f(CSOUND *csound, SLIDER64tf *p)
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

 int32_t sliderKawai_i(CSOUND *csound, SLIDERKAWAI *p)
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
      if (*sld->ifn > 0)   *ftp++ = csound->FTFind(csound, sld->ifn);
      else                 *ftp++ = NULL;
      value =  (*(sld++)->initvalue - *min) / (*max++ - *min);
      min++;
      csound->m_chnbp[j]->ctl_val[7] = (MYFLT)((int32_t)(value * f7bit + FL(0.5)));
    } while (++j < n);
    return OK;
}

 int32_t sliderKawai(CSOUND *csound, SLIDERKAWAI *p)
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

 int32_t ctrl7a_set(CSOUND *csound, CTRL7a *p)
{
    int32_t ctlno, chan;
    MYFLT   cutoff, b;


    if ((ctlno = (int32_t) *p->ictlno) < 0 || ctlno > 127)
      return csound->InitError(csound, Str("illegal controller number"));
    else if ((chan=(int32_t) *p->ichan-1) < 0 || chan > 15)
      return csound->InitError(csound, Str("illegal midi channel"));
    else p->ctlno = ctlno;

    if (*p->ifn > 0) {
      if (UNLIKELY(((p->ftp = csound->FTFind(csound, p->ifn)) == NULL)))
        p->flag = 0;  /* invalid ftable */
      else p->flag= 1;
    }
    else p->flag= 0;

    p->yt1 = FL(0.0);
    if (*p->icutoff <= 0) cutoff = 5;
    else cutoff = *p->icutoff;

    b = FL(2.0) - COS(cutoff * CS_TPIDSR * CS_KSMPS);
    p->c2 = b - SQRT(b * b - 1.0);
    p->c1 = FL(1.0) - p->c2;
    p->prev = 0;
    return OK;
}

 int32_t ctrl7a(CSOUND *csound, CTRL7a *p)
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




