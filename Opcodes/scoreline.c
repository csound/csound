/*
    scoreline.c:

    (c) Victor Lazzarini, 2004

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

typedef struct _inmess {
  OPDS h;
  MYFLT *SMess, *ktrig;
} inmess;

int messi(CSOUND *csound, inmess *p)
{
    csound->InputMessage(csound, (char *)p->SMess);
    return OK;
}

int messk(CSOUND *csound, inmess *p){
   if(*p->ktrig) csound->InputMessage(csound, (char *)p->SMess);
    return OK;
}

static OENTRY localops[] = {
  {"scoreline_i", sizeof(inmess), 1, "", "S", (SUBR)messi, NULL, NULL},
  {"scoreline", sizeof(inmess), 2, "", "Sk", NULL, (SUBR)messk, NULL}
};

LINKAGE
