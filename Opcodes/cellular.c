
/*
    pitch.c:

    Copyright (C) 2011 Gleb Rogozinsky

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

// classical 1-D Cellular Automaton by Gleb Rogozinsky.
// It is the modified version of vcella opcode by Gabriel Maldonado

typedef struct {
    OPDS    h;
    MYFLT   *ktrig, *kreinit, *ioutFunc, *initStateFunc,
            *iRuleFunc, *ielements;
    MYFLT   *currLine, *outVec, *initVec, *ruleVec;
    int     elements, NewOld;
    AUXCH   auxch;
} CELL;

static int cell_set(CSOUND *csound,CELL *p)
{
    FUNC        *ftp;
    int elements;
    MYFLT *currLine, *initVec = NULL;

    if (LIKELY((ftp = csound->FTnp2Find(csound,p->ioutFunc)) != NULL)) {
      p->outVec = ftp->ftable;
      elements = (p->elements = (int) *p->ielements);
      if (UNLIKELY( elements > (int)ftp->flen ))
        return csound->InitError(csound, Str("cell: invalid num of elements"));
    }
    else return csound->InitError(csound, Str("cell: invalid output table"));
    if (LIKELY((ftp = csound->FTnp2Find(csound,p->initStateFunc)) != NULL)) {
      initVec = (p->initVec = ftp->ftable);
      if (UNLIKELY(elements > (int)ftp->flen ))
        return csound->InitError(csound, Str("cell: invalid num of elements"));
    }
    else
      return csound->InitError(csound, Str("cell: invalid initial state table"));
    if (LIKELY((ftp = csound->FTnp2Find(csound,p->iRuleFunc)) != NULL)) {
      p->ruleVec = ftp->ftable;
    }
    else
      return csound->InitError(csound, Str("cell: invalid rule table"));

    if (p->auxch.auxp == NULL)
      csound->AuxAlloc(csound, elements * sizeof(MYFLT) * 2, &p->auxch);
    currLine = (p->currLine = (MYFLT *) p->auxch.auxp);
    p->NewOld = 0;
    memcpy(currLine, initVec, sizeof(MYFLT)*elements);
    /* do { */
    /*   *currLine++ = *initVec++; */
    /* } while (--elements); */
    return OK;
}

static int cell(CSOUND *csound,CELL *p)
{
    if (*p->kreinit) {
      p->NewOld = 0;
      memcpy(p->currLine, p->initVec, sizeof(MYFLT)*p->elements);
      /* do { */
      /*   *currLine++ = *initVec++; */
      /* } while (--elements); */
    }
    if (*p->ktrig) {
      int j, elements = p->elements, jm1;
      MYFLT *actual, *previous, *outVec = p->outVec , *ruleVec = p->ruleVec;
      previous = &(p->currLine[elements * p->NewOld]);
      p->NewOld += 1;
      p->NewOld %= 2;
      actual   = &(p->currLine[elements * p->NewOld]);
// Cellular Engine
      for (j=0; j < elements; j++) {
        jm1 = (j < 1) ? elements-1 : j-1;
        outVec[j] = previous[j];
        actual[j] = ruleVec[(int)(previous[jm1]*4 + previous[j]*2 +
                                  previous[(j+1) % elements])];
      }

    } else {
      int elements =  p->elements;
      MYFLT *actual = &(p->currLine[elements * !(p->NewOld)]);
      memcpy(p->outVec, actual, sizeof(MYFLT)*elements);
      /* do { */
      /*   *outVec++ = *actual++ ; */
      /* } while (--elements); */
    }
    return OK;
}


#define S sizeof

static OENTRY localops[] = {
  {"cell",  S(CELL),  3, TB, "",  "kkiiii",(SUBR)cell_set, (SUBR)cell        }
};

LINKAGE

// Author: Gleb Rogozinsky, October 2011
