/*
    examplePlugin.c:

    Copyright (C) 2006 Steven Yi

    Simple example plugin to use as a starting point for building Csound plugins

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

#include <csdl.h>

#define FLOOR(x) (x >= FL(0.0) ? (long)x : (long)((double)x - 0.99999999))

typedef struct {
        OPDS    h;
        MYFLT   *kout, *kindx, *avar;
} AVAR_GET;

typedef struct {
        OPDS    h;
        MYFLT   *kval, *kindx, *avar;
} AVAR_SET;

static int avar_get(CSOUND *csound, AVAR_GET *p) {
        long ndx = (long) FLOOR((double)*p->kindx);
        *p->kout = *(p->avar + ndx);
        return OK;
}

static int avar_set(CSOUND *csound, AVAR_SET *p) {
        long ndx = (long) FLOOR((double)*p->kindx);
        *(p->avar + ndx) = *p->kval;
        return OK;
}

#define S(x)    sizeof(x)

static OENTRY localops[] = {
  { "avar_get",  S(AVAR_GET),   0, 2,      "k", "ka",    NULL, (SUBR)avar_get   },
  { "avar_set", S(AVAR_SET), 0, 2, "",  "kka", NULL, (SUBR)avar_set }

};


LINKAGE
