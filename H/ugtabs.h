/*
    ugtabs.h:  new implementation of table readers and writers

    Copyright (C) 2013 V Lazzarini

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


typedef struct _tabl {
  OPDS h;
  MYFLT *sig, *ndx, *ftable, *mode, *offset, *wrap;
  MYFLT mul;
  int32 np2;
  int32 len;
  int iwrap;
  FUNC *ftp;
} TABL;

typedef struct _tlen {
  OPDS h;
  MYFLT *ans, *ftable;
} TLEN;

typedef struct _tgp {
  OPDS h;
  MYFLT  *ftable, *ftsrc;
} TGP;

typedef struct _tablmix {
  OPDS h;
  MYFLT *tab, *off, *len, *tab1, *off1, *g1, *tab2, *off2, *g2;
} TABLMIX;

typedef struct _tablra {
  OPDS h;
  MYFLT *sig,*ftable,*strt,*off;
} TABLRA;

typedef struct _tablwa {
  OPDS h;
  MYFLT *strt,*ftable,*sig,*off,*skipinit;
  MYFLT pos;
} TABLWA;
