/*
    midiops2.c:

    Copyright (C) 1997 Gabriel Maldonado
              (C) 2006 Istvan Varga

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

/****************************************/
/** midicXX   UGs by Gabriel Maldonado **/
/****************************************/
#include "csoundCore.h"
#include "midiops2.h"

#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

#define f7bit   (FL(127.0))
#define f14bit  (FL(16383.0))
#define f21bit  (FL(2097151.0))

#define oneTOf7bit       ((MYFLT)1./127.)
#define oneTOf14bit      ((MYFLT)1./16383.)
#define oneTOf21bit      ((MYFLT)1./2097151.)

/*------------------------------------------------------------------------*/
/* 7 bit midi control UGs */

 int32_t imidic7(CSOUND *csound, MIDICTL2 *p)
{
    MYFLT value;
    FUNC  *ftp;
    int32  ctlno;

    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    if (UNLIKELY((ctlno = (int32)*p->ictlno) < 0 || ctlno > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else {
      value = (MYFLT)(csound->curip->m_chnbp->ctl_val[ctlno] * oneTOf7bit);
      if (*p->ifn > 0) {
        if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))
          return NOTOK; /* if valid ftable, use value as index   */
        /* clamp it */
        value = value >= FL(0.0) ? (value <= 1.0 ? value : FL(1.0)) : FL(0.0);
        value = ftp->ftable[(int32)(value*(ftp->flen-1))]; /* no interpolation */
      }
      *p->r = value * (*p->imax - *p->imin) + *p->imin; /* scales the output*/
    }
    return OK;
}

 int32_t midic7set(CSOUND *csound, MIDICTL2 *p)
{
    int32  ctlno;
    if (UNLIKELY((ctlno = (int32)*p->ictlno) < 0 || ctlno > 127)) {
      return csound->InitError(csound, Str("illegal controller number"));
    }
    else p->ctlno = ctlno;
    if (*p->ifn > 0) {
      if (((p->ftp = csound->FTFind(csound, p->ifn)) == NULL))
        p->flag = FALSE;  /* invalid ftable */
      else p->flag= TRUE;
    }
    else p->flag= FALSE;
    return OK;
}

 int32_t midic7(CSOUND *csound, MIDICTL2 *p)
{
    IGN(csound);
    MYFLT value;
    INSDS *lcurip = p->h.insdshead;

    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    value = (MYFLT) (lcurip->m_chnbp->ctl_val[p->ctlno] * oneTOf7bit);
    if (p->flag)  {             /* if valid ftable,use value as index   */
         /* clamp it */
        value = value >= FL(0.0) ? (value <= 1.0 ? value : FL(1.0)) : FL(0.0);
        value = p->ftp->ftable[(int32)(value*(p->ftp->flen-1))]; /* no interpolation */
    }
    *p->r = value * (*p->imax - *p->imin) + *p->imin;   /* scales the output */
    return OK;
}

/*------------------------------------------------------------------------*/
/* 14 bit midi control UGs */

 int32_t imidic14(CSOUND *csound, MIDICTL3 *p)
{
    MYFLT value;
    FUNC  *ftp;
    int32  ctlno1;
    int32  ctlno2;

    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    if (UNLIKELY((ctlno1 = (int32)*p->ictlno1) < 0 || ctlno1 > 127 ||
                 (ctlno2 = (int32)*p->ictlno2) < 0 || ctlno2 > 127 ))
      return csound->InitError(csound, Str("illegal controller number"));
    else {
      value = (MYFLT) ((csound->curip->m_chnbp->ctl_val[ctlno1] * 128 +
                        csound->curip->m_chnbp->ctl_val[ctlno2])
                       * oneTOf14bit);
      if (*p->ifn > 0) {
        /* linear interpolation routine */
        MYFLT phase;
        MYFLT base;
        MYFLT top;
        MYFLT diff;

        if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))
          return NOTOK; /* if valid ftable,use value as index   */

        /* clamp it */
        value = value >= FL(0.0) ? (value <= 1.0 ? value : FL(1.0)) : FL(0.0);
        phase = value * (ftp->flen - 1);
        diff = phase - (int32) phase;
        base = ftp->ftable[(int32) phase];
        top  = ftp->ftable[(int32) phase + 1];
        /* oddity - it doesn't use the table guard point */
        value = phase < ftp->flen ?
          base + (top - base) * diff : base;
      }
      *p->r = value * (*p->imax - *p->imin) + *p->imin;  /* scales the output*/
    }
    return OK;
}

 int32_t midic14set(CSOUND *csound, MIDICTL3 *p)
{
    int32   ctlno1;
    int32   ctlno2;
    if (UNLIKELY((ctlno1 = (int32)*p->ictlno1) < 0 || ctlno1 > 127 ||
                 (ctlno2 = (int32)*p->ictlno2) < 0 || ctlno2 > 127 )) {
      return csound->InitError(csound, Str("illegal controller number"));
    }
    p->ctlno1 = ctlno1;
    p->ctlno2 = ctlno2;
    if (*p->ifn > 0) {
      if (UNLIKELY(((p->ftp = csound->FTFind(csound, p->ifn)) == NULL)))
        p->flag = FALSE;  /* invalid ftable */
      else p->flag= TRUE;
    }
    else
      p->flag= FALSE;
    return OK;
}

 int32_t midic14(CSOUND *csound, MIDICTL3 *p)
{
     IGN(csound);
    MYFLT value;
    INSDS *lcurip = p->h.insdshead;

    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    value =     (MYFLT) ((lcurip->m_chnbp->ctl_val[p->ctlno1] *128  +
                          lcurip->m_chnbp->ctl_val[p->ctlno2] )
                         * oneTOf14bit);
    if (p->flag)  {     /* if valid ftable,use value as index   */
      MYFLT phase, tmp, *tab =  p->ftp->ftable;
      /* clamp it */
      value = value >= FL(0.0) ? (value <= 1.0 ? value : FL(1.0)) : FL(0.0);
      phase = value * (p->ftp->flen - 1); /* gab-A1 */
      /* but here it does use the guard point */
      tmp = tab[(int32)phase];
      value = tmp + (tab[(int32)phase+1] - tmp) * (phase - (int32) phase);
    }
    *p->r = value * (*p->imax - *p->imin) + *p->imin;   /* scales the output */
    return OK;
}

/*-----------------------------------------------------------------------------*/
/* 21 bit midi control UGs */

 int32_t imidic21(CSOUND *csound, MIDICTL4 *p)
{
    MYFLT value;
    int32   ctlno1;
    int32   ctlno2;
    int32   ctlno3;

    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    if (UNLIKELY((ctlno1 = (int32)*p->ictlno1) < 0 || ctlno1 > 127 ||
        (ctlno2 = (int32)*p->ictlno2) < 0 || ctlno2 > 127 ||
                 (ctlno3 = (int32)*p->ictlno3) < 0 || ctlno3 > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else {
      value = (MYFLT) ((csound->curip->m_chnbp->ctl_val[ctlno1] * 16384 +
                        csound->curip->m_chnbp->ctl_val[ctlno2] * 128   +
                        csound->curip->m_chnbp->ctl_val[ctlno3])
                       * oneTOf21bit);
      if (*p->ifn > 0) {
        /* linear interpolation routine */
        FUNC *ftp = csound->FTFind(csound, p->ifn); /* gab-A1 */
        MYFLT phase, tmp, *tab;
         if (UNLIKELY(ftp == NULL))
          return csound->InitError(csound, Str("Invalid ftable no. %f"),
                                   *p->ifn);
         tab = ftp->ftable;
      /* clamp it */
      value = value >= FL(0.0) ? (value <= 1.0 ? value : FL(1.0)) : FL(0.0);
      phase = value * (p->ftp->flen - 1); /* gab-A1 */
      /* but here it also does use the guard point */
      tmp = tab[(int32)phase];
      value = tmp + (tab[(int32)phase+1] - tmp) * (phase - (int32) phase);

      }
      *p->r = value * (*p->imax - *p->imin) + *p->imin;  /* scales the output*/
    }
    return OK;
}

 int32_t midic21set(CSOUND *csound, MIDICTL4 *p)
{
    int32   ctlno1;
    int32   ctlno2;
    int32   ctlno3;
    if (UNLIKELY((ctlno1 = (int32)*p->ictlno1) < 0 || ctlno1 > 127 ||
        (ctlno2 = (int32)*p->ictlno2) < 0 || ctlno2 > 127 ||
                 (ctlno3 = (int32)*p->ictlno3) < 0 || ctlno3 > 127)) {
      return csound->InitError(csound, Str("illegal controller number"));
    }
    p->ctlno1 = ctlno1;
    p->ctlno2 = ctlno2;
    p->ctlno3 = ctlno3;
    if (*p->ifn > 0) {
      if (UNLIKELY(((p->ftp = csound->FTFind(csound, p->ifn)) == NULL)))
        p->flag = FALSE;  /* invalid ftable */
      else
        p->flag= TRUE;
    }
    else
      p->flag= FALSE;
    return OK;
}

 int32_t midic21(CSOUND *csound, MIDICTL4 *p)
{
     IGN(csound);
    MYFLT value;
    INSDS *lcurip = p->h.insdshead;

    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    value = (MYFLT)((lcurip->m_chnbp->ctl_val[p->ctlno1] * 16384 +
                     lcurip->m_chnbp->ctl_val[p->ctlno2] * 128   +
                     lcurip->m_chnbp->ctl_val[p->ctlno3] )  * oneTOf21bit);
    if (p->flag)  {     /* if valid ftable,use value as index   */
      /* linear interpolation routine */
        MYFLT phase, tmp, *tab = p->ftp->ftable;
      /* clamp it */
      value = value >= FL(0.0) ? (value <= 1.0 ? value : FL(1.0)) : FL(0.0);
      phase = value * (p->ftp->flen - 1); /* gab-A1 */
      /* but here it also does use the guard point */
      tmp = tab[(int32)phase];
      value = tmp + (tab[(int32)phase+1] - tmp) * (phase - (int32) phase);
    }
    *p->r = value * (*p->imax - *p->imin) + *p->imin;   /* scales the output */
    return OK;
}

/*-----------------------------------------------------------------*/
/* GLOBAL MIDI IN CONTROLS activable by score-oriented instruments*/
/*-----------------------------------------------------------------*/

 int32_t ictrl7(CSOUND *csound, CTRL7 *p)
{
    MYFLT value;
    FUNC *ftp;
    int32  ctlno;

    if (UNLIKELY((ctlno = (int32)*p->ictlno) < 0 || ctlno > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else {
      value = (MYFLT) (csound->m_chnbp[(int32_t) *p->ichan-1]->ctl_val[ctlno]
                       * oneTOf7bit);
      if (*p->ifn > 0) {
        if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))
          return NOTOK;               /* if valid ftable,use value as index   */
                /* clamp it */
        value = value >= FL(0.0) ? (value <= 1.0 ? value : FL(1.0)) : FL(0.0);
        value = ftp->ftable[(int32)(value*(ftp->flen-1))]; /* no interpolation */
      }
      *p->r = value * (*p->imax - *p->imin) + *p->imin;  /* scales the output*/
    }
    return OK;
}

 int32_t ctrl7set(CSOUND *csound, CTRL7 *p)
{
    int32  ctlno;
    int32_t chan;
    if (UNLIKELY((ctlno = (int32) *p->ictlno) < 0 || ctlno > 127)) {
      return csound->InitError(csound, Str("illegal controller number"));
    }
    else if (UNLIKELY((chan=(int32_t) *p->ichan-1) < 0 || chan > 1023)) {
      return csound->InitError(csound,
                     Str("illegal midi channel")); /* gab-A2 (chan number fix)*/
    }
    /*else if (midi_in_p_num < 0) midi_in_error("ctrl7");*/
    else p->ctlno = ctlno;
    if (*p->ifn > 0) {
      if (UNLIKELY(((p->ftp = csound->FTFind(csound, p->ifn)) == NULL)))
        p->flag = FALSE;  /* invalid ftable */
      else p->flag= TRUE;
    }
    else p->flag= FALSE;
    return OK;
}

 int32_t ctrl7(CSOUND *csound, CTRL7 *p)
{
    MYFLT value = (MYFLT) (csound->m_chnbp[(int32_t) *p->ichan-1]->ctl_val[p->ctlno]
                           * oneTOf7bit);
    if (p->flag)  {             /* if valid ftable,use value as index   */
              /* clamp it */
        value = value >= FL(0.0) ? (value <= 1.0 ? value : FL(1.0)) : FL(0.0);
        value = p->ftp->ftable[(int32)(value*(p->ftp->flen-1))]; /* no interpolation */
    }
    *p->r = value * (*p->imax - *p->imin) + *p->imin;   /* scales the output */
    return OK;
}

/* 14 bit midi control UGs */

 int32_t ictrl14(CSOUND *csound, CTRL14 *p)
{
    MYFLT value;
    int32  ctlno1;
    int32  ctlno2;
    int32_t chan;

    if (UNLIKELY((ctlno1 = (int32)*p->ictlno1) < 0 || ctlno1 > 127 ||
                 (ctlno2 = (int32)*p->ictlno2) < 0 || ctlno2 > 127 ))
      return csound->InitError(csound, Str("illegal controller number"));
    else if (UNLIKELY((chan=(int32_t) *p->ichan-1) < 0 || chan > 15))
      return csound->InitError(csound, Str("illegal midi channel"));
    else {
      value = (MYFLT)((csound->m_chnbp[chan]->ctl_val[ctlno1] * 128 +
                       csound->m_chnbp[chan]->ctl_val[ctlno2]) * oneTOf14bit);

      if (*p->ifn > 0) {
        /* linear interpolation routine */
        /* linear interpolation routine */
        FUNC *ftp = csound->FTFind(csound, p->ifn); /* gab-A1 */
        MYFLT phase, tmp, *tab;
         if (UNLIKELY(ftp == NULL))
          return csound->InitError(csound, Str("Invalid ftable no. %f"),
                                   *p->ifn);
         tab = ftp->ftable;
      /* clamp it */
      value = value >= FL(0.0) ? (value <= 1.0 ? value : FL(1.0)) : FL(0.0);
      phase = value * (p->ftp->flen - 1); /* gab-A1 */
      /* but here it also does use the guard point */
      tmp = tab[(int32)phase];
      value = tmp + (tab[(int32)phase+1] - tmp) * (phase - (int32) phase);
      }
      *p->r = value * (*p->imax - *p->imin) + *p->imin;  /* scales the output*/
    }
    return OK;
}

 int32_t ctrl14set(CSOUND *csound, CTRL14 *p)
{
    int32   ctlno1;
    int32   ctlno2;
    int32_t chan;
    if (UNLIKELY((ctlno1 = (int32)*p->ictlno1) < 0 || ctlno1 > 127 ||
                 (ctlno2 = (int32)*p->ictlno2) < 0 || ctlno2 > 127 )) {
      return csound->InitError(csound, Str("illegal controller number"));
    }
    else if (UNLIKELY((chan=(int32_t) *p->ichan-1) < 0 || chan > 1023)) {
      return csound->InitError(csound, Str("illegal midi channel"));
    }
    p->ctlno1 = ctlno1;
    p->ctlno2 = ctlno2;
    if (*p->ifn > 0) {
      if (UNLIKELY(((p->ftp = csound->FTFind(csound, p->ifn)) == NULL)))
        p->flag = FALSE;  /* invalid ftable */
      else p->flag= TRUE;
    }
    else
      p->flag= FALSE;
    return OK;
}

 int32_t ctrl14(CSOUND *csound, CTRL14 *p)
{
    MYFLT value;
    int32_t chan=(int32_t) *p->ichan-1;

    value = (MYFLT)((csound->m_chnbp[chan]->ctl_val[p->ctlno1] * 128 +
                     csound->m_chnbp[chan]->ctl_val[p->ctlno2]) * oneTOf14bit);

    if (p->flag)  {             /* if valid ftable,use value as index   */
                                /* linear interpolation routine */
       /* linear interpolation routine */
        MYFLT phase, tmp, *tab = p->ftp->ftable;
      /* clamp it */
      value = value >= FL(0.0) ? (value <= 1.0 ? value : FL(1.0)) : FL(0.0);
      phase = value * (p->ftp->flen - 1); /* gab-A1 */
      /* but here it also does use the guard point */
      tmp = tab[(int32)phase];
      value = tmp + (tab[(int32)phase+1] - tmp) * (phase - (int32) phase);

    }
    *p->r = value * (*p->imax - *p->imin) + *p->imin;   /* scales the output */
    return OK;
}

/*-----------------------------------------------------------------------------*/
/* 21 bit midi control UGs */

 int32_t ictrl21(CSOUND *csound, CTRL21 *p)
{
    MYFLT  value;
    int32   ctlno1;
    int32   ctlno2;
    int32   ctlno3;
    int32_t chan;

    if (UNLIKELY((ctlno1 = (int32)*p->ictlno1) < 0 || ctlno1 > 127 ||
        (ctlno2 = (int32)*p->ictlno2) < 0 || ctlno2 > 127 ||
                 (ctlno3 = (int32)*p->ictlno3) < 0 || ctlno3 > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else if (UNLIKELY((chan=(int32_t) *p->ichan-1) < 0 || chan > 1023))
      return csound->InitError(csound, Str("illegal midi channel"));
    else {
      value = (MYFLT)((csound->m_chnbp[chan]->ctl_val[ctlno1] * 16384 +
                       csound->m_chnbp[chan]->ctl_val[ctlno2] * 128   +
                       csound->m_chnbp[chan]->ctl_val[ctlno3]) * oneTOf21bit);

      if (*p->ifn > 0) {
        /* linear interpolation routine */
        FUNC *ftp = csound->FTFind(csound, p->ifn); /* gab-A1 */
        MYFLT phase, tmp, *tab;
         if (UNLIKELY(ftp == NULL))
          return csound->InitError(csound, Str("Invalid ftable no. %f"),
                                   *p->ifn);
         tab = ftp->ftable;
      /* clamp it */
      value = value >= FL(0.0) ? (value <= 1.0 ? value : FL(1.0)) : FL(0.0);
      phase = value * (p->ftp->flen - 1); /* gab-A1 */
      /* but here it also does use the guard point */
      tmp = tab[(int32)phase];
      value = tmp + (tab[(int32)phase+1] - tmp) * (phase - (int32) phase);
      }
      *p->r = value * (*p->imax - *p->imin) + *p->imin;  /* scales the output*/
    }
    return OK;
}

 int32_t ctrl21set(CSOUND *csound, CTRL21 *p)
{
    int32   ctlno1;
    int32   ctlno2;
    int32   ctlno3;
    int32_t chan;
    if (UNLIKELY((ctlno1 = (int32)*p->ictlno1) < 0 || ctlno1 > 127 ||
        (ctlno2 = (int32)*p->ictlno2) < 0 || ctlno2 > 127 ||
                 (ctlno3 = (int32)*p->ictlno3) < 0 || ctlno3 > 127)) {
      return csound->InitError(csound, Str("illegal controller number"));
    }
    else if (UNLIKELY((chan=(int32_t) *p->ichan-1) < 0 || chan > 1023)) {
      return csound->InitError(csound, Str("illegal midi channel"));
    }
    p->ctlno1 = ctlno1;
    p->ctlno2 = ctlno2;
    p->ctlno3 = ctlno3;
    if (*p->ifn > 0) {
      if (UNLIKELY(((p->ftp = csound->FTFind(csound, p->ifn)) == NULL)))
        p->flag = FALSE;  /* invalid ftable */
      else
        p->flag= TRUE;
    }
    else  p->flag= FALSE;
    return OK;
}

 int32_t ctrl21(CSOUND *csound, CTRL21 *p)
{
    MYFLT value;
    int32_t chan=(int32_t) *p->ichan-1;
    value = (csound->m_chnbp[chan]->ctl_val[p->ctlno1] * 16384 +
             csound->m_chnbp[chan]->ctl_val[p->ctlno2] * 128   +
             csound->m_chnbp[chan]->ctl_val[p->ctlno3]) / f21bit;

    if (p->flag)  {     /* if valid ftable,use value as index   */
        /* linear interpolation routine */
        MYFLT phase, tmp, *tab = p->ftp->ftable;
      /* clamp it */
      value = value >= FL(0.0) ? (value <= 1.0 ? value : FL(1.0)) : FL(0.0);
      phase = value * (p->ftp->flen - 1); /* gab-A1 */
      /* but here it also does use the guard point */
      tmp = tab[(int32)phase];
      value = tmp + (tab[(int32)phase+1] - tmp) * (phase - (int32) phase);
    }
    *p->r = value * (*p->imax - *p->imin) + *p->imin;   /* scales the output */
    return OK;
}

 int32_t initc7(CSOUND *csound, INITC7 *p)
                   /* for setting a precise value use the following formula:*/
{                  /* (value - min) / (max - min) */
    MYFLT fvalue;
    int32_t chan;
    if (UNLIKELY((fvalue = *p->ivalue) < 0. || fvalue > 1. ))
      return csound->InitError(csound, Str("value out of range"));
    else if (UNLIKELY((chan = (int32_t) *p->ichan-1) < 0 || chan > 1023 ||
                      !csound->m_chnbp[chan]))
      return csound->InitError(csound, Str("illegal midi channel"));
    else
      csound->m_chnbp[chan]->ctl_val[(int32_t) *p->ictlno] = fvalue * f7bit
                                                         + FL(0.5);
    return OK;
}

 int32_t initc14(CSOUND *csound, INITC14 *p)
{
    MYFLT fvalue;
    int32_t value, msb, lsb, chan;
    if (UNLIKELY((fvalue = *p->ivalue) < FL(0.0) || fvalue > FL(1.0) ))
      return csound->InitError(csound, Str("value out of range"));
    else if (UNLIKELY((chan = (int32_t) *p->ichan - 1) < 0 || chan > 1023 ||
                      !csound->m_chnbp[chan]))
      return csound->InitError(csound, Str("illegal midi channel"));
    else {
      value = (int32_t)MYFLT2LONG(fvalue * f14bit);
      msb = value >> 7;
      lsb = value & 0x7F;
      csound->m_chnbp[chan]->ctl_val[(int32_t) *p->ictlno1] = (MYFLT)msb;
      csound->m_chnbp[chan]->ctl_val[(int32_t) *p->ictlno2] = (MYFLT)lsb;
    }
    return OK;
}

 int32_t initc21(CSOUND *csound, INITC21 *p)
{
    MYFLT fvalue;
    int32_t value, msb, xsb, lsb, chan;
    if (UNLIKELY((fvalue = *p->ivalue) < FL(0.0) || fvalue > FL(1.0) ))
      return csound->InitError(csound, Str("value out of range"));
    else if (UNLIKELY((chan = (int32_t) *p->ichan - 1) < 0 || chan > 1023 ||
                      !csound->m_chnbp[chan]))
      return csound->InitError(csound, Str("illegal midi channel"));
    else {
      value = (int32_t)MYFLT2LONG(fvalue * f21bit);
      msb = value >> 14;
      xsb = (value >> 7) & 0x7F;
      lsb = value & 0x7F;
      csound->m_chnbp[chan]->ctl_val[(int32_t) *p->ictlno1] = (MYFLT)msb;
      csound->m_chnbp[chan]->ctl_val[(int32_t) *p->ictlno2] = (MYFLT)xsb;
      csound->m_chnbp[chan]->ctl_val[(int32_t) *p->ictlno3] = (MYFLT)lsb;
    }
    return OK;
}

/* midipgm by Istvan Varga, 2006 */
 int32_t midipgm_opcode(CSOUND *csound, MIDIPGM_OP *p)
{
    MCHNBLK *chnp;
    int32_t     channelNum;

    *(p->ipgm) = FL(0.0);
    channelNum = (int32_t) MYFLT2LONG(*(p->ichn));
    if (channelNum > 0) {
      if (UNLIKELY(channelNum > 1024))
        return csound->InitError(csound, Str("invalid channel number: %d"),
                                         channelNum);
      chnp = csound->m_chnbp[channelNum - 1];
    }
    else
      chnp = p->h.insdshead->m_chnbp;
    if (chnp != NULL) {
      if ((int32_t) chnp->pgmno >= 0)
        *(p->ipgm) = (MYFLT) ((int32_t) chnp->pgmno + 1);
    }
    return OK;
}


