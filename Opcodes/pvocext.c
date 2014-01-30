/*
    pvocext.c:

    Copyright (C) 1998 Richard Karpen

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

/******************************************/
/* The applications in this file were     */
/* designed and coded by Richard Karpen   */
/* University of Washington, Seattle 1998 */
/******************************************/

/*    PVOCEXT.C        */

#include "pvoc.h"
#include <math.h>

#define minval(val1, val2) (val1 <= val2 ? val1 : val2)

/* Spectral Extraction.  Based on ideas from Tom Erbe's SoundHack  */

void SpectralExtract(
    float   *inp,       /* pointer to input data */
    float   *pvcopy,
    int32    fsize,      /* frame size we're working with */
    int32    MaxFrame,
    int     mode,
    MYFLT   freqlim
    )
{
    int32    i, j, k;
    float   *frm_1;
    int32    ampindex, freqindex;
    MYFLT   freqTemp, freqframes[10]={0.0}, freqdiff=FL(0.0), ampscale;
    int32            framecurb;

    memcpy(pvcopy, inp, (fsize+2L)*MaxFrame*sizeof(float));
    frm_1 = pvcopy;
    for (j=0; j<(fsize/2L + 1L); j++) {
      ampindex = j + j;
      freqindex = ampindex + 1L;
      for (i=0; i<MaxFrame; i++) {
        framecurb = minval(6, MaxFrame-i);
        freqdiff = FL(0.0);
        /* get frequencies from 6 or less consecutive frames */
        for (k=0; k<=framecurb; k++)
          freqframes[k] = *(frm_1 + freqindex + ((fsize+2L)*k) +
                            ((fsize+2L)*i));

        /* average the deviation over framecurb interframe periods */
        for (k=0; k<framecurb; k++) {
          freqTemp = (MYFLT)fabs(freqframes[k] - freqframes[k+1L]);
          freqdiff += freqTemp * (FL(1.0)/(MYFLT)framecurb);
        }

        if (mode==1) { /* lets through just the "noisy" parts */
          if (freqdiff > freqlim && freqdiff < freqlim * 2) {
            ampscale = (freqdiff - freqlim) / freqlim;
            frm_1[ampindex+((fsize+2L)*i)] *= ampscale;
          }
          else if (freqdiff <= freqlim)
            frm_1[ampindex+((fsize+2L)*i)] = FL(0.0);
        }
        else if (mode==2) { /* lets through just the stable-pitched parts */
          if (freqdiff < freqlim) {
            ampscale = (freqlim - freqdiff) / freqlim;
            frm_1[ampindex+((fsize+2L)*i)] *= ampscale;
          }
          else
            frm_1[ampindex+((fsize+2L)*i)] = FL(0.0);
        }
      }
    }
}

MYFLT PvocMaxAmp(
    float   *inp,       /* pointer to input data */
    int32    fsize,      /* frame size we're working with */
    int32    MaxFrame
    )
{
    int32    j, k;
    float   *frm_0, *frmx;
    int32    ampindex;
    MYFLT   MaxAmpInData = FL(0.0);

    frm_0 = inp;

/* find max amp in the whole pvoc file */
    for (j=0; j<(fsize/2L + 1L); ++j) {
      ampindex = j + j;
      for (k=0; k<=MaxFrame; k++) {
        frmx = frm_0 + ((fsize+2L)*k);
        MaxAmpInData = (frmx[ampindex] > MaxAmpInData ?
                        frmx[ampindex] : MaxAmpInData);
      }
    }
    return(MaxAmpInData);
}

/*********************************************************************/
/* Different from Tom Erbe's Amplitude Gating. This one maps         */
/* the normalised amplitude values from the analysis bins onto       */
/* a user defined function. The amplitude values which are           */
/* normalised to be between 0 and 1 are used as indeces into         */
/* the table where and amplitude of 0 points at the beginning        */
/* of the table and an amplitude of 1 points to the end of the table */
/*********************************************************************/

void PvAmpGate(
    MYFLT   *buf,       /* where to get our mag/pha pairs */
    int32    fsize,      /* frame size we're working with */
    FUNC    *ampfunc,
    MYFLT   MaxAmpInData
    )
{
    int32    j;
    int32    ampindex, funclen, mapPoint;

    funclen = ampfunc->flen;

    for (j=0; j<(fsize/2L + 1L); ++j) {
      ampindex = j + j;
      /* use normalised amp as index into table for amp scaling */
      mapPoint = (int32)((buf[ampindex] / MaxAmpInData) * funclen);
      buf[ampindex] *= *(ampfunc->ftable + mapPoint);
    }
}

