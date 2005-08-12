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

int chanokset(CSOUND *csound, CHANO *p)
{
    return OK;
} /* end chanokset(p) */

int chanoaset(CSOUND *csound, CHANO *p)
{
    return OK;
} /* end chanset(p) */

int chanokdo(CSOUND *csound, CHANO *p)
{
    int kn = (int)(*p->kn + 0.5);
    if (kn < 0)
      return NOTOK;
    if (kn > csound->nchanok) {
      csound->chanok =
        (MYFLT*) csound->ReAlloc(csound, csound->chanok, kn * sizeof(MYFLT));
      csound->nchanok = kn;
    }
    csound->chanok[kn] = *p->ain;
    return OK;
}

int chanoado(CSOUND *csound, CHANO *p)
{
    int i,j;
    int kn = (int)(*p->kn + 0.5);
    if (kn < 0)
      return NOTOK;
    if (kn > csound->nchanoa) {
      csound->chanoa =
        (MYFLT*) csound->ReAlloc(csound, csound->chanoa,
                                         kn * sizeof(MYFLT) * csound->ksmps);
      csound->nchanoa = kn;
    }
    for (i=0, j=kn*csound->ksmps; i<csound->ksmps; i++, j++) {
      csound->chanok[j] = (*p->ain)++;
    }
    return OK;
} /* end chanoas(p) */

int chanikset(CSOUND *csound, CHANI *p)
{
    return OK;
} /* end chaniset(p) */

int chaniaset(CSOUND *csound, CHANI *p)
{
    return OK;
} /* end chaniaset(p) */

int chanikdo(CSOUND *csound, CHANI *p)
{
    int kn = (int)(*p->kn + 0.5);
    if (kn < 0)
      return NOTOK;
    if (kn > csound->nchanik) {
      csound->chanik =
        (MYFLT*) csound->ReAlloc(csound, csound->chanik, kn * sizeof(MYFLT));
      csound->nchanik = kn;
    }
    *p->ar = csound->chanok[kn];
    return OK;
}

int chaniado(CSOUND *csound, CHANI *p)
{
    int i,j;
    int kn = (int)(*p->kn + 0.5);
    if (kn < 0)
      return NOTOK;
    if (kn > csound->nchania) {
      csound->chania =
        (MYFLT*) csound->ReAlloc(csound, csound->chania,
                                         kn * sizeof(MYFLT) * csound->ksmps);
      csound->nchania = kn;
    }
    for (i=0, j=kn*csound->ksmps; i<csound->ksmps; i++, j++) {
      *(p->ar++) = csound->chanoa[j];
    }
    return OK;
} /* end chanias(p) */

#define S       sizeof

static OENTRY localops[] = {
  { "chano",  0xfffd},
  { "chano.k", S(CHANO), 2, "",  "kk", NULL,(SUBR)chanokdo, NULL},
  { "chano.a", S(CHANO), 4, "",  "ak", NULL, NULL,         (SUBR)chanoado},
  { "chani",  0xffff},
  { "chani.k", S(CHANO), 2, "k", "k", NULL, (SUBR)chanikdo, NULL},
  { "chani.k", S(CHANO), 4, "a", "k", NULL, NULL,          (SUBR)chaniado}
};

LINKAGE

