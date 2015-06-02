/*
 csound_standard_types.c:

 Copyright (C) 2012,2013 Steven Yi

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

#include "csoundCore.h"
#include "csound_standard_types.h"
#include "pstream.h"
#include <stdlib.h>

//#define Wfloats (((int) sizeof(SPECDAT) + 7) / (int) sizeof(MYFLT))
//#define Pfloats (((int) sizeof(PVSDAT) + 7) / (int) sizeof(MYFLT))

/* MEMORY COPYING FUNCTIONS */

void myflt_copy_value(void* csound, void* dest, void* src) {
    MYFLT* f1 = (MYFLT*)dest;
    MYFLT* f2 = (MYFLT*)src;
    *f1 = *f2;
}

void asig_copy_value(void* csound, void* dest, void* src) {
    memcpy(dest, src, sizeof(MYFLT) * ((CSOUND*)csound)->ksmps);
}

void wsig_copy_value(void* csound, void* dest, void* src) {
    memcpy(dest, src, sizeof(SPECDAT));
    //TODO - check if this needs to copy SPECDAT's DOWNDAT member and AUXCH
}

void fsig_copy_value(void* csound, void* dest, void* src) {
    PVSDAT *fsigout = (PVSDAT*) dest;
    PVSDAT *fsigin = (PVSDAT*) src;
    int N = fsigin->N;
    memcpy(dest, src, sizeof(PVSDAT) - sizeof(AUXCH));
    if(fsigout->frame.auxp == NULL || fsigout->frame.size < (N + 2) * sizeof(float))
      ((CSOUND *)csound)->AuxAlloc(csound, (N + 2) * sizeof(float), &fsigout->frame);
    memcpy(fsigout->frame.auxp, fsigin->frame.auxp, (N + 2) * sizeof(float));
}


void string_copy_value(void* csound, void* dest, void* src) {
    STRINGDAT* sDest = (STRINGDAT*)dest;
    STRINGDAT* sSrc = (STRINGDAT*)src;
    CSOUND* cs = (CSOUND*)csound;

    if(src == NULL) return;
    if(dest == NULL) return;

    if (sSrc->size >= sDest->size) {
      sDest->size = sSrc->size;

      if (sDest->data != NULL) {
        cs->Free(cs, sDest->data);
      }
      sDest->data = cs_strdup(csound, sSrc->data);
    } else {
      memcpy(sDest->data, sSrc->data, sDest->size);
    }
}


void array_copy_value(void* csound, void* dest, void* src) {
    ARRAYDAT* aDest = (ARRAYDAT*)dest;
    ARRAYDAT* aSrc = (ARRAYDAT*)src;
    CSOUND* cs = (CSOUND*)csound;
    size_t size, j;
    int i;
    int memMyfltSize;
    
    // TODO - this is heavy handed to reallocate memory every time,
    // should rewrite like string_copy_value to just copy values if
    // there is enough memory

    aDest->arrayMemberSize = aSrc->arrayMemberSize;
    memMyfltSize = aDest->arrayMemberSize / sizeof(MYFLT);
    aDest->dimensions = aSrc->dimensions;
    aDest->sizes = cs->Malloc(cs, sizeof(int) * aSrc->dimensions);
    memcpy(aDest->sizes, aSrc->sizes, sizeof(int) * aSrc->dimensions);
    aDest->arrayType = aSrc->arrayType;

    size = aDest->sizes[0];
    for (i = 1; i < aDest->dimensions; i++) {
      size *= aDest->sizes[i];
    }

    aDest->data = cs->Calloc(cs, aSrc->arrayMemberSize * size);
    for (j = 0; j < size; j++) {
        int index = j * memMyfltSize;
        aDest->arrayType->copyValue(csound,
                                    aDest->data + index, aSrc->data + index);
    }
    
}

/* MEM SIZE UPDATING FUNCTIONS */

void updateAsigMemBlock(void* csound, CS_VARIABLE* var) {
    CSOUND* cs = (CSOUND*)csound;
    int ksmps = cs->ksmps;
    var->memBlockSize = ksmps * sizeof (MYFLT);
}

void varInitMemory(CS_VARIABLE* var, MYFLT* memblock) {
    memset(memblock, 0, var->memBlockSize);
}

/* CREATE VAR FUNCTIONS */

CS_VARIABLE* createAsig(void* cs, void* p) {
    int ksmps;
    CSOUND* csound = (CSOUND*)cs;
    IGN(p);

    //FIXME - this needs to take into account local ksmps, once
    //context work is complete
//    if (instr != NULL) {
//      OPDS* p = (OPDS*)instr;
//      ksmps = CS_KSMPS;
//    } else {
    ksmps = csound->ksmps;
//    }

    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = ksmps * sizeof (MYFLT);
    var->updateMemBlockSize = &updateAsigMemBlock;
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

CS_VARIABLE* createMyflt(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = sizeof (MYFLT);
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

CS_VARIABLE* createBool(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = sizeof (MYFLT);
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

CS_VARIABLE* createWsig(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = sizeof(MYFLT) * ((sizeof(SPECDAT) + 7) / sizeof(MYFLT));
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

CS_VARIABLE* createFsig(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = sizeof(MYFLT) * ((sizeof(PVSDAT) + 7) / sizeof(MYFLT));
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

void arrayInitMemory(CS_VARIABLE* var, MYFLT* memblock) {
    ARRAYDAT* dat = (ARRAYDAT*)memblock;
    dat->arrayType = var->subType;
}

CS_VARIABLE* createString(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = sizeof(MYFLT) * ((sizeof(STRINGDAT) + 7) / sizeof(MYFLT));
    return var;
}

CS_VARIABLE* createArray(void* csnd, void* p) {
    CSOUND* csound = (CSOUND*)csnd;
    ARRAY_VAR_INIT* state = (ARRAY_VAR_INIT*)p;


    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = sizeof(MYFLT) * ((sizeof(ARRAYDAT) + 7 ) / sizeof(MYFLT));
    var->initializeVariableMemory = &arrayInitMemory;

    if (state) { // NB: this function is being called with p=NULL
      CS_TYPE* type = state->type;
      var->subType = type;
      var->dimensions = state->dimensions;
    }
    return var;
}


//#define ARGTYP_S        0x00000040L     /* string constant or variable */
//#define ARGTYP_l        0x00000800L     /* label */

const CS_TYPE CS_VAR_TYPE_A = {
  "a", "audio rate vector", CS_ARG_TYPE_BOTH, createAsig, asig_copy_value, NULL
};

const CS_TYPE CS_VAR_TYPE_K = {
  "k", "control rate var", CS_ARG_TYPE_BOTH, createMyflt, myflt_copy_value, NULL
};

const CS_TYPE CS_VAR_TYPE_I = {
  "i", "init time var", CS_ARG_TYPE_BOTH, createMyflt, myflt_copy_value, NULL
};

const CS_TYPE CS_VAR_TYPE_S = {
    "S", "String var", CS_ARG_TYPE_BOTH, createString, string_copy_value, NULL
};

const CS_TYPE CS_VAR_TYPE_P = {
  "p", "p-field", CS_ARG_TYPE_BOTH, createMyflt, myflt_copy_value, NULL
};

const CS_TYPE CS_VAR_TYPE_R = {
  "r", "reserved symbol", CS_ARG_TYPE_BOTH, createMyflt, myflt_copy_value, NULL
};

const CS_TYPE CS_VAR_TYPE_C = {
  "c", "constant", CS_ARG_TYPE_IN, createMyflt, myflt_copy_value, NULL
};

const CS_TYPE CS_VAR_TYPE_W = {
  "w", "spectral", CS_ARG_TYPE_BOTH, createWsig, wsig_copy_value, NULL
};

const CS_TYPE CS_VAR_TYPE_F = {
  "f", "f-sig", CS_ARG_TYPE_BOTH, createFsig, fsig_copy_value, NULL
};

const CS_TYPE CS_VAR_TYPE_B = {
  "B", "boolean", CS_ARG_TYPE_BOTH, createBool, myflt_copy_value, NULL
};

const CS_TYPE CS_VAR_TYPE_b = {
  "b", "boolean", CS_ARG_TYPE_BOTH, createBool, myflt_copy_value, NULL
};

const CS_TYPE CS_VAR_TYPE_ARRAY = {
   "[", "array", CS_ARG_TYPE_BOTH, createArray, array_copy_value, NULL
};



void csoundAddStandardTypes(CSOUND* csound, TYPE_POOL* pool) {

    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_A);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_K);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_I);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_S);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_P);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_R);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_C);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_W);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_F);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_B);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_b);
    csoundAddVariableType(csound, pool, (CS_TYPE*)&CS_VAR_TYPE_ARRAY);

}


/* Type maps for poly, optional, and var arg types
 * format is in pairs of specified type and types it can resolve into,
 * termintated by a NULL */
const char* POLY_IN_TYPES[] = {
    "x", "kacpri",
    "T", "Sicpr",
    "U", "Sikcpr",
    "i", "cpri",
    "k", "cprki",
    "B", "Bb", NULL};
const char* OPTIONAL_IN_TYPES[] = {
    "o", "icpr",
    "p", "icpr",
    "q", "icpr",
    "v", "icpr",
    "j", "icpr",
    "h", "icpr",
    "O", "kicpr",
    "J", "kicpr",
    "V", "kicpr",
    "P", "kicpr", NULL
};
const char* VAR_ARG_IN_TYPES[] = {
    "m", "icrp",
    "M", "icrpka",
    "N", "icrpkaS",
    "n", "icrp",   /* this one requires odd number of args... */
    "W", "S",
    "y", "a",
    "z", "kicrp",
    "Z", "kaicrp",  NULL  /* this one needs to be ka alternatating... */
};

const char* POLY_OUT_TYPES[] = {
    "s", "ka",
    "i", "pi", NULL
};

const char* VAR_ARG_OUT_TYPES[] = {
    "m", "a",
    "z", "k",
    "I", "Sip", /* had comment of (not implemented yet) in entry1.c */
    "X", "akip",
    "N", "akipS",
    "F", "f", NULL
};
