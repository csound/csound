/*
    ugens2a.c:

    Copyright (C) 2012 John ffitch
    Based in part on code from Vercoe and Whittle

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

#include "csoundCore.h" /*                              UGENS2.C        */
#include "ugens2.h"
#include "ugrw1.h"
#include <math.h>

#define MYFLOOR(x) (x >= FL(0.0) ? (int32)x : (int32)((double)x - 0.99999999))

/*****************************************************************************/

/* Table read code - see TABLE data structure in ugens2.h.  */

/*************************************/

static int itblchk(CSOUND *csound, TABLE *p)
{
    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->xfn)) == NULL))
      return csound->InitError(csound, Str("table does not exist"));

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

    p->wrap   = (int)*p->iwrap;
    return OK;
}

int tblset(CSOUND *csound, TABLE *p)
{
    if (UNLIKELY(IS_ASIG_ARG(p->rslt) != IS_ASIG_ARG(p->xndx))) {
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


int pktable(CSOUND *,TABLE*);
int pktabli(CSOUND *,TABLE*);
int pktabl3(CSOUND *,TABLE*);

int pitable(CSOUND *csound, TABLE *p)
{
    if (LIKELY(itblchk(csound,p)==OK)) return pktable(csound,p);
    return NOTOK;
}

int pitabli(CSOUND *csound, TABLE *p)
{
    if (LIKELY(itblchk(csound,p)==OK)) return pktabli(csound,p);
    return NOTOK;
}

int pitabl3(CSOUND *csound, TABLE *p)
{
    if (LIKELY(itblchk(csound,p)==OK)) return pktabl3(csound,p);
    return NOTOK;
}

/*---------------------------------------------------------------------------*/

/* Functions which read the table. */

int pktable(CSOUND *csound, TABLE   *p)
{
    FUNC        *ftp;
    int32        indx, length;
    MYFLT       ndx;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;            /* RWD fix */
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
     indx = (int32) MYFLOOR((double)ndx);

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
    /* The following code used to use an AND with an integer like 0000 0111
     * to wrap the current index within the range of the table.  In
     * the original version, this code always ran, but with the new
     * (length - 1) code above, it would have no effect, so it is now
     * an else operation - running only when iwrap = 1.  This may save
     * half a usec or so.  */
    /* Now safe against non power-of-2 tables */
    else if (indx>=length) indx = indx % length;
    else if (indx<0) indx = length - (-indx)%length;

    /* Now find the address of the start of the table, add it to the
     * index, read the value from there and write it to the
     * destination.  */
    *p->rslt = *(ftp->ftable + indx);
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("ptable(krate): not initialised"));
}

/* ptablefn()  */

int ptablefn(CSOUND *csound, TABLE *p)
{
    FUNC        *ftp;
    MYFLT       *rslt, *pxndx, *tab;
    int32       indx, length;
    uint32_t koffset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       ndx, xbmul, offset;
    int         wrap = p->wrap;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;            /* RWD fix */
    rslt = p->rslt;
    if (koffset) memset(rslt, '\0', koffset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    length = ftp->flen;
    pxndx = p->xndx;
    xbmul = (MYFLT)p->xbmul;
    offset = p->offset;
    //mask = ftp->lenmask;
    tab = ftp->ftable;
    for (n=koffset; n<nsmps; n++) {
      /* Read in the next raw index and increment the pointer ready
       * for the next cycle.
       *
       * Then multiply the ndx by the denormalising factor and add in
       * the offset.  */

      ndx = (pxndx[n] * xbmul) + offset;
      indx = (int32) MYFLOOR((double)ndx);

      /* Limit = non-wrap.  Limits to 0 and (length - 1), or does the
       * wrap code.  See notes above in ktable().  */
      if (!wrap) {
        if (UNLIKELY(indx >= length))
          indx = length - 1;
        else if (UNLIKELY(indx < (int32)0))
          indx = (int32)0;
      }
      /* do the wrap code only if we are not doing the non-wrap code.  */
      else if (indx >= length) indx %= length;
      else if (indx < 0) indx = length - (-indx)%length;
      rslt[n] = tab[indx];
    }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("table: not initialised"));
}

/* pktabli() */

int pktabli(CSOUND *csound, TABLE   *p)
{
    FUNC        *ftp;
    int32       indx, length;
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
    indx = (int32) MYFLOOR((double)ndx);

    /* We need to generate a fraction - How much above indx is ndx?
     * It will be between 0 and just below 1.0.  */
    fract = ndx - indx;

    if (!p->wrap) {
      if (UNLIKELY(ndx >= length)) {
        indx  = length - 1;
        fract = FL(1.0);
      }
      else if (UNLIKELY(ndx < 0)) {
        indx  = 0L;
        fract = FL(0.0);
      }
    }
    /* We are in wrap mode, so do the wrap function.  */
    else if (indx>=length) indx %= length;
    else if (indx<0) indx = length - (-indx)%length;

    /* Now read the value at indx and the one beyond */
    v1 = *(ftp->ftable + indx);
    indx++;
    if (indx>=length) indx -= length;
    v2 = *(ftp->ftable + indx);
    *p->rslt = v1 + (v2 - v1) * fract;
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("ptablei(krate): not initialised"));
}


int pktabl3(CSOUND *csound, TABLE   *p)
{
    FUNC        *ftp;
    int32        indx, length;
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
    indx = (int32) MYFLOOR((double)ndx);

    /* We need to generate a fraction - How much above indx is ndx?
     * It will be between 0 and just below 1.0.  */
    fract = ndx - indx;

    if (!p->wrap) {
      if (UNLIKELY(ndx >= length)) {
        indx  = length - 1;
        fract = FL(1.0);
      }
      else if (UNLIKELY(ndx < 0)) {
        indx  = 0L;
        fract = FL(0.0);
      }
    }
    /* We are in wrap mode, so do the wrap function.  */
    else if (indx>=length) indx %= length;
    else if (indx<0) indx = length - (-indx)%length;

    /* interpolate with cubic if we can, else linear */
    if (UNLIKELY(indx<1 || indx==length-2 || length <4)) {
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
    return csound->PerfError(csound, p->h.insdshead,
                             Str("ptable3(krate): not initialised"));
}

int ptabli(CSOUND *csound, TABLE   *p)
{
    FUNC        *ftp;
    int32       indx, length;
    uint32_t koffset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       *rslt, *pxndx, *tab;
    MYFLT       fract, v1, v2, ndx, xbmul, offset;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    rslt   = p->rslt;
    if (koffset) memset(rslt, '\0', koffset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    length = ftp->flen;
    pxndx  = p->xndx;
    xbmul  = (MYFLT)p->xbmul;
    offset = p->offset;
    //mask   = ftp->lenmask;
    tab    = ftp->ftable;
    /* As for ktabli() code to handle non wrap mode, and wrap mode.  */
    if (!p->wrap) {
      for (n=koffset; n<nsmps; n++) {
        /* Read in the next raw index and increment the pointer ready
         * for the next cycle.
         * Then multiply the ndx by the denormalising factor and add in
         * the offset.  */
        ndx = (pxndx[n] * xbmul) + offset;
        indx = (int32) ndx;
        if (UNLIKELY(ndx <= FL(0.0))) {
          rslt[n] = tab[0];
          continue;
        }
        if (UNLIKELY(indx >= length)) {
          rslt[n] = tab[length-1];
          continue;
        }
        /* We need to generate a fraction - How much above indx is ndx?
         * It will be between 0 and just below 1.0.  */
        fract = ndx - indx;
        /* As for ktabli(), read two values and interpolate between
         * them.  */
        v1 = tab[indx];
        indx++;
        if (indx>=length) indx = length-1;
        v2 = tab[indx];
        rslt[n] = v1 + (v2 - v1)*fract;
      }
    }
    else {                      /* wrap mode */
      for (n=koffset; n<nsmps; n++) {
        int j;
        /* Read in the next raw index and increment the pointer ready
         * for the next cycle.
         * Then multiply the ndx by the denormalising factor and add in
         * the offset.  */
        ndx = (pxndx[n] * xbmul) + offset;
        indx = (int32) MYFLOOR(ndx);
        /* We need to generate a fraction - How much above indx is ndx?
         * It will be between 0 and just below 1.0.  */
        fract = ndx - indx;
        if (indx >= length) indx %= length;
        else if (indx<0) indx = length-(-indx)%length;
        /* As for ktabli(), read two values and interpolate between
         * them.  */
        v1 = tab[indx];
        j = indx + 1;
        if (j >= length) j -= length;
        v2 = tab[j];
        rslt[n] = v1 + (v2 - v1)*fract;
      }
    }
    return OK;
 err1:
    return csound->PerfError(csound, p->h.insdshead,
                             Str("ptablei: not initialised"));
}

int ptabl3(CSOUND *csound, TABLE *p)     /* Like ptabli but cubic interpolation */
{
    FUNC        *ftp;
    int32       indx, length;
    uint32_t    koffset = p->h.insdshead->ksmps_offset;
    uint32_t    early  = p->h.insdshead->ksmps_no_end;
    uint32_t    n, nsmps = CS_KSMPS;
    MYFLT       *rslt, *pxndx, *tab;
    MYFLT       fract, v1, v2, ndx, xbmul, offset;
    int         wrap = p->wrap;

    ftp = p->ftp;
    if (UNLIKELY(ftp==NULL)) goto err1;
    rslt = p->rslt;
    if (koffset) memset(rslt, '\0', koffset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&rslt[nsmps], '\0', early*sizeof(MYFLT));
    }
    length = ftp->flen;
    pxndx = p->xndx;
    xbmul = (MYFLT)p->xbmul;
    offset = p->offset;
    tab = ftp->ftable;
    for (n=koffset; n<nsmps; n++) {
      /* Read in the next raw index and increment the pointer ready
       * for the next cycle.
       * Then multiply the ndx by the denormalising factor and add in
       * the offset.  */

      ndx = (pxndx[n] * xbmul) + offset;
      indx = (int32) MYFLOOR((double)ndx);

      /* We need to generate a fraction - How much above indx is ndx?
       * It will be between 0 and just below 1.0.  */
      fract = ndx - indx;
      /* As for pktabli() code to handle non wrap mode, and wrap mode.  */
      if (!wrap) {
        if (UNLIKELY(ndx >= length)) {
          indx  = length - 1;
          fract = FL(1.0);
        }
        else if (UNLIKELY(ndx < 0)) {
          indx  = 0L;
          fract = FL(0.0);
        }
      }
      else if (UNLIKELY(indx>=length)) indx %= length;
      else if (UNLIKELY(indx<0)) indx = length-(-indx)%length;
      /* interpolate with cubic if we can */
      if (UNLIKELY(indx <1 || indx == length-2 || length<4)) {
        /* Too short or at ends */
        v1 = tab[indx];
        v2 = tab[indx + 1];
        rslt[n] = v1 + (v2 - v1)*fract;
      }
      else {
        MYFLT ym1 = tab[indx-1], y0 = tab[indx];
        int j = (indx+1<length ? indx+1 : indx+1-length);
        int k = (indx+2<length ? indx+2 : indx+2-length);
        MYFLT y1 = tab[j], y2 = tab[k];
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
    return csound->PerfError(csound, p->h.insdshead,
                             Str("ptable3: not initialised"));
}

int itblchkw(CSOUND *csound, TABLEW *p)
{
    /* Get pointer to the function table data structure of the table
     * number specified in xfn. Return 0 if it cannot be found.
     *
     * csoundFTFind() generates an error message if the table cannot be
     * found. This works OK at init time.  It also deactivates the
     * instrument.
     *
     * It also checks for numbers < 0, and table 0 is never valid, so we
     * do not need to check here for the table number being < 1.  */

    if (UNLIKELY((p->ftp = csound->FTnp2Find(csound, p->xfn)) == NULL))
      return NOTOK;
    /* Although TABLEW has an integer variable for the table number
     * (p->pfn) we do not need to * write it.  We know that the * k
     * and a rate functions * which will follow will not * be
     * expecting a changed * table number.
     *
     * p->pfn exists only for checking * table number changes for
     * functions * which are expecting a k rate * table number.  */

    /* Set denormalisation factor to 1 or table length, depending on
     * the state of ixmode.  1L means a 32 bit 1.  */
    /* JPff.............................^...not so; could be any length */
    if (*p->ixmode)
            p->xbmul = p->ftp->flen;
    else    p->xbmul = 1L;
    /* Multiply the ixoff value by the xbmul denormalisation
     * factor and then check it is between 0 and the table length.  */
      if (UNLIKELY((p->offset = p->xbmul * *p->ixoff) < FL(0.0)
                   || p->offset > p->ftp->flen)) {
      return csound->InitError(csound,
                               Str("Table write offset %f < 0 or > tablelength"),
                               p->offset);
    }
    p->iwgm   = (int)*p->iwgmode;
    return OK;
}


int pktablew(CSOUND *, TABLEW*);
int pitablew(CSOUND *csound, TABLEW *p)
{
    if (LIKELY(itblchkw(csound, p) == OK))
      return pktablew(csound, p);
    return NOTOK;
}

/*---------------------------------------------------------------------------*/

/* pktablew is called with p pointing to the TABLEW data structure -
 * which contains the input arguments.  */

int pktablew(CSOUND *csound, TABLEW   *p)
{
/* Pointer to data structure for accessing the table we will be
 * writing to.
 */
    FUNC        *ftp;
    int32        indx, length;
    MYFLT       ndx;            /*  for calculating index of read.  */
    MYFLT       *ptab;          /* Where we will write */

    /*-----------------------------------*/
    /* Assume that TABLEW has been set up correctly.  */

    ftp    = p->ftp;
    ndx    = *p->xndx;
    length = ftp->flen;
    /* Multiply ndx by denormalisation factor.  and add in the
     * offset - already denormalised - by tblchkw().
     * xbmul = 1 or table length depending on state of ixmode.  */

    ndx = (ndx * p->xbmul) + p->offset;

    /* ndx now includes the offset and is ready to address the table.
     * However we have three modes to consider:
     * igwm = 0     Limit mode.
     *        1     Wrap mode.
     *        2     Guardpoint mode.
     */
    if (p->iwgm == 0) {
      /* Limit mode - when igmode = 0.
       *
       * Limit the final index to 0 and the last location in the table.
       */
      indx = (int32) MYFLOOR(ndx); /* Limit to (table length - 1) */
      if (UNLIKELY(indx > length - 1))
        indx = length - 1;      /* Limit the high values. */
      else if (UNLIKELY(indx < 0L)) indx = 0L; /* limit negative values to zero. */
    }
    /* Wrap and guard point mode.
     * In guard point mode only, add 0.5 to the index. */
    else {
      if (p->iwgm == 2) ndx += FL(0.5);
      indx = (int32) MYFLOOR(ndx);

      /* Both wrap and guard point mode.
       * The following code uses an AND with an integer like 0000 0111 to wrap
       * the current index within the range of the table. */
      if (UNLIKELY(indx>=length)) indx %= length;
      else if (UNLIKELY(indx<0)) indx = length-(-indx)%length;
    }
                                /* Calculate the address of where we
                                 * want to write to, from indx and the
                                 * starting address of the table.
                                 */
    ptab = ftp->ftable + indx;
    *ptab = *p->xsig;           /* Write the input value to the table. */
                                /* If this is guard point mode and we
                                 * have just written to location 0,
                                 * then also write to the guard point.
                                 */
    if ((p->iwgm == 2) && indx == 0) { /* Fix -- JPff 2000/1/5 */
      ptab += ftp->flen;
      *ptab = *p->xsig;
    }
    return OK;
}

/*---------------------------------------------------------------------------*/

/* tablew() is similar to ktablew()  above, except that it processes
 * two arrays of input values and indexes.  These arrays are ksmps long. */
int ptablew(CSOUND *csound, TABLEW *p)
{
    FUNC        *ftp;   /* Pointer to function table data structure. */
    MYFLT       *psig;  /* Array of input values to be written to table. */
    MYFLT       *pxndx; /* Array of input index values */
    MYFLT       *ptab;  /* Pointer to start of table we will write. */
    MYFLT       *pwrite;/* Pointer to location in table where we will write */
    int32        indx;   /* Used to read table. */
    int32        length; /* Length of table */
    int         liwgm;          /* Local copy of iwgm for speed */
    uint32_t koffset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    MYFLT       ndx, xbmul, offset;
                                /*-----------------------------------*/
    /* Assume that TABLEW has been set up correctly. */

    ftp    = p->ftp;
    psig   = p->xsig;
    pxndx  = p->xndx;
    ptab   = ftp->ftable;
    length = ftp->flen;
    liwgm  = p->iwgm;
    xbmul  = (MYFLT)p->xbmul;
    offset = p->offset;
                /* Main loop - for the number of a samples in a k cycle. */
    nsmps -= early;
    for (n=koffset; n<nsmps; n++) {
      /* Read in the next raw index and increment the pointer ready for the
         next cycle.  Then multiply the ndx by the denormalising factor and
         add in the offset.  */
      ndx = (pxndx[n] * xbmul) + offset;
      if (liwgm == 0) {         /* Limit mode - when igmode = 0. */
        indx = (int32) MYFLOOR(ndx);
        if (UNLIKELY(indx > length - 1)) indx = length - 1;
        else if (UNLIKELY(indx < 0L)) indx = 0L;
      }
      else {
        if (liwgm == 2) ndx += FL(0.5);
        indx = (int32) MYFLOOR(ndx);
        /* Both wrap and guard point mode. */
        if (UNLIKELY(indx>=length)) indx %= length;
        else if (UNLIKELY(indx<0)) indx = length-(-indx)%length;
      }
      pwrite = ptab + indx;
      *pwrite = psig[n];
                                        /* If this is guard point mode and we
                                         * have just written to location 0,
                                         * then also write to the guard point.
                                         */
      if ((liwgm == 2) && indx == 0) {  /* Fix -- JPff 2000/1/5 */
                                        /* Note that since pwrite is a pointer
                                         * to a float, adding length to it
                                         * adds (4 * length) to its value since
                                         * the length of a float is 4 bytes.
                                         */
        pwrite += length;
                                        /* Decrement psig to make it point
                                         * to the same input value.
                                         * Write to guard point.
                                         */
        *pwrite = psig[n];
      }
    }
    return OK;
}
