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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "csoundCore.h"
#include "csound_standard_types.h"
#include "arrays.h"
#include "arrays_internal.h"
#include "opcode.h"
#include "udo.h"
#include <ctype.h>
#include "interlocks.h"
#include "find_opcode.h"

#if defined(_MSC_VER)
#define OPCODE_THREAD_LOCAL __declspec(thread)
#elif defined(BARE_METAL)
#define OPCODE_THREAD_LOCAL
#else
#define OPCODE_THREAD_LOCAL _Thread_local
#endif

#if defined(MSVC)
#define OPCODE_ATOMIC_LOAD_PTR(var) \
  ((void *) InterlockedCompareExchangePointer((void **) &(var), NULL, NULL))
#define OPCODE_ATOMIC_STORE_PTR(var, val) \
  ((void) InterlockedExchangePointer((void **) &(var), (void *) (val)))
#define OPCODE_ATOMIC_CAS_PTR(var, newVal, oldVal) \
  (InterlockedCompareExchangePointer((void **) &(var), (void *) (newVal), \
                                     (void *) (oldVal)) == (void *) (oldVal))
#define OPCODE_ATOMIC_XCHG_PTR(var, val) \
  ((void *) InterlockedExchangePointer((void **) &(var), (void *) (val)))
#define OPCODE_ATOMIC_CAS_U32(var, newVal, oldVal) \
  (InterlockedCompareExchange((volatile long *) &(var), (long) (newVal), \
                              (long) (oldVal)) == (long) (oldVal))
#elif defined(HAVE_ATOMIC_BUILTIN)
#define OPCODE_ATOMIC_LOAD_PTR(var) __atomic_load_n(&(var), __ATOMIC_SEQ_CST)
#define OPCODE_ATOMIC_STORE_PTR(var, val) \
  ((void) __atomic_store_n(&(var), (val), __ATOMIC_SEQ_CST))
#define OPCODE_ATOMIC_CAS_PTR(var, newVal, oldVal) \
  __atomic_compare_exchange_n(&(var), &(oldVal), (newVal), 0, \
                              __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define OPCODE_ATOMIC_XCHG_PTR(var, val) \
  __atomic_exchange_n(&(var), (val), __ATOMIC_SEQ_CST)
#define OPCODE_ATOMIC_CAS_U32(var, newVal, oldVal) \
  __atomic_compare_exchange_n(&(var), &(oldVal), (newVal), 0, \
                              __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#else
#define OPCODE_ATOMIC_LOAD_PTR(var) (var)
#define OPCODE_ATOMIC_STORE_PTR(var, val) ((var) = (val))
#define OPCODE_ATOMIC_CAS_PTR(var, newVal, oldVal) \
  (((var) != (oldVal)) ? 0 : ((var) = (newVal), 1))
#define OPCODE_ATOMIC_XCHG_PTR(var, val) \
  (__extension__ ({ void *opcode_old_ = (void *) (var); (var) = (val); opcode_old_; }))
#define OPCODE_ATOMIC_CAS_U32(var, newVal, oldVal) \
  (((var) != (oldVal)) ? 0 : ((var) = (newVal), 1))
#endif

typedef struct opcode_retired OPCODE_RETIRED;
typedef struct opcode_dataspace_header OPCODE_DATASPACE_HEADER;

struct opcode_retired {
  OPCODE_RETIRED *next;
  CSOUND *csound;
  int32_t kind;
  int32_t runDeinit;
  int32_t freeAfter;
  void *ptr;
};

typedef struct opcode_setp_slot OPCODE_SETP_SLOT;
typedef struct opcode_setp_entry OPCODE_SETP_ENTRY;

struct opcode_setp_slot {
  OPCODE_SETP_SLOT *next;
  CS_VAR_MEM *storage;
  cs_float *value;
};

struct opcode_setp_entry {
  OPCODE_RETIRED retire;
  OPCODE_SETP_ENTRY *next;
  OPCODE_SETP_SLOT *slot;
  uint32_t ndx;
};

enum {
  OPCODE_RETIRE_DATASPACE = 1,
  OPCODE_RETIRE_SETP = 2
};

typedef struct {
  OPDS *dataspace;
  cs_float **outargp;
  cs_float **inargp;
  size_t size;
  uint32_t udo_flag;
} OPCODE_SNAPSHOT;

static OPCODE_RETIRED *opcode_retired_head;
static OPCODE_THREAD_LOCAL int32_t opcode_reclaiming;
static volatile long opcode_acquire_readers;
static OPCODE_ACQUIRE_HOOK opcode_acquire_hook;

void opcode_set_acquire_hook(OPCODE_ACQUIRE_HOOK hook) {
  opcode_acquire_hook = hook;
}

static void opcode_acquire_readers_enter(void) {
  ATOMIC_INCR(opcode_acquire_readers);
}

static void opcode_acquire_readers_leave(void) {
  ATOMIC_DECR(opcode_acquire_readers);
}

void opcode_dataspace_free(CSOUND *csound, OPDS *dataspace);
static void opcode_dataspace_free_now(CSOUND *csound, OPDS *dataspace);
static OPCODE_DATASPACE_HEADER *opcode_dataspace_header(OPDS *dataspace);
static void opcode_setp_storage_free(CSOUND *csound, CS_VAR_MEM *storage);

static void opcode_retired_push(OPCODE_RETIRED *node) {
  OPCODE_RETIRED *head;
  if(node == NULL) return;
  do {
    head = (OPCODE_RETIRED *) OPCODE_ATOMIC_LOAD_PTR(opcode_retired_head);
    node->next = head;
  } while(!OPCODE_ATOMIC_CAS_PTR(opcode_retired_head, node, head));
}

static OPCODE_RETIRED *opcode_retired_take(void) {
  return (OPCODE_RETIRED *) OPCODE_ATOMIC_XCHG_PTR(opcode_retired_head, NULL);
}

static void opcode_retired_repush(OPCODE_RETIRED *list) {
  while(list != NULL) {
    OPCODE_RETIRED *next = list->next;
    opcode_retired_push(list);
    list = next;
  }
}

/* Print opcodes in system */
/* John ffitch -- 26 Jan 97 */
/*  4 april 02 -- ma++ */
/*  restructure to retrieve externally  */
/* And suppressing deprecated Oct 2015 -- JPff */
static int opcode_cmp_func(const void *a, const void *b) {
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

 int32_t csoundNewOpcodeList(CSOUND *csound, opcodeListEntry **lstp) {
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
        ((opcodeListEntry *)lst)[cnt].deprecated =
          ep->deprecated ? ep->deprecated : ((ep->flags & _QQ) ? 1 : 0);
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
  ((opcodeListEntry *)lst)[cnt].deprecated = 0;

  cs_cons_free(csound, head);

  /* sort list */
  qsort(lst, (size_t)cnt, sizeof(opcodeListEntry), opcode_cmp_func);

  /* return the number of opcodes */
  return cnt;
}

 void csoundDisposeOpcodeList(CSOUND *csound, opcodeListEntry *lst) {
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

    //continue;
    if ((level & 1) == 0) { /* Print in 4 columns */
      if (j > 0 && strcmp(lst[j - 1].opname, lst[j].opname) == 0)
        continue;
      if ((level & 2) == 0 && ((lst[j].flags & _QQ) != 0 || lst[j].deprecated)) {
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
      if ((level & 2) == 0 &&  ((lst[j].flags & _QQ) != 0 || lst[j].deprecated)) {
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

/* The type pool stores struct names as :Name; */
static void print_type_name(CSOUND *csound, const char *name) {
  size_t len = strlen(name);
  if (len > 2 && name[0] == ':' && name[len - 1] == ';')
    csound->Message(csound, "%.*s", (int)(len - 2), name + 1);
  else
    csound->Message(csound, "%s", name);
}

/* Print the user-defined types known to this Csound, in orchestra syntax */
void list_structs(CSOUND *csound) {
  const CS_TYPE_ITEM *item;
  int32_t count = 0;

  for (item = csound->typePool->head; item != NULL; item = item->next) {
    const CS_TYPE *type = item->cstype;
    const CONS_CELL *member;
    if (!type->userDefinedType)
      continue;
    csound->Message(csound, "struct ");
    print_type_name(csound, type->varTypeName);
    csound->Message(csound, " ");
    for (member = type->members; member != NULL; member = member->next) {
      const CS_VARIABLE *var = (const CS_VARIABLE *)member->value;
      csound->Message(csound, "%s:", var->varName);
      if (var->varType == &CS_VAR_TYPE_ARRAY) {
        int32_t dimensions = var->dimensions > 0 ? var->dimensions : 1;
        print_type_name(csound, var->subType->varTypeName);
        while (dimensions-- > 0)
          csound->Message(csound, "[]");
      } else {
        print_type_name(csound, var->varType->varTypeName);
      }
      if (member->next != NULL)
        csound->Message(csound, ", ");
    }
    csound->Message(csound, "\n");
    count++;
  }
  csound->Message(csound, "\n");
  csound->Message(csound, Str("%d structs\n\n"), count);
}

/**
 * Opcodes for OpcodeDef and Opcode types
 *
 **/
struct oentries *find_opcode2(CSOUND *, char *);
void *find_or_add_constant(CSOUND *csound, CS_HASH_TABLE *constantsPool,
                           const char *name, cs_float value);

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
                        "passign",
                        ""};
  int i = 0;
  for(i = 0; *exs[i] != '\0'; i++)
    if(strcmp(exs[i], name) == 0) return NOTOK;
  return OK;
}

/** check opcodes without any inputs or *only* optional inputs
 */
static int32_t check_oentry(OENTRY *ep) {
  if(ep->intypes &&
     *(ep->intypes) != 'o' &&
     *(ep->intypes) != 'p' &&
     *(ep->intypes) != 'q' &&
     *(ep->intypes) != 'h' &&
     *(ep->intypes) != 'j' &&
     *(ep->intypes) != 'v' &&
     *(ep->intypes) != 'O' &&
     *(ep->intypes) != 'J' &&
     *(ep->intypes) != 'V' &&
     *(ep->intypes) != 'P' &&
     *(ep->intypes) != '\0'
     )
    return 1;
  else return 0;
}

CS_VARIABLE *add_global_variable(CSOUND *csound, ENGINE_STATE *engineState,
                                 const CS_TYPE *type, char *name,
                                 const void *typeArg);

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
      var = add_global_variable(csound, &csound->engineState, type, varName,
                              NULL);
    } else csound->Free(csound, varName);
    /* A global of the orchestra's own with this name shadows the opcode: it
       is not an opcode reference, and is not to be written as one. */
    if(var != NULL && var->varType == type) {
      dest = (OPCODEREF *) &(var->memBlock->value);
      ref.entries = find_opcode2(csound, name);
      if(dest->entries == NULL ||
         dest->entries->count < ref.entries->count) {
#ifdef CSOUND_WASI_REALTIME
        /* The shallow copy outlives this parse, even if a later statement
           fails verification. Keep temporary lookups scoped, but transfer
           this published array to the engine's reset-time allocation list. */
        csoundWasiReleaseAllocation(csound, ref.entries);
#endif
        dest->readonly = 0;
        type->copyValue(csound, type, dest, &ref, NULL);
        dest->readonly = 1;  // mark it as readonly
      } else csound->Free(csound, ref.entries);
    } else if (var == NULL)
      csound->Warning(csound, Str("could not create opcode ref for %s\n"), name);
  }
  if (name != ep->opname)
    csound->Free(csound, name);
}

/**
 * this function looks at all opcodes in the system
 * and creates read-only variables for all opcodes
 * skipping the ones with non-alphabetic names
 * Variables are initialised to the OENTRIES found.
 */
void add_opcode_defs(CSOUND *csound) {
  CONS_CELL *head, *item, *current;
  OENTRY *ep;
  head = current = cs_hash_table_values(csound, csound->opcodes);
  while (current != NULL) {
     item = current->value;
     ep = item->value;
     add_opcode_def(csound, ep);
     current = current->next;
  }
  cs_cons_free(csound, head);
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
  else return csound->InitError(csound, Str("could not find opcode %s\n"), str->data);
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
  csound->Message(csound, Str("%s: %d overloads\n"),
                  get_opcode_short_name(csound, ep->opname),
                  nep);
  for(n = 0; n < nep; n++) {
    ep = p->ref->entries->entries[n];
    csound->Message(csound, Str("(%d)\t%s\tout-types: %s\tin-types: %s\n"),
                    n, ep->opname, ep->outypes, ep->intypes);
  }
  return OK;
}



/**
 * Set a constant (for optional arguments)
 */
static cs_float *set_constant(CSOUND *csound, const char *name, cs_float value) {
  return (cs_float *)
    ((char *)find_or_add_constant(csound, csound->engineState.constantsPool,
                          name, value) + CS_VAR_TYPE_OFFSET);
}

/**
 * Check arg type directly or from types list
 * an argument the call did not give is NULL, and has no type
 */
static const CS_TYPE *check_arg_type(void *arg, const CS_TYPE **types, int32_t n) {
  if(types == NULL)
  return arg != NULL ? csoundGetTypeForArg(arg) : NULL;
  else return types[n];
}

static void opcode_dataspace_publish_binding(OPCODEOBJ *obj, OPDS *dataspace,
                                             cs_float **outargs,
                                             cs_float **inargs) {
  if(OPCODE_ATOMIC_LOAD_PTR(obj->dataspace) == dataspace) {
    OPCODE_ATOMIC_STORE_PTR(obj->outargp, outargs);
    OPCODE_ATOMIC_STORE_PTR(obj->inargp, inargs);
  }
}

/**
 * whether type is the one a long name in a type list names: name is the
 * text between the ':' and the ';', len its length. A struct type's own
 * name is the long name ":Name;", a builtin type's the bare name
 */
static int32_t is_named_type(const CS_TYPE *type, const char *name, size_t len) {
  const char *t = type->varTypeName;
  size_t tlen = strlen(t);
  if(tlen > 2 && t[0] == ':' && t[tlen-1] == ';') {
    t++;
    tlen -= 2;
  }
  return tlen == len && strncmp(t, name, len) == 0;
}


/**
 * Set up arguments for opcode using OENTRY type lists
 * check every out and in arg and connect it
 * if types == NULL, check arg directly for type
 * expects inargs to follow outargs + 1 skip (for obj arg)
 * returns an error if args do not match
 */
static int32_t setup_args(CSOUND *csound, OPCODEOBJ *obj, OPDS *h, cs_float *args[],
                    const CS_TYPE **cstypes, int32_t no, int32_t ni,
                    OPCODE_SNAPSHOT *snap){
  OPDS *dataspace;
  uint32_t udo_flag;
  TEXT *t;
  OENTRY *ep;
  char *opname;
  char *types;
  const CS_TYPE *argtype;
  int32_t n = 0, opt = 0;
  int32_t  len, i = 0;
  cs_float **outargs;
  cs_float **inargs;
  dataspace = snap != NULL ? snap->dataspace : obj->dataspace;
  udo_flag = snap != NULL ? snap->udo_flag : obj->udo_flag;
  t = &(dataspace->optext->t);
  ep = t->oentry;
  opname = ep->opname;
  if(udo_flag) {
    // udo args located at the end of udo struct
    UOPCODE *udo = (UOPCODE *) dataspace;
    outargs = udo->ar;
  } else {
    // opcode args located after OPDS struct
    outargs = (cs_float **) (dataspace + 1);
  }
  if(snap != NULL) snap->outargp = outargs;
  else obj->outargp = outargs;
  // out args first
  types = ep->outypes;
  while(*types != '\0') {
    // deal with multipe out args first
    if(*types == '*') {
      // expecting only a single char here
      // connect all args: with none, the opcode keeps one empty slot (as
      // oprun_args reads it), and the inputs follow either way
      for(;n < no; n++) outargs[i++] = args[n];
      if(i == 0) outargs[i++] = NULL;
      break;
    }
    // other multi output args have one letter
    // per optional output, indicating size
    // of output pointer array
    else if(*types == 'm') {
      // get the size of output array: one slot per letter left, after
      // the i slots of the outputs before them
      len = i + (int32_t) strlen(types);
      for(; i < len; i++) {
        if(n < no) {
          argtype = check_arg_type(args[n], cstypes, n);
          // if output arg exists, try to connect it
          if(argtype != &CS_VAR_TYPE_A) {
            csound->Message(csound, Str("%s outarg %d, expected type: "
                            "%s, got: %s\n"), opname, i+1,CS_VAR_TYPE_A.varTypeName,
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
      len = i + (int32_t) strlen(types);
      for(; i < len; i++) {
        if(n < no) {
          argtype = check_arg_type(args[n], cstypes, n);
          if(argtype != &CS_VAR_TYPE_K){
            csound->Message(csound, Str("%s outarg %d, expected type: "
                            "%s, got: %s\n"), opname, i+1, CS_VAR_TYPE_K.varTypeName,
                            argtype->varTypeName);
            return NOTOK;
          }
          outargs[i] = args[n++];
        } else outargs[i] = NULL;
      }
      break;
    }
    else if(*types == 'I') {
      // 'I' = variable count of i-rate scalar outputs; semantics allows i or S
      // to support passign and similar opcodes that can write numbers or strings
      len = i + (int32_t) strlen(types);
      for(; i < len; i++) {
        if(n < no) {
          argtype = check_arg_type(args[n], cstypes, n);
          if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_S &&
             argtype != &CS_VAR_TYPE_P){
            csound->Message(csound, Str("%s outarg %d, expected type: %s or %s, got: %s\n"),
                            opname, i+1, CS_VAR_TYPE_I.varTypeName, CS_VAR_TYPE_S.varTypeName,
                            argtype->varTypeName);
            return NOTOK;
          }
          outargs[i] = args[n++];
        } else {
          outargs[i] = NULL;
        }
      }
      break;
    }
    else if(*types == 'X') {
      len = i + (int32_t) strlen(types);
      for(; i < len; i++) {
        if(n < no) {
          argtype = check_arg_type(args[n], cstypes, n);
          if(argtype != &CS_VAR_TYPE_A && argtype != &CS_VAR_TYPE_K &&
             argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_P){
            csound->Message(csound,
                            Str("%s outarg %d, expected types: "
                            "%s, %s, or %s, got: %s\n"),
                            opname, i+1, CS_VAR_TYPE_A.varTypeName,
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
      len = i + (int32_t) strlen(types);
      for(; i < len; i++) {
        if(n < no) {
          argtype = check_arg_type(args[n], cstypes, n);
          if(argtype != &CS_VAR_TYPE_A && argtype != &CS_VAR_TYPE_K &&
             argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_S &&
             argtype != &CS_VAR_TYPE_P){
            csound->Message(csound,
                            Str("%s outarg %d, expected types: "
                            "%s, %s, %s, or %s, got: %s\n"),
                            opname, i+1, CS_VAR_TYPE_A.varTypeName,
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
    else if(*types == 'v' || *types == 'V') {
      // b or B outputs, as init has for booleans
      const CS_TYPE *want = *types == 'v' ? &CS_VAR_TYPE_b : &CS_VAR_TYPE_B;
      len = i + (int32_t) strlen(types);
      for(; i < len; i++) {
        if(n < no) {
          argtype = check_arg_type(args[n], cstypes, n);
          if(argtype != want) {
            csound->Message(csound, Str("%s outarg %d, expected type: "
                            "%s, got: %s\n"), opname, i+1, want->varTypeName,
                            argtype->varTypeName);
            return NOTOK;
          }
          outargs[i] = args[n++];
        } else outargs[i] = NULL;
      }
      break;
    }
    else if(*types == 'F') {
      len = i + (int32_t) strlen(types);
      for(; i < len; i++) {
        if(n < no) {
          argtype = check_arg_type(args[n], cstypes, n);
          if(argtype != &CS_VAR_TYPE_F) {
            csound->Message(csound, Str("%s outarg %d, expected type: "
                            "%s, got: %s\n"), opname, i+1, CS_VAR_TYPE_F.varTypeName,
                            argtype->varTypeName);
            return NOTOK;
          }
          outargs[i] = args[n++];
        } else outargs[i] = NULL;
      }
      break;
    }
    // now individual arg types: past the call's outputs come the object
    // and the inputs, which an output of their type would take
    if(n >= no) {
      csound->Message(csound, Str("missing %s outarg %d\n"), opname, i+1);
      return NOTOK;
    }
    // type delimiter
    if(*types == ':') {
      types++;
      size_t end = strchr(types, ';') - types;
      argtype = check_arg_type(args[n], cstypes, n);
      if(*(types+end+1) != '[' && !is_named_type(argtype, types, end)) {
        csound->Message(csound, Str("%s outarg %d, expect type: "
                        "%.*s, got %s\n"), opname, i+1, (int) end, types,
                        argtype->varTypeName);
        return NOTOK;
      }
      if(*(types+end+1) == '[' &&
         argtype != &CS_VAR_TYPE_ARRAY){
        csound->Message(csound, Str("%s outarg %d, expect array, got %s\n"),
                        opname, i+1, argtype->varTypeName);
        return NOTOK;
      }
      if(*(types+end+1) == '[') {
        ARRAYDAT *arg = (ARRAYDAT *) args[n];
        const CS_TYPE *atyp = arg->arrayType;
        if(!is_named_type(atyp, types, end)) {
          csound->Message(csound, Str("%s outarg %d, mismatching array subtype"
                          " expected %.*s, got %s\n"),
                          opname, i+1, (int) end, types, atyp->varTypeName);
          return NOTOK;
        }
      }
      types += (end+1);
      // skip array type chars
      while(types[0] == '[' && types[1] == ']') types += 2;
    } else {
    // single-char types: an i output takes a p-field too
    argtype = check_arg_type(args[n], cstypes, n);
    if(*(types+1) != '[' &&
       strncmp(argtype->varTypeName, types, 1) != 0 &&
       !(*types == 'i' && argtype == &CS_VAR_TYPE_P)) {
      csound->Message(csound, Str("%s outarg %d, expect type: "
                      "%c, got %s\n"), opname, i+1, *types, argtype->varTypeName);
      return NOTOK;
    }
    if(*(types+1) == '[' &&
       argtype != &CS_VAR_TYPE_ARRAY){
      csound->Message(csound, Str("%s outarg %d, expect array, got %s\n"),
                      opname, i+1, argtype->varTypeName);
      return NOTOK;
    }
    if(*(types+1) == '[') {
      ARRAYDAT *arg = (ARRAYDAT *) args[n];
      const CS_TYPE *atyp = arg->arrayType;
      if(strncmp(atyp->varTypeName, types, 1) != 0) {
        csound->Message(csound, Str("%s outarg %d, mismatching array subtype"
                        " expected %c, got %s\n"),
                        opname, i+1, *types, atyp->varTypeName);
        return NOTOK;
      }
    }
    types++;
    // skip array type chars
    while(types[0] == '[' && types[1] == ']') types += 2;
    }
    outargs[i++] = args[n++];
  }
  // n is the outarg count
  if(n != no) {
    // arg number mismatch?
    csound->Message(csound, Str("%s outarg number mismatch, "
                    "expected %d, got %d\n"), opname, no, n);
    return NOTOK;
  }

  // now check inargs
  n++;    // skip opc obj arg
  types = ep->intypes;
  // opcode input args located after after outargs
  inargs = outargs + i;
  if(snap != NULL) snap->inargp = inargs;
  else obj->inargp = inargs;
  i = 0;  // set inarg count to 0
  while(*types != '\0') {
    // now deal with multiple inargs
    // unlike input, these are single letter
    if(*types == '*') {
      // connect all remaining args
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        inargs[i] = args[n];
      }
      break; // no further inputs expected
    }
    // same for all other multi input types, but with type checks
    else if(*types == 'M') {
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
         if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        if(argtype != &CS_VAR_TYPE_A && argtype != &CS_VAR_TYPE_K &&
           argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_C &&
           argtype != &CS_VAR_TYPE_P){
          csound->Message(csound, Str("%s inarg %d, expected types: "
                          "%s, %s, or %s, got: %s\n"),
                          opname, i+1,CS_VAR_TYPE_A.varTypeName,
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
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        if(argtype != &CS_VAR_TYPE_A && argtype != &CS_VAR_TYPE_K &&
           argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_S &&
           argtype != &CS_VAR_TYPE_C && argtype != &CS_VAR_TYPE_P){
          csound->Message(csound,
                          Str("%s inarg %d, expected types: "
                          "%s, %s, %s, or %s, got: %s\n"),
                          opname, i+1,CS_VAR_TYPE_A.varTypeName,
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
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_C &&
           argtype != &CS_VAR_TYPE_P){
          csound->Message(csound, Str("%s inarg %d, expected type: %s, got: %s\n"),
                          opname, i+1,CS_VAR_TYPE_I.varTypeName, argtype->varTypeName);
          return NOTOK;
        }
        inargs[i] = args[n];
      }
      break;
    }
    else if(*types == 'y') {
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        if(argtype != &CS_VAR_TYPE_A){
          csound->Message(csound, Str("%s inarg %d, expected type: %s, got: %s\n"),
                          opname, i+1,CS_VAR_TYPE_A.varTypeName, argtype->varTypeName);
          return NOTOK;
        }
        inargs[i] = args[n];
      }
      break;
    }
    else if(*types == 'z') {
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        if(argtype != &CS_VAR_TYPE_K && argtype != &CS_VAR_TYPE_C &&
           argtype != &CS_VAR_TYPE_P && argtype != &CS_VAR_TYPE_I){
          csound->Message(csound, Str("%s inarg %d, expected type: %s, got: %s\n"),
                          opname, i+1,CS_VAR_TYPE_K.varTypeName, argtype->varTypeName);
          return NOTOK;
        }
        inargs[i] = args[n];
      }
      break;
    }
    else if(*types == 'W') {
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        if(argtype != &CS_VAR_TYPE_S){
          csound->Message(csound, Str("%s inarg %d, expected type: %s, got: %s\n"),
                          opname, i+1,CS_VAR_TYPE_S.varTypeName, argtype->varTypeName);
          return NOTOK;
        }
        inargs[i] = args[n];
      }
      break;
    }
    else if(*types == 'Z') {
      for(; i < ni; n++, i++) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        // kaka...: the inputs alternate from the first, whatever the
        // number of outputs before them
        if(i%2 && argtype != &CS_VAR_TYPE_A) {
          csound->Message(csound, Str("%s inarg %d, expected type: %s, got: %s\n"),
                          opname, i+1, CS_VAR_TYPE_A.varTypeName,
                          argtype->varTypeName);
          return NOTOK;
        }
        if(i%2 == 0 && argtype != &CS_VAR_TYPE_K && argtype != &CS_VAR_TYPE_I
           && argtype != &CS_VAR_TYPE_C &&
           argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, Str("%s inarg %d, expected type: %s, got: %s\n"),
                          opname, i+1, CS_VAR_TYPE_K.varTypeName, argtype->varTypeName);

          return NOTOK;
        }
        inargs[i] = args[n];
      }
      break;
    }
    else if(*types == 'x') {
      argtype = check_arg_type(args[n], cstypes, n);
      if(argtype == NULL) {
        csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
        return NOTOK;
      }
      if(argtype != &CS_VAR_TYPE_A && argtype != &CS_VAR_TYPE_K &&
         argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_C &&
         argtype != &CS_VAR_TYPE_P){
        csound->Message(csound, Str("%s inarg %d, expected types: "
                        "%s, %s, or %s, got: %s\n"),
                        opname, i+1, CS_VAR_TYPE_A.varTypeName,
                        CS_VAR_TYPE_K.varTypeName,CS_VAR_TYPE_I.varTypeName,
                        argtype->varTypeName);
        return NOTOK;
      }
      inargs[i++] = args[n++];
      types++;
    }
    else if(*types == 'T') {
      argtype = check_arg_type(args[n], cstypes, n);
      if(argtype == NULL) {
        csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
        return NOTOK;
      }
      if(argtype != &CS_VAR_TYPE_S && argtype != &CS_VAR_TYPE_I
         && argtype != &CS_VAR_TYPE_C && argtype != &CS_VAR_TYPE_P){
        csound->Message(csound, Str("%s inarg %d, expected types: "
                        "%s or %s, got: %s\n"),
                        opname, i+1, CS_VAR_TYPE_I.varTypeName,
                        CS_VAR_TYPE_S.varTypeName,argtype->varTypeName);
        return NOTOK;
      }
      inargs[i++] = args[n++];
      types++;
    }
    else if(*types == 'U') {
      argtype = check_arg_type(args[n], cstypes, n);
      if(argtype == NULL) {
        csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
        return NOTOK;
      }
      if(argtype != &CS_VAR_TYPE_S && argtype != &CS_VAR_TYPE_I &&
         argtype != &CS_VAR_TYPE_K && argtype != &CS_VAR_TYPE_C &&
         argtype != &CS_VAR_TYPE_P){
        csound->Message(csound, Str("%s inarg %d, expected types: "
                        "%s, %s, or %s, got: %s\n"),
                        opname, i+1, CS_VAR_TYPE_K.varTypeName,
                        CS_VAR_TYPE_I.varTypeName,CS_VAR_TYPE_S.varTypeName,
                        argtype->varTypeName);
        return NOTOK;
      }
      inargs[i++] = args[n++];
      types++;
    }
    // any type, but an array of any type is checked as an array below
    else if(*types == '.' && *(types+1) != '[') {
      inargs[i++] = args[n++];
      types++;
    }
    // now deal with optional types
    // we have to connect to constants if
    // no arg is provided.
    else if(*types == '?') {
      // increment opt count if no args are passed
      opt += (n < ni && args[n]) ? 0 : 1;
      // connect arg if passed, else lookup const
      inargs[i++] = (n < ni && args[n]) ? args[n++] :
        set_constant(csound, "0", 0);
      types++;
    }
    // same for other optional types
    else if(*types == 'o' || *types == 'O') {
      if(n < no + 1 + ni && args[n] != NULL) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
         csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
         return NOTOK;
        }
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_K &&
           argtype != &CS_VAR_TYPE_C && argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, Str("%s inarg %d, expected types: "
                          "%s or %s, got: %s\n"),
                          opname, i+1,CS_VAR_TYPE_I.varTypeName,
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
      if(n < no + 1 + ni && args[n] != NULL) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_K &&
           argtype != &CS_VAR_TYPE_C && argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, Str("%s inarg %d, expected types: "
                          "%s or %s, got: %s\n"),
                          opname, i+1,CS_VAR_TYPE_I.varTypeName,
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
      if(n < no + 1 + ni && args[n] != NULL) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_C &&
           argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, Str("%s inarg %d, expected types: %s got: %s\n"),
                          opname, i+1,CS_VAR_TYPE_I.varTypeName, argtype->varTypeName);
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
      if(n < no + 1 + ni && args[n] != NULL) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_K &&
           argtype != &CS_VAR_TYPE_C && argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, Str("%s inarg %d, expected types: "
                          "%s or %s, got: %s\n"), opname, i+1,
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
      if(n < no + 1 + ni && args[n] != NULL) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_K &&
           argtype != &CS_VAR_TYPE_C && argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, Str("%s inarg %d, expected types: "
                          "%s or %s, got: %s\n"),opname, i+1,
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
      if(n < no + 1 + ni && args[n] != NULL) {
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        if(argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_C &&
           argtype != &CS_VAR_TYPE_P) {
          csound->Message(csound, Str("%s inarg %d, expected type: %s got: %s\n"),
                          opname, i+1,CS_VAR_TYPE_I.varTypeName, argtype->varTypeName);
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
      if(argtype == NULL) {
        csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
        return NOTOK;
      }
      if(argtype != &CS_VAR_TYPE_P && argtype != &CS_VAR_TYPE_C
         && argtype != &CS_VAR_TYPE_I && argtype != &CS_VAR_TYPE_K) {
        csound->Message(csound, Str("%s inarg %d, expected type: k got: %s\n"),
                        opname, i+1,argtype->varTypeName);
        return NOTOK;
      }
      inargs[i++] = args[n++];
      types++;
    }
    else if(*types == 'i' && *(types+1) !=  '[') {
      argtype = check_arg_type(args[n], cstypes, n);
      if(argtype == NULL) {
        csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
        return NOTOK;
      }
      if(argtype != &CS_VAR_TYPE_P && argtype != &CS_VAR_TYPE_C
         && argtype != &CS_VAR_TYPE_I) {
        csound->Message(csound, Str("%s inarg %d, expected type: i got: %s\n"),
                        opname, i+1,argtype->varTypeName);
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
        size_t end = strchr(types, ';') - types;
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
          csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
          return NOTOK;
        }
        if(*(types+end+1) != '[' && !is_named_type(argtype, types, end)) {
          csound->Message(csound, Str("%s inarg %d, expect type: "
                          "%.*s, got %s\n"), opname, i+1, (int) end, types,
                          argtype->varTypeName);
          return NOTOK;
        }
        if(*(types+end+1) == '[' &&
           argtype != &CS_VAR_TYPE_ARRAY){
          csound->Message(csound, Str("%s inarg %d, expect array, got %s\n"),
                          opname, i+1, argtype->varTypeName);
          return NOTOK;
        }
        if(*(types+end+1) == '[') {
          ARRAYDAT *arg = (ARRAYDAT *) args[n];
          if(arg == NULL) {
           csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
           return NOTOK;
          }
          const CS_TYPE *atyp = arg->arrayType;
          if(!is_named_type(atyp, types, end)) {
            csound->Message(csound, Str("%s inarg %d, mismatching array subtype"
                            " expected %.*s, got %s\n"),
                            opname, i+1, (int) end, types, atyp->varTypeName);
            return NOTOK;
          }
        }
        types += (end+1);
        // skip array type chars
        while(types[0] == '[' && types[1] == ']') types += 2;
      } else {
        // single-char types
        argtype = check_arg_type(args[n], cstypes, n);
        if(argtype == NULL) {
         csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
         return NOTOK;
        }
        if(*(types+1) != '[' &&
           strncmp(argtype->varTypeName, types, 1) != 0) {
          csound->Message(csound, Str("%s inarg %d, expect type: "
                          "%c, got %s\n"), opname, i+1, *types, argtype->varTypeName);
          return NOTOK;
        }
        if(*(types+1) == '[' &&
           argtype != &CS_VAR_TYPE_ARRAY){
          csound->Message(csound, Str("%s inarg %d, expect array, got %s\n"),
                          opname, i+1, argtype->varTypeName);
          return NOTOK;
        }
        if(*(types+1) == '[') {
          ARRAYDAT *arg = (ARRAYDAT *) args[n];
          if(arg == NULL) {
           csound->Message(csound, Str("missing %s inarg %d"), opname, n - no);
           return NOTOK;
          }
          const CS_TYPE *atyp = arg->arrayType;
          /* Allow wildcard array subtype: ".[]" matches any array element type */
          if(*types != '.') {
            if(strncmp(atyp->varTypeName, types, 1) != 0) {
              csound->Message(csound, Str("%s inarg %d, mismatching array subtype"
                              " expected %c, got %s\n"),
                              opname, i+1, *types, atyp->varTypeName);
              return NOTOK;
            }
          }
        }
        types++;
        // skip array type chars
        while(types[0] == '[' && types[1] == ']') types += 2;
      }
      inargs[i++] = args[n++];
    }
  }
  // check that input args match.
  // ni = number of provided arguments
  // i = total number of processed arguments (required + optional)
  // opt = number of optional arguments that used defaults
  // The number of provided arguments should equal the number of non-default arguments

  if(ni != i - opt) {
    csound->Message(csound, Str("%s inarg number mismatch, expected %d, got %d\n"),
                    opname, i - opt, ni);
    return NOTOK;
  }

  // set argcount for opcode, and connect TEXT args: outlist marks a
  // dataspace a call has bound
  t->outArgCount = no;
  t->outArgs = h->optext->t.outArgs;
  t->outlist = h->optext->t.outlist;
  // connect TEXT args, skipping opcode obj
  t->inArgs = h->optext->t.inArgs->next;
  t->inlist->count = ni = t->inArgCount = ni;
  t->inlist->arg[0] = (char *) t->inArgs;
  if(snap != NULL)
    opcode_dataspace_publish_binding(obj, dataspace, outargs, inargs);
  return OK;
}

/**
 *  the number of input slots setup_args fills for a call that gave
 *  so many inputs: one for each type in intypes, given or defaulted,
 *  and as many as were given for a list of variable length
 */
static uint32_t input_slots(const char *types, uint32_t given) {
  uint32_t slots = 0;
  while(*types != '\0') {
    if(strchr("*MNmyzWZ", *types) != NULL)
      return slots > given ? slots : given;
    if(*types == ':') types = strchr(types, ';');
    types++;
    while(types[0] == '[' && types[1] == ']') types += 2;
    slots++;
  }
  return slots;
}

/**
 *  check the type of an existing argument, and set it
 *  this relies on arguments having been previously checked and set
 *  if types do not match or if object was not initialised,
 *  return an error
 */
static int32_t check_and_set_arg(CSOUND *csound, OPCODE_SNAPSHOT *snap,
                                 uint32_t ndx, cs_float *arg) {
  if(snap->inargp != NULL) {
    cs_float **inargp = snap->inargp;
    const TEXT *t = &(snap->dataspace->optext->t);
    // the slots setup_args filled; one a failed call left empty is none
    if(ndx >= input_slots(t->oentry->intypes, t->inArgCount) ||
       inargp[ndx] == NULL) return NOTOK;
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
static void set_line_num_and_loc(OPDS *dataspace, OPRUN *p) {
  dataspace->optext->t.linenum = p->h.optext->t.linenum;
  dataspace->optext->t.locn = p->h.optext->t.locn;
}

/**
 * check for mismatching context
 */
int32_t context_check(CSOUND *csound, OPDS *dataspace, INSDS *insds) {
  if(dataspace->insdshead == insds) return OK;
  INSDS *ip = dataspace->insdshead;
  INSDS *ctx = insds;
  if(ctx) {
  // different SR always fails check
  if(ip->esr != ctx->esr) return NOTOK;
  // ksmps less than ctx fails check
  if(ip->ksmps < ctx->ksmps) return NOTOK;
  // otherwise there is no context incompatibility
  }
  else {
    csound->Warning(csound, Str("no valid context for opcode object\n"));
    return NOTOK;
  }
  return OK;
}

static int32_t opcode_dataspace_was_initialized(OPDS *dataspace);
static cs_float *opcode_dataspace_setp_arg(OPDS *dataspace, uint32_t ndx);

static int32_t opcode_dataspace_ended(OPDS *dataspace, uint32_t udo_flag) {
  UOPCODE *udo;
  if(!udo_flag || dataspace == NULL) return 0;
  udo = (UOPCODE *) dataspace;
  return udo->ip == NULL && opcode_dataspace_was_initialized(dataspace);
}

int32_t opcode_udo_ended(OPCODEOBJ *obj) {
  OPDS *dataspace;
  int32_t ended;
  if(obj == NULL) return 0;
  opcode_acquire_readers_enter();
  dataspace = (OPDS *) OPCODE_ATOMIC_LOAD_PTR(obj->dataspace);
  ended = opcode_dataspace_ended(dataspace, obj->udo_flag);
  opcode_acquire_readers_leave();
  return ended;
}

#define OPCODE_RUN_SCOPE_MAX 64

static OPCODE_THREAD_LOCAL OPDS *opcode_run_stack[OPCODE_RUN_SCOPE_MAX];
static OPCODE_THREAD_LOCAL int32_t opcode_run_depth;

static int32_t opcode_dataspace_is_running(OPDS *dataspace) {
  int32_t n;
  for(n = 0; n < opcode_run_depth; n++)
    if(opcode_run_stack[n] == dataspace) return 1;
  return 0;
}

/**
 * check consistency of arg pointers (for perf-time)
 * returns OK if the args are those bound by setup_args, NOTOK otherwise
 */
static int32_t check_consistency(OPCODE_SNAPSHOT *snap, cs_float **args,
                           int32_t no, int32_t ni) {
  int32_t n;
  // setup_args stores these, in udo->ar for UDOs
  cs_float **oargs = snap->outargp, **iargs = snap->inargp;
  if(oargs == NULL || iargs == NULL) return NOTOK;
  for(n = 0; n < no; n++)
    if(oargs[n] != args[n]) return NOTOK;
  // inargs follow the opcode object arg; a recorded setp overrides the call
  for(n = 0; n < ni; n++) {
    cs_float *want = opcode_dataspace_setp_arg(snap->dataspace, (uint32_t) n);
    if(iargs[n] != (want != NULL ? want : args[no + 1 + n])) return NOTOK;
  }
  return OK;
}

struct opcode_dataspace_header {
  uint32_t refcount;
  uint32_t initialized;
  uint32_t retired;
  OPCODE_RETIRED retire;
  OPCODE_SETP_ENTRY *setp;
  OPCODE_SETP_SLOT *setp_slots;
  void *allocation;
};


#define OPCODE_DATASPACE_ALIGNMENT 16u
#define OPCODE_DATASPACE_PREFIX \
  ((sizeof(OPCODE_DATASPACE_HEADER) + OPCODE_DATASPACE_ALIGNMENT - 1) / \
   OPCODE_DATASPACE_ALIGNMENT * OPCODE_DATASPACE_ALIGNMENT)

static OPCODE_DATASPACE_HEADER *opcode_dataspace_header(OPDS *dataspace) {
  return (OPCODE_DATASPACE_HEADER *)
    ((char *) dataspace - OPCODE_DATASPACE_PREFIX);
}

static int32_t opcode_dataspace_process_retired(CSOUND *csound,
                                                OPCODE_RETIRED *list) {
  OPCODE_RETIRED *next, *other = NULL;
  int32_t found = 0;
  while(list != NULL) {
    next = list->next;
    if(list->csound == csound) {
      found = 1;
      if(list->kind == OPCODE_RETIRE_SETP) {
        csound->Free(csound, list->ptr);
      }
      else if(list->kind == OPCODE_RETIRE_DATASPACE) {
        OPDS *dataspace = (OPDS *) list->ptr;
        if(list->runDeinit && dataspace->deinit != NULL)
          dataspace->deinit(csound, dataspace);
        if(list->freeAfter) {
          ATOMIC_SET(opcode_dataspace_header(dataspace)->retired, 2);
          opcode_dataspace_free_now(csound, dataspace);
        }
        else ATOMIC_SET(opcode_dataspace_header(dataspace)->retired, 0);
      }
    }
    else {
      list->next = other;
      other = list;
    }
    list = next;
  }
  opcode_retired_repush(other);
  return found;
}

void opcode_dataspace_reclaim(CSOUND *csound) {
  OPCODE_RETIRED *list;
  if(csound == NULL || opcode_reclaiming) return;
  opcode_reclaiming = 1;
  list = opcode_retired_take();
  if(ATOMIC_GET(opcode_acquire_readers) != 0) {
    opcode_retired_repush(list);
    opcode_reclaiming = 0;
    return;
  }
  opcode_dataspace_process_retired(csound, list);
  opcode_reclaiming = 0;
}

void opcode_dataspace_drain(CSOUND *csound) {
  OPCODE_RETIRED *list;
  if(csound == NULL || opcode_reclaiming) return;
  opcode_reclaiming = 1;
  for(;;) {
    list = opcode_retired_take();
    if(!opcode_dataspace_process_retired(csound, list)) break;
  }
  opcode_reclaiming = 0;
}

static char *opcode_dataspace_base(OPDS *dataspace) {
  return (char *) opcode_dataspace_header(dataspace)->allocation;
}

static int32_t opcode_dataspace_try_incref(OPDS *dataspace);
static void opcode_dataspace_release_temp(CSOUND *csound, OPDS *dataspace,
                                          int32_t runDeinit);

static int32_t opcode_dataspace_acquire_run(CSOUND *csound, OPCODEOBJ *obj,
                                            OPCODE_SNAPSHOT *snap) {
  OPDS *dataspace;
  int32_t result = NOTOK;
  if(obj == NULL || snap == NULL) return NOTOK;
  memset(snap, 0, sizeof(*snap));
  if(opcode_run_depth >= OPCODE_RUN_SCOPE_MAX) return NOTOK;
  opcode_acquire_readers_enter();
  for(;;) {
    dataspace = (OPDS *) OPCODE_ATOMIC_LOAD_PTR(obj->dataspace);
    if(dataspace == NULL) break;
    if(!opcode_dataspace_try_incref(dataspace)) continue;
    snap->dataspace = dataspace;
    snap->outargp = (cs_float **) OPCODE_ATOMIC_LOAD_PTR(obj->outargp);
    snap->inargp = (cs_float **) OPCODE_ATOMIC_LOAD_PTR(obj->inargp);
    snap->size = obj->size;
    snap->udo_flag = obj->udo_flag;
    if((OPDS *) OPCODE_ATOMIC_LOAD_PTR(obj->dataspace) == dataspace) {
      opcode_run_stack[opcode_run_depth++] = dataspace;
      result = OK;
      break;
    }
    opcode_dataspace_release_temp(csound, dataspace, 1);
  }
  opcode_acquire_readers_leave();
  return result;
}

static void opcode_dataspace_release_run(CSOUND *csound, OPCODEOBJ *obj,
                                         OPCODE_SNAPSHOT *snap) {
  if(snap == NULL || snap->dataspace == NULL) return;
  if(opcode_run_depth > 0 &&
     opcode_run_stack[opcode_run_depth - 1] == snap->dataspace)
    opcode_run_depth--;
  opcode_dataspace_release_temp(csound, snap->dataspace, 1);
}

static uint32_t *opcode_dataspace_refslot(OPDS *dataspace) {
  return &opcode_dataspace_header(dataspace)->refcount;
}

static void opcode_dataspace_mark_initialized(OPDS *dataspace) {
  if(dataspace != NULL)
    opcode_dataspace_header(dataspace)->initialized = 1;
}

static void opcode_dataspace_clear_initialized(OPDS *dataspace) {
  if(dataspace != NULL)
    opcode_dataspace_header(dataspace)->initialized = 0;
}

static int32_t opcode_dataspace_was_initialized(OPDS *dataspace) {
  return dataspace != NULL &&
    opcode_dataspace_header(dataspace)->initialized != 0;
}

static CS_VAR_MEM *opcode_setp_storage_new(CSOUND *csound, cs_float *arg,
                                            INSDS *ctx) {
  const CS_TYPE *type = csoundGetTypeForArg(arg);
  CS_VARIABLE *var;
  CS_VAR_MEM *storage;
  cs_float *value;
  ARRAY_VAR_INIT arrayInit;
  const void *typeArg = NULL;
  size_t size;
  if(type == NULL || type->createVariable == NULL) return NULL;
  if(type == &CS_VAR_TYPE_ARRAY) {
    ARRAYDAT *array = (ARRAYDAT *) arg;
    arrayInit.dimensions = array->dimensions;
    arrayInit.type = array->arrayType;
    typeArg = &arrayInit;
  }
  var = csoundCreateVariableForType(csound, type, typeArg, ctx);
  if(var == NULL) return NULL;
  size = (size_t) CS_VAR_TYPE_OFFSET + (size_t) var->memBlockSize;
  size = (size + sizeof(cs_float) - 1) & ~(sizeof(cs_float) - 1);
  storage = (CS_VAR_MEM *) csound->Calloc(csound, size);
  if(storage == NULL) {
    csound->Free(csound, var);
    return NULL;
  }
  storage->varType = var->varType;
  value = (cs_float *) ((char *) storage + CS_VAR_TYPE_OFFSET);
  if(var->initializeVariableMemory != NULL)
    var->initializeVariableMemory(csound, var, value);
  if(type->copyValue != NULL) type->copyValue(csound, type, value, arg, ctx);
  else memcpy(value, arg, (size_t) var->memBlockSize);
  csound->Free(csound, var);
  return storage;
}

static void opcode_setp_storage_free(CSOUND *csound, CS_VAR_MEM *storage) {
  cs_float *value;
  if(storage == NULL) return;
  value = (cs_float *) ((char *) storage + CS_VAR_TYPE_OFFSET);
  if(storage->varType != NULL && storage->varType->freeVariableMemory != NULL)
    storage->varType->freeVariableMemory(csound, value);
  csound->Free(csound, storage);
}

static OPCODE_SETP_SLOT *opcode_setp_slot_new(CSOUND *csound,
                                              OPCODE_DATASPACE_HEADER *header,
                                              cs_float *arg, INSDS *ctx) {
  CS_VAR_MEM *storage = opcode_setp_storage_new(csound, arg, ctx);
  OPCODE_SETP_SLOT *slot, *head;
  if(storage == NULL) return NULL;
  slot = (OPCODE_SETP_SLOT *) csound->Calloc(csound, sizeof(*slot));
  if(slot == NULL) {
    opcode_setp_storage_free(csound, storage);
    return NULL;
  }
  slot->storage = storage;
  slot->value = (cs_float *) ((char *) storage + CS_VAR_TYPE_OFFSET);
  do {
    head = (OPCODE_SETP_SLOT *) OPCODE_ATOMIC_LOAD_PTR(header->setp_slots);
    slot->next = head;
  } while(!OPCODE_ATOMIC_CAS_PTR(header->setp_slots, slot, head));
  return slot;
}

static void opcode_setp_slot_free(CSOUND *csound, OPCODE_SETP_SLOT *slot) {
  if(slot == NULL) return;
  opcode_setp_storage_free(csound, slot->storage);
  csound->Free(csound, slot);
}

static void opcode_dataspace_free_setp_nodes(CSOUND *csound,
                                               OPCODE_SETP_ENTRY *entry) {
  while(entry != NULL) {
    OPCODE_SETP_ENTRY *next = entry->next;
    csound->Free(csound, entry);
    entry = next;
  }
}

static void opcode_dataspace_retire_setp_list(CSOUND *csound,
                                               OPCODE_SETP_ENTRY *entry) {
  while(entry != NULL) {
    OPCODE_SETP_ENTRY *next = entry->next;
    entry->retire.next = NULL;
    entry->retire.csound = csound;
    entry->retire.kind = OPCODE_RETIRE_SETP;
    entry->retire.runDeinit = 0;
    entry->retire.freeAfter = 1;
    entry->retire.ptr = entry;
    opcode_retired_push(&entry->retire);
    entry = next;
  }
}

static OPCODE_SETP_ENTRY *opcode_dataspace_detach_setp(OPDS *dataspace) {
  OPCODE_DATASPACE_HEADER *header;
  OPCODE_SETP_ENTRY *entries;
  if(dataspace == NULL) return NULL;
  header = opcode_dataspace_header(dataspace);
  do {
    entries = (OPCODE_SETP_ENTRY *) OPCODE_ATOMIC_LOAD_PTR(header->setp);
  } while(!OPCODE_ATOMIC_CAS_PTR(header->setp, NULL, entries));
  return entries;
}

static void opcode_dataspace_free_setp_slots(CSOUND *csound,
                                             OPCODE_DATASPACE_HEADER *header) {
  OPCODE_SETP_SLOT *slot, *next, *slots;
  if(header == NULL) return;
  do {
    slots = (OPCODE_SETP_SLOT *) OPCODE_ATOMIC_LOAD_PTR(header->setp_slots);
  } while(!OPCODE_ATOMIC_CAS_PTR(header->setp_slots, NULL, slots));
  for(slot = slots; slot != NULL; slot = next) {
    next = slot->next;
    opcode_setp_slot_free(csound, slot);
  }
}

static void opcode_dataspace_free_setp(CSOUND *csound, OPDS *dataspace) {
  OPCODE_DATASPACE_HEADER *header;
  if(dataspace == NULL) return;
  header = opcode_dataspace_header(dataspace);
  opcode_dataspace_retire_setp_list(csound,
                                    opcode_dataspace_detach_setp(dataspace));
  opcode_dataspace_free_setp_slots(csound, header);
}

static OPCODE_SETP_ENTRY *opcode_dataspace_find_setp(OPDS *dataspace,
                                                       uint32_t ndx) {
  OPCODE_DATASPACE_HEADER *header = opcode_dataspace_header(dataspace);
  OPCODE_SETP_ENTRY *entry;
  for(entry = (OPCODE_SETP_ENTRY *) OPCODE_ATOMIC_LOAD_PTR(header->setp);
      entry != NULL; entry = entry->next)
    if(entry->ndx == ndx) return entry;
  return NULL;
}

static OPCODE_SETP_ENTRY *opcode_dataspace_clone_setp(CSOUND *csound,
                                                       OPDS *dataspace,
                                                       uint32_t ndx,
                                                       OPCODE_SETP_ENTRY *entry) {
  OPCODE_DATASPACE_HEADER *header = opcode_dataspace_header(dataspace);
  OPCODE_SETP_ENTRY *old =
    (OPCODE_SETP_ENTRY *) OPCODE_ATOMIC_LOAD_PTR(header->setp);
  OPCODE_SETP_ENTRY *copy, *head = NULL, **tail = &head, *next;
  while(old != NULL) {
    if(old->ndx != ndx) {
      copy = (OPCODE_SETP_ENTRY *) csound->Calloc(csound, sizeof(*copy));
      if(copy == NULL) {
        opcode_dataspace_free_setp_nodes(csound, head);
        return NULL;
      }
      copy->ndx = old->ndx;
      copy->slot = old->slot;
      *tail = copy;
      tail = &copy->next;
    }
    old = old->next;
  }
  next = (OPCODE_SETP_ENTRY *) csound->Calloc(csound, sizeof(*next));
  if(next == NULL) {
    opcode_dataspace_free_setp_nodes(csound, head);
    return NULL;
  }
  next->ndx = ndx;
  next->slot = entry->slot;
  *tail = next;
  return head;
}

static int32_t opcode_dataspace_record_setp(CSOUND *csound, OPDS *dataspace,
                                              uint32_t ndx, cs_float *arg,
                                              INSDS *ctx) {
  OPCODE_DATASPACE_HEADER *header;
  OPCODE_SETP_ENTRY *entry, *old, *copy;
  OPCODE_SETP_SLOT *slot;
  if(dataspace == NULL || arg == NULL) return NOTOK;
  header = opcode_dataspace_header(dataspace);
  entry = (OPCODE_SETP_ENTRY *) csound->Calloc(csound, sizeof(*entry));
  if(entry == NULL) return NOTOK;
  slot = opcode_setp_slot_new(csound, header, arg, ctx);
  if(slot == NULL) {
    csound->Free(csound, entry);
    return NOTOK;
  }
  entry->ndx = ndx;
  entry->slot = slot;
  for(;;) {
    old = (OPCODE_SETP_ENTRY *) OPCODE_ATOMIC_LOAD_PTR(header->setp);
    copy = opcode_dataspace_clone_setp(csound, dataspace, ndx, entry);
    if(copy == NULL) break;
    if(OPCODE_ATOMIC_CAS_PTR(header->setp, copy, old)) {
      opcode_dataspace_retire_setp_list(csound, old);
      csound->Free(csound, entry);
      return OK;
    }
    opcode_dataspace_free_setp_nodes(csound, copy);
  }
  csound->Free(csound, entry);
  return NOTOK;
}

static cs_float *opcode_dataspace_setp_arg(OPDS *dataspace, uint32_t ndx) {
  OPCODE_SETP_ENTRY *entry;
  if(dataspace == NULL) return NULL;
  entry = opcode_dataspace_find_setp(dataspace, ndx);
  return entry != NULL && entry->slot != NULL ? entry->slot->value : NULL;
}

static void opcode_dataspace_restore_setp(OPDS *dataspace,
                                           cs_float **inargp) {
  const TEXT *t;
  uint32_t slots;
  OPCODE_SETP_ENTRY *entry;
  if(dataspace == NULL || inargp == NULL) return;
  t = &(dataspace->optext->t);
  slots = input_slots(t->oentry->intypes, t->inArgCount);
  for(entry = (OPCODE_SETP_ENTRY *)
        OPCODE_ATOMIC_LOAD_PTR(opcode_dataspace_header(dataspace)->setp);
      entry != NULL; entry = entry->next)
    if(entry->slot != NULL && entry->ndx < slots)
      inargp[entry->ndx] = entry->slot->value;
}



/**
 * allocate and initialise opcode dataspace (with hidden refcount prefix)
 */
static OPDS *opcode_dataspace_new(CSOUND *csound, OENTRY *entry, OPDS *h) {
    size_t dsblksiz = entry->dsblksiz > sizeof(OPDS) ?
      (size_t) entry->dsblksiz : sizeof(OPDS);
    size_t total;
    char *allocation;
    OPDS *dataspace;
    OPCODE_DATASPACE_HEADER *header;
    if(dsblksiz > (size_t) -1 - OPCODE_DATASPACE_PREFIX -
       (OPCODE_DATASPACE_ALIGNMENT - 1)) return NULL;
    total = OPCODE_DATASPACE_PREFIX + dsblksiz +
      (OPCODE_DATASPACE_ALIGNMENT - 1);
    allocation = (char *) csound->Calloc(csound, total);
    if(allocation == NULL) return NULL;
    dataspace = (OPDS *) (((uintptr_t) allocation + OPCODE_DATASPACE_PREFIX +
                           OPCODE_DATASPACE_ALIGNMENT - 1) &
                          ~(uintptr_t) (OPCODE_DATASPACE_ALIGNMENT - 1));
    header = opcode_dataspace_header(dataspace);
    header->allocation = allocation;
    header->setp = NULL;
    header->setp_slots = NULL;
    memset(&header->retire, 0, sizeof(header->retire));
    header->retired = 0;
    *opcode_dataspace_refslot(dataspace) = 1;
    header->initialized = 0;
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
}

/**
 * element i of an Opcode[] array: the array stride is the aligned
 * member size of the Opcode type, which can exceed sizeof(OPCODEOBJ)
 */
static inline OPCODEOBJ *opcode_array_obj(ARRAYDAT *array, int32_t i) {
  return (OPCODEOBJ *) ((char *) array->data +
                        (size_t) i * array->arrayMemberSize);
}

static void opcode_dataspace_free_now(CSOUND *csound, OPDS *dataspace) {
  if(dataspace == NULL) return;
  opcode_dataspace_free_setp(csound, dataspace);
  if(dataspace->optext != NULL) {
    csound->Free(csound, dataspace->optext->t.inlist);
    csound->Free(csound, dataspace->optext);
  }
  csound->Free(csound, opcode_dataspace_base(dataspace));
}

static int32_t opcode_dataspace_try_incref(OPDS *dataspace) {
  uint32_t old;
  uint32_t *ref;
  if(dataspace == NULL) return 0;
  ref = opcode_dataspace_refslot(dataspace);
  for(;;) {
    old = (uint32_t) ATOMIC_GET(*ref);
    if(old == 0 || old == 0xffffffffu) return 0;
    if(OPCODE_ATOMIC_CAS_U32(*ref, old + 1, old)) return (int32_t) (old + 1);
  }
}

static int32_t opcode_dataspace_decref_cas(OPDS *dataspace) {
  uint32_t old;
  uint32_t *ref;
  if(dataspace == NULL) return 0;
  ref = opcode_dataspace_refslot(dataspace);
  for(;;) {
    old = (uint32_t) ATOMIC_GET(*ref);
    if(old == 0) return 0;
    if(OPCODE_ATOMIC_CAS_U32(*ref, old - 1, old)) return (int32_t) (old - 1);
  }
}

static void opcode_dataspace_setref_unlocked(OPDS *dataspace, uint32_t count) {
  if(dataspace != NULL) ATOMIC_SET(*opcode_dataspace_refslot(dataspace), count);
}

static void opcode_dataspace_retire(CSOUND *csound, OPDS *dataspace,
                                     int32_t runDeinit, int32_t freeAfter) {
  OPCODE_DATASPACE_HEADER *header;
  uint32_t old;
  if(dataspace == NULL) return;
  header = opcode_dataspace_header(dataspace);
  for(;;) {
    old = (uint32_t) ATOMIC_GET(header->retired);
    if(old == 2) return;
    if(old == 1) {
      if(freeAfter) ATOMIC_SET(header->retire.freeAfter, 1);
      if(runDeinit) ATOMIC_SET(header->retire.runDeinit, 1);
      return;
    }
    old = 0;
    if(OPCODE_ATOMIC_CAS_U32(header->retired, 1, old)) break;
  }
  header->retire.next = NULL;
  header->retire.csound = csound;
  header->retire.kind = OPCODE_RETIRE_DATASPACE;
  header->retire.runDeinit = runDeinit;
  header->retire.freeAfter = freeAfter;
  header->retire.ptr = dataspace;
  opcode_retired_push(&header->retire);
}

void opcode_dataspace_free(CSOUND *csound, OPDS *dataspace) {
  OPCODE_DATASPACE_HEADER *header;
  uint32_t old;
  if(dataspace == NULL) return;
  header = opcode_dataspace_header(dataspace);
  for(;;) {
    old = (uint32_t) ATOMIC_GET(header->retired);
    if(old == 2) return;
    if(old == 1) {
      ATOMIC_SET(header->retire.freeAfter, 1);
      return;
    }
    old = 0;
    if(OPCODE_ATOMIC_CAS_U32(header->retired, 2, old)) break;
  }
  opcode_dataspace_free_now(csound, dataspace);
}


int32_t opcode_dataspace_refcount(OPDS *dataspace) {
  if(dataspace == NULL) return 0;
  return (int32_t) ATOMIC_GET(*opcode_dataspace_refslot(dataspace));
}

int32_t opcode_dataspace_incref(OPDS *dataspace) {
  return opcode_dataspace_try_incref(dataspace);
}

int32_t opcode_dataspace_decref(OPDS *dataspace) {
  return opcode_dataspace_decref_cas(dataspace);
}

void opcode_dataspace_setref(OPDS *dataspace, uint32_t count) {
  opcode_dataspace_setref_unlocked(dataspace, count);
}

static void opcode_dataspace_release_temp(CSOUND *csound, OPDS *dataspace,
                                          int32_t runDeinit) {
  int32_t rc;
  if(dataspace == NULL) return;
  rc = opcode_dataspace_decref_cas(dataspace);
  if(rc == 0) {
    opcode_dataspace_free_setp(csound, dataspace);
    opcode_dataspace_retire(csound, dataspace, runDeinit, 1);
  }
}

OPCODE_COPY_STATUS opcode_dataspace_acquire_copy(CSOUND *csound,
                                                  OPCODEOBJ *src, INSDS *ctx,
                                                  OPDS **dataspace,
                                                  size_t *size,
                                                  cs_float ***outargp,
                                                  cs_float ***inargp,
                                                  uint32_t *udo_flag) {
  OPDS *ds, *current;
  OPCODE_COPY_STATUS status = OPCODE_COPY_EMPTY;
  if(src == NULL) return OPCODE_COPY_EMPTY;
  opcode_acquire_readers_enter();
  for(;;) {
    ds = (OPDS *) OPCODE_ATOMIC_LOAD_PTR(src->dataspace);
    if(ds == NULL) break;
    if(opcode_acquire_hook != NULL) opcode_acquire_hook(csound, src, ds);
    if(!opcode_dataspace_try_incref(ds)) continue;
    if(opcode_dataspace_ended(ds, src->udo_flag)) {
      opcode_dataspace_release_temp(csound, ds, 0);
      status = OPCODE_COPY_ENDED;
      break;
    }
    if(context_check(csound, ds, ctx) != OK) {
      opcode_dataspace_release_temp(csound, ds, 0);
      status = OPCODE_COPY_CONTEXT;
      break;
    }
    current = (OPDS *) OPCODE_ATOMIC_LOAD_PTR(src->dataspace);
    if(current == ds) {
      if(dataspace != NULL) *dataspace = ds;
      if(size != NULL) *size = src->size;
      if(outargp != NULL) *outargp = (cs_float **)
        OPCODE_ATOMIC_LOAD_PTR(src->outargp);
      if(inargp != NULL) *inargp = (cs_float **)
        OPCODE_ATOMIC_LOAD_PTR(src->inargp);
      if(udo_flag != NULL) *udo_flag = src->udo_flag;
      status = OPCODE_COPY_OK;
      break;
    }
    opcode_dataspace_release_temp(csound, ds, 1);
  }
  opcode_acquire_readers_leave();
  return status;
}

int32_t opcode_dataspace_release(CSOUND *csound, OPCODEOBJ *obj,
                                  int32_t runDeinit) {
  OPDS *dataspace, *spare, *freeSpare = NULL;
  int32_t rc;
  if(obj == NULL) return 0;
  for(;;) {
    dataspace = (OPDS *) OPCODE_ATOMIC_LOAD_PTR(obj->dataspace);
    if(dataspace == NULL) return 0;
    if(OPCODE_ATOMIC_CAS_PTR(obj->dataspace, NULL, dataspace)) break;
  }
  OPCODE_ATOMIC_STORE_PTR(obj->outargp, NULL);
  OPCODE_ATOMIC_STORE_PTR(obj->inargp, NULL);
  rc = opcode_dataspace_decref_cas(dataspace);
  if(rc == 0) {
    int32_t freeAfter = 1;
    if(obj->udo_flag) {
      spare = (OPDS *) OPCODE_ATOMIC_LOAD_PTR(obj->spare);
      if(spare != NULL && spare != dataspace &&
         ((UOPCODE *) spare)->ip == NULL)
        freeSpare = spare;
      OPCODE_ATOMIC_STORE_PTR(obj->spare, dataspace);
      freeAfter = 0;
    }
    if(freeSpare != NULL) opcode_dataspace_retire(csound, freeSpare, 1, 1);
    opcode_dataspace_free_setp(csound, dataspace);
    opcode_dataspace_retire(csound, dataspace, runDeinit, freeAfter);
  }
  return rc;
}

/**
 * take back a UDO dataspace kept by delete: reuse it for the same
 * definition, or free it once its instance has been deactivated
 */
static void opcode_take_spare(CSOUND *csound, OPCODEOBJ *obj, OENTRY *entry) {
  UOPCODE *udo;
  OPDS *freeDataspace = NULL, *spare;
  spare = (OPDS *) OPCODE_ATOMIC_LOAD_PTR(obj->spare);
  udo = (UOPCODE *) spare;
  if(udo == NULL) return;
  for(;;) {
    spare = (OPDS *) OPCODE_ATOMIC_LOAD_PTR(obj->spare);
    if(spare == NULL) return;
    if(OPCODE_ATOMIC_CAS_PTR(obj->spare, NULL, spare)) break;
  }
  if((OPDS *) OPCODE_ATOMIC_LOAD_PTR(obj->dataspace) == NULL &&
     udo->h.optext->t.oentry == entry) {
    OPCODE_ATOMIC_STORE_PTR(obj->dataspace, (OPDS *) udo);
    opcode_dataspace_setref_unlocked((OPDS *) udo, 1);
  }
  else if(udo->ip == NULL) freeDataspace = (OPDS *) udo;
  if(freeDataspace != NULL)
    opcode_dataspace_retire(csound, freeDataspace, 1, 1);
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
    if(obj->dataspace != NULL) {
      int32_t rc = opcode_dataspace_refcount(obj->dataspace);
      if(rc > 1)
        opcode_dataspace_release(csound, obj, 0);
      else if(obj->size < entry->dsblksiz ||
              obj->dataspace->optext == NULL ||
              obj->dataspace->optext->t.oentry != entry)
        opcode_dataspace_release(csound, obj, 1);
    }
    opcode_take_spare(csound, obj, entry);
    opcode_dataspace_clear_initialized(obj->dataspace);
    opcode_dataspace_free_setp(csound, obj->dataspace);
    // a dataspace is kept for the opcode it was made for, and no other;
    // one too small is not read
    if(obj->dataspace == NULL || obj->size < entry->dsblksiz ||
       obj->dataspace->optext->t.oentry != entry) {
      if((obj->dataspace = opcode_dataspace_new(csound, entry, &(p->h)))
          == NULL)
        return csound->InitError(csound, Str("could not allocate opcode object\n"));
      // the arguments bound before were the other dataspace's
      obj->outargp = obj->inargp = NULL;
     }
      obj->udo_flag = entry->useropinfo == NULL ? 0  : 1;
      obj->size = entry->dsblksiz;
      return OK;
  }
  return csound->InitError(csound, Str("invalid opcode reference\n"));
}

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
    if (UNLIKELY(tabinit(csound, p->r, n, p->h.insdshead) != OK))
      return csound_array_init_resize_error(csound);
    for(i = 0; i < n; i++) {
      obj = opcode_array_obj(p->r, i);
      if(obj->dataspace != NULL) {
        int32_t rc = opcode_dataspace_refcount(obj->dataspace);
        if(rc > 1)
          opcode_dataspace_release(csound, obj, 0);
        else if(obj->size < entry->dsblksiz ||
                obj->dataspace->optext == NULL ||
                obj->dataspace->optext->t.oentry != entry)
          opcode_dataspace_release(csound, obj, 1);
      }
      opcode_take_spare(csound, obj, entry);
      opcode_dataspace_clear_initialized(obj->dataspace);
      opcode_dataspace_free_setp(csound, obj->dataspace);
      // a dataspace is kept for the opcode it was made for, and no other;
      // one too small is not read
      if(obj->dataspace == NULL || obj->size < entry->dsblksiz ||
         obj->dataspace->optext->t.oentry != entry) {
       if((obj->dataspace = opcode_dataspace_new(csound, entry, &(p->h)))
          == NULL)
        return csound->InitError(csound, Str("could not allocate opcode object\n"));
       // the arguments bound before were the other dataspace's
       obj->outargp = obj->inargp = NULL;
      }
      obj->udo_flag = entry->useropinfo == NULL ? 0  : 1;
      obj->size = entry->dsblksiz;
      }
    return OK;
  }
  return csound->InitError(csound, Str("invalid opcode reference\n"));
}

/**
 *  Delete opcode obj dataspace (refcounted)
 *
 *  delete obj:Opcode
 */
int32_t opcode_delete(CSOUND *csound, AOP *p) {
  opcode_dataspace_release(csound, (OPCODEOBJ *) p->r, 1);
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
    csound->Message(csound, Str("%s %s\tout-types: %s\tin-types: %s \n"),
                    ep->opname, obj->udo_flag ? "(UDO)" : "",
                    ep->outypes, ep->intypes);
  }
  return OK;
}


/* Opcodes declared with "*" outputs keep one argument slot even when a call
   has no outputs, so the object and its inputs then start at args[1]. */
static cs_float **oprun_args(OPRUN *p) {
  return p->OUTOCOUNT == 0 ? p->args + 1 : p->args;
}

/**
 * this opcode connects all args to opcode obj and
 * optionally runs init function
 *
 * outargs  init  opc:Opcode, inargs
 */
int32_t opcode_object_init(CSOUND *csound, OPRUN *p) {
  cs_float **pargs = oprun_args(p);
  OPCODEOBJ *obj = (OPCODEOBJ *) pargs[p->OUTOCOUNT];
  if(obj->dataspace != NULL) {
  if(opcode_udo_ended(obj))
    return csound->InitError(csound,
                             Str("opcode object UDO instance has ended\n"));
  if(context_check(csound, obj->dataspace, p->h.insdshead) != OK)
    return csound->InitError(csound, Str("incompatible context, "
                              "cannot initialise opcode obj for %s\n"),
                              obj->dataspace->optext->t.oentry->opname);
  set_line_num_and_loc(obj->dataspace, p);
  if(setup_args(csound, obj, &(p->h), pargs, NULL, p->OUTOCOUNT,
                 p->INOCOUNT - 1, NULL) == OK) {
    opcode_dataspace_free_setp(csound, obj->dataspace);
    if(obj->dataspace->init != NULL) {
       int32_t ret = obj->dataspace->init(csound, obj->dataspace);
       if(ret == OK) opcode_dataspace_mark_initialized(obj->dataspace);
       return ret;
    }
    opcode_dataspace_mark_initialized(obj->dataspace);
    return OK;
  }
  return csound->InitError(csound, Str("mismatching arguments\n"
                           "for opcode obj %s\t"
                           "outypes: %s\tintypes: %s\n"),
                           obj->dataspace->optext->t.oentry->opname,
                           obj->dataspace->optext->t.oentry->outypes,
                           obj->dataspace->optext->t.oentry->intypes);
  }
  return csound->InitError(csound, Str("opcode object not initialised\n"));
}

/**
 * this opcode runs a perf pass on an OpcodeObj
 *
 * outargs perf opc:Opcode, inargs
 */
int32_t opcode_object_perf(CSOUND *csound, OPRUN *p) {
  cs_float **pargs = oprun_args(p);
  OPCODEOBJ *obj = (OPCODEOBJ *) pargs[p->OUTOCOUNT];
  OPCODE_SNAPSHOT snap;
  int32_t result;
  if(opcode_dataspace_acquire_run(csound, obj, &snap) != OK)
    return csound->PerfError(csound, &(p->h),
                             Str("opcode object not initialised\n"));
  if(opcode_dataspace_ended(snap.dataspace, snap.udo_flag)) {
    result = csound->PerfError(csound, &(p->h),
                               Str("opcode object UDO instance has ended\n"));
    goto done;
  }
  if(context_check(csound, snap.dataspace, p->h.insdshead) != OK) {
    result = csound->PerfError(csound, &(p->h), Str("incompatible context, "
                               "cannot perform opcode obj for %s\n"),
                               snap.dataspace->optext->t.oentry->opname);
    goto done;
  }
  set_line_num_and_loc(snap.dataspace, p);
  if(check_consistency(&snap, pargs, p->OUTOCOUNT, p->INOCOUNT - 1) != OK) {
    if(snap.udo_flag)
      user_opcode_rebind_constants((UOPCODE *) snap.dataspace,
                                   pargs + p->OUTOCOUNT + 1, p->INOCOUNT - 1);
    if(setup_args(csound, obj, &(p->h), pargs, NULL, p->OUTOCOUNT,
                  p->INOCOUNT - 1, &snap) != OK) {
      result = csound->PerfError(csound, &(p->h), Str("mismatching arguments\n"
                           "for opcode obj %s\t"
                           "outypes: %s\tintypes: %s\n"),
                           snap.dataspace->optext->t.oentry->opname,
                           snap.dataspace->optext->t.oentry->outypes,
                           snap.dataspace->optext->t.oentry->intypes);
      goto done;
    }
    // setp overrides must be in the slots before the UDO body is rewired
    opcode_dataspace_restore_setp(snap.dataspace, snap.inargp);
    if(snap.udo_flag &&
       user_opcode_rebind_args(csound, (UOPCODE *) snap.dataspace) != OK) {
      result = csound->PerfError(csound, &(p->h),
                                 Str("could not rebind arguments "
                                     "for opcode obj %s\n"),
                                  snap.dataspace->optext->t.oentry->opname);
      goto done;
    }
  }
  result = snap.dataspace->perf != NULL
    ? snap.dataspace->perf(csound, snap.dataspace) : OK;
done:
  opcode_dataspace_release_run(csound, obj, &snap);
  return result;
}

/* How many opcode objects op runs, if it is a run or perf call of them. */
int32_t opcode_run_count(OPDS *op)
{
  if (op->perf == (SUBR) opcode_run_perf ||
      op->perf == (SUBR) opcode_object_perf)
    return 1;
  if (op->perf == (SUBR) opcode_array_perf) {
    OPRUN *p = (OPRUN *) op;
    const ARRAYDAT *objects = (const ARRAYDAT *) oprun_args(p)[p->OUTOCOUNT];
    return objects->dimensions > 0 ? objects->sizes[0] : 0;
  }
  return 0;
}

/* The dataspace of the i-th of the opcode objects op runs. */
OPDS *opcode_run_dataspace(OPDS *op, int32_t i)
{
  OPRUN *p = (OPRUN *) op;
  cs_float **pargs = oprun_args(p);
  if (op->perf == (SUBR) opcode_array_perf)
    return (OPDS *) OPCODE_ATOMIC_LOAD_PTR(
      opcode_array_obj((ARRAYDAT *) pargs[p->OUTOCOUNT], i)->dataspace);
  return (OPDS *) OPCODE_ATOMIC_LOAD_PTR(
    ((const OPCODEOBJ *) pargs[p->OUTOCOUNT])->dataspace);
}

/**
 * perf-time function for run opcode,
 * no checks required
 */
int32_t opcode_run_perf(CSOUND *csound, OPRUN *p) {
  cs_float **pargs = oprun_args(p);
  OPCODEOBJ *obj = (OPCODEOBJ *) pargs[p->OUTOCOUNT];
  OPCODE_SNAPSHOT snap;
  int32_t result;
  if(UNLIKELY(obj->dataspace == NULL ||
              obj->dataspace->optext->t.outlist == NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("opcode object not initialised\n"));
  if(opcode_dataspace_acquire_run(csound, obj, &snap) != OK)
    return csound->PerfError(csound, &(p->h),
                             Str("opcode object not initialised\n"));
  if(opcode_dataspace_ended(snap.dataspace, snap.udo_flag)) {
    result = csound->PerfError(csound, &(p->h),
                               Str("opcode object UDO instance has ended\n"));
    goto done;
  }
  if(context_check(csound, snap.dataspace, p->h.insdshead) != OK) {
    result = csound->PerfError(csound, &(p->h), Str("incompatible context, "
                               "cannot perform opcode obj for %s\n"),
                               snap.dataspace->optext->t.oentry->opname);
    goto done;
  }
  set_line_num_and_loc(snap.dataspace, p);
  if(check_consistency(&snap, pargs, p->OUTOCOUNT, p->INOCOUNT - 1) != OK) {
    if(snap.udo_flag)
      user_opcode_rebind_constants((UOPCODE *) snap.dataspace,
                                   pargs + p->OUTOCOUNT + 1, p->INOCOUNT - 1);
    if(setup_args(csound, obj, &(p->h), pargs, NULL, p->OUTOCOUNT,
                   p->INOCOUNT - 1, &snap) != OK) {
      result = csound->PerfError(csound, &(p->h), Str("mismatching arguments\n"
                           "for opcode obj %s\t"
                           "outypes: %s\tintypes: %s\n"),
                           snap.dataspace->optext->t.oentry->opname,
                           snap.dataspace->optext->t.oentry->outypes,
                           snap.dataspace->optext->t.oentry->intypes);
      goto done;
    }
    opcode_dataspace_restore_setp(snap.dataspace, snap.inargp);
    if(snap.udo_flag &&
       user_opcode_rebind_args(csound, (UOPCODE *) snap.dataspace) != OK) {
      result = csound->PerfError(csound, &(p->h),
                                  Str("could not rebind arguments "
                                      "for opcode obj %s\n"),
                                   snap.dataspace->optext->t.oentry->opname);
      goto done;
    }
  }
  result = snap.dataspace->perf != NULL
    ? snap.dataspace->perf(csound, snap.dataspace) : OK;
done:
  opcode_dataspace_release_run(csound, obj, &snap);
  return result;
}

/**
 *  Delete opcode obj array dataspace
 *
 *  delete obj:Opcode[]
 */
int32_t opcode_delete_array(CSOUND *csound, AOP *p) {
  ARRAYDAT  *array = (ARRAYDAT *) p->r;
  int32_t   n = array->sizes[0], i;
  cs_float *r = p->r;
  for(i = 0; i < n; i++) {
    p->r = (cs_float *) opcode_array_obj(array, i);
    opcode_delete(csound, p);
  }
  p->r = r;
  return OK;
}

// check whether argument n of the opcode is declared as an array
static int32_t isTypeArray(OPCODEOBJ *obj, int32_t n, int32_t isInput) {
  TEXT *t = &(((OPDS *) OPCODE_ATOMIC_LOAD_PTR(obj->dataspace))->optext->t);
  OENTRY *ep = t->oentry;
  const char *types = isInput ? ep->intypes : ep->outypes;
  int32_t i = 0;
  while (*types) {
    int32_t array = 0;
    if (*types == '[') {
      // internal form: "[k]", "[[k]", "[:Name;]"
      array = 1;
      while (*types == '[') types++;
      if (*types == ':')
        while (*types && *types != ';') types++;
      if (*types) types++;
      if (*types == ']') types++;
    } else {
      // external form: "k", "k[]", ":Name;", ":Name;[][]"
      if (*types == ':')
        while (*types && *types != ';') types++;
      if (*types) types++;
      while (types[0] == '[' && types[1] == ']') {
        array = 1;
        types += 2;
      }
    }
    if (i == n) return array;
    i++;
  }
  return 0;
}

/* Whether run gives each object one element of array argument a, rather
   than passing the whole array to the opcode. */
static int32_t oprun_splits_arg(OPRUN *p, cs_float **pargs, OPCODEOBJ *obj,
                                int32_t a) {
  int32_t no = (int32_t) p->OUTOCOUNT;
  if (a == no || csoundGetTypeForArg(pargs[a]) != &CS_VAR_TYPE_ARRAY)
    return 0;
  return a < no ? !isTypeArray(obj, a, 0) : !isTypeArray(obj, a - no - 1, 1);
}

/* Point the slot table at storage for every split array argument. Slots
   share one block so that re-initialisation reuses it; nesting AUXCH
   blocks inside p->mem would unlink them when AuxAlloc clears it. */
static void oprun_alloc_slots(CSOUND *csound, OPRUN *p, cs_float **pargs,
                              OPCODEOBJ *obj, int32_t n, int32_t argn) {
  CS_VAR_MEM **slot;
  size_t total = 0;
  char *base;
  int32_t a, i;

  csound->AuxAlloc(csound, sizeof(CS_VAR_MEM *) * n * argn, &p->mem);
  slot = (CS_VAR_MEM **) p->mem.auxp;
  for (a = 0; a < argn; a++)
    if (oprun_splits_arg(p, pargs, obj, a))
      total += n * (size_t) CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET +
                       ((ARRAYDAT *) pargs[a])->arrayMemberSize);
  if (total == 0)
    return;
  csound->AuxAlloc(csound, total, &p->slots);
  base = (char *) p->slots.auxp;
  for (a = 0; a < argn; a++) {
    size_t size;
    if (!oprun_splits_arg(p, pargs, obj, a))
      continue;
    size = (size_t) CS_FLOAT_ALIGN(CS_VAR_TYPE_OFFSET +
                                   ((ARRAYDAT *) pargs[a])->arrayMemberSize);
    for (i = 0; i < n; i++) {
      slot[i + a * n] = (CS_VAR_MEM *) base;
      base += size;
    }
  }
}

/**
 *  Init function for run on Opcode array
 *  sets up dataspace and optionally runs init function
 *
 */
int32_t opcode_array_init(CSOUND *csound, OPRUN *p) {
  cs_float **pargs = oprun_args(p);
  int32_t i, j, n = 0, m;
  cs_float *args[VARGMAX] = {0};
  const CS_TYPE *types[VARGMAX] = {0};
  ARRAYDAT  *array, *objects;
  OPCODEOBJ *obj;
  CS_VAR_MEM *argmem = NULL;
  CS_VAR_MEM **slot;
  int32_t ndx, argn;
  objects = (ARRAYDAT *) pargs[p->OUTOCOUNT];
  n = objects->sizes[0];
  argn = p->INOCOUNT + p->OUTOCOUNT;
  if (n == 0)
    return OK;
  for(i = 0; i < n; i++) {
    obj = opcode_array_obj(objects, i);
    if(obj->dataspace == NULL)
      return csound->InitError(csound, Str("opcode object not initialised\n"));
    if(opcode_udo_ended(obj))
      return csound->InitError(csound,
                               Str("opcode object UDO instance has ended\n"));
  }
  // slots are laid out from objects[0], so every object must split the
  // same array arguments
  for(i = 1; i < n; i++)
    for(j = 0; j < argn; j++)
      if(oprun_splits_arg(p, pargs, opcode_array_obj(objects, i), j) !=
         oprun_splits_arg(p, pargs, opcode_array_obj(objects, 0), j))
        return csound->InitError(csound,
                 Str("opcode object %d has a different signature from "
                     "object 0\n"), i);
  // check that array args split across obj[] are 1-dim and large enough
  for(i = 0; i < argn; i++)
    if(oprun_splits_arg(p, pargs, opcode_array_obj(objects, 0), i)) {
      array = (ARRAYDAT *) pargs[i];
      if(array->dimensions > 1)
        return csound->InitError(csound, Str("only 1-dim arrays are allowed\n"));
      if (array->dimensions == 0 || n > array->sizes[0]) {
        if (UNLIKELY(tabinit(csound, array, n, p->h.insdshead) != OK))
          return csound_array_init_resize_error(csound);
      }
  }
  oprun_alloc_slots(csound, p, pargs, opcode_array_obj(objects, 0),
                    n, argn);
  slot = (CS_VAR_MEM **) p->mem.auxp;
  for(i = 0; i < n; i++) {
    size_t size;
    obj = opcode_array_obj(objects, i);
    set_line_num_and_loc(obj->dataspace, p);
    if(context_check(csound, obj->dataspace, p->h.insdshead) != OK)
      return csound->InitError(csound, Str("incompatible context, "
                               "cannot initialise opcode obj for %s\n"),
                               obj->dataspace->optext->t.oentry->opname);
    for(j = 0; j < (int32_t) p->OUTOCOUNT; j++) {
      // if passed an array, check that the outype is not
      // an array first and deal with it 
      if((types[j] = csoundGetTypeForArg(pargs[j]))
          == &CS_VAR_TYPE_ARRAY &&
         !isTypeArray(obj,j, 0)) {
        /* Scalarize fixed-layout array elements into per-object arguments.
           Managed elements cannot be transferred safely as raw bytes. */
        array = (ARRAYDAT *)pargs[j];
        if (UNLIKELY(csound_array_has_managed_elements(array))) {
          return csound->InitError(
            csound,
            Str("Opcode[] run does not support managed array output elements\n"));
        }
        size = array->arrayMemberSize;
        ndx = i + j*n;
        argmem = slot[ndx];
        types[j] = array->arrayType;
        argmem->varType = types[j];
        args[j] = &argmem->value;
      } else // single var
         args[j] = pargs[j];
    }
    for(j = 0; j < (int32_t) p->INOCOUNT - 1; j++) {
      // skip the obj argument
      m = j + p->OUTOCOUNT + 1;
      // if passed an array, check that the intype is not
      // an array first and deal with it  
      if((types[m] = csoundGetTypeForArg(pargs[m]))
         == &CS_VAR_TYPE_ARRAY &&
         !isTypeArray(obj,j, 1)) {
        char *data;
        array = (ARRAYDAT *)pargs[m];
        if (UNLIKELY(csound_array_has_managed_elements(array))) {
          return csound->InitError(
            csound,
            Str("Opcode[] run does not support managed array input elements\n"));
        }
        ndx = i + n*m;
        size = array->arrayMemberSize;
        argmem = slot[ndx];
        data = (char *)array->data;
        types[m] = array->arrayType;
        argmem->varType = types[m];
        args[m] = &argmem->value;
        // copy array args data in - but not k or a vars
        if(types[m] != &CS_VAR_TYPE_K &&
           types[m] != &CS_VAR_TYPE_A)
          memcpy(&argmem->value, data+i*size, size);
      } else // single var
        args[m] = pargs[m];
     }
   
    // call setup_args with array flag checked, passing assigned args
    if(setup_args(csound, obj, &(p->h), args, types,
                   p->OUTOCOUNT, p->INOCOUNT - 1, NULL) == OK) {
      opcode_dataspace_free_setp(csound, obj->dataspace);
      if(obj->dataspace->init != NULL) {
        int32_t initResult = obj->dataspace->init(csound, obj->dataspace);
        if(initResult == OK) opcode_dataspace_mark_initialized(obj->dataspace);
        // copy array args data out
        for(j = 0; j < GetOutputArgCnt(&p->h); j++) {
           if(csoundGetTypeForArg(pargs[j]) == &CS_VAR_TYPE_ARRAY) {
            array = (ARRAYDAT *)  pargs[j]; // each inarg is an array
            size = array->arrayMemberSize;
            ndx = i + j*n;
            char *data = (char *) array->data; // copy loc pointer to args
            argmem = slot[ndx];
            // unallocated: the opcode takes this array whole
            if (argmem == NULL) continue;
            // copy array args data out - but not k or a vars
           if(array->arrayType != &CS_VAR_TYPE_K &&
             array->arrayType != &CS_VAR_TYPE_A) 
             memcpy(data+i*size, &argmem->value, size); 
             }
        }
      }
    } else return csound->InitError(csound, Str("mismatching arguments\n"
                                    "for opcode obj %s\t"
                                    "outypes: %s\tintypes: %s\n"),
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
  cs_float **pargs = oprun_args(p);
  ARRAYDAT  *objects = (ARRAYDAT *) pargs[p->OUTOCOUNT], *array;
  int32_t   n = objects->sizes[0], i, j, ndx, argn = p->OUTOCOUNT + p->INOCOUNT;
  OPCODEOBJ *obj;
  CS_VAR_MEM **slot = (CS_VAR_MEM **) p->mem.auxp;
  CS_VAR_MEM *argmem = NULL;
  char *data;

  // init made the slot table for the count of objects it saw, which
  // create may have changed since: the slots are found by that count, and
  // objects past it have none for an array argument
  int32_t count = (int32_t) (p->mem.size / (argn * sizeof(CS_VAR_MEM *)));
  if(UNLIKELY(n > count))
    for(j = 0; j < argn; j++)
      if(j != (int32_t) p->OUTOCOUNT &&
         csoundGetTypeForArg(pargs[j]) == &CS_VAR_TYPE_ARRAY)
        return csound->PerfError(csound, &(p->h),
                                 Str("opcode object array resized since init\n"));
  for(i = 0; i < n; i++) {
   size_t size;
  // copy array args data in
  for(j = p->OUTOCOUNT + 1; j < argn; j++) {
    if(csoundGetTypeForArg(pargs[j]) == &CS_VAR_TYPE_ARRAY) {
      array = (ARRAYDAT *)  pargs[j]; // each inarg is an array
      size = array->arrayMemberSize;
      data = (char *) array->data; // copy loc pointer to args
      ndx = i + j*count;
      argmem = slot[ndx];
      // unallocated: the opcode takes this array whole
      if (argmem == NULL) continue;
      // only perf-time data
      if(array->arrayType != &CS_VAR_TYPE_I)
       memcpy(&argmem->value, data+i*size, size);
    }
  }

  obj = opcode_array_obj(objects, i);
  // no call has bound the dataspace: create made it after this init, or
  // delete in another note took it away
  if(UNLIKELY(obj->dataspace == NULL ||
              obj->dataspace->optext->t.outlist == NULL))
    return csound->PerfError(csound, &(p->h),
                             Str("opcode object not initialised\n"));
  {
    OPCODE_SNAPSHOT snap;
    int32_t runResult;
    if(opcode_dataspace_acquire_run(csound, obj, &snap) != OK)
      return csound->PerfError(csound, &(p->h),
                               Str("opcode object not initialised\n"));
    if(opcode_dataspace_ended(snap.dataspace, snap.udo_flag)) {
      opcode_dataspace_release_run(csound, obj, &snap);
      return csound->PerfError(csound, &(p->h),
                               Str("opcode object UDO instance has ended\n"));
    }
    set_line_num_and_loc(snap.dataspace, p);
    runResult = snap.dataspace->perf != NULL
      ? snap.dataspace->perf(csound, snap.dataspace) : OK;
    opcode_dataspace_release_run(csound, obj, &snap);
    if(runResult != OK) return runResult;
  }

   // copy array args data out
   for(j = 0; j < GetOutputArgCnt(&p->h); j++) {
     if(csoundGetTypeForArg(pargs[j]) == &CS_VAR_TYPE_ARRAY) {
       array = (ARRAYDAT *)  pargs[j]; // each inarg is an array
       size = array->arrayMemberSize;
       ndx = i + j*count;
       data = (char *) array->data; // copy loc pointer to args
       argmem = slot[ndx];
       // unallocated: the opcode takes this array whole
       if (argmem == NULL) continue;
       // only perf-time data
       if(array->arrayType != &CS_VAR_TYPE_I)
          memcpy(data+i*size, &argmem->value, size);
     }
     } 
  }
  return OK;
}


int32_t copy_opcode_obj(CSOUND *csound, ASSIGN *p) {
  OPCODEOBJ *dst = (OPCODEOBJ *) p->r;
  OPCODEOBJ *src = (OPCODEOBJ *) p->a;
  if(dst == NULL || src == NULL) return OK;
  if(OPCODE_ATOMIC_LOAD_PTR(src->dataspace) != NULL && opcode_udo_ended(src)) {
    if(csound->mode == 1)
      return csound->InitError(csound,
                               Str("opcode object UDO instance has ended\n"));
    return csound->PerfError(csound, &(p->h),
                             Str("opcode object UDO instance has ended\n"));
  }
  if(OPCODE_ATOMIC_LOAD_PTR(src->dataspace) != NULL &&
     context_check(csound,
                   (OPDS *) OPCODE_ATOMIC_LOAD_PTR(src->dataspace),
                   p->h.insdshead) != OK) {
    if(csound->mode == 1)
      return csound->InitError(csound, Str("could not copy opcode object: "
                                           "incompatible context\n"));
    return csound->PerfError(csound, &(p->h),
                             Str("could not copy opcode object: "
                                 "incompatible context\n"));
  }
  if(dst->readonly) {
    csound->Warning(csound, Str("opcode instance var is read-only:"
                                " copy value bypassed\n"));
    return OK;
  }
  CS_VAR_TYPE_OPCODEOBJ.copyValue(csound, &CS_VAR_TYPE_OPCODEOBJ, dst,
                                  src, p->h.insdshead);
  return OK;
}

int32_t set_opcode_param(CSOUND *csound, AOP *p) {
  OPCODEOBJ *obj = (OPCODEOBJ *) p->r;
  cs_float index = *p->a;
  uint32_t ndx;
  cs_float *arg  = p->b;
  cs_float *previousArg = NULL;
  uint32_t slots;
  OPCODE_SNAPSHOT snap;
  int32_t result;
  // Defensive: object not initialised yet (e.g., wrong overload picked or missing 'create')
  if (UNLIKELY(obj == NULL || obj->dataspace == NULL)) {
    return csound->PerfError(csound, &(p->h), Str("opcode object not initialised (setp)\n"));
  }
  // a negative index selects the first input; a NaN is none, and no
  // opcode has VARGMAX inputs: refused before it is converted, as a
  // larger index might not convert
  if(index < 0) index = 0;
  if(UNLIKELY(index != index || index >= VARGMAX))
    return csound->PerfError(csound, &(p->h), Str("argument index out of range\n"));
  if(opcode_dataspace_is_running(
       (OPDS *) OPCODE_ATOMIC_LOAD_PTR(obj->dataspace)))
    return csound->PerfError(csound, &(p->h),
                             Str("cannot set an opcode object from its own run\n"));
  if(opcode_dataspace_acquire_run(csound, obj, &snap) != OK)
    return csound->PerfError(csound, &(p->h),
                             Str("opcode object not initialised (setp)\n"));
  if(opcode_dataspace_ended(snap.dataspace, snap.udo_flag)) {
    result = csound->PerfError(csound, &(p->h),
                               Str("opcode object UDO instance has ended\n"));
    goto done;
  }
  if(context_check(csound, snap.dataspace, p->h.insdshead) != OK) {
    result = csound->PerfError(csound, &(p->h),
                               Str("incompatible context for opcode %s \n"),
                               snap.dataspace->optext->t.oentry->opname);
    goto done;
  }
  ndx = (uint32_t) index;
  slots = input_slots(snap.dataspace->optext->t.oentry->intypes,
                      snap.dataspace->optext->t.inArgCount);
  if(snap.inargp != NULL && ndx < slots) previousArg = snap.inargp[ndx];
  if(check_and_set_arg(csound, &snap, ndx, arg) != 0) {
    result = csound->PerfError(csound, &(p->h), Str("could not set arg %u \n"), ndx);
    goto done;
  }
  if(opcode_dataspace_record_setp(csound, snap.dataspace, ndx, arg,
                                  p->h.insdshead) != OK) {
    if(previousArg != NULL && snap.inargp != NULL && ndx < slots)
      snap.inargp[ndx] = previousArg;
    opcode_dataspace_restore_setp(snap.dataspace, snap.inargp);
    result = csound->PerfError(csound, &(p->h), Str("could not set arg %u \n"), ndx);
    goto done;
  }
  opcode_dataspace_restore_setp(snap.dataspace, snap.inargp);
  if(snap.udo_flag &&
     user_opcode_rebind_args(csound, (UOPCODE *) snap.dataspace) != OK) {
    result = csound->PerfError(csound, &(p->h),
                               Str("could not rebind arguments "
                                   "for opcode obj %s\n"),
                               snap.dataspace->optext->t.oentry->opname);
    goto done;
  }
  result = OK;
done:
  opcode_dataspace_release_run(csound, obj, &snap);
  return result;
}

static int32_t copy_opcode_output(CSOUND *csound, AOP *p,
                                  int32_t initializing) {
  OPCODEOBJ *obj = (OPCODEOBJ *) p->a;
  OPDS *dataspace;
  cs_float index = *p->b;
  uint32_t ndx;
  cs_float **outargs;
  const CS_TYPE *destinationType;
  const CS_TYPE *sourceType;
  int32_t result;

  dataspace = obj != NULL
    ? (OPDS *) OPCODE_ATOMIC_LOAD_PTR(obj->dataspace) : NULL;
  if (UNLIKELY(dataspace == NULL)) {
    return initializing
      ? csound->InitError(csound, Str("object not initialised\n"))
      : csound->PerfError(csound, &p->h, Str("object not initialised\n"));
  }
  outargs = (cs_float **) OPCODE_ATOMIC_LOAD_PTR(obj->outargp);
  if (UNLIKELY(outargs == NULL)) {
    /* A perf-only Opcode object binds its argument pointers on the first
       performance call. Defer getp validation until that call has run. */
    OENTRY *entry = dataspace->optext != NULL
      ? dataspace->optext->t.oentry : NULL;
    if (initializing && entry != NULL && entry->init == NULL &&
        entry->perf != NULL) {
      return OK;
    }
    return initializing
      ? csound->InitError(csound, Str("object not initialised\n"))
      : csound->PerfError(csound, &p->h,
                          Str("object not initialised\n"));
  }
  if (UNLIKELY(context_check(csound, dataspace, p->h.insdshead) != OK)) {
    return initializing
      ? csound->InitError(csound, Str("incompatible context for opcode %s\n"),
                          dataspace->optext->t.oentry->opname)
      : csound->PerfError(csound, &p->h,
                          Str("incompatible context for opcode %s\n"),
                          dataspace->optext->t.oentry->opname);
  }
  // a negative index selects the first output, as the manual says; a NaN
  // is none, and a larger index is refused before it is converted
  if (index < 0) index = 0;
  if (UNLIKELY(index != index ||
               index >= dataspace->optext->t.outArgCount)) {
    return initializing
      ? csound->InitError(csound, Str("argument index out of range\n"))
      : csound->PerfError(csound, &p->h, Str("argument index out of range\n"));
  }
  ndx = (uint32_t) index;

  destinationType = csoundGetTypeForArg(p->r);
  sourceType = csoundGetTypeForArg(outargs[ndx]);
  // a p-field holds an i, as an output written into one is
  if (sourceType == &CS_VAR_TYPE_P) sourceType = &CS_VAR_TYPE_I;
  if (destinationType == &CS_VAR_TYPE_P) destinationType = &CS_VAR_TYPE_I;
  if (UNLIKELY(sourceType == NULL || destinationType == NULL)) {
    return initializing
      ? csound->InitError(csound, Str("could not resolve getp argument types\n"))
      : csound->PerfError(csound, &p->h,
                          Str("could not resolve getp argument types\n"));
  }
  if (UNLIKELY(sourceType != destinationType)) {
    return initializing
      ? csound->InitError(csound, Str("mismatching argument types: need %s, got %s\n"),
                          sourceType->varTypeName,
                          destinationType->varTypeName)
      : csound->PerfError(csound, &p->h,
                          Str("mismatching argument types: need %s, got %s\n"),
                          sourceType->varTypeName,
                          destinationType->varTypeName);
  }

  if (destinationType == &CS_VAR_TYPE_ARRAY) {
    ARRAYDAT *destination = (ARRAYDAT *)p->r;
    ARRAYDAT *source = (ARRAYDAT *)outargs[ndx];
    if (UNLIKELY(csound_array_has_managed_elements(source))) {
      return initializing
        ? csound->InitError(
            csound, Str("getp does not support managed array outputs\n"))
        : csound->PerfError(
            csound, &p->h,
            Str("getp does not support managed array outputs\n"));
    }
    result = csound_array_copy_independent(
      csound, destination, source, p->h.insdshead,
      initializing ? CSOUND_ARRAY_COPY_ALLOW_ALLOCATION
                   : CSOUND_ARRAY_COPY_NO_ALLOCATION);
    if (UNLIKELY(result != OK)) {
      return initializing
        ? csound->InitError(csound,
                            Str("could not prepare getp array output\n"))
        : csound->PerfError(
            csound, &p->h,
            Str("getp array output changed capacity during performance\n"));
    }
    return OK;
  }
  if (UNLIKELY(destinationType->userDefinedType ||
               destinationType->freeVariableMemory != NULL ||
               destinationType->copyValue == NULL)) {
    return initializing
      ? csound->InitError(csound,
                          Str("getp does not support managed outputs\n"))
      : csound->PerfError(csound, &p->h,
                          Str("getp does not support managed outputs\n"));
  }
  destinationType->copyValue(csound, destinationType, p->r,
                             outargs[ndx], p->h.insdshead);
  return OK;
}

int32_t get_opcode_output_init(CSOUND *csound, AOP *p) {
  return copy_opcode_output(csound, p, 1);
}

int32_t get_opcode_output(CSOUND *csound, AOP *p) {
  return copy_opcode_output(csound, p, 0);
}
