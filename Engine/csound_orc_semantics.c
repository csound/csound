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
#include "namedins.h"
#include "parse_param.h"
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include "csound_orc_expressions.h"
#include "csound_orc_semantics.h"

extern char *csound_orcget_text ( void *scanner );
static int is_label(char* ident, CONS_CELL* labelList);

extern uint64_t csound_orcget_locn(void *);
extern  char argtyp2(char*);
extern  int tree_arg_list_count(TREE *);
void print_tree(CSOUND *, char *, TREE *);

/* from csound_orc_compile.c */
extern int argsRequired(char* arrayName);
extern char** splitArgs(CSOUND* csound, char* argString);
extern int pnum(char*);

OENTRIES* find_opcode2(CSOUND*, char*);
char* resolve_opcode_get_outarg(CSOUND* csound,
                                OENTRIES* entries, char* inArgTypes);
int check_out_args(CSOUND* csound, char* outArgsFound, char* opOutArgs);
char* get_arg_string_from_tree(CSOUND* csound, TREE* tree,
                               TYPE_TABLE* typeTable);
char* convert_internal_to_external(CSOUND* csound, char* arg);
char* convert_external_to_internal(CSOUND* csound, char* arg);
void do_baktrace(CSOUND *csound, uint64_t files);

const char* SYNTHESIZED_ARG = "_synthesized";

char* cs_strdup(CSOUND* csound, char* str) {
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

char* cs_strndup(CSOUND* csound, char* str, size_t size) {
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

    int i, len = arrayVar->dimensions + 3;
    if (arrayVar->subType!=NULL) {
      char* retVal = csound->Malloc(csound, len);
      retVal[len - 1] = '\0';
      retVal[len - 2] = ']';
      retVal[len - 3] = *arrayVar->subType->varTypeName;
      for (i = len - 4; i >= 0; i--) {
        retVal[i] = '[';
      }
      return retVal;
    }
    else
      return NULL;
}

/* this checks if the annotated type exists */
char *check_annotated_type(CSOUND* csound, OENTRIES* entries,
                           char* outArgTypes) {
    int i;
    for (i = 0; i < entries->count; i++) {
      OENTRY* temp = entries->entries[i];
      if (check_out_args(csound, outArgTypes, temp->outypes))
        return outArgTypes;
    }
    return NULL;
}

static int isirate(/*CSOUND *csound,*/ TREE *t)
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

/* This function gets arg type with checking type table */
char* get_arg_type2(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable)
{
    char* s;
    char* t;
    //CS_TYPE* type;
    CS_VARIABLE* var;

    if (is_expression_node(tree)) {
      TREE* nodeToCheck = tree;

      if (tree->type == T_ARRAY) {
        char* leftArgType = get_arg_type2(csound, tree->left, typeTable);

        //FIXME - should use CS_TYPE and not interrogate string
        if (leftArgType[0] == '[') {
          return get_array_sub_type(csound, tree->left->value->lexeme);
        }
        else {
          char* rightArgType = get_arg_string_from_tree(csound, tree->right,
                                                        typeTable);

          leftArgType =
            csound->ReAlloc(csound, leftArgType,
                            strlen(leftArgType) + strlen(rightArgType) + 1);
          char* argString = strcat(leftArgType, rightArgType);

          OENTRIES* opentries = find_opcode2(csound, "##array_get");
          char* outype = resolve_opcode_get_outarg(csound,
                                                   opentries,
                                                   argString);

          csound->Free(csound, opentries);
          if (UNLIKELY(outype == NULL)) {
            synterr(csound,
                    Str("unable to find array operator for "
                        "types %s line %d\n"),
                    argString, tree->line);
            do_baktrace(csound, tree->locn);
            return NULL;
          }

          csound->Free(csound, leftArgType);
          csound->Free(csound, rightArgType);
          return cs_strdup(csound, outype);
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
          if (UNLIKELY(tree->right->type != LABEL_TOKEN))
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
          return NULL;
        }
        csound->Free(csound, argTypeRight);
        csound->Free(csound, entries);
        return cs_strdup(csound, out);

      }

      char* argTypeRight = get_arg_type2(csound,
                                         nodeToCheck->right, typeTable);

      if (nodeToCheck->left != NULL) {
        char* argTypeLeft = get_arg_type2(csound, nodeToCheck->left, typeTable);

        char* opname = get_expression_opcode_type(csound, nodeToCheck);
        int len1, len2;
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

        len1 = strlen(argTypeLeft);
        len2 = strlen(argTypeRight);
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
        int len1, len2;
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

        len1 = strlen(argTypeLeft);
        len2 = strlen(argTypeRight);
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
    case SRATE_TOKEN:
    case KRATE_TOKEN:
    case KSMPS_TOKEN:
    case A4_TOKEN:
    case ZERODBFS_TOKEN:
    case NCHNLS_TOKEN:
    case NCHNLSI_TOKEN:
      return cs_strdup(csound, "r");                              /* rsvd */
    case LABEL_TOKEN:
      //FIXME: Need to review why label token is used so much in parser,
      //for now treat as T_IDENT
    case T_ARRAY_IDENT:
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

      if (is_label(s, typeTable->labelList)) {
        return cs_strdup(csound, "l");
      }

      if (*s == 't') { /* Support legacy t-vars by mapping to k-array */
        return cs_strdup(csound, "[k]");
      }

      if ((*s >= '1' && *s <= '9') || *s == '.' || *s == '-' || *s == '+' ||
          (*s == '0' && strcmp(s, "0dbfs") != 0))
        return cs_strdup(csound, "c");                          /* const */
      if (*s == '"')
        return cs_strdup(csound, "S");

      if (pnum(s) >= 0)
        return cs_strdup(csound, "p");                           /* pnum */

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

      if (*s == 'g') {
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
                            "Line %d\n"),
                tree->value->lexeme, tree->line-1); /* -1 as read next line! */
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


    case T_ARRAY:

      s = tree->value->lexeme;

      if (*s == '#') s++;
      if (*s == 'g') s++;

      if (*s == 't') { /* Support legacy t-vars by mapping to k-array */
        return cs_strdup(csound, "[k]");
      }

      t = s;

      int len = 1;
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
      int opLen = dot - opname;
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

static OENTRIES* get_entries(CSOUND* csound, int count)
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
    int i = 0;
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

inline static int is_in_optional_arg(char arg) {
    return (strchr("opqvjhOJVP?", arg) != NULL);
}

inline static int is_in_var_arg(char arg) {
    return (strchr("mMNnWyzZ*", arg) != NULL);
}

int check_array_arg(char* found, char* required) {
    char* f = found;
    char* r = required;

    while (*r == '[') r++;

    if (*r == '.' || *r == '?' || *r == '*') {
      return 1;
    }

    while (*f == '[') f++;

    return (*f == *r);
}

int check_in_arg(char* found, char* required) {
    char* t;
    int i;
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

    if (is_in_optional_arg(*required)) {
      t = (char*)OPTIONAL_IN_TYPES[0];
      for (i = 0; t != NULL; i += 2) {
        if (strcmp(required, t) == 0) {
          return (strchr(OPTIONAL_IN_TYPES[i + 1], *found) != NULL);
        }
        t = (char*)OPTIONAL_IN_TYPES[i + 2];
      }
    }

    if (!is_in_var_arg(*required)) {
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

int check_in_args(CSOUND* csound, char* inArgsFound, char* opInArgs) {
    if ((inArgsFound == NULL || strlen(inArgsFound) == 0) &&
        (opInArgs == NULL || strlen(opInArgs) == 0)) {
      return 1;
    }

    if (UNLIKELY(opInArgs == NULL)) {
      return 0;
    }

    {
      int argsFoundCount = argsRequired(inArgsFound);
      int argsRequiredCount = argsRequired(opInArgs);
      char** argsRequired = splitArgs(csound, opInArgs);
      char** argsFound;
      int i;
      int argTypeIndex = 0;
      char* varArg = NULL;
      int returnVal = 1;

      if (argsRequired == NULL) {
        return 0;
      }
      if (argsFoundCount>=VARGMAX) {
        return -1;
      }

      if ((argsFoundCount > argsRequiredCount) &&
          !(is_in_var_arg(*argsRequired[argsRequiredCount - 1]))) {
        csound->Free(csound, argsRequired);
        return 0;
      }

      argsFound = splitArgs(csound, inArgsFound);

      if (argsFoundCount == 0) {
        if (is_in_var_arg(*argsRequired[0])) {
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
            if (is_in_var_arg(*argRequired)) {
              varArg = argRequired;
            }
          }
        }
      }

      if (returnVal && varArg == NULL) {
        while (argTypeIndex < argsRequiredCount) {
          char c = *argsRequired[argTypeIndex++];

          if (!is_in_optional_arg(c) && !is_in_var_arg(c)) {
            returnVal = 0;
            break;
          }
        }

      }
      //printf("delete %p\n", argsFound);
      int n;
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

inline static int is_out_var_arg(char arg) {
    return (strchr("mzIXNF*", arg) != NULL);
}

int check_out_arg(char* found, char* required) {
    char* t;
    int i;

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

    if (!is_out_var_arg(*required)) {
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

int check_out_args(CSOUND* csound, char* outArgsFound, char* opOutArgs)
{

    if ((outArgsFound == NULL || strlen(outArgsFound) == 0) &&
        (opOutArgs == NULL || strlen(opOutArgs) == 0)) {
      return 1;
    }

    {
      int argsFoundCount = argsRequired(outArgsFound);
      int argsRequiredCount = argsRequired(opOutArgs);
      char** argsRequired = splitArgs(csound, opOutArgs);
      char** argsFound;
      int i;
      int argTypeIndex = 0;
      char* varArg = NULL;
      int returnVal = 1;

      if ((argsFoundCount > argsRequiredCount) &&
          !(is_out_var_arg(*argsRequired[argsRequiredCount - 1]))) {
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
          if (is_out_var_arg(*argRequired)) {
            varArg = argRequired;
          }
        }
      }

      if (returnVal && varArg == NULL) {

        if (argTypeIndex < argsRequiredCount) {
          char* argRequired = argsRequired[argTypeIndex];
          returnVal = is_out_var_arg(*argRequired);
        } else {
          returnVal = 1;
        }
      }
      //printf("delete %p\n", argsFound);
      int n;
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
  int i, check;

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
    int i;

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
    int i;

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
    int i, dimensions;
    char* retVal;

    if (arg == NULL || *arg != '[') {
      return arg;
    }

    dimensions = 0;
    while (*arg == '[') {
      arg++;
      dimensions++;
    }

    retVal = csound->Malloc(csound, sizeof(char) * ((dimensions * 2) + 2));
    retVal[0] = *arg;
    for (i = 0; i < dimensions * 2; i += 2) {
      retVal[i + 1] = '[';
      retVal[i + 2] = ']';
    }
    retVal[dimensions * 2 + 1] = '\0';
    //csound->Free(csound, arg);
    return retVal;
}

/* ASSUMES VALID ARRAY SPECIFIER! */
char* convert_external_to_internal(CSOUND* csound, char* arg) {
    int i, dimensions;
    char* retVal;

    if (arg == NULL || *(arg + 1) != '[') {
      return arg;
    }

    dimensions = (strlen(arg) - 1) / 2;

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

    int len = tree_arg_list_count(tree);
    int i;

    if (len == 0) {
      return NULL;
    }

    char** argTypes = csound->Malloc(csound, len * sizeof(char*));
    char* argString = NULL;
    TREE* current = tree;
    int index = 0;
    int argsLen = 0;

    while (current != NULL) {
      char* argType = get_arg_type2(csound, current, typeTable);

      //FIXME - fix if argType is NULL and remove the below hack
      if (argType == NULL) {
        argsLen += 1;
        argTypes[index++] = cs_strdup(csound, "@");
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
      int size = strlen(argTypes[i]);
      memcpy(temp, argTypes[i], size);
      temp += size;
      csound->Free(csound, argTypes[i]);
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
int check_args_exist(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable) {
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
              Str("Variable type for %s could not be determined.\n"), varName);
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

void add_arg(CSOUND* csound, char* varName, TYPE_TABLE* typeTable) {

    CS_TYPE* type;
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
      t = varName;
      argLetter[1] = 0;

      if (*t == '#') t++;
      if (*t == 'g') t++;

      if (*t == '[' || *t == 't') { /* Support legacy t-vars */
        int dimensions = 1;
        CS_TYPE* varType;
        char* b = t + 1;

        while (*b == '[') {
          b++;
          dimensions++;
        }
        argLetter[0] = (*b == 't') ? 'k' : *b; /* Support legacy t-vars */

        varType = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);

        varInit.dimensions = dimensions;
        varInit.type = varType;
        typeArg = &varInit;
      }

      argLetter[0] = (*t == 't') ? '[' : *t; /* Support legacy t-vars */

      type = csoundGetTypeForVarName(csound->typePool, argLetter);

      var = csoundCreateVariable(csound, csound->typePool,
                                 type, varName, typeArg);
      csoundAddVariable(csound, pool, var);
    } else {
      //TODO - implement reference count increment
    }

}

void add_array_arg(CSOUND* csound, char* varName, int dimensions,
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
      CS_TYPE* varType;

      t = varName;
      argLetter[1] = 0;

      if (*t == '#') t++;
      if (*t == 'g') t++;

      argLetter[0] = (*t == 't') ? 'k' : *t; /* Support legacy t-vars */

      varType = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);

      varInit.dimensions = dimensions;
      varInit.type = varType;
      typeArg = &varInit;

      var = csoundCreateVariable(csound, csound->typePool,
                                 (CS_TYPE*) &CS_VAR_TYPE_ARRAY,
                                 varName, typeArg);
      csoundAddVariable(csound, pool, var);
    } else {
      //TODO - implement reference count increment
    }
}

/* return 1 on succcess, 0 on failure */
int add_args(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable)
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
        add_array_arg(csound, varName,
                      tree_arg_list_count(current->right), typeTable);

        break;

      case LABEL_TOKEN:
      case T_IDENT:
        varName = current->value->lexeme;

        if (UNLIKELY(*varName == 't')) { /* Support legacy t-vars */
          add_array_arg(csound, varName, 1, typeTable);
        } else {
          add_arg(csound, varName, typeTable);
        }

        break;

      case T_ARRAY:
        varName = current->left->value->lexeme;
        // FIXME - this needs to work for array and a-names
        add_arg(csound, varName, typeTable);
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


/*
 * Verifies:
 *    -number of args correct
 *    -types of arg correct
 *    -expressions are valid and types correct
 */
int verify_opcode(CSOUND* csound, TREE* root, TYPE_TABLE* typeTable) {

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
    //printf("%p %p (%s)\n", root, root->value, opcodeName);
    leftArgString = get_arg_string_from_tree(csound, left, typeTable);
    rightArgString = get_arg_string_from_tree(csound, right, typeTable);

    if (!strcmp(opcodeName, "xin")) {
      int nreqd = tree_arg_list_count(root->right);

      if (nreqd > OPCODENUMOUTS_LOW) {
        opcodeName = (nreqd > OPCODENUMOUTS_HIGH) ? "##xin256" : "##xin64";
      }
    }

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
      synterr(csound, Str("Unable to find opcode entry for \'%s\' "
                          "with matching argument types:\n"),
              opcodeName);
      csoundMessage(csound, Str("Found: %s %s %s\n"),
                    leftArgString, root->value->lexeme, rightArgString);
      if (root->left && root->left->value && root->right && root->right->value)
      csoundMessage(csound, Str("       %s %s %s ...\n"),
                    root->left->value->lexeme, root->value->lexeme,
                    root->right->value->lexeme);
      csoundMessage(csound, Str("Line: %d\n"),
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
        int i = 0;
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
            int i = 0;
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

static int is_label(char* ident, CONS_CELL* labelList) {
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

int verify_if_statement(CSOUND* csound, TREE* root, TYPE_TABLE* typeTable) {

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

int verify_until_statement(CSOUND* csound, TREE* root, TYPE_TABLE* typeTable) {
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

/** Verifies if xin and xout statements are correct for UDO
   needs to check:
     xin/xout number of args matches UDO input/output arg specifications
     xin/xout statements exist if UDO in and out args are not 0 */
int verify_xin_xout(CSOUND *csound, TREE *udoTree, TYPE_TABLE *typeTable) {
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
    unsigned int i;

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
                Str("invalid xin statement for UDO: defined '%s', found '%s'\n"),
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


    CONS_CELL* parentLabelList = typeTable->labelList;
    typeTable->labelList = get_label_list(csound, root);

    if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "Verifying AST\n");

    while (current != NULL) {
      switch(current->type) {
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
        csound->inZero = 0;
        if (UNLIKELY(PARSER_DEBUG))
          csound->Message(csound, "UDO found\n");

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

            if (!verify_xin_xout(csound, current, typeTable)) {
              return 0;
            }

            newRight = NULL;
        }

        typeTable->localPool = typeTable->instr0LocalPool;

        break;

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
/* #ifdef JPFF */
/*         printf("***Expand until/while %d\n", csound->inZero); */
/* #endif */
        current = expand_until_statement(csound, current,
                                         typeTable, current->type==WHILE_TOKEN);
        //print_tree(csound, "until/while", current);
        if (previous != NULL) {
          previous->next = current;
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
        csound->inZero = 1;     /* ****Is this right??????? */
        /* fall through */
      default:
        if (!verify_opcode(csound, current, typeTable)) {
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
int csound_orcwrap(void* dummy)
{
   IGN(dummy);
#ifdef DEBUG
    printf("\n === END OF INPUT ===\n");
#endif
    return (1);
}

/* UTILITY FUNCTIONS */

extern int csound_orcget_lineno(void*);
extern char *csound_orcget_current_pointer(void *);
/* BISON PARSER FUNCTION */
void csound_orcerror(PARSE_PARM *pp, void *yyscanner,
                     CSOUND *csound, TREE **astTree, const char *str)
{
    IGN(pp);
    IGN(astTree);
    char ch;
    char *p = csound_orcget_current_pointer(yyscanner)-1;
    int line = csound_orcget_lineno(yyscanner);
    uint64_t files = csound_orcget_locn(yyscanner);
    if (UNLIKELY(*p=='\0' || *p=='\n')) line--;
    //printf("LINE: %d\n", line);
    csoundErrorMsg(csound, Str("\nerror: %s  (token \"%s\")\n"),
                    str, csound_orcget_text(yyscanner));
    do_baktrace(csound, files);
    csoundErrorMsg(csound, Str(" line %d:\n >>> "), line);
    while ((ch=*--p) != '\n' && ch != '\0');
    do {
      ch = *++p;
      if (UNLIKELY(ch == '\n')) break;
      // Now get rid of any continuations
      if (ch=='#' && strncmp(p,"sline ",6)) {
        p+=7; while (isdigit(*p)) p++;
      }
      else {
          csoundErrorMsg(csound, "%c", ch);
      }
    } while (ch != '\n' && ch != '\0');
      csoundErrorMsg(csound, " <<<\n");
}

void do_baktrace(CSOUND *csound, uint64_t files)
{
    while (files) {
      unsigned int ff = files&0xff;
      files = files >>8;
      csoundErrorMsg(csound, Str(" from file %s (%d),"),
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
TREE* make_node(CSOUND *csound, int line, int locn, int type,
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

TREE* make_leaf(CSOUND *csound, int line, int locn, int type, ORCTOKEN *v)
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
    // printf("make leaf %p %p (%s)\n", ans, ans->value, ans->value->lexeme);
    csound->DebugMsg(csound, "csound_orc_semantics(%d) line = %d\n",
                     __LINE__, line);
    return ans;
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
void print_tree_i(CSOUND *csound, TREE *l, int n)
{
    int i;
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
    case SRATE_TOKEN:
      csound->Message(csound,"SRATE_TOKEN:(%d:%s)\n",
                      l->line, csound->filedir[(l->locn)&0xff]); break;
    case KRATE_TOKEN:
      csound->Message(csound,"KRATE_TOKEN:(%d:%s)\n",
                      l->line, csound->filedir[(l->locn)&0xff]); break;
    case ZERODBFS_TOKEN:
      csound->Message(csound,"ZERODFFS_TOKEN:(%d:%s)\n",
                      l->line, csound->filedir[(l->locn)&0xff]); break;
    case A4_TOKEN:
      csound->Message(csound,"A4_TOKEN:(%d:%s)\n",
                      l->line, csound->filedir[(l->locn)&0xff]); break;
    case KSMPS_TOKEN:
      csound->Message(csound,"KSMPS_TOKEN:(%d:%s)\n",
                      l->line, csound->filedir[(l->locn)&0xff]); break;
    case NCHNLS_TOKEN:
      csound->Message(csound,"NCHNLS_TOKEN:(%d:%s)\n",
                      l->line, csound->filedir[(l->locn)&0xff]); break;
    case NCHNLSI_TOKEN:
      csound->Message(csound,"NCHNLSI_TOKEN:(%d:%s)\n",
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
    case T_OPCODE0:
      csound->Message(csound,"T_OPCODE0: %s\n",
                      l->value->lexeme); break;
    case T_OPCODE:
      csound->Message(csound,"T_OPCODE: %s\n",
                      l->value->lexeme); break;
    case T_FUNCTION:
      csound->Message(csound,"T_FUNCTION: %s\n",
                      l->value->lexeme); break;
    case S_UMINUS:
      csound->Message(csound,"S_UMINUS:(%d:%s)\n",
                      l->line, csound->filedir[(l->locn)&0xff]); break;
    case T_INSTLIST:
      csound->Message(csound,"T_INSTLIST:(%d:%s)\n",
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
static void print_tree_xml(CSOUND *csound, TREE *l, int n, int which)
{
    int i;
    char *child[4] = {"", "left", "right", "next"};
    if (l==NULL) {
      return;
    }
    for (i=0; i<n; i++) {
      csound->Message(csound, " ");
    }

    csound->Message(csound,
                    "<tree%s (%p : %p) type=\"%d\" ",
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
    case '=':
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
    case SRATE_TOKEN:
      csound->Message(csound,"name=\"SRATE_TOKEN\""); break;
    case KRATE_TOKEN:
      csound->Message(csound,"name=\"KRATE_TOKEN\""); break;
    case ZERODBFS_TOKEN:
      csound->Message(csound,"name=\"ZERODBFS_TOKEN\""); break;
    case A4_TOKEN:
      csound->Message(csound,"name=\"A4_TOKEN\""); break;
    case KSMPS_TOKEN:
      csound->Message(csound,"name=\"KSMPS_TOKEN\""); break;
    case NCHNLS_TOKEN:
      csound->Message(csound,"name=\"NCHNLS_TOKEN\""); break;
    case NCHNLSI_TOKEN:
      csound->Message(csound,"name=\"NCHNLSI_TOKEN\""); break;
    case INSTR_TOKEN:
      csound->Message(csound,"name=\"INSTR_TOKEN\""); break;
    case STRING_TOKEN:
      csound->Message(csound,"name=\"T_STRCONST\" str=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT:
      csound->Message(csound,"name=\"T_IDENT\" varname=\"%s\"",
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
    case T_OPCODE0:
      csound->Message(csound,"name=\"T_OPCODE0\" opname0=\"%s\"",
                      l->value->lexeme); break;
    case T_OPCODE:
      csound->Message(csound,"name=\"T_OPCODE\" opname=\"%s\"",
                      l->value->lexeme); break;
    case T_FUNCTION:
      csound->Message(csound,"name=\"T_FUNCTION\" fname=\"%s\"",
                      l->value->lexeme); break;
    case S_UMINUS:
      csound->Message(csound,"name=\"S_UMINUS\""); break;
    case T_INSTLIST:
      csound->Message(csound,"name=\"T_INSTLIST\""); break;
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
    default:
      csound->Message(csound,"name=\"unknown\"(%d)", l->type);
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
      int nreqd = 0;
      int incnt = tree_arg_list_count(l->right);
      TREE * temp;
      char** inArgParts = NULL;

      if (UNLIKELY(ep==NULL)) { /* **** FIXME **** */
        printf("THIS SHOULD NOT HAPPEN -- ep NULL csound_orc-semantics(%d)\n",
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
        int n;
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
