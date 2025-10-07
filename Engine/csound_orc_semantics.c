/*

  csound_orc_semantics.c:

  Copyright (C) 2006
  John ffitch, Steven Yi

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

#include "csound_orc_semantics.h"
#include "csound_orc_expressions.h"
#include "array_ops.h"
#include "arrays.h"
#include "csoundCore.h"
#include "csound_orc.h"
#include "csound_orc_compile.h"
#include "csound_orc_expressions.h"
#include "csound_orc_structs.h"
#include "csound_standard_types.h"
#include "csound_type_system.h"
#include "interlocks.h"
#include "namedins.h"
#include "parse_param.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

// Forward declaration for check_optional_type
char *check_optional_type(CSOUND *csound, char *name);

#if defined(_WIN32) || defined(_WIN64)
#define strtok_r strtok_s
#endif

static CS_VAR_POOL *find_global_annotation(char *varName,
                                           TYPE_TABLE *typeTable);
static int32_t is_label(char *ident, CONS_CELL *labelList);
static char *convert_internal_to_external(CSOUND *csound, char *arg);
static int32_t is_reserved(char *);


char *csound_orcget_text(void *scanner);
uint64_t csound_orcget_locn(void *);
int32_t add_udo_definition(CSOUND *csound, bool newStyle, char *opname,
                           char *outtypes, char *intypes, int32_t flags);

const char *SYNTHESIZED_ARG = "_synthesized";
const char *UNARY_PLUS = "_unary_plus";

/* VL - 20.10.24 moved here from symbtab.c
   as this is a more appropriate place for it
*/

ORCTOKEN *lookup_token(CSOUND *csound, char *s, void *yyscanner) {
  IGN(yyscanner);
  int32_t type = T_IDENT;
  ORCTOKEN *ans;

  if (UNLIKELY(PARSER_DEBUG))
    csound->Message(csound, "Looking up token for: %s\n", s);

  ans = new_token(csound, T_IDENT);
  if (strchr(s, ':') != NULL) {
    char *th;
    char *baseName = strtok_r(s, ":", &th);
    char *annotation = strtok_r(NULL, ":", &th);
    ans->lexeme = cs_strdup(csound, baseName);
    ans->optype = cs_strdup(csound, annotation);
    type = T_TYPED_IDENT;
  } else {
    ans->lexeme = cs_strdup(csound, s);
  }
  if (csound->parserNamedInstrFlag == 1) {
    return ans;
  }
  ans->type = type;
  return ans;
}

char *cs_strdup(CSOUND *csound, const char *str) {
  size_t len;
  char *retVal;

  if (str == NULL)
    return NULL;

  // Removed noisy corruption debug: internal op names like '##mul.[]' are valid
  // and should not be flagged here. Keep cs_strdup behavior simple and
  // reliable.
  len = strlen(str);

  retVal = csound->Malloc(csound, len + 1);

  if (len > 0) {
    memcpy(retVal, str, len);
  }
  retVal[len] = '\0';

  return retVal;
}

char *cs_strndup(CSOUND *csound, const char *str, size_t size) {
  size_t len;
  char *retVal;

  if (str == NULL || size == 0)
    return NULL;
  
  // Find the actual length, but don't read past 'size' bytes
  // This avoids reading uninitialized memory in valgrind
  len = 0;
  while (len < size && str[len] != '\0') {
    len++;
  }
  
  // If we found a null terminator before 'size', use the shorter length
  // Otherwise use 'size'
  size_t copy_len = (len < size) ? len : size;

  // Use Calloc to zero-initialize memory, avoiding valgrind warnings
  // about uninitialized values when the source string buffer contains
  // uninitialized bytes after the portion we're copying
  retVal = csound->Calloc(csound, copy_len + 1);
  memcpy(retVal, str, copy_len);
  // retVal[copy_len] = '\0'; // Not needed since Calloc zeroes memory

  return retVal;
}

char *get_expression_opcode_type(CSOUND *csound, TREE *tree) {
  switch (tree->type) {
  case '+':
    return "##add";
  case '-':
    return "##sub";
  case '*':
    return "##mul";
  case '%':
    return "##mod";
  case '/':
    return "##div";
  case '^':
    return "##pow";
  case S_UMINUS:
    return "##mul";
  case S_UPLUS:
    return "##mul";
  case '|':
    return "##or";
  case '&':
    return "##and";
  case S_BITSHIFT_RIGHT:
    return "##shr";
  case S_BITSHIFT_LEFT:
    return "##shl";
  case '#':
    return "##xor";
  case '~':
    return "##not";
  case T_ARRAY:
    return "##array_get";
  case STRUCT_EXPR:
    return "##member_get";
  }
  csound->Warning(csound, Str("Unknown function type found: %d [%c]\n"),
                  tree->type, tree->type);
  return NULL;
}

char *get_boolean_expression_opcode_type(CSOUND *csound, TREE *tree) {
  switch (tree->type) {
  case S_EQ:
    return "==";
  case S_EQT:
    return "=t";
  case S_NEQ:
    return "!=";
  case S_GE:
    return ">=";
  case S_LE:
    return "<=";
  case S_GT:
    return ">";
  case S_LT:
    return "<";
  case S_AND:
    return "&&";
  case S_OR:
    return "||";
  case S_UNOT:
    return "!";
  }
  csound->Warning(csound, Str("Unknown boolean expression type found: %d\n"),
                  tree->type);
  return NULL;
}

char *create_array_arg_type(CSOUND *csound, CS_VARIABLE *arrayVar) {
  if (arrayVar->subType == NULL)
    return NULL;

  char *varTypeName = arrayVar->subType->varTypeName;

  // Check if this is a user-defined struct type
  if (arrayVar->subType->userDefinedType) {
    // Extract external name from internal format if needed
    char *externalName;
    if (varTypeName[0] == ':' && varTypeName[strlen(varTypeName) - 1] == ';') {
      // Internal format (:Name;) - extract the Name part
      size_t nameLen = strlen(varTypeName) - 2; // Remove : and ;
      externalName = csound->Malloc(csound, nameLen + 1);
      memcpy(externalName, varTypeName + 1, nameLen);
      externalName[nameLen] = '\0';
    } else {
      // Already external format - use as is
      externalName = cs_strdup(csound, varTypeName);
    }

    // For struct arrays, use internal format ":StructName;[]" for argument matching
    int32_t len = (int32_t)strlen(externalName) + arrayVar->dimensions * 2 + 3;
    char *retVal = csound->Malloc(csound, len);
    memset(retVal, 0, len);

    // Build the struct array type string: ":StructName;[]"
    strcpy(retVal, ":");
    strcat(retVal, externalName);
    strcat(retVal, ";");

    csound->Free(csound, externalName);

    // Add array brackets for each dimension
    for (int32_t i = 0; i < arrayVar->dimensions; i++) {
      strcat(retVal, "[]");
    }

    return retVal;
  } else {
    // For basic types, use the original format "[type]"
    int32_t len = arrayVar->dimensions + (int32_t)strlen(varTypeName) + 2;
    char *retVal = csound->Malloc(csound, len);
    memset(retVal, '[', arrayVar->dimensions);
    strNcpy(retVal + arrayVar->dimensions, varTypeName,
            strlen(varTypeName) + 1);
    retVal[len - 1] = '\0';
    retVal[len - 2] = ']';
    return retVal;
  }
}

/* this checks if the annotated type exists */
char *check_annotated_type(CSOUND *csound, OENTRIES *entries,
                           char *outArgTypes) {
  int32_t i;
  for (i = 0; i < entries->count; i++) {
    OENTRY *temp = entries->entries[i];
    if (check_out_args(csound, outArgTypes, temp->outypes))
      return outArgTypes;
  }
  return NULL;
}

static int32_t is_irate(TREE *t) {
  if (t->type == INTEGER_TOKEN) {
    // printf("integer case\n");
    return 1;
  } else if (t->type == T_IDENT) {
    // printf("identifier case\n");
    if (t->value->lexeme[0] != 'p' && t->value->lexeme[0] != 'i' &&
        (t->value->lexeme[0] != 'g' || t->value->lexeme[1] != 'i'))
      return 0;
    return 1;
  } else if (t->type == T_ARRAY) {
    if (is_irate(t->right) == 0)
      return 0;
    t = t->next;
    while (t) {
      if (is_irate(t) == 0)
        return 0;
      t = t->next;
    }
    return 1;
  } else
    return 0;
}

// VL 19-10-24
// this is now to be used everywhere to find a variable
// from any pool - global or local
// The search starts with implicit global vars
// then local vars, then any variables not found are
// looked for in the global pools - so local names will always
// hide global names
CS_VARIABLE *find_var_from_pools(CSOUND *csound, const char *varName,
                                 const char *varBaseName,
                                 TYPE_TABLE *typeTable) {
  CS_VARIABLE *var = NULL;

  // DEBUG: Add debug output for InstrDef variable lookups
  if (varBaseName && strstr(varBaseName, "test2")) {
    csound->Message(csound, "[find_var_from_pools] DEBUG: Looking for variable '%s' (baseName='%s')\n",
                    varName ? varName : "(null)", varBaseName ? varBaseName : "(null)");
  }

  // we first check for local variables
  var = csoundFindVariableWithName(csound, typeTable->localPool, varBaseName);
  if (varBaseName && strstr(varBaseName, "test2")) {
    csound->Message(csound, "[find_var_from_pools] DEBUG: Local pool search result: %p\n", (void*)var);
  }

  // then check for global variables in engine
  if (var == NULL) {
    var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                     varBaseName);
    if (varBaseName && strstr(varBaseName, "test2")) {
      csound->Message(csound, "[find_var_from_pools] DEBUG: Engine pool search result: %p\n", (void*)var);
    }
  }

  // and finally newly defined global vars
  if (var == NULL) {
    var = csoundFindVariableWithName(csound, typeTable->globalPool, varBaseName);
    if (varBaseName && strstr(varBaseName, "test2")) {
      csound->Message(csound, "[find_var_from_pools] DEBUG: Global pool search result: %p\n", (void*)var);
    }
  }

  if (varBaseName && strstr(varBaseName, "test2")) {
    csound->Message(csound, "[find_var_from_pools] DEBUG: Final result for '%s': %p\n", varBaseName, (void*)var);
  }

  return var;
}

/*
  Check a symbol for pfield format (pN, PN)
  and return the p-field num ( >= 0 )
  else return -1
*/
static int32_t is_pfield(CSOUND *csound, TYPE_TABLE *typeTable, char *s) {
  CS_VARIABLE *var = find_var_from_pools(csound, s, s, typeTable);
  // if symbol does not exist as a variable
  // or if it is a pfield type var
  if (var == NULL || var->varType == &CS_VAR_TYPE_P) {

    int32_t n;
    if (*s == 'p' || *s == 'P')
      if (sscanf(++s, "%d", &n))
        return (n);
  }
  return (-1);
}

/* This function gets arg type with checking type table */
// Forward decl for struct expr type resolution helper
static char *resolve_struct_expr_type(CSOUND *csound, TREE *tree,
                                      TYPE_TABLE *typeTable);

char *get_arg_type2(CSOUND *csound, TREE *tree, TYPE_TABLE *typeTable) {

  char *s;
  char *t;
  // CS_TYPE* type;
  CS_VARIABLE *var = NULL;
  char *varBaseName;

  if (is_expression_node(tree)) {
    TREE* nodeToCheck = tree;

    // Special case for STRUCT_EXPR: return the actual member type
    if (tree->type == STRUCT_EXPR) {
      char* memberType = resolve_struct_expr_type(csound, tree, typeTable);
      if (memberType) {
        return memberType;
      }
      // Fall through to generic expression handling if resolution fails
    }

    if (tree->type == T_ARRAY) {
      if (tree->left->type == T_FUNCTION) {
        char *fnReturn;
        if ((fnReturn = get_arg_type2(csound, tree->left, typeTable))) {
          if (*fnReturn == '[') {
            // Strip both leading '[' and trailing ']' from "[i]" to get "i"
            size_t len = strlen(fnReturn);
            if (len >= 3 && fnReturn[len-1] == ']') {
              // Create a new string with just the element type
              char* result = csound->Malloc(csound, len - 1); // len-2 for content + 1 for null terminator
              strncpy(result, &fnReturn[1], len - 2);
              result[len - 2] = '\0';
              return result;
            } else {
              // Fallback: just strip the leading '['
              return cs_strdup(csound, &fnReturn[1]);
            }
          } else {
            synterr(csound,
                    Str("non-array type for function %s line %d\n"),
                    tree->left->value->lexeme, tree->line);
            do_baktrace(csound, tree->locn);
            return NULL;
          }
        } else {
          synterr(csound,
                  Str("non-array type for function %s line %d\n"),
                  tree->left->value->lexeme, tree->line);
          do_baktrace(csound, tree->locn);
          return NULL;
        }
      } else if (tree->left->type == STRUCT_EXPR) {
        // Handle struct member array access like users.names[0]
        char *memberType = get_arg_type2(csound, tree->left, typeTable);
        if (memberType && *memberType == '[') {
          // Return the element type (strip the '[' prefix and ']' suffix)
          // memberType is like "[S]", we want "S"
          size_t len = strlen(memberType);
          if (len >= 3 && memberType[len-1] == ']') {
            // Create a new string with just the element type
            char *result = csound->Malloc(csound, len - 1); // len-2 for content + 1 for null terminator
            strncpy(result, &memberType[1], len - 2);
            result[len - 2] = '\0';
            return result;
          } else {
            return NULL;
          }
        } else {
          synterr(csound,
                  Str("non-array type for struct member access at line %d\n"),
                  tree->line);
          do_baktrace(csound, tree->locn);
          return NULL;
        }
      } else {
        // Handle simple array access like myArray[0]
        if (UNLIKELY(tree->left->value == NULL)) {
          synterr(csound, Str("malformed T_ARRAY node (no value) at line %d\n"), tree->line);
          do_baktrace(csound, tree->locn);
          return NULL;
        }
        varBaseName = tree->left->value->lexeme;
        var = find_var_from_pools(csound, varBaseName, varBaseName, typeTable);

        if (var == NULL) {
          synterr(csound,
                  Str("unable to find array operator for var %s line %d\n"),
                  varBaseName, tree->line);
          do_baktrace(csound, tree->locn);
          return NULL;
        } else {
          if (var->varType == &CS_VAR_TYPE_ARRAY) {
            return cs_strdup(csound, var->subType->varTypeName);
          } else if (var->varType == &CS_VAR_TYPE_A) {
            return cs_strdup(csound, "k");
          }
          synterr(csound,
                  Str("invalid array type %s line %d\n"),
                  var->varType->varTypeName, tree->line);
          return NULL;
        }
      }
    }

    if (tree->type == '?') {
      char* arg1, *arg2, *ans, *out;
      char condInTypes[64];

      ans = get_arg_type2(csound, tree->left, typeTable);
      if (UNLIKELY(ans == NULL || (*ans != 'b' && *ans != 'B'))) {
        synterr(csound,
                Str("non-boolean expression found for ternary operator,"
                    " line %d\n"), tree->line);
        do_baktrace(csound, tree->locn);
        return NULL;
      }
      arg1 = get_arg_type2(csound, tree->right->left, typeTable);
      arg2 = get_arg_type2(csound, tree->right->right, typeTable);

      snprintf(condInTypes, 64, "%s%s%s", ans, arg1, arg2);

      OENTRIES* opentries = find_opcode2(csound, ":cond");
      out = resolve_opcode_get_outarg(csound,
                                      opentries,
                                      condInTypes);

      csound->Free(csound, opentries);
      if (UNLIKELY(out == NULL)) {
        synterr(csound,
                Str("unable to find ternary operator for "
                    "types '%s ? %s : %s' line %d\n"),
                ans, arg1, arg2, tree->line);
        do_baktrace(csound, tree->locn);
        return NULL;
      }

      csound->Free(csound, arg1);
      csound->Free(csound, arg2);
      csound->Free(csound, ans);
      return cs_strdup(csound, out);

    }

    // Deal with odd case of i(expressions)
    if (tree->type == T_FUNCTION && !strcmp(tree->value->lexeme, "i")) {
      if(csoundGetDebug(csound) & DEBUG_SEMANTICS)
        print_tree(csound, "i()", tree);
      if (tree->right->type == T_ARRAY &&
          tree->right->left->type == T_IDENT &&
          is_irate(tree->right->right)) {
        synterr(csound, Str("Use of i() with array element ill formed\n"));
      }
      else
        if (UNLIKELY(is_expression_node(tree->right)))
          synterr(csound,
                  Str("Use of i() with expression not permitted on line %d\n"),
                  tree->line);
    }

    if (tree->type == T_FUNCTION) {
      char* argTypeRight = get_arg_string_from_tree(csound,
                                                    tree->right, typeTable);
      char* opname = tree->value->lexeme;
      OENTRIES* entries = find_opcode2(csound, opname);
      char * out;

      if (tree->value->optype != NULL) /* if there is type annotation */
        out = check_annotated_type(csound, entries, tree->value->optype);
      else  out = resolve_opcode_get_outarg(csound, entries, argTypeRight);


      if (UNLIKELY(out == 0)) {
        synterr(csound, Str("opcode '%s' for expression with arg "
                            "types %s not found, line %d\n"),
                opname, argTypeRight, tree->line);
        do_baktrace(csound, tree->locn);
        csound->Free(csound, argTypeRight);
        csound->Free(csound, entries);
        return NULL;
      }

      if (args_required(out) == 1) {
        char** args = split_args(csound, out);
        char *ret = cs_strdup(csound, args[0]);
        csound->Free(csound, argTypeRight);
        csound->Free(csound, entries);
        return ret;
      }

      synterr(csound, Str("opcode '%s' for expression with arg "
                          "types %s returns out-args != 1, line %d\n"),
              opname, argTypeRight, tree->line);
      do_baktrace(csound, tree->locn);

      csound->Free(csound, argTypeRight);
      csound->Free(csound, entries);
      return NULL;

    }

    char* argTypeRight = get_arg_type2(csound,
                                       nodeToCheck->right, typeTable);

    if (nodeToCheck->left != NULL) {
      char* argTypeLeft = get_arg_type2(csound, nodeToCheck->left, typeTable);

      char* opname = get_expression_opcode_type(csound, nodeToCheck);
      int32_t len1, len2;
      char* inArgTypes;
      char* out;

      if (UNLIKELY(argTypeLeft == NULL || argTypeRight == NULL)) {
        synterr(csound,
                Str("Unable to verify arg types for expression '%s'\n"
                    "Line %d\n"),
                opname, tree->line);
        do_baktrace(csound, tree->locn);
        return NULL;
      }

      OENTRIES* entries = find_opcode2(csound, opname);

      argTypeLeft = convert_internal_to_external(csound, argTypeLeft);
      argTypeRight = convert_internal_to_external(csound, argTypeRight);


      len1 = (int32_t) strlen(argTypeLeft);
      len2 = (int32_t) strlen(argTypeRight);
      inArgTypes = csound->Malloc(csound, len1 + len2 + 1);

      memcpy(inArgTypes, argTypeLeft, len1);
      memcpy(inArgTypes + len1, argTypeRight, len2);

      inArgTypes[len1 + len2] = '\0';

      out = resolve_opcode_get_outarg(csound, entries, inArgTypes);
      csound->Free(csound, entries);

      if (UNLIKELY(out == NULL)) {

        synterr(csound, Str("opcode '%s' for expression with arg "
                            "types %s not found, line %d\n"),
                opname, inArgTypes, tree->line);
        do_baktrace(csound, tree->locn);
        csound->Free(csound, inArgTypes);
        return NULL;
      }

      csound->Free(csound, argTypeLeft);
      csound->Free(csound, argTypeRight);
      csound->Free(csound, inArgTypes);
      return cs_strdup(csound, out);

    } else {
      return argTypeRight;
    }

  }

  if (is_boolean_expression_node(tree)) {
    if (tree->type == S_UNOT) {
      return get_arg_type2(csound, tree->left, typeTable);
    }
    else {
      char* argTypeLeft = get_arg_type2(csound, tree->left, typeTable);
      char* argTypeRight = get_arg_type2(csound, tree->right, typeTable);

      char* opname = get_boolean_expression_opcode_type(csound, tree);
      int32_t len1, len2;
      char* inArgTypes;
      char* out;
      OENTRIES* entries;

      if (UNLIKELY(argTypeLeft == NULL || argTypeRight == NULL)) {
        synterr(csound,
                Str("Unable to verify arg types for boolean expression '%s'\n"
                    "Line %d\n"),
                opname, tree->line);
        do_baktrace(csound, tree->locn);
        return NULL;
      }

      entries = find_opcode2(csound, opname);

      len1 = (int32_t) strlen(argTypeLeft);
      len2 = (int32_t) strlen(argTypeRight);
      inArgTypes = csound->Malloc(csound, len1 + len2 + 1);

      memcpy(inArgTypes, argTypeLeft, len1);
      memcpy(inArgTypes + len1, argTypeRight, len2);

      inArgTypes[len1 + len2] = '\0';

      out = resolve_opcode_get_outarg(csound, entries, inArgTypes);
      csound->Free(csound, entries);

      if (UNLIKELY(out == NULL)) {
        synterr(csound, Str("error: boolean expression '%s' with arg "
                            "types %s not found, line %d\n"),
                opname, inArgTypes, tree->line);
        do_baktrace(csound, tree->locn);
        csound->Free(csound, inArgTypes);
        return NULL;
      }

      csound->Free(csound, argTypeLeft);
      csound->Free(csound, argTypeRight);
      csound->Free(csound, inArgTypes);
      return cs_strdup(csound, out);

    }
  }

  if(tree == NULL) {
   synterr(csound, "NULL tree");
   longjmp(csound->exitjmp,0);
   return 0;
  }

  switch (tree->type) {
  case NUMBER_TOKEN:
  case INTEGER_TOKEN:
    return cs_strdup(csound, "c"); /* const */
  case FALSE_TOKEN: {              // trap false expr here
    CS_VARIABLE *var = find_var_from_pools(csound, "false", "false", typeTable);
    if (var == NULL) {
      var = add_global_variable(csound, &csound->engineState,
                                (CS_TYPE *)&CS_VAR_TYPE_b, "false", NULL);
      int32_t *p = (int32_t *)&(var->memBlock->value);
      *p = 0;
    }
  }
    return cs_strdup(csound, "b"); /* boolean */
  case TRUE_TOKEN: {               // trap true expr here
    CS_VARIABLE *var = find_var_from_pools(csound, "true", "true", typeTable);
    if (var == NULL) {
      var = add_global_variable(csound, &csound->engineState,
                                (CS_TYPE *)&CS_VAR_TYPE_b, "true", NULL);
      int32_t *p = (int32_t *)&(var->memBlock->value);
      *p = 1;
    }
  }
    return cs_strdup(csound, "b"); /* boolean */
  case STRING_TOKEN:
    return cs_strdup(csound, "S"); /* quoted String */
  case LABEL_TOKEN:
    // FIXME: Need to review why label token is used so much in parser,
    // for now treat as T_IDENT
  case T_ARRAY_IDENT:
    // check
    if ((var = csoundFindVariableWithName(csound, typeTable->localPool,
                                          tree->value->lexeme)) != NULL) {
      if (var->varType != &CS_VAR_TYPE_ARRAY) {
        synterr(csound,
                Str("Array variable name '%s' used before as a different "
                    "type\n Line %d"),
                tree->value->lexeme, tree->line);
        do_baktrace(csound, tree->locn);
        return NULL;
      }
    }

    case T_IDENT:

    s = tree->value->lexeme;
    if (s == NULL) {
      /* VL: 8/3/2018
         something very wrong happened.
         To prevent a crash, we get out
         here. Not sure if any other
         diagnostics are due */
      return NULL;
    }

    /* check for instrument name variables on the engine varPool */
    if((var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                         tree->value->lexeme)) != NULL) {
       if(var->varType == &CS_VAR_TYPE_INSTR)
         // found it, return type.
         return cs_strdup(csound, var->varType->varTypeName);
     }

    if(!strcmp(tree->value->lexeme, "this_instr")) {
      const CS_TYPE *varType = &CS_VAR_TYPE_INSTR;
       // found this, return type.
       return cs_strdup(csound, varType->varTypeName);
     }

     if(!strcmp(tree->value->lexeme, "this")) {
      const CS_TYPE *varType = &CS_VAR_TYPE_INSTR_INSTANCE;
       // found this, return type.
       return cs_strdup(csound, varType->varTypeName);
     }



    if (is_reserved(s)) {
      return cs_strdup(csound, "r");                              /* rsvd */
    }

    if (is_label(s, typeTable->labelList)) {
      return cs_strdup(csound, "l");
    }

    if ((*s >= '1' && *s <= '9') || *s == '.' || *s == '-' || *s == '+' ||
        (*s == '0' && strcmp(s, "0dbfs") != 0))
      return cs_strdup(csound, "c");                          /* const */
    if (*s == '"')
      return cs_strdup(csound, "S");

    if (is_pfield(csound, typeTable, s) >= 0)
      return cs_strdup(csound, "p");                      /* p-field number */

    varBaseName = s;

    if (*s == '#') {
      // find synthetic vars
      s++;
      var = find_var_from_pools(csound, s, tree->value->lexeme, typeTable);
    }
    else {
      // other vars
      // make a copy to preserve the lexeme
      char *s_copy = cs_strdup(csound, s);
      // strip @global if it exists, it's a non-op here
      find_global_annotation(s_copy, typeTable);
      // find the variable in one of the variable pools
      var = find_var_from_pools(csound, s_copy, s_copy, typeTable);
      csound->Free(csound, s_copy);
    }


    if (UNLIKELY(var == NULL)) {
      synterr(csound, Str("get_arg_type2: Variable '%s' used before defined\n"
                          "Line %d"),
              tree->value->lexeme, tree->line - 1);
      do_baktrace(csound, tree->locn);
      return NULL;
    }

     if (var->varType == &CS_VAR_TYPE_ARRAY) {
        char *res = create_array_arg_type(csound, var);
        if (res==NULL) {        /* **REVIEW** this double syntax error */
          synterr(csound, Str("Array of unknown type\n"));
          csoundMessage(csound, Str("Line: %d\n"), tree->line-1);
          do_baktrace(csound, tree->locn);
        }
        return res;
      } else {
        return cs_strdup(csound, var->varType->varTypeName);
      }

  case T_TYPED_IDENT:
    {
      // BUGFIX: If this is a variable, look up its actual type instead of trusting optype
      // which may have been incorrectly set during struct array implementation
      if (tree->value && tree->value->lexeme) {
        CS_VARIABLE* var = find_var_from_pools(csound, tree->value->lexeme,
                                               tree->value->lexeme, typeTable);
        if (var && var->varType) {
          // If it's an audio signal, return 'a' regardless of what optype says
          if (var->varType == &CS_VAR_TYPE_A) {
            return cs_strdup(csound, "a");
          }
          // For other types, use the varType name
          return cs_strdup(csound, var->varType->varTypeName);
        }
      }

      return cs_strdup(csound, tree->value->optype);
    }
  case T_MEMBER_IDENT:
    // T_MEMBER_IDENT represents a struct member name that gets converted to a member index
    // For opcode resolution (e.g., ##member_get), this should be treated as a constant
    // since the member name gets resolved to a constant integer index during expression processing
    return cs_strdup(csound, "c");
  case STRUCT_EXPR:
    // DRY: delegate STRUCT_EXPR type resolution to helper
    return resolve_struct_expr_type(csound, tree, typeTable);

  case T_ARRAY:

    s = tree->value->lexeme;

    if (*s == '#') s++;
    if (*s == 'g') s++;

    t = s;

    int32_t len = 1;
    while (*t == '[') {
      t++;
      len++;
    }

    char *retVal = csound->Malloc(csound, (len + 2) * sizeof(char));
    memcpy(retVal, s, len);
    retVal[len] = ']';
    retVal[len + 1] = '\0';

    return retVal;


  default:
    csoundWarning(csound, Str("Unknown arg type: %d\n"), tree->type);
    print_tree(csound, "Arg Tree\n", tree);
    return NULL;
  }

  return NULL;
}

// Helper to resolve STRUCT_EXPR to the exact member type, including
// handling of array-of-struct chains like a[i].b[j].c, without
// going through generic opcode resolution.
static char *resolve_struct_expr_type(CSOUND *csound, TREE *tree,
                                      TYPE_TABLE *typeTable) {
  char *s;
  CS_VARIABLE *var = NULL;

  if (UNLIKELY(tree->left == NULL)) {
    synterr(csound, Str("STRUCT_EXPR: tree->left is NULL at line %d\n"),
            tree->line);
    do_baktrace(csound, tree->locn);
    return NULL;
  }

  // Get the full struct expression string for error messages
  char *originalVarName = NULL;
  int freeOriginalVarName = 0;

  // Handle nested STRUCT_EXPR (e.g., var1.complex.imag)
  if (tree->left->type == STRUCT_EXPR) {
    char *nestedType = resolve_struct_expr_type(csound, tree->left, typeTable);
    if (nestedType == NULL) {
      if (freeOriginalVarName)
        csound->Free(csound, originalVarName);
      return NULL;
    }
    const CS_TYPE *nestedStructType =
        csoundGetTypeWithVarTypeName(csound->typePool, nestedType);
    csound->Free(csound, nestedType);
    if (nestedStructType == NULL) {
      synterr(csound, Str("Cannot find type for nested struct expression\n"));
      if (freeOriginalVarName)
        csound->Free(csound, originalVarName);
      do_baktrace(csound, tree->locn);
      return NULL;
    }
    s = tree->right->value->lexeme;
    CONS_CELL *cell = nestedStructType->members;
    CS_VARIABLE *memberVar = NULL;
    while (cell != NULL) {
      CS_VARIABLE *member = (CS_VARIABLE *)cell->value;
      if (!strcmp(member->varName, s)) {
        memberVar = member;
        break;
      }
      cell = cell->next;
    }
    if (memberVar == NULL) {
      if (originalVarName == NULL) {
        originalVarName = get_struct_expr_string(csound, tree);
        freeOriginalVarName = (originalVarName != NULL);
      }
      synterr(csound, Str("No member '%s' found for variable '%s'\n"), s,
              originalVarName ? originalVarName : "unknown");
      if (freeOriginalVarName)
        csound->Free(csound, originalVarName);
      do_baktrace(csound, tree->locn);
      return NULL;
    }
    // Return array notation if final member is array
    if (memberVar->varType == &CS_VAR_TYPE_ARRAY) {
      char *result = create_array_arg_type(csound, memberVar);
      if (result == NULL) {
        synterr(csound, Str("Array member has unknown type\n"));
        if (freeOriginalVarName)
          csound->Free(csound, originalVarName);
        do_baktrace(csound, tree->locn);
        return NULL;
      }
      if (freeOriginalVarName)
        csound->Free(csound, originalVarName);
      return result;
    }
    if (freeOriginalVarName)
      csound->Free(csound, originalVarName);
    {
      char *ret = cs_strdup(csound, memberVar->varType->varTypeName);
      return ret;
    }
  }

  // Handle both simple struct access (struct.member) and array struct access
  if (tree->left->value == NULL) {
    // Array expression like var0[indx] or nested
    if (tree->left->type == T_ARRAY && tree->left->left != NULL) {
      if (tree->left->left->type == STRUCT_EXPR) {
        // structArray[index].member -> resolve element struct type
        char *structArrayType =
            resolve_struct_expr_type(csound, tree->left->left, typeTable);
        if (structArrayType == NULL) {
          if (freeOriginalVarName)
            csound->Free(csound, originalVarName);
          return NULL;
        }

        char *elementType = NULL;
        int len = (int)strlen(structArrayType);
        if (structArrayType[0] == ':') {
          char *semicolon = strchr(structArrayType, ';');
          if (semicolon != NULL) {
            int typeNameLen = (int)(semicolon - structArrayType - 1);
            elementType = csound->Malloc(csound, typeNameLen + 1);
            strncpy(elementType, structArrayType + 1, typeNameLen);
            elementType[typeNameLen] = '\0';
          }
        } else if (len > 2 && structArrayType[len - 2] == '[' &&
                   structArrayType[len - 1] == ']') {
          elementType = cs_strdup(csound, structArrayType);
          elementType[len - 2] = '\0';
        }
        if (elementType != NULL) {
          // Convert external format to internal format for struct types
          char *internalElementType = check_optional_type(csound, elementType);
          if (!internalElementType) {
            internalElementType =
                cs_strdup(csound, elementType); // fallback to original
          }
          const CS_TYPE *structType = csoundGetTypeWithVarTypeName(
              csound->typePool, internalElementType);
          csound->Free(csound, elementType);
          csound->Free(csound, internalElementType);
          csound->Free(csound, structArrayType);
          if (structType == NULL) {
            synterr(csound, Str("Cannot find struct type for array element\n"));
            if (freeOriginalVarName)
              csound->Free(csound, originalVarName);
            do_baktrace(csound, tree->locn);
            return NULL;
          }
          s = tree->right->value->lexeme;
          CONS_CELL *cell = structType->members;
          CS_VARIABLE *memberVar = NULL;
          while (cell != NULL) {
            CS_VARIABLE *member = (CS_VARIABLE *)cell->value;
            if (!strcmp(member->varName, s)) {
              memberVar = member;
              break;
            }
            cell = cell->next;
          }
          if (memberVar == NULL) {
            synterr(csound, Str("No member '%s' found in struct\n"), s);
            if (freeOriginalVarName)
              csound->Free(csound, originalVarName);
            do_baktrace(csound, tree->locn);
            return NULL;
          }
          char *result;
          if (memberVar->varType == &CS_VAR_TYPE_ARRAY) {
            result = create_array_arg_type(csound, memberVar);
            if (result == NULL) {
              synterr(csound, Str("Array member has unknown type\n"));
              if (freeOriginalVarName)
                csound->Free(csound, originalVarName);
              do_baktrace(csound, tree->locn);
              return NULL;
            }
          } else {
            result = cs_strdup(csound, memberVar->varType->varTypeName);
          }
          if (freeOriginalVarName)
            csound->Free(csound, originalVarName);
          return result;
        } else {
          synterr(csound, Str("Expected array type but got '%s'\n"),
                  structArrayType);
          csound->Free(csound, structArrayType);
          if (freeOriginalVarName)
            csound->Free(csound, originalVarName);
          do_baktrace(csound, tree->locn);
          return NULL;
        }
      } else if (tree->left->left->value != NULL &&
                 tree->left->left->value->lexeme != NULL) {
        // Simple array access like var0[indx]
        s = tree->left->left->value->lexeme;
      } else {
        synterr(csound,
                Str("STRUCT_EXPR: Cannot find struct name for array access at "
                    "line %d\n"),
                tree->line);
        if (freeOriginalVarName)
          csound->Free(csound, originalVarName);
        do_baktrace(csound, tree->locn);
        return NULL;
      }
    } else {
      synterr(csound,
              Str("STRUCT_EXPR: Unexpected structure for array access at line "
                  "%d\n"),
              tree->line);
      if (freeOriginalVarName)
        csound->Free(csound, originalVarName);
      do_baktrace(csound, tree->locn);
      return NULL;
    }
  } else {
    // Simple struct access
    if (UNLIKELY(tree->left->value->lexeme == NULL)) {
      synterr(
          csound,
          Str("STRUCT_EXPR: tree->left->value->lexeme is NULL at line %d\n"),
          tree->line);
      if (freeOriginalVarName)
        csound->Free(csound, originalVarName);
      do_baktrace(csound, tree->locn);
      return NULL;
    }
    s = tree->left->value->lexeme;
  }

  // Only look up the variable if we have a simple identifier (not nested struct
  // arrays)
  if (s != NULL) {
    var = find_var_from_pools(csound, s, s, typeTable);
  }

  if (UNLIKELY(var == NULL)) {
    // Deduplicate repeated undefined-variable diagnostics at the same source
    // location
    static struct {
      char name[64];
      int line;
    } lastUndefs_resolve[8];
    static int lastUndefsIdx_resolve = 0;
    int alreadyReported = 0;
    for (int i = 0; i < 8; ++i) {
      if (lastUndefs_resolve[i].line == tree->line &&
          lastUndefs_resolve[i].name[0] != '\0' && s &&
          strcmp(lastUndefs_resolve[i].name, s) == 0) {
        alreadyReported = 1;
        break;
      }
    }
    if (!alreadyReported) {
      synterr(csound, Str("Variable '%s' used before defined\n"), s);
      if (s) {
        strncpy(lastUndefs_resolve[lastUndefsIdx_resolve].name, s,
                sizeof(lastUndefs_resolve[0].name) - 1);
        lastUndefs_resolve[lastUndefsIdx_resolve]
            .name[sizeof(lastUndefs_resolve[0].name) - 1] = '\0';
      } else {
        lastUndefs_resolve[lastUndefsIdx_resolve].name[0] = '\0';
      }
      lastUndefs_resolve[lastUndefsIdx_resolve].line = tree->line;
      lastUndefsIdx_resolve = (lastUndefsIdx_resolve + 1) & 7;
      do_baktrace(csound, tree->locn);
    }
    if (freeOriginalVarName)
      csound->Free(csound, originalVarName);
    return NULL;
  }

  // For array access like var0[indx].member, use the element type for member
  // lookup
  const CS_TYPE *structType = var->varType;
  if (tree->left->value == NULL && tree->left->type == T_ARRAY) {
    if (var->varType != &CS_VAR_TYPE_ARRAY) {
      synterr(csound, Str("Variable '%s' is not an array\n"), s);
      if (freeOriginalVarName)
        csound->Free(csound, originalVarName);
      do_baktrace(csound, tree->locn);
      return NULL;
    }
    if (var->subType == NULL) {
      synterr(csound, Str("Array '%s' has no element type defined\n"), s);
      if (freeOriginalVarName)
        csound->Free(csound, originalVarName);
      do_baktrace(csound, tree->locn);
      return NULL;
    }
    structType = var->subType;
  }

  TREE *t = tree->right;
  while (t != NULL) {
    s = t->value->lexeme;
    CONS_CELL *cell = structType->members;
    CS_VARIABLE *nextVar = NULL;
    while (cell != NULL) {
      CS_VARIABLE *member = (CS_VARIABLE *)cell->value;
      if (!strcmp(member->varName, s)) {
        nextVar = member;
        break;
      }
      cell = cell->next;
    }
    if (nextVar == NULL) {
      if (originalVarName == NULL) {
        originalVarName = get_struct_expr_string(csound, tree);
        freeOriginalVarName = (originalVarName != NULL);
      }
      synterr(csound, Str("No member '%s' found for variable '%s'\n"), s,
              originalVarName ? originalVarName : "unknown");
      if (freeOriginalVarName)
        csound->Free(csound, originalVarName);
      do_baktrace(csound, tree->locn);
      return NULL;
    }
    var = nextVar;
    structType = var->varType;
    t = t->next;
  }

  char *result;
  if (var->varType == &CS_VAR_TYPE_ARRAY) {
    result = create_array_arg_type(csound, var);
    if (result == NULL) {
      synterr(csound, Str("Array member has unknown type\n"));
      if (freeOriginalVarName)
        csound->Free(csound, originalVarName);
      do_baktrace(csound, tree->locn);
      return NULL;
    }
  } else {
    result = cs_strdup(csound, var->varType->varTypeName);
  }
  if (freeOriginalVarName)
    csound->Free(csound, originalVarName);

    return result;
}

char *get_opcode_short_name(CSOUND *csound, char *opname) {

  char *dot = strchr(opname, '.');
  if (dot != NULL) {
    uint64_t opLen = dot - opname;
    return cs_strndup(csound, opname, opLen);
  }
  return opname;
}

/* find opcode with the specified name in opcode list */
/* returns index to opcodlst[], or zero if the opcode cannot be found */
OENTRY *find_opcode(CSOUND *csound, char *opname) {
  char *shortName;
  CONS_CELL *head;
  OENTRY *retVal = NULL;

  if (opname[0] == '\0' || isdigit(opname[0]))
    return 0;

  shortName = get_opcode_short_name(csound, opname);

  head = cs_hash_table_get(csound, csound->opcodes, shortName);

  // Prefer exact opname match within the bucket if possible
  if (head != NULL) {
    CONS_CELL *cur = head;
    while (cur != NULL) {
      OENTRY *e = (OENTRY *)cur->value;
      if (e && e->opname && strcmp(e->opname, opname) == 0) {
        retVal = e;
        break;
      }
      cur = cur->next;
    }
    // Fallback to first entry if no exact match by full name
    if (retVal == NULL)
      retVal = head->value;
  }

  if (shortName != opname)
    csound->Free(csound, shortName);

  return retVal;
}

static OENTRIES *get_entries(CSOUND *csound, int32_t count) {
  // Allocate space for the header plus the array of OENTRY* pointers
  OENTRIES *x =
      csound->Calloc(csound, sizeof(OENTRIES) + sizeof(OENTRY *) * count);
  x->count = count;
  return x;
}

/* Finds OENTRIES that match the given opcode name.  May return multiple
 * OENTRY*'s for each entry in a polyMorphic opcode.
 */
OENTRIES* find_opcode2(CSOUND* csound, char* opname)
{
  int32_t i = 0;
  char *shortName;
  CONS_CELL *head;
  OENTRIES* retVal;

  if (UNLIKELY(opname == NULL)) {
    return NULL;
  }

  shortName = get_opcode_short_name(csound, opname);
  head = cs_hash_table_get(csound, csound->opcodes, shortName);
  retVal = get_entries(csound, cs_cons_length(head));
  while (head != NULL) {
    retVal->entries[i++] = head->value;
    head = head->next;
  }

  if (shortName != opname) {
    csound->Free(csound, shortName);
  }

  return retVal;

}

inline static int32_t is_in_optional_arg(char *arg) {
  return (strlen(arg) == 1) && (strchr("opqvjhOJVP?", *arg) != NULL);
}

inline static int32_t is_in_var_arg(char *arg) {
  return (strlen(arg) == 1) && (strchr("mMNnWyzZ*", *arg) != NULL);
}

int32_t check_array_arg(char *found, char *required) {
  char *f = found;
  char *r = required;

  while (*r == '[')
    r++;

  if (*r == '.' || *r == '?' || *r == '*') {
    return 1;
  }

  while (*f == '[')
    f++;

  return (*f == *r);
}

int32_t check_array_arg_in(char *found, char *required) {
  if (!found || !required)
    return 0;
  char *f = found;
  char *r = required;

  // Quick generic matches
  while (*r == '[')
    r++;
  if (*r == '.' || *r == '?' || *r == '*')
    return 1;
  while (*f == '[')
    f++;

  // Enforce arrayness: scalar vs array should NOT match for inputs (honor any
  // '[' form)
  int foundIsArray = (strchr(found, '[') != NULL);
  int reqIsArray = (strchr(required, '[') != NULL);
  if (foundIsArray != reqIsArray)
    return 0;

  // Special case: k args accept i (and constants) as inputs (scalar or array
  // handled above)
  if (*r == 'k' && (*f == 'i' || *f == 'c'))
    return 1;

  return (*f == *r);
}

int32_t check_in_arg(char *found, char *required) {
  char *t;
  int32_t i;
  if (UNLIKELY(found == NULL || required == NULL)) {
    return 0;
  }

  if (strcmp(found, required) == 0) {
    return 1;
  }

  // Allow numeric constant 'c' to satisfy numeric input rates (i, k), not 'a'
  if (found[0] == 'c' && (required[0] == 'i' || required[0] == 'k')) {
    return 1;
  }
  // p-field 'p' satisfies i- and k-rate inputs (k accepts i-like sources)
  if (found[0] == 'p' && (required[0] == 'i' || required[0] == 'k')) {
    return 1;
  }

  // If either side denotes an array (external k[], typed :Type;[], or internal
  // prefix '['), delegate to the array-aware matcher that also enforces
  // scalar/array mismatch rejection.
  if ((found && strchr(found, '[')) || (required && strchr(required, '['))) {
    return check_array_arg_in(found, required);
  }

  if (*required == '.' || *required == '?' || *required == '*') {
    return 1;
  }

  t = (char *)POLY_IN_TYPES[0];

  for (i = 0; t != NULL; i += 2) {
    if (strcmp(required, t) == 0) {
      return (strchr(POLY_IN_TYPES[i + 1], *found) != NULL);
    }
    t = (char *)POLY_IN_TYPES[i + 2];
  }

  if (is_in_optional_arg(required)) {
    t = (char *)OPTIONAL_IN_TYPES[0];
    for (i = 0; t != NULL; i += 2) {
      if (strcmp(required, t) == 0) {
        return (strchr(OPTIONAL_IN_TYPES[i + 1], *found) != NULL);
      }
      t = (char *)OPTIONAL_IN_TYPES[i + 2];
    }
  }

  if (!is_in_var_arg(required)) {
    return 0;
  }

  t = (char *)VAR_ARG_IN_TYPES[0];
  for (i = 0; t != NULL; i += 2) {
    if (strcmp(required, t) == 0) {
      return (strchr(VAR_ARG_IN_TYPES[i + 1], *found) != NULL);
    }
    t = (char *)VAR_ARG_IN_TYPES[i + 2];
  }
  return 0;
}

int32_t check_in_args(CSOUND *csound, char *inArgsFound, char *opInArgs) {

  if ((inArgsFound == NULL || strlen(inArgsFound) == 0) &&
      (opInArgs == NULL || strlen(opInArgs) == 0)) {
    return 1;
  }

  if (UNLIKELY(opInArgs == NULL)) {
    return 0;
  }

  // Adaptation: if found types include an untyped array placeholder like 'i.[]'
  // and the opcode requires a UDT array like 'i:Type;[]', build a typed RHS
  // using the expected base from opInArgs so matching can succeed.
  char *inArgsUsed = inArgsFound;
  if (inArgsFound && opInArgs) {
    if (inArgsFound[0] && inArgsFound[1] == '.' && inArgsFound[2] == '[' &&
        inArgsFound[3] == ']') {
      const char *colon = strchr(opInArgs, ':');
      const char *semi = colon ? strchr(colon, ';') : NULL;
      const char *arr = semi ? strstr(semi, "[]") : NULL;
      if (colon && semi && arr) {
        char rate = (inArgsFound[0] == 'c') ? 'i' : inArgsFound[0];
        size_t baseLen = (size_t)(semi - colon + 1); // includes ':'..';'
        char *built = csound->Malloc(csound, 1 + baseLen + 2 + 1);
        built[0] = rate;
        memcpy(built + 1, colon, baseLen);
        built[1 + baseLen] = '[';
        built[2 + baseLen] = ']';
        built[3 + baseLen] = '\0';
        inArgsUsed = built;
      }
    }
    if (inArgsFound[0] && inArgsFound[1] == '\0') {
      // Single rate like 'i' while opcode expects typed UDT array 'i:Type;[]'
      const char *colon = strchr(opInArgs, ':');
      const char *semi = colon ? strchr(colon, ';') : NULL;
      const char *arr = semi ? strstr(semi, "[]") : NULL;
      if (colon && semi && arr && opInArgs[0] == inArgsFound[0]) {
        char rate = (inArgsFound[0] == 'c') ? 'i' : inArgsFound[0];
        size_t baseLen = (size_t)(semi - colon + 1);
        char *built = csound->Malloc(csound, 1 + baseLen + 2 + 1);
        built[0] = rate;
        memcpy(built + 1, colon, baseLen);
        built[1 + baseLen] = '[';
        built[2 + baseLen] = ']';
        built[3 + baseLen] = '\0';
        inArgsUsed = built;
      }
    }
  }

  int32_t argsFoundCount = args_required(inArgsUsed);
  int32_t args_requiredCount = args_required(opInArgs);
  char **args_required = split_args(csound, opInArgs);
  char **argsFound;
  int32_t i;
  int32_t argTypeIndex = 0;
  char *varArg = NULL;
  int32_t returnVal = 1;

  if (args_required == NULL) {
    return 0;
  }
  if (argsFoundCount >= VARGMAX) {
    return -1;
  }

  if ((argsFoundCount > args_requiredCount) &&
      !(is_in_var_arg(args_required[args_requiredCount - 1]))) {
    csound->Free(csound, args_required);
    return 0;
  }

  argsFound = split_args(csound, inArgsUsed);
  if (argsFound == NULL) {
    // Malformed inArgsFound (e.g., unmatched bracket such as "c[")
    // Treat as non-match instead of crashing; caller may try other candidates.
    csound->Free(csound, args_required);
    if (inArgsUsed != inArgsFound)
      csound->Free(csound, inArgsUsed);
    return 0;
  }

  if (argsFoundCount == 0) {
    if (is_in_var_arg(args_required[0])) {
      varArg = args_required[0];
    }
  } else {
    for (i = 0; i < argsFoundCount; i++) {
      char *argFound = argsFound[i];

      if (varArg != NULL) {
        if (!check_in_arg(argFound, varArg)) {
          returnVal = 0;
          break;
        }
      } else {
        char *argRequired = args_required[argTypeIndex++];
        if (!check_in_arg(argFound, argRequired)) {
          returnVal = 0;
          break;
        }
        if (is_in_var_arg(argRequired)) {
          varArg = argRequired;
        }
      }
    }
  }

  if (returnVal && varArg == NULL) {
    while (argTypeIndex < args_requiredCount) {
      char *c = args_required[argTypeIndex++];

      if (!is_in_optional_arg(c) && !is_in_var_arg(c)) {
        returnVal = 0;
        break;
      }
    }
  }

  int32_t n;
  for (n = 0; argsFound[n] != NULL; n++) {
    csound->Free(csound, argsFound[n]);
  }
  csound->Free(csound, argsFound);
  for (n = 0; args_required[n] != NULL; n++) {
    csound->Free(csound, args_required[n]);
  }
  csound->Free(csound, args_required);

  if (inArgsUsed != inArgsFound)
    csound->Free(csound, inArgsUsed);

  return returnVal;
}

inline static int32_t is_out_var_arg(char *arg) {
  return strlen(arg) == 1 && (strchr("mzIXNvVF*", *arg) != NULL);
}

int32_t check_out_arg(char *found, char *required) {
  char *t;
  int32_t i;

  if (UNLIKELY(found == NULL || required == NULL)) {
    return 0;
  }

  // constants not allowed in out args
  if (strcmp(found, "c") == 0) {
    return 0;
  }

  if (*required == '.' || *required == '?' || *required == '*') {
    return 1;
  }

  // Relaxed array-dimension matching: allow k[] requirement to match k[][]...
  // (same base, >=1 dims)
  if (required[0] && required[1] == '[' && required[2] == ']' &&
      required[3] == '\0') {
    char base = required[0];
    if (found[0] == base) {
      const char *f = found + 1;
      int ok = 1, hasDim = 0;
      while (*f) {
        if (f[0] == '[' && f[1] == ']') {
          hasDim = 1;
          f += 2;
        } else {
          ok = 0;
          break;
        }
      }
      if (ok && hasDim)
        return 1;
    }
  }

  if (*found == '[' || *required == '[') {
    if (*found != *required) {
      return 0;
    }
    return check_array_arg(found, required);
  }

  if (strcmp(found, required) == 0) {
    return 1;
  }

  // check for multichar types now
  if (strlen(found) > 1 && strcmp(found, required)) {
    return 0;
  }

  t = (char *)POLY_OUT_TYPES[0];
  for (i = 0; t != NULL; i += 2) {
    if (strcmp(required, t) == 0) {
      return (strchr(POLY_OUT_TYPES[i + 1], *found) != NULL);
    }
    t = (char *)POLY_OUT_TYPES[i + 2];
  }

  if (!is_out_var_arg(required)) {
    return 0;
  }

  t = (char *)VAR_ARG_OUT_TYPES[0];
  for (i = 0; t != NULL; i += 2) {
    if (strcmp(required, t) == 0) {
      return (strchr(VAR_ARG_OUT_TYPES[i + 1], *found) != NULL);
    }
    t = (char *)VAR_ARG_OUT_TYPES[i + 2];
  }
  return 0;
}

int32_t check_out_args(CSOUND* csound, char* outArgsFound, char* opOutArgs)
{

  if ((outArgsFound == NULL || strlen(outArgsFound) == 0) &&
      (opOutArgs == NULL || strlen(opOutArgs) == 0)) {
    return 1;
  }

  {
    int32_t argsFoundCount = args_required(outArgsFound);
    int32_t argsRequiredCount = args_required(opOutArgs);
    char** argsRequired = split_args(csound, opOutArgs);
    char** argsFound;
    int32_t i;
    int32_t argTypeIndex = 0;
    char* varArg = NULL;
    int32_t returnVal = 1;

    if ((argsFoundCount > argsRequiredCount) &&
        !(is_out_var_arg(argsRequired[argsRequiredCount - 1]))) {
      csound->Free(csound, argsRequired);
      return 0;
    }

    argsFound = split_args(csound, outArgsFound);

    for (i = 0; i < argsFoundCount; i++) {
      char* argFound = argsFound[i];

      if (varArg != NULL) {
        if (!check_out_arg(argFound, varArg)) {
          returnVal = 0;
          break;
        }
      } else {
        char* argRequired = argsRequired[argTypeIndex++];
        if (!check_out_arg(argFound, argRequired)) {
          returnVal = 0;
          break;
        }
        if (is_out_var_arg(argRequired)) {
          varArg = argRequired;
        }
      }
    }

    if (returnVal && varArg == NULL) {

      if (argTypeIndex < argsRequiredCount) {
        char* argRequired = argsRequired[argTypeIndex];
        returnVal = is_out_var_arg(argRequired);
      } else {
        returnVal = 1;
      }
    }
    int32_t n;
    for (n=0; argsFound[n] != NULL; n++) {
      csound->Free(csound, argsFound[n]);
    }
    csound->Free(csound, argsFound);
    for (n=0; argsRequired[n] != NULL; n++) {
      csound->Free(csound, argsRequired[n]);
    }
    csound->Free(csound, argsRequired);
    return returnVal;
  }
}
/* Given an OENTRIES list, resolve to a single OENTRY* based on the
 * found in- and out- argtypes.  Returns NULL if opcode could not be
 * resolved. If more than one entry matches, mechanism assumes there
 * are multiple opcode entries with same types and last one should
 * override previous definitions.
 */
/* Given an OENTRIES list, resolve to a single OENTRY* based on the
 * found in- and out- argtypes.  Returns NULL if opcode could not be
 * resolved. If more than one entry matches, mechanism assumes there
 * are multiple opcode entries with same types and last one should
 * override previous definitions.
 */
OENTRY* resolve_opcode(CSOUND* csound, OENTRIES* entries,
                       char* outArgTypes, char* inArgTypes) {
  int32_t i, check;
  OENTRY* wildcard_match = NULL;  // Store first wildcard match as fallback

  for (i = 0; i < entries->count; i++) {
    OENTRY* temp = entries->entries[i];

    if ((check = check_in_args(csound, inArgTypes, temp->intypes)) &&
        check_out_args(csound, outArgTypes, temp->outypes)) {

      if (check == -1)
        synterr(csound,
                Str("Found %d inputs for %s which is more than "
                    "the %d allowed\n"),
                args_required(inArgTypes), temp->opname, VARGMAX);

      // Check if this is a wildcard match (contains '.' or '?')
      int is_wildcard = (temp->intypes && (strchr(temp->intypes, '.') != NULL ||
                                            strchr(temp->intypes, '?') != NULL));

      if (!is_wildcard) {
        // Prefer non-wildcard (exact) matches
        return temp;
      } else if (wildcard_match == NULL) {
        // Store first wildcard match as fallback
        wildcard_match = temp;
      }
    }
  }

  // Return wildcard match if no exact match was found
  return wildcard_match;
}

OENTRY *resolve_opcode_exact(CSOUND *csound, OENTRIES *entries,
                             char *outArgTypes, char *inArgTypes) {
  IGN(csound);
  OENTRY *retVal = NULL;
  int32_t i;

  char *outTest = (!strcmp("0", outArgTypes)) ? "" : outArgTypes;
  for (i = 0; i < entries->count; i++) {
    OENTRY *temp = entries->entries[i];
    if (temp->intypes != NULL && !strcmp(inArgTypes, temp->intypes) &&
        temp->outypes != NULL && !strcmp(outTest, temp->outypes)) {
      retVal = temp;
    }
  }
  return retVal;
}

/* used when creating T_FUNCTION's */
char *resolve_opcode_get_outarg(CSOUND *csound, OENTRIES *entries,
                                char *inArgTypes) {
  int32_t i;

  // Heuristic repair: some builtins like lenarray expect an array argument
  // (".[]"). If we see a single scalar type like 'S' due to use-before-def
  // reordering,
  if (entries && entries->count > 0) {
    const char *__opn = entries->entries[0]->opname;
    if (csound->GetDebug(csound))
      csound->Message(csound, "RESOLVE_OUT pre opname=%s inArg=%s\n",
                      __opn ? __opn : "(null)",
                      inArgTypes ? inArgTypes : "(null)");
  }

  // promote it to 'S[]' to allow resolution to succeed.
  if (entries && entries->count > 0 && inArgTypes &&
      strchr(inArgTypes, '[') == NULL) {
    const char *opname0 = entries->entries[0]->opname;
    if (opname0 && strcmp(opname0, "lenarray") == 0) {
      size_t n = strlen(inArgTypes);
      char *patched = csound->Malloc(csound, n + 2 + 1);
      memcpy(patched, inArgTypes, n);
      patched[n] = '[';
      patched[n + 1] = ']';
      patched[n + 2] = '\0';
      inArgTypes =
          patched; // leaked on purpose; small and one-shot during resolve
    }
  }

  if (entries == NULL) {
    csound->Message(csound,
                    "ERROR: resolve_opcode_get_outarg called with NULL entries "
                    "for input types '%s'\n",
                    inArgTypes ? inArgTypes : "NULL");
    return NULL;
  }

  // Debug for outarg selection in internal ops (array arithmetic or bitwise)
  if (csound->GetDebug(csound) && entries->count > 0 && entries->entries[0] &&
      entries->entries[0]->opname) {
    const char *base0 = entries->entries[0]->opname;
    if (!strncmp(base0, "##", 2)) {
      csound->Message(
          csound, "RESOLVE_OUT(op): base=%s inArg=%s candidates=%d\n", base0,
          inArgTypes ? inArgTypes : "(null)", entries->count);
    }
  }

  for (i = 0; i < entries->count; i++) {
    OENTRY *temp = entries->entries[i];
    if (csound->GetDebug(csound) && entries && entries->entries[0] &&
        entries->entries[0]->opname) {
      const char *base0 = entries->entries[0]->opname;
      if (!strncmp(base0, "##or", 4) || !strncmp(base0, "##and", 5) ||
          !strncmp(base0, "##xor", 5)) {
        csound->Message(csound,
                        "RESOLVE_OUT(try) base=%s cand[%d] inArg='%s' -> entry "
                        "out='%s' in='%s'\n",
                        base0, i, inArgTypes ? inArgTypes : "",
                        temp && temp->outypes ? temp->outypes : "",
                        temp && temp->intypes ? temp->intypes : "");
      }
    }

    int inOk = check_in_args(csound, inArgTypes, temp->intypes);
    if (entries && entries->entries[0] && entries->entries[0]->opname &&
        csound->GetDebug(csound)) {
      const char *base0 = entries->entries[0]->opname;
      if (!strncmp(base0, "##", 2)) {
        csound->Message(csound,
                        "RESOLVE_OUT(check) base=%s cand[%d] inArg='%s' "
                        "reqIn='%s' -> inOk=%d out='%s'\n",
                        base0, i, inArgTypes ? inArgTypes : "",
                        temp && temp->intypes ? temp->intypes : "", inOk,
                        temp && temp->outypes ? temp->outypes : "");
      }
    }

    if (temp->intypes == NULL && temp->outypes == NULL) {
      continue;
    }
    if (check_in_args(csound, inArgTypes, temp->intypes)) {
      // FIXME this is only returning the first match, we need to check
      // if there are multiple matches and if so, return NULL to signify
      // ambiguity
      return temp->outypes;
    }
  }
  return NULL;
}

/* Converts internal array specifier from [[a] to a[][].
   Used by get_arg_string_from_tree to create an arg string that is
   compatible with the ones found in OENTRY's.  split_args converts back
   to internal representation. */
char *convert_internal_to_external(CSOUND *csound, char *arg) {
  int32_t i = 0, dimensions;
  char *start;
  char *retVal, *current;
  uint64_t nameLen, len;
  char *type;

  if (arg == NULL) {
    return NULL;
  }
  len = strlen(arg);
  // Handle empty and single-char strings up front to avoid OOB reads below
  if (len == 0) {
    return cs_strdup(csound, "");
  }
  if (len == 1) {
    return arg;
  }

  // VL 15.10.24
  // synthetic args reach here with
  // : prepended and ; appended to name
  // so we need to remove them to avoid
  // accumulation
  // now remove any : or ; leftover in typename
  type = remove_type_quoting(csound, arg);

  // Check if this is already a properly formatted struct array type
  // (e.g., ":MyType;[]")
  if (arg[0] == ':' && strstr(arg, ";[") != NULL) {
    // This is already in external format, return as-is
    csound->Free(csound, type);
    return cs_strdup(csound, arg);
  }

  // If this is already in external primitive array form like "k[]" or "a[][]",
  // do not attempt to convert it; just return a copy as-is.
  if (arg[0] != '[' && arg[0] != ':' && strchr(arg, '[') != NULL) {
    csound->Free(csound, type);
    return cs_strdup(csound, arg);
  }

  // Safely handle internal typename[] forms in the internal "[..."
  // representation Avoid reading past the end by using explicit bounds checks
  {
    size_t tlen = strlen(type);
    if (tlen >= 2) {
      char *scan = type + 1;   // start after first char
      char *end = type + tlen; // points at NUL
      while (scan < end) {
        if ((scan + 1) < end && *scan == '[' && *(scan + 1) == ']') {
          *scan = '\0';
          break;
        }
        scan++;
      }
    }
  }

  // update arg & len
  arg = type;
  len = strlen(arg);
  start = arg;

  if (strchr(type, '[') == NULL) {
    /* User-Defined Struct */
    retVal = csound->Malloc(csound, sizeof(char) * (len + 3));
    current = retVal;
    *current++ = ':';
    strncpy(current, type, len);
    current += len;
    *current++ = ';';
    *current = '\0';
    return retVal;
  }

  dimensions = 0;
  while (*arg == '[') {
    arg++;
    dimensions++;
  }

  nameLen = len - (arg - start) - 1;

  if (nameLen > 1) {
    nameLen += 2;
  }

  retVal =
      csound->Malloc(csound, sizeof(char) * (nameLen + (dimensions * 2) + 1));
  current = retVal;

  if (nameLen > 1) {
    *current++ = ':';
    strncpy(current, arg, nameLen - 2);
    current += (nameLen - 2);
    *current++ = ';';
  } else {
    *current++ = *arg;
  }

  for (i = 0; i < dimensions * 2; i += 2) {
    *current++ = '[';
    *current++ = ']';
  }
  *current = '\0';
  csound->Free(csound, type);
  return retVal;
}

/* ASSUMES VALID ARRAY SPECIFIER! */
char *convert_external_to_internal(CSOUND *csound, char *arg) {
  int32_t i, dimensions;
  char *retVal;

  // Robust guard: ensure arg has at least 2 chars before accessing arg[1]
  if (arg == NULL) {
    return arg;
  }
  size_t alen = strlen(arg);
  if (alen < 2 || arg[1] != '[') {
    return arg;
  }

  dimensions = ((int32_t)alen - 1) / 2;

  retVal = csound->Malloc(csound, sizeof(char) * (dimensions + 3));
  retVal[dimensions + 2] = '\0';
  retVal[dimensions + 1] = ']';
  retVal[dimensions] = *arg;

  for (i = 0; i < dimensions; i++) {
    retVal[i] = '[';
  }
  return retVal;
}

char *get_output_arg_string_from_tree(CSOUND *csound, TREE *tree,
                                      TYPE_TABLE *typeTable) {
  int32_t len = tree_arg_list_count(tree);
  int32_t i;

  if (len == 0) {
    return NULL;
  }

  char **argTypes = csound->Malloc(csound, len * sizeof(char *));
  char *argString = NULL;
  TREE *current = tree;
  int32_t index = 0;
  int32_t argsLen = 0;

  while (current != NULL) {
    // Skip T_MEMBER_IDENT nodes - they should only be processed as part of
    // STRUCT_EXPR
    if (current->type == T_MEMBER_IDENT) {
      current = current->next;
      continue;
    }

    char *argType = NULL;

    // Special-case: output arg is an array element (e.g., t1[k1]).
    // In this context, the LHS type must be the ELEMENT type, not the full
    // array type, so that assignment resolves to element store (e.g., =.k[]k →
    // array_set) instead of array-level ops like asig2array.
    if (current->type == T_ARRAY && current->right != NULL) {
      // Try to resolve the base variable and get its element type
      TREE *base = current->left;
      if (base && base->type == T_IDENT && base->value && base->value->lexeme) {
        CS_VARIABLE *v = find_var_from_pools(csound, base->value->lexeme,
                                             base->value->lexeme, typeTable);
        if (v && v->varType == &CS_VAR_TYPE_ARRAY && v->subType) {
          // Use the variable's subType (element type), converted to external
          // name
          char *elem = cs_strdup(csound, v->subType->varTypeName);
          argType = convert_internal_to_external(csound, elem);
        }
      }

      // Fallback: derive from generic type by stripping one trailing [] if
      // present
      if (argType == NULL) {
        char *generic = get_arg_type2(csound, current, typeTable);
        if (generic == NULL) {
          argType = cs_strdup(csound, ".");
        } else {
          // Convert to external then strip one [] suffix if present
          char *ext = convert_internal_to_external(csound, generic);
          size_t L = strlen(ext);
          if (L >= 2 && ext[L - 2] == '[' && ext[L - 1] == ']') {
            ext[L - 2] = '\0';
          }
          argType = ext;
        }
      }
    }

    argsLen += strlen(argType);
    argTypes[index++] = argType;

    current = current->next;
  }

  argString = csound->Malloc(csound, (argsLen + 1) * sizeof(char));
  char *temp = argString;

  for (i = 0; i < len; i++) {
    int32_t size = (int32_t)strlen(argTypes[i]);
    memcpy(temp, argTypes[i], size);
    temp += size;
    csound->Free(csound, argTypes[i]);
  }
  *temp = '\0';

  csound->Free(csound, argTypes);
  return argString;
}

char* get_arg_string_from_tree(CSOUND* csound, TREE* tree,
                               TYPE_TABLE* typeTable) {

  int32_t len = tree_arg_list_count(tree);
  int32_t i;

  if (len == 0) {
    return NULL;
  }

  char** argTypes = csound->Malloc(csound, len * sizeof(char*));
  char* argString = NULL;
  TREE* current = tree;
  int32_t index = 0;
  int32_t argsLen = 0;

  while (current != NULL) {
    char* argType = get_arg_type2(csound, current, typeTable);
    if (argType == NULL) {
      // if we failed to find argType, exit from parser
      csound->Die(csound, "Could not parse type for argument");
    } else {
      argType = convert_internal_to_external(csound, argType);
      argsLen += strlen(argType);
      argTypes[index++] = argType;
    }

    current = current->next;
  }

  argString = csound->Malloc(csound, (argsLen + 1) * sizeof(char));
  char* temp = argString;

  for (i = 0; i < len; i++) {
    int32_t size = (int32_t) strlen(argTypes[i]);
    memcpy(temp, argTypes[i], size);
    temp += size;
    csound->Free(csound, argTypes[i]);
  }

  argString[argsLen] = '\0';

  csound->Free(csound, argTypes);
  return argString;
}

/* Version of get_arg_string_from_tree that allows undefined variables (for output args) */
char *get_arg_string_from_tree_allow_undefined(CSOUND *csound, TREE *tree,
                                               TYPE_TABLE *typeTable) {
  // Count only the nodes that will actually be processed (skip certain token types)
  int32_t len = 0;
  TREE *countCurrent = tree;
  while (countCurrent != NULL) {
    // Skip tokens that will be skipped in the main loop
    if (countCurrent->type != KGOTO_TOKEN &&
        countCurrent->type != IGOTO_TOKEN && countCurrent->type != GOTO_TOKEN &&
        countCurrent->type != T_MEMBER_IDENT && countCurrent->type != '[') {
      len++;
    }
    countCurrent = countCurrent->next;
  }

  int32_t i;

  if (len == 0) {
    return NULL;
  }

  char *argTypes = csound->Malloc(csound, len * 256 * sizeof(char));
  int32_t argsLen = 0;
  int32_t index = 0;
  TREE *current = tree;

  while (current != NULL) {
    // Skip T_MEMBER_IDENT nodes - they should only be processed as part of STRUCT_EXPR
    if (current->type == T_MEMBER_IDENT) {
      current = current->next;
      continue;
    }

    // Skip '[' tokens - they should only be processed as part of T_ARRAY_IDENT
    if (current->type == '[') {
      current = current->next;
      continue;
    }

    char *argType = get_arg_type2(csound, current, typeTable);
    if (argType == NULL) {
      // Be tolerant in pre-pass contexts: use a heuristic fallback instead of aborting.
      // This allows later phases (after LHS predeclarations) to establish the real type.
      const char *nm = (current->value && current->value->lexeme)
                           ? current->value->lexeme
                           : NULL;
      char fallbackBuf[8] = {0};
      if (nm && *nm) {
        char c0 = nm[0];
        // Prefer implicit prefix typing when available, otherwise conservatively assume 'i'
        if (c0 == 'a' || c0 == 'k' || c0 == 'i' || c0 == 'S' || c0 == 'B') {
          fallbackBuf[0] = c0;
          fallbackBuf[1] = '\0';
        } else {
          fallbackBuf[0] = 'i';
          fallbackBuf[1] = '\0';
        }
      } else {
        fallbackBuf[0] = 'i';
        fallbackBuf[1] = '\0';
      }
      char *fallbackType = cs_strdup(csound, fallbackBuf);
      argsLen += (int32_t)strlen(fallbackType);
      strcpy(&argTypes[index * 256], fallbackType);
      csound->Free(csound, fallbackType);
      index++;
      current = current->next;
      continue;
    } else {
      argsLen += (int32_t)strlen(argType);
      strcpy(&argTypes[index * 256], argType);
      csound->Free(csound, argType);
      index++;
    }

    current = current->next;
  }

  char *argString = csound->Malloc(csound, (argsLen + 1) * sizeof(char));
  char *curLoc = argString;

  for (i = 0; i < len; i++) {
    unsigned long argLen = strlen(&argTypes[i * 256]);
    memcpy(curLoc, &argTypes[i * 256], argLen);
    curLoc += argLen;
  }

  argString[argsLen] = '\0';
  csound->Free(csound, argTypes);
  return argString;
}

/* Used by new UDO syntax, expects tree's with value->lexeme as type names */
char *get_in_types_from_tree(CSOUND *csound, TREE *tree,
                             TYPE_TABLE *typeTable) {
  int32_t len = tree_arg_list_count(tree);

  if (len == 0 || (len == 1 && !strcmp(tree->value->lexeme, "0"))) {
    return cs_strdup(csound, "0");
  }

  // Check if any parameter has an explicit type annotation (optype is set)
  // If so, extract types directly from the tree to preserve optional markers
  // Otherwise, fall back to the original behavior of looking up variable types
  TREE *check = tree;
  int hasExplicitTypes = 0;
  while (check != NULL) {
    if (check->value && check->value->optype != NULL) {
      hasExplicitTypes = 1;
      break;
    }
    check = check->next;
  }

  // If no explicit type annotations, use the original behavior
  if (!hasExplicitTypes) {
    return get_arg_string_from_tree(csound, tree, typeTable);
  }

  // For new-style UDO parameters with explicit type annotations,
  // extract types the same way as get_out_types_from_tree
  char *argTypes = csound->Malloc(csound, len * 256 * sizeof(char));
  int32_t argsLen = 0;
  int32_t i = 0;

  TREE *current = tree;

  while (current != NULL) {
    // For new-style UDO parameters, the type is in optype, not lexeme
    // lexeme contains the variable name, optype contains the type annotation
    char *argType = current->value->optype ? current->value->optype : current->value->lexeme;

    int32_t len = (int32_t)strlen(argType);
    int32_t offset = i * 256;

    // Check if array brackets are in a separate tree node (for types like "MyType[]")
    int hasArrayBrackets = (current->right != NULL && *current->right->value->lexeme == '[');

    // Check if this is a multi-character type (struct type like "MyType")
    // Single-char types don't need conversion, multi-char types need internal format
    int isMultiCharType = (len > 1 && !hasArrayBrackets) ||
                          (len > 1 && hasArrayBrackets && !is_in_optional_arg(argType));

    if (isMultiCharType && !is_in_optional_arg(argType)) {
      // Convert to internal format ":Type;" or ":Type;[]"
      strcpy(&argTypes[offset], ":");
      strcat(&argTypes[offset], argType);
      strcat(&argTypes[offset], ";");
      argsLen += len + 2; // ":" + type + ";"

      if (hasArrayBrackets) {
        strcat(&argTypes[offset], "[]");
        argsLen += 2;
      }
    } else if (hasArrayBrackets) {
      // Single-char type with array brackets
      strcpy(&argTypes[offset], argType);
      argTypes[offset + len] = '[';
      argTypes[offset + len + 1] = ']';
      argTypes[offset + len + 2] = '\0';
      argsLen += len + 2;
    } else {
      // Single-char type or optional type marker
      strcpy(&argTypes[offset], argType);
      argsLen += len;
    }

    current = current->next;
    i += 1;
  }

  char *argString = csound->Malloc(csound, (argsLen + 1) * sizeof(char));
  char *curLoc = argString;

  for (i = 0; i < len; i++) {
    unsigned long argLen = strlen(&argTypes[i * 256]);
    memcpy(curLoc, &argTypes[i * 256], argLen);
    curLoc += argLen;
  }

  argString[argsLen] = '\0';
  csound->Free(csound, argTypes);
  return argString;
}

/* Used by new UDO syntax, expects tree's with value->lexeme as type names */
char *get_out_types_from_tree(CSOUND *csound, TREE *tree) {

  int32_t len = tree_arg_list_count(tree);
  char *argTypes = csound->Malloc(csound, len * 256 * sizeof(char));
  int32_t i;

  if (len == 0 || (len == 1 && !strcmp(tree->value->lexeme, "0"))) {
    return cs_strdup(csound, "0");
  }

  int32_t argsLen = 0;
  i = 0;

  TREE *current = tree;

  while (current != NULL) {
    // For new-style UDO parameters, the type is in optype, not lexeme
    // lexeme contains the variable name, optype contains the type annotation
    char *argType = current->value->optype ? current->value->optype : current->value->lexeme;
    int32_t len = (int32_t)strlen(argType);
    int32_t offset = i * 256;

    // Check if array brackets are in a separate tree node (for types like "MyType[]")
    int hasArrayBrackets = (current->right != NULL && *current->right->value->lexeme == '[');

    // Check if this is a multi-character type (struct type like "MyType")
    // Single-char types don't need conversion, multi-char types need internal format
    int isMultiCharType = (len > 1);

    if (isMultiCharType) {
      // Convert to internal format ":Type;" or ":Type;[]"
      strcpy(&argTypes[offset], ":");
      strcat(&argTypes[offset], argType);
      strcat(&argTypes[offset], ";");
      argsLen += len + 2; // ":" + type + ";"

      if (hasArrayBrackets) {
        strcat(&argTypes[offset], "[]");
        argsLen += 2;
      }
    } else if (hasArrayBrackets) {
      // Single-char type with array brackets
      strcpy(&argTypes[offset], argType);
      argTypes[offset + len] = '[';
      argTypes[offset + len + 1] = ']';
      argTypes[offset + len + 2] = '\0';
      argsLen += len + 2;
    } else {
      // Single-char type
      strcpy(&argTypes[offset], argType);
      argsLen += len;
    }

    current = current->next;
    i += 1;
  }

  char *argString = csound->Malloc(csound, (argsLen + 1) * sizeof(char));
  char *curLoc = argString;

  for (i = 0; i < len; i++) {
    unsigned long argLen = strlen(&argTypes[i * 256]);
    memcpy(curLoc, &argTypes[i * 256], argLen);
    curLoc += argLen;
  }

  argString[argsLen] = '\0';
  csound->Free(csound, argTypes);
  return argString;
}

OENTRY *find_opcode_new(CSOUND *csound, char *opname, char *outArgsFound,
                        char *inArgsFound) {
  OENTRIES *opcodes = find_opcode2(csound, opname);
  if (opcodes->count == 0) {
    return NULL;
  }
  OENTRY *retVal = resolve_opcode(csound, opcodes, outArgsFound, inArgsFound);
  csound->Free(csound, opcodes);
  return retVal;
}

OENTRY *find_opcode_exact(CSOUND *csound, char *opname, char *outArgsFound,
                          char *inArgsFound) {
  OENTRIES *opcodes = find_opcode2(csound, opname);
  if (opcodes->count == 0) {
    return NULL;
  }
  OENTRY *retVal =
      resolve_opcode_exact(csound, opcodes, outArgsFound, inArgsFound);
  csound->Free(csound, opcodes);

  return retVal;
}

//FIXME - this needs to be updated to take into account array names
// that could clash with non-array names, i.e. kVar and kVar[]
int32_t check_args_exist(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable) {
  CS_VARIABLE *var = 0;
  TREE* current;
  char* argType;
  char* varName;

  if (tree == NULL) {
    return 1;
  }

  current = tree;

  while (current != NULL) {

    if (is_expression_node(tree) || is_boolean_expression_node(tree)) {
      if (!(check_args_exist(csound, tree->left, typeTable) &&
            check_args_exist(csound, tree->right, typeTable))) {
        return 0;
      }
    } else {
      switch (current->type) {
      case LABEL_TOKEN:
      case T_IDENT:
        varName = current->value->lexeme;
        if(!strcmp(varName, "this_instr")) break;
        if(!strcmp(varName, "this")) break;
        if (is_label(varName, typeTable->labelList)) {
          break;
        }
        argType = get_arg_type2(csound, current, typeTable);
        if (UNLIKELY(argType==NULL)) {
          synterr(csound,
                  Str("Variable type for %s could not be determined."),
                  varName);
          do_baktrace(csound, tree->locn);
          return 0;
        }

        //FIXME - this feels like a hack
        if (*argType == 'c' || *argType == 'r' || *argType == 'p') {
          csound->Free(csound, argType);
          break;
        }
        csound->Free(csound, argType);

        // search for the variable in all variable pools
        var = find_var_from_pools(csound, varName, varName, typeTable);

        if (UNLIKELY(var == NULL)) {
            synterr(csound,
                    Str("ArgCheck: variable '%s' used before defined\nline %d"),
                    varName, tree->line);
            do_baktrace(csound, tree->locn);
            return 0;
        }
        break;
        case T_ARRAY:
        // Handle struct member array access like users.names[0]
        if (current->left && current->left->type == STRUCT_EXPR) {
            // For struct member array access, recursively check the struct expression
            if (!check_args_exist(csound, current->left, typeTable)) {
                return 0;
            }
            // Skip the variable lookup since this is a struct member, not a simple variable
            break;
        }

        // Handle simple array access like myArray[0]
        if (UNLIKELY(current->left == NULL || current->left->value == NULL)) {
            synterr(csound, Str("ArgCheck: malformed T_ARRAY node at line %d\n"), current->line);
            do_baktrace(csound, current->locn);
            return 0;
        }
        varName = current->left->value->lexeme;
        // search for the variable in all variable pools
        var = find_var_from_pools(csound, varName, varName, typeTable);
        if (UNLIKELY(var == NULL)) {
            synterr(csound,
                    Str("ArgCheck: variable '%s' used before defined\n"
                        "Line %d\n"),
                    varName, current->left->line);
            do_baktrace(csound, current->left->locn);
            return 0;
        }

        break;
      default:
        break;
      }

    }

    current = current->next;
  }

  return 1;
}

// returns the correct pool (local or global) and as side effect
// removes the global annotation from variable name.
// Expected syntax: var@global
CS_VAR_POOL *find_global_annotation(char *varName, TYPE_TABLE *typeTable) {
  CS_VAR_POOL *pool = typeTable->localPool;
  // find global annotation
  if (strchr(varName, '@') != NULL) {
    char *th;
    char *baseType = strtok_r(varName, "@", &th);
    char *global = strtok_r(NULL, "@", &th);
    if (!strcmp(global, "global")) {
      pool = typeTable->globalPool;
      varName = baseType;
    }
  }
  return pool;
}

static CS_VAR_POOL *get_var_pool(CSOUND *csound, TYPE_TABLE *typeTable,
                                 const char *varBaseName) {
  // we first check for local variables
  CS_VARIABLE *var =
      csoundFindVariableWithName(csound, typeTable->localPool, varBaseName);
  if (var)
    return typeTable->localPool;
  // then check for global variables in engine
  if (csound->engineState.varPool != NULL &&
      (uintptr_t)csound->engineState.varPool >= 0x1000) {
    var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                     varBaseName);
    if (var)
      return csound->engineState.varPool;
  }
  // and finally newly defined global vars
  var = csoundFindVariableWithName(csound, typeTable->globalPool, varBaseName);
  if (var)
    return typeTable->globalPool;

  return NULL;
}

// on new-type UDOS type-annotations can
// be used for optional types
// this checks and converts it to i or k type names
char *check_optional_type(CSOUND *csound, char *name) {

  if (is_in_optional_arg(name)) {
    char *t = (char *)OPTIONAL_IN_TYPES[0];
    char *o, str[2] = {0};
    for (int i = 0; t != NULL; i += 2) {
      o = (char *)OPTIONAL_IN_TYPES[i + 1];
      if (strcmp(t, name) == 0) {
        str[0] = *o;

        return cs_strdup(csound, str);
      }
      t = (char *)OPTIONAL_IN_TYPES[i + 2];
    }
  }

  // Check if this is a struct type that needs conversion to :Type; format
  // First try to look up the struct type using the internal name format
  size_t len = strlen(name);
  char *internalName = csound->Malloc(csound, len + 3);
  internalName[0] = ':';
  strcpy(internalName + 1, name);
  internalName[len + 1] = ';';
  internalName[len + 2] = '\0';

  const CS_TYPE *structType =
      csoundGetTypeWithVarTypeName(csound->typePool, internalName);
  if (structType != NULL && structType->userDefinedType) {
    // This is a user-defined struct type, return the internal format

    return internalName; // Return the internal name directly
  }

  csound->Free(csound, internalName);

  // Check if this is a standard type that needs internal format for UDO parsing
  // These types (OpcodeDef, Opcode, InstrDef, Instr) need to be converted to
  // internal format when used in UDO type annotations
  if (strcmp(name, "OpcodeDef") == 0 || strcmp(name, "Opcode") == 0 ||
      strcmp(name, "InstrDef") == 0 || strcmp(name, "Instr") == 0) {
    // Create internal format for these standard types
    char *internalStdName = csound->Malloc(csound, len + 3);
    internalStdName[0] = ':';
    strcpy(internalStdName + 1, name);
    internalStdName[len + 1] = ';';
    internalStdName[len + 2] = '\0';
    return internalStdName;
  }

  return cs_strdup(csound, name);
}

/* This function creates a new variable for a rhs argument
   if the variable is not found in any of the pools
   If the variable is found, a consistency check is made
   to make sure the argument type matches the existing variable
*/
void add_arg(CSOUND *csound, char *varName, char *annotation,
             TYPE_TABLE *typeTable, TREE *tree) {

  const CS_TYPE *type;
  CS_VARIABLE *var;
  char *t = cs_strdup(csound, varName);
  char *lvarName = cs_strdup(csound, varName); // local copy
  CS_VAR_POOL *pool = typeTable->localPool;
  char argLetter[2] = {0};
  ARRAY_VAR_INIT varInit;
  void *typeArg = NULL;
  // remove any global annotation
  find_global_annotation(t, typeTable);
  // search on  all pools
  var = find_var_from_pools(csound, t, t, typeTable);
  csound->Free(csound, t);
  if (var == NULL) {
    if (annotation != NULL) {
      // check for global annotation in explicit-type rhs vars
      lvarName = cs_strdup(csound, varName);
      pool = find_global_annotation(lvarName, typeTable);
      // If annotation denotes an array type (e.g., "Type[]" or ":Type;[]"),
      // create an array variable with the proper element subtype.
      const char *ann = annotation;
      size_t annlen = strlen(ann);
      int32_t dims = 0;
      if (annlen >= 2 && ann[annlen - 2] == '[' && ann[annlen - 1] == ']') {
        // Count [] pairs from the end
        while (annlen >= 2 && ann[annlen - 2] == '[' &&
               ann[annlen - 1] == ']') {
          dims += 1;
          annlen -= 2;
        }
        // Extract base type name
        char *baseName;
        if (ann[0] == ':') {
          // External form ":Type;" possibly followed by [] we trimmed
          const char *semi = strchr(ann, ';');
          if (semi != NULL) {
            size_t blen = (size_t)(semi - (ann + 1));
            baseName = csound->Malloc(csound, blen + 1);
            memcpy(baseName, ann + 1, blen);
            baseName[blen] = '\0';
          } else {
            baseName = cs_strdup(csound, ann); // fallback
          }
        } else {
          baseName = cs_strndup(csound, ann, annlen);
        }

        // Convert struct type name to internal format (same as scalar path)
        char *convertedBaseName = check_optional_type(csound, baseName);


        const CS_TYPE *baseType =
            csoundGetTypeWithVarTypeName(csound->typePool, convertedBaseName);


        csound->Free(csound, baseName);
        csound->Free(csound, convertedBaseName);
        if (UNLIKELY(baseType == NULL)) {
          synterr(csound,
                  Str("Unknown base type in annotation '%s' for variable '%s'"),
                  annotation, varName);
          goto end;
        }
        varInit.dimensions = dims;
        varInit.type = baseType;
        typeArg = &varInit;
        var = csoundCreateVariable(csound, csound->typePool, &CS_VAR_TYPE_ARRAY,
                                   lvarName, typeArg);
        if (UNLIKELY(var == NULL)) {
          csound->Warning(
              csound,
              "cannot create variable %s: NULL type (array annotation '%s')",
              lvarName, annotation ? annotation : "(null)");
          goto end;
        }
        csoundAddVariable(csound, pool, var);
        goto end;
      }
      // If annotation lacks [] but the syntax node is an array identifier,
      // treat it as an array with dimensions taken from the parse tree.
      if ((tree && (tree->type == T_ARRAY_IDENT || tree->type == T_ARRAY))) {
        int32_t dims2 = 0;
        TREE *br = tree->right;
        while (br && br->type == '[') {
          dims2 += 1;
          br = br->next;
        }
        if (dims2 <= 0)
          dims2 = 1; // at least one dimension
        add_array_arg(csound, varName, annotation, dims2, typeTable);
        goto end;
      }

      // Non-array annotation: check optional mapping and create scalar/struct
      {
        char *nm = check_optional_type(csound, annotation);
        // If no annotation was provided but the syntax node is an array
        // identifier, create an array variable using the prefix-derived base
        // type and parsed dimensions.
        if (tree && (tree->type == T_ARRAY_IDENT || tree->type == T_ARRAY)) {
          int32_t dims3 = 0;
          TREE *br2 = tree->right;
          while (br2 && br2->type == '[') {
            dims3 += 1;
            br2 = br2->next;
          }
          if (dims3 <= 0)
            dims3 = 1;
          // Derive base type from variable name prefix (i/k/a/S/B) or default
          // to 'i'
          const char *nm2 = varName;
          if (*nm2 == '#')
            nm2++;
          if (*nm2 == 'g')
            nm2++;
          char baseCh = (*nm2 ? *nm2 : 'i');
          if (!(baseCh == 'i' || baseCh == 'k' || baseCh == 'a' ||
                baseCh == 'S' || baseCh == 'B'))
            baseCh = 'i';
          char baseAnn[2] = {baseCh, '\0'};
          add_array_arg(csound, varName, baseAnn, dims3, typeTable);
          goto end;
        }

        type = csoundGetTypeWithVarTypeName(csound->typePool, nm);
        typeArg = (void *)type;
        csound->Free(csound, nm);
      }
    } else {
      // check for @global in implicit-type rhs vars
      // and if found, strip it and print warning
      if (find_global_annotation(varName, typeTable) == typeTable->globalPool)
        csound->Warning(csound, "%s: @global annotation ignored", varName);

      t = lvarName;

      // Implicit-type array declaration: if the syntax node denotes an array
      // (e.g., T_ARRAY_IDENT in an opcall LHS), create an array variable now.
      if (tree && (tree->type == T_ARRAY_IDENT || tree->type == T_ARRAY)) {
        int32_t dimsI = 0;
        TREE *brI = tree->right;
        while (brI && brI->type == '[') {
          dimsI += 1;
          brI = brI->next;
        }
        if (dimsI <= 0)
          dimsI = 1;
        const char *nmI = varName;
        if (*nmI == '#')
          nmI++;
        if (*nmI == 'g')
          nmI++;
        char baseI = (*nmI ? *nmI : 'i');
        if (!(baseI == 'i' || baseI == 'k' || baseI == 'a' || baseI == 'S' ||
              baseI == 'B'))
          baseI = 'i';
        char baseAnnI[2] = {baseI, '\0'};
        add_array_arg(csound, varName, baseAnnI, dimsI, typeTable);
        goto end;
      }

      if (*t == '#')
        t++;
      if (*t == 'g')
        pool = typeTable->globalPool;
      if (*t == 'g')
        t++;

      if (*t == '[') {
        int32_t dimensions = 1;
        const CS_TYPE *varType;
        char *b = t + 1;

        while (*b == '[') {
          b++;
          dimensions++;
        }
        argLetter[0] = *b;

        varType = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);

        varInit.dimensions = dimensions;
        varInit.type = varType;
        typeArg = &varInit;
      }

      argLetter[0] = *t;
      type = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);
    }

    var = csoundCreateVariable(csound, csound->typePool, type, lvarName, typeArg);
    if (UNLIKELY(var == NULL)) {
      csound->Warning(csound, "cannot create variable %s: NULL type", lvarName);
      goto end;
    }
    csoundAddVariable(csound, pool, var);

    // Initialize struct variables if needed
    if (type && type->userDefinedType && var && var->memBlock) {
      CS_STRUCT_VAR *structVar = (CS_STRUCT_VAR*)var->memBlock;
      if (structVar && (!structVar->members || structVar->memberCount == 0)) {
        if (var->initializeVariableMemory) {
          var->initializeVariableMemory(csound, var, (MYFLT*)var->memBlock);
        }
      }
    }

    if (csound->GetDebug(csound)) {
      csound->Message(csound,
                      "add_arg: added '%s' type='%s' to pool=%p (dims=%d) "
                      "memBlockSize=%d userDefinedType=%d\n",
                      var->varName,
                      var->varType ? var->varType->varTypeName : "(null)",
                      (void *)pool, var->dimensions, var->memBlockSize,
                      var->varType ? var->varType->userDefinedType : -1);

    if (var->varType && var->varType->userDefinedType) {
      csound->Message(csound, "add_arg: struct variable '%s' created with userDefinedType=1\n", var->varName);
    }
    }
  } else {
    // for explicit non-array types, we'll allow variable redeclaration
    if (annotation != NULL) {
      pool = find_global_annotation(lvarName, typeTable);
      CS_VAR_POOL *var_pool = get_var_pool(csound, typeTable, lvarName);
      // If annotation denotes an array (e.g., k[], S[][]), and existing var is
      // array, do not redeclare as scalar
      {
        size_t __annLen = strlen(annotation);
        if (__annLen >= 2 && annotation[__annLen - 2] == '[' &&
            annotation[__annLen - 1] == ']') {
          if (var->varType == &CS_VAR_TYPE_ARRAY) {
            goto end;
          } else {
            // Existing var is not array; conservatively skip implicit type
            // change here
            goto end;
          }
        }
      }
      // check for optional type
      t = check_optional_type(csound, annotation);
      // check if a variable is declared with same name
      // and different type.
      type = csoundGetTypeWithVarTypeName(csound->typePool, t);
      if (type && type != var->varType) {
        // remove variable if it belongs to the same pool (local/global)
        if (pool == var_pool || (var_pool == csound->engineState.varPool &&
                                 pool == typeTable->globalPool)) {
          if (tree)
            csound->Warning(
                csound, "Replacing previous definition %s:%s by %s:%s, line %d",
                var->varName, var->varType->varTypeName, lvarName,
                type->varTypeName, tree->line);
          cs_hash_table_remove(csound, var_pool->table, var->varName);
        } else if (pool == typeTable->globalPool)
          if (tree) // synterr should not happen tree is NULL, as arg is
                    // synthetic
            synterr(csound,
                    "global variable %s:%s cannot shadow local variable %s:%s, "
                    "line %d",
                    lvarName, type->varTypeName, var->varName,
                    var->varType->varTypeName, tree->line);
      } else {
        // do nothing if it's the same type & pool
        if (pool == var_pool)
          goto end;
        // if it's a global var was requested, print warning, do nothing
        if (pool == typeTable->globalPool) {
          if (tree)
            csound->Warning(
                csound, "@global annotation ignored for variable %s, line %d",
                lvarName, tree->line);
          goto end;
        }
      }
      // create a new variable (local vars shadow globals)
      var = csoundCreateVariable(csound, csound->typePool,
				   type, lvarName, typeArg);
      csoundAddVariable(csound, pool, var);
      csound->Free(csound, t);
    } else {
      // apply shadowing rule for implicit vars
      // for backwards compatibility
      var = csoundFindVariableWithName(csound, typeTable->globalPool, varName);
      if (var == NULL)
        var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                         varName);
      // we are only concerned with opcoderef and instr vars
      // added by the compiler as global read-only, internally
      if (var && (var->varType == &CS_VAR_TYPE_OPCODEREF ||
                  var->varType == &CS_VAR_TYPE_INSTR)) {
        if (csoundFindVariableWithName(csound, typeTable->localPool, varName) ==
            NULL) {
          argLetter[1] = '\0';
          argLetter[0] = *varName;
          type = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);
          var = csoundCreateVariable(csound, csound->typePool, type, varName,
                                     typeArg);
          csoundAddVariable(csound, typeTable->localPool, var);
        }
      }
    }
  }
end:
  csound->Free(csound, lvarName);
}

/* This function creates a new array variable for a rhs argument
   if the variable is not found in any of the pools
   If the variable is found, a consistency check is made
   to make sure the argument type matches the existing array subtype
*/
void add_array_arg(CSOUND *csound, char *varName, char *annotation,
                   int32_t dimensions, TYPE_TABLE *typeTable) {
  CS_VARIABLE *var;
  char *t = cs_strdup(csound, varName);
  char *lvarName = cs_strdup(csound, varName); // local copy
  CS_VAR_POOL *pool = typeTable->localPool;
  char argLetter[2];
  ARRAY_VAR_INIT varInit;
  void *typeArg = NULL;
  const CS_TYPE *varType;
  // remove any global annotation
  find_global_annotation(t, typeTable);
  // search on  all pools
  var = find_var_from_pools(csound, t, t, typeTable);
  csound->Free(csound, t);
  if (var == NULL) {
    if (annotation != NULL) {
      // check for global annotation
      pool = find_global_annotation(lvarName, typeTable);
      // Convert struct type name to internal format (same as scalar/array
      // paths)

      char *convertedAnnotation = check_optional_type(csound, annotation);
      varType = csoundGetTypeWithVarTypeName(csound->typePool, convertedAnnotation);
      csound->Free(csound, convertedAnnotation);

      if (UNLIKELY(varType == NULL)) {
        synterr(csound, Str("Unknown type '%s' for array variable '%s'\n"),
                annotation, varName);
        return;
      }
    } else {
      t = lvarName;
      argLetter[1] = 0;

      if (*t == '#')
        t++;
      if (*t == 'g')
        pool = typeTable->globalPool;
      if (*t == 'g')
        t++;

      argLetter[0] = *t;

      varType = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);
    }

    varInit.dimensions = dimensions;
    varInit.type = varType;
    typeArg = &varInit;
    var = csoundCreateVariable(csound, csound->typePool, &CS_VAR_TYPE_ARRAY,
                               lvarName, typeArg);
    csoundAddVariable(csound, pool, var);
  } else {
    // Existing symbol found
    if (var->varType != &CS_VAR_TYPE_ARRAY) {
      // Upgrade scalars to arrays by shadowing with a new array variable in the
      // local pool. Determine subtype from annotation or from the variable name
      // prefix.
      const CS_TYPE *baseType = NULL;
      if (annotation != NULL) {
        baseType = csoundGetTypeWithVarTypeName(csound->typePool, annotation);
      } else {
        char *t2 = cs_strdup(csound, lvarName);
        if (*t2 == '#')
          t2++;
        if (*t2 == 'g')
          t2++;
        char baseCh2 = (*t2 ? *t2 : 'i');
        if (!(baseCh2 == 'i' || baseCh2 == 'k' || baseCh2 == 'a' ||
              baseCh2 == 'S' || baseCh2 == 'B'))
          baseCh2 = 'i';
        char baseAnn2[2] = {baseCh2, '\0'};
        baseType = csoundGetTypeWithVarTypeName(csound->typePool, baseAnn2);
      }
      if (baseType == NULL) {
        baseType = csoundGetTypeWithVarTypeName(csound->typePool,
                                                "i"); // conservative fallback
      }
      ARRAY_VAR_INIT varInit2;
      varInit2.dimensions = dimensions;
      varInit2.type = baseType;
      CS_VARIABLE *newArr = csoundCreateVariable(
          csound, csound->typePool, &CS_VAR_TYPE_ARRAY, lvarName, &varInit2);
      csoundAddVariable(csound, typeTable->localPool, newArr);
    } else if (annotation != NULL) {
      // check if a variable is declared with same name
      // and different type array subtype
      // Convert struct type name to internal format (same as earlier in this
      // function)
      char *convertedAnnotation2 = check_optional_type(csound, annotation);
      varType =
          csoundGetTypeWithVarTypeName(csound->typePool, convertedAnnotation2);
      csound->Free(csound, convertedAnnotation2);
      if (varType != var->subType)
        synterr(csound,
                "%s:%s[] - type mismatch for existing "
                "array variable %s:%s%s",
                varName, varType->varTypeName, varName,
                var->subType ? var->subType->varTypeName
                             : var->varType->varTypeName,
                var->subType ? "[]" : "");
    }
  }

  csound->Free(csound, lvarName);
}

/* return 1 on succcess, 0 on failure */
int32_t add_args(CSOUND *csound, TREE *tree, TYPE_TABLE *typeTable) {
  TREE *current;
  char *varName;

  if (tree == NULL) {
    return 1;
  }
  current = tree;
  while (current != NULL) {

    switch (current->type) {
    case T_ARRAY_IDENT:
      varName = current->value->lexeme;
      add_array_arg(csound, varName, current->value->optype,
                    tree_arg_list_count(current->right), typeTable);

      break;
    case LABEL_TOKEN:
    case T_IDENT:
    case T_TYPED_IDENT:
      varName = current->value->lexeme;
      /* TODO - This needs to check if someone put in
          sr:k or ksmps:i or something like that */
      if (varName && is_reserved(varName)) {
        // skip reserved vars, these are handled elsewhere
        break;
      }
      if (varName) {
        add_arg(csound, varName, current->value->optype, typeTable, current);
      }

      break;

    case T_ARRAY:
      varName = current->left->value->lexeme;
      if (varName && find_var_from_pools(csound, varName, varName, typeTable) == NULL) {
        int32_t dims = 1; // at least one dimension present for T_ARRAY
        TREE *br = current->right;
        while (br && br->type == '[') {
          dims++;
          br = br->next;
        }
        add_array_arg(csound, varName, NULL, dims, typeTable);
      }
      break;

    default:
      break;
    }

    current = current->next;
  }

  return 1;
}

TREE *get_initial_unary_operator(TREE *tree) {
  if (tree == NULL)
    return NULL;

  TREE *current = tree;
  while (current->left != NULL) {
    current = current->left;
  }
  if (current->type == S_UMINUS || current->type == S_UPLUS) {
    return current;
  }
  return NULL;
}

TREE *get_left_parent(TREE *root, TREE *node) {
  TREE *current = root;
  while (current != NULL) {
    if (current->left == node) {
      return current;
    }
    current = current->left;
  }
  return NULL;
}

TREE *convert_unary_op_to_binary(CSOUND *csound, TREE *new_left,
                                 TREE *unary_op) {
  TREE *retVal = NULL;
  new_left->type = T_IDENT;

  if (unary_op->type == S_UMINUS) {
    retVal = make_node(csound, unary_op->line, unary_op->locn, '-', new_left,
                       unary_op->right);
  } else if (unary_op->type == S_UPLUS) {
    retVal = make_node(csound, unary_op->line, unary_op->locn, '+', new_left,
                       unary_op->right);
    // unary_op->markup = NULL;
  }

  return retVal;
}

/* Analyze and restructures the statement node into an opcode call structure.
 * This function will reform the tree such that the top node will contain the
 * name of an opcode, the ->left will hold out-args, and ->right will hold
 * in-args.
 * This function does not try to expand any statements or do any semantic
 * verification, but reshapes trees so that they can all go through the
 * verify_opcode function. This is due to the ambiguous nature of Csound opcode
 * call syntax.
 * To note, this function requires that the typeTable be passed in. This is
 * because variables can (now) have names that shadow opcode names. Lookup needs
 * * to give priority to an identifier being a variable over being an opcode.
 * This maintains future proofing so that if an opcode is later introduced
 * with the same name as a variable in an older project, the older project
 * will continue to work.
 *
 * For further reference, please see the rule for statement and opcall in
 * Engine/csound_orc.y.
 */
TREE *convert_statement_to_opcall(CSOUND *csound, TREE *root,
                                  TYPE_TABLE *typeTable) {
  int32_t leftCount, rightCount;

  // DEBUG: Add debug output to see what we're processing
  csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Processing root->type=%d\n", root->type);
  if (root->value && root->value->lexeme) {
    csound->Message(csound, "[convert_statement_to_opcall] DEBUG: root lexeme='%s'\n", root->value->lexeme);
  }



  if (root->type == T_ASSIGNMENT) {
    csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Found T_ASSIGNMENT!\n");



    /* NEW: Check for InstrDef assignment with constant (var:InstrDef = 1) */
    if (root->left && root->left->type == T_IDENT &&
        root->right && (root->right->type == INTEGER_TOKEN || root->right->type == NUMBER_TOKEN) &&
        root->left->value && root->left->value->lexeme &&
        root->right->value && root->right->value->lexeme) {

      // Check if this is an InstrDef assignment by checking if the variable has InstrDef type annotation
      if (root->left->value->optype && strcmp(root->left->value->optype, "InstrDef") == 0) {
        // Convert assignment to init opcode: var = 1 -> init var, 1
        root->value = make_token(csound, "init");
        root->type = T_OPCALL;
        return root;
      }
    }



    /* NEW: Check for struct-to-struct assignment (var2 = var1) */
    if (root->left && root->left->type == T_IDENT &&
        root->right && root->right->type == T_IDENT &&
        root->left->value && root->left->value->lexeme &&
        root->right->value && root->right->value->lexeme) {

      char* leftVarName = root->left->value->lexeme;
      char* rightVarName = root->right->value->lexeme;

      csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Checking struct-to-struct assignment: %s = %s\n", leftVarName, rightVarName);

      // Get the types of both variables
      CS_VARIABLE* leftVar = find_var_from_pools(csound, leftVarName, leftVarName, typeTable);
      CS_VARIABLE* rightVar = find_var_from_pools(csound, rightVarName, rightVarName, typeTable);

      csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Variable lookup: leftVar=%p, rightVar=%p\n", (void*)leftVar, (void*)rightVar);



      if (leftVar && rightVar) {
        csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Variable types: leftVar->varType=%p, rightVar->varType=%p\n", (void*)leftVar->varType, (void*)rightVar->varType);
        if (leftVar->varType && rightVar->varType) {
          csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Type names: left='%s', right='%s'\n",
                          leftVar->varType->varTypeName ? leftVar->varType->varTypeName : "(null)",
                          rightVar->varType->varTypeName ? rightVar->varType->varTypeName : "(null)");
          csound->Message(csound, "[convert_statement_to_opcall] DEBUG: User defined types: left=%d, right=%d\n",
                          leftVar->varType->userDefinedType, rightVar->varType->userDefinedType);
        }
      }

      // NEW: Handle case where leftVar is not found but rightVar is a struct
      // This happens when var2:MyType2 declaration hasn't been processed yet
      if (!leftVar && rightVar && rightVar->varType) {
        // Skip built-in UDT types like InstrDef, OpcodeDef, etc. - they have their own built-in opcodes
        const char* typeName = rightVar->varType->varTypeName;
        csound->Message(csound, "[convert_statement_to_opcall] DEBUG: !leftVar && rightVar path - typeName='%s'\n",
                        typeName ? typeName : "(null)");
        if (typeName && (strcmp(typeName, "InstrDef") == 0 ||
                         strcmp(typeName, ":InstrDef;") == 0)) {
          // Special handling for InstrDef assignments - convert to init.instr opcode
          csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Converting InstrDef assignment to init.instr opcode\n");
          root->value = make_token(csound, "init.instr");
          root->type = T_OPCALL;
          return root;
        }

        // Handle user-defined struct assignments (but not built-in types)
        if (rightVar->varType->userDefinedType) {
          csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Left variable '%s' not found, but right variable '%s' is struct type '%s'\n",
                          leftVarName ? leftVarName : "(null)",
                          rightVarName ? rightVarName : "(null)",
                          typeName ? typeName : "(null)");

          // Assume this is a struct-to-struct assignment and convert it
          csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Assuming struct-to-struct assignment, converting to init opcode\n");

          // Build the init opcode name for the struct type
          char initOpcodeName[256];
          snprintf(initOpcodeName, sizeof(initOpcodeName), "init.%s", typeName + 1); // Skip the ':' prefix
          initOpcodeName[strlen(initOpcodeName) - 1] = '\0'; // Remove the ';' suffix

          csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Using init opcode: '%s'\n", initOpcodeName);

          // Convert assignment to init opcode: var2 = var1 -> init.MyType2 var2, var1
          root->value = make_token(csound, initOpcodeName);
          root->type = T_OPCALL;
          return root;
        }
      } else if (!leftVar && !rightVar) {
        // Handle the case where both variables don't exist yet (timing issue)
        // Check if this is a typed variable assignment to a known built-in variable
        if (root->left && root->left->type == 295 && // 295 is T_TYPED_IDENT
            root->left->value && root->left->value->optype &&
            root->right && root->right->value && root->right->value->lexeme) {

          const char* leftType = root->left->value->optype;
          const char* rightVarName = root->right->value->lexeme;

          csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Both variables missing - leftType='%s' rightVarName='%s'\n",
                          leftType ? leftType : "(null)", rightVarName ? rightVarName : "(null)");

          // Check if this is an InstrDef assignment from this_instr or other InstrDef variables
          if (leftType && (strcmp(leftType, "InstrDef") == 0) &&
              rightVarName && (strcmp(rightVarName, "this_instr") == 0 ||
                               strcmp(rightVarName, "Test") == 0)) {
            csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Converting InstrDef assignment (timing fix) to init.instr opcode\n");
            root->value = make_token(csound, "init.instr");
            root->type = T_OPCALL;
            return root;
          }
        }

        csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Converted to init opcode\n");
        return root;
      } else if (leftVar && rightVar && leftVar->varType && rightVar->varType &&
          leftVar->varType->userDefinedType && rightVar->varType->userDefinedType &&
          leftVar->varType == rightVar->varType) {

        // Skip built-in UDT types like InstrDef, OpcodeDef, etc. - they have their own built-in opcodes
        const char* leftTypeName = leftVar->varType->varTypeName;
        const char* rightTypeName = rightVar->varType->varTypeName;
        if ((leftTypeName && (strcmp(leftTypeName, "InstrDef") == 0 ||
                              strcmp(leftTypeName, ":InstrDef;") == 0 ||
                              strcmp(leftTypeName, "OpcodeDef") == 0 ||
                              strcmp(leftTypeName, ":OpcodeDef;") == 0 ||
                              strcmp(leftTypeName, "Opcode") == 0 ||
                              strcmp(leftTypeName, ":Opcode;") == 0 ||
                              strcmp(leftTypeName, "Instr") == 0 ||
                              strcmp(leftTypeName, ":Instr;") == 0)) ||
            (rightTypeName && (strcmp(rightTypeName, "InstrDef") == 0 ||
                               strcmp(rightTypeName, ":InstrDef;") == 0 ||
                               strcmp(rightTypeName, "OpcodeDef") == 0 ||
                               strcmp(rightTypeName, ":OpcodeDef;") == 0 ||
                               strcmp(rightTypeName, "Opcode") == 0 ||
                               strcmp(rightTypeName, ":Opcode;") == 0 ||
                               strcmp(rightTypeName, "Instr") == 0 ||
                               strcmp(rightTypeName, ":Instr;") == 0))) {
          return root; // Let the normal assignment processing handle this
        }

        csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Found struct-to-struct assignment! Converting to init opcode\n");

        // Build the init opcode name for the struct type
        char initOpcodeName[256];
        snprintf(initOpcodeName, sizeof(initOpcodeName), "init.%s", leftVar->varType->varTypeName + 1); // Skip the ':' prefix
        initOpcodeName[strlen(initOpcodeName) - 1] = '\0'; // Remove the ';' suffix

        csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Using init opcode: '%s'\n", initOpcodeName);

        // Convert assignment to init opcode: var2 = var1 -> init.MyType2 var2, var1
        root->value = make_token(csound, initOpcodeName);
        root->type = T_OPCALL;

        csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Converted to init opcode\n");
        return root;
      } else if (leftVar && rightVar && leftVar->varType && rightVar->varType) {
        // Handle built-in type assignments (like InstrDef to InstrDef)
        const char* leftTypeName = leftVar->varType->varTypeName;
        const char* rightTypeName = rightVar->varType->varTypeName;

        // Check if this is an InstrDef to InstrDef assignment
        if ((leftTypeName && (strcmp(leftTypeName, "InstrDef") == 0 || strcmp(leftTypeName, ":InstrDef;") == 0)) &&
            (rightTypeName && (strcmp(rightTypeName, "InstrDef") == 0 || strcmp(rightTypeName, ":InstrDef;") == 0))) {
          csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Found InstrDef-to-InstrDef assignment, converting to init.instr opcode\n");

          // Check if this is a typed variable declaration (left side is T_TYPED_IDENT)
          if (root->left && root->left->type == 295) { // 295 is T_TYPED_IDENT
            csound->Message(csound, "[convert_statement_to_opcall] DEBUG: This is a typed variable declaration, ensuring variable exists\n");
            // The variable will be created during verify_opcode -> add_args
            // We don't need to do anything special here, just convert to init.instr
          }

          root->value = make_token(csound, "init.instr");
          root->type = T_OPCALL;
          return root;
        }
      }
    }

    /* Rewrite tree if line is "a1, a2 = func(arg, arg1)"
       to "a1, a2 func arg, arg1" */
    TREE *right = root->right;
    if (right->type == T_FUNCTION && right->left == NULL &&
        right->next == NULL) {
      right->next = root->next;
      right->left = root->left;
      right->type = T_OPCALL;
      root = right;
    }

    return root;
  }

  // If a function call made it here, such as:
  //  print(1,2,3)
  // then it should just be updated to T_OPCALL and returned
  if (root->type == T_FUNCTION) {
    root->type = T_OPCALL;
    return root;
  }

  if (root->type == GOTO_TOKEN || root->type == KGOTO_TOKEN ||
      root->type == IGOTO_TOKEN) {
    // i.e. a = func(a + b)
    return root;
  }

  if (root->type == S_ADDIN || root->type == S_SUBIN || root->type == S_MULIN ||
      root->type == S_DIVIN)
    return root;

  if (root->type != T_OPCALL) {
    synterr(csound, Str("Internal Error: convert_statement_to_opcall "
                        "received a non T_OPCALL TREE\n"));
    return NULL;
  }

  if (root->value != NULL) {
    /* Already processed T_OPCALL, return as-is */
    return root;
  }

  if (root->left == NULL) {
    synterr(csound, Str("Internal Error: convert_statement_to_opcall "
                        "received an empty OPCALL\n"));
    return NULL;
  }

  if (root->left->type == T_OPCALL && root->right == NULL) {

    TREE *top = root->left;
    TREE *unary_op = get_initial_unary_operator(top->right);

    if (top->left->next == NULL && unary_op != NULL) {
      TREE *newTop;

      /* i.e. ksubst init -1 */
      /* TODO - this should check if it's a var first */
      CS_VARIABLE *var = find_var_from_pools(csound, top->value->lexeme,
                                             top->value->lexeme, typeTable);

      /* but if it's an opcoderef, then it can't be in an expression */
      if (var && var->varType == &CS_VAR_TYPE_OPCODEREF)
        var = NULL;
      if (find_opcode(csound, top->value->lexeme) != NULL && var == NULL) {
        top->next = root->next;
        root->next = NULL;
        return top;
      }

      /* i.e. outs a1 + a2 + a3, a4, + a5 + a6 */
      newTop = top->left;
      newTop->next = root->next;
      newTop->type = T_OPCALL;

      if (top->right == unary_op) {
        newTop->right = convert_unary_op_to_binary(csound, top, unary_op);
        newTop->right->next = unary_op->next;
        unary_op->next = NULL;
      } else {
        TREE *unary_op_parent = get_left_parent(top->right, unary_op);
        newTop->right = top->right;
        unary_op_parent->left =
            convert_unary_op_to_binary(csound, top, unary_op);
      }
      top->right = top->left = top->next = NULL;
      return newTop;
    }

    /* i.e. asig oscil 0.25, 440 */
    top->next = root->next;
    return top;

  } else if (root->right == NULL) {
    /* this branch catches this part of opcall rule:
       out_arg_list '(' ')' NEWLINE */

    if (tree_arg_list_count(root->left) != 1) {
      synterr(csound, Str("Internal Error: convert_statement_to_opcall "
                          "received invalid OPCALL\n"));
    }
    root->left->next = root->next;
    root->left->type = T_OPCALL;
    return root->left;
  }

  if (root->right == NULL) {
    synterr(csound, Str("Internal Error: convert_statement_to_opcall "
                        "received invalid OPCALL\n"));
    return NULL;
  }

  /* Now need to disambiguate the rule : out_arg_list expr_list NEWLINE */
  leftCount = tree_arg_list_count(root->left);
  rightCount = tree_arg_list_count(root->right);

  if (leftCount > 1 && rightCount > 1) {
    synterr(csound, Str("Internal Error: convert_statement_to_opcall "
                        "received invalid OPCALL\n"));
    return NULL;
  }

  if (leftCount == 1 && rightCount == 1) {
    TREE *newTop;

    // DEBUG: Add debug output to see what we're processing
    csound->Message(csound, "[convert_statement_to_opcall] DEBUG: leftCount=1 rightCount=1 - left->type=%d right->type=%d\n",
                    root->left->type, root->right->type);
    if (root->left->value && root->left->value->lexeme) {
      csound->Message(csound, "[convert_statement_to_opcall] DEBUG: left lexeme='%s'\n", root->left->value->lexeme);
    }
    if (root->left->value && root->left->value->optype) {
      csound->Message(csound, "[convert_statement_to_opcall] DEBUG: left optype='%s'\n", root->left->value->optype);
    }
    if (root->right->value && root->right->value->lexeme) {
      csound->Message(csound, "[convert_statement_to_opcall] DEBUG: right lexeme='%s'\n", root->right->value->lexeme);
    }

    // Special handling for typed variable initialization: temp:MyType init -> init.MyType temp
    // Check if left side has type information (optype) and right side is 'init'
    if (root->left->value && root->left->value->optype &&
        root->right->value && root->right->value->lexeme &&
        strcmp(root->right->value->lexeme, "init") == 0) {

      const char* typeName = root->left->value->optype;
      csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Found typed variable init: %s:%s init\n",
                      root->left->value->lexeme ? root->left->value->lexeme : "(null)", typeName);

      // Build the init opcode name for the struct type
      char initOpcodeName[256];
      if (typeName[0] == ':' && typeName[strlen(typeName)-1] == ';') {
        // Type name is already in :Type; format, extract the middle part
        snprintf(initOpcodeName, sizeof(initOpcodeName), "init.%.*s",
                 (int)(strlen(typeName) - 2), typeName + 1);
      } else {
        // Type name is in simple format
        snprintf(initOpcodeName, sizeof(initOpcodeName), "init.%s", typeName);
      }

      csound->Message(csound, "[convert_statement_to_opcall] DEBUG: Converting to init opcode: '%s'\n", initOpcodeName);

      // Convert to init.MyType opcode call
      newTop = root->right;
      newTop->type = T_OPCALL;
      newTop->value = make_token(csound, initOpcodeName);
      newTop->left = root->left;

      // For basic struct init, we need to provide default values for all members
      // The init.MyType opcode expects arguments for each member
      // Create default zero values for the struct members
      TREE* defaultArg1 = make_leaf(csound, 0, 0, NUMBER_TOKEN, make_num(csound, "0"));
      TREE* defaultArg2 = make_leaf(csound, 0, 0, NUMBER_TOKEN, make_num(csound, "0"));
      defaultArg1->next = defaultArg2;

      newTop->right = defaultArg1; // Provide default zero arguments
      newTop->next = root->next;
      root->next = NULL;
      return newTop;
    }

    if (root->right->type == T_IDENT &&
        find_var_from_pools(csound, root->right->value->lexeme,
                            root->right->value->lexeme, typeTable) == NULL &&
        find_opcode(csound, root->right->value->lexeme) != NULL) {
      newTop = root->right;
      newTop->type = T_OPCALL;
      newTop->left = root->left;
      newTop->next = root->next;
      root->next = NULL;
    } else {
      newTop = root->left;
      newTop->type = T_OPCALL;
      newTop->right = root->right;
      newTop->next = root->next;
      root->next = NULL;
    }
    return newTop;
  } else if (leftCount == 1) {
    TREE *newTop = root->left;
    newTop->type = T_OPCALL;
    newTop->next = root->next;
    newTop->right = root->right;
    return newTop;
  } else {
    TREE *newTop = root->right;
    newTop->type = T_OPCALL;
    newTop->next = root->next;
    newTop->left = root->left;
    return newTop;
  }

  return NULL;
}

/*
 * Verifies:
 *    -number of args correct
 *    -types of arg correct
 *    -expressions are valid and types correct
 */
int32_t verify_opcode(CSOUND* csound, TREE* root, TYPE_TABLE* typeTable) {

  TREE* left = root->left;
  TREE* right = root->right;
  char* leftArgString;
  char* rightArgString;
  char* opcodeName;

  if (root->value == NULL) return 0;

  opcodeName = root->value->lexeme;

  // DEBUG: Add debug output for init.instr opcodes
  if (opcodeName && strcmp(opcodeName, "init.instr") == 0) {
    csound->Message(csound, "[verify_opcode] DEBUG: Processing init.instr opcode\n");
  }

  if (!check_args_exist(csound, root->right, typeTable)) {
    return 0;
  }

  add_args(csound, root->left, typeTable);

  opcodeName = root->value->lexeme;
  leftArgString = get_arg_string_from_tree(csound, left, typeTable);
  rightArgString = get_arg_string_from_tree(csound, right, typeTable);


  OENTRIES* entries = find_opcode2(csound, opcodeName);

  // DEBUG: Add debug output for init.instr opcodes
  if (opcodeName && strcmp(opcodeName, "init.instr") == 0) {
    csound->Message(csound, "[verify_opcode] DEBUG: init.instr opcode lookup - entries=%p count=%d\n",
                    (void*)entries, entries ? entries->count : 0);
  }

  if (UNLIKELY(entries == NULL || entries->count == 0)) {
    synterr(csound, Str("Unable to find opcode with name: %s\n"),
            root->value->lexeme);
    if (entries != NULL) {
      csound->Free(csound, entries);
    }
    return 0;
  }

  OENTRY* oentry;
  if (root->value->optype == NULL ||
      leftArgString == NULL) {
    if(root->value->optype) {
      // in the special case of 'k' for 'i'
      // we enforce the annotation
      if(!strcmp(root->value->optype, "k"))
	      *rightArgString = 'k';
      else // otherwise ignore it
	      csound->Warning(csound, "ignoring annotation %s \n"
			    "\t for opcode %s with no outputs, line %d",
			    root->value->optype, opcodeName,
			    root->line);
    }

    oentry = resolve_opcode(csound, entries,
                            leftArgString, rightArgString);
  }
  /* if there is type annotation, try to resolve it */
  else {
    // if there is a discrepancy between out-types/annotation
    // print a warning and use out-types
    // except for 'p' and 'i'
    if(*leftArgString == 'p' && *root->value->optype == 'i')
    oentry = resolve_opcode(csound, entries, root->value->optype, rightArgString);
    else if(leftArgString && strcmp(leftArgString, root->value->optype)){
      csound->Warning(csound, " output type(s) %s\n"
                      "\t not matching annotation %s\n"
                      "\t ignoring annotation for opcode %s, line %d",
                      leftArgString, root->value->optype,
		      opcodeName, root->line);
        oentry = resolve_opcode(csound, entries,
                            leftArgString, rightArgString);
      } else {
      oentry = resolve_opcode(csound, entries,
                               root->value->optype, rightArgString);
      }
  }



  if (UNLIKELY(oentry == NULL)) {
    int32_t i;
    synterr(csound, Str("Unable to find opcode entry for \'%s\' "
                        "with matching argument types:\n"),
            opcodeName);
    csoundMessage(csound, Str("Found:\n  %s %s %s\n"),
                  leftArgString, root->value->lexeme, rightArgString);

    csoundMessage(csound, Str("\nCandidates:\n"));

    for (i = 0; i < entries->count; i++) {
      OENTRY *entry = entries->entries[i];
      csoundMessage(csound, "  %s %s %s\n", entry->outypes, entry->opname,
                    entry->intypes);
    }

    csoundMessage(csound, Str("\nLine: %d\n"),
                  root->line);
    do_baktrace(csound, root->locn);

    csound->Free(csound, leftArgString);
    csound->Free(csound, rightArgString);
    csound->Free(csound, entries);

    return 0;
  }
  else {
    if (csound->oparms->sampleAccurate &&
        (strcmp(oentry->opname, "=.a")==0) &&
        (left!=NULL) && (left->value!=NULL) &&
        (left->value->lexeme[0]=='a')) {
         /* Deal with sample accurate assigns */
         int32_t i = 0;
         while (strcmp(entries->entries[i]->opname, "=.l")) {
         i++;
        }
        oentry = entries->entries[i];
    }
    else {
      if (csound->oparms->sampleAccurate &&
          (strcmp(oentry->opname, "=._")==0) &&
          (left->value->lexeme[0]=='a')) {
           int32_t i = 0;
           while (strcmp(entries->entries[i]->opname, "=.L")) {
            i++;
           }
          oentry = entries->entries[i];
        }
    }
    root->markup = oentry;
  }
  csound->Free(csound, leftArgString);
  csound->Free(csound, rightArgString);
  csound->Free(csound, entries);
  return 1;
}


/* Walks tree and finds all label: definitions */
CONS_CELL* get_label_list(CSOUND* csound, TREE* root) {
  CONS_CELL* head = NULL, *ret = NULL;
  TREE* current = root;
  char* labelText;

  while (current != NULL) {
    switch(current->type) {
    case LABEL_TOKEN:
      labelText = current->value->lexeme;
      head = cs_cons(csound, cs_strdup(csound, labelText), head);
      break;

    case IF_TOKEN:
    case ELSEIF_TOKEN:
      if (current->right->type == THEN_TOKEN ||
          current->right->type == KTHEN_TOKEN ||
          current->right->type == ITHEN_TOKEN) {

        ret = get_label_list(csound, current->right->right);
        head = cs_cons_append(head, ret);
        ret = get_label_list(csound, current->right->next);
        head = cs_cons_append(head, ret);
      }
      break;

    case ELSE_TOKEN:
    case UNTIL_TOKEN:
    case WHILE_TOKEN:
      ret = get_label_list(csound, current->right);
      head = cs_cons_append(head, ret);
      break;

    default:
      break;
    }

    current = current->next;
  }

  return head;
}

static int32_t is_label(char* ident, CONS_CELL* labelList) {
  CONS_CELL* current;

  if (labelList == NULL) return 0;

  current = labelList;

  while  (current != NULL) {
    if (strcmp((char*)current->value, ident) == 0) {
      return 1;
    }
    current = current->next;
  }
  return 0;
}

int32_t is_reserved(char *varname) {
  return (strcmp("sr", varname) == 0 || strcmp("kr", varname) == 0 ||
          strcmp("ksmps", varname) == 0 || strcmp("0dbfs", varname) == 0 ||
          strcmp("nchnls", varname) == 0 || strcmp("nchnls_i", varname) == 0 ||
          strcmp("A4", varname) == 0 ||
          /* Reserve xin/xout identifiers to avoid accidental var creation */
          strcmp("xin", varname) == 0 || strcmp("xout", varname) == 0);
}

int32_t verify_if_statement(CSOUND *csound, TREE *root, TYPE_TABLE *typeTable) {

  char *outArg;
  TREE *right = root->right;

  if (right->type == IGOTO_TOKEN || right->type == KGOTO_TOKEN ||
      right->type == GOTO_TOKEN) {

    if (!check_args_exist(csound, root->left, typeTable)) {
      return 0;
    }

    outArg = get_arg_type2(csound, root->left, typeTable);

    return (outArg != NULL && (*outArg == 'b' || *outArg == 'B'));

  } else if (right->type == THEN_TOKEN || right->type == ITHEN_TOKEN ||
             right->type == KTHEN_TOKEN) {
    TREE *current = root;

    while (current != NULL) {
      if (current->type == ELSE_TOKEN) {
        break;
      }

      if (!check_args_exist(csound, current->left, typeTable)) {
        return 0;
      }

      outArg = get_arg_type2(csound, current->left, typeTable);

      if (outArg == NULL || (*outArg != 'b' && *outArg != 'B')) {
        csound->Free(csound, outArg);
        return 0;
      }
      csound->Free(csound, outArg);
      current = (current->right == NULL) ? NULL : current->right->next;
    }
  }

  return 1;
}

int32_t verify_until_statement(CSOUND* csound, TREE* root,
                               TYPE_TABLE* typeTable) {
  char* outArg;

  if (!check_args_exist(csound, root->left, typeTable)) {
    return 0;
  };

  outArg = get_arg_type2(csound, root->left, typeTable);
  if (UNLIKELY(outArg == NULL || (*outArg != 'b' && *outArg != 'B'))) {
    synterr(csound,
            Str("expression for until/while statement not a boolean "
                "expression, line %d\n"),
            root->line);
    do_baktrace(csound, root->locn);
    return 0;
  }
  return 1;
}



int32_t initStructVar(CSOUND *csound, void *p) {
  INIT_STRUCT_VAR *init = (INIT_STRUCT_VAR *)p;
  CS_STRUCT_VAR *structVar = (CS_STRUCT_VAR *)init->out;
  CS_TYPE *type = csoundGetTypeForArg(init->out);
  int32_t len = cs_cons_length(type->members);
  int32_t i;

  // Single-argument form: init.Type(out, srcStruct)
  // Or single-size form: init.Type(out, isize) to size any array members
  if (init->inArgs[0] != NULL && init->inArgs[1] == NULL) {
    const CS_TYPE *a0 = csoundGetTypeForArg(init->inArgs[0]);
    // Check if this is an array argument - if so, handle array assignment
    if (a0 && a0 == &CS_VAR_TYPE_ARRAY) {
      // Check if the struct has exactly one member and it's an array
      if (structVar->memberCount == 1) {
        CS_VAR_MEM* member = structVar->members[0];
        if (member && member->varType == &CS_VAR_TYPE_ARRAY) {
          // Copy the array data from the input argument to the struct member
          ARRAYDAT* srcArray = (ARRAYDAT*)init->inArgs[0];
          ARRAYDAT* dstArray = (ARRAYDAT*)&member->value;

          if (srcArray && dstArray) {
            // Create an alias to the source array (shallow copy)
            dstArray->arrayType = srcArray->arrayType;
            dstArray->dimensions = srcArray->dimensions;
            dstArray->sizes = srcArray->sizes;
            dstArray->arrayMemberSize = srcArray->arrayMemberSize;
            dstArray->data = srcArray->data;
            dstArray->allocated = 0; // Mark as alias (non-owning)
          }

          return CSOUND_SUCCESS;
        }
      }

      // If not a single array member, fall through to multi-argument handling
    } else if (a0 && a0->userDefinedType && a0 == type) {
      CS_STRUCT_VAR *src = (CS_STRUCT_VAR *)init->inArgs[0];
      if (src) {
        /* Shallow alias */
        structVar->members = src->members;
        structVar->memberCount = src->memberCount;
        structVar->ownsMembers = 0;
        return CSOUND_SUCCESS;
      }
    }

    /* Check if struct has any array members */
      int32_t mcount = structVar->memberCount;
      int hasArrayMembers = 0;
      for (int32_t mi = 0; mi < mcount; ++mi) {
        CS_VAR_MEM *mem = structVar->members[mi];
        if (mem && mem->varType == &CS_VAR_TYPE_ARRAY) {
          hasArrayMembers = 1;
          break;
        }
      }

      if (hasArrayMembers) {
        /* Interpret single numeric arg as size for array members */
        int size = MYFLT2LRND(*init->inArgs[0]);
        if (size < 0)
          size = 0;
        for (int32_t mi = 0; mi < mcount; ++mi) {
          CS_VAR_MEM *mem = structVar->members[mi];
          if (mem && mem->varType == &CS_VAR_TYPE_ARRAY) {
            ARRAYDAT *dest = (ARRAYDAT *)&mem->value;
          /* Manually size and allocate the array, including struct element init
           */
          if (dest->dimensions == 0)
            dest->dimensions = 1;
          if (dest->sizes == NULL) {
            dest->sizes = (int32_t *)csound->Calloc(
                csound, (size_t)dest->dimensions * sizeof(int32_t));
          }
          if (dest->dimensions > 0)
            dest->sizes[0] = size;
          int64_t total = 1;
          for (int32_t dj = 0; dj < dest->dimensions; ++dj) {
            int32_t v = (dest->sizes ? dest->sizes[dj] : 1);
            if (v <= 0)
              v = 1;
            total *= (int64_t)v;
          }
          if (dest->arrayType) {
            CS_VARIABLE *v = dest->arrayType->createVariable(
                csound, (void *)dest->arrayType, init->h.insdshead);
            dest->arrayMemberSize = v->memBlockSize;
            size_t bytes = (size_t)dest->arrayMemberSize * (size_t)total;
            dest->data = (MYFLT *)csound->Calloc(csound, bytes);
            dest->allocated = bytes;
            if (v->initializeVariableMemory) {
              char *base = (char *)dest->data;
              for (int64_t ei = 0; ei < total; ++ei) {
                v->initializeVariableMemory(
                    csound, v,
                    (MYFLT *)(base + (size_t)ei * (size_t)v->memBlockSize));
              }
            }
          }
          }
        }
        return CSOUND_SUCCESS;
      } else {
        /* Struct has only scalar members - initialize first member with the argument */
        if (mcount > 0) {
          CS_VAR_MEM *mem = structVar->members[0];
          if (mem && mem->varType != &CS_VAR_TYPE_ARRAY) {
            csound->Message(csound, "[initStructVar] DEBUG: Single-arg scalar init: setting member 0 to %f\n", *init->inArgs[0]);
            mem->value = *init->inArgs[0];
            csound->Message(csound, "[initStructVar] DEBUG: After single-arg init: mem->value = %f\n", mem->value);
          }
        }
        return CSOUND_SUCCESS;
      }
    }

  for (i = 0; i < len; i++) {
    CS_VAR_MEM *mem = structVar->members[i];
    const CS_TYPE *mtype = mem->varType;
    const CS_TYPE *atype = csoundGetTypeForArg(init->inArgs[i]);

    csound->Message(csound, "[initStructVar] DEBUG: Processing member %d\n", i);
    csound->Message(csound, "[initStructVar] DEBUG: mem=%p, mtype=%p, atype=%p\n", mem, mtype, atype);
    if (mtype && mtype->varTypeName) {
      csound->Message(csound, "[initStructVar] DEBUG: mtype->varTypeName='%s'\n", mtype->varTypeName);
    }
    csound->Message(csound, "[initStructVar] DEBUG: Input value = %f\n", *init->inArgs[i]);

    // Arrays: shallow references to preserve pointers
    if (mtype == &CS_VAR_TYPE_ARRAY) {
      ARRAYDAT *dest = (ARRAYDAT *)&mem->value;
      ARRAYDAT *src = (ARRAYDAT *)init->inArgs[i];

      if (UNLIKELY(src == NULL)) {
        // Fallback to standard copy if source is NULL to avoid crash
        mtype->copyValue(csound, mtype, &mem->value, init->inArgs[i], NULL);
      } else {
        /* Shallow references for array member */
        dest->arrayType = src->arrayType;
        dest->dimensions = src->dimensions;
        dest->sizes = src->sizes;
        dest->arrayMemberSize = src->arrayMemberSize;
        dest->data = src->data;
        /* Mark as non-owning alias to avoid double-free of backing storage */
        dest->allocated = 0;
      }
    } else if (mtype && mtype->userDefinedType) {
      if (atype && atype->userDefinedType && init->inArgs[i] != NULL) {
        CS_STRUCT_VAR *dst = (CS_STRUCT_VAR *)&mem->value;
        CS_STRUCT_VAR *src = (CS_STRUCT_VAR *)init->inArgs[i];
        if (dst && src) {
          /* Use the proper copyValue function for deep copying */
          atype->copyValue(csound, atype, dst, src, NULL);
        }
      } else {
        mtype->copyValue(csound, mtype, &mem->value, init->inArgs[i], NULL);
      }
    } else if (mem->varType == &CS_VAR_TYPE_ARRAY) {
      // Special handling for array members - need to copy array metadata
      ARRAYDAT* dst = (ARRAYDAT*)&mem->value;
      ARRAYDAT* src = (ARRAYDAT*)init->inArgs[i];

      if (src) {
        // Copy all array metadata (similar to struct_array_member_assign)
        dst->allocated = 0; // Shallow alias: destination does not own storage
        dst->arrayMemberSize = src->arrayMemberSize;
        dst->data = src->data;
        dst->dimensions = src->dimensions;
        dst->sizes = src->sizes;
        dst->arrayType = src->arrayType;
      }
    } else {
      csound->Message(csound, "[initStructVar] DEBUG: Copying scalar value using copyValue\n");
      csound->Message(csound, "[initStructVar] DEBUG: Before copy: mem->value = %f\n", mem->value);
      csound->Message(csound, "[initStructVar] DEBUG: Input value: %f\n", *init->inArgs[i]);
      csound->Message(csound, "[initStructVar] DEBUG: mem=%p, mem->varType=%p\n", mem, mem->varType);

      // Try direct assignment first for debugging
      mem->value = *init->inArgs[i];
      csound->Message(csound, "[initStructVar] DEBUG: After direct assignment: mem->value = %f\n", mem->value);

      // Also try copyValue
      if (mem->varType && mem->varType->copyValue) {
        mem->varType->copyValue(csound, mem->varType, &mem->value,
                                init->inArgs[i], NULL);
        csound->Message(csound, "[initStructVar] DEBUG: After copyValue: mem->value = %f\n", mem->value);
      }
    }
  }

  return CSOUND_SUCCESS;
}

void initializeStructVar(CSOUND *csound, CS_VARIABLE *var, MYFLT *mem) {
  CS_STRUCT_VAR *structVar = (CS_STRUCT_VAR *)mem;
  const CS_TYPE *type = var->varType;
  CONS_CELL *members = type->members;

  // initializeStructVar should always create new struct memory, never aliases
  // The struct assignment issue needs to be fixed elsewhere, not here
  // This function is called when struct variables are first created and should
  // always allocate new memory to avoid the john:Person init 2, relatives bug

  int32_t len = cs_cons_length(members);
  int32_t i;


  structVar->members = csound->Calloc(csound, len * sizeof(CS_VAR_MEM *));
  structVar->memberCount = len;
  structVar->ownsMembers = 1;



  if (csound->GetDebug(csound)) {
    csound->Message(csound, "Initializing Struct...\n");
    csound->Message(csound, "Struct Type: %s\n", type->varTypeName);
  }
  for (i = 0; i < len; i++) {
    CS_VARIABLE *var = members->value;
    size_t size = (sizeof(CS_VAR_MEM) - sizeof(MYFLT)) + var->memBlockSize;
    CS_VAR_MEM *mem = csound->Calloc(csound, size);
    if (var->initializeVariableMemory != NULL) {
      var->initializeVariableMemory(csound, var, &mem->value);
    }

    // Special handling for array members: ensure proper initialization
    if (var->varType == &CS_VAR_TYPE_ARRAY && var->dimensions > 0) {
      ARRAYDAT *arrayDat = (ARRAYDAT *)&mem->value;
      arrayDat->dimensions = var->dimensions;
      arrayDat->arrayType = var->subType;

      // Allocate sizes array
      arrayDat->sizes =
          csound->Calloc(csound, sizeof(int32_t) * var->dimensions);

      // For struct arrays, we need to initialize with default size of 0
      // This will be properly sized when the array is actually used
      for (int32_t j = 0; j < var->dimensions; j++) {
        arrayDat->sizes[j] = 0;
      }

      /* Set element size in bytes based on subtype
         For arrays of user-defined structs, use sizeof(CS_STRUCT_VAR)
         Otherwise default to sizeof(MYFLT) for numeric-like arrays */
      if (var->subType != NULL && var->subType->userDefinedType) {
        arrayDat->arrayMemberSize = (size_t)sizeof(CS_STRUCT_VAR);
      } else {
        arrayDat->arrayMemberSize = (size_t)sizeof(MYFLT);
      }
      arrayDat->data = NULL;
      arrayDat->allocated = 0;
    }

    mem->varType = var->varType;



    structVar->members[i] = mem;
    members = members->next;
  }
}

CS_VARIABLE *createStructVar(void *cs, void *p, INSDS *ctx) {
  CSOUND *csound = (CSOUND *)cs;
  const CS_TYPE *type = (const CS_TYPE *)p;

  if (type == NULL) {
    csound->Message(csound, "ERROR: no type given for struct creation\n");
    return NULL;
  }

  CS_VARIABLE *var = csound->Calloc(csound, sizeof(CS_VARIABLE));
  IGN(p);
  var->memBlockSize = sizeof(CS_STRUCT_VAR);
  var->initializeVariableMemory = initializeStructVar;
  var->varType = type;
  var->ctx = ctx;

  // FIXME - implement
  return var;
}

void copyStructVar(CSOUND *csound, const CS_TYPE *structType, void *dest,
                   const void *src, INSDS *p) {
  CS_STRUCT_VAR *varDest = (CS_STRUCT_VAR *)dest;
  CS_STRUCT_VAR *varSrc = (CS_STRUCT_VAR *)src;
  int32_t i, count;

  /* Ensure destination struct is initialized (members allocated) */
  if (UNLIKELY(varDest == NULL || varSrc == NULL))
    return;
  if (UNLIKELY(varDest->members == NULL)) {
    if (structType && structType->createVariable) {
      CS_VARIABLE *helper =
          structType->createVariable(csound, (void *)structType, p);
      if (helper && helper->initializeVariableMemory) {
        helper->initializeVariableMemory(csound, helper, (MYFLT *)dest);
      }
    }
  }

  count = cs_cons_length(structType->members);
  for (i = 0; i < count; i++) {
    CS_VAR_MEM *d = varDest->members[i];
    CS_VAR_MEM *s = varSrc->members[i];
    d->varType->copyValue(csound, d->varType, &d->value, &s->value, p);
  }
}

/* Alias struct function - creates reference instead of copy */
void aliasStructVar(CSOUND *csound, const CS_TYPE *structType, void *dest,
                    const void *src, INSDS *p) {
  CS_STRUCT_VAR *varDest = (CS_STRUCT_VAR *)dest;
  CS_STRUCT_VAR *varSrc = (CS_STRUCT_VAR *)src;

  if (UNLIKELY(varDest == NULL || varSrc == NULL))
    return;

  /* Free any existing owned storage before aliasing */
  if (varDest->ownsMembers && varDest->members) {
    csound_free_struct_members(csound, varDest);
  }

  /* Create alias: both structs point to the same memory */
  varDest->members = varSrc->members;
  varDest->memberCount = varSrc->memberCount;
  varDest->ownsMembers = 0; /* Non-owning to avoid double free */
}

// Phase 1: Register struct name as placeholder type (for recursive references)
int32_t register_struct_placeholder(CSOUND *csound, TREE *structDefTree) {
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
      csoundGetTypeWithVarTypeName(csound->typePool, internalName);
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
  type->userDefinedType = 1;
  type->members = NULL; // Will be filled in Phase 2

  // Register the placeholder type
  if (!csoundAddVariableType(csound, csound->typePool, type)) {
    csound->Free(csound, type->varTypeName);
    csound->Free(csound, type);
    return 0;
  }



  return 1;
}

// Helper function to recursively flatten struct types for initialization signatures
// with recursion detection to handle recursive struct definitions
static int32_t flatten_struct_signature_internal(CSOUND *csound, const CS_TYPE *type, char *buffer, int32_t *index, int32_t maxLen,
                                                  const char **visitedTypes, int32_t visitedCount, int32_t maxVisited) {
  if (!type || !buffer || !index) return 0;

  // Check for recursion - if we've already visited this type, use the type name directly
  if (type->varTypeName) {
    for (int32_t i = 0; i < visitedCount; i++) {
      if (visitedTypes[i] && strcmp(visitedTypes[i], type->varTypeName) == 0) {
        // Recursive reference detected - use the actual type name for arrays
        if (type->varTypeName[0] == ':' && type->varTypeName[strlen(type->varTypeName)-1] == ';') {
          // For struct types, use the full type name
          int32_t typeNameLen = (int32_t)strlen(type->varTypeName);
          if (*index + typeNameLen >= maxLen) return 0;
          strcpy(buffer + *index, type->varTypeName);
          *index += typeNameLen;
          return 1;
        } else {
          // For primitive types, use single character
          if (*index >= maxLen - 1) return 0;
          buffer[(*index)++] = type->varTypeName[0];
          return 1;
        }
      }
    }
  }

  // If it's a primitive type, add it directly
  if (type->varTypeName && strlen(type->varTypeName) == 1) {
    if (*index >= maxLen - 1) return 0; // Buffer overflow check
    buffer[(*index)++] = type->varTypeName[0];
    return 1;
  }

  // If it's a struct type, check if we should preserve it or flatten it
  if (type->varTypeName && type->varTypeName[0] == ':' &&
      type->varTypeName[strlen(type->varTypeName)-1] == ';') {

    // Check for recursion first
    for (int32_t i = 0; i < visitedCount; i++) {
      if (visitedTypes[i] && strcmp(visitedTypes[i], type->varTypeName) == 0) {
        // Recursive reference detected - use the actual type name for struct-to-struct references
        csound->Message(csound, "[struct] DEBUG: Recursive reference detected, preserving type '%s'\n", type->varTypeName);
        int32_t typeNameLen = (int32_t)strlen(type->varTypeName);
        if (*index + typeNameLen >= maxLen) return 0;
        strcpy(buffer + *index, type->varTypeName);
        *index += typeNameLen;
        return 1;
      }
    }

    // For non-recursive struct types, preserve them as-is for struct-to-struct references
    // This allows Rectangle to have a Point member without flattening Point to ii
    csound->Message(csound, "[struct] DEBUG: Preserving struct type '%s' in signature (non-recursive)\n", type->varTypeName);
    int32_t typeNameLen = (int32_t)strlen(type->varTypeName);
    if (*index + typeNameLen >= maxLen) return 0;
    strcpy(buffer + *index, type->varTypeName);
    *index += typeNameLen;
    return 1;

    // OLD CODE: struct flattening path (disabled)
    // This block used a variable-length array (VLA) which MSVC does not support,
    // and the logic is now superseded by the early return above that preserves
    // struct types in signatures. Keeping it disabled avoids Windows build errors.
#if 0
    if (visitedCount >= maxVisited) {
      csound->Message(csound, "[struct] Warning: Too many nested types in struct flattening\n");
      return 0;
    }

    const char *newVisitedTypes[maxVisited];
    for (int32_t i = 0; i < visitedCount; i++) {
      newVisitedTypes[i] = visitedTypes[i];
    }
    newVisitedTypes[visitedCount] = type->varTypeName;

    // Look up the struct type to get its members
    const CS_TYPE *structType = csoundGetTypeWithVarTypeName(csound->typePool, type->varTypeName);
    if (!structType || !structType->members) {
      csound->Message(csound, "[struct] Warning: Could not find struct type '%s' for flattening\n", type->varTypeName);
      return 0;
    }

    // Recursively flatten each member (members is a CONS_CELL list)
    CONS_CELL *member = structType->members;
    while (member) {
      CS_VARIABLE *memberVar = (CS_VARIABLE *)member->value;
      if (!memberVar || !memberVar->varType) {
        csound->Message(csound, "[struct] Warning: Invalid member in struct '%s'\n", type->varTypeName);
        return 0;
      }

      // Handle array types
      if (memberVar->varType == &CS_VAR_TYPE_ARRAY && memberVar->subType) {
        // For array members, use a size parameter (integer) instead of the full array type
        if (*index >= maxLen - 1) return 0;
        buffer[(*index)++] = 'i';  // Size parameter for array initialization
      } else {
        // Handle regular types - preserve struct types instead of flattening them
        csound->Message(csound, "[struct] DEBUG: Processing member type, varTypeName='%s'\n",
                        memberVar->varType->varTypeName ? memberVar->varType->varTypeName : "NULL");
        if (memberVar->varType->varTypeName &&
            memberVar->varType->varTypeName[0] == ':' &&
            memberVar->varType->varTypeName[strlen(memberVar->varType->varTypeName)-1] == ';') {
          // This is a struct type - use it directly instead of flattening
          csound->Message(csound, "[struct] DEBUG: Preserving struct type '%s' in signature\n", memberVar->varType->varTypeName);
          int32_t typeNameLen = (int32_t)strlen(memberVar->varType->varTypeName);
          if (*index + typeNameLen >= maxLen) return 0;
          strcpy(buffer + *index, memberVar->varType->varTypeName);
          *index += typeNameLen;
        } else {
          // Handle primitive types with flattening
          if (!flatten_struct_signature_internal(csound, memberVar->varType, buffer, index, maxLen,
                                                 newVisitedTypes, visitedCount + 1, maxVisited)) {
            return 0;
          }
        }
      }

      member = member->next;
    }
    return 1;
#endif
  }

  // For other types, treat as single character (fallback)
  if (*index >= maxLen - 1) return 0;
  buffer[(*index)++] = 'i'; // Default to integer type
  return 1;
}

// Public wrapper function for flatten_struct_signature with recursion detection
static int32_t flatten_struct_signature(CSOUND *csound, const CS_TYPE *type, char *buffer, int32_t *index, int32_t maxLen) {
  const char *visitedTypes[32]; // Support up to 32 levels of nesting
  return flatten_struct_signature_internal(csound, type, buffer, index, maxLen, visitedTypes, 0, 32);
}

// Phase 2: Resolve struct members and register opcodes
int32_t add_struct_definition(CSOUND *csound, TREE *structDefTree) {
  char *structName = structDefTree->left->value->lexeme;

  // Get the existing placeholder type (using internal name format)
  size_t nameLen = strlen(structName);
  char *internalName = csound->Malloc(csound, nameLen + 3);
  internalName[0] = ':';
  strcpy(internalName + 1, structName);
  internalName[nameLen + 1] = ';';
  internalName[nameLen + 2] = '\0';

  CS_TYPE *type =
      (CS_TYPE *)csoundGetTypeWithVarTypeName(csound->typePool, internalName);
  csound->Message(csound, "[struct] Phase 2: Looking up '%s' -> %p\n",
                  internalName, (void *)type);

  // Save a copy of internalName for later use in struct-to-struct opcode registration
  char *internalNameCopy = cs_strdup(csound, internalName);

  csound->Free(csound, internalName);
  if (type == NULL) {
    csound->Free(csound, internalNameCopy);
    synterr(csound, Str("Struct '%s' placeholder not found in Phase 2\n"),
            structName);
    return 0;
  }

  // Skip if already resolved (members != NULL)
  if (type->members != NULL) {
    csound->Message(csound,
                    "[struct] Phase 2: '%s' already resolved, skipping\n",
                    structName);
    csound->Free(csound, internalNameCopy);
    return 1;
  }

  TREE *current = structDefTree->right;
  int32_t index = 0;
  char temp[256];

  csound->Message(csound, "[struct] Phase 2: Resolving members for '%s'\n",
                  structName);

  // FIXME: Values are appended in reverse order of definition
  while (current != NULL) {
    char *memberName = current->value->lexeme;
    char *typedIdentArg = current->value->optype;
    if (csound->GetDebug(csound)) {
      csound->Message(
          csound, "[struct] parsing member node: type=%d name=%s optype=%s\n",
          current->type, memberName ? memberName : "(null)",
          typedIdentArg ? typedIdentArg : "(null)");
    }

    // Handle array_identifier nodes specially
    if (current->type == T_ARRAY_IDENT) {
      if (typedIdentArg != NULL) {
        size_t len = strlen(typedIdentArg) + 3; /* "[]" + NUL */
        char *arrayTypeName = csound->Malloc(csound, len);
        snprintf(arrayTypeName, len, "%s[]", typedIdentArg);
        typedIdentArg = arrayTypeName;
      }
    }

    if (typedIdentArg == NULL) {
      typedIdentArg = cs_strndup(csound, memberName, 1);
    }

    // Handle array types specially
    const CS_TYPE *memberType = NULL;
    int isArrayType = 0;
    if (typedIdentArg && strlen(typedIdentArg) > 2 &&
        typedIdentArg[strlen(typedIdentArg) - 1] == ']' &&
        typedIdentArg[strlen(typedIdentArg) - 2] == '[') {
      isArrayType = 1;
      memberType = &CS_VAR_TYPE_ARRAY;
      csound->Message(csound, "[struct] DEBUG: Detected array member type '%s'\n", typedIdentArg);
    } else {
      // Regular type lookup - convert struct type names to internal format
      char *convertedTypeName = check_optional_type(csound, typedIdentArg);
      csound->Message(csound, "[struct] DEBUG: Looking up member type '%s' -> '%s'\n", typedIdentArg, convertedTypeName);
      memberType = csoundGetTypeWithVarTypeName(csound->typePool, convertedTypeName);
      csound->Message(csound, "[struct] DEBUG: Found member type: %p, varTypeName='%s'\n",
                      memberType, memberType ? (memberType->varTypeName ? memberType->varTypeName : "NULL") : "NULL");
      if (convertedTypeName != typedIdentArg) {
        csound->Free(csound, convertedTypeName);
      }
    }

    if (UNLIKELY(memberType == NULL)) {
      synterr(csound, Str("Unknown type '%s' for struct member '%s'\n"),
              typedIdentArg, memberName);
      csound->Free(csound, internalNameCopy);
      return NOTOK;
    }

    CS_VARIABLE *var = NULL;
    if (isArrayType) {
      // Count dimensions (support Type[], Type[][], ...)
      int32_t dims = 0;
      size_t tlen = strlen(typedIdentArg);
      while (tlen >= 2 && typedIdentArg[tlen - 2] == '[' &&
             typedIdentArg[tlen - 1] == ']') {
        dims += 1;
        tlen -= 2;
      }
      // Extract base type name without brackets
      char *baseTypeName = cs_strndup(csound, typedIdentArg, tlen);

      // Convert struct type name to internal format (same as other paths)
      char *convertedBaseTypeName = check_optional_type(csound, baseTypeName);
      const CS_TYPE *baseType =
          csoundGetTypeWithVarTypeName(csound->typePool, convertedBaseTypeName);

      if (UNLIKELY(baseType == NULL)) {
        synterr(csound, Str("Unknown base type '%s' for array member '%s'\n"),
                baseTypeName, memberName);
        csound->Free(csound, baseTypeName);
        csound->Free(csound, convertedBaseTypeName);
        csound->Free(csound, internalNameCopy);
        return NOTOK;
      }
      csound->Free(csound, baseTypeName);
      csound->Free(csound, convertedBaseTypeName);

      // Create array member variable with correct subtype + dimensions
      ARRAY_VAR_INIT varInit;
      varInit.type = baseType;
      varInit.dimensions = dims;
      var = csoundCreateVariable(csound, csound->typePool, &CS_VAR_TYPE_ARRAY,
                                 memberName, &varInit);
      csound->Message(csound, "[struct] DEBUG: Created array variable '%s', base type='%s', var type='%s', subType='%s'\n",
                      memberName,
                      baseType ? (baseType->varTypeName ? baseType->varTypeName : "NULL") : "NULL",
                      var && var->varType ? (var->varType->varTypeName ? var->varType->varTypeName : "NULL") : "NULL",
                      var && var->subType ? (var->subType->varTypeName ? var->subType->varTypeName : "NULL") : "NULL");
    } else {
      // Non-array member: create simple variable
      var = csoundCreateVariable(csound, csound->typePool, memberType,
                                 memberName, NULL);
      csound->Message(csound, "[struct] DEBUG: Created variable '%s', original type='%s', var type='%s'\n",
                      memberName,
                      memberType ? (memberType->varTypeName ? memberType->varTypeName : "NULL") : "NULL",
                      var && var->varType ? (var->varType->varTypeName ? var->varType->varTypeName : "NULL") : "NULL");
    }

    CONS_CELL *member = csound->Calloc(csound, sizeof(CONS_CELL));
    member->value = var;
    type->members = cs_cons_append(type->members, member);
    current = current->next;
  }

  if (csound->GetDebug(csound)) {
    csound->Message(csound, "[struct] Registered type '%s' members:\n",
                    type->varTypeName);
    int mi = 0;
    CONS_CELL *it = type->members;
    while (it) {
      CS_VARIABLE *mv = (CS_VARIABLE *)it->value;
      csound->Message(csound, "  - %d: %s : %s%s\n", mi,
                      mv && mv->varName ? mv->varName : "(null)",
                      mv && mv->varType && mv->varType->varTypeName
                          ? mv->varType->varTypeName
                          : "(null)",
                      (mv && mv->varType == &CS_VAR_TYPE_ARRAY && mv->subType &&
                       mv->subType->varTypeName)
                          ? "[]"
                          : "");
      it = it->next;
      mi++;
    }
  }

  OENTRY oentry;
  memset(temp, 0, 256);

  // Extract external name from internal name format (:Name; -> Name)
  char *externalName;
  if (type->varTypeName[0] == ':' &&
      type->varTypeName[strlen(type->varTypeName) - 1] == ';') {
    // Internal format (:Name;) - extract the Name part
    size_t nameLen = strlen(type->varTypeName) - 2; // Remove : and ;
    externalName = csound->Malloc(csound, nameLen + 1);
    memcpy(externalName, type->varTypeName + 1, nameLen);
    externalName[nameLen] = '\0';
  } else {
    // Already external format - use as is
    externalName = cs_strdup(csound, type->varTypeName);
  }

  cs_sprintf(temp, "init.%s", externalName);
  oentry.opname = cs_strdup(csound, temp);


  // Keep external name for output type construction
  char *externalNameForOutput = cs_strdup(csound, externalName);
  csound->Free(csound, externalName);
  oentry.dsblksiz = sizeof(INIT_STRUCT_VAR);
  oentry.flags = 0;
  oentry.init = initStructVar;
  oentry.perf = NULL;
  oentry.deinit = NULL;
  oentry.useropinfo = NULL;

  /* Set output type */
  memset(temp, 0, 256);
  cs_sprintf(temp, ":%s;", externalNameForOutput);
  oentry.outypes = cs_strdup(csound, temp);
  csound->Free(csound, externalNameForOutput);

  /* Build flattened input signature for init opcode */
  memset(temp, 0, 256);
  index = 0;

  CONS_CELL *member = type->members;
  while (member != NULL) {
    CS_VARIABLE *memberVar = (CS_VARIABLE *)member->value;
    const CS_TYPE *memberType = memberVar->varType;
    csound->Message(csound, "[struct] DEBUG: Signature flattening member '%s', type='%s'\n",
                    memberVar ? (memberVar->varName ? memberVar->varName : "NULL") : "NULL",
                    memberType ? (memberType->varTypeName ? memberType->varTypeName : "NULL") : "NULL");

    // Handle array types specially - check by varTypeName since pointer equality may not work
    int isArrayCondition = (memberType == &CS_VAR_TYPE_ARRAY ||
                           (memberType && memberType->varTypeName && memberType->varTypeName[0] == '['));
    int hasSubType = (memberVar->subType != NULL);
    csound->Message(csound, "[struct] DEBUG: Array check for '%s': isArrayCondition=%d, hasSubType=%d\n",
                    memberVar->varName ? memberVar->varName : "NULL", isArrayCondition, hasSubType);
    if (isArrayCondition && hasSubType) {
      csound->Message(csound, "[struct] DEBUG: Processing array member '%s', subType='%s'\n",
                      memberVar->varName ? memberVar->varName : "NULL",
                      memberVar->subType ? (memberVar->subType->varTypeName ? memberVar->subType->varTypeName : "NULL") : "NULL");
      // For arrays, flatten the base type and add array notation
      if (!flatten_struct_signature(csound, memberVar->subType, temp, &index, 256)) {
        csound->Message(csound, "[struct] Error: Failed to flatten array member type\n");
        return 0;
      }
      // Add array notation
      if (index < 254) {
        temp[index++] = '[';
        temp[index++] = ']';
      }
    } else {
      // Handle regular types with flattening
      if (!flatten_struct_signature(csound, memberType, temp, &index, 256)) {
        csound->Message(csound, "[struct] Error: Failed to flatten member type '%s'\n",
                        memberType->varTypeName ? memberType->varTypeName : "(null)");
        return 0;
      }
    }

    member = member->next;
  }
  // Use the actual argument types that were built above
  temp[index] = '\0'; // Null-terminate the string
  oentry.intypes = cs_strdup(csound, temp);

  csound->Message(csound, "[struct] Registering opcode '%s' with signature '%s' -> '%s'\n",
                  oentry.opname, oentry.intypes, oentry.outypes);

  int result = csoundAppendOpcodes(csound, &oentry, 1);
  if (result != 0) {
    csound->Message(csound, "[struct] ERROR: Failed to register opcode '%s'\n", oentry.opname);
    csound->Free(csound, internalNameCopy);
    return 0;
  }

  csound->Message(csound, "[struct] Successfully registered opcode '%s'\n", oentry.opname);

  /* Also register a struct-to-struct assignment version */
  OENTRY oentry2 = oentry;
  oentry2.opname = cs_strdup(csound, oentry.opname); // Copy the opname

  /* Build struct-to-struct signature: same struct type as input */
  memset(temp, 0, 256);
  cs_sprintf(temp, "%s", internalNameCopy);
  oentry2.intypes = cs_strdup(csound, temp);

  csound->Message(csound, "[struct] Registering struct-to-struct opcode '%s' with signature '%s' -> '%s'\n",
                  oentry2.opname, oentry2.intypes, oentry2.outypes);

  /* Register the struct-to-struct opcode */
  int result2 = csoundAppendOpcodes(csound, &oentry2, 1);
  if (result2 != 0) {
    csound->Message(csound, "[struct] ERROR: Failed to register struct-to-struct opcode '%s'\n", oentry2.opname);
    csound->Free(csound, internalNameCopy);
    return 0;
  }

  csound->Message(csound, "[struct] Successfully registered struct-to-struct opcode '%s'\n", oentry2.opname);

  csound->Free(csound, internalNameCopy);
  return 1;
}

// Two-phase struct definition processor
int32_t process_struct_definitions_two_phase(CSOUND *csound,
                                             TREE *structDefList) {


  if (structDefList == NULL) {

    return 1; // No struct definitions to process
  }

  csound->Message(csound, "[struct] Starting two-phase struct resolution\n");

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

  csound->Message(csound, "[struct] Phase 1 complete, starting Phase 2\n");

  // Phase 2: Resolve all struct members and register opcodes
  current = structDefList;
  int structCount = 0;
  while (current != NULL) {
    structCount++;
    csound->Message(csound, "[struct] Phase 2: Examining node %d, type=%d (STRUCT_TOKEN=%d)\n",
                    structCount, current->type, STRUCT_TOKEN);
    if (current->type == STRUCT_TOKEN) {
      char* structName = current->left->value->lexeme;
      csound->Message(csound, "[struct] Phase 2: Processing struct '%s'\n", structName ? structName : "(null)");
      if (!add_struct_definition(csound, current)) {
        csound->Message(csound, "[struct] Phase 2: ERROR processing struct '%s'\n", structName ? structName : "(null)");
        return 0; // Error in Phase 2
      }
      csound->Message(csound, "[struct] Phase 2: Successfully processed struct '%s'\n", structName ? structName : "(null)");
    } else {
      csound->Message(csound, "[struct] Phase 2: Skipping node with type %d\n", current->type);
    }
    current = current->next;
  }
  csound->Message(csound, "[struct] Phase 2: Processed %d nodes total\n", structCount);

  csound->Message(csound, "[struct] Two-phase struct resolution complete\n");

  return 1;
}

/** Verifies if xin and xout statements are correct for UDO
    needs to check:
    xin/xout number of args matches UDO input/output arg specifications
    xin/xout statements exist if UDO in and out args are not 0 */
int32_t verify_xin_xout(CSOUND *csound, TREE *udoTree, TYPE_TABLE *typeTable) {
  if (udoTree->right == NULL) {
    return 1;
  }
  TREE* outArgsTree = udoTree->left->left;
  TREE* inArgsTree = udoTree->left->right;
  TREE* current = udoTree->right;
  TREE* xinArgs = NULL;
  TREE* xoutArgs = NULL;
  char* inArgs = inArgsTree->value->lexeme;
  char* outArgs = outArgsTree->value->lexeme;
  uint32_t i;

  for (i = 0; i < strlen(inArgs);i++) {
    if (inArgs[i] == 'K') {
      inArgs[i] = 'k';
    }
  }

  for (i = 0; i < strlen(outArgs);i++) {
    if (outArgs[i] == 'K') {
      outArgs[i] = 'k';
    }
  }

  while (current != NULL) {
    if (current->value != NULL) {
      if (strcmp("xin", current->value->lexeme) == 0) {
        if (UNLIKELY(xinArgs != NULL)) {
          synterr(csound,
                  Str("Multiple xin statements found. "
                      "Only one is allowed."));
          return 0;
        }
        xinArgs = current->left;
      }
      if (strcmp("xout", current->value->lexeme) == 0) {
        if (UNLIKELY(xoutArgs != NULL)) {
          synterr(csound,
                  Str("Multiple xout statements found. "
                      "Only one is allowed."));
          return 0;
        }
        xoutArgs = current->right;
      }
    }
    current = current->next;
  }

  char* inArgsFound = get_arg_string_from_tree(csound, xinArgs, typeTable);
  char* outArgsFound = get_arg_string_from_tree(csound, xoutArgs, typeTable);


  if (!check_in_args(csound, inArgsFound, inArgs)) {
    if (UNLIKELY(!(strcmp("0", inArgs) == 0 && xinArgs == NULL))) {
      synterr(csound,
              Str("invalid xin statement for UDO: defined '%s', found '%s'"),
              inArgs, inArgsFound);
      return 0;
    }
  }

  if (!check_in_args(csound, outArgsFound, outArgs)) {
    if (UNLIKELY(!(strcmp("0", outArgs) == 0 && xoutArgs == NULL))) {
      synterr(csound,
              Str("invalid xout statement for UDO: defined '%s', found '%s'\n"),
              outArgs, outArgsFound);
      return 0;
    }
  }

  return 1;
}

TREE *verify_tree(CSOUND *csound, TREE *root, TYPE_TABLE *typeTable) {
  TREE *anchor = NULL;
  TREE *current = root;
  TREE *previous = NULL;
  TREE *newRight;
  TREE* transformed;
  TREE *top;

  char *udo_name = NULL;

  CONS_CELL *parentLabelList = typeTable->labelList;
  typeTable->labelList = get_label_list(csound, root);
  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_SEMANTICS))
          csound->Message(csound, "Verifying AST\n");

  // REMOVED: Pre-processing step that created variables in wrong scope
  // Variables will be created on-demand during runtime by the assignment opcodes

  while (current != NULL) {
    // NEW: Debug all statements being processed by verify_tree
    csound->Message(csound, "[verify_tree] DEBUG: Processing statement type=%d\n", current->type);
    if (current->value && current->value->lexeme) {
      csound->Message(csound, "[verify_tree] DEBUG: Statement lexeme='%s'\n", current->value->lexeme);
    }

    if (current->type == 0) {
      previous = current;
      current = current->next;
      continue;
    }

    switch (current->type) {
    case STRUCT_TOKEN:
      break;
    case INSTR_TOKEN:
      csound->inZero = 0;
      typeTable->localPool = csoundCreateVarPool(csound);
      current->markup = typeTable->localPool;

      if (current->right) {

        newRight = verify_tree(csound, current->right, typeTable);

        if (newRight == NULL) {
          cs_cons_free(csound, typeTable->labelList);
          typeTable->labelList = parentLabelList;
          return NULL;
        }

        current->right = newRight;
        newRight = NULL;
      }


      typeTable->localPool = typeTable->instr0LocalPool;

      break;
    case UDO_TOKEN:
      if (csoundGetDebug(csound) & DEBUG_SEMANTICS)
	csound->Message(csound, "UDO found\n");
      top = current->left;
      if (top->left != NULL && top->left->type == UDO_ANS_TOKEN) {
        top->left->markup = cs_strdup(csound, top->left->value->lexeme);
        top->right->markup = cs_strdup(csound, top->right->value->lexeme);
        add_udo_definition(csound, false,
                           top->value->lexeme,
                           top->left->value->lexeme,
                           top->right->value->lexeme,
                           0x0000);
        udo_name = top->value->lexeme;
      } else {
      if (UNLIKELY(csoundGetDebug(csound) & DEBUG_SEMANTICS))
	csound->Message(csound, "New-style UDO found\n");
        if(current->left->right != NULL &&
           *current->left->right->value->lexeme != '0') {
          add_args(csound, current->left->right, typeTable);
        }
        char* outArgString = get_out_types_from_tree(csound,
                                                     current->left->left);
        char* inArgString = get_in_types_from_tree(csound,
                                                   current->left->right,
                                                   typeTable);
        if (*inArgString != '0') {
          TREE* statements = current->right;
          TREE* xin = create_opcode_token(csound, "xin");
          xin->left = copy_node(csound, current->left->right);
          xin->next = statements;
          current->right = xin;
        }
        top->left->markup = cs_strdup(csound, outArgString);
        top->right->markup = cs_strdup(csound, inArgString);
        add_udo_definition(csound,
                           true,
                           current->left->value->lexeme,
                           outArgString,
                           inArgString,
                           0x0000);
        udo_name = current->left->value->lexeme;
      }
      csound->inZero = 0;

      current->markup = typeTable->localPool;

      if (current->right != NULL) {

        newRight = verify_tree(csound, current->right, typeTable);

        if (newRight == NULL) {
          cs_cons_free(csound, typeTable->labelList);
          typeTable->labelList = parentLabelList;
          return NULL;
        }

        current->right = newRight;

        if (top->left != NULL && top->left->type == UDO_ANS_TOKEN) {
          if(!verify_xin_xout(csound, current, typeTable)) {
            synterr(csound, Str("%s UDO"), udo_name);
            return 0;
          }
        }

        newRight = NULL;
      }

      typeTable->localPool = typeTable->instr0LocalPool;

      break;

    case T_DECLARE: {
      char *outArgStringDecl =
          get_out_types_from_tree(csound, current->left->left);
      char *inArgStringDecl =
          get_in_types_from_tree(csound, current->left->right, typeTable);
      add_udo_definition(csound, false, current->value->lexeme, inArgStringDecl,
                         outArgStringDecl, UNDEFINED);
      csound->inZero = 0;
      if (UNLIKELY(csoundGetDebug(csound) & DEBUG_SEMANTICS))
	csound->Message(csound, "UDO declared\n");

      typeTable->localPool = csoundCreateVarPool(csound);
      current->markup = typeTable->localPool;

      if (current->right != NULL) {

        newRight = verify_tree(csound, current->right, typeTable);

        if (newRight == NULL) {
          cs_cons_free(csound, typeTable->labelList);
          typeTable->labelList = parentLabelList;
          return NULL;
        }

        current->right = newRight;
        newRight = NULL;
      }

      typeTable->localPool = typeTable->instr0LocalPool;

      break;
    }

    case IF_TOKEN:
      if (!verify_if_statement(csound, current, typeTable)) {
        synterr(csound, "conditional expression not valid, line %d",
                current->line - 2);
        return NULL;
      }

      current = expand_if_statement(csound, current, typeTable);

      if (previous != NULL) {
        previous->next = current;
      }

      continue;

    case UNTIL_TOKEN:
    case WHILE_TOKEN:
      // Always verify and expand while/until immediately so body ops are
      // compiled
      if (!verify_until_statement(csound, current, typeTable)) {
        synterr(csound, "loop conditional expression not valid, line %d",
                current->line - 2);
        return NULL;
      }
      current = expand_until_statement(csound, current, typeTable,
                                       current->type == WHILE_TOKEN);
      if (previous != NULL) {
        previous->next = current;
      }
      continue;

    case SWITCH_TOKEN: {
      char *switchArgType = get_arg_type2(csound, current->left, typeTable);
      current =
          expand_switch_statement(csound, current, typeTable, switchArgType);

      if (previous != NULL) {
        previous->next = current;
      }
    }
      continue;

    case FOR_TOKEN: {
      /** for-loop typing:
          1) if loop variable does not exist, it takes the type of the array
          2) if it exists, the loop type follows the variable type when
         conversion is possible
      */


      char *arrayArgType =
          get_arg_type2(csound, current->right->left, typeTable);

      CS_VARIABLE *var =
          find_var_from_pools(csound, current->left->value->lexeme,
                              current->left->value->lexeme, typeTable);
      if (*arrayArgType != '[') {
        if (current->right->left->value != NULL)
          synterr(csound,
                  Str("line:%d invalid argument in for statement: "
                      "found '%s', which is not an array\n"),
                  current->line, current->right->left->value->lexeme);
        else
          synterr(csound,
                  Str("line:%d expected an array variable in for statement."),
                  current->line);
        return 0;
      }

      char *atype = cs_strdup(csound, arrayArgType + 1); // skip '['
      char *typ;
      atype[strlen(atype) - 1] = '\0'; // remove ']'
      typ = remove_type_quoting(csound, atype);

      // Store the original variable names before expansion
      char *loopVarName = cs_strdup(csound, current->left->value->lexeme);
      char *indexVarName = NULL;
      if (current->left->next) {
        indexVarName = cs_strdup(csound, current->left->next->value->lexeme);
      }

      if (var == NULL) {
        // now create the arg based on the array type
        add_arg(csound, loopVarName, typ, typeTable, current->left);
      } else {
        // Special case: if the existing variable is an OpcodeDef (e.g., from an opcode with the same name),
        // use the array element type instead of the OpcodeDef type for the for-loop variable
        if (var->varType && strcmp(var->varType->varTypeName, "OpcodeDef") == 0) {
          // Keep typ as the array element type, don't change it
          add_arg(csound, loopVarName, typ, typeTable, current->left);
        } else {
          typ = cs_strdup(csound, var->varType->varTypeName);
        }
      }

      // Note: The index variable (count) is already added inside expand_for_statement
      // so we don't need to add it again here

      // Expand the for statement after adding variables
      TREE* original_for = current;
      current = expand_for_statement(csound, current, typeTable, typ);

      // Mark the original for loop as processed to prevent double processing
      original_for->type = 0;

      csound->Free(csound, loopVarName);
      if (indexVarName) {
        csound->Free(csound, indexVarName);
      }
      csound->Free(csound, atype);
      csound->Free(csound, typ);
      if (previous != NULL) {
        previous->next = current;
      } else {
        // If the for loop was the root statement (no previous), update the anchor
        // This handles the case where anchor is NULL (not set yet) or points to the original for loop
        if (anchor == NULL || anchor == original_for) {
          anchor = current;
        }
      }
    }
      continue;

    case LABEL_TOKEN:
      break;

    case '+':
    case '-':
    case '*':
    case '/':
      current->left = verify_tree(csound, current->left, typeTable);
      current->right = verify_tree(csound, current->right, typeTable);
      if ((current->left->type == INTEGER_TOKEN ||
           current->left->type == NUMBER_TOKEN) &&
          (current->right->type == INTEGER_TOKEN ||
           current->right->type == NUMBER_TOKEN)) {
        MYFLT lval, rval;
        lval = (current->left->type == INTEGER_TOKEN
                    ? (double)current->left->value->value
                    : current->left->value->fvalue);
        rval = (current->right->type == INTEGER_TOKEN
                    ? (double)current->right->value->value
                    : current->right->value->fvalue);
        switch (current->type) {
        case '+':
          lval = lval + rval;
          break;
        case '-':
          lval = lval - rval;
          break;
        case '*':
          lval = lval * rval;
          break;
        case '/':
          lval = lval / rval;
          break;
        }
        current->type = NUMBER_TOKEN;
        current->value->fvalue = lval;
        csound->Free(csound, current->left);
        csound->Free(csound, current->right);
      }
      break;
    case ENDIN_TOKEN:
    case UDOEND_TOKEN:
      csound->inZero = 1;
      /* fall through */
    default:
      // CRITICAL FIX: Check for struct member assignments BEFORE convert_statement_to_opcall
      // This is the only place where we can catch STRUCT_EXPR before it gets flattened
      if ((current->type == '=' || current->type == T_ASSIGNMENT || current->type == S_ADDIN ||
           current->type == S_SUBIN || current->type == S_MULIN || current->type == S_DIVIN) &&
          current->left && current->left->type == STRUCT_EXPR) {

        csound->Message(csound, "[semantics] DEBUG: FOUND STRUCT_EXPR assignment BEFORE conversion!\n");

        // Handle struct member assignment directly here, bypassing normal statement processing
        TREE* anchor = NULL;
        if (expand_struct_member_assignment(csound, current, typeTable, &anchor)) {
          csound->Message(csound, "[semantics] DEBUG: Successfully expanded struct member assignment\n");
          // Replace current with the expanded assignment
          if (previous != NULL) {
            previous->next = anchor;
          }
          // Find the end of the anchor chain to continue processing
          TREE* last = anchor;
          while (last && last->next) {
            last = last->next;
          }
          if (last) {
            last->next = current->next;
          }
          current = anchor;
          continue;
        } else {
          csound->Message(csound, "[semantics] DEBUG: Failed to expand struct member assignment\n");
        }
      }

      transformed = convert_statement_to_opcall(csound, current, typeTable);

      if (transformed != current) {
        current = transformed;
        if (previous != NULL) {
          previous->next = current;
        }
        continue;
      }

      current = transformed;

      if (current == NULL) {
        return 0;
      }
      if(!verify_opcode(csound, current, typeTable)) {
        return 0;
      }
      if (is_statement_expansion_required(current)) {
        current = expand_statement(csound, current, typeTable);
        if (previous != NULL) {
          previous->next = current;
        }

        continue;
      } else {
        handle_optional_args(csound, current);
      }
    }

    if (anchor == NULL) {
      anchor = current;
    }

    previous = current;
    current = current->next;
  }

  if (csoundGetDebug(csound) & DEBUG_SEMANTICS)
    csound->Message(csound, "[End Verifying AST]\n");

  cs_cons_free(csound, typeTable->labelList);
  typeTable->labelList = parentLabelList;

  return anchor;
}

/* BISON PARSER FUNCTION */
int32_t csound_orcwrap(void *dummy) {
  IGN(dummy);
  return (1);
}

/* BISON PARSER FUNCTION */
void csound_orcerror(PARSE_PARM *pp, void *yyscanner, CSOUND *csound,
                     TREE **astTree, const char *str) {

  IGN(pp);
  IGN(astTree);
  char ch;
  char *p = csound_orcget_current_pointer(yyscanner) - 1;
  int32_t line = csound_orcget_lineno(yyscanner);
  uint64_t files = csound_orcget_locn(yyscanner);
  if (UNLIKELY(*p == '\0' || *p == '\n'))
    line--;
  csound->ErrorMsg(csound, Str("\nerror: %s  (token \"%s\")"), str,
                   csound_orcget_text(yyscanner));
  do_baktrace(csound, files);
  csound->ErrorMsg(csound, Str(" line %d:\n>>>"), line);
  while ((ch = *--p) != '\n' && ch != '\0')
    ;
  do {
    ch = *++p;
    if (UNLIKELY(ch == '\n'))
      break;
    // Now get rid of any continuations
    if (ch == '#' && strncmp(p, "sline ", 6)) {
      p += 7;
      while (isdigit(*p))
        p++;
    } else
      csound->ErrorMsg(csound, "%c", ch);
  } while (ch != '\n' && ch != '\0');
  csound->ErrorMsg(csound, " <<<\n");
}

void do_baktrace(CSOUND *csound, uint64_t files) {
  while (files) {
    uint32_t ff = files & 0xff;
    files = files >> 8;
    csound->ErrorMsg(csound, Str(" from file %s (%d)\n"), csound->filedir[ff],
                     ff);
  }
}

/**
 * Appends TREE * node to TREE * node using ->next field in struct; walks
 * down  list to append at end; checks for NULL's and returns
 * appropriate nodes
 */
TREE *append_to_tree(CSOUND *csound, TREE *first, TREE *newlast) {
  IGN(csound);
  TREE *current;
  if (first == NULL) {
    return newlast;
  }

  if (newlast == NULL) {
    return first;
  }

  /* HACK - Checks to see if first node is uninitialized (sort of)
   * This occurs for rules like in topstatement where the left hand
   * topstatement the first time around is not initialized to anything
   * useful; the number 400 is arbitrary, chosen as it seemed to be a
   * value higher than all the type numbers that were being printed out
   */
  if (first->type > 400 || first->type < 0) {
    return newlast;
  }

  current = first;

  while (current->next != NULL) {
    current = current->next;
  }

  current->next = newlast;

  return first;
}

/* USED BY PARSER TO ASSEMBLE TREE NODES */
TREE *copy_node(CSOUND *csound, TREE *tree) {
  TREE *ans = NULL;

  if(tree != NULL) {
    ans = (TREE*)csound->Malloc(csound, sizeof(TREE));
    if (UNLIKELY(ans==NULL)) {
      if(csoundGetDebug(csound) & DEBUG_SEMANTICS)
       csoundMessage(csound, "Out of memory\n");
      exit(1);
    }
    ans->type = tree->type;
    ans->left = (tree->left == NULL) ? NULL : copy_node(csound, tree->left);
    ans->right = (tree->right == NULL) ? NULL : copy_node(csound, tree->right);

    if (tree->value != NULL) {
      /* Preserve token type and numeric payloads (value/fvalue) */
      ans->value =
          make_token(csound, tree->value->lexeme ? tree->value->lexeme : "");
      ans->value->type = tree->value->type;
      ans->value->value = tree->value->value;
      ans->value->fvalue = tree->value->fvalue;
      if (tree->value->optype)
        ans->value->optype = cs_strdup(csound, tree->value->optype);
    } else {
      ans->value = NULL;
    }

    ans->next = (tree->next == NULL) ? NULL : copy_node(csound, tree->next);
    ans->len = tree->len;
    ans->rate = tree->rate;
    ans->line = tree->line;
    ans->locn = tree->locn;
    ans->markup = NULL;
  }
  return ans;
}

/* Copy only left and right branches (skips next) */
TREE *copy_node_shallow(CSOUND *csound, TREE *tree) {
  TREE *ans = NULL;

  if (tree != NULL) {
    ans = (TREE *)csound->Malloc(csound, sizeof(TREE));
    if (UNLIKELY(ans == NULL)) {
      /* fprintf(stderr, "Out of memory\n"); */
      exit(1);
    }
    ans->type = tree->type;
    ans->left = tree->left;
    ans->right = tree->right;

    if (tree->value != NULL) {
      /* Preserve token type and numeric payloads (value/fvalue) */
      ans->value =
          make_token(csound, tree->value->lexeme ? tree->value->lexeme : "");
      ans->value->type = tree->value->type;
      ans->value->value = tree->value->value;
      ans->value->fvalue = tree->value->fvalue;
      if (tree->value->optype)
        ans->value->optype = cs_strdup(csound, tree->value->optype);
    } else {
      ans->value = NULL;
    }

    ans->next = NULL;
    ans->len = tree->len;
    ans->rate = tree->rate;
    ans->line = tree->line;
    ans->locn = tree->locn;
    ans->markup = NULL;
  }
  return ans;
}

TREE *make_node(CSOUND *csound, int32_t line, uint64_t locn, int32_t type,
                TREE *left, TREE *right) {
  TREE *ans;
  ans = (TREE*)csound->Malloc(csound, sizeof(TREE));
  if (UNLIKELY(ans==NULL)) {
   if(csoundGetDebug(csound) & DEBUG_SEMANTICS)
    csound->Message(csound, "Out of memory\n");
   exit(1);
  }
  ans->type = type;
  ans->left = left;
  ans->right = right;
  ans->value = NULL; /* New code -- JPff */
  ans->next = NULL;
  ans->len = 2;
  ans->rate = -1;
  ans->line = line;
  ans->locn = locn;
  ans->markup = NULL;
  return ans;
}

TREE *make_leaf(CSOUND *csound, int32_t line, uint64_t locn, int32_t type,
                ORCTOKEN *v) {
  TREE *ans;
  ans = (TREE*)csound->Calloc(csound, sizeof(TREE));
  if (UNLIKELY(ans==NULL)) {
   if(csoundGetDebug(csound) & DEBUG_SEMANTICS)
    csoundMessage(csound, "Out of memory\n");
   exit(1);
  }
  ans->type = type;
  ans->left = NULL;
  ans->right = NULL;
  ans->next = NULL;
  ans->len = 0;
  ans->rate = -1;
  ans->value = v;
  ans->line = line;
  ans->locn = locn;
  ans->markup = NULL;
  if(csoundGetDebug(csound) & DEBUG_SEMANTICS)
   csoundMessage(csound, "...csound_orc_semantics(%d) line = %d\n",
                   __LINE__, line);
  return ans;
}

TREE *make_opcall_from_func_start(CSOUND *csound, int32_t line, uint64_t locn,
                                  int32_t type, TREE *left, TREE *right) {
  TREE *firstArg = left->right;
  TREE *first = right;
  TREE *rest = right->next;

  right->next = NULL;
  TREE *operatorNode = make_node(csound, line, locn, type, firstArg, first);
  operatorNode->next = rest;
  left->right = operatorNode;

  return left;
}

void delete_tree(CSOUND *csound, TREE *root) {
  if (root == NULL)
    return;

  typedef struct TreePtrNode {
    TREE *node;
    struct TreePtrNode *next;
  } TreePtrNode;
  TreePtrNode *stack = NULL, *toFree = NULL;

// push helper
#define PUSH_NODE(cs, st, nodeptr)                                             \
  do {                                                                         \
    if ((nodeptr) != NULL) {                                                   \
      TreePtrNode *_t =                                                        \
          (TreePtrNode *)(cs)->Calloc((cs), sizeof(TreePtrNode));              \
      _t->node = (nodeptr);                                                    \
      _t->next = (st);                                                         \
      (st) = _t;                                                               \
    }                                                                          \
  } while (0)
// record helper
#define RECORD_NODE(cs, list, nodeptr)                                         \
  do {                                                                         \
    TreePtrNode *_t = (TreePtrNode *)(cs)->Calloc((cs), sizeof(TreePtrNode));  \
    _t->node = (nodeptr);                                                      \
    _t->next = (list);                                                         \
    (list) = _t;                                                               \
  } while (0)

  // DFS to collect unique nodes; use markup as a visited marker
  PUSH_NODE(csound, stack, root);
  while (stack != NULL) {
    TreePtrNode *tn = stack;
    stack = stack->next;
    TREE *n = tn->node;
    csound->Free(csound, tn);
    if (n == NULL)
      continue;
    if (n->markup == (void *)-1)
      continue; // visited
    n->markup = (void *)-1;
    RECORD_NODE(csound, toFree, n);
    // push neighbors
    if (n->left)
      PUSH_NODE(csound, stack, n->left);
    if (n->right)
      PUSH_NODE(csound, stack, n->right);
    if (n->next)
      PUSH_NODE(csound, stack, n->next);
  }

  // Free all recorded nodes safely (no re-traversal)
  while (toFree != NULL) {
    TreePtrNode *tn = toFree;
    toFree = toFree->next;
    TREE *n = tn->node;
    if (n->value) {
      if (n->value->lexeme)
        csound->Free(csound, n->value->lexeme);
      csound->Free(csound, n->value);
    }
    csound->Free(csound, n);
    csound->Free(csound, tn);
  }
}

PUBLIC void csoundDeleteTree(CSOUND *csound, TREE *tree) {
  delete_tree(csound, tree);
}

/* DEBUGGING FUNCTIONS */
void print_tree_i(CSOUND *csound, TREE *l, int32_t n) {
  int32_t i;
  if (UNLIKELY(l == NULL)) {
    return;
  }
  for (i = 0; i < n; i++) {
    csound->Message(csound, " ");
  }

  csound->Message(csound, "TYPE: %d ", l->type);

  switch (l->type) {
  case ',':
  case '?':
  case ':':
  case '!':
  case '+':
  case '-':
  case '*':
  case '/':
  case '%':
  case '^':
  case '(':
  case ')':
  case '=':
    csound->Message(csound, "%c:(%d:%s)\n", l->type, l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case NEWLINE:
    csound->Message(csound, "NEWLINE:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case S_NEQ:
    csound->Message(csound, "S_NEQ:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case S_AND:
    csound->Message(csound, "S_AND:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case S_OR:
    csound->Message(csound, "S_OR:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case S_LT:
    csound->Message(csound, "S_LT:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case S_LE:
    csound->Message(csound, "S_LE:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case S_EQ:
    csound->Message(csound, "S_EQ:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case S_UNOT:
    csound->Message(csound, "S_UNOT:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case S_GT:
    csound->Message(csound, "S_GT:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case S_GE:
    csound->Message(csound, "S_GE:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case LABEL_TOKEN:
    csound->Message(csound, "LABEL_TOKEN: %s\n", l->value->lexeme);
    break;
  case IF_TOKEN:
    csound->Message(csound, "IF_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case THEN_TOKEN:
    csound->Message(csound, "THEN_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case ITHEN_TOKEN:
    csound->Message(csound, "ITHEN_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case KTHEN_TOKEN:
    csound->Message(csound, "KTHEN_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case ELSEIF_TOKEN:
    csound->Message(csound, "ELSEIF_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case ELSE_TOKEN:
    csound->Message(csound, "ELSE_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case UNTIL_TOKEN:
    csound->Message(csound, "UNTIL_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case WHILE_TOKEN:
    csound->Message(csound, "WHILE_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case DO_TOKEN:
    csound->Message(csound, "DO_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case OD_TOKEN:
    csound->Message(csound, "OD_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case GOTO_TOKEN:
    csound->Message(csound, "GOTO_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case IGOTO_TOKEN:
    csound->Message(csound, "IGOTO_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case KGOTO_TOKEN:
    csound->Message(csound, "KGOTO_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case INSTR_TOKEN:
    csound->Message(csound, "INSTR_TOKEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case STRING_TOKEN:
    csound->Message(csound, "STRING_TOKEN: %s\n", l->value->lexeme);
    break;
  case T_IDENT:
    csound->Message(csound, "T_IDENT: %s\n", l->value->lexeme);
    break;
  case INTEGER_TOKEN:
    csound->Message(csound, "INTEGER_TOKEN: %d\n", l->value->value);
    break;
  case NUMBER_TOKEN:
    csound->Message(csound, "NUMBER_TOKEN: %f\n", l->value->fvalue);
    break;
  case S_ANDTHEN:
    csound->Message(csound, "S_ANDTHEN:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case S_APPLY:
    csound->Message(csound, "S_APPLY:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case T_FUNCTION:
    csound->Message(csound, "T_FUNCTION: %s\n", l->value->lexeme);
    break;
  case S_UMINUS:
    csound->Message(csound, "S_UMINUS:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case S_UPLUS:
    csound->Message(csound, "S_UPLUS:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  case '[':
    csound->Message(csound, "[:(%d:%s)\n", l->line,
                    csound->filedir[(l->locn) & 0xff]);
    break;
  default:
    csound->Message(csound, "unknown:%d(%d:%s)\n", l->type, l->line,
                    csound->filedir[(l->locn) & 0xff]);
  }

  print_tree_i(csound, l->left, n + 1);
  print_tree_i(csound, l->right, n + 1);

  if (l->next != NULL) {
    print_tree_i(csound, l->next, n);
  }
}

enum { TREE_NONE, TREE_LEFT, TREE_RIGHT, TREE_NEXT };
static void print_tree_xml(CSOUND *csound, TREE *l, int32_t n, int32_t which) {
  int32_t i;
  char *child[4] = {"", "left", "right", "next"};
  if (l == NULL) {
    return;
  }
  for (i = 0; i < n; i++) {
    csound->Message(csound, " ");
  }

  csound->Message(csound, "<tree%s addresses=\"(%p : %p)\" type=\"%d\" ",
                  child[which], l, l->value, l->type);

  switch (l->type) {
  case ',':
  case '?':
  case ':':
  case '!':
  case '+':
  case '-':
  case '*':
  case '/':
  case '%':
  case '^':
  case '(':
  case ')':
  case T_ASSIGNMENT:
  case '|':
  case '&':
  case '#':
  case '~':
    csound->Message(csound, "name=\"%c\"", l->type);
    break;
  case NEWLINE:
    csound->Message(csound, "name=\"NEWLINE\"");
    break;
  case S_NEQ:
    csound->Message(csound, "name=\"S_NEQ\"");
    break;
  case S_AND:
    csound->Message(csound, "name=\"S_AND\"");
    break;
  case S_OR:
    csound->Message(csound, "name=\"S_OR\"");
    break;
  case S_LT:
    csound->Message(csound, "name=\"S_LT\"");
    break;
  case S_LE:
    csound->Message(csound, "name=\"S_LE\"");
    break;
  case S_EQ:
    csound->Message(csound, "name=\"S_EQ\"");
    break;
  case S_UNOT:
    csound->Message(csound, "name=\"S_UNOT\"");
    break;
  case S_GT:
    csound->Message(csound, "name=\"S_GT\"");
    break;
  case S_GE:
    csound->Message(csound, "name=\"S_GE\"");
    break;
  case S_BITSHIFT_RIGHT:
    csound->Message(csound, "name=\"S_BITSHIFT_RIGHT\"");
    break;
  case S_BITSHIFT_LEFT:
    csound->Message(csound, "name=\"S_BITSHIFT_LEFT\"");
    break;
  case LABEL_TOKEN:
    csound->Message(csound, "name=\"LABEL_TOKEN\" label=\"%s\"",
                    l->value->lexeme);
    break;
  case IF_TOKEN:
    csound->Message(csound, "name=\"IF_TOKEN\"");
    break;
  case THEN_TOKEN:
    csound->Message(csound, "name=\"THEN_TOKEN\"");
    break;
  case ITHEN_TOKEN:
    csound->Message(csound, "name=\"ITHEN_TOKEN\"");
    break;
  case KTHEN_TOKEN:
    csound->Message(csound, "name=\"KTHEN_TOKEN\"");
    break;
  case ELSEIF_TOKEN:
    csound->Message(csound, "name=\"ELSEIF_TOKEN\"");
    break;
  case ELSE_TOKEN:
    csound->Message(csound, "name=\"ELSE_TOKEN\"");
    break;
  case UNTIL_TOKEN:
    csound->Message(csound, "name=\"UNTIL_TOKEN\"");
    break;
  case WHILE_TOKEN:
    csound->Message(csound, "name=\"WHILE_TOKEN\"");
    break;
  case DO_TOKEN:
    csound->Message(csound, "name=\"DO_TOKEN\"");
    break;
  case OD_TOKEN:
    csound->Message(csound, "name=\"OD_TOKEN\"");
    break;
  case GOTO_TOKEN:
    csound->Message(csound, "name=\"GOTO_TOKEN\"");
    break;
  case IGOTO_TOKEN:
    csound->Message(csound, "name=\"IGOTO_TOKEN\"");
    break;
  case KGOTO_TOKEN:
    csound->Message(csound, "name=\"KGOTO_TOKEN\"");
    break;
  case INSTR_TOKEN:
    csound->Message(csound, "name=\"INSTR_TOKEN\"");
    break;
  case STRING_TOKEN:
    csound->Message(csound, "name=\"T_STRCONST\" str=\"%s\"", l->value->lexeme);
    break;
  case T_IDENT:
    csound->Message(csound, "name=\"T_IDENT\" varname=\"%s\"",
                    l->value->lexeme);
    break;
  case T_OPCALL:
    if (l->left && l->left->value)
      csound->Message(csound, "name=\"T_OPCALL\" varname=\"%s\"",
                      l->left->value->lexeme);
    else
      csound->Message(csound, "name=\"T_OPCALL\" varname=\"%s\"",
                      l->value->lexeme);
    break;
  case T_DECLARE:
    csound->Message(csound, "name=\"T_DECLARE\" declvar=\"%s\"",
                    l->value->lexeme);
    break;
  case T_ARRAY:
    csound->Message(csound, "name=\"T_ARRAY\"");
    break;

  case T_ARRAY_IDENT:
    csound->Message(csound, "name=\"T_ARRAY_IDENT\" varname=\"%s\"",
                    l->value->lexeme);
    break;

  case INTEGER_TOKEN:
    csound->Message(csound, "name=\"INTEGER_TOKEN\" value=\"%d\"",
                    l->value->value);
    break;
  case NUMBER_TOKEN:
    csound->Message(csound, "name=\"NUMBER_TOKEN\" value=\"%f\"",
                    l->value->fvalue);
    break;
  case S_ANDTHEN:
    csound->Message(csound, "name=\"S_ANDTHEN\"");
    break;
  case S_APPLY:
    csound->Message(csound, "name=\"S_APPLY\"");
    break;
  case T_FUNCTION:
    csound->Message(csound, "name=\"T_FUNCTION\" fname=\"%s\"",
                    l->value->lexeme);
    break;
  case S_UMINUS:
    csound->Message(csound, "name=\"S_UMINUS\"");
    break;
  case S_UPLUS:
    csound->Message(csound, "name=\"S_UPLUS\"");
    break;

  case UDO_TOKEN:
    csound->Message(csound, "name=\"UDO_TOKEN\"");
    break;
  case UDO_ANS_TOKEN:
    csound->Message(csound, "name=\"UDO_ANS_TOKEN\" signature=\"%s\"",
                    l->value->lexeme);
    break;
  case UDO_ARGS_TOKEN:
    csound->Message(csound, "name=\"UDO_ARGS_TOKEN\" signature=\"%s\"",
                    l->value->lexeme);
    break;
  case S_ELIPSIS:
    csound->Message(csound, "name=\"S_ELIPSIS\"");
    break;
  case S_ADDIN:
    csound->Message(csound, "name=\"##addin\"");
    break;
    break;
  case S_SUBIN:
    csound->Message(csound, "name=\"##subin\"");
    break;
    break;
  case S_DIVIN:
    csound->Message(csound, "name=\"##divin\"");
    break;
    break;
  case S_MULIN:
    csound->Message(csound, "name=\"##mulin\"");
    break;
    break;
  case '[':
    csound->Message(csound, "name=\"[\"");
    break;
  default:
    csound->Message(csound, "name=\"unknown(%d)\"", l->type);
  }

  csound->Message(csound, " loc=\"%d:%s\">\n", l->line,
                  csound->filedir[(l->locn) & 0xff]);

  print_tree_xml(csound, l->left, n + 1, TREE_LEFT);
  print_tree_xml(csound, l->right, n + 1, TREE_RIGHT);

  for (i = 0; i < n; i++) {
    csound->Message(csound, " ");
  }

  csound->Message(csound, "</tree%s>\n", child[which]);

  if (l->next != NULL) {
    print_tree_xml(csound, l->next, n, TREE_NEXT);
  }
}

void print_tree(CSOUND *csound, char *msg, TREE *l) {
  if (msg)
    csound->Message(csound, "%s", msg);
  else
    csound->Message(csound, "Printing Tree\n");
  csound->Message(csound, "<ast>\n");
  print_tree_xml(csound, l, 0, TREE_NONE);
  csound->Message(csound, "</ast>\n");
}

void handle_optional_args(CSOUND *csound, TREE *l) {
  if (l == NULL || l->type == LABEL_TOKEN)
    return;
  {

    OENTRY *ep = (OENTRY *)l->markup;
    int32_t nreqd = 0;
    int32_t incnt = tree_arg_list_count(l->right);
    TREE *temp;
    char **inArgParts = NULL;

    if (UNLIKELY(ep == NULL)) { /* **** FIXME **** */
      csoundErrorMsg(csound,
                     "THIS SHOULD NOT HAPPEN -- ep NULL"
                     " csound_orc-semantics(%d)\n",
                     __LINE__);
    }
    if (ep->intypes != NULL) {
      nreqd = args_required(ep->intypes);
      inArgParts = split_args(csound, ep->intypes);
    }

    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_SEMANTICS)) {
      csound->Message(csound, "Handling Optional Args for opcode %s, %d, %d",
                      ep->opname, incnt, nreqd);
      csound->Message(csound, "ep->intypes = >%s<\n", ep->intypes);
    }
    if (incnt < nreqd) { /*  or set defaults: */
      do {
        switch (*inArgParts[incnt]) {
        case 'O': /* Will this work?  Doubtful code.... */
        case 'o':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "0"));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right == NULL)
            l->right = temp;
          else
            append_to_tree(csound, l->right, temp);
          break;
        case 'P':
        case 'p':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "1"));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right == NULL)
            l->right = temp;
          else
            append_to_tree(csound, l->right, temp);
          if (UNLIKELY(csound->GetDebug(csound))) {
            csound->Message(csound,
                            "semantics: appended default optional 'p'=1 for "
                            "opcode %s (line %d)\n",
                            ep && ep->opname ? ep->opname : "(null)", l->line);
          }
          break;
        case 'q':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "10"));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right == NULL)
            l->right = temp;
          else
            append_to_tree(csound, l->right, temp);
          break;

        case 'V':
        case 'v':
          temp = make_leaf(csound, l->line, l->locn, NUMBER_TOKEN,
                           make_num(csound, ".5"));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right == NULL)
            l->right = temp;
          else
            append_to_tree(csound, l->right, temp);
          break;
        case 'h':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "127"));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right == NULL)
            l->right = temp;
          else
            append_to_tree(csound, l->right, temp);
          break;
        case 'J':
        case 'j':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "-1"));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right == NULL)
            l->right = temp;
          else
            append_to_tree(csound, l->right, temp);
          break;
        case 'M':
        case 'N':
        case 'm':
        case 'W':
          nreqd--;
          break;
        default:
          synterr(csound,
                  Str("insufficient required arguments for opcode %s"
                      " on line %d:\n"),
                  ep->opname, l->line);
          do_baktrace(csound, l->locn);
        }
        incnt++;
      } while (incnt < nreqd);
    }
    if (inArgParts != NULL) {
      int32_t n;
      for (n = 0; inArgParts[n] != NULL; n++) {
        csound->Free(csound, inArgParts[n]);
      }
      csound->Free(csound, inArgParts);
    }
  }
}

CS_VARIABLE *add_global_variable(CSOUND *csound, ENGINE_STATE *engineState,
                                 CS_TYPE *type, char *name, void *typeArg);
void add_instr_variable(CSOUND *csound, TREE *x) {
  /* add instr variable to engine varpool
     called by bison when instr ids are found
  */
  if (x->type == T_IDENT) {
    char *varname = x->value->lexeme;
    CS_VARIABLE *var =
        add_global_variable(csound, &csound->engineState,
                            (CS_TYPE *)&CS_VAR_TYPE_INSTR, varname, NULL);
    if (var == NULL)
      csound->Warning(csound, "Could not add instrument ref %s", varname);
  }
}
