/*
    scoreline.c:

    Copyright (c) Victor Lazzarini, 2004,2008

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

//#include "csdl.h"
#include "csoundCore.h"
//extern void csoundInputMessageInternal(CSOUND *, const char *);

typedef struct _inmess {
  OPDS h;
  STRINGDAT *SMess;
  MYFLT *ktrig;
} INMESS;


typedef struct _scorepos {
  OPDS h;
  MYFLT *spos;
} SCOREPOS;

int32_t messi(CSOUND *csound, INMESS *p)
{
    csound->InputMessage(csound, (char *)p->SMess->data);
    return OK;
}

int32_t messk(CSOUND *csound, INMESS *p){
    if (*p->ktrig) csound->InputMessage(csound, (char *)p->SMess->data);
    return OK;
}

int32_t setscorepos(CSOUND *csound, SCOREPOS *p){
    csound->SetScoreOffsetSeconds(csound, *p->spos);
    return OK;
}

int32_t
rewindscore(CSOUND *csound, SCOREPOS *p){
    IGN(p);
    csound->RewindScore(csound);
    return OK;
}


static OENTRY scoreline_localops[] = {
  {"scoreline_i", sizeof(INMESS), 0, 1, "", "S", (SUBR)messi, NULL, NULL, NULL},
  {"scoreline", sizeof(INMESS), 0, 2, "", "Sk", NULL, (SUBR)messk, NULL, NULL},
  {"setscorepos", sizeof(SCOREPOS), 0, 1, "", "i", (SUBR)setscorepos, NULL, NULL, NULL},
  {"rewindscore", sizeof(SCOREPOS), 0, 1, "", "", (SUBR)rewindscore, NULL, NULL, NULL}
};

LINKAGE_BUILTIN(scoreline_localops)
