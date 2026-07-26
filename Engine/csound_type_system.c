/*
 csound_type_system.c:

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
 Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
 */

#include <string.h>
#include <stdio.h>
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include "csound_orc_structs.h"
#include "arrays_internal.h"
#include "csoundCore.h"
#include "aops.h"
#include "arrays.h"

/* Forward declaration for p-field string extraction */
extern char* csoundGetArgString(CSOUND *csound, MYFLT p);

static int32_t type_exists_with_same_name(TYPE_POOL* pool, CS_TYPE* typeInstance) {
    CS_TYPE_ITEM* current = pool->head;
    while (current != NULL) {

      /* printf("Search if type [%s] == [%s]",
         current->varTypeName, typeInstance->varTypeName); */

      if (strcmp(current->cstype->varTypeName,
                 typeInstance->varTypeName) == 0) {
        return 1;
      }
      current = current->next;
    }

    return 0;
}

const CS_TYPE* csoundGetTypeWithVarTypeName(const TYPE_POOL* pool, const char* typeName) {

    CS_TYPE_ITEM* current = pool->head;
    if (typeName == NULL) return NULL;
    size_t namelen = strlen(typeName);
    if (namelen == 0) return NULL;

    // First try direct match
    while (current != NULL) {
      if (strcmp(typeName, current->cstype->varTypeName) == 0) {
        return current->cstype;
      }
      current = current->next;
    }

    // If no direct match and typeName starts with ':', try stripping the quotes
    if (typeName[0] == ':' && namelen > 2 && typeName[namelen-1] == ';') {
      char unquotedName[256];
      snprintf(unquotedName, 256, "%.*s", (int)(namelen - 2), typeName + 1);
      current = pool->head;
      while (current != NULL) {
        if (strcmp(unquotedName, current->cstype->varTypeName) == 0) {
          return current->cstype;
        }
        current = current->next;
      }
    }

    // If no direct match and typeName doesn't start with ':', try internal struct format
    if (typeName[0] != ':' && typeName[0] != '[') {
      char internalName[256];
      snprintf(internalName, 256, ":%s;", typeName);
      current = pool->head;
      while (current != NULL) {
        if (strcmp(internalName, current->cstype->varTypeName) == 0) {
          return current->cstype;
        }
        current = current->next;
      }
    }

    if(UNLIKELY(typeName[namelen-1] != ']')) return NULL;
    // now check again with braces
    char type[64];
    current = pool->head;
    while (current != NULL) {
      snprintf(type, 64, "%s[]", current->cstype->varTypeName);
      if (strcmp(typeName, type) == 0) {
        return current->cstype;
      }
      current = current->next;
    }
    return NULL;
}

int32_t csoundAddVariableType(CSOUND* csound, TYPE_POOL* pool, CS_TYPE* typeInstance)
{
    CS_TYPE_ITEM* item;
    if (type_exists_with_same_name(pool, typeInstance)) {
      return 0;
    }

    item = (CS_TYPE_ITEM*)csound->Calloc(csound, sizeof(CS_TYPE_ITEM));
    item->cstype = typeInstance;

    if (pool->head == NULL) {
      pool->head = item;
    } else {
      CS_TYPE_ITEM* current = pool->head;
      while (current->next) {
        current = current->next;
      }
      current->next = item;
      item->next = NULL;
    }

    /* printf("Adding type with type name: %s\n", typeInstance->varTypeName); */

    return 1;
}

TYPE_POOL *csoundGetTypePool(CSOUND* csound) {
  return csound->typePool;
}


/* VAR POOL FUNCTIONS */


CS_VAR_POOL* csoundCreateVarPool(CSOUND* csound) {
    CS_VAR_POOL* varPool = csound->Calloc(csound, sizeof(CS_VAR_POOL));
    if (varPool == NULL || (uintptr_t)varPool < 0x1000) {
        csound->Message(csound, "ERROR: csoundCreateVarPool failed to allocate memory, got %p\n", varPool);
        return NULL;
    }
    varPool->table = cs_hash_table_create(csound);
    if (varPool->table == NULL) {
        csound->Message(csound, "ERROR: cs_hash_table_create failed in csoundCreateVarPool\n");
        csound->Free(csound, varPool);
        return NULL;
    }

    return varPool;
}

void csoundFreeVarPool(CSOUND* csound, CS_VAR_POOL* pool) {
    if(pool->table) cs_hash_table_mfree_complete(csound, pool->table);
    csound->Free(csound, pool);
}

char* csoundGetVarSimpleName(CSOUND* csound, const char* varName) {
    char* retVal;

    if (varName[0] != '[') {
      retVal = (char*)csound->Calloc(csound, sizeof(char) * (strlen(varName) + 1));
      strcpy(retVal, varName);
    } else {
      int32_t start = 0;
      int32_t typeEnd = 0;
      int32_t len = (int32_t) strlen(varName);
      int32_t newFirstLen, newSecondLen, newTotalLen;
      char* t = (char*) varName;
      char* t2;

      while(*t == '[') {
        t++;
        start++;
      }
      typeEnd = start;
      t2 = t;
      while(*t2 != ']' && *t2 != (char)0) {
        t2++;
        typeEnd++;
      }
      t2++;
      typeEnd++;

      newFirstLen = (typeEnd - start - 1);
      newSecondLen = (len - typeEnd);
      newTotalLen = newFirstLen + newSecondLen;

      retVal = (char*)csound->Calloc(csound, sizeof(char) * (newTotalLen + 1));
      strncpy(retVal, t, newFirstLen);
      strncpy(retVal + newFirstLen, t2, newSecondLen);
    }

    return retVal;
}


CS_VARIABLE* csoundCreateVariableForType(CSOUND* csound,
                                         const CS_TYPE* type,
                                         const void* typeArg,
                                         INSDS* ctx)
{
    CS_VARIABLE *var;

    if (UNLIKELY(csound == NULL || type == NULL ||
                 type->createVariable == NULL)) {
      return NULL;
    }
    var = type->createVariable(csound, type, typeArg, ctx);
    if (UNLIKELY(var == NULL)) {
      return NULL;
    }
    if (UNLIKELY(var->memBlockSize <= 0)) {
      csound->Free(csound, var);
      return NULL;
    }
    var->varType = type;
    return var;
}

/** Create variable outside an instrument context */
CS_VARIABLE* csoundCreateVariable(CSOUND* csound, TYPE_POOL* pool,
                                  const CS_TYPE* type, char* name,
                                  const void* typeArg)
{
    CS_TYPE_ITEM* current = pool->head;
    if (LIKELY(type != NULL))
      while (current != NULL) {
        if (strcmp(type->varTypeName, current->cstype->varTypeName) == 0) {
          CS_VARIABLE* var = csoundCreateVariableForType(
              csound, current->cstype, typeArg, NULL);
          if (UNLIKELY(var == NULL)) {
            ((CSOUND *)csound)->ErrorMsg(csound,
              Str("cannot create variable %s: type '%s' createVariable returned NULL\n"),
              name ? name : "(null)", type ? type->varTypeName : "(null)");
            return NULL;
          }
          var->varName = csoundStrdup(csound, name);
          return var;
        }
        current = current->next;
      }
    else {
      // Always print debug info for NULL type errors
      ((CSOUND *)csound)->ErrorMsg(csound, Str("cannot create variable %s: NULL type\n"), name);
    }
    return NULL;
}

CS_VARIABLE* csoundFindVariableWithName(CSOUND* csound, CS_VAR_POOL* pool,
                                        const char* name)
{
    // Check for null or corrupted pool to prevent segfault
    if (pool == NULL || (uintptr_t)pool < 0x1000) {
      csound->ErrorMsg(csound, "csoundFindVariableWithName: skipping due to null/corrupted pool (pool=%p, name=%s)\n", pool, name ? name : "(null)");
      return NULL;
    }

    CS_VARIABLE* returnValue = cs_hash_table_get(csound, pool->table, (char*)name);

    if (returnValue == NULL && pool->parent != NULL) {
      returnValue = csoundFindVariableWithName(csound, pool->parent, name);
    }

    return returnValue;
}

CS_VARIABLE* csoundGetVariable(CS_VAR_POOL* pool, int32_t index) {

    CS_VARIABLE* current = pool->head;
    int32_t i;

    for(i = 0; i < index || current != NULL; i++) {
      /* THIS WAS WRONG!! && or || meant foR , ?? */
      current = current->next;
    }

    return current;
}

int32_t csoundAddVariable(CSOUND* csound, CS_VAR_POOL* pool, CS_VARIABLE* var) {
  // Check for null or corrupted pool to prevent segfault
  if (pool == NULL || (uintptr_t)pool < 0x1000) {  // Detect corrupted small addresses
    csound->ErrorMsg(csound, "csoundAddVariable: skipping due to null/corrupted pool (pool=%p)\n", pool);
    return 0;
  }

  // Pool must have been created with csoundCreateVarPool (which always sets table)
  // If table is NULL, treat this as an invalid/mis-cast pool and bail out safely.
  if (pool->table == NULL) {
    csound->ErrorMsg(csound,
                     "csoundAddVariable: invalid pool (table==NULL) at %p; refusing to add var '%s'\n",
                     (void*)pool, var ? var->varName : "(null)");
    return -1;
  }

  if(var != NULL) {
    if(pool->head == NULL) {
      pool->head = var;
      pool->tail = var;
    } else {
      pool->tail->next = var;
      pool->tail = var;
    }
    cs_hash_table_put(csound, pool->table, var->varName, var);
    // may need to revise this; var pools are accessed as MYFLT*,
    // so need to ensure all memory is aligned to sizeof(MYFLT)
    var->memBlockIndex = (pool->poolSize / sizeof(MYFLT)) +
      ((pool->varCount + 1) * (CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET) / sizeof(MYFLT)));
    pool->poolSize += var->memBlockSize;
    pool->varCount += 1;
    return 0;
  } else return -1;
}


void csoundRecalculateVarPoolMemory(CSOUND* csound, CS_VAR_POOL* pool)
{
    // Check for null or corrupted pool to prevent segfault
    if (pool == NULL) {
        csound->ErrorMsg(csound, "csoundRecalculateVarPoolMemory: pool is NULL, skipping\n");
        return;
    }
    if ((uintptr_t)pool < 0x1000) {
        csound->ErrorMsg(csound, "csoundRecalculateVarPoolMemory: pool address %p is corrupted, skipping\n", pool);
        return;
    }

    CS_VARIABLE* current = pool->head;
    int32_t varCount = 1;
    pool->poolSize = 0;

    while (current != NULL) {
      /* VL 26-12-12: had to revert these lines to avoid memory crashes
         with higher ksmps */
      if(current->updateMemBlockSize != NULL) {
        current->updateMemBlockSize(csound, current);
      }

      current->memBlockIndex = (pool->poolSize / sizeof(MYFLT)) +
        (varCount * CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET) / sizeof(MYFLT));
      pool->poolSize += current->memBlockSize;

      current = current->next;
      varCount++;
    }
}

void csoundReallocateVarPoolMemory(CSOUND* csound, CS_VAR_POOL* pool) {
    // Check for null or corrupted pool to prevent segfault
    if (pool == NULL || (uintptr_t)pool < 0x1000) {  // Detect corrupted small addresses
      csound->Message(csound, "csoundReallocateVarPoolMemory: skipping due to null/corrupted pool (pool=%p)\n", pool);
      return;
    }

    CS_VARIABLE* current = pool->head;
    CS_VAR_MEM* varMem = NULL;
    size_t memSize;
    pool->poolSize = 0;

    while (current != NULL) {
      varMem = current->memBlock;
      memSize = current->memBlockSize;

      if(current->updateMemBlockSize != NULL) {
        current->updateMemBlockSize(csound, current);
      }
      // VL 14-3-2015 only realloc if we need to
      if(memSize < (size_t)current->memBlockSize) {
          memSize = CS_VAR_TYPE_OFFSET + current->memBlockSize;
          varMem =
               (CS_VAR_MEM *)((CSOUND *)csound)->ReAlloc(csound,varMem,
                                             memSize);
	  varMem->varType = current->varType;
          current->memBlock = varMem;
       }
       pool->poolSize += current->memBlockSize;
       current = current->next;
    }
}

void csoundDeleteVarPoolMemory(CSOUND* csound, CS_VAR_POOL* pool) {
    CS_VARIABLE* current = pool->head, *tmp;
    const CS_TYPE* type;
    while (current != NULL) {
      tmp = current;
      type = current->subType;
      if (type->freeVariableMemory != NULL) {
        type->freeVariableMemory(csound, current->memBlock);
      }
      csound->Free(csound, current->memBlock);
      current = current->next;
      csound->Free(csound, tmp);
    }
}



void csoundInitializeVarPool(CSOUND* csound, MYFLT* memBlock, CS_VAR_POOL* pool) {
    if (pool == NULL) {
        csound->ErrorMsg(csound, "Warning: csoundInitializeVarPool called with NULL pool\n");
        return;
    }
    CS_VARIABLE* current = pool->head;
    //int varNum = 1;

    while (current != NULL) {
      if (current->initializeVariableMemory != NULL) {
        current->initializeVariableMemory(csound, current,
                                          memBlock + current->memBlockIndex);
      }
      current = current->next;
    }
}

void debug_print_varpool(CSOUND* csound, CS_VAR_POOL* pool) {
    if (pool == NULL) {
        csound->Warning(csound, "Warning: debug_print_varpool called with NULL pool\n");
        return;
    }
    CS_VARIABLE* gVar = pool->head;
    int32_t count = 0;
    while(gVar != NULL) {
      csound->Message(csound, "  %d) %s:%s\n", count++,
                      gVar->varName, gVar->varType->varTypeName);
      gVar = gVar->next;
    }
}

static int32_t copy_var_no_op(CSOUND *csound, void *p) {
  return OK;
}

/**
 * Helper function to normalize type names for comparison.
 * Converts both ":TypeName;" and "TypeName" to the same format for comparison.
 * Returns a newly allocated string that must be freed by the caller.
 */
static char* normalize_type_name_for_comparison(CSOUND *csound, const char* typeName) {
  if (!typeName) return NULL;

  // If it's already in internal format (:TypeName;), extract the core name
  if (typeName[0] == ':' && typeName[strlen(typeName) - 1] == ';') {
    size_t len = strlen(typeName) - 2; // Remove : and ;
    char* result = csound->Malloc(csound, len + 1);
    memcpy(result, typeName + 1, len);
    result[len] = '\0';
    return result;
  }

  // Otherwise, return a copy of the original name
  return csoundStrdup(csound, typeName);
}

/**
 * Check if two type names represent the same type, handling both
 * internal format (:TypeName;) and external format (TypeName).
 */
static int types_are_equivalent(CSOUND *csound, const char* type1, const char* type2) {
  if (!type1 || !type2) return 0;

  // Quick check for exact match
  if (strcmp(type1, type2) == 0) return 1;

  // Normalize both type names and compare
  char* norm1 = normalize_type_name_for_comparison(csound, type1);
  char* norm2 = normalize_type_name_for_comparison(csound, type2);

  int result = (norm1 && norm2) ? (strcmp(norm1, norm2) == 0) : 0;

  if (norm1) csound->Free(csound, norm1);
  if (norm2) csound->Free(csound, norm2);

  return result;
}



/* GENERIC VARIABLE COPYING */
static int32_t copy_var_generic_impl(CSOUND *csound, void *p,
                                     CSOUND_STRUCT_COPY_MODE structCopyMode,
                                     int32_t initializing) {
    ASSIGN* assign = (ASSIGN*)p;
    CS_TYPE* typeR = csoundGetTypeForArg(assign->r);
    CS_TYPE* typeA = csoundGetTypeForArg(assign->a);

    // Check for type equivalence, handling both internal (:Type;) and external (Type) formats
    int types_different = (typeR != typeA) && !types_are_equivalent(csound, typeR->varTypeName, typeA->varTypeName);

    if(types_different) {
      /* Allow numeric constant 'c' and p-field 'p' to be assigned to numeric variables (i/k).
         Underlying storage is MYFLT-compatible, so a direct copy is valid. */
      int allow_num_to_num = ((typeR == &CS_VAR_TYPE_I || typeR == &CS_VAR_TYPE_K) && (typeA == &CS_VAR_TYPE_C || typeA == &CS_VAR_TYPE_P));
      /* Allow scalar-to-audio by broadcasting the scalar across the audio block. */
      int allow_scalar_to_audio = (typeR == &CS_VAR_TYPE_A && (typeA == &CS_VAR_TYPE_C || typeA == &CS_VAR_TYPE_I || typeA == &CS_VAR_TYPE_K));
      /* Allow p-field to string assignment when p-field contains string data. */
      int allow_pfield_to_string = (typeR == &CS_VAR_TYPE_S && typeA == &CS_VAR_TYPE_P);

      if (!allow_num_to_num && !allow_scalar_to_audio && !allow_pfield_to_string) {
        if(assign->h.perf != copy_var_no_op)
          return csound->PerfError(csound,&(assign->h),
            Str("Opcode given variables "
                "with two different types: %s : %s"),
            typeR->varTypeName, typeA->varTypeName);
        else
          return csound->InitError(csound,
            Str("Opcode given variables "
                "with two different types: %s : %s"),
            typeR->varTypeName, typeA->varTypeName);
      }
      if (allow_pfield_to_string) {
        /* P-field to string assignment - perform the actual conversion */
        /* This is called at runtime when the instrument is executing */
        MYFLT pval = *assign->a;
        STRINGDAT *strOut = (STRINGDAT *)assign->r;

        if (IsStringCode(pval)) {
          /* P-field contains string data - extract it using csoundGetArgString */
          const char *pstr = csoundGetArgString(csound, pval);

          if (pstr != NULL && strlen(pstr) > 0) {
            if (strOut->data != NULL) {
              csound->Free(csound, strOut->data);
            }
            strOut->data = csound->Strdup(csound, pstr);
            strOut->size = strlen(pstr) + 1;
          } else {
            /* String code but no string data - set empty string */
            if (strOut->data != NULL) {
              csound->Free(csound, strOut->data);
            }
            strOut->data = csound->Strdup(csound, "");
            strOut->size = 1;
          }
        } else {
          /* P-field contains numeric data - convert to string representation */
          if (strOut->data != NULL) {
            csound->Free(csound, strOut->data);
          }
          char numStr[64];
          snprintf(numStr, sizeof(numStr), "%.17g", pval);
          strOut->data = csound->Strdup(csound, numStr);
          strOut->size = strlen(numStr) + 1;
        }
        return OK;
      }
      if (allow_scalar_to_audio) {
        /* Broadcast scalar 'a' to audio 'r' (equivalent to ainit/upsamp behavior). */
        MYFLT val = *assign->a;
        uint32_t nsmps = assign->h.insdshead->ksmps;
        uint32_t offset = assign->h.insdshead->ksmps_offset;
        uint32_t early  = assign->h.insdshead->ksmps_no_end;
        if (UNLIKELY(offset)) memset(assign->r, '\0', offset*sizeof(MYFLT));
        if (UNLIKELY(early)) {
          nsmps -= early;
          memset(&assign->r[nsmps], '\0', early*sizeof(MYFLT));
        }
        for (uint32_t n = offset; n < nsmps; n++) assign->r[n] = val;
        return OK;
      }
    }



    if (typeR->userDefinedType) {
      if (UNLIKELY(csound_copy_struct_value(
                     csound, typeR, assign->r, assign->a,
                     assign->h.insdshead,
                     structCopyMode) != OK)) {
        return initializing
          ? csound->InitError(csound, "could not copy structured value")
          : csound->PerfError(csound, &assign->h,
                              "could not copy structured value");
      }
      return OK;
    }
    typeR->copyValue(csound, typeR, assign->r, assign->a,
                     assign->h.insdshead);
    return OK;
}

int32_t copy_var_generic(CSOUND *csound, void *p) {
    /* Init prepares independent nested storage. Performance copies may update
       that storage, but must not detach or clone a shared aggregate. */
    return copy_var_generic_impl(
      csound, p, CSOUND_STRUCT_COPY_INDEPENDENT_NO_ALLOCATION, 0);
}


int32_t copy_var_generic_init(CSOUND *csound, void *p)
{
    ASSIGN *assign = (ASSIGN *)p;

    // Determine destination (right-hand?) type once.
    CS_TYPE *destType = csoundGetTypeForArg(assign->r);
    CS_TYPE *srcType = csoundGetTypeForArg(assign->a);

    // If destination is an array, handle array-specific init/copy paths.
    if (destType == &CS_VAR_TYPE_ARRAY) {
        ARRAYDAT *dstArr = (ARRAYDAT *)assign->r;
        if (srcType != &CS_VAR_TYPE_ARRAY) {
            // Source is a scalar (e.g., assigning to array element), not array-to-array
            return copy_var_generic_impl(
              csound, p, CSOUND_STRUCT_COPY_INDEPENDENT_ALLOW_ALLOCATION, 1);
        }
        ARRAYDAT *srcArr = (ARRAYDAT *)assign->a;
        if (srcArr->arrayType && srcArr->arrayType->userDefinedType) {
            assign->h.perf = copy_var_no_op;
            if (UNLIKELY(csound_array_copy_independent(
                           csound, dstArr, srcArr, assign->h.insdshead,
                           CSOUND_ARRAY_COPY_ALLOW_ALLOCATION) != OK)) {
              return csound->InitError(
                csound, "could not copy structured array value");
            }
            return OK;
        }
        // If the destination really is an array, make it like the source.
        if (csoundGetTypeForArg(dstArr) == &CS_VAR_TYPE_ARRAY) {
            if (UNLIKELY(tabinit_like(csound, dstArr, srcArr) != OK)) {
                return csound->InitError(
                  csound, "%s",
                  Str("array assignment: could not initialize destination"));
            }
        }

        // Special-case: complex arrays copy immediately (no perf hook, no flag).
        if (srcArr->arrayType == &CS_VAR_TYPE_COMPLEX) {
            return copy_var_generic_impl(
              csound, p, CSOUND_STRUCT_COPY_INDEPENDENT_ALLOW_ALLOCATION, 1);
        }

        // For integer or instrument arrays, fall through to the "flag" path below.
        if (srcArr->arrayType == &CS_VAR_TYPE_I ||
            srcArr->arrayType == &CS_VAR_TYPE_INSTR) {
            assign->h.perf = copy_var_no_op;
            return copy_var_generic_impl(
              csound, p, CSOUND_STRUCT_COPY_INDEPENDENT_ALLOW_ALLOCATION, 1);
        }

        // Other array element types: nothing more to do here.
        return OK;
    }

    return copy_var_generic_impl(
      csound, p, CSOUND_STRUCT_COPY_INDEPENDENT_ALLOW_ALLOCATION, 1);
}




int32_t type_of(CSOUND *csound, ASSIGN *p) {
  CS_TYPE *type = GetTypeForArg(p->a);
  STRINGDAT *types = (STRINGDAT *) p->r;
  strncpy(types->data, type->varTypeName, types->size);
  return OK;
}

int32_t check_type(CSOUND *csound, RELAT *p) {
  *p->rbool = (GetTypeForArg(p->a) == GetTypeForArg(p->b)) ? 1 : 0;
  return OK;
}
