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

int     pvset(CSOUND *, void *), pvset_S(CSOUND *, void *), pvoc(CSOUND *, void *);
int     pvaddset(CSOUND *, void *), pvadd(CSOUND *, void *);
int     pvaddset_S(CSOUND *, void *);
int     tblesegset(CSOUND *, void *), ktableseg(CSOUND *, void *);
int     ktablexseg(CSOUND *, void *);
int     vpvset(CSOUND *, void *), vpvset_S(CSOUND *, void *),
        vpvoc(CSOUND *, void *);
int     pvreadset(CSOUND *, void *), pvread(CSOUND *, void *);
int     pvcrossset(CSOUND *, void *), pvcross(CSOUND *, void *);
int     pvbufreadset(CSOUND *, void *), pvbufread(CSOUND *, void *);
int     pvinterpset(CSOUND *, void *), pvinterp(CSOUND *, void *);

int     pvreadset_S(CSOUND *, void *);
int     pvcrossset_S(CSOUND *, void *);
int     pvbufreadset_S(CSOUND *, void *);
int     pvinterpset_S(CSOUND *, void *);

#define S(x)    sizeof(x)

static OENTRY pvoc_localops[] = {
{ "pvoc",      S(PVOC),      0, 5, "a",  "kkSoooo", pvset_S, NULL, pvoc        },
{ "pvoc.i",      S(PVOC),      0, 5, "a",  "kkioooo", pvset, NULL, pvoc        },
{ "tableseg",  S(TABLESEG),  TR, 3, "",   "iim",     tblesegset, ktableseg, NULL  },
{ "ktableseg", S(TABLESEG),  _QQ|TR, 3, "",   "iim",  tblesegset, ktableseg, NULL },
{ "tablexseg", S(TABLESEG),  TW, 3, "",   "iin",     tblesegset, ktablexseg, NULL },
{ "vpvoc",     S(VPVOC),     TR, 5, "a",  "kkSoo",   vpvset_S, NULL, vpvoc        },
{ "vpvoc.i",     S(VPVOC),     TR, 5, "a",  "kkioo",   vpvset, NULL, vpvoc        },
{ "pvread",    S(PVREAD),  0,  3, "kk", "kSi",     pvreadset_S, pvread, NULL      },
{ "pvread.i",    S(PVREAD),  0,  3, "kk", "kii",     pvreadset, pvread, NULL      },
{ "pvcross",   S(PVCROSS), 0,  5, "a",  "kkSkko",  pvcrossset_S, NULL, pvcross    },
{ "pvbufread", S(PVBUFREAD),0, 3, "",   "kS",      pvbufreadset_S, pvbufread, NULL},
{ "pvinterp",  S(PVINTERP), 0, 5, "a",  "kkSkkkkkk", pvinterpset_S, NULL, pvinterp},
{ "pvcross.i",   S(PVCROSS), 0,  5, "a",  "kkikko",  pvcrossset, NULL, pvcross    },
{ "pvbufread.i", S(PVBUFREAD),0, 3, "",   "ki",      pvbufreadset, pvbufread, NULL},
{ "pvinterp.i",  S(PVINTERP), 0, 5, "a",  "kkikkkkkk", pvinterpset, NULL, pvinterp},
{ "pvadd",     S(PVADD),   0,  5, "a",  "kkSiiopooo", pvaddset_S, NULL, pvadd     },
{ "pvadd.i",     S(PVADD),   0,  5, "a",  "kkiiiopooo", pvaddset, NULL, pvadd     }
};

PVOC_GLOBALS *PVOC_AllocGlobals(CSOUND *csound)
{
    PVOC_GLOBALS  *p;
#ifdef BETA
    csound->Message(csound, "calling alloc globals");
#endif
    if (UNLIKELY(csound->CreateGlobalVariable(csound, "pvocGlobals",
                                              sizeof(PVOC_GLOBALS)) != 0)){
      csound->ErrorMsg(csound, Str("Error allocating PVOC globals"));
      return NULL;
    }
    p = (PVOC_GLOBALS*) csound->QueryGlobalVariable(csound, "pvocGlobals");
    p->csound = csound;
    p->dsputil_sncTab = (MYFLT*) NULL;
    p->pvbufreadaddr = (PVBUFREAD*) NULL;
    p->tbladr = (TABLESEG*) NULL;

    return p;
}

LINKAGE_BUILTIN(pvoc_localops)

