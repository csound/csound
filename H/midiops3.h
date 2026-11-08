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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#pragma once


typedef struct {
    cs_float *ictlno, *imin, *imax, *initvalue, *ifn;
} SLD;


typedef struct {
    OPDS   h;
    cs_float  *r[8];             /* output */
    cs_float  *ichan;              /* input */
    SLD    s[8];
    cs_float  min[8], max[8];
    unsigned char   slchan, slnum[8];
    FUNC   *ftp[8];
} SLIDER8;

typedef struct {
    OPDS   h;
    cs_float  *r[16];             /* output */
    cs_float  *ichan;              /* input */
    SLD    s[16];
    cs_float  min[16], max[16];
    unsigned char   slchan, slnum[16];
    FUNC   *ftp[16];
} SLIDER16;

typedef struct {
    OPDS   h;
    cs_float  *r[32];             /* output */
    cs_float  *ichan;              /* input */
    SLD    s[32];
    cs_float  min[32], max[32];
    unsigned char   slchan, slnum[32];
    FUNC   *ftp[32];
} SLIDER32;

typedef struct {
    OPDS   h;
    cs_float  *r[64];             /* output */
    cs_float  *ichan;              /* input */
    SLD    s[64];
    cs_float  min[64], max[64];
    unsigned char   slchan, slnum[64];
    FUNC   *ftp[64];
} SLIDER64;

/*=============================*/

typedef struct {
    cs_float *ictlno, *imin, *imax, *initvalue, *ifn, *ihp;
} SLDf;

typedef struct {
    OPDS   h;
    cs_float  *r[8];              /* output */
    cs_float  *ichan;         /* input */
    SLDf   s[8];
    cs_float  min[8], max[8];
    unsigned char   slchan, slnum[8];
    FUNC   *ftp[8];
    cs_float  c1[8], c2[8];
    cs_float  yt1[8];
} SLIDER8f;

typedef struct {
    OPDS   h;
    cs_float  *r[16];              /* output */
    cs_float  *ichan;         /* input */
    SLDf   s[16];
    cs_float  min[16], max[16];
    unsigned char   slchan, slnum[16];
    FUNC   *ftp[16];
    cs_float  c1[16], c2[16];
    cs_float  yt1[16];
} SLIDER16f;

typedef struct {
    OPDS   h;
    cs_float  *r[32];              /* output */
    cs_float  *ichan;         /* input */
    SLDf   s[32];
    cs_float  min[32], max[32];
    unsigned char   slchan, slnum[32];
    FUNC   *ftp[32];
    cs_float  c1[32], c2[32];
    cs_float  yt1[32];
} SLIDER32f;

typedef struct {
    OPDS   h;
    cs_float  *r[64];              /* output */
    cs_float  *ichan;         /* input */
    SLDf   s[64];
    cs_float  min[64], max[64];
    unsigned char   slchan, slnum[64];
    FUNC   *ftp[64];
    cs_float  c1[64], c2[64];
    cs_float  yt1[64];
} SLIDER64f;

/*---------------------*/

typedef struct {
    cs_float *ictlno, *imin, *imax, *ifn;
} ISLD;

typedef struct {
    OPDS   h;
    cs_float  *r[8];              /* output */
    cs_float  *ichan;              /* input */
    ISLD   s[8];
} ISLIDER8;

typedef struct {
    OPDS   h;
    cs_float  *r[16];              /* output */
    cs_float  *ichan;              /* input */
    ISLD   s[16];
} ISLIDER16;

typedef struct {
    OPDS   h;
    cs_float  *r[32];              /* output */
    cs_float  *ichan;              /* input */
    ISLD   s[32];
} ISLIDER32;

typedef struct {
    OPDS   h;
    cs_float  *r[64];              /* output */
    cs_float  *ichan;              /* input */
    ISLD   s[64];
} ISLIDER64;

/*------------------------------*/

typedef struct {
    cs_float *ictlno_msb, *ictlno_lsb, *imin, *imax, *initvalue, *ifn;
} SLD14;

typedef struct {
    OPDS   h;
    cs_float  *r[16];              /* output */
    cs_float  *ichan;              /* input */
    SLD14  s[16];
    cs_float  min[16], max[16];
    unsigned char   slchan, slnum_msb[16],slnum_lsb[16];
    FUNC   *ftp[16];
} SLIDER16BIT14;

typedef struct {
    OPDS   h;
    cs_float  *r[32];              /* output */
    cs_float  *ichan;              /* input */
    SLD14  s[32];
    cs_float  min[32], max[32];
    unsigned char   slchan, slnum_msb[32],slnum_lsb[32];
    FUNC   *ftp[32];
} SLIDER32BIT14;
/*--------------------*/

typedef struct {
    cs_float *ictlno_msb, *ictlno_lsb, *imin, *imax, *ifn;
} ISLD14;

typedef struct {
    OPDS   h;
    cs_float  *r[16];             /* output */
    cs_float  *ichan;              /* input */
    ISLD14 s[16];
} ISLIDER16BIT14;

typedef struct {
    OPDS   h;
    cs_float  *r[32];             /* output */
    cs_float  *ichan;              /* input */
    ISLD14 s[32];
} ISLIDER32BIT14;

typedef struct {
    cs_float *imin, *imax, *initvalue, *ifn;
} SLD2;

typedef struct {
    OPDS   h;
    cs_float  *r[16];             /* output */
    SLD2   s[16];
    cs_float  min[16], max[16];
    FUNC   *ftp[16];
} SLIDERKAWAI;


typedef struct {
    OPDS   h;
    cs_float  *ktrig;                          /* output */
    cs_float  *ichan,  *ioutfn, *ioffset;      /* input */
    SLDf   s[64];
    cs_float  min[64], max[64], *outTable;
    unsigned char   slchan, slnum[64], oldvalue[64];
    FUNC   *ftp[64];
    cs_float  c1[6], c2[64];
    cs_float  yt1[64];
} SLIDER64tf;


typedef struct {
    OPDS   h;
    cs_float  *ktrig;                          /* output */
    cs_float  *ichan,  *ioutfn, *ioffset;      /* input */
    SLDf   s[32];
    cs_float  min[32], max[32], *outTable;
    unsigned char   slchan, slnum[32], oldvalue[32];
    FUNC   *ftp[32];
    cs_float  c1[32], c2[32];
    cs_float  yt1[32];
} SLIDER32tf;

typedef struct {
    OPDS   h;
    cs_float  *ktrig;                          /* output */
    cs_float  *ichan,  *ioutfn, *ioffset;      /* input */
    SLDf   s[16];
    cs_float  min[16], max[16], *outTable;
    unsigned char   slchan, slnum[16], oldvalue[16];
    FUNC   *ftp[16];
    cs_float  c1[16], c2[16];
    cs_float  yt1[6];
} SLIDER16tf;

typedef struct {
    OPDS   h;
    cs_float  *ktrig;                          /* output */
    cs_float  *ichan,  *ioutfn, *ioffset;      /* input */
    SLDf   s[8];
    cs_float  min[8], max[8], *outTable;
    unsigned char   slchan, slnum[8], oldvalue[8];
    FUNC   *ftp[8];
    cs_float  c1[8], c2[8];
    cs_float  yt1[8];
} SLIDER8tf;

typedef struct {
    OPDS   h;
    cs_float  *ktrig;      /* output */
    cs_float  *ichan,  *ioutfn, *ioffset;              /* input */
    SLD    s[64];
    cs_float  min[64], max[64], *outTable;
    unsigned char   slchan, slnum[64], oldvalue[64];
    FUNC   *ftp[64];
} SLIDER64t; /* GAB */


typedef struct {
    OPDS   h;
    cs_float  *ktrig;      /* output */
    cs_float  *ichan,  *ioutfn, *ioffset;              /* input */
    SLD    s[32];
    cs_float  min[32], max[32], *outTable;
    unsigned char   slchan, slnum[32], oldvalue[32];
    FUNC   *ftp[32];
} SLIDER32t; /* GAB */

typedef struct {
    OPDS   h;
    cs_float  *ktrig;      /* output */
    cs_float  *ichan,  *ioutfn, *ioffset;              /* input */
    SLD    s[16];
    cs_float  min[16], max[16], *outTable;
    unsigned char   slchan, slnum[16], oldvalue[16];
    FUNC   *ftp[16];
} SLIDER16t; /* GAB */


typedef struct {
    OPDS   h;
    cs_float  *ktrig;      /* output */
    cs_float  *ichan,  *ioutfn, *ioffset;              /* input */
    SLD    s[8];
    cs_float  min[8], max[8], *outTable;
    unsigned char   slchan, slnum[8], oldvalue[8];
    FUNC   *ftp[8];
} SLIDER8t; /* GAB */


typedef struct {
    OPDS   h;
    cs_float  *r, *ichan, *ictlno, *imin, *imax, *ifn, *icutoff;
    short flag;
    FUNC *ftp;
    int64_t   ctlno;

    cs_float   c1;     /* Value to multiply with input value  */
    cs_float   c2;     /* Value to multiply with previous state */
    cs_float   yt1;        /* Previous state */
    cs_float   prev;

} CTRL7a;
