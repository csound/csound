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
