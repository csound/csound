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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "csoundCore.h"
#include "csound_orc.h"
#include "interlocks.h"
#include "namedins.h"
#include "parse_param.h"
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include "csound_orc_expressions.h"
#include "csound_orc_semantics.h"
#include "csound_orc_compile.h"


static CS_VAR_POOL *find_global_annotation(char *varName,
                                           TYPE_TABLE* typeTable);
static int32_t is_label(char* ident, CONS_CELL* labelList);
static char* convert_internal_to_external(CSOUND* csound, char* arg);
static int32_t is_reserved(char*);
char *check_optional_type(CSOUND *csound, char *name);
static char *resolve_struct_expr_type(CSOUND *csound, TREE *tree,
                                      TYPE_TABLE *typeTable);

char *csound_orcget_text ( void *scanner );
uint64_t csound_orcget_locn(void *);
int32_t add_udo_definition(CSOUND *csound, bool newStyle, char *opname,
                           char *outtypes, char *intypes, int32_t flags);

const char* SYNTHESIZED_ARG = "_synthesized";
const char* UNARY_PLUS = "_unary_plus";

char* csoundStrdup(CSOUND* csound, const char* str) {
  size_t len;
  char* retVal;

  if (str == NULL) return NULL;

  len = strlen(str);
  retVal = csound->Malloc(csound, len + 1);

  if (len > 0) {
    memcpy(retVal, str, len);
  }
  retVal[len] = '\0';

  return retVal;
}

char* cs_strndup(CSOUND* csound, const char* str, size_t size) {
  size_t len;
  char* retVal;

  if (str == NULL || size == 0) return NULL;
  len = strlen(str);

  if (size > len) { // catches if str is empty string
    return csoundStrdup(csound, str);
  }

  retVal = csound->Malloc(csound, size + 1);
  memcpy(retVal, str, size);
  retVal[size] = '\0';

  return retVal;
}

char* get_expression_opcode_type(CSOUND* csound, TREE* tree) {
  switch(tree->type) {
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
  }
  csound->Warning(csound, Str("Unknown function type found: %d [%c]\n"),
                  tree->type, tree->type);
  return NULL;
}

char* get_boolean_expression_opcode_type(CSOUND* csound, TREE* tree) {
  switch(tree->type) {
  case S_EQ:
    return "==";
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
  csound->Warning(csound,
                  Str("Unknown boolean expression type found: %d\n"),
                  tree->type);
  return NULL;
}

char* create_array_arg_type(CSOUND* csound, CS_VARIABLE* arrayVar) {
  if (arrayVar->subType == NULL) return NULL;

  char* varTypeName = arrayVar->subType->varTypeName;
  int32_t typeLen = (int32_t) strlen(varTypeName);
  int32_t len = arrayVar->dimensions + typeLen + 2;
  char* retVal = csound->Malloc(csound, len);

  // Use internal format: [typename] or [[typename]] for multi-dimensional
  memset(retVal, '[', arrayVar->dimensions);
  strNcpy(retVal + arrayVar->dimensions, varTypeName, typeLen + 1);
  retVal[len - 1] = '\0';
  retVal[len - 2] = ']';

  return retVal;
}

/* this checks if the annotated type exists */
char *check_annotated_type(CSOUND* csound, OENTRIES* entries,
                           char* outArgTypes) {
  int32_t i;
  for (i = 0; i < entries->count; i++) {
    OENTRY* temp = entries->entries[i];
    if (check_out_args(csound, outArgTypes, temp->outypes))
      return outArgTypes;
  }
  return NULL;
}

static int32_t is_irate(TREE *t)
{ /* check that argument is an i-rate constant or variable */
  if (t->type == INTEGER_TOKEN) {
    return 1;
  }
  else if (t->type == T_IDENT) {
    if (t->value->lexeme[0] != 'p' &&
        t->value->lexeme[0] != 'i' &&
        (t->value->lexeme[0] != 'g' ||
         t->value->lexeme[1] != 'i')) return 0;
    return 1;
  }
  else if (t->type == T_ARRAY) {
    if (is_irate(t->right)==0) return 0;
    t = t->next;
    while (t) {
      if (is_irate(t)==0) return 0;
      t = t->next;
    }
    return 1;
  }
  else return 0;
}

// VL 19-10-24
// this is now to be used everywhere to find a variable
// from any pool - global or local
// The search starts with implicit global vars
// then local vars, then any variables not found are
// looked for in the global pools - so local names will always
// hide global names
CS_VARIABLE* find_var_from_pools(CSOUND* csound, const char* varName,
                                 const char* varBaseName, TYPE_TABLE* typeTable) {
  CS_VARIABLE* var = NULL;

  // we first check for local variables
  var = csoundFindVariableWithName(csound, typeTable->localPool,
                                   varBaseName);
  // then check for global variables in engine
  if(var == NULL)
    var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                     varBaseName);
  // and finally newly defined global vars
  if(var == NULL)
    var = csoundFindVariableWithName(csound, typeTable->globalPool,
                                     varBaseName);

  return var;
}

/*
  Check a symbol for pfield format (pN, PN)
  and return the p-field num ( >= 0 )
  else return -1
*/
static int32_t is_pfield(CSOUND *csound, TYPE_TABLE* typeTable, char *s)
{
  CS_VARIABLE *var = find_var_from_pools(csound, s, s, typeTable);
  // if symbol does not exist as a variable
  // or if it is a pfield type var
  if(var == NULL || var->varType == &CS_VAR_TYPE_P) {
    int32_t n;
    if (*s == 'p' || *s == 'P')
      if (sscanf(++s, "%d", &n))
        return (n);
  }
  return (-1);
}

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

  // Handle nested STRUCT_EXPR (e.g., var1.complex.imag)
  if (tree->left->type == STRUCT_EXPR) {
    char *nestedType = resolve_struct_expr_type(csound, tree->left, typeTable);
    if (nestedType == NULL) {
      return NULL;
    }
    const CS_TYPE *nestedStructType =
        csoundGetTypeWithVarTypeName(csound->typePool, nestedType);
    csound->Free(csound, nestedType);
    if (nestedStructType == NULL) {
      synterr(csound, Str("Cannot find type for nested struct expression\n"));
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
      synterr(csound, Str("No member '%s' found for variable at line %d\n"), s, tree->line);
      do_baktrace(csound, tree->locn);
      return NULL;
    }
    // Return array notation if final member is array
    if (memberVar->varType == &CS_VAR_TYPE_ARRAY) {
      char *result = create_array_arg_type(csound, memberVar);
      if (result == NULL) {
        synterr(csound, Str("Array member has unknown type at line %d\n"), tree->line);
        do_baktrace(csound, tree->locn);
        return NULL;
      }
      return result;
    }
    return csoundStrdup(csound, memberVar->varType->varTypeName);
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
          return NULL;
        }

        char *elementType = NULL;
        int len = (int)strlen(structArrayType);
        if (len >= 3 && structArrayType[0] == '[' &&
            structArrayType[len - 1] == ']') {
          elementType = csound->Malloc(csound, len - 1);
          strncpy(elementType, structArrayType + 1, len - 2);
          elementType[len - 2] = '\0';
        } else if (structArrayType[0] == ':') {
          char *semicolon = strchr(structArrayType, ';');
          if (semicolon != NULL) {
            int typeNameLen = (int)(semicolon - structArrayType - 1);
            elementType = csound->Malloc(csound, typeNameLen + 1);
            strncpy(elementType, structArrayType + 1, typeNameLen);
            elementType[typeNameLen] = '\0';
          }
        } else if (len > 2 && structArrayType[len - 2] == '[' &&
                   structArrayType[len - 1] == ']') {
          elementType = csoundStrdup(csound, structArrayType);
          elementType[len - 2] = '\0';
        }
        if (elementType != NULL) {
          // Ensure user-defined names are in internal format :Type;
          char *internalElementType = NULL;
          if (strlen(elementType) > 1 && elementType[0] != ':' && elementType[0] != '[') {
            size_t bl = strlen(elementType);
            internalElementType = csound->Malloc(csound, bl + 3);
            internalElementType[0] = ':';
            memcpy(internalElementType + 1, elementType, bl);
            internalElementType[bl + 1] = ';';
            internalElementType[bl + 2] = '\0';
          } else {
            internalElementType = check_optional_type(csound, elementType);
            if (!internalElementType) internalElementType = csoundStrdup(csound, elementType);
          }
          const CS_TYPE *structType = csoundGetTypeWithVarTypeName(
              csound->typePool, internalElementType);
          csound->Free(csound, elementType);
          csound->Free(csound, internalElementType);
          csound->Free(csound, structArrayType);
          if (structType == NULL) {
            synterr(csound, Str("Cannot find struct type for array element at line %d\n"), tree->line);
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
            synterr(csound, Str("No member '%s' found in struct at line %d\n"), s, tree->line);
            do_baktrace(csound, tree->locn);
            return NULL;
          }
          char *result;
          if (memberVar->varType == &CS_VAR_TYPE_ARRAY) {
            result = create_array_arg_type(csound, memberVar);
            if (result == NULL) {
              synterr(csound, Str("Array member has unknown type at line %d\n"), tree->line);
              do_baktrace(csound, tree->locn);
              return NULL;
            }
          } else {
            result = csoundStrdup(csound, memberVar->varType->varTypeName);
          }
          return result;
        } else {
          synterr(csound, Str("Expected array type but got '%s' at line %d\n"),
                  structArrayType, tree->line);
          csound->Free(csound, structArrayType);
          do_baktrace(csound, tree->locn);
          return NULL;
        }
      } else if (tree->left->left->value != NULL &&
                 tree->left->left->value->lexeme != NULL) {
        // Simple array access like var0[indx]
        s = tree->left->left->value->lexeme;
      } else {
        synterr(csound,
                Str("STRUCT_EXPR: Cannot find struct name for array access at line %d\n"),
                tree->line);
        do_baktrace(csound, tree->locn);
        return NULL;
      }
    } else {
      synterr(csound,
              Str("STRUCT_EXPR: Unexpected structure for array access at line %d\n"),
              tree->line);
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
    synterr(csound, Str("Variable '%s' used before defined at line %d\n"), s, tree->line);
    do_baktrace(csound, tree->locn);
    return NULL;
  }

  // For array access like var0[indx].member, use the element type for member
  // lookup
  const CS_TYPE *structType = var->varType;
  if (tree->left->value == NULL && tree->left->type == T_ARRAY) {
    if (var->varType != &CS_VAR_TYPE_ARRAY) {
      synterr(csound, Str("Variable '%s' is not an array at line %d\n"), s, tree->line);
      do_baktrace(csound, tree->locn);
      return NULL;
    }
    if (var->subType == NULL) {
      synterr(csound, Str("Array '%s' has no element type defined at line %d\n"), s, tree->line);
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
      synterr(csound, Str("No member '%s' found for variable at line %d\n"), s, tree->line);
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
      synterr(csound, Str("Array member has unknown type at line %d\n"), tree->line);
      do_baktrace(csound, tree->locn);
      return NULL;
    }
  } else {
    result = csoundStrdup(csound, var->varType->varTypeName);
  }
  return result;
}

/* convert array type string from type[] to [type]
   accepts multidimensional type strings (type[][]) 
   non-op for non-array type strings
*/
static const char *convert_array_type_string(CSOUND *csound,
                                       const char *str) {
  const char *check = str;
  char dim = 0;
  while(*check != '\0') {
    if(*check == '[') dim++;
    check++;
  }
  if(dim){
    int len = (int) strlen(str);
    char *arr = csoundCalloc(csound, strlen(str)+1);
    for(int i = 0; i < dim; i++) {
      arr[i] = '[';
    }
    memcpy(arr+dim, str, len-dim);
    for(int i = len-dim; i < len; i++) {
      arr[i] = ']';
    }
    return arr;
  } else return str;
}

/* This function gets arg type with checking type table */
char* get_arg_type2(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable)
{
  char* s;
  char* t;
  //CS_TYPE* type;
  CS_VARIABLE* var;
  char* varBaseName;

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
        if ((fnReturn = get_arg_type2(csound, tree->left, typeTable)) &&
            *fnReturn == '[') {
          // Strip both '[' and ']' from array type notation like "[i]" to get "i"
          size_t len = strlen(fnReturn);
          if (len >= 3 && fnReturn[len-1] == ']') {
            char *result = csound->Malloc(csound, len - 1);
            strncpy(result, &fnReturn[1], len - 2);
            result[len - 2] = '\0';
            csound->Free(csound, fnReturn);
            return result;
          } else {
            csound->Free(csound, fnReturn);
            return NULL;
          }
        } else {
          if (fnReturn) csound->Free(csound, fnReturn);
          synterr(csound,
                  Str("non-array type for function %s line %d\n"),
                  tree->left->value->lexeme, tree->line);
          do_baktrace(csound, tree->locn);
          return NULL;
        }
      } else if (tree->left->type == STRUCT_EXPR) {
        // Handle struct member array access like users.names[0]
        char *memberType = get_arg_type2(csound, tree->left, typeTable);
        if (memberType) {
          size_t len = strlen(memberType);
          // Check if it's an array type - internal format is [type] or [[type]] etc.
          if (len >= 3 && memberType[0] == '[' && memberType[len-1] == ']') {
            // Return the element type (strip the [ prefix and ] suffix)
            // memberType is like "[S]", we want "S"
            char *result = csound->Malloc(csound, len - 1); // len-2 for content + 1 for null terminator
            strncpy(result, &memberType[1], len - 2);
            result[len - 2] = '\0';
            csound->Free(csound, memberType);
            return result;
          } else {
            csound->Free(csound, memberType);
            synterr(csound,
                    Str("non-array type for struct member access at line %d\n"),
                    tree->line);
            do_baktrace(csound, tree->locn);
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
        varBaseName = tree->left->value->lexeme;
        var = find_var_from_pools(csound, varBaseName, varBaseName, typeTable);

        if (var == NULL) {
          synterr(csound,
                  Str("unable to find array operator for var %s line %d\n"),
                  varBaseName, tree->line);
          do_baktrace(csound, tree->locn);
          return NULL;
        } else {
          // Generic array (CS_VAR_TYPE_ARRAY) vs typed arrays (dimensions>0 with element varType)
          if (var->varType == &CS_VAR_TYPE_ARRAY) {
            // Generic array: element type in subType
            return csoundStrdup(csound, var->subType->varTypeName);
          } else if (var->dimensions > 0) {
            // Typed array (e.g., k[], i[], S[]): element type is varType itself
            return csoundStrdup(csound, var->varType->varTypeName);
          } else if (var->varType == &CS_VAR_TYPE_A) {
            return csoundStrdup(csound, "k");
          } else if (tree->type == T_ARRAY) {
            // If we're accessing as T_ARRAY but have no subType/dimensions,
            // it's a typed array like k[], return the varType (element type)
            return csoundStrdup(csound, var->varType->varTypeName);
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
      return csoundStrdup(csound, out);

    }

    // Deal with odd case of i(expressions)
    if (tree->type == T_FUNCTION && !strcmp(tree->value->lexeme, "i")) {
      if(csoundGetDebug(csound) & DEBUG_SEMANTICS)
        print_tree(csound, "i()", tree);
      if (tree->right->type == T_ARRAY &&
          tree->right->left->type == T_IDENT &&
          is_irate(tree->right->right)) {
        synterr(csound, Str("Use of i() with array element ill formed on line %d\n"), tree->line);
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
        char *ret = csoundStrdup(csound, args[0]);
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
                Str("Unable to verify arg types for expression '%s' at line %d\n"),
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
      return csoundStrdup(csound, out);

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
      return csoundStrdup(csound, out);

    }
  }

  if(tree == NULL) {
   synterr(csound, "NULL tree");
   longjmp(csound->exitjmp,0);
   return 0;
  }

  switch(tree->type) {
  case NUMBER_TOKEN:
  case INTEGER_TOKEN:
    return csoundStrdup(csound, "c");     /* const */
  case FALSE_TOKEN: {  // trap false expr here
    CS_VARIABLE *var = find_var_from_pools(csound, "false",
                                           "false", typeTable);
    if(var == NULL) {
    var = add_global_variable(csound, &csound->engineState,
                        (CS_TYPE*)&CS_VAR_TYPE_b, "false", NULL);
    int32_t *p = (int32_t *) &(var->memBlock->value);
    *p = 0;
    }
  }
  return csoundStrdup(csound, "b");  /* boolean */
  case TRUE_TOKEN: { // trap true expr here
     CS_VARIABLE *var = find_var_from_pools(csound, "true",
                                           "true", typeTable);
    if(var == NULL) {
     var = add_global_variable(csound, &csound->engineState,
                        (CS_TYPE*)&CS_VAR_TYPE_b, "true", NULL);
    int32_t *p = (int32_t *) &(var->memBlock->value);
    *p = 1;
    }
  }
  return csoundStrdup(csound, "b");     /* boolean */
  case FALSEK_TOKEN: {  // trap false expr here
    CS_VARIABLE *var = find_var_from_pools(csound, "falsek",
                                           "falsek", typeTable);
    if(var == NULL) {
    var = add_global_variable(csound, &csound->engineState,
                        (CS_TYPE*)&CS_VAR_TYPE_B, "falsek", NULL);
    int32_t *p = (int32_t *) &(var->memBlock->value);
    *p = 0;
    }
  }
  return csoundStrdup(csound, "B");  /* Boolean */
  case TRUEK_TOKEN: { // trap true expr here
     CS_VARIABLE *var = find_var_from_pools(csound, "truek",
                                           "truek", typeTable);
    if(var == NULL) {
     var = add_global_variable(csound, &csound->engineState,
                        (CS_TYPE*)&CS_VAR_TYPE_B, "truek", NULL);
    int32_t *p = (int32_t *) &(var->memBlock->value);
    *p = 1;
    }
  }
  return csoundStrdup(csound, "B");     /* Boolean */
  case STRING_TOKEN:
    return csoundStrdup(csound, "S");   /* quoted String */
  case LABEL_TOKEN:
    //FIXME: Need to review why label token is used so much in parser,
    //for now treat as T_IDENT
  case T_ARRAY_IDENT:
    //check
    if((var = csoundFindVariableWithName(csound, typeTable->localPool,
                                         tree->value->lexeme)) != NULL) {
      if(var->varType != &CS_VAR_TYPE_ARRAY) {
      synterr(csound, Str("Array variable name '%s' used before as a different "
                          "type at line %d\n"),
              tree->value->lexeme, tree->line);
      do_baktrace(csound, tree->locn);
      return NULL;
      }
    }
    __attribute__((fallthrough));
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
         return csoundStrdup(csound, var->varType->varTypeName);
     }

    if(!strcmp(tree->value->lexeme, "this_instr")) {
      const CS_TYPE *varType = &CS_VAR_TYPE_INSTR;
       // found this, return type.
       return csoundStrdup(csound, varType->varTypeName);
     }

     if(!strcmp(tree->value->lexeme, "this")) {
      const CS_TYPE *varType = &CS_VAR_TYPE_INSTR_INSTANCE;
       // found this, return type.
       return csoundStrdup(csound, varType->varTypeName);
     }

    if (is_reserved(s)) {
      return csoundStrdup(csound, "r");                              /* rsvd */
    }

    if (is_label(s, typeTable->labelList)) {
      return csoundStrdup(csound, "l");
    }

    if ((*s >= '1' && *s <= '9') || *s == '.' || *s == '-' || *s == '+' ||
        (*s == '0' && strcmp(s, "0dbfs") != 0))
      return csoundStrdup(csound, "c");                          /* const */
    if (*s == '"')
      return csoundStrdup(csound, "S");

    if (is_pfield(csound, typeTable, s) >= 0)
      return csoundStrdup(csound, "p");                      /* p-field number */

    varBaseName = s;

    if (*s == '#') {
      // find synthetic vars
      s++;
      var = find_var_from_pools(csound, s, tree->value->lexeme, typeTable);
    }
    else {
      // other vars
      // make a copy to preserve the lexeme
      char *s_copy = csoundStrdup(csound, s);
      // strip @global if it exists, it's a non-op here
      find_global_annotation(s_copy, typeTable);
      // find the variable in one of the variable pools
      var = find_var_from_pools(csound, s_copy, s_copy, typeTable);
      csound->Free(csound, s_copy);
    }

    if (UNLIKELY(var == NULL)) {
      synterr(csound, Str("get_arg_type2: Variable '%s' used before defined, line %d"),
              tree->value->lexeme, tree->line);
      do_baktrace(csound, tree->locn);
      return NULL;
    }

     if (var->varType == &CS_VAR_TYPE_ARRAY) {
        char *res = create_array_arg_type(csound, var);
        if (res==NULL) {        /* **REVIEW** this double syntax error */
          synterr(csound, Str("Array of unknown type at line %d\n"), tree->line);
          do_baktrace(csound, tree->locn);
        }
        return res;
      } else {
        return csoundStrdup(csound, var->varType->varTypeName);
      }

  case T_TYPED_IDENT:
    // in the case of arrays, we need to convert from type[] to [type]
    // so that semantic analysis can identify the type correctly
    return csoundStrdup(csound,
                        convert_array_type_string(csound, tree->value->optype));
  case STRUCT_EXPR:
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

    char* retVal = csound->Malloc(csound, (len + 2) * sizeof(char));
    memcpy(retVal, s, len);
    retVal[len] = ']';
    retVal[len + 1] = '\0';

    return retVal;

  default:
    csoundWarning(csound, Str("Unknown arg type: %d at line %d\n"), tree->type, tree->line);
    print_tree(csound, "Arg Tree\n", tree);
    return NULL;
  }
}



char* get_opcode_short_name(CSOUND* csound, char* opname) {

  char* dot = strchr(opname, '.');
  if (dot != NULL) {
    uint64_t opLen = dot - opname;
    return cs_strndup(csound, opname, opLen);
  }
  return opname;
}

/* find opcode with the specified name in opcode list */
/* returns index to opcodlst[], or zero if the opcode cannot be found */
OENTRY* find_opcode(CSOUND *csound, char *opname)
{
  char *shortName;
  CONS_CELL* head;
  OENTRY* retVal;

  if (opname[0] == '\0' || isdigit(opname[0]))
    return 0;

  shortName = get_opcode_short_name(csound, opname);

  head = cs_hash_table_get(csound, csound->opcodes, shortName);

  retVal = (head != NULL) ? head->value : NULL;
  if (shortName != opname) csound->Free(csound, shortName);

  return retVal;

   }

static OENTRIES* get_entries(CSOUND* csound, int32_t count)
{
  OENTRIES* x = csound->Calloc(csound, sizeof(OENTRIES*)+sizeof(OENTRY*)*count);
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

  /* Lock to protect hash table read during concurrent compilations */
  if (csound->init_pass_threadlock)
    csoundLockMutex(csound->init_pass_threadlock);

  head = cs_hash_table_get(csound, csound->opcodes, shortName);
  retVal = get_entries(csound, cs_cons_length(head));
  while (head != NULL) {
    retVal->entries[i++] = head->value;
    head = head->next;
  }

  if (csound->init_pass_threadlock)
    csoundUnlockMutex(csound->init_pass_threadlock);

  if (shortName != opname) {
    csound->Free(csound, shortName);
  }

  return retVal;

}

inline static int32_t is_in_optional_arg(char* arg) {
  return (strlen(arg) == 1) && (strchr("opqvjhOJVP?", *arg) != NULL);
}

inline static int32_t is_in_var_arg(char* arg) {
  return (strlen(arg) == 1) && (strchr("mMNnWyzZ*", *arg) != NULL);
}

int32_t check_array_arg(char* found, char* required) {
  char* f = found;
  char* r = required;

  while (*r == '[') r++;

  if (*r == '.' || *r == '?' || *r == '*') {
    return 1;
  }

  while (*f == '[') f++;

  return (*f == *r);
}

int32_t check_array_arg_in(char* found, char* required) {
  char* f = found;
  char* r = required;

  while (*r == '[') r++;

  if (*r == '.' || *r == '?' || *r == '*') {
    return 1;
  }

  while (*f == '[') f++;

  // special case: k args with i inputs
  if(*r == 'k' && *f == 'i') return 1;
  return (*f == *r);
}




/* Helper function to normalize type names for comparison */
static char* normalize_type_for_comparison(char* typeName) {
  if (!typeName) return NULL;

  // If it's in internal format (:TypeName;), extract the core name
  if (typeName[0] == ':' && typeName[strlen(typeName) - 1] == ';') {
    size_t len = strlen(typeName) - 2; // Remove : and ;
    char* result = malloc(len + 1);
    memcpy(result, typeName + 1, len);
    result[len] = '\0';
    return result;
  }

  // Otherwise, return a copy of the original name
  return strdup(typeName);
}

int32_t check_in_arg(char* found, char* required) {
  char* t;
  int32_t i;
  if (UNLIKELY(found == NULL || required == NULL)) {
    return 0;
  }

  if (strcmp(found, required) == 0) {
    return 1;
  }

  // For user-defined types, normalize both sides and compare
  if (strlen(found) > 1 || strlen(required) > 1) {
    char* found_normalized = normalize_type_for_comparison(found);
    char* required_normalized = normalize_type_for_comparison(required);
    int32_t result = (found_normalized && required_normalized &&
                      strcmp(found_normalized, required_normalized) == 0);
    if (found_normalized) free(found_normalized);
    if (required_normalized) free(required_normalized);
    if (result) return 1;
  }



  // k-rate scalar can be used as k-rate boolean (B)
  if (found[0] == 'k' && required[0] == 'B') {
    return 1;
  }

  if (*required == '.' || *required == '?' || *required == '*') {
    return 1;
  }

  if (*found == '[' || *required == '[') {
    if (*found != *required) {
      return 0;
    }
    return check_array_arg_in(found, required);
  }

  t = (char*)POLY_IN_TYPES[0];

  for (i = 0; t != NULL; i += 2) {
    if (strcmp(required, t) == 0) {
      return (strchr(POLY_IN_TYPES[i + 1], *found) != NULL);
    }
    t = (char*)POLY_IN_TYPES[i + 2];
  }

  if (is_in_optional_arg(required)) {
    t = (char*)OPTIONAL_IN_TYPES[0];
    for (i = 0; t != NULL; i += 2) {
      if (strcmp(required, t) == 0) {
        return (strchr(OPTIONAL_IN_TYPES[i + 1], *found) != NULL);
      }
      t = (char*)OPTIONAL_IN_TYPES[i + 2];
    }
  }

  if (!is_in_var_arg(required)) {
    return 0;
  }

  t = (char*)VAR_ARG_IN_TYPES[0];
  for (i = 0; t != NULL; i += 2) {
    if (strcmp(required, t) == 0) {
      return (strchr(VAR_ARG_IN_TYPES[i + 1], *found) != NULL);
    }
    t = (char*)VAR_ARG_IN_TYPES[i + 2];
  }
  return 0;
}

int32_t check_in_args(CSOUND* csound, char* inArgsFound, char* opInArgs) {
  if ((inArgsFound == NULL || strlen(inArgsFound) == 0) &&
      (opInArgs == NULL || strlen(opInArgs) == 0)) {
    return 1;
  }

  if (UNLIKELY(opInArgs == NULL)) {
    return 0;
  }

  {
    int32_t argsFoundCount = args_required(inArgsFound);
    int32_t args_requiredCount = args_required(opInArgs);
    char** args_required = split_args(csound, opInArgs);
    char** argsFound;
    int32_t i;
    int32_t argTypeIndex = 0;
    char* varArg = NULL;
    int32_t returnVal = 1;

    if (args_required == NULL) {
      return 0;
    }
    if (argsFoundCount>=VARGMAX) {
      return -1;
    }

    if ((argsFoundCount > args_requiredCount) &&
        !(is_in_var_arg(args_required[args_requiredCount - 1]))) {
      csound->Free(csound, args_required);
      return 0;
    }

    argsFound = split_args(csound, inArgsFound);

    if (argsFoundCount == 0) {
      if (is_in_var_arg(args_required[0])) {
        varArg = args_required[0];
      }
    } else {
      for (i = 0; i < argsFoundCount; i++) {
        char* argFound = argsFound[i];

        if (varArg != NULL) {
          if (!check_in_arg(argFound, varArg)) {
            returnVal = 0;
            break;
          }
        } else {
          char* argRequired = args_required[argTypeIndex++];
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
        char* c = args_required[argTypeIndex++];

        if (!is_in_optional_arg(c) && !is_in_var_arg(c)) {
          returnVal = 0;
          break;
        }
      }

    }

    int32_t n;
    for (n=0; argsFound[n] != NULL; n++) {
      csound->Free(csound, argsFound[n]);
    }
    csound->Free(csound, argsFound);
    for (n=0; args_required[n] != NULL; n++) {
      csound->Free(csound, args_required[n]);
    }
    csound->Free(csound, args_required);

    return returnVal;
  }
}

inline static int32_t is_out_var_arg(char* arg) {
  return strlen(arg) == 1 && (strchr("mzIXNvVF*", *arg) != NULL);
}

int32_t check_out_arg(char* found, char* required) {
  char* t;
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

  if (*found == '[' || *required == '[') {
    if (*found != *required) {
      return 0;
    }
    return check_array_arg(found, required);
  }

  if (strcmp(found, required) == 0) {
    return 1;
  }

  // For user-defined types, normalize both sides and compare
  if (strlen(found) > 1 || strlen(required) > 1) {
    char* found_normalized = normalize_type_for_comparison(found);
    char* required_normalized = normalize_type_for_comparison(required);
    int32_t result = (found_normalized && required_normalized &&
                      strcmp(found_normalized, required_normalized) == 0);
    if (found_normalized) free(found_normalized);
    if (required_normalized) free(required_normalized);
    if (result) return 1;
  }

  // check for multichar types now
  if(strlen(found) > 1 &&
     strcmp(found, required)) {
    return 0;
  }


  t = (char*)POLY_OUT_TYPES[0];
  for (i = 0; t != NULL; i += 2) {
    if (strcmp(required, t) == 0) {
      return (strchr(POLY_OUT_TYPES[i + 1], *found) != NULL);
    }
    t = (char*)POLY_OUT_TYPES[i + 2];
  }

  if (!is_out_var_arg(required)) {
    return 0;
  }

  t = (char*)VAR_ARG_OUT_TYPES[0];
  for (i = 0; t != NULL; i += 2) {
    if (strcmp(required, t) == 0) {
      return (strchr(VAR_ARG_OUT_TYPES[i + 1], *found) != NULL);
    }
    t = (char*)VAR_ARG_OUT_TYPES[i + 2];
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


OENTRY* resolve_opcode_exact(CSOUND* csound, OENTRIES* entries,
                             char* outArgTypes, char* inArgTypes) {
  IGN(csound);
  int32_t i;

  const char* outTest = (outArgTypes == NULL || !strcmp("0", outArgTypes)) ?
                          "" : outArgTypes;
  const char* inTest = (inArgTypes == NULL || !strcmp("0", inArgTypes)) ?
                         "" : inArgTypes;
  for (i = 0; i < entries->count; i++) {
    OENTRY* temp = entries->entries[i];
    if (temp->intypes != NULL && !strcmp(inTest, temp->intypes) &&
        temp->outypes != NULL && !strcmp(outTest, temp->outypes)) {
      return temp;
    }
  }
  return NULL;
}

/* Given an OENTRIES list, resolve to a single OENTRY* based on the
 * found in- and out- argtypes.  Returns NULL if opcode could not be
 * resolved. If more than one entry matches, mechanism assumes there
 * are multiple opcode entries with same types and last one should
 * override previous definitions.
 */
OENTRY* resolve_opcode(CSOUND* csound, OENTRIES* entries,
                       char* outArgTypes, char* inArgTypes) {
  int32_t i, check;
  OENTRY* exact;

  /* A polymorphic or variadic entry must not shadow a concrete signature. */
  if ((exact = resolve_opcode_exact(csound, entries,
                                    outArgTypes, inArgTypes)) != NULL) {
    return exact;
  }

  for (i = 0; i < entries->count; i++) {
    OENTRY* temp = entries->entries[i];
    if ((check = check_in_args(csound, inArgTypes, temp->intypes)) &&
        check_out_args(csound, outArgTypes, temp->outypes)) {
      if (check == -1) {
        synterr(csound,
                Str("Found %d inputs for %s which is more than "
                    "the %d allowed\n"),
                args_required(inArgTypes), temp->opname, VARGMAX);
        return NULL;
      }
      return temp;
    }
  }
  return NULL;
}

/* used when creating T_FUNCTION's */
char* resolve_opcode_get_outarg(CSOUND* csound, OENTRIES* entries,
                                char* inArgTypes) {
  int32_t i;

  for (i = 0; i < entries->count; i++) {
    OENTRY* temp = entries->entries[i];
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

/**
 * Converts array type from INTERNAL to EXTERNAL format.
 *
 * Array Type Format Contract:
 * ---------------------------
 * INTERNAL format: Brackets before type, single closing bracket
 *   - "[k]" = 1D k-rate array
 *   - "[[a]" = 2D audio array
 *   - "[:MyType;]" = 1D UDT array
 *
 * EXTERNAL format: Type followed by bracket pairs
 *   - "k[]" = 1D k-rate array
 *   - "a[][]" = 2D audio array
 *   - ":MyType;[]" = 1D UDT array
 *
 * EXTERNAL format is used in OENTRY.intypes/outypes and is human-readable.
 * INTERNAL format is used in parse trees and after split_args() processing.
 *
 * NOTE: This function also wraps non-array UDT names with :; delimiters.
 */
char* convert_internal_to_external(CSOUND* csound, char* arg) {
  int32_t i = 0, dimensions;
  char *start;
  char *retVal, *current;
  uint64_t nameLen, len = strlen(arg);
  char *type;

  if (arg == NULL || len == 1) {
    return arg;
  }

  // VL 15.10.24
  // synthetic args reach here with
  // : prepended and ; appended to name
  // so we need to remove them to avoid
  // accummulation
  // now remove any : or ; leftover in typename
  type = remove_type_quoting(csound, arg);

  // Check if this is already a properly formatted struct array type
  // (e.g., ":MyType;[]")
  if (arg[0] == ':' && strstr(arg, ";[") != NULL) {
    // This is already in external format, return as-is
    csound->Free(csound, type);
    return csoundStrdup(csound, arg);
  }

  // If this is already in external primitive array form like "k[]" or "a[][]",
  // do not attempt to convert it; just return a copy as-is.
  if (arg[0] != '[' && arg[0] != ':' && strchr(arg, '[') != NULL) {
    csound->Free(csound, type);
    return csoundStrdup(csound, arg);
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
    /* User-Defined Struct - use single-escaping for type strings */
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
    nameLen += 2;  // Need 2 extra chars for single-escaping (:;)
  }

  retVal = csound->Malloc(csound, sizeof(char) * (nameLen + (dimensions * 2)
                                                  + 1));
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
char* convert_external_to_internal(CSOUND* csound, char* arg) {
  int32_t i, dimensions;
  char* retVal;

  if (arg == NULL || *(arg + 1) != '[') {
    return arg;
  }

  dimensions = ((int32_t)strlen(arg) - 1) / 2;

  retVal = csound->Malloc(csound, sizeof(char) * (dimensions + 3));
  retVal[dimensions + 2] = '\0';
  retVal[dimensions + 1] = ']';
  retVal[dimensions] = *arg;

  for (i = 0; i < dimensions; i++) {
    retVal[i] = '[';
  }
  return retVal;
}


static int is_external(const char *s) {
  if(*s != '[') {
    if(strchr(s+1, '[') != NULL)
        return 1;
  }
  return 0;
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
      	// catch type[] in expressions to opcall - no conversion
      if(!is_external(argType)) {
        argType = convert_internal_to_external(csound, argType);
      }
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



/**
 * Builds a concatenated input type string from the parse tree for new-style UDOs.
 *
 * Array Type Format Contract:
 * ---------------------------
 * OUTPUT: Returns a string in EXTERNAL format for use in OENTRY.intypes
 *         Examples: "ik[]", "a:MyType;[]", "SS"
 *         - Primitive arrays: "k[]", "a[][]"
 *         - UDT arrays: ":TypeName;[]"
 *
 * IMPORTANT: get_arg_type2() returns INTERNAL format ("[k]"), so we must
 * convert to EXTERNAL format before concatenation. Failure to do so produces
 * invalid hybrid strings like "i[k]" that split_args() cannot parse.
 *
 * This string is passed to add_udo_definition() and ultimately to split_args()
 * which expects EXTERNAL format input.
 */
char* get_in_types_from_tree(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable) {
  int32_t len = tree_arg_list_count(tree);

  if (len == 0 || (len == 1 && !strcmp(tree->value->lexeme, "0"))) {
    return csoundStrdup(csound, "0");
  }

  // Use the same logic as get_out_types_from_tree to ensure consistent formatting
  char* argTypes = csound->Malloc(csound, len * 256 * sizeof(char));
  int32_t i;
  int32_t argsLen = 0;
  i = 0;

  TREE* current = tree;

  while (current != NULL) {
    // Use get_arg_type2 to extract the actual type from the tree node
    char* argType = get_arg_type2(csound, current, typeTable);
    if (argType == NULL) {
      csound->Die(csound, "Could not parse type for argument");
    }

    int32_t argLen = (int32_t) strlen(argType);
    int32_t offset = i * 256;

    // Check if this is in internal array format (starts with '[')
    // Convert to external format for proper concatenation
    if (argType[0] == '[') {
      char* converted = convert_internal_to_external(csound, argType);
      int32_t convLen = (int32_t) strlen(converted);
      strcpy(&argTypes[offset], converted);
      argsLen += convLen;
      csound->Free(csound, argType);
      csound->Free(csound, converted);
    } else if (argType[0] == ':' && argType[argLen - 1] == ';') {
      // Already in UDT format, use as-is
      strcpy(&argTypes[offset], argType);
      argsLen += argLen;
      csound->Free(csound, argType);
    } else if (argLen == 1 || (argLen == 3 && argType[1] == '[' && argType[2] == ']')) {
      // Built-in types (single char like 'i' or array like 'k[]')
      strcpy(&argTypes[offset], argType);
      argsLen += argLen;
      csound->Free(csound, argType);
    } else {
      // User-defined types (length > 1 and not built-in array) - convert to :TypeName; format
      // Check if it's an array type
      if (argLen > 2 && argType[argLen-2] == '[' && argType[argLen-1] == ']') {
        // User-defined array type like "MyType[]" -> ":MyType;[]"
        int32_t baseLen = argLen - 2; // Length without []
        argTypes[offset] = ':';
        memcpy(argTypes + offset + 1, argType, baseLen);
        argTypes[offset + baseLen + 1] = ';';
        argTypes[offset + baseLen + 2] = '[';
        argTypes[offset + baseLen + 3] = ']';
        argTypes[offset + baseLen + 4] = '\0';
        argsLen += baseLen + 4;
      } else {
        // User-defined non-array type like "MyType" -> ":MyType;"
        argTypes[offset] = ':';
        memcpy(argTypes + offset + 1, argType, argLen);
        argTypes[offset + argLen + 1] = ';';
        argTypes[offset + argLen + 2] = '\0';
        argsLen += argLen + 2;
      }
      csound->Free(csound, argType);
    }

    current = current->next;
    i += 1;
  }

  char* argString = csound->Malloc(csound, (argsLen + 1) * sizeof(char));
  char* temp = argString;

  for (i = 0; i < len; i++) {
    int32_t size = (int32_t) strlen(&argTypes[i * 256]);
    memcpy(temp, &argTypes[i * 256], size);
    temp += size;
  }

  argString[argsLen] = '\0';
  csound->Free(csound, argTypes);
  return argString;
}

/**
 * Builds a concatenated output type string from the parse tree for new-style UDOs.
 *
 * Array Type Format Contract:
 * ---------------------------
 * OUTPUT: Returns a string in EXTERNAL format for use in OENTRY.outypes
 *         Examples: "k[]", ":MyType;[]", "aS"
 *         - Primitive arrays: "k[]", "a[][]"
 *         - UDT arrays: ":TypeName;[]"
 *
 * This string is passed to add_udo_definition() and ultimately to split_args()
 * which expects EXTERNAL format input.
 */
char* get_out_types_from_tree(CSOUND* csound, TREE* tree) {

  int32_t len = tree_arg_list_count(tree);
  char* argTypes = csound->Malloc(csound, len * 256 * sizeof(char));
  int32_t i;

  if (len == 0 || (len == 1 && !strcmp(tree->value->lexeme, "0"))) {
    return csoundStrdup(csound, "0");
  }

  int32_t argsLen = 0;
  i = 0;

  TREE* current = tree;

  while (current != NULL) {
    char* argType = current->value->lexeme;
    int32_t len = (int32_t) strlen(argType);
    int32_t offset = i * 256;
    argsLen += len;

    // Check if this is an array type (has brackets in next node)
    int32_t isArray = (current->right != NULL && *current->right->value->lexeme == '[');

    if (len == 1) {
      // Built-in single-character types (i, k, a, S, etc.)
      strcpy(&argTypes[offset], argType);
      if (isArray) {
        argTypes[offset + len] = '[';
        argTypes[offset + len + 1] = ']';
        argTypes[offset + len + 2] = '\0';
        argsLen += 2;
      }
    } else {
      // User-defined types (length > 1) - use :TypeName; format
      argTypes[offset] = ':';
      memcpy(argTypes + offset + 1, argType, len);
      argTypes[offset + len + 1] = ';';
      if (isArray) {
        // For user-defined array types, use :TypeName;[] format
        argTypes[offset + len + 2] = '[';
        argTypes[offset + len + 3] = ']';
        argTypes[offset + len + 4] = '\0';
        argsLen += 4;
      } else {
        argTypes[offset + len + 2] = '\0';
        argsLen += 2;
      }
    }

    current = current->next;
    i += 1;
  }

  char* argString = csound->Malloc(csound, (argsLen + 1) * sizeof(char));
  char* curLoc = argString;

  for (i = 0; i < len; i++) {
    unsigned long argLen = strlen(&argTypes[i * 256]);
    memcpy(curLoc, &argTypes[i * 256], argLen);
    curLoc += argLen;
  }

  argString[argsLen] = '\0';
  csound->Free(csound, argTypes);
  return argString;
}


OENTRY* find_opcode_new(CSOUND* csound, char* opname,
                        char* outArgsFound, char* inArgsFound) {
  OENTRIES* opcodes = find_opcode2(csound, opname);
  if (opcodes->count == 0) {
    return NULL;
  }
  OENTRY* retVal = resolve_opcode(csound, opcodes, outArgsFound, inArgsFound);
  csound->Free(csound, opcodes);
  return retVal;
}

OENTRY* find_opcode_exact(CSOUND* csound, char* opname,
                          char* outArgsFound, char* inArgsFound) {
  OENTRIES* opcodes = find_opcode2(csound, opname);
  if (opcodes == NULL || opcodes->count == 0) {
    return NULL;
  }
  OENTRY* retVal = resolve_opcode_exact(csound, opcodes,
                                        outArgsFound, inArgsFound);
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
                  Str("Variable type for %s could not be determined, line %d"),
                  varName, tree->line);
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
        // Handle T_ARRAY - for complex expressions, delegate to recursive check
        if (current->left && current->left->value && current->left->value->lexeme) {
          varName = current->left->value->lexeme;
          // search for the variable in all variable pools
          var = find_var_from_pools(csound, varName, varName, typeTable);
          if (UNLIKELY(var == NULL)) {
              synterr(csound,
                      Str("ArgCheck: variable '%s' used before defined at line %d\n"),
                      varName, current->left->line);
              do_baktrace(csound, current->left->locn);
              return 0;
          }
        } else {
          // Complex expression (like struct member access), recursively check
          if (!(check_args_exist(csound, current->left, typeTable) &&
                check_args_exist(csound, current->right, typeTable))) {
            return 0;
          }
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
CS_VAR_POOL *find_global_annotation(char *varName, TYPE_TABLE* typeTable) {
  CS_VAR_POOL* pool = typeTable->localPool;
  // find global annotation
  if(strchr(varName, '@') != NULL) {
    char* th;
    char* baseType = strtok_r(varName, "@", &th);
    char* global = strtok_r(NULL, "@", &th);
    if(!strcmp(global, "global")) {
      pool = typeTable->globalPool;
      varName = baseType;
    }
  }
  return pool;
}

static CS_VAR_POOL *get_var_pool(CSOUND *csound, TYPE_TABLE* typeTable,
				 const char *varBaseName) {
     // we first check for local variables
    CS_VARIABLE *var = csoundFindVariableWithName(csound, typeTable->localPool,
                                     varBaseName);
    if(var) return typeTable->localPool;
    // then check for global variables in engine
    var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                     varBaseName);
    if(var) return csound->engineState.varPool;
    // and finally newly defined global vars
    var = csoundFindVariableWithName(csound, typeTable->globalPool,
                                       varBaseName);
    if(var) return typeTable->globalPool;

    return NULL;
}

// on new-type UDOS type-annotations can
// be used for optional types
// this checks and converts it to i or k type names
char *check_optional_type(CSOUND *csound, char *name) {
    if(is_in_optional_arg(name)) {
      char *t = (char*)OPTIONAL_IN_TYPES[0];
      char *o, str[2] =  {0};
      for (int i = 0; t != NULL; i += 2) {
        o = (char*)OPTIONAL_IN_TYPES[i + 1];
        if (strcmp(t, name) == 0) {
          str[0] = *o;
          return csoundStrdup(csound, str);
        }
        t = (char*)OPTIONAL_IN_TYPES[i + 2];
      }
    }
    return csoundStrdup(csound, name);
}

/* This function creates a new variable for a rhs argument
   if the variable is not found in any of the pools
   If the variable is found, a consistency check is made
   to make sure the argument type matches the existing variable
*/
void add_arg(CSOUND* csound, char* varName, char* annotation,
             TYPE_TABLE* typeTable, TREE* tree) {

  const CS_TYPE* type = NULL;
  CS_VARIABLE* var;
  char *t = csoundStrdup(csound, varName);
  char *lvarName = csoundStrdup(csound, varName); // local copy
  CS_VAR_POOL* pool = typeTable->localPool;
  char argLetter[2] = {0};
  ARRAY_VAR_INIT varInit;
  void* typeArg = NULL;
  // remove any global annotation
  find_global_annotation(t, typeTable);
  // search on  all pools
  var = find_var_from_pools(csound, t, t, typeTable);
  csound->Free(csound, t);
  // VL: since arrays are now also created here now, we
  // need to apply similar checks to add_array_arg()
  if (var == NULL ||
      (var && var->varType == &CS_VAR_TYPE_OPCODEREF)) {
    if (annotation != NULL) {
      // check for global annotation in explicit-type rhs vars
      lvarName = csoundStrdup(csound, varName);
      pool = find_global_annotation(lvarName, typeTable);
      if(pool == csound->engineState.varPool
         || pool == typeTable->globalPool) {
        // remove var from pool
        if(var)
          cs_hash_table_remove(csound,
                               get_var_pool(csound,typeTable,
                                            var->varName)->table,
                               var->varName);
      }
      // check to see if annotation is optional type
      char *nm = check_optional_type(csound, annotation);

      // Check if annotation is an array type (contains [])
      char* firstBracket = strchr(nm, '[');
      if (firstBracket != NULL) {
        // Parse array annotation like "k[]", "S[][]", ":MyType;[]" etc.
        int32_t dimensions = 0;
        char* p = firstBracket;

        // Count dimensions by counting "[]" pairs
        while (*p == '[') {
          if (*(p+1) == ']') {
            dimensions++;
            p += 2;
          } else {
            break;
          }
        }

        // Extract element type (everything before the first '[')
        size_t baseLen = firstBracket - nm;
        char* baseType = csound->Malloc(csound, baseLen + 1);
        strncpy(baseType, nm, baseLen);
        baseType[baseLen] = '\0';

        const CS_TYPE* elemType = csoundGetTypeWithVarTypeName(csound->typePool, baseType);

        if (elemType == NULL) {
          csound->ErrorMsg(csound, "Unknown element type for array: %s\n", baseType);
          csound->Free(csound, baseType);
          csound->Free(csound, nm);
          csound->Free(csound, lvarName);
          return;
        }
        csound->Free(csound, baseType);

        // Get the array type (use "[" which has create_array)
        type = csoundGetTypeWithVarTypeName(csound->typePool, "[");

        // Set up ARRAY_VAR_INIT with dimensions and element type
        varInit.dimensions = dimensions;
        varInit.type = elemType;
        typeArg = &varInit;
      } else {
        // Not an array, use the type directly
        type = csoundGetTypeWithVarTypeName(csound->typePool,nm);
        typeArg = (void *) type;
      }
      csound->Free(csound, nm);
    } else {
      // check for @global in implicit-type rhs vars
      // and if found, strip it and print warning
      if(find_global_annotation(varName, typeTable) == typeTable->globalPool)
        csound->Warning(csound, "%s: @global annotation ignored", varName);

      t = lvarName;

      int32_t isSynthetic = 0;
      if (*t == '#') {
        isSynthetic = 1;
        t++;
      }
      if (*t == 'g') pool = typeTable->globalPool;
      if (*t == 'g') t++;

      if (*t == '[') {
        int32_t dimensions = 1;
        const CS_TYPE* varType;
        char* b = t + 1;

        while(*b == '[') {
          b++;
          dimensions++;
        }
        argLetter[0] = *b;

        varType = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);

        varInit.dimensions = dimensions;
        varInit.type =  varType;
        typeArg = &varInit;
      } else if (isSynthetic) {
        char *syntheticType = csoundStrdup(csound, t);
        char *end = syntheticType + strlen(syntheticType);
        while (end > syntheticType && isdigit((unsigned char) end[-1])) {
          *--end = '\0';
        }
        if (strlen(syntheticType) > 1) {
          type = csoundGetTypeWithVarTypeName(csound->typePool, syntheticType);
        }
        csound->Free(csound, syntheticType);
      }

      if (type == NULL) {
        argLetter[0] = *t;
        type = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);
      }
    }
    var = csoundCreateVariable(csound, csound->typePool,
				 type, lvarName, typeArg);
    csoundAddVariable(csound, pool, var);
  } else {
    // for explicit non-array types, we'll allow variable redeclaration
    if (annotation != NULL) {
      pool = find_global_annotation(lvarName, typeTable);
      CS_VAR_POOL *var_pool = get_var_pool(csound, typeTable, lvarName);
      // check for optional type
      t = check_optional_type(csound, annotation);

      // Check if t is an array type annotation (contains [])
      char* firstBracket = strchr(t, '[');
      if (firstBracket != NULL) {
        // Array type - use "[" as the type
        type = csoundGetTypeWithVarTypeName(csound->typePool, "[");
      } else {
        // Regular type
        type = csoundGetTypeWithVarTypeName(csound->typePool, t);
      }
      if(type && type != var->varType){
	 // remove variable if it belongs to the same pool (local/global)
	if(pool == var_pool ||
	   (var_pool == csound->engineState.varPool
	    && pool == typeTable->globalPool)) {
	  if(tree)
	   csound->Warning(csound, "Replacing previous definition %s:%s by %s:%s, line %d",
                              var->varName, var->varType->varTypeName,
			  lvarName, type->varTypeName, tree->line);
	  cs_hash_table_remove(csound, var_pool->table, var->varName);
	}
	else if(pool == typeTable->globalPool)
	  if(tree) // synterr should not happen tree is NULL, as arg is synthetic
	  synterr(csound, "global variable %s:%s cannot shadow local variable %s:%s, line %d",
		  lvarName, type->varTypeName, var->varName, var->varType->varTypeName, tree->line);
      } else {
	// do nothing if it's the same type & pool
	if(pool == var_pool) goto end;
	// if it's a global var was requested, print warning, do nothing
        if(pool == typeTable->globalPool) {
	  if(tree)
	    csound->Warning(csound, "@global annotation ignored for variable %s, line %d",
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
      if(var == NULL)
	var = csoundFindVariableWithName(csound, csound->engineState.varPool,
	                                  varName);
      // we are only concerned with opcoderef and instr vars
      // added by the compiler as global read-only, internally
      if(var && (var->varType == &CS_VAR_TYPE_OPCODEREF ||
		 var->varType == &CS_VAR_TYPE_INSTR)) {
        if(csoundFindVariableWithName(csound, typeTable->localPool, varName)
	      == NULL){
	   argLetter[0] = *varName;
           if(argLetter[0] == 'g') argLetter[0] = varName[1];
           type =
	   csoundGetTypeWithVarTypeName(csound->typePool, argLetter);
           var = csoundCreateVariable(csound, csound->typePool,
   	              type, varName, typeArg);
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
void add_array_arg(CSOUND* csound, char* varName, char* annotation,
                   int32_t dimensions, TYPE_TABLE* typeTable) {

  CS_VARIABLE* var;
  char *t = csoundStrdup(csound, varName);
  char *lvarName = csoundStrdup(csound, varName); // local copy
  CS_VAR_POOL* pool = typeTable->localPool;
  char argLetter[2];
  ARRAY_VAR_INIT varInit;
  void* typeArg = NULL;
  const CS_TYPE* varType;
  // remove any global annotation
  find_global_annotation(t, typeTable);
  // search on  all pools
  var = find_var_from_pools(csound, t, t, typeTable);
  csound->Free(csound, t);
  if (var == NULL ||
      // treat the case of global opcode ref vars
      (var && var->varType == &CS_VAR_TYPE_OPCODEREF)) {
    if (annotation != NULL) {
      // check for global annotation
      pool = find_global_annotation(lvarName, typeTable);
      if(pool == csound->engineState.varPool
         || pool == typeTable->globalPool) {
        // remove var from pool
        if(var)
          cs_hash_table_remove(csound, get_var_pool(csound,
                                                    typeTable,
                                                    var->varName)->table,
                               var->varName);
      }
      varType = csoundGetTypeWithVarTypeName(csound->typePool, annotation);
    } else {
      t = lvarName;
      argLetter[1] = 0;

      if (*t == '#') t++;
      if (*t == 'g') {
           pool = typeTable->globalPool;
       // remove var from pool
       if(var)
         cs_hash_table_remove(csound,
                              get_var_pool(csound,
                                           typeTable,
                                           var->varName)->table,
                              var->varName);
      }
      if (*t == 'g') t++;

      argLetter[0] = *t;

      varType =
        csoundGetTypeWithVarTypeName(csound->typePool, argLetter);
    }


    varInit.dimensions = dimensions;
    varInit.type = varType;
    typeArg = &varInit;
    var = csoundCreateVariable(csound, csound->typePool,
				 &CS_VAR_TYPE_ARRAY,
				 lvarName, typeArg);
    csoundAddVariable(csound, pool, var);
  } else {
    //TODO - implement reference count increment
     if (annotation != NULL) {
       // check if a variable is declared with same name
       // and different type array subtype
       varType = csoundGetTypeWithVarTypeName(csound->typePool, annotation);
       if(varType != var->subType)
         synterr(csound, "%s:%s[] -- type mismatch for existing "
                          "array variable %s:%s%s",
                 varName, varType->varTypeName, varName,
                 var->subType ? var->subType->varTypeName :
                 var->varType->varTypeName,
                 var->subType ? "[]" : "");
    }
  }
  csound->Free(csound, lvarName);
}

/* return 1 on succcess, 0 on failure */
int32_t add_args(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable)
{
  TREE* current;
  char* varName;
  CS_VARIABLE *arrvar;

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
      if (is_reserved(varName)) {
        // skip reserved vars, these are handled elsewhere
        break;
      }
      add_arg(csound, varName, current->value->optype, typeTable, current);

      break;

    case T_ARRAY:

      if (array_target_missing_lexeme(current)) {
        char *arrayElementType = get_arg_type2(csound, current, typeTable);
        if (arrayElementType == NULL) {
          csound->LongJmp(csound, 1);
        }
        csound->Free(csound, arrayElementType);
        break;
      }

      varName = current->left->value->lexeme;
      // check if the array variable exists, it needs to be declared
      arrvar = find_var_from_pools(csound, varName, varName, typeTable);
      if(arrvar == NULL) {
        synterr(csound,"cannot find array variable %s, line %d",
                varName, current->line);
        csound->LongJmp(csound, 1);
      }
      // & needs to be an array or asigs
      if(arrvar->varType != &CS_VAR_TYPE_ARRAY &&
         arrvar->varType != &CS_VAR_TYPE_A) {
        synterr(csound,"variable %s is not an array, line %d",
                varName, current->line);
        csound->LongJmp(csound, 1);
      }

      add_arg(csound, varName, current->left->value->optype,
              typeTable, current);
      break;

    default:
      break;
    }

    current = current->next;
  }

  return 1;
}

TREE* get_initial_unary_operator(TREE* tree) {
  if (tree == NULL) return NULL;

  TREE* current = tree;
  while (current->left != NULL) {
    current = current->left;
  }
  if (current->type == S_UMINUS || current->type == S_UPLUS) {
    return current;
  }
  return NULL;
}

TREE* get_left_parent(TREE* root, TREE* node) {
  TREE* current = root;
  while (current != NULL) {
    if (current->left == node) {
      return current;
    }
    current = current->left;
  }
  return NULL;
}

TREE* convert_unary_op_to_binary(CSOUND* csound, TREE* new_left, TREE* unary_op) {
  TREE* retVal = NULL;
  new_left->type = T_IDENT;

  if (unary_op->type == S_UMINUS) {
    retVal = make_node(csound, unary_op->line, unary_op->locn, '-', new_left, unary_op->right);
  } else if (unary_op->type == S_UPLUS) {
    retVal = make_node(csound, unary_op->line, unary_op->locn, '+', new_left, unary_op->right);
    //unary_op->markup = NULL;
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
 * because variables can (now) have names that shadow opcode names. Lookup needs * to give priority to an identifier being a variable over being an opcode.
 * This maintains future proofing so that if an opcode is later introduced
 * with the same name as a variable in an older project, the older project
 * will continue to work.
 *
 * For further reference, please see the rule for statement and opcall in
 * Engine/csound_orc.y.
 */
static char *get_assignment_lhs_type_hint(CSOUND *csound, TREE *left)
{
  if (left == NULL || left->value == NULL) {
    return NULL;
  }

  if (left->value->optype != NULL) {
    const char *type = convert_array_type_string(csound, left->value->optype);
    char *ret = csoundStrdup(csound, type);
    if (type != left->value->optype) {
      csound->Free(csound, (void*)type);
    }
    return ret;
  }

  char *name = left->value->lexeme;
  if (name == NULL) {
    return NULL;
  }
  if (*name == '#') {
    name++;
  }
  if (*name == 'g' && name[1] != '\0') {
    name++;
  }
  if (strchr("aikSKBbf", *name) != NULL) {
    char type[2] = {*name, '\0'};
    return csoundStrdup(csound, type);
  }
  return NULL;
}

TREE* convert_statement_to_opcall(CSOUND* csound, TREE* root,
                                  TYPE_TABLE* typeTable) {
  int32_t leftCount, rightCount;

  if (root->type == T_ASSIGNMENT) {
    /* Rewrite tree if line is "a1, a2 = func(arg, arg1)"
       to "a1, a2 func arg, arg1" */
    TREE *right = root->right;
    if (right->type == T_FUNCTION &&
        right->left == NULL &&
        right->next == NULL) {
      char *outType = NULL;
      TREE *arg = right->right;
      int32_t hasExpressionArg = 0;

      while (arg != NULL && !hasExpressionArg) {
        hasExpressionArg = is_expression_node(arg) ||
                           is_boolean_expression_node(arg);
        arg = arg->next;
      }

      if (hasExpressionArg && tree_arg_list_count(root->left) == 1) {
        char *leftType = get_assignment_lhs_type_hint(csound, root->left);

        if (leftType != NULL && strlen(leftType) == 1 &&
            (outType = get_arg_type2(csound, right, typeTable)) != NULL) {
          if (!strcmp(leftType, outType)) {
            csound->Free(csound, leftType);
            csound->Free(csound, outType);
            return root;
          }
          csound->Free(csound, outType);
        }
        if (leftType != NULL) {
          csound->Free(csound, leftType);
        }
      }

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
  if(root->type == T_FUNCTION) {
    root->type = T_OPCALL;
    return root;
  }

  if (root->type == GOTO_TOKEN ||
      root->type == KGOTO_TOKEN ||
      root->type == IGOTO_TOKEN) {
    // i.e. a = func(a + b)
    return root;
  }

  if (root->type == S_ADDIN ||
      root->type == S_SUBIN ||
      root->type == S_MULIN ||
      root->type == S_DIVIN) return root;

  if (root->type != T_OPCALL) {
    synterr(csound,
            Str("Internal Error: convert_statement_to_opcall "
                "received a non T_OPCALL TREE\n"));
    return NULL;
  }

  if (root->value != NULL) {
    /* Already processed T_OPCALL, return as-is */
    return root;
  }

  if (root->left == NULL) {
    synterr(csound,
            Str("Internal Error: convert_statement_to_opcall "
                "received an empty OPCALL\n"));
    return NULL;
  }

  if (root->left->type == T_OPCALL && root->right == NULL) {

    TREE* top = root->left;
    TREE* unary_op = get_initial_unary_operator(top->right);

    if (top->left->next == NULL && unary_op != NULL) {
      TREE* newTop;

      /* i.e. ksubst init -1 */
      /* TODO - this should check if it's a var first */
      CS_VARIABLE *var = find_var_from_pools(csound, top->value->lexeme,
                                          top->value->lexeme, typeTable);

      /* but if it's an opcoderef, then it can't be in an expression */
      if(var && var->varType == &CS_VAR_TYPE_OPCODEREF)
        var = NULL;

      if (find_opcode(csound, top->value->lexeme) != NULL && var == NULL
          ) {
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
        TREE* unary_op_parent = get_left_parent(top->right, unary_op);
        newTop->right = top->right;
        unary_op_parent->left = convert_unary_op_to_binary(csound, top,
                                                           unary_op);
      }
      top->right = top->left = top->next = NULL;
      return newTop;
    }

    /* i.e. asig oscil 0.25, 440 */
    top->next = root->next;
    return top;

  } else if(root->right == NULL) {
    /* this branch catches this part of opcall rule:
       out_arg_list '(' ')' NEWLINE */

    if (tree_arg_list_count(root->left) != 1) {
      synterr(csound,
              Str("Internal Error: convert_statement_to_opcall "
                  "received invalid OPCALL\n"));
    }
    root->left->next = root->next;
    root->left->type = T_OPCALL;
    return root->left;
  }

  if (root->right == NULL) {
    synterr(csound,
            Str("Internal Error: convert_statement_to_opcall "
                "received invalid OPCALL\n"));
    return NULL;
  }

  /* Now need to disambiguate the rule : out_arg_list expr_list NEWLINE */
  leftCount = tree_arg_list_count(root->left);
  rightCount = tree_arg_list_count(root->right);

  if (leftCount > 1 && rightCount > 1) {
    synterr(csound,
            Str("Internal Error: convert_statement_to_opcall "
                "received invalid OPCALL\n"));
    return NULL;
  }

  if (leftCount == 1 && rightCount == 1) {
    TREE* newTop;
    if(root->right->type == T_IDENT &&
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
    TREE* newTop = root->left;
    newTop->type = T_OPCALL;
    newTop->next = root->next;
    newTop->right = root->right;
    return newTop;
  } else {
    TREE* newTop = root->right;
    newTop->type = T_OPCALL;
    newTop->next = root->next;
    newTop->left = root->left;
    return newTop;
  }

  return NULL;
}

char *strip_extension(CSOUND *csound, const char *s) {
  char *s1 = csoundStrdup(csound, s);
  char *dot = strchr(s1, '.');
  if(dot != NULL)
      *dot = '\0';
  return s1;
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

  if (!check_args_exist(csound, root->right, typeTable)) {
    return 0;
  }

  add_args(csound, root->left, typeTable);

  opcodeName = root->value->lexeme;
  leftArgString = get_arg_string_from_tree(csound, left, typeTable);
  rightArgString = get_arg_string_from_tree(csound, right, typeTable);



  OENTRIES* entries = find_opcode2(csound, opcodeName);
  if (UNLIKELY(entries == NULL || entries->count == 0)) {
    synterr(csound, Str("unable to find opcode with name: %s, line %d,"
                         " columns %d-%d"),
            root->value->lexeme, root->line,
            root->value->first_column, root->value->last_column);
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
    oentry = resolve_opcode(csound, entries,
                               root->value->optype, rightArgString);
    else if(leftArgString &&
       strcmp(leftArgString, root->value->optype)){
      csound->Warning(csound, " output type(s) %s\n"
                      "\t not matching annotation %s\n"
                      "\t ignoring annotation for opcode %s, line %d",
                      leftArgString, root->value->optype,
		      opcodeName, root->line);
        oentry = resolve_opcode(csound, entries,
                            leftArgString, rightArgString);
      } else
      oentry = resolve_opcode(csound, entries,
                               root->value->optype, rightArgString);
  }

  /* check for deprecation */
  if(oentry && oentry->deprecated) {
    if(oentry->deprecated == 1) {
      if(csound->oparms->error_deprecated) {
        synterr(csound, "opcode %s is deprecated, line %d,"
                " columns %d-%d",
                oentry->opname,
                root->line, root->value->first_column,
                root->value->last_column);
        csoundMessage(csound, Str(" %s %s %s\n"),
                        leftArgString ? leftArgString : "",
                        oentry->opname, rightArgString ? rightArgString : "");
        return 0;
       }
      else csoundWarning(csound, "opcode %s is deprecated, line %d",
                         oentry->opname, root->line);
    }
    else if(oentry->deprecated == 2) {
      if(csound->oparms->error_deprecated) {
        synterr(csound, "opcode %s has been renamed"
                " (uppercase to lowercase / underscores removed), line %d"
                " columns %d-%d",
                oentry->opname, root->line,root->value->first_column,
                root->value->last_column);
        csoundMessage(csound, Str(" %s %s %s\n"),
                        leftArgString ? leftArgString : "",
                        oentry->opname, rightArgString ? rightArgString : "");
        return 0;
       }
      else csoundWarning(csound, "opcode %s has been renamed"
                         " (uppercase to lowercase / underscores removed), line %d"
                         " columns %d-%d",
                         oentry->opname, root->line,
                         root->value->first_column,
                         root->value->last_column);
    }
   }



  if (UNLIKELY(oentry == NULL)) {
    int32_t i;
    char *name = strip_extension(csound, opcodeName);
    synterr(csound, Str("Unable to find opcode entry for \'%s\'\n"
                        "  with matching argument types, line %d"
                        " columns %d-%d"), name, root->line,
                          root->value->first_column,
                         root->value->last_column);
    csound->Free(csound, name);
    name = strip_extension(csound, root->value->lexeme);
    csoundMessage(csound, Str("Found:\n  %s %s %s\n"),
                  leftArgString ? leftArgString : "", name,
                  rightArgString ? rightArgString : "");
    csound->Free(csound, name);
    csoundMessage(csound, Str("\nCandidates:\n"));

    for (i = 0; i < entries->count; i++) {
      OENTRY *entry = entries->entries[i];
      name = strip_extension(csound, entry->opname);
      csoundMessage(csound, "  %s %s %s\n", entry->outypes, name,
                    entry->intypes);
      csound->Free(csound, name);
    }

    csoundMessage(csound, Str("\nLine: %d "
                              " columns %d-%d\n"),
                  root->line,
                  root->value->first_column,
                  root->value->last_column);
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
      head = cs_cons(csound, csoundStrdup(csound, labelText), head);
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

int32_t is_reserved(char* varname) {
  return (strcmp("sr", varname) == 0 ||
          strcmp("kr", varname) == 0 ||
          strcmp("ksmps", varname) == 0 ||
          strcmp("0dbfs", varname) == 0 ||
          strcmp("nchnls", varname) == 0 ||
          strcmp("nchnls_i", varname) == 0) ||
          strcmp("A4", varname) == 0;
}

int32_t verify_if_statement(CSOUND* csound, TREE* root, TYPE_TABLE* typeTable) {

  char* outArg;
  TREE* right = root->right;

  if (right->type == IGOTO_TOKEN ||
      right->type == KGOTO_TOKEN ||
      right->type == GOTO_TOKEN) {

    if (!check_args_exist(csound, root->left, typeTable)) {
      return 0;
    }

    outArg = get_arg_type2(csound, root->left, typeTable);

    return (outArg != NULL && (*outArg == 'b' || *outArg == 'B'));

  }
  else if (right->type == THEN_TOKEN ||
           right->type == ITHEN_TOKEN ||
           right->type == KTHEN_TOKEN) {
    TREE* current = root;

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

int32_t initStructVar(CSOUND* csound, void* p) {
  INIT_STRUCT_VAR* init = (INIT_STRUCT_VAR*)p;
  CS_STRUCT_VAR* structVar = (CS_STRUCT_VAR*)init->out;
  CS_TYPE* type = csoundGetTypeForArg(init->out);
  int32_t len = cs_cons_length(type->members);
  int32_t i;
  if(csoundGetDebug(csound) & DEBUG_SEMANTICS) {
     csound->Message(csound, "Initializing Struct...\n");
     csound->Message(csound, "Struct Type: %s\n", type->varTypeName);
  }
  for (i = 0; i < len; i++) {
    CS_VAR_MEM* mem = structVar->members[i];
    mem->varType->copyValue(csound, mem->varType, &mem->value,
                            init->inArgs[i], init->h.insdshead);
  }

  return CSOUND_SUCCESS;
}

void initializeStructVar(CSOUND* csound, CS_VARIABLE* var, MYFLT* mem) {
  CS_STRUCT_VAR* structVar = (CS_STRUCT_VAR*)mem;
  const CS_TYPE* type = var->varType;
  CONS_CELL* members = type->members;

  int32_t len = cs_cons_length(members);
  int32_t i;

  structVar->members = csound->Calloc(csound, len * sizeof(CS_VAR_MEM*));
  structVar->memberCount = len;  // Set the member count
  structVar->ownsMembers = 1;    // This struct owns its members
  if(csoundGetDebug(csound) & DEBUG_SEMANTICS) {
      csound->Message(csound, "Initializing Struct...\n");
      csound->Message(csound, "Struct Type: %s\n", type->varTypeName);
  }
  for (i = 0; i < len; i++) {
    CS_VARIABLE* var = members->value;
    size_t size = (sizeof(CS_VAR_MEM) - sizeof(MYFLT)) + var->memBlockSize;
    CS_VAR_MEM* mem = csound->Calloc(csound, size);
    if (var->initializeVariableMemory != NULL) {
      var->initializeVariableMemory(csound, var, &mem->value);
    }
    mem->varType = var->varType;
    structVar->members[i] = mem;

    members = members->next;
  }
}

CS_VARIABLE* createStructVar(void* cs, void* p, INSDS *ctx) {
  CSOUND* csound = (CSOUND*)cs;
  const CS_TYPE* type = (const CS_TYPE*)p;

  if (type == NULL) {
    csound->Message(csound, "ERROR: no type given for struct creation\n");
    return NULL;
  }

  CS_VARIABLE* var = csound->Calloc(csound, sizeof (CS_VARIABLE));
  IGN(p);
  var->memBlockSize = sizeof(CS_STRUCT_VAR);
  var->initializeVariableMemory = initializeStructVar;
  var->varType = type;
  var->ctx = ctx;

  //FIXME - implement
  return var;
}

void copyStructVar(CSOUND* csound, const CS_TYPE* structType, void* dest, const
                   void* src, INSDS *p) {
  CS_STRUCT_VAR* varDest = (CS_STRUCT_VAR*)dest;
  CS_STRUCT_VAR* varSrc = (CS_STRUCT_VAR*)src;
  int32_t i, count;

  // Don't copy to itself
  if (dest == src) {
    return;
  }

  if (varDest->members == NULL || varSrc->members == NULL) {
    csound->Message(csound, "struct not initialised - cannot copy\n");
    return;  // Can't copy if members aren't initialized
  }

  count = cs_cons_length(structType->members);
  for (i = 0; i < count; i++) {
    CS_VAR_MEM* d = varDest->members[i];
    CS_VAR_MEM* s = varSrc->members[i];
    if (d != NULL && s != NULL) {
      // Check if d and s are the same (aliased)
      if (d == s) {
        // Already aliased, nothing to copy
        continue;
      }
      d->varType->copyValue(csound, d->varType, &d->value, &s->value, p);
    }
  }
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

int32_t add_struct_definition(CSOUND* csound, TREE* structDefTree) {
  TREE* current = structDefTree->right;
  int32_t index = 0;
  char temp[256];
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
  type = (CS_TYPE*)csoundGetTypeWithVarTypeName(csound->typePool, internalName);

  if (type != NULL) {
    // Update existing placeholder with member information
    csound->Free(csound, internalName);
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
    type->userDefinedType = 1;
  }

  // Clear existing members if updating placeholder
  if (type->members != NULL) {
    // Free existing member list (they're just empty placeholders)
    type->members = NULL;
  }

  // FIXME: Values are appended in reverse order of definition
  while (current != NULL) {
    char* memberName = current->value->lexeme;
    char* typedIdentArg = current->value->optype;

    if (typedIdentArg == NULL) {
      typedIdentArg = cs_strndup(csound, memberName, 1);
    }

    memberName = csoundStrdup(csound, memberName);

    // Check if this is an array type (ends with [])
    size_t typeLen = strlen(typedIdentArg);
    int32_t isArray = (typeLen >= 2 &&
                       typedIdentArg[typeLen-2] == '[' &&
                       typedIdentArg[typeLen-1] == ']');

    const CS_TYPE* memberType;
    CS_VARIABLE* var;

    if (isArray) {
      // Extract base type (remove [] from the end)
      char* baseType = cs_strndup(csound, typedIdentArg, typeLen - 2);
      const CS_TYPE* baseCSType = csoundGetTypeWithVarTypeName(csound->typePool, baseType);

      // Get the array type (use "[" which has create_array as its createVariable function)
      const CS_TYPE* arrayType = csoundGetTypeWithVarTypeName(csound->typePool, "[");

      // Create array variable with dimensions set
      ARRAY_VAR_INIT varInit;
      varInit.dimensions = 1;
      varInit.type = baseCSType;

      // Use array type - csoundCreateVariable will set var->varType to arrayType
      // and create_array will set var->subType to baseCSType
      var = csoundCreateVariable(csound, csound->typePool, arrayType, memberName, &varInit);
      csound->Free(csound, baseType);
    } else {
      memberType = csoundGetTypeWithVarTypeName(csound->typePool, typedIdentArg);
      var = memberType->createVariable(csound, type, NULL);
      var->varType = memberType;  // Set varType for non-array types
    }

    var->varName = csoundStrdup(csound, memberName);
    CONS_CELL* member = csound->Calloc(csound, sizeof(CONS_CELL));
   member->value = var;
    type->members = cs_cons_append(type->members, member);
    current = current->next;
  }

  // Only add to type pool if it's a new type (not updating placeholder)
  const CS_TYPE* existingType = csoundGetTypeWithVarTypeName(csound->typePool, type->varTypeName);
  if(existingType == NULL) {
    if(!csoundAddVariableType(csound, csound->typePool, type)) {
      return 0;
    }
  }

  OENTRY oentry;
  memset(&oentry, 0, sizeof(OENTRY));
  memset(temp, 0, 256);

  // Extract plain struct name from internal format (:name;) for opcode name
  char* plainName = type->varTypeName + 1;  // Skip leading ':'
  size_t plainNameLen = strlen(plainName) - 1;  // Exclude trailing ';'

  csoundSprintf(temp, "init.");
  strncat(temp, plainName, plainNameLen);
  oentry.opname = csoundStrdup(csound, temp);
  oentry.dsblksiz = sizeof(INIT_STRUCT_VAR);
  oentry.flags = 0;
  oentry.init = initStructVar;
  oentry.perf = NULL;
  oentry.deinit = NULL;
  oentry.useropinfo = NULL;

  /* FIXME - this is not yet implemented */
  memset(temp, 0, 256);
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
  oentry.intypes = csoundStrdup(csound, temp);
  csoundAppendOpcodes(csound, &oentry, 1);
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
        csound->ErrorMsg(csound, "[struct] Phase 2: ERROR processing struct '%s'\n", structName ? structName : "(null)");
        return 0; // Error in Phase 2
      }
    }
    current = current->next;
  }

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

TREE* verify_tree(CSOUND * csound, TREE *root, TYPE_TABLE* typeTable)
{
  TREE *anchor = NULL;
  TREE *current = root;
  TREE *previous = NULL;
  TREE* newRight;
  TREE* transformed;
  TREE* top;
  char *udo_name = NULL;
  CONS_CELL* activeLoopStack = NULL;

  CONS_CELL* parentLabelList = typeTable->labelList;
  typeTable->labelList = get_label_list(csound, root);
  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_SEMANTICS))
          csound->Message(csound, "Verifying AST\n");

  while (current != NULL) {
    switch(current->type) {
    case STRUCT_TOKEN:
      if (csoundGetDebug(csound) & DEBUG_SEMANTICS)
	csound->Message(csound, "Struct definition found\n");
      if(!add_struct_definition(csound, current)) {
        csound->ErrorMsg(csound,
                         "Error: Unable to define new struct type: %s\n",
                         current->left->value->lexeme);
        return NULL;
      }
      break;
    case INSTR_TOKEN:
      csound->inZero = 0;
      if (UNLIKELY(csoundGetDebug(csound) & DEBUG_SEMANTICS))
	csound->Message(csound, "Instrument found\n");
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
      typeTable->localPool = csoundCreateVarPool(csound);
      current->markup = typeTable->localPool;
      top = current->left;
      if (top->left != NULL && top->left->type == UDO_ANS_TOKEN) {
        top->left->markup = csoundStrdup(csound, top->left->value->lexeme);
        top->right->markup = csoundStrdup(csound, top->right->value->lexeme);
        add_udo_definition(csound, false,
                           top->value->lexeme,
                           top->left->value->lexeme,
                           top->right->value->lexeme,
                           0x0000);
        udo_name = top->value->lexeme;
      } else {
        if(UNLIKELY(csoundGetDebug(csound) & DEBUG_SEMANTICS))
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
        top->left->markup = csoundStrdup(csound, outArgString);
        top->right->markup = csoundStrdup(csound, inArgString);
        add_udo_definition(csound, true,
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
      char* outArgStringDecl = get_out_types_from_tree(csound,
                                                       current->left->left);
      char* inArgStringDecl = get_in_types_from_tree(csound,
                                                     current->left->right,
                                                     typeTable);
      add_udo_definition(csound, false, current->value->lexeme,
                         outArgStringDecl, inArgStringDecl, UNDEFINED);
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
    case WHILE_TOKEN: {
      LOOP_JUMP_TARGETS* targets = csound->Calloc(csound, sizeof(LOOP_JUMP_TARGETS));

      if (!verify_until_statement(csound, current, typeTable)) {
        synterr(csound, "loop conditional expression not valid, line %d",
                current->line - 2);
        return NULL;
      }

      current = expand_until_statement(csound, current,
                                       typeTable, current->type==WHILE_TOKEN,
                                       targets);

      if (previous != NULL) {
        previous->next = current;
      }

      activeLoopStack = cs_cons(csound, targets, activeLoopStack);
      continue;
    }

    case SWITCH_TOKEN: {
      char* switchArgType = get_arg_type2(csound, current->left, typeTable);
      current = expand_switch_statement(csound, current, typeTable,
                                        switchArgType);

      if (previous != NULL) {
        previous->next = current;
      }
    }
    continue;

    case FOR_TOKEN: {
      /** for-loop typing:
          1) if loop variable does not exist, it takes the type of the array
          2) if it exists, the loop type follows the variable type when conversion is possible
      */

      char* arrayArgType = get_arg_type2(csound, current->right->left,
                                         typeTable);
      CS_VARIABLE* var = find_var_from_pools(csound,
                                             current->left->value->lexeme,
                                             current->left->value->lexeme,
                                             typeTable);
      if (*arrayArgType != '[') {
        if(current->right->left->value != NULL)
        synterr(csound,Str("line:%d invalid argument in for statement: "
                           "found '%s', which is not an array\n"),
          current->line,current->right->left->value->lexeme);
        else synterr(csound,Str("line:%d expected an array variable in for statement."),
                     current->line);
        return 0;
      }

      char* atype = csoundStrdup(csound, arrayArgType+1); // skip '['
      char* typ;
      atype[strlen(atype)-1] = '\0'; // remove ']'
      typ = remove_type_quoting(csound, atype);
      if(var == NULL) {
       // check for optype
        if(current->left->value->optype != NULL) {
          csoundFree(csound, typ);
          typ = csoundStrdup(csound, current->left->value->optype);
        }
       // now create the arg based on the array type
       add_arg(csound, current->left->value->lexeme, typ,
	       typeTable, current);
       } else {
        // now we need to check that the array matches the
        // var type - automatic conversion possible for i and k
        const CS_TYPE *iType = &CS_VAR_TYPE_I;
        const CS_TYPE *kType = &CS_VAR_TYPE_K;
        const CS_TYPE *avartyp = csoundGetTypeWithVarTypeName(csound->typePool, atype);
        if(var->varType != avartyp) {
	  char *otype = var->varType->varTypeName;
          if((avartyp == iType && var->varType == kType) ||
             (avartyp == kType && var->varType == iType)) {
            if(csoundGetDebug(csound) & DEBUG_SEMANTICS)
              csound->Message(csound, "%s-type loop with %s-type array\n",
                              var->varType->varTypeName, atype);
          }
          else {
	    // otherwise we just shadow the var with a new one
            add_arg(csound, current->left->value->lexeme, typ,
	                        typeTable, current);
	    var = find_var_from_pools(csound, current->left->value->lexeme,
                                              current->left->value->lexeme,
                                              typeTable);
            csound->Warning(csound, "redefining variable %s in loop (type: %s)\n"
			    "\t - now using %s type, line %d",
			    var->varName, otype,
			    var->varType->varTypeName, current->line);
          }
	}
       csoundFree(csound, typ);
       typ = csoundStrdup(csound, var->varType->varTypeName);
      }
      LOOP_JUMP_TARGETS* targets = csound->Calloc(csound, sizeof(LOOP_JUMP_TARGETS));
      current = expand_for_statement(csound, current, typeTable, typ, targets);
      csound->Free(csound, atype);
      csound->Free(csound, typ);
      if (previous != NULL) {
        previous->next = current;
      }
      activeLoopStack = cs_cons(csound, targets, activeLoopStack);
    }
    continue;

    case LABEL_TOKEN: {
      if (activeLoopStack != NULL) {
        LOOP_JUMP_TARGETS* currentTargets = (LOOP_JUMP_TARGETS*)activeLoopStack->value;
        if (currentTargets->breakTargetLabel == current) {
          CONS_CELL* temp = activeLoopStack;
          activeLoopStack = activeLoopStack->next;
          csound->Free(csound, temp->value);
          csound->Free(csound, temp);
        }
      }
      break;
    }

    case BREAK_TOKEN: {
      TREE* breakGoto;

      if (activeLoopStack == NULL) {
        synterr(csound,Str("line:%d found break statement outside of loop."),
                current->line);
        return NULL;
      }

      breakGoto = convert_break_to_goto(csound, (LOOP_JUMP_TARGETS*)activeLoopStack->value);
      if (previous != NULL) {
        previous->next = breakGoto;
      }
      breakGoto->next = current->next;
      current = breakGoto;
      continue;
    }

    case CONTINUE_TOKEN: {
      TREE* continueGoto;

      if (activeLoopStack == NULL) {
        synterr(csound,Str("line:%d found continue statement outside of loop."),
                current->line);
        return NULL;
      }

      continueGoto = convert_continue_to_goto(csound,
                        (LOOP_JUMP_TARGETS*)activeLoopStack->value);
      if (previous != NULL) {
        previous->next = continueGoto;
      }
      continueGoto->next = current->next;
      current = continueGoto;
      continue;
    }

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
        lval = (current->left->type == INTEGER_TOKEN ?
                (double)current->left->value->value :
                current->left->value->fvalue);
        rval = (current->right->type == INTEGER_TOKEN ?
                (double)current->right->value->value :
                current->right->value->fvalue);
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
        csound->Free(csound, current->left); csound->Free(csound,
                                                          current->right);
      }
      break;
    case ENDIN_TOKEN:
    case UDOEND_TOKEN:
      csound->inZero = 1;
      /* fall through */
    default:
      // Check for struct array member assignments: array[index].member = value
      if ((current->type == '=' || current->type == T_ASSIGNMENT) &&
          current->left && current->left->type == STRUCT_EXPR) {

        TREE* anchor_expansion = NULL;
        if (expand_struct_array_member_assignment(csound, current, typeTable, &anchor_expansion)) {
          // Successfully expanded, replace current with the expanded sequence
          if (previous != NULL) {
            previous->next = anchor_expansion;
          } else if (anchor == NULL) {
            anchor = anchor_expansion;
          }

          // Find the end of the expansion chain and link to current->next
          TREE* last = anchor_expansion;
          while (last && last->next) {
            last = last->next;
          }
          if (last) {
            last->next = current->next;
          }

          current = anchor_expansion;
          continue;
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

  cs_cons_free_complete(csound, activeLoopStack);
  cs_cons_free(csound, typeTable->labelList);
  typeTable->labelList = parentLabelList;

  return anchor;
}


/* BISON PARSER FUNCTION */
int32_t csound_orcwrap(void* dummy)
{
  IGN(dummy);
  return (1);
}

/* BISON PARSER FUNCTION */
void csound_orcerror(PARSE_PARM *pp, void *yyscanner,
                     CSOUND *csound, TREE **astTree, const char *str)
{

  IGN(pp);
  IGN(astTree);
  char *p = csound_orcget_current_pointer(yyscanner)-1;
  int32_t line = csound_orcget_lineno(yyscanner);
  uint64_t files = csound_orcget_locn(yyscanner);
  uint32_t column1 = csound_orcget_first_column(yyscanner);
  uint32_t column2 = csound_orcget_last_column(yyscanner);
  if (UNLIKELY(*p=='\0' || *p=='\n')) line--;
  csound->ErrorMsg(csound, Str("%s (token \"%s\"), "),
                  str, csound_orcget_text(yyscanner));
  csound->ErrorMsg(csound, Str(" line %d, columns %d,%d\n"), line, column1, column2);
  do_baktrace(csound, files);
}

void do_baktrace(CSOUND *csound, uint64_t files)
{
  while (files) {
    uint32_t ff = files&0xff;
    files = files >>8;
    csound->ErrorMsg(csound, Str("from file %s (%d)\n"),
                    csound->filedir[ff], ff);
  }

}

/**
 * Appends TREE * node to TREE * node using ->next field in struct; walks
 * down  list to append at end; checks for NULL's and returns
 * appropriate nodes
 */
TREE* append_to_tree(CSOUND * csound, TREE *first, TREE *newlast)
{
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
  if (first->type > 400 || first-> type < 0) {
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
TREE* copy_node(CSOUND* csound, TREE* tree) {
  TREE *ans = NULL;

  if(tree != NULL) {
    ans = (TREE*)csound->Calloc(csound, sizeof(TREE));
    if (UNLIKELY(ans==NULL)) {
      if(csoundGetDebug(csound) & DEBUG_SEMANTICS)
       csoundMessage(csound, "Out of memory\n");
      exit(1);
    }
    ans->type = tree->type;
    ans->left = (tree->left == NULL) ? NULL : copy_node(csound, tree->left);
    ans->right = (tree->right == NULL) ? NULL : copy_node(csound, tree->right);
    if (tree->value != NULL) {
      ans->value = make_token(csound, tree->value->lexeme, NULL);
      ans->value->optype = csoundStrdup(csound, tree->value->optype);
      ans->value->first_column = tree->value->first_column;
      ans->value->last_column = tree->value->last_column;
    } else {
      ans->value = NULL;
    }

    ans->next = (tree->next == NULL) ? NULL : copy_node(csound, tree->next);
    ans->len = tree->len;
    ans->rate = tree->rate;
    ans->line = tree->line;
    ans->locn  = tree->locn;
    ans->markup = NULL;
  }
  return ans;
}

/* Copy only left and right branches (skips next) */
TREE* copy_node_shallow(CSOUND* csound, TREE* tree) {
  TREE *ans = NULL;

  if(tree != NULL) {
    ans = (TREE*)csound->Calloc(csound, sizeof(TREE));
    if (UNLIKELY(ans==NULL)) {
      /* fprintf(stderr, "Out of memory\n"); */
      exit(1);
    }
    ans->type = tree->type;
    ans->left = tree->left;
    ans->right = tree->right;

    if (tree->value != NULL) {
      ans->value = make_token(csound, tree->value->lexeme, NULL);
      ans->value->optype = csoundStrdup(csound, tree->value->optype);
      ans->value->first_column = tree->value->first_column;
      ans->value->last_column = tree->value->last_column;
    } else {
      ans->value = NULL;
    }

    ans->next = NULL;
    ans->len = tree->len;
    ans->rate = tree->rate;
    ans->line = tree->line;
    ans->locn  = tree->locn;
    ans->markup = NULL;
  }
  return ans;
}

TREE* make_node(CSOUND *csound, int32_t line, uint64_t locn, int32_t type,
                TREE* left, TREE* right)
{
  TREE *ans;
  ans = (TREE*)csound->Calloc(csound, sizeof(TREE));
  if (UNLIKELY(ans==NULL)) {
   if(csoundGetDebug(csound) & DEBUG_SEMANTICS)
    csound->Message(csound, "Out of memory\n");
   exit(1);
  }
  ans->type = type;
  ans->left = left;
  ans->right = right;
  ans->value = NULL;          /* New code -- JPff */
  ans->next = NULL;
  ans->len = 2;
  ans->rate = -1;
  ans->line = line;
  ans->locn  = locn;
  ans->markup = NULL;
  return ans;
}

TREE* make_leaf(CSOUND *csound, int32_t line, uint64_t locn, int32_t type,
                ORCTOKEN *v)
{
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
  ans->locn  = locn;
  ans->markup = NULL;
  if(csoundGetDebug(csound) & DEBUG_SEMANTICS)
   csoundMessage(csound, "...csound_orc_semantics(%d) line = %d\n",
                   __LINE__, line);
  return ans;
}

TREE* make_opcall_from_func_start(CSOUND *csound, int32_t line, uint64_t locn,
                                  int32_t type, TREE* left, TREE* right) {
  TREE* firstArg = left->right;
  TREE* first = right;
  TREE* rest = right->next;

  right->next = NULL;
  TREE* operatorNode = make_node(csound, line, locn, type, firstArg, first);
  operatorNode->next = rest;
  left->right = operatorNode;

  return left;
}

static void delete_tree(CSOUND *csound, TREE **l)
{
  while (1) {
    TREE *old = *l;
    TREE *tree = *l;
    if (UNLIKELY(*l==NULL)) {
      return;
    }
    if (tree->value) {
      if (tree->value->lexeme) {
        csound->Free(csound, tree->value->lexeme);
        tree->value->lexeme = NULL;
      }
      csound->Free(csound,tree->value);
      tree->value = NULL;
    }
    delete_tree(csound, &(tree->left));
    tree->left = NULL;
    delete_tree(csound, &(tree->right));
    tree->right = NULL;
    *l = tree->next;
    csound->Free(csound, old);
  }
}

 void csoundDeleteTree(CSOUND *csound, TREE *tree)
{
  delete_tree(csound, &tree);
}


/* DEBUGGING FUNCTIONS */
void print_tree_i(CSOUND *csound, TREE *l, int32_t n)
{
  int32_t i;
  if (UNLIKELY(l==NULL)) {
    return;
  }
  for (i=0; i<n; i++) {
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
    csound->Message(csound,"%c:(%d:%s)\n", l->type,
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case NEWLINE:
    csound->Message(csound,"NEWLINE:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case S_NEQ:
    csound->Message(csound,"S_NEQ:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case S_AND:
    csound->Message(csound,"S_AND:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case S_OR:
    csound->Message(csound,"S_OR:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case S_LT:
    csound->Message(csound,"S_LT:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case S_LE:
    csound->Message(csound,"S_LE:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case S_EQ:
    csound->Message(csound,"S_EQ:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case S_UNOT:
    csound->Message(csound,"S_UNOT:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case S_GT:
    csound->Message(csound,"S_GT:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case S_GE:
    csound->Message(csound,"S_GE:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case LABEL_TOKEN:
    csound->Message(csound,"LABEL_TOKEN: %s\n", l->value->lexeme); break;
  case IF_TOKEN:
    csound->Message(csound,"IF_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case THEN_TOKEN:
    csound->Message(csound,"THEN_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case ITHEN_TOKEN:
    csound->Message(csound,"ITHEN_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case KTHEN_TOKEN:
    csound->Message(csound,"KTHEN_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case ELSEIF_TOKEN:
    csound->Message(csound,"ELSEIF_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case ELSE_TOKEN:
    csound->Message(csound,"ELSE_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case UNTIL_TOKEN:
    csound->Message(csound,"UNTIL_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case WHILE_TOKEN:
    csound->Message(csound,"WHILE_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case DO_TOKEN:
    csound->Message(csound,"DO_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case OD_TOKEN:
    csound->Message(csound,"OD_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case GOTO_TOKEN:
    csound->Message(csound,"GOTO_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case IGOTO_TOKEN:
    csound->Message(csound,"IGOTO_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case KGOTO_TOKEN:
    csound->Message(csound,"KGOTO_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case INSTR_TOKEN:
    csound->Message(csound,"INSTR_TOKEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case STRING_TOKEN:
    csound->Message(csound,"STRING_TOKEN: %s\n", l->value->lexeme); break;
  case T_IDENT:
    csound->Message(csound,"T_IDENT: %s\n", l->value->lexeme); break;
  case INTEGER_TOKEN:
    csound->Message(csound,"INTEGER_TOKEN: %d\n", l->value->value); break;
  case NUMBER_TOKEN:
    csound->Message(csound,"NUMBER_TOKEN: %f\n", l->value->fvalue); break;
  case S_ANDTHEN:
    csound->Message(csound,"S_ANDTHEN:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case S_APPLY:
    csound->Message(csound,"S_APPLY:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case T_FUNCTION:
    csound->Message(csound,"T_FUNCTION: %s\n",
                    l->value->lexeme); break;
  case S_UMINUS:
    csound->Message(csound,"S_UMINUS:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case S_UPLUS:
    csound->Message(csound,"S_UPLUS:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  case '[':
    csound->Message(csound,"[:(%d:%s)\n",
                    l->line, csound->filedir[(l->locn)&0xff]); break;
  default:
    csound->Message(csound,"unknown:%d(%d:%s)\n",
                    l->type, l->line, csound->filedir[(l->locn)&0xff]);
  }

  print_tree_i(csound, l->left,n+1);
  print_tree_i(csound, l->right,n+1);

  if (l->next != NULL) {
    print_tree_i(csound, l->next, n);
  }
}

enum {TREE_NONE, TREE_LEFT, TREE_RIGHT, TREE_NEXT};
static void print_tree_xml(CSOUND *csound, TREE *l, int32_t n, int32_t which)
{
  int32_t i;
  char *child[4] = {"", "left", "right", "next"};
  if (l==NULL) {
    return;
  }
  for (i=0; i<n; i++) {
    csound->Message(csound, " ");
  }

  csound->Message(csound,
                  "<tree%s addresses=\"(%p : %p)\" type=\"%d\" ",
                  child[which],l, l->value, l->type);

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
  case '|':
  case '&':
  case '#':
  case '~':
    csound->Message(csound,"name=\"%c\"", l->type); break;
  case T_ASSIGNMENT:
    csound->Message(csound,"name=\"T_ASSIGNMENT\""); break;
  case NEWLINE:
    csound->Message(csound,"name=\"NEWLINE\""); break;
  case S_NEQ:
    csound->Message(csound,"name=\"S_NEQ\""); break;
  case S_AND:
    csound->Message(csound,"name=\"S_AND\""); break;
  case S_OR:
    csound->Message(csound,"name=\"S_OR\""); break;
  case S_LT:
    csound->Message(csound,"name=\"S_LT\""); break;
  case S_LE:
    csound->Message(csound,"name=\"S_LE\""); break;
  case S_EQ:
    csound->Message(csound,"name=\"S_EQ\""); break;
  case S_UNOT:
    csound->Message(csound,"name=\"S_UNOT\""); break;
  case S_GT:
    csound->Message(csound,"name=\"S_GT\""); break;
  case S_GE:
    csound->Message(csound,"name=\"S_GE\""); break;
  case S_BITSHIFT_RIGHT:
    csound->Message(csound,"name=\"S_BITSHIFT_RIGHT\""); break;
  case S_BITSHIFT_LEFT:
    csound->Message(csound,"name=\"S_BITSHIFT_LEFT\""); break;
  case LABEL_TOKEN:
    csound->Message(csound,"name=\"LABEL_TOKEN\" label=\"%s\"",
                    l->value->lexeme); break;
  case IF_TOKEN:
    csound->Message(csound,"name=\"IF_TOKEN\""); break;
  case THEN_TOKEN:
    csound->Message(csound,"name=\"THEN_TOKEN\""); break;
  case ITHEN_TOKEN:
    csound->Message(csound,"name=\"ITHEN_TOKEN\""); break;
  case KTHEN_TOKEN:
    csound->Message(csound,"name=\"KTHEN_TOKEN\""); break;
  case ELSEIF_TOKEN:
    csound->Message(csound,"name=\"ELSEIF_TOKEN\""); break;
  case ELSE_TOKEN:
    csound->Message(csound,"name=\"ELSE_TOKEN\""); break;
  case UNTIL_TOKEN:
    csound->Message(csound,"name=\"UNTIL_TOKEN\""); break;
  case WHILE_TOKEN:
    csound->Message(csound,"name=\"WHILE_TOKEN\""); break;
  case DO_TOKEN:
    csound->Message(csound,"name=\"DO_TOKEN\""); break;
  case OD_TOKEN:
    csound->Message(csound,"name=\"OD_TOKEN\""); break;
  case GOTO_TOKEN:
    csound->Message(csound,"name=\"GOTO_TOKEN\""); break;
  case IGOTO_TOKEN:
    csound->Message(csound,"name=\"IGOTO_TOKEN\""); break;
  case KGOTO_TOKEN:
    csound->Message(csound,"name=\"KGOTO_TOKEN\""); break;
  case INSTR_TOKEN:
    csound->Message(csound,"name=\"INSTR_TOKEN\""); break;
  case STRING_TOKEN:
    csound->Message(csound,"name=\"T_STRCONST\" str=\"%s\"",
                    l->value->lexeme); break;
  case T_IDENT:
    csound->Message(csound,"name=\"T_IDENT\" varname=\"%s\"",
                    l->value->lexeme); break;
  case T_OPCALL:
    if (l->left && l->left->value)
      csound->Message(csound,"name=\"T_OPCALL\" varname=\"%s\"",
                      l->left->value->lexeme);
    else
       csound->Message(csound,"name=\"T_OPCALL\" varname=\"%s\"",
                       l->value->lexeme);
    break;
  case T_DECLARE:
    csound->Message(csound,"name=\"T_DECLARE\" declvar=\"%s\"",
                    l->value->lexeme); break;
  case T_ARRAY:
    csound->Message(csound,"name=\"T_ARRAY\""); break;

  case T_ARRAY_IDENT:
    csound->Message(csound,"name=\"T_ARRAY_IDENT\" varname=\"%s\"",
                    l->value->lexeme); break;

  case INTEGER_TOKEN:
    csound->Message(csound,"name=\"INTEGER_TOKEN\" value=\"%d\"",
                    l->value->value); break;
  case NUMBER_TOKEN:
    csound->Message(csound,"name=\"NUMBER_TOKEN\" value=\"%f\"",
                    l->value->fvalue); break;
  case S_ANDTHEN:
    csound->Message(csound,"name=\"S_ANDTHEN\""); break;
  case S_APPLY:
    csound->Message(csound,"name=\"S_APPLY\""); break;
  case T_FUNCTION:
    csound->Message(csound,"name=\"T_FUNCTION\" fname=\"%s\"",
                    l->value->lexeme); break;
  case S_UMINUS:
    csound->Message(csound,"name=\"S_UMINUS\""); break;
  case S_UPLUS:
    csound->Message(csound,"name=\"S_UPLUS\""); break;

  case UDO_TOKEN:
    csound->Message(csound,"name=\"UDO_TOKEN\""); break;
  case UDO_ANS_TOKEN:
    csound->Message(csound,"name=\"UDO_ANS_TOKEN\" signature=\"%s\"",
                    l->value->lexeme); break;
  case UDO_ARGS_TOKEN:
    csound->Message(csound,"name=\"UDO_ARGS_TOKEN\" signature=\"%s\"",
                    l->value->lexeme); break;
  case S_ELIPSIS:
    csound->Message(csound,"name=\"S_ELIPSIS\""); break;
  case S_ADDIN:
    csound->Message(csound,"name=\"##addin\""); break;
    break;
  case S_SUBIN:
    csound->Message(csound,"name=\"##subin\""); break;
    break;
  case S_DIVIN:
    csound->Message(csound,"name=\"##divin\""); break;
    break;
  case S_MULIN:
    csound->Message(csound,"name=\"##mulin\""); break;
    break;
  case '[':
    csound->Message(csound,"name=\"[\""); break;
  default:
    csound->Message(csound,"name=\"unknown(%d)\"", l->type);
  }

  csound->Message(csound, " loc=\"%d:%s\">\n",
                  l->line, csound->filedir[(l->locn)&0xff]);

  print_tree_xml(csound, l->left,n+1, TREE_LEFT);
  print_tree_xml(csound, l->right,n+1, TREE_RIGHT);

  for (i=0; i<n; i++) {
    csound->Message(csound, " ");
  }

  csound->Message(csound, "</tree%s>\n", child[which]);

  if (l->next != NULL) {
    print_tree_xml(csound, l->next, n, TREE_NEXT);
  }
}

void print_tree(CSOUND * csound, char* msg, TREE *l)
{
  if (msg)
    csound->Message(csound, "%s", msg);
  else
    csound->Message(csound, "Printing Tree\n");
  csound->Message(csound, "<ast>\n");
  print_tree_xml(csound, l, 0, TREE_NONE);
  csound->Message(csound, "</ast>\n");
}

void handle_optional_args(CSOUND *csound, TREE *l)
{
  if (l == NULL || l->type == LABEL_TOKEN) return;
  {

    OENTRY *ep = (OENTRY*)l->markup;
    int32_t nreqd = 0;
    int32_t incnt = tree_arg_list_count(l->right);
    TREE * temp;
    char** inArgParts = NULL;

    if (UNLIKELY(ep==NULL)) { /* **** FIXME **** */
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
    if (incnt < nreqd) {         /*  or set defaults: */
      do {
        switch (*inArgParts[incnt]) {
        case 'O':             /* Will this work?  Doubtful code.... */
        case 'o':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "0", NULL));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right==NULL) l->right = temp;
          else append_to_tree(csound, l->right, temp);
          break;
        case 'P':
        case 'p':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "1", NULL));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right==NULL) l->right = temp;
          else append_to_tree(csound, l->right, temp);
          break;
        case 'q':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "10", NULL));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right==NULL) l->right = temp;
          else append_to_tree(csound, l->right, temp);
          break;

        case 'V':
        case 'v':
          temp = make_leaf(csound, l->line, l->locn, NUMBER_TOKEN,
                           make_num(csound, ".5", NULL));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right==NULL) l->right = temp;
          else append_to_tree(csound, l->right, temp);
          break;
        case 'h':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "127", NULL));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right==NULL) l->right = temp;
          else append_to_tree(csound, l->right, temp);
          break;
        case 'J':
        case 'j':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "-1", NULL));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right==NULL) l->right = temp;
          else append_to_tree(csound, l->right, temp);
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
      for (n=0; inArgParts[n] != NULL; n++) {
        csound->Free(csound, inArgParts[n]);
      }
      csound->Free(csound, inArgParts);
    }
  }
}


CS_VARIABLE *add_global_variable(CSOUND *csound, ENGINE_STATE *engineState,
                               CS_TYPE *type, char *name, void *typeArg);
void add_instr_variable(CSOUND *csound,  TREE *x) {
  /* add instr variable to engine varpool
     called by bison when instr ids are found
  */
  if (x->type == T_IDENT) {
    char *varname = x->value->lexeme;
    CS_VARIABLE *var = add_global_variable(csound, &csound->engineState,
                                         (CS_TYPE*)&CS_VAR_TYPE_INSTR, varname,
                                           NULL);
    if(var == NULL)
      csound->Warning(csound, "Could not add instrument ref %s", varname);
  }
}
