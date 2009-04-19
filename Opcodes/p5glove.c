/*
    p5glove.c:

    Copyright (C) 2009 by John ffitch,

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

                                                        /* wiimote.c */
#include "csdl.h"
#include <p5glove.h>
#include <errno.h>

typedef struct {
    OPDS      h;
 /* ------------------------------------- */
    P5Glove   p5g;
} P5GLOVEINIT;

typedef struct {
    OPDS      h;
    MYFLT     *res;
    MYFLT     *kControl;
    MYFLT     *num;
 /* ------------------------------------- */
    P5Glove   p5g;
} P5GLOVE;

typedef struct {
    OPDS      h;
    MYFLT     *iControl;
    MYFLT     *iMin;
    MYFLT     *iMax;
    MYFLT     *num;
    P5Glove   p5g;
} P5GRANGE;

int p5glove_find(CSOUND *csound, P5GLOVEINIT *p)
{
    int n, i;
    P5Glove    *glove;

    glove = (P5Glove*)csound->QueryGlobalVariable(csound, "p5glove");
    if (glove == NULL) {
      csound->CreateGlobalVariable(csound, "p5glove", sizeof(P5Glove));
      glove = (P5Glove*)csound->QueryGlobalVariable(csound, "p5glove");
    }
    p->p5g = *glove = p5glove_open(0);
    if (*glove==NULL) {
      return csound->InitError(csound, Str("unable to open p5glove\n"));
    }
    return OK;
}

int p5glove_poll(CSOUND *csound, P5GLOVE *p)
{
    P5Glove glove = p->p5g;
    int res = p5glove_sample(glove, -1);
    while (res < 0 && errno == EAGAIN) res = p5glove_sample(glove, -1);
    if (res < 0)
      return csound->PerfError(csound, "P5Glove failure");
    return OK;
}

int p5glove_closer(CSOUND *csound, P5GLOVE *p)
{
    P5Glove glove = p->p5g;
    P5Glove *g;

    g = (P5Glove*)csound->QueryGlobalVariable(csound, "p5glove");
    p5glove_close(p->p5g);
    *g = NULL;
    return OK;
}

int p5g_data_init(CSOUND *csound, P5GLOVE *p)
{
    P5Glove *p5g;
    p5g = (P5Glove*)csound->QueryGlobalVariable(csound, "p5glove");
    if (p5g==NULL) 
      return csound->InitError(csound, Str("No p5glove open"));
    p->p5g = *p5g;
}

int p5g_data(CSOUND *csound, P5GLOVE *p)
{
    P5Glove p5g = p->p5g;
    int kontrol = (int)(*p->kControl+FL(0.5));
    if (kontrol<0) {
      printf("debug: \n");
      *p->res = FL(0.0);
      return OK;
    }
    else switch (kontrol) {
      default: break;
    }
    return NOTOK;
}

int p5g_data_init(CSOUND *csound, P5GLOVE *p)
{
    P5Glove *p5glove;
    p5gloves = (p5glove**)csound->QueryGlobalVariable(csound, "p5glove");
    if (p5gloves==NULL) 
      return csound->InitError(csound, Str("No p5g open"));
    p->p5g = p5gloves;
}


#define S(x)    sizeof(x)

static OENTRY localops[] = {
  {"p5gconnect", S(P5GLOVE), 3, "", "", (SUBR)p5glove_find, (SUBR)p5glove_poll, NULL, (SUBR)p5glove_closer },
  {"p5gdata", S(P5GLOVE), 3, "k", "k", (SUBR)p5g_data_init, (SUBR)p5g_data }
};

LINKAGE
