/*
    pvoc.c:

    Copyright (c) 2006 Istvan Varga

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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "pvoc.h"

int32_t     pvset(CSOUND *, void *), pvset_S(CSOUND *, void *);
int32_t     pvoc(CSOUND *, void *);
int32_t     pvaddset(CSOUND *, void *), pvadd(CSOUND *, void *);
int32_t     pvaddset_S(CSOUND *, void *);
int32_t     tblesegset(CSOUND *, void *), ktableseg(CSOUND *, void *);
int32_t     ktablexseg(CSOUND *, void *);
int32_t     vpvset(CSOUND *, void *), vpvset_S(CSOUND *, void *),
        vpvoc(CSOUND *, void *);
int32_t     pvreadset(CSOUND *, void *), pvread(CSOUND *, void *);
int32_t     pvcrossset(CSOUND *, void *), pvcross(CSOUND *, void *);
int32_t     pvbufreadset(CSOUND *, void *), pvbufread(CSOUND *, void *);
int32_t     pvinterpset(CSOUND *, void *), pvinterp(CSOUND *, void *);

int32_t     pvreadset_S(CSOUND *, void *);
int32_t     pvcrossset_S(CSOUND *, void *);
int32_t     pvbufreadset_S(CSOUND *, void *);
int32_t     pvinterpset_S(CSOUND *, void *);

#define S(x)    sizeof(x)

static OENTRY pvoc_localops[] =
  {
   { "pvoc",      S(PVOC),      0, "a",  "kkSooooo", pvset_S, pvoc        },
   { "pvoc.i",      S(PVOC),      0, "a",  "kkiooooo", pvset, pvoc        },
{ "tableseg",  S(TABLESEG),  TR, "",   "iim",     tblesegset, ktableseg, NULL  },
{ "ktableseg", S(TABLESEG),  _QQ|TR, "",   "iim",  tblesegset, ktableseg, NULL },
{ "tablexseg", S(TABLESEG),  TW, "",   "iin",     tblesegset, ktablexseg, NULL },
   { "vpvoc",     S(VPVOC),     TR, "a",  "kkSoo",   vpvset_S, vpvoc        },
   { "vpvoc.i",     S(VPVOC),     TR, "a",  "kkioo",   vpvset, vpvoc        },
{ "pvread",    S(PVREAD),  0,  "kk", "kSio",     pvreadset_S, pvread, NULL      },
{ "pvread.i",    S(PVREAD),  0,  "kk", "kiio",     pvreadset, pvread, NULL      },
   { "pvcross",   S(PVCROSS), 0,  "a",  "kkSkkoo",  pvcrossset_S, pvcross    },
{ "pvbufread", S(PVBUFREAD),0, "",   "kSo",      pvbufreadset_S, pvbufread, NULL},
   { "pvinterp",  S(PVINTERP), 0, "a",  "kkSkkkkkko", pvinterpset_S, pvinterp},
   { "pvcross.i",   S(PVCROSS), 0,  "a",  "kkikkoo",  pvcrossset, pvcross    },
{ "pvbufread.i", S(PVBUFREAD),0, "",   "kio",      pvbufreadset, pvbufread, NULL},
   { "pvinterp.i",  S(PVINTERP), 0, "a",  "kkikkkkkko", pvinterpset, pvinterp},
   { "pvadd",     S(PVADD),   0,  "a",  "kkSiiopoooo", pvaddset_S, pvadd     },
   { "pvadd.i",     S(PVADD),   0,  "a",  "kkiiiopoooo", pvaddset, pvadd     }
};

PVOC_GLOBALS *PVOC_AllocGlobals(CSOUND *csound)
{
    PVOC_GLOBALS  *p;
#ifdef BETA
    csound->Message(csound, "%s", "calling alloc globals");
#endif
    if (UNLIKELY(csound->CreateGlobalVariable(csound, "pvocGlobals",
                                              sizeof(PVOC_GLOBALS)) != 0)){
      csound->ErrorMsg(csound, "%s", Str("Error allocating PVOC globals"));
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

