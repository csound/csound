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
/* First version has a limit of NUMCHAN channels of each type, but
   that can be changed with a little more code.
   Second version has expanding chan size, now coded.
   JPff -- Roquebrun, France
*/
#include "csdl.h"

typedef struct  {
        OPDS    h;
        MYFLT   *ain, *kn;   /* The parameter */
} CHANO;

typedef struct  {
        OPDS    h;
        MYFLT   *ar, *kn;   /* The parameter */
} CHANI;

int chanokset(CHANO *p)
{
    return OK;
} /* end chanokset(p) */

int chanoaset(CHANO *p)
{
    return OK;
} /* end chanset(p) */

int chanokdo(CHANO *p)
{
    int kn = (int)(*p->kn + 0.5);
    if (kn<0)
      return NOTOK;
    if (kn>nchanok) {
      chanok = (MYFLT*)realloc(chanok,kn*sizeof(MYFLT));
      nchanok = kn;
    }
    chanok[kn] = *p->ain;
    return OK;
}

int chanoado(CHANO *p)
{
    int i,j;
    int kn = (int)(*p->kn + 0.5);
    if (kn<0)
      return NOTOK;
    if (kn>nchanoa) {
      chanoa = (MYFLT*)realloc(chanoa,kn*sizeof(MYFLT)*ksmps);
      nchanoa = kn;
    }
    for (i=0, j=kn*ksmps; i<ksmps; i++, j++) {
      chanok[j] = (*p->ain)++;
    }
    return OK;
} /* end chanoas(p) */

int chanikset(CHANI *p)
{
    return OK;
} /* end chaniset(p) */

int chaniaset(CHANI *p)
{
    return OK;
} /* end chaniaset(p) */

int chanikdo(CHANI *p)
{
    int kn = (int)(*p->kn + 0.5);
    if (kn<0)
      return NOTOK;
    if (kn>nchanik) {
      chanik = (MYFLT*)realloc(chanik, kn*sizeof(MYFLT));
      nchanik = kn;
    }
    *p->ar = chanok[kn];
    return OK;
}

int chaniado(CHANI *p)
{
    int i,j;
    int kn = (int)(*p->kn + 0.5);
    if (kn<0)
      return NOTOK;
    if (kn>nchania) {
      chania = (MYFLT*)realloc(chania, kn*sizeof(MYFLT));
      nchania = kn;
    }
    for (i=0, j=kn*ksmps; i<ksmps; i++, j++) {
      *(p->ar++) = chanoa[j];
    }
    return OK;
} /* end chanias(p) */


#define S       sizeof

static OENTRY localops[] = {
  { "chano",  0xfffd},
  { "chano_k", S(CHANO), 2, "",  "kk", NULL,(SUBR)chanokdo, NULL},
  { "chano_a", S(CHANO), 4, "",  "ak", NULL, NULL,         (SUBR)chanoado},
  { "chani",  0xffff},
  { "chani_k", S(CHANO), 2, "k", "k", NULL, (SUBR)chanikdo, NULL},
  { "chani_k", S(CHANO), 4, "a", "k", NULL, NULL,          (SUBR)chaniado}
};

LINKAGE

