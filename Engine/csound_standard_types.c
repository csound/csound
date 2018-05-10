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
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 02110-1301 USA
 */

#include "csoundCore.h"
#include "csound_standard_types.h"
#include "pstream.h"
#include <stdlib.h>


/* MEMORY COPYING FUNCTIONS */

void myflt_copy_value(void* csound, void* dest, void* src) {
  IGN(csound);
    MYFLT* f1 = (MYFLT*)dest;
    MYFLT* f2 = (MYFLT*)src;
    *f1 = *f2;
}

void asig_copy_value(void* csound, void* dest, void* src) {
    IGN(csound);
    memcpy(dest, src, sizeof(MYFLT) * ((CSOUND*)csound)->ksmps);
}

void wsig_copy_value(void* csound, void* dest, void* src) {
    IGN(csound);
    memcpy(dest, src, sizeof(SPECDAT));
    //TODO - check if this needs to copy SPECDAT's DOWNDAT member and AUXCH
}

void fsig_copy_value(void* csound, void* dest, void* src) {
    PVSDAT *fsigout = (PVSDAT*) dest;
    PVSDAT *fsigin = (PVSDAT*) src;
    int N = fsigin->N;
    memcpy(dest, src, sizeof(PVSDAT) - sizeof(AUXCH));
    if(fsigout->frame.auxp == NULL ||
       fsigout->frame.size < (N + 2) * sizeof(float))
      ((CSOUND *)csound)->AuxAlloc(csound,
                                   (N + 2) * sizeof(float), &fsigout->frame);
    memcpy(fsigout->frame.auxp, fsigin->frame.auxp, (N + 2) * sizeof(float));
}


void string_copy_value(void* csound, void* dest, void* src) {
    STRINGDAT* sDest = (STRINGDAT*)dest;
    STRINGDAT* sSrc = (STRINGDAT*)src;
    CSOUND* cs = (CSOUND*)csound;

    if (UNLIKELY(src == NULL)) return;
    if (UNLIKELY(dest == NULL)) return;

    if (sSrc->size > sDest->size) {
      if (sDest->data != NULL) {
        cs->Free(cs, sDest->data);
      }
      sDest->data = cs_strdup(csound, sSrc->data);
      sDest->size = strlen(sDest->data)+1;
    } else {
      if (sDest->data == NULL) {
        sDest->data = cs_strdup(csound, sSrc->data);
        sDest->size = strlen(sDest->data)+1;
      } else {//breaks here
        //fprintf(stderr, "\n in:src %p size=%d >>>%s<<<dstsize=%d dst->data=%p\n",
        //   sSrc->data, sSrc->size, sSrc->data, sDest->size, sDest->data);
        //memcpy(sDest->data, sSrc->data, sDest->size);
        //memset(sDest->data,0,sDest->size);
        strncpy(sDest->data, sSrc->data, sDest->size-1);

        //cs->Free(cs, sDest->data); sDest->data = cs_strdup(csound, sSrc->data);
        //sDest->size = strlen(sDest->data)+1;
      }
    }
    //sDest->size = sSrc->size;
    //fprintf(stderr, "out:srcsize=%d >>>%s<<<dstsize=%d dst->data=%p\n",
    //        sSrc->size, sSrc->data, sDest->size, sDest->data);
}

static size_t array_get_num_members(ARRAYDAT* aSrc) {
    int i, retVal = 0;

    if (aSrc->dimensions <= 0) {
      return retVal;
    }

    retVal = aSrc->sizes[0];

    for (i = 1; i < aSrc->dimensions; i++) {
      retVal *= aSrc->sizes[i];
    }
    return (size_t)retVal;
}

void array_copy_value(void* csound, void* dest, void* src) {
    ARRAYDAT* aDest = (ARRAYDAT*)dest;
    ARRAYDAT* aSrc = (ARRAYDAT*)src;
    CSOUND* cs = (CSOUND*)csound;
    size_t j;
    int memMyfltSize;
    size_t arrayNumMembers;

    arrayNumMembers = array_get_num_members(aSrc);
    memMyfltSize = aSrc->arrayMemberSize / sizeof(MYFLT);

    if(aDest->data == NULL ||
       aSrc->arrayMemberSize != aDest->arrayMemberSize ||
       aSrc->dimensions != aDest->dimensions ||
       aSrc->arrayType != aDest->arrayType ||
       arrayNumMembers != array_get_num_members(aDest)) {

        aDest->arrayMemberSize = aSrc->arrayMemberSize;
        aDest->dimensions = aSrc->dimensions;
        if(aDest->sizes != NULL) {
            cs->Free(cs, aDest->sizes);
        }
        aDest->sizes = cs->Malloc(cs, sizeof(int) * aSrc->dimensions);
        memcpy(aDest->sizes, aSrc->sizes, sizeof(int) * aSrc->dimensions);
        aDest->arrayType = aSrc->arrayType;

        if(aDest->data != NULL) {
            cs->Free(cs, aDest->data);
        }
        aDest->data = cs->Calloc(cs, aSrc->arrayMemberSize * arrayNumMembers);
    }

    for (j = 0; j < arrayNumMembers; j++) {
        int index = j * memMyfltSize;
        aDest->arrayType->copyValue(csound,
                                    aDest->data + index, aSrc->data + index);
    }

}

/* MEM SIZE UPDATING FUNCTIONS */

void updateAsigMemBlock(void* csound, CS_VARIABLE* var) {
    CSOUND* cs = (CSOUND*)csound;
    int ksmps = cs->ksmps;
    var->memBlockSize = CS_FLOAT_ALIGN(ksmps * sizeof (MYFLT));
}

void varInitMemory(void *csound, CS_VARIABLE* var, MYFLT* memblock) {
    IGN(csound);
    memset(memblock, 0, var->memBlockSize);
}

void arrayInitMemory(void *csound, CS_VARIABLE* var, MYFLT* memblock) {
    IGN(csound);
    ARRAYDAT* dat = (ARRAYDAT*)memblock;
    dat->arrayType = var->subType;
}

void varInitMemoryString(void *csound, CS_VARIABLE* var, MYFLT* memblock) {
    IGN(var);
    STRINGDAT *str = (STRINGDAT *)memblock;
    CSOUND* cs = (CSOUND*)csound;
    str->data = (char *) cs->Calloc(csound, 8);
    str->size = 8;
    //printf("initialised %s %p %s %d\n", var->varName, str,  str->data, str->size);
}

void varInitMemoryFsig(void *csound, CS_VARIABLE* var, MYFLT* memblock) {
    IGN(var);
    PVSDAT *fsig = (PVSDAT *)memblock;
    IGN(csound);
    memset(fsig, 0, sizeof(PVSDAT));  /* VL: clear memory for now */
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
    var->memBlockSize = CS_FLOAT_ALIGN(ksmps * sizeof (MYFLT));
    var->updateMemBlockSize = &updateAsigMemBlock;
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

CS_VARIABLE* createMyflt(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof (MYFLT));
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

CS_VARIABLE* createBool(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof (MYFLT));
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

CS_VARIABLE* createWsig(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(SPECDAT));
    var->initializeVariableMemory = &varInitMemory;
    return var;
}

CS_VARIABLE* createFsig(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(PVSDAT));
    var->initializeVariableMemory = &varInitMemoryFsig;
    return var;
}


CS_VARIABLE* createString(void* cs, void* p) {
    CSOUND* csound = (CSOUND*)cs;
    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    IGN(p);
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(STRINGDAT));
    var->initializeVariableMemory = &varInitMemoryString;
    return var;
}

CS_VARIABLE* createArray(void* csnd, void* p) {
    CSOUND* csound = (CSOUND*)csnd;
    ARRAY_VAR_INIT* state = (ARRAY_VAR_INIT*)p;


    CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
    var->memBlockSize = CS_FLOAT_ALIGN(sizeof(ARRAYDAT));
    var->initializeVariableMemory = &arrayInitMemory;

    if (state) { // NB: this function is being called with p=NULL
      CS_TYPE* type = state->type;
      var->subType = type;
      var->dimensions = state->dimensions;
    }
    return var;
}

/* FREE VAR MEM FUNCTIONS */

void string_free_var_mem(void* csnd, void* p ) {
    CSOUND* csound = (CSOUND*)csnd;
    STRINGDAT* dat = (STRINGDAT*)p;

    if(dat->data != NULL) {
        csound->Free(csound, dat->data);
    }
}

void array_free_var_mem(void* csnd, void* p) {
    CSOUND* csound = (CSOUND*)csnd;
    ARRAYDAT* dat = (ARRAYDAT*)p;

    if(dat->data != NULL) {
        CS_TYPE* arrayType = dat->arrayType;

        if (arrayType->freeVariableMemory != NULL) {
            MYFLT* mem = dat->data;
            size_t memMyfltSize = dat->arrayMemberSize / sizeof(MYFLT);
            int i, size = dat->sizes[0];
            for (i = 1; i < dat->dimensions; i++) {
                size *= dat->sizes[i];
            }
            //size = MYFLT2LRND(size); // size is not a float  but int
            for (i = 0; i < size; i++) {
                arrayType->freeVariableMemory(csound,
                                              mem+ (i * memMyfltSize));
            }
        }

        csound->Free(csound, dat->data);
    }

    if (dat->sizes != NULL) {
        csound->Free(csound, dat->sizes);
    }
}

/* STANDARD TYPE DEFINITIONS */
const CS_TYPE CS_VAR_TYPE_A = {
  "a", "audio rate vector", CS_ARG_TYPE_BOTH, createAsig, asig_copy_value,
  NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_K = {
  "k", "control rate var", CS_ARG_TYPE_BOTH, createMyflt, myflt_copy_value,
  NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_I = {
  "i", "init time var", CS_ARG_TYPE_BOTH, createMyflt, myflt_copy_value,
  NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_S = {
    "S", "String var", CS_ARG_TYPE_BOTH, createString, string_copy_value,
    NULL, string_free_var_mem
};

const CS_TYPE CS_VAR_TYPE_P = {
  "p", "p-field", CS_ARG_TYPE_BOTH, createMyflt, myflt_copy_value,
  NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_R = {
  "r", "reserved symbol", CS_ARG_TYPE_BOTH, createMyflt, myflt_copy_value,
  NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_C = {
  "c", "constant", CS_ARG_TYPE_IN, createMyflt, myflt_copy_value, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_W = {
  "w", "spectral", CS_ARG_TYPE_BOTH, createWsig, wsig_copy_value, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_F = {
  "f", "f-sig", CS_ARG_TYPE_BOTH, createFsig, fsig_copy_value, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_B = {
  "B", "boolean", CS_ARG_TYPE_BOTH, createBool, myflt_copy_value, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_b = {
  "b", "boolean", CS_ARG_TYPE_BOTH, createBool, myflt_copy_value, NULL, NULL
};

const CS_TYPE CS_VAR_TYPE_ARRAY = {
   "[", "array", CS_ARG_TYPE_BOTH, createArray, array_copy_value,
    NULL, array_free_var_mem
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
    "x", "kacpri",              /* ***Deprecated*** */
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
    "s", "ka",                  /* ***Deprecated*** */
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
