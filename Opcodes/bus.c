/*
    bus.c:

    Copyright (C) 2004 John ffitch

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

/* The model is that of the e-mail of 6 May 2004 09:31:16 BST */
/* First version has a limit of NUMBUS buses of each type, but
   that can be changed with a little more code.
   Second version has expanding bus size, now coded.
   JPff -- Roquebrun, France
*/
#include "csdl.h"

typedef struct  {
        OPDS    h;
        MYFLT   *ain, *kn;   /* The parameter */
} BUSO;

typedef struct  {
        OPDS    h;
        MYFLT   *ar, *kn;   /* The parameter */
} BUSI;




#define NUMBUS (16)

int busokset(BUSO *p)
{
    return OK;
} /* end busokset(p) */

int busoaset(BUSO *p)
{
    return OK;
} /* end busset(p) */

int busokdo(BUSO *p)
{
    int kn = (int)(*p->kn + 0.5);
    if (kn<0)
      return NOTOK;
    if (kn>nbusok) {
      busok = (MYFLT*)realloc(busok,kn*sizeof(MYFLT));
      nbusok = kn;
    }
    busok[kn] = *p->ain;
    return OK;
}

int busoado(BUSO *p)
{
    int i,j;
    int kn = (int)(*p->kn + 0.5);
    if (kn<0)
      return NOTOK;
    if (kn>nbusoa) {
      busoa = (MYFLT*)realloc(busoa,kn*sizeof(MYFLT)*ksmps);
      nbusoa = kn;
    }
    for (i=0, j=kn*ksmps; i<ksmps; i++, j++) {
      busok[j] = (*p->ain)++;
    }
    return OK;
} /* end busoas(p) */

int busikset(BUSI *p)
{
    return OK;
} /* end busiset(p) */

int busiaset(BUSI *p)
{
    return OK;
} /* end busiaset(p) */

int busikdo(BUSI *p)
{
    int kn = (int)(*p->kn + 0.5);
    if (kn<0)
      return NOTOK;
    if (kn>nbusik) {
      busik = (MYFLT*)realloc(busik, kn*sizeof(MYFLT));
      nbusik = kn;
    }
    *p->ar = busok[kn];
    return OK;
}

int busiado(BUSI *p)
{
    int i,j;
    int kn = (int)(*p->kn + 0.5);
    if (kn<0)
      return NOTOK;
    if (kn>nbusia) {
      busia = (MYFLT*)realloc(busia, kn*sizeof(MYFLT));
      nbusia = kn;
    }
    for (i=0, j=kn*ksmps; i<ksmps; i++, j++) {
      *(p->ar++) = busoa[j];
    }
    return OK;
} /* end busias(p) */


#define S       sizeof

static OENTRY localops[] = {
  { "buso",  0xfffd},
  { "buso_k", S(BUSO), 2, "",  "kk", NULL,(SUBR)busokdo, NULL},
  { "buso_a", S(BUSO), 4, "",  "ak", NULL, NULL,         (SUBR)busoado},
  { "busi",  0xffff},
  { "busi_k", S(BUSO), 2, "k", "k", NULL, (SUBR)busikdo, NULL},
  { "busi_k", S(BUSO), 4, "a", "k", NULL, NULL,          (SUBR)busiado}
};

LINKAGE

