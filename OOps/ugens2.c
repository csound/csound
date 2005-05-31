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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

#include "cs.h"         /*                              UGENS2.C        */
#include "ugens2.h"
#include <math.h>

/* Macro form of Istvan's speedup ; constant should be 3fefffffffffffff */
/*
#define FLOOR(x) (x >= FL(0.0) ? (long)x : (long)((double)x - 0.999999999999999))
*/
/* 1.0-1e-8 is safe for a maximum table length of 16777216 */
/* 1.0-1e-15 could incorrectly round down large negative integers, */
/* because doubles do not have sufficient resolution for numbers like */
/* -1000.999999999999999 (FLOOR(-1000) might possibly be -1001 which is wrong)*/
/* it should be noted, though, that the above incorrect result would not be */
/* a problem in the case of interpolating table opcodes, as the fractional */
/* part would then be exactly 1.0, still giving a correct output value */
#define FLOOR(x) (x >= FL(0.0) ? (long) x : (long) ((double) x - 0.99999999))

int phsset(ENVIRON *csound, PHSOR *p)
{
    MYFLT       phs;
    long  longphs;
    if ((phs = *p->iphs) >= FL(0.0)) {
      if ((longphs = (long)phs)) {
        if (csound->oparms->msglevel & WARNMSG)
          csound->Message(csound, Str("WARNING: init phase truncation\n"));
      }
      p->curphs = phs - (MYFLT)longphs;
    }
    return OK;
}

int kphsor(ENVIRON *csound, PHSOR *p)
{
    double      phs;
    *p->sr = (MYFLT)(phs = p->curphs);
    if ((phs += (double)*p->xcps * csound->onedkr) >= 1.0)
      phs -= 1.0;
    else if (phs < 0.0)
      phs += 1.0;
    p->curphs = phs;
    return OK;
}

int phsor(ENVIRON *csound, PHSOR *p)
{
    double      phase;
    int         n, nsmps=csound->ksmps;
    MYFLT       *rs;
    double      incr;

    rs = p->sr;
    phase = p->curphs;
    if (p->XINCODE) {
      MYFLT *cps = p->xcps;
      for (n=0; n<nsmps; n++) {
        incr = (double)(cps[n] * csound->onedsr);
        rs[n] = (MYFLT)phase;
        phase += incr;
        if (phase >= 1.0)
          phase -= 1.0;
        else if (phase < 0.0)
          phase += 1.0;
      }
    }
    else {
      incr = (double)(*p->xcps * csound->onedsr);
      for (n=0; n<nsmps; n++) {
        rs[n] = (MYFLT)phase;
        phase += incr;
        if (phase >= 1.0)
          phase -= 1.0;
        else if (phase < 0.0)
          phase += 1.0;
      }
    }
    p->curphs = phase;
    return OK;
}

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

int itblchk(ENVIRON *csound, TABLE *p)
{
    if ((p->ftp = csound->FTFind(csound, p->xfn)) == NULL)
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

    else    p->xbmul = 1L;

    /* Multiply the ixoff value by the xbmul denormalisation
     * factor and then check it is between 0 and the table length.
     *
     * Bug fix: was p->offset * *p->ixoff */

    if ((p->offset = p->xbmul * *p->ixoff) < FL(0.0) ||
        p->offset > p->ftp->flen) {
      return csound->InitError(csound, Str("Offset %f < 0 or > tablelength"),
                                       p->offset);
    }

    p->wrap   = (int)*p->iwrap;
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
int ptblchk(ENVIRON *csound, TABLE *p)
{
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
    p->wrap   = (int)*p->iwrap;
    return OK;
}

/*---------------------------------------------------------------------------*/

/* tblset() */

int tblset(ENVIRON *csound, TABLE *p)
{
    return itblchk(csound,p);
}

/* tblsetkt() */

int tblsetkt(ENVIRON *csound, TABLE *p)
{
    return ptblchk(csound,p);
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
int ktable(ENVIRON *,TABLE*);
int ktabli(ENVIRON *,TABLE*);
int ktabl3(ENVIRON *,TABLE*);

int itable(ENVIRON *csound, TABLE *p)
{
    if (itblchk(csound,p)==OK) return ktable(csound,p);
    return NOTOK;
}

int itabli(ENVIRON *csound, TABLE *p)
{
    if (itblchk(csound,p)==OK) return ktabli(csound,p);
    return NOTOK;
}

int itabl3(ENVIRON *csound, TABLE *p)
{
    if (itblchk(csound,p)==OK) return ktabl3(csound,p);
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
 * as a result of being listed in a line in opcodlst.  They are also
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
 * They are called directly via their entries in opcodlst, and also by
 * two functions which call them after they have coped with any change
 * in the k rate function table number.
 *
 * tablekt() and tablikt().
 *
 * */

/*************************************/

/* ktable() */

int ktable(ENVIRON *csound, TABLE  *p)
{
    FUNC        *ftp;
    long        indx, length;
    MYFLT       ndx;

    ftp = p->ftp;
    if (ftp==NULL) {            /* RWD fix */
      return csound->PerfError(csound, Str("table(krate): not initialised"));
    }
    ndx = *p->xndx;
    length = ftp->flen;
    /* Multiply ndx by denormalisation factor, and add in the offset
     * - already denormalised - by tblchk().
     * xbmul = 1 or table length depending on state of ixmode.  */

    ndx = ( ndx * p->xbmul) + p->offset;

    /* ndx now includes the offset and is ready to address the table.
     *
     * The original code was:
     *  indx = (long) (ndx + p->offset);
     *
     * This is a problem, causes problems with negative numbers.
     *
     */
     indx = (long) FLOOR((double)ndx);

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
      if (indx > length - 1)
        indx = length - 1;

      /* Now limit negative values to zero.  */
      else if (indx < 0L)
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
}

/* tablefn()  */

/* table() is similar to ktable() above, except that it processes an
 * array of input indexes, to send results to another array.  These
 * arrays are ksmps long.  */
/*sbrandon: NeXT m68k does not like 'table' */
int tablefn(ENVIRON *csound, TABLE  *p)
{
    FUNC        *ftp;
    MYFLT       *rslt, *pxndx, *tab;
    long        indx, mask, length;
    int         n, nsmps=csound->ksmps;
    MYFLT       ndx, xbmul, offset;

    ftp = p->ftp;
    if (ftp==NULL) {            /* RWD fix */
      return csound->PerfError(csound, Str("table: not initialised"));
    }
    rslt = p->rslt;
    length = ftp->flen;
    pxndx = p->xndx;
    xbmul = (MYFLT)p->xbmul;
    offset = p->offset;
    mask = ftp->lenmask;
    tab = ftp->ftable;
    for (n=0; n<nsmps; n++) {
      /* Read in the next raw index and increment the pointer ready
       * for the next cycle.
       *
       * Then multiply the ndx by the denormalising factor and add in
       * the offset.  */

      ndx = (pxndx[n] * xbmul) + offset;
      indx = (long) FLOOR((double)ndx);

      /* Limit = non-wrap.  Limits to 0 and (length - 1), or does the
       * wrap code.  See notes above in ktable().  */
      if (!p->wrap) {
        if (indx > length - 1)
          indx = length - 1;
        else if (indx < 0L)
          indx = 0L;
      }
      /* do the wrap code only if we are not doing the non-wrap code.  */
      else
        indx &= mask;
      rslt[n] = tab[indx];
    }
    return OK;
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
int ktabli(ENVIRON *csound, TABLE  *p)
{
    FUNC        *ftp;
    long        indx, length;
    MYFLT       v1, v2, fract, ndx;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("tablei(krate): not initialised"));
    }
    ndx = *p->xndx;
    length = ftp->flen;
    /* Multiply ndx by denormalisation factor.
     * xbmul is 1 or table length depending on state of ixmode.
     * Add in the offset, which has already been denormalised by
     * tblchk().  */

    ndx    = (ndx * p->xbmul) + p->offset;
    indx = (long) FLOOR((double)ndx);

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
      if (ndx > length) {
        indx  = length - 1;
        fract = FL(1.0);
      }
      else if (ndx < 0) {
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
}

int ktabl3(ENVIRON *csound, TABLE  *p)
{
    FUNC        *ftp;
    long        indx, length;
    MYFLT       v1, v2, fract, ndx;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("table3(krate): not initialised"));
    }
    ndx = *p->xndx;
    length = ftp->flen;
    /* Multiply ndx by denormalisation factor.
     * xbmul is 1 or table length depending on state of ixmode.
     * Add in the offset, which has already been denormalised by
     * tblchk().  */

    ndx    = (ndx * p->xbmul) + p->offset;
    indx = (long) FLOOR((double)ndx);

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
      if (ndx > length) {
        indx  = length - 1;
        fract = FL(1.0);
      }
      else if (ndx < 0) {
        indx  = 0L;
        fract = FL(0.0);
      }
    }
    /* We are in wrap mode, so do the wrap function.  */
    else        indx &= ftp->lenmask;

    /* interpolate with cubic if we can, else linear */
    if (indx<1 || indx==length-1 || length <4) {
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
}

/* tabli() */

/* tabli() is similar to ktabli() above, except that it processes an
 * array of input indexes, to send results to another array. */

int tabli(ENVIRON *csound, TABLE  *p)
{
    FUNC        *ftp;
    long        indx, mask, length;
    int         n, nsmps=csound->ksmps;
    MYFLT       *rslt, *pxndx, *tab;
    MYFLT       fract, v1, v2, ndx, xbmul, offset;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("tablei: not initialised"));
    }
    rslt   = p->rslt;
    length = ftp->flen;
    pxndx  = p->xndx;
    xbmul  = (MYFLT)p->xbmul;
    offset = p->offset;
    mask   = ftp->lenmask;
    tab    = ftp->ftable;
    /* As for ktabli() code to handle non wrap mode, and wrap mode.  */
    if (!p->wrap) {
      for (n=0; n<nsmps; n++) {
        /* Read in the next raw index and increment the pointer ready
         * for the next cycle.
         * Then multiply the ndx by the denormalising factor and add in
         * the offset.  */
        ndx = (pxndx[n] * xbmul) + offset;
        indx = (long) ndx;
        if (ndx <= FL(0.0)) {
          rslt[n] = tab[0];
          continue;
        }
        if (indx >= length) {
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
      for (n=0; n<nsmps; n++) {
        /* Read in the next raw index and increment the pointer ready
         * for the next cycle.
         * Then multiply the ndx by the denormalising factor and add in
         * the offset.  */
        ndx = (pxndx[n] * xbmul) + offset;
        indx = (long) FLOOR(ndx);
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
}

int tabl3(ENVIRON *csound, TABLE  *p)   /* Like tabli but cubic interpolation */
{
    FUNC        *ftp;
    long        indx, mask, length;
    int         n, nsmps=csound->ksmps;
    MYFLT       *rslt, *pxndx, *tab;
    MYFLT       fract, v1, v2, ndx, xbmul, offset;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("table3: not initialised"));
    }
    rslt = p->rslt;
    length = ftp->flen;
    pxndx = p->xndx;
    xbmul = (MYFLT)p->xbmul;
    offset = p->offset;
    mask = ftp->lenmask;
    tab = ftp->ftable;
    for (n=0; n<nsmps; n++) {
      /* Read in the next raw index and increment the pointer ready
       * for the next cycle.
       * Then multiply the ndx by the denormalising factor and add in
       * the offset.  */

      ndx = (pxndx[n] * xbmul) + offset;
      indx = (long) FLOOR((double)ndx);

      /* We need to generate a fraction - How much above indx is ndx?
       * It will be between 0 and just below 1.0.  */
      fract = ndx - indx;
      /* As for ktabli() code to handle non wrap mode, and wrap mode.  */
      if (!p->wrap) {
        if (ndx > length) {
          indx  = length - 1;
          fract = FL(1.0);
        }
        else if (ndx < 0) {
          indx  = 0L;
          fract = FL(0.0);
        }
      }
      else
        indx &= mask;
      /* interpolate with cubic if we can */
      if (indx <1 || indx == length-1 || length<4) {/* Too short or at ends */
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

int ftkrchk(ENVIRON *csound, TABLE *p)
{
    /* Check the table number is >= 1.  Print error and deactivate if
     * it is not.  Return NOTOK to tell calling function not to proceed
     * with a or k rate operations.
     *
     * We must do this to catch the situation where the first call has
     * a table number of 0, and since that equals pfn, we would
     * otherwise proceed without checking the table number - and none
     * of the pointers would have been set up.  */
    if (*p->xfn < 1) {
      return csound->PerfError(csound, Str("k rate function table no. %f < 1"),
                                       *p->xfn);
    }
    /* Check to see if table number has changed from previous value.
     * On the first run through, the previous value will be 0.  */

    if (p->pfn != (long)*p->xfn) {
        /* If it is different, check to see if the table exists.
         *
         * If it doesn't, an error message should be produced by
         * csoundFTFindP() which should also deactivate the instrument.
         *
         * Return 0 to tell calling function not to proceed with a or
         * k rate operations. */

      if ( (p->ftp = csound->FTFindP(csound, p->xfn) ) == NULL) {
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
      p->pfn = (long)*p->xfn;

        /* Set denormalisation factor to 1 or table length, depending
         * on the state of ixmode. */
      if (*p->ixmode)
        p->xbmul = p->ftp->flen;
      else    p->xbmul = 1L;

        /* Multiply the ixoff value by the xbmul denormalisation
         * factor and then check it is between 0 and the table length.  */

      if ((p->offset = p->xbmul * *p->ixoff) < FL(0.0) ||
          p->offset > p->ftp->flen) {
        return csound->PerfError(csound, Str("Offset %f < 0 or > tablelength"),
                                         p->offset);
      }
    }
    return OK;
}

/* Now for the four functions, which are called as a result of being
 * listed in opcodlst in entry.c */

int    ktablekt(ENVIRON *csound, TABLE *p)
{
    if (ftkrchk(csound,p)==OK) return ktable(csound,p);
    return NOTOK;
}

int    tablekt(ENVIRON *csound, TABLE *p)
{
    if (ftkrchk(csound,p)==OK) return tablefn(csound,p);
    return NOTOK;
}

int    ktablikt(ENVIRON *csound, TABLE *p)
{
    if (ftkrchk(csound,p)==OK) return ktabli(csound,p);
    return NOTOK;
}

int    tablikt(ENVIRON *csound, TABLE *p)
{
    if (ftkrchk(csound,p)==OK) return tabli(csound,p);
    return NOTOK;
}

int    ktabl3kt(ENVIRON *csound, TABLE *p)
{
    if (ftkrchk(csound,p)==OK) return ktabl3(csound,p);
    return NOTOK;
}

int    tabl3kt(ENVIRON *csound, TABLE *p)
{
    if (ftkrchk(csound,p)==OK) return tabl3(csound,p);
    return NOTOK;
}

int ko1set(ENVIRON *csound, OSCIL1 *p)
{
    FUNC        *ftp;

    if ((ftp = csound->FTFind(csound, p->ifn)) == NULL)
      return NOTOK;
    if (*p->idur <= FL(0.0)) {
      if (csound->oparms->msglevel & WARNMSG)
        csound->Message(csound, Str("WARNING: duration < zero\n"));
    }
    p->ftp = ftp;
    p->phs = 0;
    p->dcnt = (long)(*p->idel * csound->ekr);
    p->kinc = (long) (csound->kicvt / *p->idur);
    return OK;
}

int kosc1(ENVIRON *csound, OSCIL1 *p)
{
    FUNC *ftp;
    long  phs, dcnt;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil1(krate): not initialised"));
    }
    phs = p->phs;
    *p->rslt = *(ftp->ftable + (phs >> ftp->lobits)) * *p->kamp;
    if ((dcnt = p->dcnt) > 0L)
      dcnt--;
    else if (dcnt == 0L) {
      phs += p->kinc;
      if (phs >= MAXLEN) {
        phs = MAXLEN;
        dcnt--;
      }
      p->phs = phs;
    }
    p->dcnt = dcnt;
    return OK;
}

int kosc1i(ENVIRON *csound, OSCIL1  *p)
{
    FUNC        *ftp;
    MYFLT       fract, v1, *ftab;
    long        phs, dcnt;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil1i(krate): not initialised"));
    }
    phs = p->phs;
    fract = PFRAC(phs);
    ftab = ftp->ftable + (phs >> ftp->lobits);
    v1 = *ftab++;
    *p->rslt = (v1 + (*ftab - v1) * fract) * *p->kamp;
    if ((dcnt = p->dcnt) > 0L) {
      dcnt--;
      p->dcnt = dcnt;
    }
    else if (dcnt == 0L) {
      phs += p->kinc;
      if (phs >= MAXLEN) {
        phs = MAXLEN;
        dcnt--;
        p->dcnt = dcnt;
      }
      p->phs = phs;
    }
    return OK;
}

int oscnset(ENVIRON *csound, OSCILN *p)
{
    FUNC        *ftp;

    if ((ftp = csound->FTFind(csound, p->ifn)) != NULL) {
      p->ftp = ftp;
      p->inc = ftp->flen * *p->ifrq * csound->onedsr;
      p->index = FL(0.0);
      p->maxndx = ftp->flen - FL(1.0);
      p->ntimes = (long)*p->itimes;
      return OK;
    }
    else return NOTOK;
}

int osciln(ENVIRON *csound, OSCILN *p)
{
    MYFLT *rs = p->rslt;
    long  n, nsmps=csound->ksmps;

    if (p->ftp==NULL) {
      return csound->PerfError(csound, Str("osciln: not initialised"));
    }
    if (p->ntimes) {
      MYFLT *ftbl = p->ftp->ftable;
      MYFLT amp = *p->kamp;
      MYFLT ndx = p->index;
      MYFLT inc = p->inc;
      MYFLT maxndx = p->maxndx;
      for (n=0; n<nsmps; n++) {
        rs[n] = ftbl[(long)ndx] * amp;
        if ((ndx += inc) > maxndx) {
          if (--p->ntimes)
            ndx -= maxndx;
          else if (n==nsmps)
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
      for (; n<nsmps; n++) {
        rs[n] = FL(0.0);
      }
    }
    return OK;
}

int oscset(ENVIRON *csound, OSC *p)
{
    FUNC        *ftp;

    if ((ftp = csound->FTFind(csound, p->ifn)) != NULL) {
      p->ftp = ftp;
      if (*p->iphs >= 0)
        p->lphs = ((long)(*p->iphs * FMAXLEN)) & PHMASK;
      return OK;
    }
    return NOTOK;
}

int koscil(ENVIRON *csound, OSC *p)
{
    FUNC    *ftp;
    long    phs, inc;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil(krate): not initialised"));
    }
    phs = p->lphs;
    inc = (long) (*p->xcps * csound->kicvt);
    *p->sr = ftp->ftable[phs >> ftp->lobits] * *p->xamp;
    phs += inc;
    phs &= PHMASK;
    p->lphs = phs;
    return OK;
}

int osckk(ENVIRON *csound, OSC *p)
{
    FUNC    *ftp;
    MYFLT   amp, *ar, *ftbl;
    long    phs, inc, lobits;
    int     n, nsmps=csound->ksmps;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil: not initialised"));
    }
    ftbl = ftp->ftable;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * csound->sicvt);
    lobits = ftp->lobits;
    amp = *p->xamp;
    ar = p->sr;
    for (n=0;n<nsmps;n++) {
      ar[n] = ftbl[phs >> lobits] * amp;
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
}

int oscka(ENVIRON *csound, OSC *p)
{
    FUNC    *ftp;
    MYFLT   *ar, amp, *cpsp, *ftbl;
    long    phs, lobits;
    int     n, nsmps=csound->ksmps;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil: not initialised"));
    }
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    amp = *p->xamp;
    cpsp = p->xcps;
    phs = p->lphs;
    ar = p->sr;
    for (n=0;n<nsmps;n++) {
      long inc = MYFLT2LONG(cpsp[n] * csound->sicvt);
      ar[n] = ftbl[phs >> lobits] * amp;
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
}

int oscak(ENVIRON *csound, OSC *p)
{
    FUNC    *ftp;
    MYFLT   *ar, *ampp, *ftbl;
    long    phs, inc, lobits;
    int     n, nsmps=csound->ksmps;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil: not initialised"));
    }
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * csound->sicvt);
    ampp = p->xamp;
    ar = p->sr;
    for (n=0;n<nsmps;n++) {
      ar[n] = ftbl[phs >> lobits] * ampp[n];
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
}

int oscaa(ENVIRON *csound, OSC *p)
{
    FUNC    *ftp;
    MYFLT   *ar, *ampp, *cpsp, *ftbl;
    long    phs, lobits;
    int     n, nsmps=csound->ksmps;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil: not initialised"));
    }
    ftbl = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    ampp = p->xamp;
    cpsp = p->xcps;
    ar = p->sr;
    for (n=0;n<nsmps;n++) {
      long inc = MYFLT2LONG(cpsp[n] * csound->sicvt);
      ar[n] = ftbl[phs >> lobits] * ampp[n];
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
}

int koscli(ENVIRON *csound, OSC  *p)
{
    FUNC    *ftp;
    long    phs, inc;
    MYFLT  *ftab, fract, v1;

    phs = p->lphs;
    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscili(krate): not initialised"));
    }
    fract = PFRAC(phs);
    ftab = ftp->ftable + (phs >> ftp->lobits);
    v1 = ftab[0];
    *p->sr = (v1 + (ftab[1] - v1) * fract) * *p->xamp;
    inc = (long)(*p->xcps * csound->kicvt);
    phs += inc;
    phs &= PHMASK;
    p->lphs = phs;
    return OK;
}

int osckki(ENVIRON *csound, OSC  *p)
{
    FUNC    *ftp;
    MYFLT   fract, v1, amp, *ar, *ftab;
    long    phs, inc, lobits;
    int     n, nsmps=csound->ksmps;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscili: not initialised"));
    }
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * csound->sicvt);
    amp = *p->xamp;
    ar = p->sr;
    for (n=0;n<nsmps;n++) {
      fract = PFRAC(phs);
      ftab = ftp->ftable + (phs >> lobits);
      v1 = ftab[0];
      ar[n] = (v1 + (ftab[1] - v1) * fract) * amp;
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
}

int osckai(ENVIRON *csound, OSC  *p)
{
    FUNC    *ftp;
    MYFLT   *ar, amp, *cpsp, fract, v1, *ftab;
    long    phs, lobits;
    int     n, nsmps=csound->ksmps;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscili: not initialised"));
    }
    lobits = ftp->lobits;
    amp = *p->xamp;
    cpsp = p->xcps;
    phs = p->lphs;
    ar = p->sr;
    for (n=0;n<nsmps;n++) {
      long inc;
      inc = MYFLT2LONG(cpsp[n] * csound->sicvt);
      fract = PFRAC(phs);
      ftab = ftp->ftable + (phs >> lobits);
      v1 = ftab[0];
      ar[n] = (v1 + (ftab[1] - v1) * fract) * amp;
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
}

int oscaki(ENVIRON *csound, OSC  *p)
{
    FUNC    *ftp;
    MYFLT   v1, fract, *ar, *ampp, *ftab;
    long    phs, inc, lobits;
    int     n, nsmps=csound->ksmps;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscili: not initialised"));
    }
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * csound->sicvt);
    ampp = p->xamp;
    ar = p->sr;
    for (n=0;n<nsmps;n++) {
      fract = (MYFLT) PFRAC(phs);
      ftab = ftp->ftable + (phs >> lobits);
      v1 = ftab[0];
      ar[n] = (v1 + (ftab[1] - v1) * fract) * ampp[n];
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
}

int oscaai(ENVIRON *csound, OSC  *p)
{
    FUNC    *ftp;
    MYFLT   v1, fract, *ar, *ampp, *cpsp, *ftab;
    long    phs, lobits;
    int     n, nsmps=csound->ksmps;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscili: not initialised"));
    }
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    ampp = p->xamp;
    cpsp = p->xcps;
    ar = p->sr;
    for (n=0;n<nsmps;n++) {
      long inc;
      inc = MYFLT2LONG(cpsp[n] * csound->sicvt);
      fract = (MYFLT) PFRAC(phs);
      ftab = ftp->ftable + (phs >> lobits);
      v1 = *ftab++;
      ar[n] = (v1 + (*ftab - v1) * fract) * ampp[n];
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
}

int koscl3(ENVIRON *csound, OSC  *p)
{
    FUNC    *ftp;
    long    phs, inc;
    MYFLT  *ftab, fract;
    int     x0;
    MYFLT   y0, y1, ym1, y2;

    phs = p->lphs;
    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil3(krate): not initialised"));
    }
    ftab = ftp->ftable;
    fract = PFRAC(phs);
    x0 = (phs >> ftp->lobits);
    x0--;
    if (x0<0) {
      ym1 = ftab[ftp->flen-1]; x0 = 0;
    }
    else ym1 = ftab[x0++];
    y0 = ftab[x0++];
    y1 = ftab[x0++];
    if (x0>ftp->flen) y2 = ftab[1]; else y2 = ftab[x0];
    {
      MYFLT frsq = fract*fract;
      MYFLT frcu = frsq*ym1;
      MYFLT t1 = y2 + y0+y0+y0;
      *p->sr = y0 + FL(0.5)*frcu +
        fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0)) +
        frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) + frsq*(FL(0.5)* y1 - y0);
    }
    inc = (long)(*p->xcps * csound->kicvt);
    phs += inc;
    phs &= PHMASK;
    p->lphs = phs;
    return OK;
}

int osckk3(ENVIRON *csound, OSC  *p)
{
    FUNC    *ftp;
    MYFLT   fract, amp, *ar, *ftab;
    long    phs, inc, lobits;
    int     n, nsmps=csound->ksmps;
    int     x0;
    MYFLT   y0, y1, ym1, y2;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil3: not initialised"));
    }
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * csound->sicvt);
    amp = *p->xamp;
    ar = p->sr;
    for(n=0;n<nsmps;n++) {
      fract = PFRAC(phs);
      x0 = (phs >> lobits);
      x0--;
      if (x0<0) {
        ym1 = ftab[ftp->flen-1]; x0 = 0;
      }
      else ym1 = ftab[x0++];
      y0 = ftab[x0++];
      y1 = ftab[x0++];
      if (x0>ftp->flen) y2 = ftab[1]; else y2 = ftab[x0];
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
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
}

int oscka3(ENVIRON *csound, OSC  *p)
{
    FUNC    *ftp;
    MYFLT   *ar, amp, *cpsp, fract, *ftab;
    long    phs, lobits;
    int     n, nsmps=csound->ksmps;
    int     x0;
    MYFLT   y0, y1, ym1, y2;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscil3: not initialised"));
    }
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    amp = *p->xamp;
    cpsp = p->xcps;
    phs = p->lphs;
    ar = p->sr;
    for(n=0;n<nsmps;n++) {
      long inc;
      inc = MYFLT2LONG(cpsp[n] * csound->sicvt);
      fract = PFRAC(phs);
      x0 = (phs >> lobits);
      x0--;
      if (x0<0) {
        ym1 = ftab[ftp->flen-1]; x0 = 0;
      }
      else ym1 = ftab[x0++];
      y0 = ftab[x0++];
      y1 = ftab[x0++];
      if (x0>ftp->flen) y2 = ftab[1]; else y2 = ftab[x0];
      {
        MYFLT frsq = fract*fract;
        MYFLT frcu = frsq*ym1;
        MYFLT t1 = y2 + y0+y0+y0;
        ar[n] = amp * (y0 + FL(0.5)*frcu +
                       fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0)) +
                       frsq*fract*(t1/FL(6.0) - FL(0.5)*y1) + frsq*(FL(0.5)*
                                                                    y1 - y0));
      }
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
}

int oscak3(ENVIRON *csound, OSC  *p)
{
    FUNC    *ftp;
    MYFLT   fract, *ar, *ampp, *ftab;
    long    phs, inc, lobits;
    int     n, nsmps=csound->ksmps;
    int     x0;
    MYFLT   y0, y1, ym1, y2;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscili: not initialised"));
    }
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    inc = MYFLT2LONG(*p->xcps * csound->sicvt);
    ampp = p->xamp;
    ar = p->sr;
    for (n=0;n<nsmps;n++) {
      fract = (MYFLT) PFRAC(phs);
      x0 = (phs >> lobits);
      x0--;
      if (x0<0) {
        ym1 = ftab[ftp->flen-1]; x0 = 0;
      }
      else ym1 = ftab[x0++];
      y0 = ftab[x0++];
      y1 = ftab[x0++];
      if (x0>ftp->flen) y2 = ftab[1]; else y2 = ftab[x0];
      {
        MYFLT frsq = fract*fract;
        MYFLT frcu = frsq*ym1;
        MYFLT t1 = y2 + y0+y0+y0;
        ar[n] = ampp[n] *(y0 + FL(0.5)*frcu
                          + fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0))
                          + frsq*fract*(t1/FL(6.0) - FL(0.5)*y1)
                          + frsq*(FL(0.5)* y1 - y0));
      }
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
}

int oscaa3(ENVIRON *csound, OSC  *p)
{
    FUNC    *ftp;
    MYFLT   fract, *ar, *ampp, *cpsp, *ftab;
    long    phs, lobits;
    int     n, nsmps=csound->ksmps;
    int     x0;
    MYFLT   y0, y1, ym1, y2;

    ftp = p->ftp;
    if (ftp==NULL) {
      return csound->PerfError(csound, Str("oscili: not initialised"));
    }
    ftab = ftp->ftable;
    lobits = ftp->lobits;
    phs = p->lphs;
    ampp = p->xamp;
    cpsp = p->xcps;
    ar = p->sr;
    for (n=0;n<nsmps;n++) {
      long inc;
      inc = MYFLT2LONG(cpsp[n] * csound->sicvt);
      fract = (MYFLT) PFRAC(phs);
      x0 = (phs >> lobits);
      x0--;
      if (x0<0) {
        ym1 = ftab[ftp->flen-1]; x0 = 0;
      }
      else ym1 = ftab[x0++];
      y0 = ftab[x0++];
      y1 = ftab[x0++];
      if (x0>ftp->flen) y2 = ftab[1]; else y2 = ftab[x0];
      {
        MYFLT frsq = fract*fract;
        MYFLT frcu = frsq*ym1;
        MYFLT t1 = y2 + y0+y0+y0;
        ar[n] = ampp[n] *(y0 + FL(0.5)*frcu
                          + fract*(y1 - frcu/FL(6.0) - t1/FL(6.0) - ym1/FL(3.0))
                          + frsq*fract*(t1/FL(6.0) - FL(0.5)*y1)
                          + frsq*(FL(0.5)* y1 - y0));
      }
      phs += inc;
      phs &= PHMASK;
    }
    p->lphs = phs;
    return OK;
}

