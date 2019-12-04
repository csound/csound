/*
    ugens2.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch, Robin Whittle

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

#include "csoundCore.h" /*                              UGENS2.C        */
#include "ugens2.h"
#include <math.h>

/* Macro form of Istvan's speedup ; constant should be 3fefffffffffffff */
/* #define FLOOR(x) (x >= FL(0.0) ? (int64_t)x                          */
/*                                  : (int64_t)((double)x - 0.999999999999999))
*/
/* 1.0-1e-8 is safe for a maximum table length of 16777216 */
/* 1.0-1e-15 could incorrectly round down large negative integers, */
/* because doubles do not have sufficient resolution for numbers like */
/* -1000.999999999999999 (FLOOR(-1000) might possibly be -1001 which is wrong)*/
/* it should be noted, though, that the above incorrect result would not be */
/* a problem in the case of interpolating table opcodes, as the fractional */
/* part would then be exactly 1.0, still giving a correct output value */
#define MYFLOOR(x) (x >= FL(0.0) ? (int32_t)x : (int32_t)((double)x - 0.99999999))



int32_t phsset(CSOUND *csound, PHSOR *p)
{
    MYFLT       phs;
    int32_t  longphs;
    if ((phs = *p->iphs) >= FL(0.0)) {
      if (UNLIKELY((longphs = (int32_t)phs))) {
        csound->Warning(csound, Str("init phase truncation\n"));
      }
      p->curphs = phs - (MYFLT)longphs;
    }
    return OK;
}

int32_t ephsset(CSOUND *csound, EPHSOR *p)
{
    MYFLT       phs;
    int32_t  longphs;
    if ((phs = *p->iphs) >= FL(0.0)) {
      if (UNLIKELY((longphs = (int32_t)phs))) {
        csound->Warning(csound, Str("init phase truncation\n"));
      }
      p->curphs = phs - (MYFLT)longphs;
    }
    p->b = 1.0;
    return OK;
}

int32_t ephsor(CSOUND *csound, EPHSOR *p)
{
    double      phase;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    MYFLT       *rs, *aphs, onedsr = csound->onedsr;
    double      b = p->b;
    double      incr, R = *p->kR;

    rs = p->sr;
    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }
    aphs = p->aphs;
    phase = p->curphs;
    if (IS_ASIG_ARG(p->xcps)) {
      MYFLT *cps = p->xcps;
      for (n=offset; n<nsmps; n++) {
        incr = (double)(cps[n] * onedsr);
        rs[n] = (MYFLT) b;
        aphs[n] = (MYFLT) phase;
        phase += incr;
        b *= R;
        if (UNLIKELY(phase >= 1.0)) {
          phase -= 1.0;
          b = pow(R, 1.0+phase);
        }
        else if (UNLIKELY(phase < 0.0)) {
          phase += 1.0;
          b = pow(R, 1.0+phase);
        }
      }
    }
    else {
      incr = (double)(*p->xcps * onedsr);
      for (n=offset; n<nsmps; n++) {
        rs[n] = (MYFLT) b;
        aphs[n] = (MYFLT) phase;
        phase += incr;
        b *= R;
        if (UNLIKELY(phase >= 1.0)) {
          phase -= 1.0;
          b =  pow(R, 1.0+phase);
        }
        else if (UNLIKELY(phase < 0.0)) {
          phase += 1.0;
          b = pow(R, 1.0+phase);
        }
      }
    }
    p->curphs = phase;
    p->b = b;
    return OK;
}

int32_t kphsor(CSOUND *csound, PHSOR *p)
{
    IGN(csound);
    double      phs;
    *p->sr = (MYFLT)(phs = p->curphs);
    if (UNLIKELY((phs += (double)*p->xcps * CS_ONEDKR) >= 1.0))
      phs -= 1.0;
    else if (UNLIKELY(phs < 0.0))
      phs += 1.0;
    p->curphs = phs;
    return OK;
}

int32_t phsor(CSOUND *csound, PHSOR *p)
{
    double      phase;
    uint32_t    offset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    MYFLT       *rs, onedsr = csound->onedsr;
    double      incr;

    rs = p->sr;
    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }
    phase = p->curphs;
    if (IS_ASIG_ARG(p->xcps)) {
      MYFLT *cps = p->xcps;
      for (n=offset; n<nsmps; n++) {
        incr = (double)(cps[n] * onedsr);
        rs[n] = (MYFLT)phase;
        phase += incr;
        if (UNLIKELY((MYFLT)phase >= FL(1.0))) /* VL convert to MYFLT
                                                  to avoid rounded output
                                                  exceeding 1.0 on float version */
          phase -= 1.0;
        else if (UNLIKELY((MYFLT)phase < FL(0.0)))
          phase += 1.0;
      }
    }
    else {
      incr = (double)(*p->xcps * onedsr);
      for (n=offset; n<nsmps; n++) {
        rs[n] = (MYFLT)phase;
        phase += incr;
        if (UNLIKELY((MYFLT)phase >= FL(1.0))) {
          phase -= 1.0;
        }
        else if (UNLIKELY((MYFLT)phase < FL(0.0)))
          phase += 1.0;
      }
    }
    p->curphs = phase;
    return OK;
}

#ifdef SOME_FINE_DAY
/*****************************************************************************/
/*****************************************************************************/

/* Table read code - see TABLE data structure in ugens2.h.  */

/*************************************/

/* itblchk()
 *
 * This is called at init time by tblset() to set up the TABLE data
 * structure for subsequent k and a rate operations.
 *
 * It is also called at init time by itable() and itablei() prior to
 * them calling ktable() and ktabli() respectively to produce a single
 * result at init time.
 *
 * A similar function - ptblchk() does the same job, but reports
 * errors in a way suitable for performance time.  */

/* If the specified table number can be found, then the purpose is to
 * read the three i rate input variables and the function table number
 * input variable - (which we can assume here is also i rate) to set
 * up the TABLE data structure ready for the k and a rate functions.  */

static int32_t itblchk(CSOUND *csound, TABLE *p)
{
    if (UNLIKELY((p->ftp = csound->FTnp2Finde(csound, p->xfn)) == NULL))
      return NOTOK;

    /* Although TABLE has an integer variable for the table number
     * (p->pfn) we do not need to write it.  We know that the k
     * and a rate functions which will follow will not be
     * expecting a changed table number.
     *
     * p->pfn exists only for checking table number changes for
     * functions which are expecting a k rate table number.  */

    /* Set denormalisation factor to 1 or table length, depending
     * on the state of ixmode. */
    if (*p->ixmode)
      p->xbmul = p->ftp->flen;
    else
      p->xbmul = 1L;

    /* Multiply the ixoff value by the xbmul denormalisation
     * factor and then check it is between 0 and the table length.
     *
     * Bug fix: was p->offset * *p->ixoff */

    if (UNLIKELY((p->offset = p->xbmul * *p->ixoff) < FL(0.0) ||
                 p->offset > p->ftp->flen)) {
      return csound->InitError(csound, Str("Offset %f < 0 or > tablelength"),
                                       p->offset);
    }

    p->wrap   = (int32_t)*p->iwrap;
    return OK;
}

/* ptblchk()
 *
 * This is called at init time by tblsetkt() to set up the TABLE data
 * structure for subsequent k and a rate operations which are
 * expecting the table number to change at k rate.
 *
 * tblsetkt() does very little - just setting up the wrap variable in
 * TABLE. All the other variables depend on the table number. This is
 * not available at init time, so the following 4 functions must look
 * for the changed table number and set up the variables accordingly -
 * generating error messages in a way which works at performance time.
 *
 * k rate   a rate
 *
 * ktablekt tablekt   Non interpolated
 * ktablikt tablikt   Interpolated
 *  */
static int32_t ptblchk(CSOUND *csound, TABLE *p)
{
    IGN(csound);                /* Argument is needed to fit structure */
    /* TABLE has an integer variable for the previous table number
     * (p->pfn).
     *
     * Now (at init time) we do not know the function table number
     * which will be provided at perf time, so set p->pfn to 0, so
     * that the k or a rate code will recognise that the first table
     * number is different from the "previous" one.  */
    p->pfn = 0;

    /* The only other thing to do is write the wrap value into the
     * immediate copy of it in TABLE.  */
    p->wrap   = (int32_t)*p->iwrap;
    return OK;
}

/*---------------------------------------------------------------------------*/

/* tblset() */

int32_t tblset(CSOUND *csound, TABLE *p)
{
    if (UNLIKELY(p->XINCODE != p->XOUTCODE)) {
      const char  *opname = csound->GetOpcodeName(p);
      const char  *msg = Str("%s: table index type inconsistent with output");
      if (UNLIKELY(CS_KSMPS == 1))
        csound->Warning(csound, msg, opname);
      else {
        return csound->InitError(csound, msg, opname);
      }
    }
    p->h.iopadr = (SUBR) itblchk;
    return itblchk(csound, p);
}

/* tblsetkt() */

int32_t tblsetkt(CSOUND *csound, TABLE *p)
{
    if (UNLIKELY(p->XINCODE != p->XOUTCODE)) {
      const char  *opname = csound->GetOpcodeName(p);
      const char  *msg = Str("%s: table index type inconsistent with output");
      if (UNLIKELY(CS_KSMPS == 1))
        csound->Warning(csound, msg, opname);
      else {
        return csound->InitError(csound, msg, opname);
      }
    }
    p->h.iopadr = (SUBR) ptblchk;
    return ptblchk(csound, p);
}

/*************************************/

/* Special functions to use when the output value is an init time
 * variable.
 *
 * These are called by the opodlst lines for itable and itablei ugens.
 *
 * They call itblchk() and if the table was found, they call the k
 * rate function just once.
 *
 * If the table was not found, an error will result from ftfind.  */
int32_t ktable(CSOUND *,TABLE*);
int32_t ktabli(CSOUND *,TABLE*);
int32_t ktabl3(CSOUND *,TABLE*);

int32_t itable(CSOUND *csound, TABLE *p)
{
    if (LIKELY(itblchk(csound,p)==OK)) return ktable(csound,p);
    return NOTOK;
}

int32_t itabli(CSOUND *csound, TABLE *p)
{
    if (LIKELY(itblchk(csound,p)==OK)) return ktabli(csound,p);
    return NOTOK;
}

int32_t itabl3(CSOUND *csound, TABLE *p)
{
    if (LIKELY(itblchk(csound,p)==OK)) return ktabl3(csound,p);
    return NOTOK;
}

/*---------------------------------------------------------------------------*/

/* Functions which read the table.
 *
 * First we have the four basic functions for a and k rate, non
 * interpolated and interpolated reading.  These all assume that the
 * TABLE data structure has been correctly set up - they are not
 * expecting the table number to change at k rate.
 *
 * These are:
 * k rate  a rate
 *
 * ktable  table   Non interpolated
 * ktabli  tabli   Interpolated
 * ktabl3  tabl3   Interpolated with cubic
 *
 * Then we have four more functions which are expecting the table
 * number to change at k rate.  They deal with this, and then call one
 * of the above functions to do the reading.
 *
 * These are:
 * k rate   a rate
 *
 * ktablekt tablekt   Non interpolated
 * ktablikt tablikt   Interpolated
 *  */

/* ktable() and ktabli()
 * ---------------------
 *
 * These both read a single value from the table. ktabli() does it
 * with interpolation.
 *
 * This is typically used for k rate reading - where they are called
 * as a result of being listed in a line in engineState.opcodlst.  They are also
 * called by two functions which after they have coped with any change
 * in the k rate function table number.
 *
 * ktablekt() and ktablikt().
 *
 * In addition, they can be called by the init time functions:
 * itable() and itabli().
 *
 *
 * tablefn() and tabli()
 * -------------------
 *
 * These do the reading at a rate with an a rate index.
 *
 * They are called directly via their entries in engineState.opcodlst, and also by
 * two functions which call them after they have coped with any change
 * in the k rate function table number.
 *
 * tablekt() and tablikt().
 *
 * */

/*************************************/

/* ktable() */

int32_t ktable(CSOUND *csound, TABLE   *p)
{
    FUNC        *ftp;
    int32_t        indx, length;
    MYFLT       ndx;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;            /* RWD fix */
    ndx = *p->xndx;
    length = ftp->flen;
    /* Multiply ndx by denormalisation factor, and add in the offset
     * - already denormalised - by tblchk().
     * xbmul = 1 or table length depending on state of ixmode.  */

    ndx = (ndx * p->xbmul) + p->offset;

    /* ndx now includes the offset and is ready to address the table.
     *
     * The original code was:
     *  indx = (int64_t) (ndx + p->offset);
     *
     * This is a problem, causes problems with negative numbers.
     *
     */
     indx = (int32_t) MYFLOOR((double)ndx);

    /* Now for "limit mode" - the "non-wrap" function, depending on
     * iwrap.
     *
     * The following section of code limits the final index to 0 and
     * the last location in the table.
     *
     * It is only used when wrap is OFF.  The wrapping is achieved by
     * code after this - when this code is not run.  */
    if (!p->wrap) {
      /* Previously this code limited the upper range of the indx to
       * the table length - for instance 8.  Then the result was ANDed
       * with a mask (for instance 7).
       *
       * This meant that when the input index was 8 or above, it got
       * reduced to 0.  What we want is for it to stick at the index
       * which reads the last value from the table - in this example
       * from location 7.
       *
       * So instead of limiting to the table length, we limit to
       * (table length - 1).  */
      if (UNLIKELY(indx > length - 1))
        indx = length - 1;

      /* Now limit negative values to zero.  */
      else if (UNLIKELY(indx < 0L))
        indx = 0L;
    }
    /* The following code uses an AND with an integer like 0000 0111
     * to wrap the current index within the range of the table.  In
     * the original version, this code always ran, but with the new
     * (length - 1) code above, it would have no effect, so it is now
     * an else operation - running only when iwrap = 1.  This may save
     * half a usec or so.  */
    else        indx &= ftp->lenmask;

    /* Now find the address of the start of the table, add it to the
     * index, read the value from there and write it to the
     * destination.  */
    *p->rslt = *(ftp->ftable + indx);
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("table(krate): not initialised"));
}

/* tablefn()  */

/* table() is similar to ktable() above, except that it processes an
 * array of input indexes, to send results to another array.  These
 * arrays are ksmps long.  */
/*sbrandon: NeXT m68k does not like 'table' */
int32_t tablefn(CSOUND *csound, TABLE *p)
{
    FUNC        *ftp;
    MYFLT       *rslt, *pxndx, *tab;
    int32_t       indx, mask, length;
    uint32_t    koffset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    MYFLT       ndx, xbmul, offset;
    int32_t         wrap = p->wrap;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;            /* RWD fix */
    rslt = p->rslt;
    if (UNLIKELY(koffset)) memset(rslt, '\0', koffset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    length = ftp->flen;
    pxndx = p->xndx;
    xbmul = (MYFLT)p->xbmul;
    offset = p->offset;
    mask = ftp->lenmask;
    tab = ftp->ftable;
    for (n=koffset; n<nsmps; n++) {
      /* Read in the next raw index and increment the pointer ready
       * for the next cycle.
       *
       * Then multiply the ndx by the denormalising factor and add in
       * the offset.  */

      ndx = (pxndx[n] * xbmul) + offset;
      indx = (int32_t) MYFLOOR((double)ndx);

      /* Limit = non-wrap.  Limits to 0 and (length - 1), or does the
       * wrap code.  See notes above in ktable().  */
      if (!wrap) {
        if (UNLIKELY(indx > length - 1))
          indx = length - 1;
        else if (UNLIKELY(indx < (int32_t)0))
          indx = 0L;
      }
      /* do the wrap code only if we are not doing the non-wrap code.  */
      else
        indx &= mask;
      rslt[n] = tab[indx];
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("table: not initialised"));
}

/* ktabli() */

/* ktabli() is similar to ktable() above, except that it uses the
 * fractional part of the final index to interpolate between one value
 * in the table and the next.
 *
 * This means that it may read the guard point.  In a table of
 * "length" 8, the guardpoint is at locaton 8. The normal part of the
 * table is locations 0 to 7.
 *
 * In non-wrap mode, when the final index is negative, the output
 * should be the value in location 0.
 *
 * In non-wrap mode, when the final index is >= length, then the
 * output should be the value in the guard point location.  */
int32_t ktabli(CSOUND *csound, TABLE   *p)
{
    FUNC        *ftp;
    int32_t        indx, length;
    MYFLT       v1, v2, fract, ndx;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ndx = *p->xndx;
    length = ftp->flen;
    /* Multiply ndx by denormalisation factor.
     * xbmul is 1 or table length depending on state of ixmode.
     * Add in the offset, which has already been denormalised by
     * tblchk().  */

    ndx    = (ndx * p->xbmul) + p->offset;
    indx = (int32_t) MYFLOOR((double)ndx);

    /* We need to generate a fraction - How much above indx is ndx?
     * It will be between 0 and just below 1.0.  */
    fract = ndx - indx;

    /* Start of changes to fix non- wrap bug.
     *
     * There are two changes here:
     *
     * 1 - We only run the wrap code if iwrap = 1. Previously it was
     * always run.
     *
     * 2 - The other change is similar in concept to limiting the
     * index to (length - 1) when in non-wrap mode.
     *
     * This would be fine - the fractional code would enable us to
     * interpolate using an index value which is almost as high as the
     * length of the table.  This would be good for 7.99999 etc.
     * However, to be a little pedantic, what we want is for any index
     * of 8 or more to produce a result exactly equal to the value at
     * the guard point.
     *
     * We will let all (non negative) values which are less than
     * length pass through. This deals with all cases 0 to 7.9999
     * . . .
     *
     * However we will look for final indexes of length (8) and above
     * and take the following steps:
     *
     * fract = 1
     * indx = length - 1
     *
     * We then continue with the rest of code.  This causes the result
     * to be the value read from the guard point - which is what we
     * want.
     *
     * Likewise, if the final index is negative, set both fract and
     * indx to 0.  */
    if (!p->wrap) {
      if (UNLIKELY(ndx > length)) {
        indx  = length - 1;
        fract = FL(1.0);
      }
      else if (UNLIKELY(indx < 0L)) {
        indx  = 0L;
        fract = FL(0.0);
      }
    }
    /* We are in wrap mode, so do the wrap function.  */
    else        indx &= ftp->lenmask;

    /* Now read the value at indx and the one beyond */
    v1 = *(ftp->ftable + indx);
    v2 = *(ftp->ftable + indx + 1);
    *p->rslt = v1 + (v2 - v1) * fract;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("tablei(krate): not initialised"));
}


int32_t ktabl3(CSOUND *csound, TABLE   *p)
{
    FUNC        *ftp;
    int32_t        indx, length;
    MYFLT       v1, v2, fract, ndx;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ndx = *p->xndx;
    length = ftp->flen;
    /* Multiply ndx by denormalisation factor.
     * xbmul is 1 or table length depending on state of ixmode.
     * Add in the offset, which has already been denormalised by
     * tblchk().  */

    ndx    = (ndx * p->xbmul) + p->offset;
    indx = (int32_t) MYFLOOR((double)ndx);

    /* We need to generate a fraction - How much above indx is ndx?
     * It will be between 0 and just below 1.0.  */
    fract = ndx - indx;

    /* Start of changes to fix non- wrap bug.
     *
     * There are two changes here:
     *
     * 1 - We only run the wrap code if iwrap = 1. Previously it was
     * always run.
     *
     * 2 - The other change is similar in concept to limiting the
     * index to (length - 1) when in non-wrap mode.
     *
     * This would be fine - the fractional code would enable us to
     * interpolate using an index value which is almost as high as the
     * length of the table.  This would be good for 7.99999 etc.
     * However, to be a little pedantic, what we want is for any index
     * of 8 or more to produce a result exactly equal to the value at
     * the guard point.
     *
     * We will let all (non negative) values which are less than
     * length pass through. This deals with all cases 0 to 7.9999
     * . . .
     *
     * However we will look for final indexes of length (8) and above
     * and take the following steps:
     *
     * fract = 1
     * indx = length - 1
     *
     * We then continue with the rest of code.  This causes the result
     * to be the value read from the guard point - which is what we
     * want.
     *
     * Likewise, if the final index is negative, set both fract and
     * indx to 0.  */
    if (!p->wrap) {
      if (UNLIKELY(ndx > length)) {
        indx  = length - 1;
        fract = FL(1.0);
      }
      else if (UNLIKELY(indx < 0L)) {
        indx  = 0L;
        fract = FL(0.0);
      }
    }
    /* We are in wrap mode, so do the wrap function.  */
    else        indx &= ftp->lenmask;

    /* interpolate with cubic if we can, else linear */
    if (UNLIKELY(indx<1 || indx==length-1 || length <4)) {
      v1 = *(ftp->ftable + indx);
      v2 = *(ftp->ftable + indx + 1);
      *p->rslt = v1 + (v2 - v1) * fract;
    }
    else {
      MYFLT *tab = ftp->ftable;
      MYFLT ym1 = tab[indx-1], y0 = tab[indx];
      MYFLT y1 = tab[indx+1], y2 = tab[indx+2];
      MYFLT frsq = fract*fract;
      MYFLT frcu = frsq*ym1;
      MYFLT t1 = y2 + y0+y0+y0;
      *p->rslt = y0 + FL(0.5)*frcu
        + fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0))
        + frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) + frsq*(FL(0.5)* y1 - y0);
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("table3(krate): not initialised"));
}

/* tabli() */

/* tabli() is similar to ktabli() above, except that it processes an
 * array of input indexes, to send results to another array. */

int32_t tabli(CSOUND *csound, TABLE   *p)
{
    FUNC        *ftp;
    int32_t        indx, mask, length;
    uint32_t     koffset = p->h.insdshead->ksmps_offset;
    uint32_t     early  = p->h.insdshead->ksmps_no_end;
    uint32_t     n, nsmps = CS_KSMPS;
    MYFLT       *rslt, *pxndx, *tab;
    MYFLT        fract, v1, v2, ndx, xbmul, offset;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    rslt   = p->rslt;
    if (UNLIKELY(koffset)) memset(rslt, '\0', koffset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    length = ftp->flen;
    pxndx  = p->xndx;
    xbmul  = (MYFLT)p->xbmul;
    offset = p->offset;
    mask   = ftp->lenmask;
    tab    = ftp->ftable;
    /* As for ktabli() code to handle non wrap mode, and wrap mode.  */
    if (!p->wrap) {
      for (n=koffset; n<nsmps; n++) {
        /* Read in the next raw index and increment the pointer ready
         * for the next cycle.
         * Then multiply the ndx by the denormalising factor and add in
         * the offset.  */
        ndx = (pxndx[n] * xbmul) + offset;
        indx = (int32_t) ndx;
        if (UNLIKELY(indx <= 0L)) {
          rslt[n] = tab[0];
          continue;
        }
        if (UNLIKELY(indx >= length)) {
          rslt[n] = tab[length];
          continue;
        }
        /* We need to generate a fraction - How much above indx is ndx?
         * It will be between 0 and just below 1.0.  */
        fract = ndx - indx;
        /* As for ktabli(), read two values and interpolate between
         * them.  */
        v1 = tab[indx];
        v2 = tab[indx + 1];
        rslt[n] = v1 + (v2 - v1)*fract;
      }
    }
    else {                      /* wrap mode */
      for (n=koffset; n<nsmps; n++) {
        /* Read in the next raw index and increment the pointer ready
         * for the next cycle.
         * Then multiply the ndx by the denormalising factor and add in
         * the offset.  */
        ndx = (pxndx[n] * xbmul) + offset;
        indx = (int32_t) MYFLOOR(ndx);
        /* We need to generate a fraction - How much above indx is ndx?
         * It will be between 0 and just below 1.0.  */
        fract = ndx - indx;
        indx &= mask;
        /* As for ktabli(), read two values and interpolate between
         * them.  */
        v1 = tab[indx];
        v2 = tab[indx + 1];
        rslt[n] = v1 + (v2 - v1)*fract;
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("tablei: not initialised"));
}

int32_t tabl3(CSOUND *csound, TABLE *p)     /* Like tabli but cubic interpolation */
{
    FUNC        *ftp;
    int32_t        indx, mask, length;
    uint32_t     koffset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t     n, nsmps = CS_KSMPS;
    MYFLT       *rslt, *pxndx, *tab;
    MYFLT        fract, v1, v2, ndx, xbmul, offset;
    int32_t          wrap = p->wrap;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    rslt = p->rslt;
    if (UNLIKELY(koffset)) memset(rslt, '\0', koffset*sizeof(MYFLT));
        if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    length = ftp->flen;
    pxndx = p->xndx;
    xbmul = (MYFLT)p->xbmul;
    offset = p->offset;
    mask = ftp->lenmask;
    tab = ftp->ftable;
    for (n=koffset; n<nsmps; n++) {
      /* Read in the next raw index and increment the pointer ready
       * for the next cycle.
       * Then multiply the ndx by the denormalising factor and add in
       * the offset.  */

      ndx = (pxndx[n] * xbmul) + offset;
      indx = (int32_t) MYFLOOR((double)ndx);

      /* We need to generate a fraction - How much above indx is ndx?
       * It will be between 0 and just below 1.0.  */
      fract = ndx - indx;
      /* As for ktabli() code to handle non wrap mode, and wrap mode.  */
      if (!wrap) {
        if (UNLIKELY(ndx > length)) {
          indx  = length - 1;
          fract = FL(1.0);
        }
        else if (UNLIKELY(indx < 0L)) {
          indx  = 0L;
          fract = FL(0.0);
        }
      }
      else
        indx &= mask;
      /* interpolate with cubic if we can */
      if (UNLIKELY(indx <1 || indx == length-1 || length<4)) {
        /* Too short or at ends */
        v1 = tab[indx];
        v2 = tab[indx + 1];
        rslt[n] = v1 + (v2 - v1)*fract;
      }
      else {
        MYFLT ym1 = tab[indx-1], y0 = tab[indx];
        MYFLT y1 = tab[indx+1], y2 = tab[indx+2];
        MYFLT frsq = fract*fract;
        MYFLT frcu = frsq*ym1;
        MYFLT t1 = y2 + y0+y0+y0;
        rslt[n] = y0 + FL(0.5)*frcu +
          fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0)) +
          frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) + frsq*(FL(0.5)* y1 - y0);
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("table3: not initialised"));
}

/*************************************/

/* Four functions to call the above four, after handling the k rate
 * table number variable.
 *
 * tblsetkt() does very little - just setting up the wrap variable in
 * TABLE. All the other variables depend on the table number. This is
 * not available at init time, so the following 4 functions must look
 * for the changed table number and set up the variables accordingly -
 * generating error messages in a way which works at performance time.
 * * k rate   a rate
 *
 * ktablekt tablekt   Non interpolated
 * ktablikt tablikt   Interpolated
 *
 * Since these perform identical operations, apart from the function
 * they call, create a common function to do this work:
 *
 * ftkrchk() */

static int32_t ftkrchk(CSOUND *csound, TABLE *p)
{
    /* Check the table number is >= 1.  Print error and deactivate if
     * it is not.  Return NOTOK to tell calling function not to proceed
     * with a or k rate operations.
     *
     * We must do this to catch the situation where the first call has
     * a table number of 0, and since that equals pfn, we would
     * otherwise proceed without checking the table number - and none
     * of the pointers would have been set up.  */
    if (*p->xfn < 1) goto err1;
    /* Check to see if table number has changed from previous value.
     * On the first run through, the previous value will be 0.  */

    if (p->pfn != (int32_t)*p->xfn) {
        /* If it is different, check to see if the table exists.
         *
         * If it doesn't, an error message should be produced by
         * csoundFTFindP() which should also deactivate the instrument.
         *
         * Return 0 to tell calling function not to proceed with a or
         * k rate operations. */

      if (UNLIKELY((p->ftp = csound->FTFindP(csound, p->xfn) ) == NULL)) {
        return NOTOK;
      }

        /* p->ftp now points to the FUNC data structure of the newly
         * selected table.
         *
         * Now we set up some variables in TABLE ready for the k or a
         * rate functions which follow.  */

        /* Write the integer version of the table number into pfn so
         * we can later decide whether subsequent calls to the k and a
         * rate functions occur with a table number value which points
         * to a different table. */
      p->pfn = (int32_t)*p->xfn;

        /* Set denormalisation factor to 1 or table length, depending
         * on the state of ixmode. */
      if (*p->ixmode)
        p->xbmul = p->ftp->flen;
      else    p->xbmul = 1L;

        /* Multiply the ixoff value by the xbmul denormalisation
         * factor and then check it is between 0 and the table length.  */

      if (UNLIKELY((p->offset = p->xbmul * *p->ixoff) < FL(0.0) ||
                   p->offset > p->ftp->flen)) goto err2;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("k rate function table no. %f < 1"),
                             *p->xfn);
 err2:
    return csound->PerfError(csound, &(p->h),
                             Str("Offset %f < 0 or > tablelength"),
                             p->offset);
}

/* Now for the four functions, which are called as a result of being
 * listed in engineState.opcodlst in entry.c */

int32_t    ktablekt(CSOUND *csound, TABLE *p)
{
    if (LIKELY(ftkrchk(csound,p)==OK)) return ktable(csound,p);
    return NOTOK;
}

int32_t    tablekt(CSOUND *csound, TABLE *p)
{
    if (LIKELY(ftkrchk(csound,p)==OK)) return tablefn(csound,p);
    return NOTOK;
}

int32_t    ktablikt(CSOUND *csound, TABLE *p)
{
    if (LIKELY(ftkrchk(csound,p)==OK)) return ktabli(csound,p);
    return NOTOK;
}

int32_t    tablikt(CSOUND *csound, TABLE *p)
{
    if (LIKELY(ftkrchk(csound,p)==OK)) return tabli(csound,p);
    return NOTOK;
}

int32_t    ktabl3kt(CSOUND *csound, TABLE *p)
{
    if (LIKELY(ftkrchk(csound,p)==OK)) return ktabl3(csound,p);
    return NOTOK;
}

int32_t    tabl3kt(CSOUND *csound, TABLE *p)
{
    if (LIKELY(ftkrchk(csound,p)==OK)) return tabl3(csound,p);
    return NOTOK;
}
#endif /* SOME_FINE_DAY */

int32_t ko1set(CSOUND *csound, OSCIL1 *p)
{
    FUNC        *ftp;

    if (UNLIKELY((ftp = csound->FTFind(csound, p->ifn)) == NULL))
      return NOTOK;
    if (UNLIKELY(*p->idur <= FL(0.0))) {
      /*csound->Warning(csound, Str("duration < zero\n"));*/
      p->phs = MAXLEN-1;
    }
    else p->phs = 0;
    p->ftp = ftp;
    p->dcnt = (int32_t)(*p->idel * CS_EKR);
    p->kinc = (int32_t) (CS_KICVT / *p->idur);
    if (p->kinc==0) p->kinc = 1;

    return OK;
}

int32_t kosc1(CSOUND *csound, OSCIL1 *p)
{
    FUNC *ftp;
    int32_t  phs, dcnt;
    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    phs = p->phs;
    *p->rslt = *(ftp->ftable + (phs >> ftp->lobits)) * *p->kamp;
    if ((dcnt = p->dcnt) > 0)
      dcnt--;
    else if (dcnt == 0) {
      phs += p->kinc;
      if (UNLIKELY(phs >= MAXLEN)){
        phs = MAXLEN;
        dcnt--;
      }
      else if (UNLIKELY(phs < 0)){
      phs = 0;
        dcnt--;
      }
      p->phs = phs;
    }
    p->dcnt = dcnt;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil1(krate): not initialised"));
}

int32_t kosc1i(CSOUND *csound, OSCIL1   *p)
{
    FUNC        *ftp;
    MYFLT       fract, v1, *ftab;
    int32_t        phs, dcnt;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    phs = p->phs;
    fract = PFRAC(phs);
    ftab = ftp->ftable + (phs >> ftp->lobits);
    v1 = *ftab++;
    *p->rslt = (v1 + (*ftab - v1) * fract) * *p->kamp;
    if ((dcnt = p->dcnt) > 0) {
      dcnt--;
      p->dcnt = dcnt;
    }
    else if (dcnt == 0) {
      phs += p->kinc;
      if (UNLIKELY(phs >= MAXLEN)) {
        phs = MAXLEN;
        dcnt--;
        p->dcnt = dcnt;
      } else if (UNLIKELY(phs < 0)){
      phs = 0;
        dcnt--;
      }
      p->phs = phs;
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil1i(krate): not initialised"));
}

int32_t oscnset(CSOUND *csound, OSCILN *p)
{
    FUNC        *ftp;
    if (LIKELY((ftp = csound->FTnp2Finde(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
      p->inc = ftp->flen * *p->ifrq * csound->onedsr;
      p->index = FL(0.0);
      p->maxndx = ftp->flen - FL(1.0);
      p->ntimes = (int32_t)*p->itimes;
      return OK;
    }
    else return NOTOK;
}

int32_t osciln(CSOUND *csound, OSCILN *p)
{
    MYFLT *rs = p->rslt;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY(p->ftp==NULL)) goto err1;
    if (UNLIKELY(offset)) memset(rs, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rs[nsmps], '\0', early*sizeof(MYFLT));
    }
    if (p->ntimes) {
      MYFLT *ftbl = p->ftp->ftable;
      MYFLT amp = *p->kamp;
      MYFLT ndx = p->index;
      MYFLT inc = p->inc;
      MYFLT maxndx = p->maxndx;
      for (n=offset; n<nsmps; n++) {
        rs[n] = ftbl[(int32_t)ndx] * amp;
        if (UNLIKELY((ndx += inc) > maxndx)) {
          if (--p->ntimes)
            ndx -= maxndx;
          else if (UNLIKELY(n==nsmps))
            return OK;
          else
            goto putz;
        }
      }
      p->index = ndx;
    }
    else {
      n=0;              /* Can jump out of previous loop into this one */
    putz:
      memset(&rs[n], 0, (nsmps-n)*sizeof(MYFLT));
      /* for (; n<nsmps; n++) { */
      /*   rs[n] = FL(0.0); */
      /* } */
    }
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("osciln: not initialised"));
}

static int32_t fill_func_from_array(ARRAYDAT *a, FUNC *f)
{
    int32_t     lobits, ltest, flen, i;
    int32_t     nonpowof2_flag = 0;

    flen = f->flen = a->sizes[0];
    flen &= -2L;
    for (ltest = flen, lobits = 0;
         (ltest & MAXLEN) == 0L;
         lobits++, ltest <<= 1)
      ;
    if (UNLIKELY(ltest != MAXLEN)) {
      lobits = 0;
      nonpowof2_flag = 1;
    }
    f->ftable   = a->data;
    f->lenmask  = ((flen & (flen - 1L)) ?
                   0L : (flen - 1L));      /*  init hdr w powof2 data  */
    f->lobits   = lobits;
    i           = (1 << lobits);
    f->lomask   = (int32_t) (i - 1);
    f->lodiv    = FL(1.0) / (MYFLT) i;        /*    & other useful vals   */
    f->nchanls  = 1;                          /*    presume mono for now  */
    f->flenfrms = flen;
    if (nonpowof2_flag)
      f->lenmask = 0xFFFFFFFF;
    return OK;
}

int32_t oscsetA(CSOUND *csound, OSC *p)
{
    FUNC        *ftp = &p->FF;
    int32_t x;

    if (*p->iphs >= 0)
      p->lphs = ((int32_t)(*p->iphs * FMAXLEN)) & PHMASK;
    //check p->ifn is a valid array with power-of-two length
    x = ((ARRAYDAT*)p->ifn)->sizes[0];
    if (LIKELY((x != 0) && !(x & (x - 1)))) {
       p->ftp = ftp;
       fill_func_from_array((ARRAYDAT*)p->ifn, ftp);
      return OK;
      }
    else return csound->InitError(csound, Str("array size not pow-of-two\n"));
}

int32_t oscset(CSOUND *csound, OSC *p)
{
    FUNC        *ftp;
    if (LIKELY((ftp = csound->FTFind(csound, p->ifn)) != NULL)) {
      p->ftp = ftp;
      if (*p->iphs >= 0)
        p->lphs = ((int32_t)(*p->iphs * FMAXLEN)) & PHMASK;
      return OK;
    }
    return NOTOK;
}

int32_t koscil(CSOUND *csound, OSC *p)
{
    FUNC    *ftp;
    int32_t    phs, inc;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    phs = p->lphs;
    inc = (int32_t) (*p->xcps * CS_KICVT);
    *p->sr = ftp->ftable[phs >> ftp->lobits] * *p->xamp;
    phs += inc;
    phs &= PHMASK;
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil(krate): not initialised"));
}

int32_t osckk(CSOUND *csound, OSC *p)
{
    FUNC    *ftp;
    MYFLT   amp, *ar, *ftbl;
    int32_t   phs, inc, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftbl = ftp->ftable;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * csound->sicvt);
    lobits = ftp->lobits;
    amp = *p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (n=offset;n<nsmps;n++) {
      ar[n] = ftbl[phs >> lobits] * amp;
      /* phs += inc; */
      /* phs &= PHMASK; */
      phs = (phs+inc)&PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil: not initialised"));
}

int32_t oscka(CSOUND *csound, OSC *p)
{
    FUNC    *ftp;
    MYFLT   *ar, amp, *cpsp, *ftbl;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   sicvt = csound->sicvt;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    amp = *p->xamp;
    cpsp = p->xcps;
    phs = p->lphs;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      int32_t inc = MYFLT2LONG(cpsp[n] * sicvt);
      ar[n] = ftbl[phs >> lobits] * amp;
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil: not initialised"));
}

int32_t oscak(CSOUND *csound, OSC *p)
{
    FUNC    *ftp;
    MYFLT   *ar, *ampp, *ftbl;
    int32_t    phs, inc, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * csound->sicvt);
    ampp = p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      ar[n] = ftbl[phs >> lobits] * ampp[n];
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil: not initialised"));
}

int32_t oscaa(CSOUND *csound, OSC *p)
{
    FUNC    *ftp;
    MYFLT   *ar, *ampp, *cpsp, *ftbl;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   sicvt = csound->sicvt;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    ampp = p->xamp;
    cpsp = p->xcps;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      int32_t inc = MYFLT2LONG(cpsp[n] * sicvt);
      ar[n] = ftbl[phs >> lobits] * ampp[n];
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil: not initialised"));
}

int32_t koscli(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    int32_t    phs, inc;
    MYFLT  *ftab, fract, v1;

    phs = p->lphs;
    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    fract = PFRAC(phs);
    ftab = ftp->ftable + (phs >> ftp->lobits);
    v1 = ftab[0];
    *p->sr = (v1 + (ftab[1] - v1) * fract) * *p->xamp;
    inc = (int32_t)(*p->xcps * CS_KICVT);
    phs += inc;
    phs &= PHMASK;
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscili(krate): not initialised"));
}

int32_t osckki(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    MYFLT   fract, v1, amp, *ar, *ft, *ftab;
    int32_t   phs, inc, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    if (UNLIKELY((ftp = p->ftp)==NULL)) goto err1;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * csound->sicvt);
    amp = *p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    ft = ftp->ftable;
    for (n=offset; n<nsmps; n++) {
      fract = PFRAC(phs);
      ftab = ft + (phs >> lobits);
      v1 = ftab[0];
      ar[n] = (v1 + (ftab[1] - v1) * fract) * amp;
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscili: not initialised"));
}

int32_t osckai(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    MYFLT   *ar, amp, *cpsp, fract, v1, *ftab, *ft;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   sicvt = csound->sicvt;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    lobits = ftp->lobits;
    amp = *p->xamp;
    cpsp = p->xcps;
    phs = p->lphs;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    ft = ftp->ftable;
    for (n=offset;n<nsmps;n++) {
      int32_t inc;
      inc = MYFLT2LONG(cpsp[n] * sicvt);
      fract = PFRAC(phs);
      ftab = ft + (phs >> lobits);
      v1 = ftab[0];
      ar[n] = (v1 + (ftab[1] - v1) * fract) * amp;
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscili: not initialised"));
}

int32_t oscaki(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    MYFLT    v1, fract, *ar, *ampp, *ftab, *ft;
    int32_t    phs, inc, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * csound->sicvt);
    ampp = p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    ft = ftp->ftable;
    for (n=offset;n<nsmps;n++) {
      fract = (MYFLT) PFRAC(phs);
      ftab = ft + (phs >> lobits);
      v1 = ftab[0];
      ar[n] = (v1 + (ftab[1] - v1) * fract) * ampp[n];
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscili: not initialised"));
}

int32_t oscaai(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    MYFLT   v1, fract, *ar, *ampp, *cpsp, *ftab, *ft;
    int32_t   phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT   sicvt = csound->sicvt;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ft = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    ampp = p->xamp;
    cpsp = p->xcps;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      int32_t inc;
      inc = MYFLT2LONG(cpsp[n] * sicvt);
      fract = (MYFLT) PFRAC(phs);
      ftab = ft + (phs >> lobits);
      v1 = ftab[0];
      ar[n] = (v1 + (ftab[1] - v1) * fract) * ampp[n];
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscili: not initialised"));
}

int32_t koscl3(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    int32_t    phs, inc;
    MYFLT  *ftab, fract;
    int32_t   x0;
    MYFLT   y0, y1, ym1, y2, amp = *p->xamp;

    phs = p->lphs;
    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftab = ftp->ftable;
    fract = PFRAC(phs);
    x0 = (phs >> ftp->lobits);
    x0--;
    if (UNLIKELY(x0<0)) {
      ym1 = ftab[ftp->flen-1]; x0 = 0;
    }
    else ym1 = ftab[x0++];
    y0 = ftab[x0++];
    y1 = ftab[x0++];
    if (UNLIKELY(x0>(int32_t)ftp->flen)) y2 = ftab[1]; else y2 = ftab[x0];
    {
      MYFLT frsq = fract*fract;
      MYFLT frcu = frsq*ym1;
      MYFLT t1 = y2 + y0+y0+y0;
      *p->sr = amp * (y0 + FL(0.5)*frcu +
                      fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0)) +
                      frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) +
                      frsq*(FL(0.5)* y1 - y0));
    }
    inc = (int32_t)(*p->xcps * CS_KICVT);
    phs += inc;
    phs &= PHMASK;
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil3(krate): not initialised"));
}

int32_t osckk3(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    MYFLT   fract, amp, *ar, *ftab;
    int32_t    phs, inc, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t   x0;
    MYFLT   y0, y1, ym1, y2;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * csound->sicvt);
    amp = *p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      fract = PFRAC(phs);
      x0 = (phs >> lobits);
      x0--;
      if (UNLIKELY(x0<0)) {
        ym1 = ftab[ftp->flen-1]; x0 = 0;
      }
      else ym1 = ftab[x0++];
      y0 = ftab[x0++];
      y1 = ftab[x0++];
      if (UNLIKELY(x0>(int32_t)ftp->flen)) y2 = ftab[1]; else y2 = ftab[x0];
/*    printf("fract = %f; y = %f, %f, %f, %f\n", fract,ym1,y0,y1,y2); */
      {
        MYFLT frsq = fract*fract;
        MYFLT frcu = frsq*ym1;
        MYFLT t1 = y2 + y0+y0+y0;
/*      MYFLT old = (y0 + (y1 - y0) * fract) * amp; */
/*      double x = ((double)(x0-2)+fract)*twopi/32.0; */
/*      MYFLT tr = amp*sin(x); */
        ar[n] = amp * (y0 + FL(0.5)*frcu +
                       fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0)) +
                       frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) +
                       frsq*(FL(0.5)* y1 - y0));
/*      printf("oscilkk3: old=%.4f new=%.4f true=%.4f (%f; %f)\n", */
/*                       old, *(ar-1), tr, fabs(*(ar-1)-tr), fabs(old-tr)); */
      }
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil3: not initialised"));
}

int32_t oscka3(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    MYFLT   *ar, amp, *cpsp, fract, *ftab;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t   x0;
    MYFLT   y0, y1, ym1, y2;
    MYFLT   sicvt = csound->sicvt;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    amp = *p->xamp;
    cpsp = p->xcps;
    phs = p->lphs;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      int32_t inc;
      inc = MYFLT2LONG(cpsp[n] * sicvt);
      fract = PFRAC(phs);
      x0 = (phs >> lobits);
      x0--;
      if (UNLIKELY(x0<0)) {
        ym1 = ftab[ftp->flen-1]; x0 = 0;
      }
      else ym1 = ftab[x0++];
      y0 = ftab[x0++];
      y1 = ftab[x0++];
      if (UNLIKELY(x0>(int32_t)ftp->flen)) y2 = ftab[1]; else y2 = ftab[x0];
      {
        MYFLT frsq = fract*fract;
        MYFLT frcu = frsq*ym1;
        MYFLT t1 = y2 + y0+y0+y0;
        ar[n] = amp * (y0 + FL(0.5)*frcu +
                       fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0)) +
                       frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) + frsq*(FL(0.5)*
                                                                    y1 - y0));
      }
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil3: not initialised"));
}

int32_t oscak3(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    MYFLT   fract, *ar, *ampp, *ftab;
    int32_t    phs, inc, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t   x0;
    MYFLT   y0, y1, ym1, y2;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * csound->sicvt);
    ampp = p->xamp;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      fract = (MYFLT) PFRAC(phs);
      x0 = (phs >> lobits);
      x0--;
      if (UNLIKELY(x0<0)) {
        ym1 = ftab[ftp->flen-1]; x0 = 0;
      }
      else ym1 = ftab[x0++];
      y0 = ftab[x0++];
      y1 = ftab[x0++];
      if (UNLIKELY(x0>(int32_t)ftp->flen)) y2 = ftab[1]; else y2 = ftab[x0];
      {
        MYFLT frsq = fract*fract;
        MYFLT frcu = frsq*ym1;
        MYFLT t1 = y2 + y0+y0+y0;
        ar[n] = ampp[n] *(y0 + FL(0.5)*frcu
                          + fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0))
                          + frsq*fract*(t1/FL(6.0) - FL(0.5)*y1)
                          + frsq*(FL(0.5)* y1 - y0));
      }
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil3: not initialised"));
}

int32_t oscaa3(CSOUND *csound, OSC   *p)
{
    FUNC    *ftp;
    MYFLT    fract, *ar, *ampp, *cpsp, *ftab;
    int32_t    phs, lobits;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t    x0;
    MYFLT    y0, y1, ym1, y2;
    MYFLT    sicvt = csound->sicvt;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    ampp = p->xamp;
    cpsp = p->xcps;
    ar = p->sr;
    if (UNLIKELY(offset)) memset(ar, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&ar[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset;n<nsmps;n++) {
      int32_t inc = MYFLT2LONG(cpsp[n] * sicvt);
      fract = (MYFLT) PFRAC(phs);
      x0 = (phs >> lobits);
      x0--;
      if (UNLIKELY(x0<0)) {
        ym1 = ftab[ftp->flen-1]; x0 = 0;
      }
      else ym1 = ftab[x0++];
      y0 = ftab[x0++];
      y1 = ftab[x0++];
      if (UNLIKELY(x0>(int32_t)ftp->flen)) y2 = ftab[1]; else y2 = ftab[x0];
      {
        MYFLT frsq = fract*fract;
        MYFLT frcu = frsq*ym1;
        MYFLT t1 = y2 + y0+y0+y0;
        ar[n] = ampp[n] *(y0 + FL(0.5)*frcu
                          + fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0))
                          + frsq*fract*(t1/FL(6.0) - FL(0.5)*y1)
                          + frsq*(FL(0.5)* y1 - y0));
      }
      phs = (phs+inc) & PHMASK;
    }
    p->lphs = phs;
    return OK;
 err1:
    return csound->PerfError(csound, &(p->h),
                             Str("oscil3: not initialised"));
}
