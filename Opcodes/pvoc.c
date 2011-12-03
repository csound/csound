/*
    pvoc.c:

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

#include "pvoc.h"

int     pvset(CSOUND *, void *), pvoc(CSOUND *, void *);
int     pvaddset(CSOUND *, void *), pvadd(CSOUND *, void *);
int     tblesegset(CSOUND *, void *), ktableseg(CSOUND *, void *);
int     ktablexseg(CSOUND *, void *);
int     vpvset(CSOUND *, void *), vpvoc(CSOUND *, void *);
int     pvreadset(CSOUND *, void *), pvread(CSOUND *, void *);
int     pvcrossset(CSOUND *, void *), pvcross(CSOUND *, void *);
int     pvbufreadset(CSOUND *, void *), pvbufread(CSOUND *, void *);
int     pvinterpset(CSOUND *, void *), pvinterp(CSOUND *, void *);

#define S(x)    sizeof(x)

static OENTRY pvoc_localops[] = {
{ "pvoc",      S(PVOC),      5, "a",  "kkToooo", pvset, NULL, pvoc            },
{ "tableseg",  S(TABLESEG),  TR|3, "",   "iin",     tblesegset, ktableseg, NULL  },
{ "ktableseg", S(TABLESEG),  DP|TR|3, "",   "iin",  tblesegset, ktableseg, NULL  },
{ "tablexseg", S(TABLESEG),  TW|3, "",   "iin",     tblesegset, ktablexseg, NULL },
{ "vpvoc",     S(VPVOC),     TR|5, "a",  "kkToo",   vpvset, NULL, vpvoc          },
{ "pvread",    S(PVREAD),    3, "kk", "kTi",     pvreadset, pvread, NULL      },
{ "pvcross",   S(PVCROSS),   5, "a",  "kkTkko",  pvcrossset, NULL, pvcross    },
{ "pvbufread", S(PVBUFREAD), 3, "",   "kT",      pvbufreadset, pvbufread, NULL},
{ "pvinterp",  S(PVINTERP),  5, "a",  "kkTkkkkkk", pvinterpset, NULL, pvinterp},
{ "pvadd",     S(PVADD),     5, "a",  "kkTiiopooo", pvaddset, NULL, pvadd     }
};

PVOC_GLOBALS *PVOC_AllocGlobals(CSOUND *csound)
{
    PVOC_GLOBALS  *p;
#ifdef BETA
    csound->Message(csound, "calling alloc globals");
#endif
    if (UNLIKELY(csound->CreateGlobalVariable(csound, "pvocGlobals",
                                              sizeof(PVOC_GLOBALS)) != 0))
      csound->Die(csound, Str("Error allocating PVOC globals"));
    p = (PVOC_GLOBALS*) csound->QueryGlobalVariable(csound, "pvocGlobals");
    p->csound = csound;
    p->dsputil_sncTab = (MYFLT*) NULL;
    p->pvbufreadaddr = (PVBUFREAD*) NULL;
    p->tbladr = (TABLESEG*) NULL;

    return p;
}

LINKAGE1(pvoc_localops)

