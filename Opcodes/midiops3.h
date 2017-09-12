/*
    midiops3.h:

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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
*/

typedef struct {
    MYFLT *ictlno, *imin, *imax, *initvalue, *ifn;
} SLD;

typedef struct {
    OPDS   h;
    MYFLT  *r[8];             /* output */
    MYFLT  *ichan;              /* input */
    SLD    s[8];
    MYFLT  min[8], max[8];
    unsigned char   slchan, slnum[8];
    FUNC   *ftp[8];
} SLIDER8;

typedef struct {
    OPDS   h;
    MYFLT  *r[16];             /* output */
    MYFLT  *ichan;              /* input */
    SLD    s[16];
    MYFLT  min[16], max[16];
    unsigned char   slchan, slnum[16];
    FUNC   *ftp[16];
} SLIDER16;

typedef struct {
    OPDS   h;
    MYFLT  *r[32];             /* output */
    MYFLT  *ichan;              /* input */
    SLD    s[32];
    MYFLT  min[32], max[32];
    unsigned char   slchan, slnum[32];
    FUNC   *ftp[32];
} SLIDER32;

typedef struct {
    OPDS   h;
    MYFLT  *r[64];             /* output */
    MYFLT  *ichan;              /* input */
    SLD    s[64];
    MYFLT  min[64], max[64];
    unsigned char   slchan, slnum[64];
    FUNC   *ftp[64];
} SLIDER64;

/*=============================*/

typedef struct {
    MYFLT *ictlno, *imin, *imax, *initvalue, *ifn, *ihp;
} SLDf;

typedef struct {
    OPDS   h;
    MYFLT  *r[8];              /* output */
    MYFLT  *ichan;         /* input */
    SLDf   s[8];
    MYFLT  min[8], max[8];
    unsigned char   slchan, slnum[8];
    FUNC   *ftp[8];
    MYFLT  c1[8], c2[8];
    MYFLT  yt1[8];
} SLIDER8f;

typedef struct {
    OPDS   h;
    MYFLT  *r[16];              /* output */
    MYFLT  *ichan;         /* input */
    SLDf   s[16];
    MYFLT  min[16], max[16];
    unsigned char   slchan, slnum[16];
    FUNC   *ftp[16];
    MYFLT  c1[16], c2[16];
    MYFLT  yt1[16];
} SLIDER16f;

typedef struct {
    OPDS   h;
    MYFLT  *r[32];              /* output */
    MYFLT  *ichan;         /* input */
    SLDf   s[32];
    MYFLT  min[32], max[32];
    unsigned char   slchan, slnum[32];
    FUNC   *ftp[32];
    MYFLT  c1[32], c2[32];
    MYFLT  yt1[32];
} SLIDER32f;

typedef struct {
    OPDS   h;
    MYFLT  *r[64];              /* output */
    MYFLT  *ichan;         /* input */
    SLDf   s[64];
    MYFLT  min[64], max[64];
    unsigned char   slchan, slnum[64];
    FUNC   *ftp[64];
    MYFLT  c1[64], c2[64];
    MYFLT  yt1[64];
} SLIDER64f;

/*---------------------*/

typedef struct {
    MYFLT *ictlno, *imin, *imax, *ifn;
} ISLD;

typedef struct {
    OPDS   h;
    MYFLT  *r[8];              /* output */
    MYFLT  *ichan;              /* input */
    ISLD   s[8];
} ISLIDER8;

typedef struct {
    OPDS   h;
    MYFLT  *r[16];              /* output */
    MYFLT  *ichan;              /* input */
    ISLD   s[16];
} ISLIDER16;

typedef struct {
    OPDS   h;
    MYFLT  *r[32];              /* output */
    MYFLT  *ichan;              /* input */
    ISLD   s[32];
} ISLIDER32;

typedef struct {
    OPDS   h;
    MYFLT  *r[64];              /* output */
    MYFLT  *ichan;              /* input */
    ISLD   s[64];
} ISLIDER64;

/*------------------------------*/

typedef struct {
    MYFLT *ictlno_msb, *ictlno_lsb, *imin, *imax, *initvalue, *ifn;
} SLD14;

typedef struct {
    OPDS   h;
    MYFLT  *r[16];              /* output */
    MYFLT  *ichan;              /* input */
    SLD14  s[16];
    MYFLT  min[16], max[16];
    unsigned char   slchan, slnum_msb[16],slnum_lsb[16];
    FUNC   *ftp[16];
} SLIDER16BIT14;

typedef struct {
    OPDS   h;
    MYFLT  *r[32];              /* output */
    MYFLT  *ichan;              /* input */
    SLD14  s[32];
    MYFLT  min[32], max[32];
    unsigned char   slchan, slnum_msb[32],slnum_lsb[32];
    FUNC   *ftp[32];
} SLIDER32BIT14;
/*--------------------*/

typedef struct {
    MYFLT *ictlno_msb, *ictlno_lsb, *imin, *imax, *ifn;
} ISLD14;

typedef struct {
    OPDS   h;
    MYFLT  *r[16];             /* output */
    MYFLT  *ichan;              /* input */
    ISLD14 s[16];
} ISLIDER16BIT14;

typedef struct {
    OPDS   h;
    MYFLT  *r[32];             /* output */
    MYFLT  *ichan;              /* input */
    ISLD14 s[32];
} ISLIDER32BIT14;
