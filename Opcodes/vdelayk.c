/*  
    vdelayk.c:

    Copyright (C) 2004 Gabriel Maldonado

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

#include "csdl.h"

typedef struct {
	OPDS	h;
	MYFLT	*kr, *kin, *kdel, *imaxd, *istod, *interp;
	AUXCH	aux;
	long	left, maxd;
} KDEL;


int kdel_set(KDEL *p)
{
    unsigned long n;
    MYFLT *buf;
    n = (p->maxd = (long) (*p->imaxd * ekr));
    if (n == 0)	n = (p->maxd = 1);

    if (!*p->istod) {
      if (p->aux.auxp == NULL || (int)(n*sizeof(MYFLT)) > p->aux.size)
        auxalloc(n * sizeof(MYFLT), &p->aux);
      else {
        buf = (MYFLT *)p->aux.auxp;
        do {
          *buf++ = FL(0.0);
        } while (--n);
      }
      p->left = 0;
    }
    return OK;
}

int kdelay(KDEL *p)
{
    long maxd = p->maxd, indx, v1, v2;
    MYFLT *buf = (MYFLT *)p->aux.auxp, fv1, fv2;

    if (buf==NULL) {
      return initerror("vdelayk: not initialized");
    }
    indx = p->left;
    buf[indx] = *p->kin;
    fv1 = indx - *p->kdel * ekr;
    while (fv1 < 0.0f)	fv1 += (MYFLT)maxd;
    while (fv1 >= (MYFLT)maxd) fv1 -= (MYFLT)maxd;
    if (*p->interp) {           /* no interpolation */
      *p->kr = buf[(long) fv1];
    }
    else {
      if (fv1 < maxd - 1) fv2 = fv1 + FL(1.0);
      else                fv2 = FL(0.0);
      v1 = (long)fv1;
      v2 = (long)fv2;
      *p->kr = buf[v1] + (fv1 - v1) * (buf[v2]-buf[v1]);
    }
    if (++(p->left) == maxd) p->left = 0;
    return OK;
}

#define S       sizeof

static OENTRY localops[] = {
{ "vdelayk",S(KDEL),   3,        "k",   "kkioo",  (SUBR)kdel_set,  (SUBR)kdelay },
};

LINKAGE
