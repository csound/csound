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

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
//#include <stdbool.h>
#include "csoundCore.h"
#include "csound_orc.h"
#include "interlocks.h"
#include "namedins.h"
#include "parse_param.h"
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include "csound_orc_expressions.h"
#include "csound_orc_semantics.h"

extern char *csound_orcget_text ( void *scanner );
static int32_t is_label(char* ident, CONS_CELL* labelList);

extern uint64_t csound_orcget_locn(void *);
extern  char argtyp2(char*);
extern  int32_t tree_arg_list_count(TREE *);
void print_tree(CSOUND *, char *, TREE *);

/* from csound_orc_compile.c */
extern int32_t argsRequired(char* arrayName);
extern char** splitArgs(CSOUND* csound, char* argString);
extern int32_t pnum(char*);

OENTRIES* find_opcode2(CSOUND*, char*);
char* resolve_opcode_get_outarg(CSOUND* csound,
                                OENTRIES* entries, char* inArgTypes);
int32_t check_out_args(CSOUND* csound, char* outArgsFound, char* opOutArgs);
char* get_arg_string_from_tree(CSOUND* csound, TREE* tree,
                               TYPE_TABLE* typeTable);
char* convert_internal_to_external(CSOUND* csound, char* arg);
char* convert_external_to_internal(CSOUND* csound, char* arg);
void do_baktrace(CSOUND *csound, uint64_t files);

extern int32_t add_udo_definition(CSOUND *csound, bool newStyle, char *opname,
                              char *outtypes, char *intypes, int32_t flags);
extern TREE * create_opcode_token(CSOUND *csound, char* op);
int32_t is_reserved(char*);

const char* SYNTHESIZED_ARG = "_synthesized";
const char* UNARY_PLUS = "_unary_plus";

char* cs_strdup(CSOUND* csound, const char* str) {
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
    return cs_strdup(csound, str);
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
  case S_ADDIN:
    return "##addin";
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

//FIXME - current just returns subtype but assumes single char type name,
// should check for long type names, as well as check dimensions and remove one
char* get_array_sub_type(CSOUND* csound, char* arrayName) {
  char temp[2];
  char *t = arrayName;

  if (*t == '#') t++;
  if (*t == 'g') t++;

  if (*t == 't') { /* Support legacy t-vars by mapping to k subtypes */
    return cs_strdup(csound, "k");
  }

  while (*t == '[') {
    t++;
  }
  temp[0] = *t;
  temp[1] = 0;
  return cs_strdup(csound, temp);
}

char* create_array_arg_type(CSOUND* csound, CS_VARIABLE* arrayVar) {
  if (arrayVar->subType == NULL) return NULL;

  char* varTypeName = arrayVar->subType->varTypeName;
  int32_t len = arrayVar->dimensions + (int32_t) strlen(varTypeName) + 2;
  char* retVal = csound->Malloc(csound, len);
  memset(retVal, '[', arrayVar->dimensions);
  strNcpy(retVal + arrayVar->dimensions, varTypeName, strlen(varTypeName) + 1);
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

static int32_t isirate(/*CSOUND *csound,*/ TREE *t)
{                  /* check that argument is an i-rate constant or variable */
  //print_tree(csound, "isirate",  t);
  if (t->type == INTEGER_TOKEN) {
    //printf("integer case\n");
    return 1;
  }
  else if (t->type == T_IDENT) {
    //printf("identifier case\n");
    if (t->value->lexeme[0] != 'p' &&
        t->value->lexeme[0] != 'i' &&
        (t->value->lexeme[0] != 'g' ||
         t->value->lexeme[1] != 'i')) return 0;
    return 1;
  }
  else if (t->type == T_ARRAY) {
    //printf("array case\n");
    if (isirate(/*csound, */t->right)==0) return 0;
    t = t->next;
    while (t) {
      //printf("t=%p t->type=%d\n", t, t->type);
      if (isirate(/*csound,*/ t)==0) return 0;
      t = t->next;
    }
    return 1;
  }
  else return 0;
}

CS_VARIABLE* find_var_from_pools(CSOUND* csound, char* varName, char* varBaseName, TYPE_TABLE* typeTable) {
  CS_VARIABLE* var = NULL;

  /* VL: 16/01/2014
     in a second compilation, the
     typeTable->globalPool is incorrect and will not
     contain the correct addresses of global variables,
     which are stored correctly in the engineState.varPool.
     Ideally we should remove typeTable->globalPool and only use
     the varPool in the engineState
  */

  if (*varName == 'g') {
    var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                     varBaseName);
    if(var == NULL)
      var = csoundFindVariableWithName(csound, typeTable->globalPool,
                                       varBaseName);
  } else {
    var = csoundFindVariableWithName(csound, typeTable->localPool,
                                     varBaseName);
  }
  return var;
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

    if (tree->type == T_ARRAY) {
      varBaseName = tree->left->value->lexeme;

      var = find_var_from_pools(csound, varBaseName, varBaseName, typeTable);

      if (var == NULL) {
        char *fnReturn;
        if (tree->left->type == T_FUNCTION &&
            (fnReturn = get_arg_type2(csound, tree->left, typeTable)) &&
            *fnReturn == '[') {
          return cs_strdup(csound, &fnReturn[1]);
        } else {
          synterr(csound,
                  Str("unable to find array operator for var %s line %d\n"), varBaseName, tree->line);
          do_baktrace(csound, tree->locn);
          return NULL;
        }
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
      //print_tree(csound, "i()", tree);
      if (tree->right->type == T_ARRAY &&
          tree->right->left->type == T_IDENT &&
          isirate(/*csound,*/ tree->right->right)) {
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
        synterr(csound, Str("error: opcode '%s' for expression with arg "
                            "types %s not found, line %d\n"),
                opname, argTypeRight, tree->line);
        do_baktrace(csound, tree->locn);
        csound->Free(csound, argTypeRight);
        csound->Free(csound, entries);
        return NULL;
      }

      if (argsRequired(out) == 1) {
        char** args = splitArgs(csound, out);
        char *ret = cs_strdup(csound, args[0]);
        csound->Free(csound, argTypeRight);
        csound->Free(csound, entries);
        return ret;
      }

      synterr(csound, Str("error: opcode '%s' for expression with arg "
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
        synterr(csound, Str("error: opcode '%s' for expression with arg "
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

  switch(tree->type) {
  case NUMBER_TOKEN:
  case INTEGER_TOKEN:
    return cs_strdup(csound, "c");                              /* const */
  case STRING_TOKEN:
    return cs_strdup(csound, "S");                /* quoted String */
  case LABEL_TOKEN:
    //FIXME: Need to review why label token is used so much in parser,
    //for now treat as T_IDENT
  case T_ARRAY_IDENT:
    //check
    if((var = csoundFindVariableWithName(csound, typeTable->localPool,
                                         tree->value->lexeme)) != NULL) {
      if(var->varType != &CS_VAR_TYPE_ARRAY) {
      synterr(csound, Str("Array variable name '%s' used before as a different type\n"
                          "Line %d"),
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

    if (pnum(s) >= 0)
      return cs_strdup(csound, "p");                           /* pnum */

    varBaseName = s;

    if (*s == '#')
      s++;

    /* VL: 16/01/2014
       in a second compilation, the
       typeTable->globalPool is incorrect and will not
       contain the correct addresses of global variables,
       which are stored correctly in the engineState.varPool.
       Ideally we should remove typeTable->globalPool and only use
       the varPool in the engineState
    */

    if (*s == 'g' || is_reserved(s)) {
      var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                       tree->value->lexeme);
      if (var == NULL)
        var = csoundFindVariableWithName(csound, typeTable->globalPool,
                                         tree->value->lexeme);
      //printf("var: %p %s\n", var, var->varName);
    } else
      var = csoundFindVariableWithName(csound, typeTable->localPool,
                                       tree->value->lexeme);

    if (UNLIKELY(var == NULL)) {
      synterr(csound, Str("Variable '%s' used before defined\n"
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
    return cs_strdup(csound, tree->value->optype);
  case STRUCT_EXPR:
    s = tree->left->value->lexeme;
    var = find_var_from_pools(csound, s, s, typeTable);

    if (UNLIKELY(var == NULL)) {
      synterr(csound, Str("Variable '%s' used before defined\n"), s);
      do_baktrace(csound, tree->locn);
      return NULL;
    }

    tree = tree->right;
    while (tree != NULL) {
      s = tree->value->lexeme;
      CONS_CELL* cell = var->varType->members;
      CS_VARIABLE* nextVar = NULL;
      while (cell != NULL) {
        CS_VARIABLE* member = (CS_VARIABLE*)cell->value;
        if (!strcmp(member->varName, s)) {
          nextVar = member;
          break;
        }
        cell = cell->next;
      }
      if (nextVar == NULL) {
        synterr(csound, Str("No member '%s' found for variable 'xxx'\n"), s);
        do_baktrace(csound, tree->locn);
        return NULL;
      }
      var = nextVar;
      tree = tree->next;
    }

    return cs_strdup(csound, var->varType->varTypeName);

  case T_ARRAY:

    s = tree->value->lexeme;

    if (*s == '#') s++;
    if (*s == 'g') s++;

    if (*s == 't') { /* Support legacy t-vars by mapping to k-array */
      return cs_strdup(csound, "[k]");
    }

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
    csoundWarning(csound, Str("Unknown arg type: %d\n"), tree->type);
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

int32_t check_in_arg(char* found, char* required) {
  char* t;
  int32_t i;
  if (UNLIKELY(found == NULL || required == NULL)) {
    return 0;
  }

  if (strcmp(found, required) == 0) {
    return 1;
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
    int32_t argsFoundCount = argsRequired(inArgsFound);
    int32_t argsRequiredCount = argsRequired(opInArgs);
    char** argsRequired = splitArgs(csound, opInArgs);
    char** argsFound;
    int32_t i;
    int32_t argTypeIndex = 0;
    char* varArg = NULL;
    int32_t returnVal = 1;

    if (argsRequired == NULL) {
      return 0;
    }
    if (argsFoundCount>=VARGMAX) {
      return -1;
    }

    if ((argsFoundCount > argsRequiredCount) &&
        !(is_in_var_arg(argsRequired[argsRequiredCount - 1]))) {
      csound->Free(csound, argsRequired);
      return 0;
    }

    argsFound = splitArgs(csound, inArgsFound);

    if (argsFoundCount == 0) {
      if (is_in_var_arg(argsRequired[0])) {
        varArg = argsRequired[0];
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
          char* argRequired = argsRequired[argTypeIndex++];
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
      while (argTypeIndex < argsRequiredCount) {
        char* c = argsRequired[argTypeIndex++];

        if (!is_in_optional_arg(c) && !is_in_var_arg(c)) {
          returnVal = 0;
          break;
        }
      }

    }
    //printf("delete %p\n", argsFound);
    int32_t n;
    for (n=0; argsFound[n] != NULL; n++) {
      // printf("delete %p\n", argsFound[n]);
      csound->Free(csound, argsFound[n]);
    }
    csound->Free(csound, argsFound);
    //printf("delete %p\n", argsRequired);
    for (n=0; argsRequired[n] != NULL; n++) {
      //printf("delete %p\n", argsRequired[n]);
      csound->Free(csound, argsRequired[n]);
    }
    csound->Free(csound, argsRequired);

    return returnVal;
  }
}

inline static int32_t is_out_var_arg(char* arg) {
  return strlen(arg) == 1 && (strchr("mzIXNF*", *arg) != NULL);
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
    int32_t argsFoundCount = argsRequired(outArgsFound);
    int32_t argsRequiredCount = argsRequired(opOutArgs);
    char** argsRequired = splitArgs(csound, opOutArgs);
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

    argsFound = splitArgs(csound, outArgsFound);

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
    //printf("delete %p\n", argsFound);
    int32_t n;
    for (n=0; argsFound[n] != NULL; n++) {
      // printf("delete %p\n", argsFound[n]);
      csound->Free(csound, argsFound[n]);
    }
    csound->Free(csound, argsFound);
    //printf("delete %p\n", argsRequired);
    for (n=0; argsRequired[n] != NULL; n++) {
      //printf("delete %p\n", argsRequired[n]);
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
OENTRY* resolve_opcode(CSOUND* csound, OENTRIES* entries,
                       char* outArgTypes, char* inArgTypes) {

  //    OENTRY* retVal = NULL;
  int32_t i, check;

  for (i = 0; i < entries->count; i++) {
    OENTRY* temp = entries->entries[i];
    //        if (temp->intypes == NULL && temp->outypes == NULL) {
    //            if (outArgTypes == NULL && inArgTypes == NULL) {
    //
    //            }
    //            continue;
    //        }
    if ((check = check_in_args(csound, inArgTypes, temp->intypes)) &&
        check_out_args(csound, outArgTypes, temp->outypes)) {
      //            if (retVal != NULL) {
      //                return NULL;
      //            }
      //            retVal = temp;
      if (check == -1)
        synterr(csound,
                Str("Found %d inputs for %s which is more than "
                    "the %d allowed\n"),
                argsRequired(inArgTypes), temp->opname, VARGMAX);

      return temp;
    }
  }
  return NULL;
  //    return retVal;
}

OENTRY* resolve_opcode_exact(CSOUND* csound, OENTRIES* entries,
                             char* outArgTypes, char* inArgTypes) {
  IGN(csound);
  OENTRY* retVal = NULL;
  int32_t i;

  char* outTest = (!strcmp("0", outArgTypes)) ? "" : outArgTypes;
  for (i = 0; i < entries->count; i++) {
    OENTRY* temp = entries->entries[i];
    if (temp->intypes != NULL && !strcmp(inArgTypes, temp->intypes) &&
        temp->outypes != NULL && !strcmp(outTest, temp->outypes)) {
      retVal = temp;
    }
  }
  return retVal;
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

/* Converts internal array specifier from [[a] to a[][].
   Used by get_arg_string_from_tree to create an arg string that is
   compatible with the ones found in OENTRY's.  splitArgs converts back
   to internal representation. */
char* convert_internal_to_external(CSOUND* csound, char* arg) {
  int32_t i = 0, n = 0, dimensions;
  char *start;
  char *retVal, *current;
  uint64_t nameLen, len = strlen(arg);
  char type[64] = {0}, c;

  if (arg == NULL || len == 1) {
    return arg;
  }

  // VL 15.10.24
  // synthetic args reach here with
  // : prepended and ; appended to name
  // so we need to remove them to avoid
  // accummulation

  // now remove any : or ; leftover in typename
  do  {
         c = arg[n++];
         if(c == ':' || c == ';') continue;  
         type[i++] = c;
   } while (c);
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

  retVal = csound->Malloc(csound, sizeof(char) * (nameLen + (dimensions * 2) + 1));
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
  //csound->Free(csound, arg);
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
  //csound->Free(csound, arg);
  return retVal;
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


/* Used by new UDO syntax, expects tree's with value->lexeme as type names */
char* get_in_types_from_tree(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable) {
  int32_t len = tree_arg_list_count(tree);

  if (len == 0 || (len == 1 && !strcmp(tree->value->lexeme, "0"))) {
    return cs_strdup(csound, "0");
  }
  return get_arg_string_from_tree(csound, tree, typeTable);
}

/* Used by new UDO syntax, expects tree's with value->lexeme as type names */
char* get_out_types_from_tree(CSOUND* csound, TREE* tree) {

  int32_t len = tree_arg_list_count(tree);
  char* argTypes = csound->Malloc(csound, len * 256 * sizeof(char));
  int32_t i;

  if (len == 0 || (len == 1 && !strcmp(tree->value->lexeme, "0"))) {
    return cs_strdup(csound, "0");
  }

  int32_t argsLen = 0;
  i = 0;

  TREE* current = tree;

  while (current != NULL) {
    char* argType = current->value->lexeme;
    int32_t len = (int32_t) strlen(argType);
    int32_t offset = i * 256;
    argsLen += len;

    // relying on the fact that built-in array types have
    // arrays in different tree nodes from user defined structs
    if (current->right != NULL && *current->right->value->lexeme == '[') {
      strcpy(&argTypes[offset], argType);
      argTypes[offset + len] = '[';
      argTypes[offset + len + 1] = ']';
      argTypes[offset + len + 2] = '\0';
      argsLen += 2;
    } else if (len > 1) {
      argTypes[offset] = ':';
      memcpy(argTypes + offset + 1, argType, len);
      argTypes[offset + len + 1] = ';';
      argTypes[offset + len + 2] = '\0';
      argsLen += 2;
    } else {
      strcpy(&argTypes[offset], argType);
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

  //    csound->Message(csound, "Searching for opcode: %s | %s | %s\n",
  //                    outArgsFound, opname, inArgsFound);

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

  if (opcodes->count == 0) {
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
  CS_VAR_POOL* pool;

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

        if (is_label(varName, typeTable->labelList)) {
          break;
        }

        argType = get_arg_type2(csound, current, typeTable);
        if (UNLIKELY(argType==NULL)) {
          synterr(csound,
                  Str("Variable type for %s could not be determined."), varName);
          do_baktrace(csound, tree->locn);
          return 0;
        }

        //FIXME - this feels like a hack
        if (*argType == 'c' || *argType == 'r' || *argType == 'p') {
          csound->Free(csound, argType);
          break;
        }
        csound->Free(csound, argType);
        pool = (*varName == 'g') ?
          typeTable->globalPool : typeTable->localPool;
        var = csoundFindVariableWithName(csound, pool, varName);
        if (UNLIKELY(var == NULL)) {
          /* VL: 13-06-13
             if it is not found, we still check the global (merged) pool */
          if (*varName == 'g')
            var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                             varName);
          if (UNLIKELY(var == NULL)) {
            synterr(csound,
                    Str("Variable '%s' used before defined\nline %d"),
                    varName, tree->line);
            do_baktrace(csound, tree->locn);
            return 0;
          }
        }

        break;
      case T_ARRAY:
        varName = current->left->value->lexeme;

        pool = (*varName == 'g') ?
          typeTable->globalPool : typeTable->localPool;

        if (UNLIKELY(csoundFindVariableWithName(csound, pool, varName) == NULL)) {
          CS_VARIABLE *var = 0;
          /* VL: 13-06-13
             if it is not found, we still check the global (merged) pool */
          if (var == NULL && *varName == 'g')
            var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                             varName);
          if (UNLIKELY(var == NULL)) {
            synterr(csound,
                    Str("Variable '%s' used before defined\nLine %d\n"),
                    varName, current->left->line);
            do_baktrace(csound, current->left->locn);
            return 0;
          }
        }
        break;
      default:
        //synterr(csound, "Unknown arg type: %s\n", current->value->lexeme);
        //printf("\t->FOUND OTHER: %s %d\n", current->value->lexeme,
        //                                   current->type);
        break;
      }

    }

    current = current->next;
  }

  return 1;
}

void add_arg(CSOUND* csound, char* varName, char* annotation, TYPE_TABLE* typeTable) {

  const CS_TYPE* type;
  CS_VARIABLE* var;
  char *t;
  CS_VAR_POOL* pool;
  char argLetter[2];
  ARRAY_VAR_INIT varInit;
  void* typeArg = NULL;

  t = varName;

  if (*t == '#') t++;
  pool = (*t == 'g') ? typeTable->globalPool : typeTable->localPool;

  var = csoundFindVariableWithName(csound, pool, varName);
  if (var == NULL) {
    if (annotation != NULL) {
      type = csoundGetTypeWithVarTypeName(csound->typePool, annotation);
      typeArg = (void *) type;
    } else {
      t = varName;
      argLetter[1] = 0;

      if (*t == '#') t++;
      if (*t == 'g') t++;
      
      

      if (*t == '[' || *t == 't') { /* Support legacy t-vars */
        int32_t dimensions = 1;
        const CS_TYPE* varType;
        char* b = t + 1;

        while(*b == '[') {
          b++;
          dimensions++;
        }
        argLetter[0] = (*b == 't') ? 'k' : *b; /* Support legacy t-vars */

        varType = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);

        varInit.dimensions = dimensions;
        varInit.type =  varType;
        typeArg = &varInit;
      }

      argLetter[0] = (*t == 't') ? '[' : *t; /* Support legacy t-vars */
      
      

      type = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);
    }

    var = csoundCreateVariable(csound, csound->typePool,
                               type, varName, typeArg);
    csoundAddVariable(csound, pool, var);
  } else {
    //TODO - implement reference count increment
  }

}

void add_array_arg(CSOUND* csound, char* varName, char* annotation, int32_t dimensions,
                   TYPE_TABLE* typeTable) {

  CS_VARIABLE* var;
  char *t;
  CS_VAR_POOL* pool;
  char argLetter[2];
  ARRAY_VAR_INIT varInit;
  void* typeArg = NULL;

  pool = (*varName == 'g') ? typeTable->globalPool : typeTable->localPool;

  var = csoundFindVariableWithName(csound, pool, varName);
  if (var == NULL) {
    const CS_TYPE* varType;

    if (annotation != NULL) {
      varType = csoundGetTypeWithVarTypeName(csound->typePool, annotation);
    } else {
      t = varName;
      argLetter[1] = 0;

      if (*t == '#') t++;
      if (*t == 'g') t++;

      argLetter[0] = (*t == 't') ? 'k' : *t; /* Support legacy t-vars */

      varType =
        csoundGetTypeWithVarTypeName(csound->typePool, argLetter);
    }


    varInit.dimensions = dimensions;
    varInit.type = varType;
    typeArg = &varInit;

    var = csoundCreateVariable(csound, csound->typePool,
                               &CS_VAR_TYPE_ARRAY,
                               varName, typeArg);
    csoundAddVariable(csound, pool, var);
  } else {
    //TODO - implement reference count increment
  }
}

/* return 1 on succcess, 0 on failure */
int32_t add_args(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable)
{
  TREE* current;
  char* varName;

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

      /* TODO - This needs to check if someone put in sr:k or ksmps:i or something like that */
      if (is_reserved(varName)) {
        // skip reserved vars, these are handled elsewhere
        break;
      }

      if (*varName == 't' && current->value->optype == NULL) { /* Support legacy t-vars */
        add_array_arg(csound, varName, "k", 1, typeTable);
      } else {
        add_arg(csound, varName, current->value->optype, typeTable);
      }

      break;

    case T_ARRAY:
      varName = current->left->value->lexeme;
      // FIXME - this needs to work for array and a-names
      add_arg(csound, varName, NULL, typeTable);
      break;

    default:
      //synterr(csound, "Unknown arg type: %s\n", current->value->lexeme);
      //printf("\t->FOUND OTHER: %s %d\n",
      //         current->value->lexeme, current->type);
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

/* Analyze and restructures the statement node into an opcode call structure. T
 * This function will reform the tree such that the top node will contain the name of an opcode,
 * the ->left will hold out-args, and ->right will hold in-args.  This function does not try to
 * expand any statements or do any semantic verification, but reshapes trees so that they can all
 * go through the verify_opcode function.  This is due to the ambiguous nature of Csound opcode
 * call syntax.
 *
 * To note, this function requires that the typeTable be passed in. This is because variables
 * can (now) have names that shadow opcode names.  Lookup needs to give priority to an identifier
 * being a variable over being an opcode. This maintains future proofing so that if an opcode
 * is later introduced with the same name as a variable in an older project, the older project
 * will continue to work.
 *
 * For further reference, please see the rule for statement and opcall in Engine/csound_orc.y.
 */
TREE* convert_statement_to_opcall(CSOUND* csound, TREE* root, TYPE_TABLE* typeTable) {
  int32_t leftCount, rightCount;

  if (root->type == T_ASSIGNMENT) {
    /* Rewrite tree if line is "a1, a2 = func(arg, arg1)" to "a1, a2 func arg, arg1" */
    TREE *right = root->right;
    if (right->type == T_FUNCTION &&
        right->left == NULL &&
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

  //    print_tree(csound, "TEST\n", root);
  if (root->type != T_OPCALL) {
    synterr(csound,
            Str("Internal Error: convert_statement_to_opcall received a non T_OPCALL TREE\n"));
    return NULL;
  }

  if (root->value != NULL) {
    /* xout exp(0) */
    if (root->left != NULL &&
        root->left->type == T_IDENT &&
        find_opcode(csound, root->left->value->lexeme) != NULL) {
      TREE* top = root->left;
      root->left = NULL;
      top->right = root;
      top->next = root->next;
      root->next = NULL;
      top->type = T_OPCALL;
      top->right->type = T_FUNCTION;
      return top;
    }

    /* Already processed T_OPCALL, return as-is */

    return root;
  }

  if (root->left == NULL) {
    synterr(csound,
            Str("Internal Error: convert_statement_to_opcall received an empty OPCALL\n"));
    return NULL;
  }

  if (root->left->type == T_OPCALL && root->right == NULL) {

    TREE* top = root->left;
    TREE* unary_op = get_initial_unary_operator(top->right);

    if (top->left->next == NULL && unary_op != NULL) {
      TREE* newTop;

      /* i.e. ksubst init -1 */
      /* TODO - this should check if it's a var first */
      if (find_opcode(csound, top->value->lexeme) != NULL) {
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
        unary_op_parent->left = convert_unary_op_to_binary(csound, top, unary_op);
      }
      top->right = top->left = top->next = NULL;

      return newTop;
    }

    /* i.e. asig oscil 0.25, 440 */
    top->next = root->next;
    return top;

  } else if(root->right == NULL) {
    /* this branch catches this part of opcall rule: out_arg_list '(' ')' NEWLINE */

    if (tree_arg_list_count(root->left) != 1) {
      synterr(csound,
              Str("Internal Error: convert_statement_to_opcall received invalid OPCALL\n"));
    }
    root->left->next = root->next;
    root->left->type = T_OPCALL;
    return root->left;
  }

  if (root->right == NULL) {
    synterr(csound,
            Str("Internal Error: convert_statement_to_opcall received invalid OPCALL\n"));
    return NULL;
  }

  /* Now need to disambiguate the rule : out_arg_list expr_list NEWLINE */

  leftCount = tree_arg_list_count(root->left);
  rightCount = tree_arg_list_count(root->right);

  if (leftCount > 1 && rightCount > 1) {
    synterr(csound,
            Str("Internal Error: convert_statement_to_opcall received invalid OPCALL\n"));
    return NULL;
  }

  //    printf("ARG COUNTS: %d %d\n", leftCount, rightCount);

  if (leftCount == 1 && rightCount == 1) {
    TREE* newTop;
    if(root->right->type == T_IDENT && find_opcode(csound, root->right->value->lexeme) != NULL) {
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

  //    print_tree(csound, "Verifying Opcode: Left\n", root->left);
  //    print_tree(csound, "Verifying Opcode: Right\n", root->right);
  add_args(csound, root->left, typeTable);

  opcodeName = root->value->lexeme;
  //printf("%p %p (%s)\n", root, root->value, opcodeName);
  leftArgString = get_arg_string_from_tree(csound, left, typeTable);
  rightArgString = get_arg_string_from_tree(csound, right, typeTable);

  OENTRIES* entries = find_opcode2(csound, opcodeName);
  if (UNLIKELY(entries == NULL || entries->count == 0)) {
    synterr(csound, Str("Unable to find opcode with name: %s\n"),
            root->value->lexeme);
    if (entries != NULL) {
      csound->Free(csound, entries);
    }
    return 0;
  }

  OENTRY* oentry;
  if (root->value->optype == NULL)
    oentry = resolve_opcode(csound, entries,
                            leftArgString, rightArgString);
  /* if there is type annotation, try to resolve it */
  else oentry = resolve_opcode(csound, entries,
                               root->value->optype, rightArgString);


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
      csoundMessage(csound, "  %s %s %s\n", entry->outypes, entry->opname, entry->intypes);
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
    //fprintf(stderr, "left=%p\n", left);
    //fprintf(stderr, "left->value=%p\n", left->value);
    //fprintf(stderr, "left->value->lexeme=%p\n", left->value->lexeme);
    //fprintf(stderr, "opname = %s\n", oentry->opname);
    if (csound->oparms->sampleAccurate &&
        (strcmp(oentry->opname, "=.a")==0) &&
        (left!=NULL) && (left->value!=NULL) &&
        (left->value->lexeme[0]=='a')) { /* Deal with sample accurate assigns */
      int32_t i = 0;
      while (strcmp(entries->entries[i]->opname, "=.l")) {
        //printf("not %d %s\n",i, entries->entries[i]->opname);
        i++;
      }
      oentry = entries->entries[i];
    }
    else {
      if (csound->oparms->sampleAccurate &&
          (strcmp(oentry->opname, "=._")==0) &&
          (left->value->lexeme[0]=='a'))
        {
          int32_t i = 0;
          while (strcmp(entries->entries[i]->opname, "=.L")) {
            //printf("not %d %s\n",i, entries->entries[i]->opname);
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

    //TREE *tempLeft;
    //TREE *tempRight;
    TREE* current = root;

    while (current != NULL) {
      //tempLeft = current->left;
      //tempRight = current->right;

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

int32_t verify_until_statement(CSOUND* csound, TREE* root, TYPE_TABLE* typeTable) {
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

typedef struct initstructvar {
  OPDS h;
  MYFLT* out;
  MYFLT* inArgs[128];
} INIT_STRUCT_VAR;

int32_t initStructVar(CSOUND* csound, void* p) {
  INIT_STRUCT_VAR* init = (INIT_STRUCT_VAR*)p;
  CS_STRUCT_VAR* structVar = (CS_STRUCT_VAR*)init->out;
  CS_TYPE* type = csoundGetTypeForArg(init->out);
  int32_t len = cs_cons_length(type->members);
  int32_t i;

  //    csound->Message(csound, "Initializing Struct...\n");
  //    csound->Message(csound, "Struct Type: %s\n", type->varTypeName);

  for (i = 0; i < len; i++) {
    CS_VAR_MEM* mem = structVar->members[i];
    mem->varType->copyValue(csound, mem->varType, &mem->value,
                            init->inArgs[i], NULL);
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

  //    csound->Message(csound, "Initializing Struct...\n");
  //    csound->Message(csound, "Struct Type: %s\n", type->varTypeName);

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

CS_VARIABLE* createStructVar(void* cs, void* p, OPDS *ctx) {
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

  //FIXME - implement
  return var;
}

void copyStructVar(CSOUND* csound, const CS_TYPE* structType, void* dest, const
                   void* src, OPDS *p) {
  CS_STRUCT_VAR* varDest = (CS_STRUCT_VAR*)dest;
  CS_STRUCT_VAR* varSrc = (CS_STRUCT_VAR*)src;
  int32_t i, count;

  count = cs_cons_length(structType->members);
  for (i = 0; i < count; i++) {
    CS_VAR_MEM* d = varDest->members[i];
    CS_VAR_MEM* s = varSrc->members[i];
    d->varType->copyValue(csound, d->varType, &d->value, &s->value, NULL);
  }
}


int32_t add_struct_definition(CSOUND* csound, TREE* structDefTree) {
  CS_TYPE* type = csound->Calloc(csound, sizeof(CS_TYPE));
  TREE* current = structDefTree->right;
  int32_t index = 0;
  char temp[256];

  type->varTypeName = cs_strdup(csound, structDefTree->left->value->lexeme);
  type->varDescription = "user-defined struct";
  type->argtype = CS_ARG_TYPE_BOTH;
  type->createVariable = createStructVar;
  type->copyValue = copyStructVar;
  type->userDefinedType = 1;

  // FIXME: Values are appended in reverse order of definition
  while (current != NULL) {
    char* memberName = current->value->lexeme;
    char* typedIdentArg = current->value->optype;

    if (typedIdentArg == NULL) {
      typedIdentArg = cs_strndup(csound, memberName, 1);
    }

    memberName = cs_strdup(csound, memberName);
    const CS_TYPE* memberType =
      csoundGetTypeWithVarTypeName(csound->typePool, typedIdentArg);
    CS_VARIABLE* var = memberType->createVariable(csound, type, NULL);
    var->varName = cs_strdup(csound, memberName);
    var->varType = memberType;

    //        csound->Message(csound, "Member Found: %s : %s\n", memBase, typedIdentArg);

    CONS_CELL* member = csound->Calloc(csound, sizeof(CONS_CELL));
   member->value = var;
    type->members = cs_cons_append(type->members, member);
    current = current->next;
  }

  if(!csoundAddVariableType(csound, csound->typePool, type)) {
    return 0;
  }

  OENTRY oentry;
  memset(temp, 0, 256);
  cs_sprintf(temp, "init.%s", type->varTypeName);
  oentry.opname = cs_strdup(csound, temp);
  oentry.dsblksiz = sizeof(INIT_STRUCT_VAR);
  oentry.flags = 0;
  oentry.init = initStructVar;
  oentry.perf = NULL;
  oentry.deinit = NULL;
  oentry.useropinfo = NULL;

  /* FIXME - this is not yet implemented */
  memset(temp, 0, 256);
  cs_sprintf(temp, ":%s;", type->varTypeName);
  oentry.outypes = cs_strdup(csound, temp);

  CONS_CELL* member = type->members;
  while (member != NULL) {
    char* memberTypeName = ((CS_VARIABLE*)member->value)->varType->varTypeName;
    int32_t len = (int32_t) strlen(memberTypeName);

    if (len == 1) {
      temp[index++] = *memberTypeName;
    } else {
      temp[index++] = ':';
      memcpy(temp + index, memberTypeName, len);
      index += len;
      temp[index++] = ';';
    }

    member = member->next;
  }
  temp[index] = 0;
  oentry.intypes = cs_strdup(csound, temp);

  csoundAppendOpcodes(csound, &oentry, 1);
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


  CONS_CELL* parentLabelList = typeTable->labelList;
  typeTable->labelList = get_label_list(csound, root);

  //if (root->value)
  //printf("###verify %p %p (%s)\n", root, root->value, root->value->lexeme);

  if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "Verifying AST\n");

  while (current != NULL) {
    switch(current->type) {
    case STRUCT_TOKEN:
      if (PARSER_DEBUG) csound->Message(csound, "Struct definition found\n");
      //        csound->Message(csound, "%s: ", current->left->value->lexeme);
      //        TREE* args = current->right;
      //        while (args != NULL) {
      //          csound->Message(csound, "%s ", args->value->lexeme);
      //          args = args->next;
      //        }
      //        csound->Message(csound, "\n");
      if(!add_struct_definition(csound, current)) {
        csound->ErrorMsg(csound,
                         "Error: Unable to define new struct type: %s\n",
                         current->left->value->lexeme);
        return NULL;
      }
      break;
    case INSTR_TOKEN:
      csound->inZero = 0;
      if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "Instrument found\n");
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
      if (PARSER_DEBUG) csound->Message(csound, "UDO found\n");

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
        //            printf(">>> NEW STYLE UDO FOUND <<<\n");
        if(current->left->right != NULL && *current->left->right->value->lexeme != '0') {
          add_args(csound, current->left->right, typeTable);
        }
        char* outArgString = get_out_types_from_tree(csound, current->left->left);
        char* inArgString = get_in_types_from_tree(csound, current->left->right, typeTable);
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
      if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "UDO found\n");

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
      char* outArgStringDecl = get_out_types_from_tree(csound, current->left->left);
      char* inArgStringDecl = get_in_types_from_tree(csound, current->left->right, typeTable);
      add_udo_definition(csound, false, current->value->lexeme, inArgStringDecl, outArgStringDecl, UNDEFINED);
      csound->inZero = 0;
      if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "UDO found\n");

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
        return 0;
      }

      current = expand_if_statement(csound, current, typeTable);

      if (previous != NULL) {
        previous->next = current;
      }

      continue;

    case UNTIL_TOKEN:
    case WHILE_TOKEN:
      if (!verify_until_statement(csound, current, typeTable)) {
        return 0;
      }

      current = expand_until_statement(csound, current,
                                       typeTable, current->type==WHILE_TOKEN);

      if (previous != NULL) {
        previous->next = current;
      }

      continue;
    case FOR_TOKEN: {
      char* arrayArgType = get_arg_type2(csound, current->right->left, typeTable);
      char* assignmentSymbol = current->left->value->lexeme;

      if (*arrayArgType != '[') {
        synterr(
          csound,
          Str("Line: %d invalid array argument in for-of statement: found '%s'\n"),
          current->line,
          current->right->left->value->lexeme
        );
        return 0;
      }

      if (arrayArgType[1] != *assignmentSymbol) {
        synterr(
          csound,
          Str("Line: %d mismatching argument types in for-of statement!\n"
              "'%s' runs on different rate from '%s'\n"),
          current->line,
          assignmentSymbol,
          current->right->left->value->lexeme
        );
        return 0;
      }

      current = expand_for_statement(
        csound,
        current,
        typeTable,
        arrayArgType
      );

      if (previous != NULL) {
        previous->next = current;
      }
    }
      continue;
    case LABEL_TOKEN:
      break;

    case '+':
    case '-':
    case '*':
    case '/':
      //printf("Folding case?\n");
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
        csound->Free(csound, current->left); csound->Free(csound, current->right);
      }
      break;
    case ENDIN_TOKEN:
    case UDOEND_TOKEN:
      csound->inZero = 1;
      /* fall through */
    default:
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
      //print_tree(csound, "verify_tree", current);
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

  if (PARSER_DEBUG) csound->Message(csound, "[End Verifying AST]\n");

  cs_cons_free(csound, typeTable->labelList);
  typeTable->labelList = parentLabelList;

  return anchor;
}


/* BISON PARSER FUNCTION */
int32_t csound_orcwrap(void* dummy)
{
  IGN(dummy);
  //#ifdef DEBUG
  //printf("\n === END OF INPUT ===\n");
  //#endif
  return (1);
}

/* UTILITY FUNCTIONS */

extern int32_t csound_orcget_lineno(void*);
extern char *csound_orcget_current_pointer(void *);
/* BISON PARSER FUNCTION */
void csound_orcerror(PARSE_PARM *pp, void *yyscanner,
                     CSOUND *csound, TREE **astTree, const char *str)
{

  IGN(pp);
  IGN(astTree);
  char ch;
  char *p = csound_orcget_current_pointer(yyscanner)-1;
  int32_t line = csound_orcget_lineno(yyscanner);
  uint64_t files = csound_orcget_locn(yyscanner);
  if (UNLIKELY(*p=='\0' || *p=='\n')) line--;
  //printf("LINE: %d\n", line);

  csound->ErrorMsg(csound, Str("\nerror: %s  (token \"%s\")"),
                  str, csound_orcget_text(yyscanner));
  do_baktrace(csound, files);
  csound->ErrorMsg(csound, Str(" line %d:\n>>>"), line);
  while ((ch=*--p) != '\n' && ch != '\0');
  do {
    ch = *++p;
    if (UNLIKELY(ch == '\n')) break;
    // Now get rid of any continuations
    if (ch=='#' && strncmp(p,"sline ",6)) {
      p+=7; while (isdigit(*p)) p++;
    }
    else csound->ErrorMsg(csound, "%c", ch);
  } while (ch != '\n' && ch != '\0');
  csound->ErrorMsg(csound, " <<<\n");


}

void do_baktrace(CSOUND *csound, uint64_t files)
{
  while (files) {
    uint32_t ff = files&0xff;
    files = files >>8;
    csound->ErrorMsg(csound, Str(" from file %s (%d)\n"),
                    csound->filedir[ff], ff);
  }

}

/**
 * Appends TREE * node to TREE * node using ->next field in struct; walks
 * down  list to append at end; checks for NULL's and returns
 * appropriate nodes
 */
TREE* appendToTree(CSOUND * csound, TREE *first, TREE *newlast)
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
    ans = (TREE*)csound->Malloc(csound, sizeof(TREE));
    if (UNLIKELY(ans==NULL)) {
      /* fprintf(stderr, "Out of memory\n"); */
      exit(1);
    }
    ans->type = tree->type;
    ans->left = (tree->left == NULL) ? NULL : copy_node(csound, tree->left);
    ans->right = (tree->right == NULL) ? NULL : copy_node(csound, tree->right);
    if (tree->value != NULL) {
      ans->value = make_token(csound, tree->value->lexeme);
      ans->value->optype = cs_strdup(csound, tree->value->optype);
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

TREE* make_node(CSOUND *csound, int32_t line, uint64_t locn, int32_t type,
                TREE* left, TREE* right)
{
  TREE *ans;
  ans = (TREE*)csound->Malloc(csound, sizeof(TREE));
  if (UNLIKELY(ans==NULL)) {
    /* fprintf(stderr, "Out of memory\n"); */
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
  //printf("make node %p %p %p\n", ans, ans->left, ans->right);
  //csound->DebugMsg(csound, "%s(%d) line = %d\n", __FILE__, __LINE__, line);
  return ans;
}

TREE* make_leaf(CSOUND *csound, int32_t line, uint64_t locn, int32_t type, ORCTOKEN *v)
{
  TREE *ans;
  ans = (TREE*)csound->Calloc(csound, sizeof(TREE));
  if (UNLIKELY(ans==NULL)) {
    /* fprintf(stderr, "Out of memory\n"); */
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
  //if (ans->value)
  //printf("make leaf %p %p (%s)\n", ans, ans->value, ans->value->lexeme);
  csound->DebugMsg(csound, "csound_orc_semantics(%d) line = %d\n",
                   __LINE__, line);
  return ans;
}

TREE* make_opcall_from_func_start(CSOUND *csound, int32_t line, uint64_t locn, int32_t type,
                                  TREE* left, TREE* right) {
  TREE* firstArg = left->right;
  TREE* first = right;
  TREE* rest = right->next;

  right->next = NULL;

  TREE* operatorNode = make_node(csound, line, locn, type, firstArg, first);
  operatorNode->next = rest;
  left->right = operatorNode;

  return left;
}

void delete_tree(CSOUND *csound, TREE *l)
{
  while (1) {
    TREE *old = l;

    if (UNLIKELY(l==NULL)) {
      return;
    } //else printf("l = %p\n", l);

    if (l->value) {
      if (l->value->lexeme) {
        //printf("Free %p %p (%s)\n", l, l->value, l->value->lexeme);
        csound->Free(csound, l->value->lexeme);
        //l->value->lexeme = NULL;
      }
      //printf("Free val %p\n", l->value);
      csound->Free(csound, l->value);
      //l->value = NULL;
    }
    // printf("left %p right %p\n", l->left, l->right);
    delete_tree(csound, l->left);
    //l->left = NULL;
    delete_tree(csound, l->right);
    //l->right = NULL;
    l = l->next;
    //printf("Free old %p next: %p\n", old, l);
    csound->Free(csound, old);
  }
}

PUBLIC void csoundDeleteTree(CSOUND *csound, TREE *tree)
{
  //printf("Tree %p\n", tree);
  delete_tree(csound, tree);
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
  /* case T_OPCALL: */
  /*   csound->Message(csound,"T_OPCALL: %s\n", l->value->lexeme); break; */
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
  case T_ASSIGNMENT:
  case '|':
  case '&':
  case '#':
  case '~':
    csound->Message(csound,"name=\"%c\"", l->type); break;
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
    if (l->left)
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
    //    case T_MAPI:
    //      csound->Message(csound,"name=\"T_MAPI\""); break;
    //    case T_MAPK:
    //      csound->Message(csound,"name=\"T_MAPK\""); break;
    //    case T_TADD:
    //      csound->Message(csound,"name=\"T_TADD\""); break;
    //    case T_SUB:
    //      csound->Message(csound,"name=\"T_SUB\""); break;
    //    case S_TUMINUS:
    //      csound->Message(csound,"name=\"S_TUMINUS\""); break;
    //    case T_TMUL:
    //      csound->Message(csound,"name=\"T_TMUL\""); break;
    //    case T_TDIV:
    //      csound->Message(csound,"name=\"T_TDIV\""); break;
    //    case T_TREM:
    //      csound->Message(csound,"name=\"T_TREM\""); break;
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
      csoundErrorMsg(csound, "THIS SHOULD NOT HAPPEN -- ep NULL csound_orc-semantics(%d)\n",
             __LINE__);
    }
    if (ep->intypes != NULL) {
      nreqd = argsRequired(ep->intypes);
      inArgParts = splitArgs(csound, ep->intypes);
    }

    if (UNLIKELY(PARSER_DEBUG)) {
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
                           make_int(csound, "0"));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'P':
        case 'p':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "1"));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'q':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "10"));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;

        case 'V':
        case 'v':
          temp = make_leaf(csound, l->line, l->locn, NUMBER_TOKEN,
                           make_num(csound, ".5"));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'h':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "127"));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'J':
        case 'j':
          temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                           make_int(csound, "-1"));
          temp->markup = &SYNTHESIZED_ARG;
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
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
    //      printf("delete %p\n", inArgParts);
    if (inArgParts != NULL) {
      int32_t n;
      for (n=0; inArgParts[n] != NULL; n++) {
        //printf("delete %p\n", inArgParts[n]);
        csound->Free(csound, inArgParts[n]);
      }
      csound->Free(csound, inArgParts);
    }
  }
}

char tree_argtyp(CSOUND *csound, TREE *tree) {
  IGN(csound);
  if (tree->type == INTEGER_TOKEN || tree->type == NUMBER_TOKEN) {
    return 'i';
  }

  return argtyp2( tree->value->lexeme);
}
