/*
    convolve.h:

    Copyright (C) 1996 Greg Sullivan

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

#ifndef _CONVOLVE_H_
#define _CONVOLVE_H_

#define CONVOLVE_VERSION_STRING "CONVOLVE VERSION: V1.1\n"

#ifndef NULL
#define NULL 0L
#endif /* !NULL */

#define CVMAGIC 666     /* Evil, eh? */

#define CVDFLTBYTS 4

typedef struct cvstruct
{
    int32        magic;                  /* magic number to identify */
    int32        headBsize;              /* byte offset from start to data */
    int32        dataBsize;              /* total number of bytes of data */
    int32        dataFormat;             /* (int) format specifier */
    MYFLT        samplingRate;           /* of original sample */
    int32        src_chnls;              /* no. of channels in source */
    int32        channel;                /* requested channel(s) */
    int32        Hlen;                   /* length of impulse reponse */
    int32        Format;                 /* (int) how words are org'd in frm */
    char         info[CVDFLTBYTS];       /* extendable byte area */
} CVSTRUCT;

/* Error codes returned by CONVOLVE file functions */
#define CVE_OK          0       /* no error*/
#define CVE_NOPEN       -1      /* couldn't open file */
#define CVE_NCV         -2      /* not a CONVOLVE file */
#define CVE_MALLOC      -3      /* couldn't allocate memory */
#define CVE_RDERR       -4      /* read error */
#define CVE_WRERR       -5      /* write error */

#define CV_UNK_LEN      -1L     /* flag if dataBsize unknown in hdr */

/* values for dataFormat field */
#define CVMYFLT (4+32)  /* 32 bit float data */

/* values for frameFormat field */
#define CVRECT  1       /* real, imag pairs */

/********************************/
/* exported function prototypes */
/********************************/

#endif /* !_CONVOLVE_H_ */
