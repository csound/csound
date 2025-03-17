/*
  opcode.c: opcode manipulation

  Copyright (C) 1997 John ffitch
  (C) 2005 Istvan Varga
  (C) 2024 V Lazzarini

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
#include "opcode.h"
#include "udo.h"
#include <ctype.h>
#include "interlocks.h"

/* Print opcodes in system */
/* John ffitch -- 26 Jan 97 */
/*  4 april 02 -- ma++ */
/*  restructure to retrieve externally  */
/* And suppressing deprecated Oct 2015 -- JPff */
static int32_t opcode_cmp_func(const void *a, const void *b) {
  opcodeListEntry *ep1 = (opcodeListEntry *)a;
  opcodeListEntry *ep2 = (opcodeListEntry *)b;
  int32_t retval;

  if ((retval = strcmp(ep1->opname, ep2->opname)) != 0)
    return retval;
  if ((retval = strcmp(ep1->outypes, ep2->outypes)) != 0)
    return retval;
  if ((retval = strcmp(ep1->intypes, ep2->intypes)) != 0)
    return retval;
  if (ep1 < ep2)
    return -1;
  if (ep1 > ep2)
    return 1;

  return 0;
}

/**
 * Gets an alphabetically sorted list of all opcodes.
 * Should be called after externals are loaded by csoundCompile().
 * Returns the number of opcodes, or a negative error code on failure.
 * Make sure to call csoundDisposeOpcodeList() when done with the list.
 */

PUBLIC int32_t csoundNewOpcodeList(CSOUND *csound, opcodeListEntry **lstp) {
  void *lst = NULL;
  OENTRY *ep;
  char *s;
  size_t nBytes = (size_t)0;
  int32_t i, cnt = 0;
  CONS_CELL *head, *items, *temp;

  (*lstp) = NULL;
  if (UNLIKELY(csound->opcodes == NULL))
    return -1;

  head = items = cs_hash_table_values(csound, csound->opcodes);

  /* count the number of opcodes, and bytes to allocate */
  while (items != NULL) {
    temp = items->value;
    while (temp != NULL) {
      ep = temp->value;
      if (ep->opname != NULL && ep->opname[0] != '\0' &&
          isalpha(ep->opname[0]) && ep->outypes != NULL &&
          ep->intypes != NULL) {
        cnt++;
#ifdef JPFF
        if (strchr(ep->intypes, 'x'))
          printf("%s, type %d %s -> %s\n", ep->opname,
                 (ep->init && ep->perf ? 3
                  : ep->init           ? 1
                                       : 2) ep->intypes,
                 ep->outypes);
#endif
        nBytes += sizeof(opcodeListEntry);
        for (i = 0; ep->opname[i] != '\0' && ep->opname[i] != '.'; i++)
          ;
        nBytes += (size_t)i;
        nBytes += strlen(ep->outypes);
        nBytes += strlen(ep->intypes);
        nBytes += 3; /* for null characters */
      }
      temp = temp->next;
    }
    items = items->next;
  }
  nBytes += sizeof(opcodeListEntry);
  /* allocate memory for opcode list */
  lst = csound->Malloc(csound, nBytes);
  if (UNLIKELY(lst == NULL))
    return CSOUND_MEMORY;
  (*lstp) = (opcodeListEntry *)lst;
  /* store opcodes in list */
  items = head;
  s = (char *)lst + ((int32_t)sizeof(opcodeListEntry) * (cnt + 1));
  cnt = 0;
  while (items != NULL) {
    temp = items->value;
    while (temp != NULL) {
      ep = temp->value;

      if (ep->opname != NULL && ep->opname[0] != '\0' &&
          isalpha(ep->opname[0]) && ep->outypes != NULL &&
          ep->intypes != NULL) {
        for (i = 0; ep->opname[i] != '\0' && ep->opname[i] != '.'; i++)
          s[i] = ep->opname[i];
        s[i++] = '\0';
        ((opcodeListEntry *)lst)[cnt].opname = s;
        s += i;
        strcpy(s, ep->outypes);
        ((opcodeListEntry *)lst)[cnt].outypes = s;
        s += ((int32_t)strlen(ep->outypes) + 1);
#ifdef JPFF
        if (strlen(ep->outypes) == 0)
          printf("***potential WI opcode %s\n", ep->opname);
#endif
        strcpy(s, ep->intypes);
        ((opcodeListEntry *)lst)[cnt].intypes = s;
        s += ((int32_t)strlen(ep->intypes) + 1);
        ((opcodeListEntry *)lst)[cnt].flags = ep->flags;
        // if (ep->flags&_QQ) printf("DEPRICATED: %s\n", ep->opname);
        // if (ep->flags&_QQ) *deprec++;
        cnt++;
      }
      temp = temp->next;
    }
    items = items->next;
  }
  ((opcodeListEntry *)lst)[cnt].opname = NULL;
  ((opcodeListEntry *)lst)[cnt].outypes = NULL;
  ((opcodeListEntry *)lst)[cnt].intypes = NULL;
  ((opcodeListEntry *)lst)[cnt].flags = 0;

  cs_cons_free(csound, head);

  /* sort list */
  qsort(lst, (size_t)cnt, sizeof(opcodeListEntry), opcode_cmp_func);

  /* return the number of opcodes */
  return cnt;
}

PUBLIC void csoundDisposeOpcodeList(CSOUND *csound, opcodeListEntry *lst) {
  csound->Free(csound, lst);
}

void list_opcodes(CSOUND *csound, int32_t level) {
  opcodeListEntry *lst;
  const char *sp = "                    "; /* length should be 20 */
  int32_t j, k;
  int32_t cnt, len = 0, xlen = 0;
  int32_t count = 0;

  cnt = csoundNewOpcodeList(csound, &lst);
  if (UNLIKELY(cnt <= 0)) {
    csound->ErrorMsg(csound, Str("Error creating opcode list"));
    csoundDisposeOpcodeList(csound, lst);
    return;
  }

  for (j = 0, k = -1; j < cnt; j++) {
    if ((level & 1) == 0) { /* Print in 4 columns */
      if (j > 0 && strcmp(lst[j - 1].opname, lst[j].opname) == 0)
        continue;
      if ((level & 2) == 0 && ((lst[j].flags & _QQ) != 0)) {
        // printf("dropping %s\n", lst[j].opname);
        continue;
      }
      k++;
      xlen = 0;
      if (!(k & 3))
        csound->Message(csound, "\n");
      else {
        if (len > 19) {
          xlen = len - 19;
          len = 19;
        }
        csound->Message(csound, "%s", sp + len);
      }
      csound->Message(csound, "%s", lst[j].opname);
      len = (int32_t)strlen(lst[j].opname) + xlen;
    } else {
      char *ans = lst[j].outypes, *arg = lst[j].intypes;
      if ((level & 2) == 0 && ((lst[j].flags & _QQ) != 0)) {
        // printf("dropping %s\n", lst[j].opname);
        continue;
      }
      csound->Message(csound, "%s", lst[j].opname);
      len = (int32_t)strlen(lst[j].opname);
      if (len > 11) {
        xlen = len - 11;
        len = 11;
      }
      csound->Message(csound, "%s", sp + (len + 8));
      if (ans == NULL || *ans == '\0')
        ans = "(null)";
      if (arg == NULL || *arg == '\0')
        arg = "(null)";
      csound->Message(csound, "%s", ans);
      len = (int32_t)strlen(ans) + xlen;
      len = (len < 11 ? len : 11);
      xlen = 0;
      csound->Message(csound, "%s", sp + (len + 8));
      csound->Message(csound, "%s\n", arg);
    }
    count++;
  }
  csound->Message(csound, "\n");
  csound->Message(csound, Str("%d opcodes\n\n"), count);
  csoundDisposeOpcodeList(csound, lst);
}

/** 
 * Opcodes for OpcodeDef and Opcode types
 *
 **/
struct oentries *find_opcode2(CSOUND *, char *);
void *find_or_add_constant(CSOUND *csound, CS_HASH_TABLE *constantsPool,
                           const char *name, MYFLT value);

/** exclude some names 
 */
static int32_t  check_name(const char *name) {
  const char *exs[] = { "in", "out", "xin", "xout",  
                        "instr", "endin", "opcode", "endop",
                        "pset", "subinstr", "goto", "igoto",
                        "cigoto", "ckgoto", "cggoto", "reinit",
                        "timout","kgoto", "declare","setksmps",
                        "oversample", "undersample", "create",
                        "delete", "rigoto", "rireturn", "tigoto",
                        ""};
  int i = 0;
  for(i = 0; *exs[i] != '\0'; i++)
    if(strcmp(exs[i], name) == 0) return NOTOK;
  return OK;
}

/** check opcodes without any inputs
 */
static int32_t check_oentry(OENTRY *ep) {
  if(ep->intypes &&
     *(ep->intypes) != '\0') return 1; 
    else return 0;
}

CS_VARIABLE *addGlobalVariable(CSOUND *csound, ENGINE_STATE *engineState, CS_TYPE *type,
                               char *name, void *typeArg);

/** This function takes an OENTRY and adds a corresponding
 *   OpcodeDef global var, if the variable does not exist.
 *   skipping the ones with non-alphabetic names    
 */
void add_opcode_def(CSOUND *csound, OENTRY *ep) {
  char *name, *varName;
  OPCODEREF ref = {NULL, 0}, *dest;
  const CS_TYPE *type = &CS_VAR_TYPE_OPCODEREF;
  CS_VARIABLE *var;
  name = get_opcode_short_name(csound, ep->opname);
  if(isalpha(*name) != 0 && strlen(name) > 1
     && check_name(name) == OK) { // 
    varName = csound->Calloc(csound, strlen(name) + 2);
    // add underscore to opcodes with no inputs 
    snprintf(varName, strlen(name) + 2, check_oentry(ep) ? "%s" : "_%s", name);  
    if ((var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                          varName)) == NULL) {
      // printf("var: %s \n", varName);
      // create new variable
      var = addGlobalVariable(csound, &csound->engineState, (CS_TYPE *) type, varName,
                              NULL);
    } else csound->Free(csound, varName);
    if(var != NULL) {
      dest = (OPCODEREF *) &(var->memBlock->value);      
      ref.entries = find_opcode2(csound, name);
      if(dest->entries == NULL ||
         dest->entries->count < ref.entries->count) {
        dest->readonly = 0;
        type->copyValue(csound, type, dest, &ref, NULL);
        dest->readonly = 1;  // mark it as readonly
      } else csound->Free(csound, ref.entries);
    } else csound->Warning(csound, "could not create opcode ref for %s\n", name);
  }
}

/**
 * this function looks at all opcodes in the system
 * and creates read-only variables for all opcodes
 * skipping the ones with non-alphabetic names
 * Variables are initialised to the OENTRIES found.
 */
void add_opcode_defs(CSOUND *csound) {
  CONS_CELL *head, *item;
  OENTRY *ep;
  head = cs_hash_table_values(csound, csound->opcodes);
  while (head != NULL) {
     item = head->value;
     ep = item->value;
     add_opcode_def(csound, ep);
     head = head->next;
  }
}

/**
 * create opcodeRef from an opcode name
 *
 * opcoderef opname:s
 */
int32_t opcode_ref(CSOUND *csound, ASSIGN *p) {
  OPCODEREF *pp = (OPCODEREF *) p->r;
  STRINGDAT *str = (STRINGDAT *) p->a;
  if(find_opcode(csound, str->data))
    pp->entries = find_opcode2(csound, str->data);
  else return csound->InitError(csound, "could not find opcode %s", str->data); 
  return OK;
}

/**
 * print info on OpcodeDef (overloads, types)
 *
 * opcodeinfo opc:OpcodeDef
 */
int32_t opcode_info(CSOUND *csound, OPINFO *p) {
  OENTRY *ep = p->ref->entries->entries[0];
  int n, nep =  p->ref->entries->count;
  csound->Message(csound, "%s: %d overloads\n",
                  get_opcode_short_name(csound, ep->opname),
                  nep);
  for(n = 0; n < nep; n++) {
    ep = p->ref->entries->entries[n];
    csound->Message(csound, "(%d)\t%s\tout-types: %s\tin-types: %s\n",
                    n, ep->opname, ep->outypes, ep->intypes);
  }
  return OK;
}



/**
 * Set a constant (for optional arguments)
 */
MYFLT *set_constant(CSOUND *csound, const char *name, MYFLT value) {
  return (MYFLT *)
    ((char *)find_or_add_constant(csound, csound->engineState.constantsPool,
                          name, value) + CS_VAR_TYPE_OFFSET);
}

/** 
 * Check arg type directly or from types list
 */
CS_TYPE *check_arg_type(void *arg, CS_TYPE **types, int32_t n) {
  if(types == NULL)
  return csoundGetTypeForArg(arg);
  else return types[n];
}


/** 
 * Set up arguments for opcode using OENTRY type lists
 * check every out and in arg and connect it
 * if types == NULL, check arg directly for type
 * expects inargs to follow outargs + 1 skip (for obj arg)
 * returns an error if args do not match
 */
int32_t setup_args(CSOUND *csound, OPCODEOBJ *obj, OPDS *h, MYFLT *args[],
                   CS_TYPE **cstypes, int32_t no, int32_t ni){
  TEXT *t = &(obj->dataspace->optext->t);
  OENTRY *ep = t->oentry;
  char *types;
  CS_TYPE *argtype;
  int32_t n = 0, i = 0, opt = 0;
  size_t len;
  MYFLT **outargs;
  MYFLT **inargs;
  if(obj->udo_flag) {
    // udo args located at the end of udo struct
    UOPCODE *udo = (UOPCODE *) obj->dataspace;
    outargs = udo->ar;
  } else {
    // opcode args located after OPDS struct
    outargs = (MYFLT **) (obj->dataspace + 1);
  }
  obj->outargp = outargs;
  // out args first
  types = ep->outypes;
  while(*types != '\0') {
    // deal with multipe out args first
    if(*types == '*') {
      // expecting only a single char here
      // connect all args
      for(;n < no; n++) outargs[n] = args[n];
      break;
    }
    // other multi output args have one letter
    // per optional output, indicating size
    // of output pointer array
    else if(*types == 'm') {
      // get the size of output array
      len = strlen(types);
      for(; i < len; i++) {
        if(n < no) {
          argtype = check_arg_type(args[n], cstypes, n);
          // if output arg exists, try to connect it
          if(argtype != &CS_VAR_TYPE_A) {
            csound->Message(csound, "Output arg %d, expected type: "
                            "%s, got: %s\n", i+1,CS_VAR_TYPE_A.varTypeName,
                            argtype->varTypeName);
            return NOTOK;
          }
          outargs[i] = args[n++];
        } else // otherwise set it to NULL
          outargs[i] = NULL;
      }
      break; // no further output expected
    }
    // same for all other multi output types
    else if(*types == 'z') {
      len = strlen(types);
      for(; i < len; i++) {
        if(n < no) {
          argtype = check_arg_type(args[n], cstypes, n);
          if(argtype != &CS_VAR_TYPE_K){
            csound->Message(csound, "Output arg %d, expected type: "
                            "%s, got: %s\n", i+1, CS_VAR_TYPE_K.varTypeName,
                            argtype->varTypeName);
            return NOTOK;
          }
          outargs[i] = args[n++];
        } else outargs[i] = NULL;
      }
      break;
    }
    else if(*types == 'I') {
      len = strlen(types);
      for(; i < len; i++) {
        if(n < no) {
          argtype = check_arg_type(args[n], cstypes, n);
          if(argtype != &CS_VAR_TYPE_I){
            csound->Message(csound, "Output arg %d, expected type: "
                            "%s, got: %s\n",i+1, CS_VAR_TYPE_I.varTypeName,
                            argtype->varTypeName);
            return NOTOK;
          }
          outargs[i] = args[n++];
        } else outargs[i] = NULL;        
      }
      break;
    }
    else if(*types == 'X') {
      len = strlen(types);
      for(; i < len; i++) {
        if(n < no) {  
          argtype = check_arg_type(args[n], cstypes, n);
          if(argtype != &CS_VAR_TYPE_A && argtype != &CS_VAR_TYPE_K &&
             argtype != &CS_VAR_TYPE_I ){
            csound->Message(csound,
                            "Output arg %d, expected types: "
                            "%s, %s, or %s, got: %s\n",
                            i+1, CS_VAR_TYPE_A.varTypeName,
                            CS_VAR_TYPE_K.varTypeName,
                            CS_VAR_TYPE_I.varTypeName, argtype->varTypeName);
            return NOTOK;
          }
          outargs[i] = args[n++];
        } else outargs[i] = NULL;         
      }
      break;
    }
    else if(*types == 'N') {
      len = strlen(types);
      for(; i < len; i++) {
        if(n < no) { 
          argtype = check_arg_type(args[n], cstypes, n);
          if(argtype != &CS_VAR_TYPE_A && argtype != &CS_VAR_TYPE_K &&
             argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_S){
            csound->Message(csound,
                            "Output arg %d, expected types: "
                            "%s, %s, %s, or %s, got: %s\n",
                            i+1, CS_VAR_TYPE_A.varTypeName,
                            CS_VAR_TYPE_K.varTypeName,
                            CS_VAR_TYPE_I.varTypeName,
                            CS_VAR_TYPE_S.varTypeName, argtype->varTypeName);
            return NOTOK;
          }
          
          outargs[i] = args[n++];
        } else outargs[i] = NULL;        
      }
      break;
    }   
    else if(*types == 'F') {
      len = strlen(types);
      for(; i < len; i++) {
        if(n < no) {
          argtype = check_arg_type(args[n], cstypes, n);  
          if(argtype != &CS_VAR_TYPE_F) {
            csound->Message(csound, "Output arg %d, expected type: "
                            "%s, got: %s\n", i+1, CS_VAR_TYPE_F.varTypeName,
                            argtype->varTypeName);
            return NOTOK;
          }
          outargs[i] = args[n++];
        } else outargs[i] = NULL;          
      }
      break;
    }
    // now individual arg types
    // type delimiter
    if(*types == ':') {
      types++;
      char typeName[64] = {0};
      size_t end = types - strchr(types, ';');
      memcpy(typeName, types, end);
      argtype = check_arg_type(args[n], cstypes, n);
      if(*(types+end+1) != '[' && strncmp(argtype->varTypeName,
                                          typeName, end) != 0) {
        csound->Message(csound, "Output arg %d, expect type: "
                        "%s, got %s\n", i+1, typeName,
                        argtype->varTypeName);
        return NOTOK;
      }
      if(*(types+end+1) == '[' &&
         argtype != &CS_VAR_TYPE_ARRAY){
        csound->Message(csound, "Output arg %d, expect array, got %s\n",
                        i+1, typeName);
        return NOTOK;
      }
      if(*(types+1) == '[') {
        ARRAYDAT *arg = (ARRAYDAT *) args[n];
        const CS_TYPE *atyp = arg->arrayType;
        if(strncmp(atyp->varTypeName, argtype->varTypeName, 1) != 0) {
          csound->Message(csound, "Output arg %d, mismatching array subtype"
                          " expected %s, got %s\n",
                          i+1, argtype->varTypeName, atyp->varTypeName);
          return NOTOK;
        }
      }
      types += (end+1);
      // skip array type chars
      if(*types == '[') types++;
      if(*types == ']') types++;
    } else {
    // single-char types
    argtype = check_arg_type(args[n], cstypes, n);
    if(*(types+1) != '[' &&
       strncmp(argtype->varTypeName, types, 1) != 0) {
      csound->Message(csound, "Output arg %d, expect type: "
                      "%c, got %s\n", i+1, *types, argtype->varTypeName);
      return NOTOK;
    }
    if(*(types+1) == '[' &&
       argtype != &CS_VAR_TYPE_ARRAY){
      csound->Message(csound, "Output arg %d, expect array, got %c\n",
                      i+1, *types);
      return NOTOK;
    }
    if(*(types+1) == '[') {
      ARRAYDAT *arg = (ARRAYDAT *) args[n];
      const CS_TYPE *atyp = arg->arrayType;
      if(strncmp(atyp->varTypeName, types, 1) != 0) {
        csound->Message(csound, "Output arg %d, mismatching array subtype"
                        " expected %c, got %s\n",
                        i+1, *types, atyp->varTypeName);
        return NOTOK;
      }
    }
    types++;
    // skip array type chars
    if(*types == '[') types++;
    if(*types == ']') types++;
    }
    outargs[i++] = args[n++];
  }
  // n is the outarg count
  if(n != no) {
    // arg number mismatch?
    csound->Message(csound, "Output arg number mismatch, " 
                            "expected %d, got %d\n", no, n);
    return NOTOK;
  }
     
  // set argcount for opcode
  t->outArgCount = no;
  // connect TEXT args
  t->outArgs = h->optext->t.outArgs;
  t->outlist = h->optext->t.outlist;
  // now check inargs
  n++;    // skip opc obj arg
  types = ep->intypes;
  // opcode input args located after after outargs
  obj->inargp = inargs = outargs + i;
  i = 0;  // set inarg count to 0
  while(*types != '\0') {
    // now deal with multiple inargs
    // unlike input, these are single letter
    if(*types == '*') {
      // connect all remaining args
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        inargs[i] = args[n];
      }
      break; // no further inputs expected
    }
    // same for all other multi input types, but with type checks
    else if(*types == 'M') {
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype != &CS_VAR_TYPE_A && argtype != &CS_VAR_TYPE_K &&
           argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_C &&
           argtype != &CS_VAR_TYPE_P){
          csound->Message(csound, "Input arg %d, expected types: "
                          "%s, %s, or %s, got: %s\n",
                          i+1,CS_VAR_TYPE_A.varTypeName,
                          CS_VAR_TYPE_K.varTypeName,
                          CS_VAR_TYPE_I.varTypeName, argtype->varTypeName);
          return NOTOK;
        }
        inargs[i] = args[n];
      }
      break; 
    }
    else if(*types == 'N') {
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype != &CS_VAR_TYPE_A && argtype != &CS_VAR_TYPE_K &&
           argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_S &&
           argtype != &CS_VAR_TYPE_C && argtype != &CS_VAR_TYPE_P){
          csound->Message(csound,
                          "Input arg %d, expected types: "
                          "%s, %s, %s, or %s, got: %s\n",
                          i+1,CS_VAR_TYPE_A.varTypeName,
                          CS_VAR_TYPE_K.varTypeName,
                          CS_VAR_TYPE_I.varTypeName, CS_VAR_TYPE_S.varTypeName,
                          argtype->varTypeName);
          return NOTOK;
        }
        inargs[i] = args[n];
      }
      break;
    }    
    else if(*types == 'm') {
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_C &&
           argtype != &CS_VAR_TYPE_P){
          csound->Message(csound, "Input arg %d, expected type: %s, got: %s\n",
                          i+1,CS_VAR_TYPE_I.varTypeName, argtype->varTypeName);
          return NOTOK;
        }
        inargs[i] = args[n];
      }
      break;
    }
    else if(*types == 'y') {
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype != &CS_VAR_TYPE_A){
          csound->Message(csound, "Input arg %d, expected type: %s, got: %s\n",
                          i+1,CS_VAR_TYPE_A.varTypeName, argtype->varTypeName);
          return NOTOK;
        }
        inargs[i] = args[n];
      }
      break;
    }
    else if(*types == 'z') { 
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype != &CS_VAR_TYPE_K && argtype != &CS_VAR_TYPE_C &&
           argtype != &CS_VAR_TYPE_P && argtype != &CS_VAR_TYPE_I){
          csound->Message(csound, "Input arg %d, expected type: %s, got: %s\n",
                          i+1,CS_VAR_TYPE_K.varTypeName, argtype->varTypeName);
          return NOTOK;
        }
        inargs[i] = args[n];
      }
      break;
    }
    else if(*types == 'W') {
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype != &CS_VAR_TYPE_S){
          csound->Message(csound, "Input arg %d, expected type: %s, got: %s\n",
                          i+1,CS_VAR_TYPE_S.varTypeName, argtype->varTypeName);
          return NOTOK;
        }
        inargs[i] = args[n];
      }
      break;
    }
    else if(*types == 'Z') {
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(n%2 && argtype != &CS_VAR_TYPE_A) {
          csound->Message(csound, "Input arg %d, expected type: %s, got: %s\n",
                          i+1, CS_VAR_TYPE_A.varTypeName,
                          argtype->varTypeName);
          return NOTOK;
        }
        if(n%2 == 0 && argtype != &CS_VAR_TYPE_K && argtype != &CS_VAR_TYPE_I
           && argtype != &CS_VAR_TYPE_C &&
           argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, "Input arg %d, expected type: %s, got: %s\n",
                          i+1, CS_VAR_TYPE_K.varTypeName, argtype->varTypeName);

          return NOTOK;
        }
        inargs[i] = args[n];   
      }
      break;
    }
    else if(*types == 'x') {
      argtype = check_arg_type(args[n], cstypes, n);
      if(argtype != &CS_VAR_TYPE_A && argtype != &CS_VAR_TYPE_K &&
         argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_C &&
         argtype != &CS_VAR_TYPE_P){
        csound->Message(csound, "Input arg %d, expected types: "
                        "%s, %s, or %s, got: %s\n",
                        i+1, CS_VAR_TYPE_A.varTypeName,
                        CS_VAR_TYPE_K.varTypeName,CS_VAR_TYPE_I.varTypeName,
                        argtype->varTypeName);
        return NOTOK;
      }
      inargs[i++] = args[n++]; 
      types++;
    }
    else if(*types == 'T') {
      argtype = check_arg_type(args[n], cstypes, n);
      if(argtype != &CS_VAR_TYPE_S && argtype != &CS_VAR_TYPE_I
         && argtype != &CS_VAR_TYPE_C && argtype != &CS_VAR_TYPE_P){
        csound->Message(csound, "Input arg %d, expected types: "
                        "%s or %s, got: %s\n",
                        i+1, CS_VAR_TYPE_I.varTypeName,
                        CS_VAR_TYPE_S.varTypeName,argtype->varTypeName);
        return NOTOK;
      }
      inargs[i++] = args[n++]; 
      types++;  
    }
    else if(*types == 'U') {
      argtype = check_arg_type(args[n], cstypes, n);
      if(argtype != &CS_VAR_TYPE_S && argtype != &CS_VAR_TYPE_I &&
         argtype != &CS_VAR_TYPE_K && argtype != &CS_VAR_TYPE_C &&
         argtype != &CS_VAR_TYPE_P){
        csound->Message(csound, "Input arg %d, expected types: "
                        "%s, %s, or %s, got: %s\n",
                        i+1, CS_VAR_TYPE_K.varTypeName,
                        CS_VAR_TYPE_I.varTypeName,CS_VAR_TYPE_S.varTypeName,
                        argtype->varTypeName);
        return NOTOK;
      }
      inargs[i++] = args[n++]; 
      types++;
    }
    else if(*types == '.') {
      inargs[i++] = args[n++]; 
      types++;
    }
    // now deal with optional types
    // we have to connect to constants if
    // no arg is provided.
    else if(*types == '?') {
      // increment opt count if no args are passed
      opt += args[n] ? 0 : 1;
      // connect arg if passed, else lookup const
      inargs[i++] = args[n] ? args[n++] :
        set_constant(csound, "0", 0); 
      types++;
    }
    // same for other optional types
    else if(*types == 'o' || *types == 'O') {
      if(args[n] != NULL) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_K &&
           argtype != &CS_VAR_TYPE_C && argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, "Input arg %d, expected types: "
                          "%s or %s, got: %s\n",
                          i+1,CS_VAR_TYPE_I.varTypeName,
                          CS_VAR_TYPE_K.varTypeName,argtype->varTypeName);
          return NOTOK;
        }
        inargs[i++] = args[n++];
      }
      else {
        inargs[i++] = set_constant(csound, "0", 0);       
        opt++;
      }
      types++;
    }
    else if(*types == 'p' || *types == 'P') {
      if(args[n] != NULL) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_K &&
           argtype != &CS_VAR_TYPE_C && argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, "Input arg %d, expected types: "
                          "%s or %s, got: %s\n",
                          i+1,CS_VAR_TYPE_I.varTypeName,
                          CS_VAR_TYPE_K.varTypeName,argtype->varTypeName);
          return NOTOK;
        }
        inargs[i++] = args[n++];
      }
      else {
        inargs[i++] = set_constant(csound, "1", 1);       
        opt++;
      }
      types++;
    }
    else if(*types == 'q') {
      if(args[n] != NULL) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_C &&
           argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, "Input arg %d, expected types: %s got: %s\n",
                          i+1,CS_VAR_TYPE_I.varTypeName, argtype->varTypeName);
          return NOTOK;
        }
        inargs[i++] = args[n++];
      }
      else {
        inargs[i++] = set_constant(csound, "10", 10);       
        opt++;
      }
      types++;
    }
    else if(*types == 'v' || *types == 'V') {
      if(args[n] != NULL) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_K &&
           argtype != &CS_VAR_TYPE_C && argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, "Input arg %d, expected types: "
                          "%s or %s, got: %s\n", i+1,
                          CS_VAR_TYPE_I.varTypeName, CS_VAR_TYPE_K.varTypeName,
                          argtype->varTypeName);
          return NOTOK;
        }
        inargs[i++] = args[n++];
      }
      else {
        inargs[i++] = set_constant(csound, ".5", 0.5);       
        opt++;
      }
      types++;
    }
    else if(*types == 'j' || *types == 'J') {
      if(args[n] != NULL) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_K &&
           argtype != &CS_VAR_TYPE_C && argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, "Input arg %d, expected types: "
                          "%s or %s, got: %s\n",i+1,
                          CS_VAR_TYPE_I.varTypeName, CS_VAR_TYPE_K.varTypeName,
                          argtype->varTypeName);
          return NOTOK;
        }
        inargs[i++] = args[n++];
      }
      else {
        inargs[i++] = set_constant(csound, "-1", -1);      
        opt++;
      }
      types++;
    }
    else if(*types == 'h') {
      if(args[n] != NULL) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_C &&
           argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, "Input arg %d, expected type: %s got: %s\n",
                          i+1,CS_VAR_TYPE_I.varTypeName, argtype->varTypeName);
          return NOTOK;
        }
        inargs[i++] = args[n++];
      }
      else {
        inargs[i++] = set_constant(csound, "127", 127);       
        opt++;
      }
      types++;
    }
    else if(*types == 'k' && *(types+1) !=  '[') {
      argtype = check_arg_type(args[n], cstypes, n);
      if(argtype != &CS_VAR_TYPE_P && argtype != &CS_VAR_TYPE_C
         && argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_K) {
        csound->Message(csound, "Input arg %d, expected type: k got: %s\n",
                        i+1,argtype->varTypeName);
        return NOTOK;
      }
      inargs[i++] = args[n++];
      types++;
    }
    else if(*types == 'i' && *(types+1) !=  '[') {
      argtype = check_arg_type(args[n], cstypes, n);
      if(argtype != &CS_VAR_TYPE_P && argtype != &CS_VAR_TYPE_C
         && argtype != &CS_VAR_TYPE_I) {
        csound->Message(csound, "Input arg %d, expected type: i got: %s\n",
                        i+1,argtype->varTypeName);
        return NOTOK;
      }
      inargs[i++] = args[n++];
      types++;
    } 
    else {             
      // now arg types with no special cases
      // long-name types first
      if(*types == ':') {
        types++;
        char typeName[64] = {0};
        size_t end = types - strchr(types, ';');
        memcpy(typeName, types, end);
        argtype = check_arg_type(args[n], cstypes, n);
        if(*(types+end+1) != '[' && strncmp(argtype->varTypeName,
                                            typeName, end) != 0) {
          csound->Message(csound, "Input arg %d, expect type: "
                          "%s, got %s\n", i+1, typeName,
                          argtype->varTypeName);
          return NOTOK;
        }
        if(*(types+end+1) == '[' &&
           argtype != &CS_VAR_TYPE_ARRAY){
          csound->Message(csound, "Input arg %d, expect array, got %s\n",
                          i+1, typeName);
          return NOTOK;
        }
        if(*(types+end+1) == '[') {
          ARRAYDAT *arg = (ARRAYDAT *) args[n];
          const CS_TYPE *atyp = arg->arrayType;
          if(strncmp(atyp->varTypeName, argtype->varTypeName, 1) != 0) {
            csound->Message(csound, "Input arg %d, mismatching array subtype"
                            " expected %s, got %s\n",
                            i+1, argtype->varTypeName, atyp->varTypeName);
            return NOTOK;
          }
        }
        types += (end+1);
        // skip array type chars
        if(*types == '[') types++;
        if(*types == ']') types++;
      } else {
        // single-char types
        argtype = check_arg_type(args[n], cstypes, n);
        if(*(types+1) != '[' &&
           strncmp(argtype->varTypeName, types, 1) != 0) {
          csound->Message(csound, "Input arg %d, expect type: "
                          "%c, got %s\n", i+1, *types, argtype->varTypeName);
          return NOTOK;
        }
        if(*(types+1) == '[' &&
           argtype != &CS_VAR_TYPE_ARRAY){
          csound->Message(csound, "Input arg %d, expect array, got %c\n",
                          i+1, *types);
          return NOTOK;
        }
        if(*(types+1) == '[') {
          ARRAYDAT *arg = (ARRAYDAT *) args[n];
          const CS_TYPE *atyp = arg->arrayType;
          if(strncmp(atyp->varTypeName, types, 1) != 0) {
            csound->Message(csound, "Input arg %d, mismatching array subtype"
                            " expected %c, got %s\n",
                            i+1, *types, atyp->varTypeName);
            return NOTOK;
          }
        }
        types++;
        // skip array type chars
        if(*types == '[') types++;
        if(*types == ']') types++;
      }
      inargs[i++] = args[n++];  
    }
  }
  // check that input args match. 
  if(ni != i - opt) {
    csound->Message(csound, "Input arg number mismatch, expected %d, got %d\n",
                    ni, i - opt);    
    return NOTOK;
  }

  // connect TEXT args, skipping opcode obj
  t->inArgs = h->optext->t.inArgs + 1;
  t->inlist->count = ni = t->inArgCount = ni;
  t->inlist->arg[0] = (char *) t->inArgs;
  return OK;
}

/** 
 *  check the type of an existing argument, and set it 
 *  this relies on arguments having been previously checked and set
 *  if types do not match or if object was not initialised,
 *  return an error
 */
int32_t check_and_set_arg(CSOUND *csound, OPCODEOBJ *obj, uint32_t ndx,
                          MYFLT *arg) {
  if(obj->inargp != NULL) {
    MYFLT **inargp = obj->inargp;
    int32_t n = obj->dataspace->optext->t.inArgCount;
    if(ndx > n) return NOTOK;
    if(csoundGetTypeForArg(inargp[ndx]) != csoundGetTypeForArg(arg)) {
      // We check if set arg was k and now we send in a constant or ivar
      if(csoundGetTypeForArg(inargp[ndx]) == &CS_VAR_TYPE_K && 
         (csoundGetTypeForArg(arg) == &CS_VAR_TYPE_I ||
          csoundGetTypeForArg(arg) == &CS_VAR_TYPE_C)) inargp[ndx] = arg;
      // also for ivar and constant equivalence
      else if((csoundGetTypeForArg(inargp[ndx]) == &CS_VAR_TYPE_I
              && csoundGetTypeForArg(arg) == &CS_VAR_TYPE_C) ||
              (csoundGetTypeForArg(inargp[ndx]) == &CS_VAR_TYPE_C
               && csoundGetTypeForArg(arg) == &CS_VAR_TYPE_I)) inargp[ndx] = arg;
       // otherwise no cigar as we can't guarantee the opcode will take arg       
      else return NOTOK;        
    } else if(csoundGetTypeForArg(inargp[ndx]) == &CS_VAR_TYPE_ARRAY) {
      ARRAYDAT *src = (ARRAYDAT *) arg;
      ARRAYDAT *dest = (ARRAYDAT *) inargp[ndx];
      // matching array types only (no i-k equivalence test)
      if(src->arrayType == dest->arrayType) inargp[ndx] = arg;
      else return NOTOK;
    }
     else inargp[ndx] = arg;
    return OK;
  }
  return NOTOK;
}

/**
 * connect caller locn to opcode TXT
 */
void set_line_num_and_loc(OPCODEOBJ *obj, OPRUN *p) {
  obj->dataspace->optext->t.linenum = p->h.optext->t.linenum;
  obj->dataspace->optext->t.linenum = p->h.optext->t.locn;
}

/**
 * check for mismatching context
 */
int32_t context_check(CSOUND *csound, OPCODEOBJ *obj, INSDS *insds) {
  if(obj->dataspace->insdshead == insds) return OK;
  INSDS *ip = obj->dataspace->insdshead;
  INSDS *ctx = insds;
  // different SR always fails check
  if(ip->esr != ctx->esr) return NOTOK;
  // ksmps less than ctx fails check
  if(ip->ksmps < ctx->ksmps) return NOTOK;
  // otherwise there is no context incompatibility
  return OK;
}

/**
 * check consistency of arg pointers (for perf-time)
 */
int32_t check_consistency(OPCODEOBJ *obj, MYFLT **args,
                          int32_t no, int32_t ni) {
  int32_t n, i = no + 1;
  MYFLT **oargs = (MYFLT **) (obj->dataspace + 1);
  for(n = 0; n < no; n++)
    if(oargs[n] != args[n]) return 1;
  for(n = 0; n < ni; n++)
    if(oargs[i-1] != args[i]) return 1;
  return 0;
}


/**
 * allocate and initialise opcode dataspace
 */
OPDS *opcode_dataspace_new(CSOUND *csound, OENTRY *entry, OPDS *h) {
    OPDS *dataspace;
    dataspace = (OPDS *) csound->Calloc(csound, entry->dsblksiz);
    if(dataspace != NULL) {
      // fill OPDS with as much information as we have now, rest on init
      dataspace->insdshead = h->insdshead;
      dataspace->optext = csound->Calloc(csound, sizeof(OPTXT));
      if(dataspace->optext != NULL) {
        dataspace->optext->t.oentry = entry;
        dataspace->optext->t.opcod = entry->opname;
        dataspace->optext->t.inlist =
          csound->Calloc(csound, sizeof(ARGLST));
      }
      dataspace->init = entry->init;
      dataspace->perf = entry->perf;
      dataspace->deinit = entry->deinit;
      return dataspace;
    } return NULL;
}

/** 
 *  create single OpcodeObj from OpcodeDef
 *
 *  opc:Opcode create ref:OpcodeDef[,overload:i]
 */
int32_t create_opcode_simple(CSOUND *csound, AOP *p) {
  OPCODEREF *ref = (OPCODEREF *) p->a;
  if(ref->entries != NULL) {
    OPCODEOBJ *obj = (OPCODEOBJ *) p->r;
    int32_t n = (int32_t) (*p->b >= 0 ? *p->b : 0);
    OENTRY *entry =
      ref->entries->entries[n < ref->entries->count ?
                            n : ref->entries->count-1];
    if(obj->dataspace == NULL || obj->size < entry->dsblksiz) {
      if((obj->dataspace = opcode_dataspace_new(csound, entry, &(p->h)))
          == NULL)
        return csound->InitError(csound, "could not allocate opcode object");
     }
      obj->udo_flag = entry->useropinfo == NULL ? 0  : 1;
      obj->size = entry->dsblksiz;
      return OK;
  }
  return csound->InitError(csound, "invalid opcode reference");
}

#include "arrays.h"

/** 
 *  create OpcodeObj array from OpcodeDef
 *
 *  opc:Opcode[] create ref:OpcodeDef, n:i,[,overload:i]
 */
int32_t create_opcode_array(CSOUND *csound, OPARRAY *p) {
  OPCODEREF *ref = p->ref;
  if(ref->entries != NULL) {
    int n  =  (int32_t) (*p->ovl >= 0 ? *p->ovl : 0), i;
    OPCODEOBJ *obj;
    OENTRY *entry =
     ref->entries->entries[n < ref->entries->count ? n : ref->entries->count-1];
    n  = *p->n;
    tabinit(csound, p->r, n, p->h.insdshead);
    obj = (OPCODEOBJ *) p->r->data;
    for(i = 0; i < n; i++) {
      if(obj[i].dataspace == NULL || obj[i].size < entry->dsblksiz) {
       if((obj[i].dataspace = opcode_dataspace_new(csound, entry, &(p->h)))
          == NULL)
        return csound->InitError(csound, "could not allocate opcode object");
      }
      obj[i].udo_flag = entry->useropinfo == NULL ? 0  : 1;
      obj[i].size = entry->dsblksiz; 
      }
    return OK;
  }
  return csound->InitError(csound, "invalid opcode reference");
}

/** 
 *  Delete opcode obj dataspace
 *
 *  delete obj:Opcode
 */
int32_t opcode_delete(CSOUND *csound, AOP *p) {
  OPCODEOBJ *obj = (OPCODEOBJ *) p->r;
  if(obj->dataspace) {
    if(obj->dataspace->deinit != NULL)
      obj->dataspace->deinit(csound, obj->dataspace);
    csound->Free(csound, obj->dataspace->optext->t.inlist);
    csound->Free(csound, obj->dataspace->optext);
    if(obj->udo_flag == 0) {
     csound->Free(csound, obj->dataspace);
     obj->dataspace = NULL;
    }
  }
  return OK;
}

/**
 * print info on opcode (name, types)
 *
 * opcodeinfo opc:Opcode
 */
int32_t opcode_object_info(CSOUND *csound, OPINFO *p) {
  OPCODEOBJ *obj = (OPCODEOBJ *) p->ref;
  if(obj->dataspace != NULL) {
    OENTRY *ep = obj->dataspace->optext->t.oentry;
    csound->Message(csound, "%s %s\tout-types: %s\tin-types: %s \n",
                    ep->opname, obj->udo_flag ? "(UDO)" : "",
                    ep->outypes, ep->intypes);
  }
  return OK;
}


/** 
 * this opcode connects all args to opcode obj and
 * optionally runs init function
 * 
 * outargs  init  opc:Opcode, inargs 
 */
int32_t opcode_object_init(CSOUND *csound, OPRUN *p) {
  OPCODEOBJ *obj = (OPCODEOBJ *) p->args[p->OUTOCOUNT];
  if(obj->dataspace != NULL) {
  if(context_check(csound, obj, p->h.insdshead) != OK)
    return csound->InitError(csound, "incompatible context, "
                             "cannot initialise opcode obj for %s\n",
                             obj->dataspace->optext->t.oentry->opname);
  set_line_num_and_loc(obj, p);
  if(setup_args(csound, obj, &(p->h), p->args, NULL, p->OUTOCOUNT,
                p->INOCOUNT - 1) == OK) {
    if(obj->dataspace->init != NULL) {
       int32_t ret = obj->dataspace->init(csound, obj->dataspace);
       return ret;
    }
    else return OK;
  }
  return csound->InitError(csound, "mismatching arguments\n"
                           "for opcode obj %s\t"
                           "outypes: %s\tintypes: %s",
                           obj->dataspace->optext->t.oentry->opname,
                           obj->dataspace->optext->t.oentry->outypes,
                           obj->dataspace->optext->t.oentry->intypes);
  }
  return csound->InitError(csound, "opcode object not initialised\n");
}

/**
 * this opcode runs a perf pass on an OpcodeObj 
 *
 * outargs perf opc:Opcode, inargs
 */
int32_t opcode_object_perf(CSOUND *csound, OPRUN *p) {
  OPCODEOBJ *obj = (OPCODEOBJ *) p->args[p->OUTOCOUNT];
  if(obj->dataspace != NULL) {
  if(context_check(csound, obj, p->h.insdshead) != OK)
    return csound->PerfError(csound, &(p->h), "incompatible context, "
                             "cannot perform opcode obj for %s\n",
                             obj->dataspace->optext->t.oentry->opname);
  set_line_num_and_loc(obj, p);
  if(check_consistency(obj, p->args, p->OUTOCOUNT, p->INOCOUNT - 1)){
    if(setup_args(csound, obj, &(p->h), p->args, NULL, p->OUTOCOUNT,
                p->INOCOUNT - 1) != OK)
      return csound->PerfError(csound, &(p->h), "mismatching arguments\n"
                           "for opcode obj %s\t"
                           "outypes: %s\tintypes: %s",
                           obj->dataspace->optext->t.oentry->opname,
                           obj->dataspace->optext->t.oentry->outypes,
                               obj->dataspace->optext->t.oentry->intypes);
   }
   if(obj->dataspace->perf != NULL)
       return obj->dataspace->perf(csound, obj->dataspace);
    else return OK; // nothing to do
  }
  return csound->PerfError(csound, &(p->h),
                           "opcode object not initialised\n");
}

/** 
 * perf-time function for run opcode,
 * no checks required
 */
int32_t opcode_run_perf(CSOUND *csound, OPRUN *p) {
  OPCODEOBJ *obj = (OPCODEOBJ *) p->args[p->OUTOCOUNT];
  set_line_num_and_loc(obj, p);                        
  if(obj->dataspace->perf != NULL)
       return obj->dataspace->perf(csound, obj->dataspace);
   else return OK; // nothing to do
}

/** 
 *  Delete opcode obj array dataspace
 *
 *  delete obj:Opcode[]
 */
int32_t opcode_delete_array(CSOUND *csound, AOP *p) {
  ARRAYDAT  *array = (ARRAYDAT *) p->r;
  int32_t   n = array->sizes[0], i;
  OPCODEOBJ *obj= (OPCODEOBJ *) array->data;
  MYFLT *r = p->r;
  for(i = 0; i < n; i++) { 
    p->r = (MYFLT *) &(obj[i]);
    opcode_delete(csound, p);
  }
  p->r = r;
  return OK;
}

/** 
 *  Init function for run on Opcode array
 *  sets up dataspace and optionally runs init function
 *
 */
int32_t opcode_array_init(CSOUND *csound, OPRUN *p) {
  int32_t i, j, n = 0, m;
  MYFLT *args[VARGMAX] = {0};
  CS_TYPE *types[VARGMAX] = {0};
  ARRAYDAT  *array;
  OPCODEOBJ *obj;
  array = (ARRAYDAT *) p->args[p->OUTOCOUNT];
  obj = (OPCODEOBJ *) array->data;
  n = array->sizes[0];
  // check all array args are 1-dim arrays of at least same size as obj[]
  for(i = 0; i < p->INOCOUNT + p->OUTOCOUNT; i++)
    if(csoundGetTypeForArg(p->args[i]) == &CS_VAR_TYPE_ARRAY) {
      array = (ARRAYDAT *) p->args[i];
      if(array->dimensions > 1)
        return csound->InitError(csound, "only 1-dim arrays are allowed\n");
      if(array->dimensions == 0 || n > array->sizes[0]) 
        tabinit(csound, array, n, p->h.insdshead);
  }    
  for(i = 0; i < n; i++) {
    set_line_num_and_loc(&obj[i], p);
    if(context_check(csound, &obj[i], p->h.insdshead) != OK)
      return csound->InitError(csound, "incompatible context, "
                               "cannot initialise opcode obj for %s\n",
                               obj[i].dataspace->optext->t.oentry->opname);
    for(j = 0; j < p->OUTOCOUNT; j++) {
      if((types[j] = csoundGetTypeForArg(p->args[j]))
          == &CS_VAR_TYPE_ARRAY) {
      array = (ARRAYDAT *)  p->args[j]; // each outarg is an array
      char *data = (char *) array->data;
      types[j] = (CS_TYPE *) array->arrayType; // set type
      args[j] = (MYFLT *)(data + i*array->arrayMemberSize); // set pointer
      
      } else // single var
         args[j] = p->args[j];
    }
    for(j = 0; j < p->INOCOUNT - 1; j++) {
      // skip the obj argument
      m = j + p->OUTOCOUNT + 1;
      if((types[m] = csoundGetTypeForArg(p->args[m]))
         == &CS_VAR_TYPE_ARRAY) {
      array = (ARRAYDAT *)  p->args[m]; // each inarg is an array
      char *data = (char *) array->data; // copy loc pointer to args
      types[m] = (CS_TYPE *) array->arrayType;
      args[m] = (MYFLT *)(data + i*array->arrayMemberSize);
      } else // single var
        args[m] = p->args[m];
    }
    // call setup_args with array flag checked, passing assigned args
    if(setup_args(csound, &obj[i], &(p->h), args, types,
                             p->OUTOCOUNT, p->INOCOUNT - 1) == OK) {
      if(obj[i].dataspace->init != NULL) 
        obj[i].dataspace->init(csound, obj[i].dataspace);
    } else return csound->InitError(csound, "mismatching arguments\n"
                                    "for opcode obj %s\t"
                                    "outypes: %s\tintypes: %s",
                                    obj->dataspace->optext->t.oentry->opname,
                                    obj->dataspace->optext->t.oentry->outypes,
                                    obj->dataspace->optext->t.oentry->intypes);
  }
  return OK;
}

/** 
 *  Perf function for run on Opcode array
 *  no checks required
 */
int32_t opcode_array_perf(CSOUND *csound, OPRUN *p) {
  ARRAYDAT  *array = (ARRAYDAT *) p->args[p->OUTOCOUNT];
  int32_t   n = array->sizes[0], i;
  OPCODEOBJ *obj = (OPCODEOBJ *) array->data;
  for(i = 0; i < n; i++) {
  set_line_num_and_loc(&obj[i], p);
  if(obj[i].dataspace->perf != NULL)
    obj[i].dataspace->perf(csound, obj[i].dataspace);
  }
  return OK;
}


int32_t copy_opcode_obj(CSOUND *csound, ASSIGN *p) {
  CS_VAR_TYPE_OPCODEOBJ.copyValue(csound, (CS_TYPE *)
                                  &CS_VAR_TYPE_OPCODEOBJ, p->r,
                                  p->a, p->h.insdshead);
  return OK;
}

int32_t set_opcode_param(CSOUND *csound, AOP *p) {
  OPCODEOBJ *obj = (OPCODEOBJ *) p->r;
  uint32_t ndx = (uint32_t) (*p->a >= 0 ? *p->a : 0);
  MYFLT *arg  = p->b;
  if(context_check(csound, obj, p->h.insdshead) != OK)
    return csound->PerfError(csound, &(p->h), "incompatible context for opcode %s \n",
                             obj->dataspace->optext->t.oentry->opname);
  if(check_and_set_arg(csound, obj, ndx, arg) != 0)
    return csound->PerfError(csound, &(p->h), "could not set arg %u \n", ndx);
  return OK;
}

int32_t get_opcode_output(CSOUND *csound, AOP *p) {
  OPCODEOBJ *obj = (OPCODEOBJ *) p->a;
  uint32_t ndx = (uint32_t) (*p->b >= 0 ? *p->b : 0);
  MYFLT **outarg = obj->outargp;
  if(outarg != NULL) {
  if(context_check(csound, obj, p->h.insdshead) != OK)
    return csound->PerfError(csound, &(p->h), "incompatible context for opcode %s \n",
                             obj->dataspace->optext->t.oentry->opname);
  if(ndx < obj->dataspace->optext->t.outArgCount) {
    CS_TYPE *destype = csoundGetTypeForArg(p->r);
    if(csoundGetTypeForArg(outarg[ndx]) == destype) {
      if(csoundGetTypeForArg(p->r) == &CS_VAR_TYPE_ARRAY) {
        ARRAYDAT *dest = (ARRAYDAT *) p->r;
        ARRAYDAT *src = (ARRAYDAT *) outarg[ndx];
        if(dest->allocated < src->allocated)
         tabinit_like(csound, dest, src);
      }
      destype->copyValue(csound, destype, p->r, outarg[ndx], p->h.insdshead); 
    }
    else return csound->PerfError(csound, &(p->h), "mimatching argument types: "
                                  "need %s, got %s \n",
                                  csoundGetTypeForArg(outarg[ndx])->varTypeName,
                                  csoundGetTypeForArg(p->r)->varTypeName);
  } else return csound->PerfError(csound, &(p->h),
                                   "argument index out of range\n");
   return OK;
  } else return csound->PerfError(csound, &(p->h),
                                   "object not initialised\n");
}

