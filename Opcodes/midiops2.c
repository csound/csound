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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

/****************************************/
/** midicXX   UGs by Gabriel Maldonado **/
/****************************************/

#include "csdl.h"
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

static int imidic7(CSOUND *csound, MIDICTL2 *p)
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
        value = *(ftp->ftable + (int32)(value*ftp->flen)); /* no interpolation */
      }
      *p->r = value * (*p->imax - *p->imin) + *p->imin; /* scales the output*/
    }
    return OK;
}

static int midic7set(CSOUND *csound, MIDICTL2 *p)
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

static int midic7(CSOUND *csound, MIDICTL2 *p)
{
    MYFLT value;
    INSDS *lcurip = p->h.insdshead;

    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    value = (MYFLT) (lcurip->m_chnbp->ctl_val[p->ctlno] * oneTOf7bit);
    if (p->flag)  {             /* if valid ftable,use value as index   */
      value = *(p->ftp->ftable +
                (int32)(value*p->ftp->flen));            /* no interpolation */
    }
    *p->r = value * (*p->imax - *p->imin) + *p->imin;   /* scales the output */
    return OK;
}

/*------------------------------------------------------------------------*/
/* 14 bit midi control UGs */

static int imidic14(CSOUND *csound, MIDICTL3 *p)
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
        MYFLT *base_address;
        MYFLT *base;
        MYFLT *top;
        MYFLT diff;
        int32 length;

        if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))
          return NOTOK; /* if valid ftable,use value as index   */
        phase = value * (length = ftp->flen);
        diff = phase - (int32) phase;
        base = (base_address = ftp->ftable) + (int32)(phase);
        top  = base + 1 ;
        top = (top - base_address > length) ?  base_address : top;
        value = *base + (*top - *base) * diff;
      }
      *p->r = value * (*p->imax - *p->imin) + *p->imin;  /* scales the output*/
    }
    return OK;
}

static int midic14set(CSOUND *csound, MIDICTL3 *p)
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

static int midic14(CSOUND *csound, MIDICTL3 *p)
{
    MYFLT value;
    INSDS *lcurip = p->h.insdshead;

    if (!p->h.insdshead->m_chnbp) {
      return OK;
    }
    value =     (MYFLT) ((lcurip->m_chnbp->ctl_val[p->ctlno1] *128  +
                          lcurip->m_chnbp->ctl_val[p->ctlno2] )
                         * oneTOf14bit);
    if (p->flag)  {     /* if valid ftable,use value as index   */
      MYFLT phase = value * p->ftp->flen; /* gab-A1 */
      MYFLT *base = p->ftp->ftable + (int32)(phase);
      value = *base + (*(base+1) - *base) * (phase - (int32) phase);

      /* linear interpolation routine */
      /*
        MYFLT phase;
        MYFLT *base_address;
        MYFLT *base;
        MYFLT *top;
        MYFLT diff;
        int32 length;

        phase =  value * (length = p->ftp->flen);
        diff = phase - (int32) phase;
        base = (base_address = p->ftp->ftable) + (int32)(phase);
        top  = base + 1 ;
        top = (top - base_address > length) ?  base_address : top;
        value = *base + (*top - *base) * diff;
      */
    }
    *p->r = value * (*p->imax - *p->imin) + *p->imin;   /* scales the output */
    return OK;
}

/*-----------------------------------------------------------------------------*/
/* 21 bit midi control UGs */

static int imidic21(CSOUND *csound, MIDICTL4 *p)
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
        MYFLT phase;
        MYFLT *base;
        if (UNLIKELY(ftp == NULL))
          return csound->InitError(csound, Str("Invalid ftable no. %f"),
                                           *p->ifn);
        phase = value * ftp->flen;
        base = ftp->ftable + (int32)(phase);
        value = *base + (*(base+1) - *base) * (phase - (int32)phase);
      }
      *p->r = value * (*p->imax - *p->imin) + *p->imin;  /* scales the output*/
    }
    return OK;
}

static int midic21set(CSOUND *csound, MIDICTL4 *p)
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

static int midic21(CSOUND *csound, MIDICTL4 *p)
{
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
      MYFLT phase = value * p->ftp->flen;
      MYFLT *base = p->ftp->ftable + (int32)(phase);
      value = *base + (*(base+1) - *base) * (phase - (int32) phase);

      /*
        MYFLT phase;
        MYFLT *base_address;
        MYFLT *base;
        MYFLT *top;
        MYFLT diff;
        int32 length;

        phase = value * (length = p->ftp->flen);
        diff = phase - (int32) phase;
        base = (base_address = p->ftp->ftable) + (int32)(phase);
        top  = base + 1 ;
        top = (top - base_address > length) ?  base_address : top;
        value = *base + (*top - *base) * diff;
      */
    }
    *p->r = value * (*p->imax - *p->imin) + *p->imin;   /* scales the output */
    return OK;
}

/*-----------------------------------------------------------------*/
/* GLOBAL MIDI IN CONTROLS activable by score-oriented instruments*/
/*-----------------------------------------------------------------*/

static int ictrl7(CSOUND *csound, CTRL7 *p)
{
    MYFLT value;
    FUNC *ftp;
    int32  ctlno;

    if (UNLIKELY((ctlno = (int32)*p->ictlno) < 0 || ctlno > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else {
      value = (MYFLT) (csound->m_chnbp[(int) *p->ichan-1]->ctl_val[ctlno]
                       * oneTOf7bit);
      if (*p->ifn > 0) {
        if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))
          return NOTOK;               /* if valid ftable,use value as index   */
        value = *(ftp->ftable + (int32)(value*ftp->flen)); /* no interpolation */
      }
      *p->r = value * (*p->imax - *p->imin) + *p->imin;  /* scales the output*/
    }
    return OK;
}

static int ctrl7set(CSOUND *csound, CTRL7 *p)
{
    int32  ctlno;
    int chan;
    if (UNLIKELY((ctlno = (int32) *p->ictlno) < 0 || ctlno > 127)) {
      return csound->InitError(csound, Str("illegal controller number"));
    }
    else if (UNLIKELY((chan=(int) *p->ichan-1) < 0 || chan > 15)) {
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

static int ctrl7(CSOUND *csound, CTRL7 *p)
{
    MYFLT value = (MYFLT) (csound->m_chnbp[(int) *p->ichan-1]->ctl_val[p->ctlno]
                           * oneTOf7bit);
    if (p->flag)  {             /* if valid ftable,use value as index   */
      value =
        *(p->ftp->ftable + (int32)(value*p->ftp->flen)); /* no interpolation */
    }
    *p->r = value * (*p->imax - *p->imin) + *p->imin;   /* scales the output */
    return OK;
}

/* 14 bit midi control UGs */

static int ictrl14(CSOUND *csound, CTRL14 *p)
{
    MYFLT value;
    int32  ctlno1;
    int32  ctlno2;
    int chan;

    if (UNLIKELY((ctlno1 = (int32)*p->ictlno1) < 0 || ctlno1 > 127 ||
                 (ctlno2 = (int32)*p->ictlno2) < 0 || ctlno2 > 127 ))
      return csound->InitError(csound, Str("illegal controller number"));
    else if (UNLIKELY((chan=(int) *p->ichan-1) < 0 || chan > 15))
      return csound->InitError(csound, Str("illegal midi channel"));
    else {
      value = (MYFLT)((csound->m_chnbp[chan]->ctl_val[ctlno1] * 128 +
                       csound->m_chnbp[chan]->ctl_val[ctlno2]) * oneTOf14bit);

      if (*p->ifn > 0) {
        /* linear interpolation routine */
        FUNC *ftp = csound->FTFind(csound, p->ifn);
        MYFLT phase;
        MYFLT *base;
        if (UNLIKELY(ftp == NULL))
          return csound->InitError(csound, Str("Invalid ftable no. %f"),
                                           *p->ifn);
        phase = value * ftp->flen;
        base = ftp->ftable + (int32)(phase);
        value = *base + (*(base+1) - *base) * (phase - (int32)phase);
      }
      *p->r = value * (*p->imax - *p->imin) + *p->imin;  /* scales the output*/
    }
    return OK;
}

static int ctrl14set(CSOUND *csound, CTRL14 *p)
{
    int32   ctlno1;
    int32   ctlno2;
    int chan;
    if (UNLIKELY((ctlno1 = (int32)*p->ictlno1) < 0 || ctlno1 > 127 ||
                 (ctlno2 = (int32)*p->ictlno2) < 0 || ctlno2 > 127 )) {
      return csound->InitError(csound, Str("illegal controller number"));
    }
    else if (UNLIKELY((chan=(int) *p->ichan-1) < 0 || chan > 15)) {
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

static int ctrl14(CSOUND *csound, CTRL14 *p)
{
    MYFLT value;
    int chan=(int) *p->ichan-1;

    value = (MYFLT)((csound->m_chnbp[chan]->ctl_val[p->ctlno1] * 128 +
                     csound->m_chnbp[chan]->ctl_val[p->ctlno2]) * oneTOf14bit);

    if (p->flag)  {             /* if valid ftable,use value as index   */
                                /* linear interpolation routine */
       MYFLT phase = value * p->ftp->flen;
       MYFLT *base = p->ftp->ftable + (int32)(phase);
       value = *base + (*(base+1) - *base) * (phase - (int32) phase);

    }
    *p->r = value * (*p->imax - *p->imin) + *p->imin;   /* scales the output */
    return OK;
}

/*-----------------------------------------------------------------------------*/
/* 21 bit midi control UGs */

static int ictrl21(CSOUND *csound, CTRL21 *p)
{
    MYFLT  value;
    int32   ctlno1;
    int32   ctlno2;
    int32   ctlno3;
    int chan;

    if (UNLIKELY((ctlno1 = (int32)*p->ictlno1) < 0 || ctlno1 > 127 ||
        (ctlno2 = (int32)*p->ictlno2) < 0 || ctlno2 > 127 ||
                 (ctlno3 = (int32)*p->ictlno3) < 0 || ctlno3 > 127))
      return csound->InitError(csound, Str("illegal controller number"));
    else if (UNLIKELY((chan=(int) *p->ichan-1) < 0 || chan > 15))
      return csound->InitError(csound, Str("illegal midi channel"));
    else {
      value = (MYFLT)((csound->m_chnbp[chan]->ctl_val[ctlno1] * 16384 +
                       csound->m_chnbp[chan]->ctl_val[ctlno2] * 128   +
                       csound->m_chnbp[chan]->ctl_val[ctlno3]) * oneTOf21bit);

      if (*p->ifn > 0) {
        /* linear interpolation routine */
        FUNC *ftp = csound->FTFind(csound, p->ifn);
        MYFLT phase;
        MYFLT *base;
        if (UNLIKELY(ftp == NULL))
          return csound->InitError(csound, Str("Invalid ftable no. %f"),
                                           *p->ifn);
        phase = value * ftp->flen;
        base = ftp->ftable + (int32)(phase);
        value = *base + (*(base+1) - *base) * (phase - (int32)phase);
      }
      *p->r = value * (*p->imax - *p->imin) + *p->imin;  /* scales the output*/
    }
    return OK;
}

static int ctrl21set(CSOUND *csound, CTRL21 *p)
{
    int32   ctlno1;
    int32   ctlno2;
    int32   ctlno3;
    int chan;
    if (UNLIKELY((ctlno1 = (int32)*p->ictlno1) < 0 || ctlno1 > 127 ||
        (ctlno2 = (int32)*p->ictlno2) < 0 || ctlno2 > 127 ||
                 (ctlno3 = (int32)*p->ictlno3) < 0 || ctlno3 > 127)) {
      return csound->InitError(csound, Str("illegal controller number"));
    }
    else if (UNLIKELY((chan=(int) *p->ichan-1) < 0 || chan > 15)) {
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

static int ctrl21(CSOUND *csound, CTRL21 *p)
{
    MYFLT value;
    int chan=(int) *p->ichan-1;
    value = (csound->m_chnbp[chan]->ctl_val[p->ctlno1] * 16384 +
             csound->m_chnbp[chan]->ctl_val[p->ctlno2] * 128   +
             csound->m_chnbp[chan]->ctl_val[p->ctlno3]) / f21bit;

    if (p->flag)  {     /* if valid ftable,use value as index   */
        /* linear interpolation routine */
       MYFLT phase = value * p->ftp->flen;
       MYFLT *base = p->ftp->ftable + (int32)(phase);
       value = *base + (*(base+1) - *base) * (phase - (int32) phase);
    }
    *p->r = value * (*p->imax - *p->imin) + *p->imin;   /* scales the output */
    return OK;
}

static int initc7(CSOUND *csound, INITC7 *p)
                   /* for setting a precise value use the following formula:*/
{                  /* (value - min) / (max - min) */
    MYFLT fvalue;
    int chan;
    if (UNLIKELY((fvalue = *p->ivalue) < 0. || fvalue > 1. ))
      return csound->InitError(csound, Str("value out of range"));
    else if (UNLIKELY((chan = (int) *p->ichan-1) < 0 || chan > 15 ||
                      !csound->m_chnbp[chan]))
      return csound->InitError(csound, Str("illegal midi channel"));
    else
      csound->m_chnbp[chan]->ctl_val[(int) *p->ictlno] = fvalue * f7bit
                                                         + FL(0.5);
    return OK;
}

static int initc14(CSOUND *csound, INITC14 *p)
{
    MYFLT fvalue;
    int value, msb, lsb, chan;
    if (UNLIKELY((fvalue = *p->ivalue) < FL(0.0) || fvalue > FL(1.0) ))
      return csound->InitError(csound, Str("value out of range"));
    else if (UNLIKELY((chan = (int) *p->ichan - 1) < 0 || chan > 15 ||
                      !csound->m_chnbp[chan]))
      return csound->InitError(csound, Str("illegal midi channel"));
    else {
      value = (int)MYFLT2LONG(fvalue * f14bit);
      msb = value >> 7;
      lsb = value & 0x7F;
      csound->m_chnbp[chan]->ctl_val[(int) *p->ictlno1] = (MYFLT)msb;
      csound->m_chnbp[chan]->ctl_val[(int) *p->ictlno2] = (MYFLT)lsb;
    }
    return OK;
}

static int initc21(CSOUND *csound, INITC21 *p)
{
    MYFLT fvalue;
    int value, msb, xsb, lsb, chan;
    if (UNLIKELY((fvalue = *p->ivalue) < FL(0.0) || fvalue > FL(1.0) ))
      return csound->InitError(csound, Str("value out of range"));
    else if (UNLIKELY((chan = (int) *p->ichan - 1) < 0 || chan > 15 ||
                      !csound->m_chnbp[chan]))
      return csound->InitError(csound, Str("illegal midi channel"));
    else {
      value = (int)MYFLT2LONG(fvalue * f21bit);
      msb = value >> 14;
      xsb = (value >> 7) & 0x7F;
      lsb = value & 0x7F;
      csound->m_chnbp[chan]->ctl_val[(int) *p->ictlno1] = (MYFLT)msb;
      csound->m_chnbp[chan]->ctl_val[(int) *p->ictlno2] = (MYFLT)xsb;
      csound->m_chnbp[chan]->ctl_val[(int) *p->ictlno3] = (MYFLT)lsb;
    }
    return OK;
}

/* midipgm by Istvan Varga, 2006 */

typedef struct MIDIPGM_OP_ {
    OPDS    h;
    MYFLT   *ipgm, *ichn;
} MIDIPGM_OP;

static int midipgm_opcode(CSOUND *csound, MIDIPGM_OP *p)
{
    MCHNBLK *chnp;
    int     channelNum;

    *(p->ipgm) = FL(0.0);
    channelNum = (int) MYFLT2LONG(*(p->ichn));
    if (channelNum > 0) {
      if (UNLIKELY(channelNum > 16))
        return csound->InitError(csound, Str("invalid channel number: %d"),
                                         channelNum);
      chnp = csound->m_chnbp[channelNum - 1];
    }
    else
      chnp = p->h.insdshead->m_chnbp;
    if (chnp != NULL) {
      if ((int) chnp->pgmno >= 0)
        *(p->ipgm) = (MYFLT) ((int) chnp->pgmno + 1);
    }
    return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
{ "ctrl14", 0xffff,                                                     },
{ "ctrl21", 0xffff,                                                     },
{ "ctrl7", 0xffff,                                                      },
{ "midic14", 0xffff,                                                    },
{ "midic21", 0xffff,                                                    },
{ "midic7", 0xffff,                                                     },
{ "midic7.i",S(MIDICTL2),1,  "i", "iiio", (SUBR)imidic7,   NULL,     NULL },
{ "midic7.k", S(MIDICTL2),3, "k", "ikko", (SUBR)midic7set, (SUBR)midic7,  NULL },
{ "midic14.i", S(MIDICTL3), 1,"i", "iiiio",(SUBR)imidic14,   NULL,     NULL },
{ "midic14.k", S(MIDICTL3), 3,"k", "iikko",(SUBR)midic14set, (SUBR)midic14, NULL },
{ "midic21.i", S(MIDICTL4),1,"i", "iiiiio",(SUBR)imidic21,   NULL,     NULL },
{ "midic21.k", S(MIDICTL4), 3,"k", "iiikko",(SUBR)midic21set,(SUBR)midic21, NULL },
{ "ctrl7.i", S(CTRL7), 1,    "i", "iiiio", (SUBR)ictrl7,     NULL,     NULL },
{ "ctrl7.k", S(CTRL7),  3,   "k", "iikko", (SUBR)ctrl7set,  (SUBR)ctrl7, NULL },
{ "ctrl14.i", S(CTRL14),1,   "i", "iiiiio",(SUBR)ictrl14,    NULL,     NULL },
{ "ctrl14.k", S(CTRL14), 3,  "k", "iiikko",(SUBR)ctrl14set, (SUBR)ctrl14,  NULL },
{ "ctrl21.i", S(CTRL21),1,   "i", "iiiiiio", (SUBR)ictrl21,  NULL,     NULL },
{ "ctrl21.k", S(CTRL21), 3,  "k", "iiiikko", (SUBR)ctrl21set, (SUBR)ctrl21, NULL },
{ "initc7", S(INITC7), 1,     "",  "iii",  (SUBR)initc7,     NULL,     NULL },
{ "initc14", S(INITC14), 1,   "",  "iiii", (SUBR)initc14,    NULL,     NULL },
{ "initc21", S(INITC21), 1,   "",  "iiiii",(SUBR)initc21,    NULL,     NULL },
{ "midipgm", S(MIDIPGM_OP), 1, "i", "o",   (SUBR)midipgm_opcode, NULL, NULL }
};

int midiops2_init_(CSOUND *csound)
{
    return csound->AppendOpcodes(csound, &(localops[0]),
                                 (int) (sizeof(localops) / sizeof(OENTRY)));
}

