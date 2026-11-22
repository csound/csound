/*
  csound_orc_structs.c:

  Copyright (C) 2025
  John ffitch, Steven Yi, Hlöðver Sigurðsson

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
#include "csound_orc.h"
#include "csound_orc_structs.h"
#include "csound_type_system.h"
#include "csound_orc_semantics.h"
#include "csound_standard_types.h"
#include "csound_orc_compile.h"
#include "arrays_internal.h"
#include "aops.h"
#include "entry.h"

static int find_struct_member_entry(CONS_CELL* members, const char* memberName,
                                    CS_VARIABLE** outMember) {
    int i = 0;
    while (members != NULL) {
        CS_VARIABLE* member = (CS_VARIABLE*)members->value;
        if (!strcmp(member->varName, memberName)) {
            if (outMember != NULL) {
                *outMember = member;
            }
            return i;
        }
        i++;
        members = members->next;
    }
    return -1;
}

int findStructMemberIndex(CONS_CELL* members, char* memberName) {
    return find_struct_member_entry(members, memberName, NULL);
}

CS_VARIABLE* getStructMember(CONS_CELL* members, char* memberName) {
    CS_VARIABLE* member = NULL;
    find_struct_member_entry(members, memberName, &member);
    return member;
}

typedef struct csound_type_path {
  const CS_TYPE *type;
  const struct csound_type_path *parent;
} CSOUND_TYPE_PATH;

static int32_t type_contains_type(const CS_TYPE *type,
                                  const CS_TYPE *target,
                                  const CSOUND_TYPE_PATH *path);

static int32_t variable_contains_type(const CS_VARIABLE *var,
                                      const CS_TYPE *target,
                                      const CSOUND_TYPE_PATH *path) {
  if (var == NULL || var->varType == NULL || target == NULL)
    return 0;
  if (var->varType == target)
    return 1;
  return type_contains_type(var->varType == &CS_VAR_TYPE_ARRAY
                              ? var->subType : var->varType,
                            target, path);
}

static int32_t type_contains_type(const CS_TYPE *type,
                                  const CS_TYPE *target,
                                  const CSOUND_TYPE_PATH *path) {
  const CSOUND_TYPE_PATH *current = path;
  const CONS_CELL *members;

  if (type == NULL || target == NULL)
    return 0;
  if (type == target)
    return 1;
  if (!type->userDefinedType)
    return 0;
  while (current != NULL) {
    if (current->type == type)
      return 0;
    current = current->parent;
  }

  CSOUND_TYPE_PATH next = { type, path };
  members = type->members;
  while (members != NULL) {
    if (variable_contains_type((const CS_VARIABLE *) members->value,
                               target, &next))
      return 1;
    members = members->next;
  }
  return 0;
}

int32_t csound_variable_contains_type(const CS_VARIABLE *var,
                                      const CS_TYPE *target) {
  return variable_contains_type(var, target, NULL);
}

/* Deep-free struct members if this instance owns them.
   Safe to call on aliases; does nothing if ownsMembers==0 or members==NULL. */
void csound_free_struct_members(CSOUND *csound, CS_STRUCT_VAR *var) {
  if (var == NULL || var->members == NULL || var->ownsMembers == 0) {
    return;
  }

  int32_t i;
  for (i = 0; i < var->memberCount; i++) {
    CS_VAR_MEM *mem = var->members[i];
    if (mem != NULL) {
      if (mem->varType && mem->varType->freeVariableMemory) {
        mem->varType->freeVariableMemory(csound, &mem->value);
      }
      if (var->ownsMembers != CSOUND_STRUCT_MEMBERS_POOLED) {
        csound->Free(csound, mem);
      }
      var->members[i] = NULL;  // Prevent double-free
    }
  }
  csound->Free(csound, var->members);
  var->members = NULL;
  var->memberCount = 0;
  var->ownsMembers = 0;
}

void freeStructVarMemory(void *csnd, void *p) {
  CSOUND *csound = (CSOUND *)csnd;
  CS_STRUCT_VAR *var = (CS_STRUCT_VAR *)p;
  csound_free_struct_members(csound, var);
}

/* Names are bounded by the type lookup/signature format used by the compiler. */
static int struct_identifier(const char *name)
{
  int first = 1;
  if (name == NULL || *name == '\0') return 0;
  for (const unsigned char *p = (const unsigned char *)name; *p; ++p) {
    if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
          *p == '_' || (!first && *p >= '0' && *p <= '9'))) return 0;
    first = 0;
  }
  return !first;
}

const CS_TYPE *csoundRegisterStruct(CSOUND *csound, const char *name,
                                    const CSOUND_STRUCT_MEMBER *members,
                                    size_t count)
{
  if (csound == NULL || !struct_identifier(name) || strlen(name) > 250 ||
      members == NULL || count == 0 || count > VARGMAX ||
      csoundGetTypeWithVarTypeName(csound->typePool, name) != NULL)
    return NULL;
  for (size_t i = 0; i < count; ++i) {
    if (!struct_identifier(members[i].name) ||
        strlen(members[i].name) > 250 || members[i].type == NULL)
      return NULL;
    for (size_t j = 0; j < i; ++j)
      if (!strcmp(members[i].name, members[j].name)) return NULL;
    size_t len = strlen(members[i].type);
    int array = len > 2 && !strcmp(members[i].type + len - 2, "[]");
    size_t base_len = len - (array ? 2u : 0u);
    if (base_len == 0 || base_len > 250) return NULL;
    char base[251];
    memcpy(base, members[i].type, base_len);
    base[base_len] = '\0';
    if (!struct_identifier(base)) return NULL;
    const CS_TYPE *type = csoundGetTypeWithVarTypeName(csound->typePool, base);
    if (type == NULL && !(array && !strcmp(base, name))) return NULL;
    if (type != NULL && (type->createVariable == NULL ||
                         type->copyValue == NULL)) return NULL;
    if (type != NULL) {
      ARRAY_VAR_INIT init = {1, type};
      CS_VARIABLE *probe = csoundCreateVariableForType(
        csound, array ? &CS_VAR_TYPE_ARRAY : type, array ? &init : NULL, NULL);
      if (probe == NULL) return NULL;
      int valid = probe->initializeVariableMemory != NULL;
      csound->Free(csound, probe);
      if (!valid) return NULL;
    }
  }

  /* Reuse the orchestra's constructors, ownership rules and init overloads. */
  TREE definition = {0}, name_tree = {0};
  ORCTOKEN name_token = {0};
  name_token.lexeme = (char *)name;
  name_tree.value = &name_token;
  definition.left = &name_tree;
  TREE *fields = csound->Calloc(csound, count * sizeof(TREE));
  ORCTOKEN *tokens = csound->Calloc(csound, count * sizeof(ORCTOKEN));
  for (size_t i = 0; i < count; ++i) {
    tokens[i].lexeme = (char *)members[i].name;
    tokens[i].optype = (char *)members[i].type;
    fields[i].value = &tokens[i];
    fields[i].next = i + 1 < count ? &fields[i + 1] : NULL;
  }
  definition.right = fields;
  int32_t result = add_struct_definition(csound, &definition);
  csound->Free(csound, tokens);
  csound->Free(csound, fields);
  if (!result) return NULL;
  CS_TYPE *type = (CS_TYPE *)csoundGetTypeWithVarTypeName(csound->typePool, name);
  type->varDescription = "plugin-defined struct";
  type->userDefinedType |= CS_TYPE_PLUGIN_DEFINED;
  return type;
}

/* Look up a type by its exact internal name (":Name;"), without the quote
   stripping that csoundGetTypeWithVarTypeName() applies. That fallback makes
   the lookup ":Name;" also match a builtin type called "Name", so a struct
   named after a builtin type would resolve to the builtin. Builtin types are
   registered from const storage, so writing through such a result crashes.
   Struct types are only ever registered under their internal name, so
   requiring an exact match is enough to tell them apart. */
static const CS_TYPE* get_struct_type_by_internal_name(const TYPE_POOL* pool,
                                                       const char* internalName) {
  const CS_TYPE_ITEM* current = pool->head;
  while (current != NULL) {
    if (strcmp(internalName, current->cstype->varTypeName) == 0)
      return current->cstype;
    current = current->next;
  }
  return NULL;
}

int32_t initStructVar(CSOUND* csound, void* p) {
  INIT_STRUCT_VAR* init = (INIT_STRUCT_VAR*)p;
  CS_STRUCT_VAR* structVar = (CS_STRUCT_VAR*)init->out;
  const CS_TYPE* type = csoundGetTypeForArg(init->out);
  int32_t len = cs_cons_length(type->members);
  int32_t incnt = (int32_t)init->h.optext->t.inArgCount;
  int32_t i;
  if(csoundGetDebug(csound) & DEBUG_SEMANTICS) {
     csound->Message(csound, "Initializing Struct...\n");
     csound->Message(csound, "Struct Type: %s\n", type->varTypeName);
  }
  if (incnt == 0) {
    return CSOUND_SUCCESS;
  }
  for (i = 0; i < len; i++) {
    CS_VAR_MEM* mem = structVar->members[i];
    mem->varType->copyValue(csound, mem->varType, &mem->value,
                            init->inArgs[i], init->h.insdshead);
  }

  return CSOUND_SUCCESS;
}

static size_t align_struct_member(size_t size) {
  const size_t alignment = sizeof(void *) > sizeof(cs_float)
                            ? sizeof(void *) : sizeof(cs_float);
  return (size + alignment - 1) & ~(alignment - 1);
}

void initializeStructVar(CSOUND* csound, CS_VARIABLE* var, cs_float* mem) {
  CS_STRUCT_VAR* structVar = (CS_STRUCT_VAR*)mem;
  const CS_TYPE* type = var->varType;
  CONS_CELL* members = type->members;

  int32_t len = cs_cons_length(members);
  int32_t i;
  size_t offset = align_struct_member(len * sizeof(CS_VAR_MEM *));
  size_t size = offset;

  for (CONS_CELL *member = members; member != NULL; member = member->next) {
    CS_VARIABLE *field = member->value;
    /* Audio fields were sized before the orchestra header set ksmps. */
    if (field->updateMemBlockSize != NULL) {
      field->updateMemBlockSize(csound, field);
    }
    size += align_struct_member(CS_VAR_TYPE_OFFSET + field->memBlockSize);
  }
  structVar->members = csound->Calloc(csound, size);
  structVar->memberCount = len;  // Set the member count
  structVar->ownsMembers = CSOUND_STRUCT_MEMBERS_POOLED;
  if(csoundGetDebug(csound) & DEBUG_SEMANTICS) {
      csound->Message(csound, "Initializing Struct...\n");
      csound->Message(csound, "Struct Type: %s\n", type->varTypeName);
  }
  for (i = 0; i < len; i++) {
    CS_VARIABLE* memberVar = members->value;
    CS_VAR_MEM* memberMem = (CS_VAR_MEM *)((char *)structVar->members + offset);
    offset += align_struct_member(CS_VAR_TYPE_OFFSET + memberVar->memBlockSize);
    memberMem->varType = memberVar->varType;
    structVar->members[i] = memberMem;
    members = members->next;
  }

  /* Finish the layout before callbacks initialize nested values. */
  members = type->members;
  for (i = 0; i < len; i++, members = members->next) {
    CS_VARIABLE *field = members->value;
    if (field->initializeVariableMemory != NULL) {
      field->initializeVariableMemory(csound, field,
                                      &structVar->members[i]->value);
    }
  }
}

CS_VARIABLE* createStructVar(void* cs, const CS_TYPE* type,
                             const void* typeArg, INSDS *ctx) {
  CSOUND* csound = (CSOUND*)cs;
  IGN(typeArg);

  if (type == NULL) {
    csound->Message(csound, Str("ERROR: no type given for struct creation\n"));
    return NULL;
  }

  CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
  /* Array storage advances in cs_float units. On wasm32 CS_STRUCT_VAR is 12
     bytes, so leaving this unaligned would truncate the element stride. */
  var->memBlockSize = CS_FLOAT_ALIGN(sizeof(CS_STRUCT_VAR));
  var->initializeVariableMemory = initializeStructVar;
  var->ctx = ctx;

  //FIXME - implement
  return var;
}

int32_t csound_copy_struct_value(CSOUND *csound,
                                 const CS_TYPE *structType,
                                 void *dest, const void *src, INSDS *ctx,
                                 CSOUND_STRUCT_COPY_MODE mode) {
  CS_STRUCT_VAR* varDest = (CS_STRUCT_VAR*)dest;
  const CS_STRUCT_VAR* varSrc = (const CS_STRUCT_VAR*)src;
  int32_t i, count;

  if (dest == src) {
    return OK;
  }

  if (structType == NULL || varDest == NULL || varSrc == NULL ||
      varDest->members == NULL || varSrc->members == NULL) {
    return NOTOK;
  }

  count = cs_cons_length(structType->members);
  if (count != varDest->memberCount || count != varSrc->memberCount) {
    return NOTOK;
  }
  for (i = 0; i < count; i++) {
    CS_VAR_MEM* d = varDest->members[i];
    CS_VAR_MEM* s = varSrc->members[i];
    if (d == NULL || s == NULL || d->varType == NULL ||
        d->varType != s->varType) {
      return NOTOK;
    }
    if (d == s) {
      continue;
    }
    if (mode != CSOUND_STRUCT_COPY_SHARED_ARRAYS &&
        d->varType == &CS_VAR_TYPE_ARRAY) {
      if (csound_array_copy_independent(
            csound, (ARRAYDAT *)&d->value,
            (const ARRAYDAT *)&s->value, ctx,
            mode == CSOUND_STRUCT_COPY_INDEPENDENT_ALLOW_ALLOCATION
              ? CSOUND_ARRAY_COPY_ALLOW_ALLOCATION
              : CSOUND_ARRAY_COPY_NO_ALLOCATION) != OK) {
        return NOTOK;
      }
    }
    else if (mode != CSOUND_STRUCT_COPY_SHARED_ARRAYS &&
             d->varType->userDefinedType) {
      if (csound_copy_struct_value(csound, d->varType,
                                   &d->value, &s->value, ctx,
                                   mode) != OK) {
        return NOTOK;
      }
    }
    else if (d->varType->copyValue != NULL) {
      d->varType->copyValue(csound, d->varType,
                            &d->value, &s->value, ctx);
    }
    else {
      return NOTOK;
    }
  }
  return OK;
}

void copyStructVar(CSOUND* csound, const CS_TYPE* structType, void* dest,
                   const void* src, INSDS *p) {
  if (UNLIKELY(csound_copy_struct_value(
                 csound, structType, dest, src, p,
                 CSOUND_STRUCT_COPY_SHARED_ARRAYS) != OK)) {
    csound->Message(csound, Str("struct not initialised - cannot copy\n"));
  }
}

// Phase 1: Register struct name as placeholder type (for recursive references)
static int32_t register_struct_placeholder(CSOUND *csound, TREE *structDefTree) {
  if (!structDefTree || !structDefTree->left || !structDefTree->left->value) {
    return 0;
  }

  char *structName = structDefTree->left->value->lexeme;

  // Create internal name format for consistency with Phase 2 lookup
  size_t nameLen = strlen(structName);
  char *internalName = csound->Malloc(csound, nameLen + 3);
  internalName[0] = ':';
  strcpy(internalName + 1, structName);
  internalName[nameLen + 1] = ';';
  internalName[nameLen + 2] = '\0';

  // Check if struct type already exists (using internal name format)
  const CS_TYPE *existingType =
      get_struct_type_by_internal_name(csound->typePool, internalName);
  if (existingType != NULL) {
    csound->Free(csound, internalName);
    // Struct already registered, return success
    return 1;
  }

  // Create placeholder type with minimal information
  CS_TYPE *type = csound->Calloc(csound, sizeof(CS_TYPE));
  // Use the already-created internal name
  type->varTypeName = internalName;
  type->varDescription = "user-defined struct (placeholder)";
  type->argtype = CS_ARG_TYPE_BOTH;
  type->createVariable = createStructVar;
  type->copyValue = copyStructVar;
  type->freeVariableMemory = freeStructVarMemory;
  type->userDefinedType = CS_TYPE_USER_DEFINED;
  type->members = NULL; // Will be filled in Phase 2

  // Register the placeholder type
  if (!csoundAddVariableType(csound, csound->typePool, type)) {
    csound->Free(csound, type->varTypeName);
    csound->Free(csound, type);
    return 0;
  }

  return 1;
}

int32_t add_struct_definition(CSOUND* csound, TREE* structDefTree) {
  TREE* current = structDefTree->right;
  size_t index = 0;
  char *temp;
  CS_TYPE* type;

  // Create internal name format to check if placeholder already exists
  char *structName = structDefTree->left->value->lexeme;
  size_t nameLen = strlen(structName);
  char *internalName = csound->Malloc(csound, nameLen + 3);
  internalName[0] = ':';
  strcpy(internalName + 1, structName);
  internalName[nameLen + 1] = ';';
  internalName[nameLen + 2] = '\0';

  // Check if placeholder type already exists
  const CS_TYPE *placeholder =
      get_struct_type_by_internal_name(csound->typePool, internalName);

  if (placeholder != NULL) {
    // Update existing placeholder with member information
    csound->Free(csound, internalName);
    if (placeholder->userDefinedType != CS_TYPE_USER_DEFINED)
      return 0;
    /* This file allocates orchestra struct descriptors as writable objects. */
    type = (CS_TYPE *)placeholder;
    type->varDescription = "user-defined struct";
  } else {
    // Create new type if no placeholder exists (for backward compatibility)
    type = csound->Calloc(csound, sizeof(CS_TYPE));
    type->varTypeName = internalName;
    type->varDescription = "user-defined struct";
    type->argtype = CS_ARG_TYPE_BOTH;
    type->createVariable = createStructVar;
    type->copyValue = copyStructVar;
    type->freeVariableMemory = freeStructVarMemory;
    type->userDefinedType = CS_TYPE_USER_DEFINED;
  }

  if (get_struct_type_by_internal_name(csound->typePool, type->varTypeName)
      == NULL)
    csoundAddVariableType(csound, csound->typePool, type);

  // Clear existing members if updating placeholder
  if (type->members != NULL) {
    // Free existing member list (they're just empty placeholders)
    type->members = NULL;
  }

  // FIXME: Values are appended in reverse order of definition
  while (current != NULL) {
    const char* memberName = current->value->lexeme;
    const char* annotation = current->value->optype;
    char* inferredAnnotation = NULL;

    if (annotation == NULL) {
      char elementType[] = { memberName[0], '\0' };
      int32_t dimensions = current->type == T_ARRAY_IDENT
        ? tree_arg_list_count(current->right) : 0;
      inferredAnnotation = csoundFormatTypeName(csound, elementType, dimensions);
      annotation = inferredAnnotation;
    }

    ARRAY_VAR_INIT varInit;
    const void* typeArg;
    const CS_TYPE* memberType =
      resolve_type_annotation(csound, annotation, &varInit, &typeArg);
    CS_VARIABLE* var =
      csoundCreateVariableForType(csound, memberType, typeArg, NULL);

    if (UNLIKELY(var == NULL)) {
      csound->ErrorMsg(csound,
                       Str("unknown type '%s' for member '%s' of struct\n"),
                       annotation, memberName);
      csound->Free(csound, inferredAnnotation);
      return 0;
    }
    csound->Free(csound, inferredAnnotation);

    var->varName = csoundStrdup(csound, memberName);
    CONS_CELL* member = csound->Calloc(csound, sizeof(CONS_CELL));
    member->value = var;
    type->members = cs_cons_append(type->members, member);
    current = current->next;
  }

  // Only add to type pool if it's a new type (not updating placeholder)
  const CS_TYPE* existingType =
      get_struct_type_by_internal_name(csound->typePool, type->varTypeName);
  if(existingType == NULL) {
    if(!csoundAddVariableType(csound, csound->typePool, type)) {
      return 0;
    }
  }

  OENTRY oentry;
  memset(&oentry, 0, sizeof(OENTRY));
  size_t signature_size = 1;
  for (CONS_CELL *cell = type->members; cell != NULL; cell = cell->next) {
    CS_VARIABLE *field = cell->value;
    signature_size += strlen(field->subType != NULL
      ? field->subType->varTypeName : field->varType->varTypeName) + 4;
  }
  size_t temp_size = signature_size + strlen(type->varTypeName) + 8;
  temp = csound->Calloc(csound, temp_size);

  // Extract plain struct name from internal format (:name;) for opcode name
  char* plainName = type->varTypeName + 1;  // Skip leading ':'
  size_t plainNameLen = strlen(plainName) - 1;  // Exclude trailing ';'

  csoundSprintf(temp, "init.");
  strncat(temp, plainName, plainNameLen);
  oentry.opname = csoundStrdup(csound, temp);
  oentry.dsblksiz = CS_FLOAT_ALIGN(
    offsetof(INIT_STRUCT_VAR, inArgs) +
    cs_cons_length(type->members) * sizeof(cs_float *));
  oentry.flags = 0;
  oentry.init = initStructVar;
  oentry.perf = NULL;
  oentry.deinit = NULL;
  oentry.useropinfo = NULL;

  /* FIXME - this is not yet implemented */
  memset(temp, 0, temp_size);
  csoundSprintf(temp, "%s", type->varTypeName);  // Use internal format for outypes
  oentry.outypes = csoundStrdup(csound, temp);

  CONS_CELL* member = type->members;
  while (member != NULL) {
    CS_VARIABLE* memberVar = (CS_VARIABLE*)member->value;
    int32_t isArray = (memberVar->subType != NULL);

    // For arrays, get the base type from subType, otherwise use varType
    char* memberTypeName = isArray ?
                           memberVar->subType->varTypeName :
                           memberVar->varType->varTypeName;
    int32_t len = (int32_t) strlen(memberTypeName);

    if (len == 1) {
      temp[index++] = *memberTypeName;
      if (isArray) {
        temp[index++] = '[';
        temp[index++] = ']';
      }
    } else if (memberTypeName[0] == ':') {
      // Already in internal format (:Type;), copy as-is
      memcpy(temp + index, memberTypeName, len);
      index += len;
      if (isArray) {
        temp[index++] = '[';
        temp[index++] = ']';
      }
    } else {
      // Plain type name, add internal format escaping
      temp[index++] = ':';
      memcpy(temp + index, memberTypeName, len);
      index += len;
      temp[index++] = ';';
      if (isArray) {
        temp[index++] = '[';
        temp[index++] = ']';
      }
    }

    member = member->next;
  }
  temp[index] = 0;
  oentry.intypes = csoundStrdup(csound, "");
  csoundAppendOpcodes(csound, &oentry, 1);
  oentry.opname = csoundStrdup(csound, oentry.opname);
  oentry.outypes = csoundStrdup(csound, oentry.outypes);
  oentry.intypes = csoundStrdup(csound, temp);
  csoundAppendOpcodes(csound, &oentry, 1);

  /* Register an exact copy constructor without shadowing member-based
     constructors for the same struct. */
  oentry.opname = csoundStrdup(csound, oentry.opname);
  oentry.outypes = csoundStrdup(csound, type->varTypeName);
  oentry.intypes = csoundStrdup(csound, type->varTypeName);
  oentry.dsblksiz = sizeof(ASSIGN);
  oentry.init = copy_var_generic_init;
  csoundAppendOpcodes(csound, &oentry, 1);
  csound->Free(csound, temp);
  return 1;
}

int32_t process_struct_definitions_two_phase(CSOUND *csound,
                                             TREE *structDefList) {
  if (structDefList == NULL) {
    return 1; // No struct definitions to process
  }

  // Phase 1: Register all struct names as placeholders
  TREE *current = structDefList;
  while (current != NULL) {
    if (current->type == STRUCT_TOKEN) {
      if (!register_struct_placeholder(csound, current)) {
        return 0; // Error in Phase 1
      }
    }
    current = current->next;
  }

  // Phase 2: Resolve all struct members and register opcodes
  current = structDefList;
  while (current != NULL) {
    if (current->type == STRUCT_TOKEN) {
      char* structName = current->left->value->lexeme;
      if (!add_struct_definition(csound, current)) {
        csound->ErrorMsg(csound, Str("[struct] Phase 2: ERROR processing struct '%s'\n"), structName ? structName : "(null)");
        return 0; // Error in Phase 2
      }
    }
    current = current->next;
  }

  return 1;
}
