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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
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

char *csound_orcget_text ( void *scanner );
int is_label(char* ident, CONS_CELL* labelList);
int is_reserved(char* varName);

extern int csound_orcget_locn(void *);
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

extern int add_udo_definition(CSOUND *csound, char *opname,
                              char *outtypes, char *intypes);

const char* SYNTHESIZED_ARG = "_synthesized";
const char* UNARY_PLUS = "_unary_plus";

char* cs_strdup(CSOUND* csound, char* str) {
    size_t len;
    char* retVal;

    if (str == NULL) return NULL;

    len = strlen(str);
    retVal = csound->Malloc(csound, (len + 1) * sizeof(char));

    if (len > 0) memcpy(retVal, str, len * sizeof(char));//why not strcpy?
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

    retVal = csound->Malloc(csound, (size + 1) * sizeof(char));
    memcpy(retVal, str, size * sizeof(char));
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
    char* retVal = csound->Malloc(csound, len);
    retVal[len - 1] = '\0';
    retVal[len - 2] = ']';
    retVal[len - 3] = *arrayVar->subType->varTypeName;
    for (i = len - 4; i >= 0; i--) {
        retVal[i] = '[';
    }
    return retVal;
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
    char* brkt;
    char* varBaseName;

    if (is_expression_node(tree)) {
      TREE* nodeToCheck = tree;

      if (tree->type == T_ARRAY) {
        varBaseName = strtok_r(tree->left->value->lexeme, ":", &brkt);
        
        if (*varBaseName == 'g') {
          var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                           varBaseName);
          if(var == NULL)
            var = csoundFindVariableWithName(csound, typeTable->globalPool,
                                             varBaseName);
        } else
          var = csoundFindVariableWithName(csound, typeTable->localPool,
                                           varBaseName);
          
        if (var == NULL) {
          synterr(csound,
                  Str("unable to find array operator for var %s line %d\n"), varBaseName, tree->line);
          return NULL;
        } else {
          if (var->varType == &CS_VAR_TYPE_ARRAY) {
            return strdup(var->subType->varTypeName);
          } else if (var->varType == &CS_VAR_TYPE_A) {
            return strdup("k");
          }
            
          synterr(csound,
                  Str("invalid array type %s line %d\n"), var->varType->varTypeName, tree->line);
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

        out = resolve_opcode_get_outarg(csound,
                                        find_opcode2(csound, ":cond"),
                                        condInTypes);

        if (UNLIKELY(out == NULL)) {
          synterr(csound,
                  Str("unable to find ternary operator for "
                      "types '%s ? %s : %s' line %d\n"),
                  ans, arg1, arg2, tree->line);
          do_baktrace(csound, tree->locn);
          return NULL;
        }

        return cs_strdup(csound, out);

      }

      // Deal with odd case if i(expressions)
      if (tree->type == T_FUNCTION && !strcmp(tree->value->lexeme, "i")) {
        //print_tree(csound, "i()", tree);
        if (tree->right->type == T_ARRAY &&
            tree->right->left->type == T_IDENT &&
            isirate(/*csound,*/ tree->right->right)) {
          //printf("OK array case\n");
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

        if(tree->value->optype != NULL) /* if there is type annotation */
          out = check_annotated_type(csound, entries, tree->value->optype);
        else  out = resolve_opcode_get_outarg(csound, entries, argTypeRight);


        if (UNLIKELY(out == 0)) {
          synterr(csound, Str("error: opcode '%s' for expression with arg "
                              "types %s not found, line %d\n"),
                  opname, argTypeRight, tree->line);
          do_baktrace(csound, tree->locn);
          return NULL;
        }

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
                  Str("Unable to verify arg types for expression '%s'\n"),
                  opname);
          do_baktrace(csound, tree->locn);
          return NULL;
        }

        OENTRIES* entries = find_opcode2(csound, opname);


        argTypeLeft = convert_internal_to_external(csound, argTypeLeft);
        argTypeRight = convert_internal_to_external(csound, argTypeRight);

        len1 = strlen(argTypeLeft);
        len2 = strlen(argTypeRight);
        inArgTypes = malloc(len1 + len2 + 1);

        strncpy(inArgTypes, argTypeLeft, len1);
        strncpy(inArgTypes + len1, argTypeRight, len2);

        inArgTypes[len1 + len2] = '\0';

        out = resolve_opcode_get_outarg(csound, entries, inArgTypes);

        if (UNLIKELY(out == NULL)) {
          synterr(csound, Str("error: opcode '%s' for expression with arg "
                              "types %s not found, line %d \n"),
                  opname, inArgTypes, tree->line);
          do_baktrace(csound, tree->locn);
          free(inArgTypes);
          return NULL;
        }

        free(inArgTypes);
        return cs_strdup(csound, out);

      } else {
        return argTypeRight;
      }

    }

    if (is_boolean_expression_node(tree)) {
      char* argTypeLeft = get_arg_type2(csound, tree->left, typeTable);
      char* argTypeRight = get_arg_type2(csound, tree->right, typeTable);

      char* opname = get_boolean_expression_opcode_type(csound, tree);
      int len1, len2;
      char* inArgTypes;
      char* out;
      OENTRIES* entries;

      if (UNLIKELY(argTypeLeft == NULL || argTypeRight == NULL)) {
        synterr(csound,
                Str("Unable to verify arg types for boolean expression '%s'\n"),
                opname);
        do_baktrace(csound, tree->locn);
        return NULL;
      }

      entries = find_opcode2(csound, opname);

      len1 = strlen(argTypeLeft);
      len2 = strlen(argTypeRight);
      inArgTypes = malloc(len1 + len2 + 1);

      strncpy(inArgTypes, argTypeLeft, len1);
      strncpy(inArgTypes + len1, argTypeRight, len2);

      inArgTypes[len1 + len2] = '\0';

      out = resolve_opcode_get_outarg(csound, entries, inArgTypes);

      if (UNLIKELY(out == NULL)) {
        synterr(csound, Str("error: boolean expression '%s' with arg "
                            "types %s not found, line %d \n"),
                opname, inArgTypes, tree->line);
        do_baktrace(csound, tree->locn);
        free(inArgTypes);
        return NULL;
      }
      free(inArgTypes);
      return cs_strdup(csound, out);

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
    case T_IDENT:
            
      s = tree->value->lexeme;

      if (is_reserved(s)) {
        return cs_strdup(csound, "r");                              /* rsvd */
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

      varBaseName = strtok_r(s, ":", &brkt);
            
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
                                         varBaseName);
        if(var == NULL)
          var = csoundFindVariableWithName(csound, typeTable->globalPool,
                                           varBaseName);
      } else
        var = csoundFindVariableWithName(csound, typeTable->localPool,
                                         varBaseName);

      if (UNLIKELY(var == NULL)) {
        synterr(csound, Str("Variable '%s' used before defined\n"),
                tree->value->lexeme);
        do_baktrace(csound, tree->locn);
        return NULL;
      }

      if (var->varType == &CS_VAR_TYPE_ARRAY) {
        return create_array_arg_type(csound, var);
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
    if(dot != NULL) {
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


/* Finds OENTRIES that match the given opcode name.  May return multiple
 * OENTRY*'s for each entry in a polyMorphic opcode.
 */
OENTRIES* find_opcode2(CSOUND* csound, char* opname) {

    int i = 0;
    char *shortName;
    CONS_CELL *head;
    OENTRIES* retVal;

    if (UNLIKELY(opname == NULL)) {
      return NULL;
    }

    retVal = csound->Calloc(csound, sizeof(OENTRIES));

    shortName = get_opcode_short_name(csound, opname);

    head = cs_hash_table_get(csound, csound->opcodes, shortName);

    retVal->count = cs_cons_length(head);
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

    while(*r == '[') r++;

    if (*r == '.' || *r == '?' || *r == '*') {
      return 1;
    }

    while(*f == '[') f++;

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
    if((inArgsFound == NULL || strlen(inArgsFound) == 0) &&
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

      csound->Free(csound, argsFound);
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
      if(*found != *required) {
        return 0;
      }
      return check_array_arg(found, required);
    }

    if(strcmp(found, required) == 0) {
      return 1;
    }

    t = (char*)POLY_OUT_TYPES[0];
    for(i = 0; t != NULL; i += 2) {
      if(strcmp(required, t) == 0) {
        return (strchr(POLY_OUT_TYPES[i + 1], *found) != NULL);
      }
      t = (char*)POLY_OUT_TYPES[i + 2];
    }

    if (!is_out_var_arg(*required)) {
      return 0;
    }

    t = (char*)VAR_ARG_OUT_TYPES[0];
    for(i = 0; t != NULL; i += 2) {
      if(strcmp(required, t) == 0) {
        return (strchr(VAR_ARG_OUT_TYPES[i + 1], *found) != NULL);
      }
      t = (char*)VAR_ARG_OUT_TYPES[i + 2];
    }
    return 0;
}

int check_out_args(CSOUND* csound, char* outArgsFound, char* opOutArgs)
{

    if((outArgsFound == NULL || strlen(outArgsFound) == 0) &&
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

      csound->Free(csound, argsFound);
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
    int i;


    for (i = 0; i < entries->count; i++) {
        OENTRY* temp = entries->entries[i];
//        if (temp->intypes == NULL && temp->outypes == NULL) {
//            if (outArgTypes == NULL && inArgTypes == NULL) {
//
//            }
//            continue;
//        }
        if (check_in_args(csound, inArgTypes, temp->intypes) &&
            check_out_args(csound, outArgTypes, temp->outypes)) {
//            if (retVal != NULL) {
//                return NULL;
//            }
//            retVal = temp;
            return temp;
        }
    }
    return NULL;
//    return retVal;
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

//PUBLIC int resolve_opcode_num(CSOUND* csound, OENTRIES* entries,
//                              char* outArgTypes, char* inArgTypes) {
//
//    int i;
////    int retVal = -1;
//
//    for (i = 0; i < entries->count; i++) {
//        OENTRY* temp = entries->entries[i];
//        if (temp->intypes == NULL && temp->outypes == NULL) {
//            continue;
//        }
//        if(check_in_args(csound, inArgTypes, temp->intypes) &&
//           check_out_args(csound, outArgTypes, temp->outypes)) {
////            if (retVal >= 0) {
////                return 0;
////            }
////            retVal = entries->opnum[i];
//            return entries->opnum[i];
//        }
//
//    }
//
////    return (retVal < 0) ? 0 : retVal;
//    return 0;
//}


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
        if(argType == NULL) {
            argsLen += 1;
            argTypes[index++] = "@";
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
    }

    argString[argsLen] = '\0';

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
          if(argType==NULL) {
            synterr(csound,
              Str("Variable type for %s could not be determined.\n"), varName);
            do_baktrace(csound, tree->locn);
            return 0;
          }

          //FIXME - this feels like a hack
          if (*argType == 'c' || *argType == 'r' || *argType == 'p') {
            break;
          }

          pool = (*varName == 'g') ?
            typeTable->globalPool : typeTable->localPool;
          var = csoundFindVariableWithName(csound, pool, varName);
          if (UNLIKELY(var == NULL)) {
            /* VL: 13-06-13
               if it is not found, we still check the global (merged) pool */
            if (*varName == 'g')
              var = csoundFindVariableWithName(csound, csound->engineState.varPool,
                                               varName);
            if(var == NULL) {
              synterr(csound,
                      Str("Variable '%s' used before defined\n"), varName);
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
            if (var == NULL) {
              synterr(csound,
                      Str("Variable '%s' used before defined\n"), varName);
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
    char *brkt; /* used with strtok_r */
    
    char* varBase = strtok_r(varName, ":", &brkt);
    char* typedIdentArg = strtok_r(NULL, ":", &brkt);
    
    
    t = varBase;
    if (*t == '#') t++;
    pool = (*t == 'g') ? typeTable->globalPool : typeTable->localPool;
    
    var = csoundFindVariableWithName(csound, pool, varBase);
    if (var == NULL) {
      if (typedIdentArg != NULL) {
        argLetter[0] = typedIdentArg[0];
      } else {
        t = varBase;
        argLetter[1] = 0;

        if (*t == '#') t++;
        if (*t == 'g') t++;

        if (*t == '[' || *t == 't') { /* Support legacy t-vars */
          int dimensions = 1;
          CS_TYPE* varType;
          char* b = t + 1;

          while(*b == '[') {
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
      }
      

      type = csoundGetTypeForVarName(csound->typePool, argLetter);

      var = csoundCreateVariable(csound, csound->typePool,
                                 type, varBase, typeArg);
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

    char *brkt; /* used with strtok_r */
    
    char* varBase = strtok_r(varName, ":", &brkt);
    char* typedIdentArg = strtok_r(NULL, ":", &brkt);
    
    pool = (*varName == 'g') ? typeTable->globalPool : typeTable->localPool;

    var = csoundFindVariableWithName(csound, pool, varBase);
    if (var == NULL) {
      CS_TYPE* varType;

      if (typedIdentArg != NULL) {
        argLetter[0] = typedIdentArg[0];
      } else {
        t = varBase;
        argLetter[1] = 0;
          
        if (*t == '#') t++;
        if (*t == 'g') t++;
            
        argLetter[0] = (*t == 't') ? 'k' : *t; /* Support legacy t-vars */
      }
     
      varType = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);

      varInit.dimensions = dimensions;
      varInit.type = varType;
      typeArg = &varInit;

      var = csoundCreateVariable(csound, csound->typePool,
                                 (CS_TYPE*) &CS_VAR_TYPE_ARRAY,
                                 varBase, typeArg);
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
      case T_TYPED_IDENT:
        varName = current->value->lexeme;
            
        /* TODO - This needs to check if someone put in sr:k or ksmps:i or something like that */
        if (is_reserved(varName)) {
          // skip reserved vars, these are handled elsewhere
          break;
        }

        if (*varName == 't') { /* Support legacy t-vars */
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

TREE* get_initial_unary_operator(TREE* tree) {
    if (tree == NULL) return NULL;
    
    TREE* current = tree;
    while (current->left != NULL) {
        current = current->left;
    }
    if (current->type == S_UMINUS || current->markup == &UNARY_PLUS) {
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
    TREE* retVal;
    new_left->type = T_IDENT;
   
    if (unary_op->type == S_UMINUS) {
        retVal = make_node(csound, unary_op->line, unary_op->locn, '-', new_left, unary_op->right);
    } else {
        retVal = make_node(csound, unary_op->line, unary_op->locn, '+', new_left, unary_op);
        unary_op->markup = NULL;
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
    int leftCount, rightCount;
    
    if (root->type == T_ASSIGNMENT ||
        root->type == GOTO_TOKEN ||
        root->type == KGOTO_TOKEN ||
        root->type == IGOTO_TOKEN) {
        // i.e. a = func(a + b)
        return root;
    }
    
    if (root->type != T_OPCALL) {
        synterr(csound,
              Str("Internal Error: convert_statement_to_opcall received a non T_OPCALL TREE\n"));
        return NULL;
    }
    
    if (root->value != NULL) {
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
int verify_opcode(CSOUND* csound, TREE* root, TYPE_TABLE* typeTable) {

    TREE* left = root->left;
    TREE* right = root->right;
    char* leftArgString;
    char* rightArgString;
    char* opcodeName;

    if(root->value == NULL) return 0;

    if (!check_args_exist(csound, root->right, typeTable)) {
      return 0;
    }

//    print_tree(csound, "Verifying Opcode: Left\n", root->left);
//    print_tree(csound, "Verifying Opcode: Right\n", root->right);
    add_args(csound, root->left, typeTable);

    opcodeName = root->value->lexeme;
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
      return 0;
    }

    OENTRY* oentry;
    if(root->value->optype == NULL)
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
      csoundMessage(csound, Str("Line: %d\n"),
                    root->line);
      do_baktrace(csound, root->locn);
      return 0;
    } else {
      root->markup = oentry;
    }

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

int is_label(char* ident, CONS_CELL* labelList) {
    CONS_CELL* current;

    if (labelList == NULL) return 0;

    current = labelList;

    while (current != NULL) {
      if (strcmp((char*)current->value, ident) == 0) {
        return 1;
      }
      current = current->next;
    }
    return 0;
}

int is_reserved(char* varname) {
    return (strcmp("sr", varname) == 0 ||
            strcmp("kr", varname) == 0 ||
            strcmp("ksmps", varname) == 0 ||
            strcmp("0dbfs", varname) == 0 ||
            strcmp("nchnls", varname) == 0 ||
            strcmp("nchnls_i", varname) == 0);
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

      while(current != NULL) {
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
          return 0;
        }

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

TREE* verify_tree(CSOUND * csound, TREE *root, TYPE_TABLE* typeTable)
{
    TREE *anchor = NULL;
    TREE *current = root;
    TREE *previous = NULL;
    TREE* newRight;
    TREE* transformed;

    CONS_CELL* parentLabelList = typeTable->labelList;
    typeTable->labelList = get_label_list(csound, root);

    if (PARSER_DEBUG) csound->Message(csound, "Verifying AST\n");

    while (current != NULL) {
      switch(current->type) {
      case INSTR_TOKEN:
        if (PARSER_DEBUG) csound->Message(csound, "Instrument found\n");
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
              
        add_udo_definition(csound,
                           current->left->value->lexeme,
                           current->left->left->value->lexeme,
                           current->left->right->value->lexeme);
 

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

      case LABEL_TOKEN:
        break;

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
int csound_orcwrap()
{
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
                     CSOUND *csound, TREE *astTree, const char *str)
{
    char ch;
    char *p = csound_orcget_current_pointer(yyscanner)-1;
    int line = csound_orcget_lineno(yyscanner);
    uint64_t files = csound_orcget_locn(yyscanner);
    if (*p=='\0') line--;
    csound->Message(csound, Str("\nerror: %s  (token \"%s\")"),
                    str, csound_orcget_text(yyscanner));
    do_baktrace(csound, files);
    csound->Message(csound, Str(" line %d:\n>>>"), line);
    while ((ch=*--p) != '\n' && ch != '\0');
    do {
      ch = *++p;
      if (ch == '\n') break;
      csound->Message(csound, "%c", ch);
    } while (ch != '\n' && ch != '\0');
    csound->Message(csound, " <<<\n");
}

void do_baktrace(CSOUND *csound, uint64_t files)
{
    while (files) {
      unsigned int ff = files&0xff;
      files = files >>8;
      csound->Message(csound, Str(" from file %s (%d)\n"),
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
    //csound->DebugMsg(csound, "%s(%d) line = %d\n", __FILE__, __LINE__, line);
    return ans;
}

TREE* make_leaf(CSOUND *csound, int line, int locn, int type, ORCTOKEN *v)
{
    TREE *ans;
    ans = (TREE*)csound->Malloc(csound, sizeof(TREE));
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
    csound->DebugMsg(csound, "%s(%d) line = %d\n", __FILE__, __LINE__, line);
    return ans;
}

static void delete_tree(CSOUND *csound, TREE *l)
{
    while (1) {
      TREE *old = l;
      if (UNLIKELY(l==NULL)) {
        return;
      }
      if (l->value) {
        if (l->value->lexeme) {
          //printf("Free %p (%s)\n", l->value->lexeme, l->value->lexeme);
          csound->Free(csound, l->value->lexeme);
          //l->value->lexeme = NULL;
        }
        //printf("Free val %p\n", l->value);
        csound->Free(csound, l->value);
        //l->value = NULL;
      }
      delete_tree(csound, l->left);
      //l->left = NULL;
      delete_tree(csound, l->right);
      //l->right = NULL;
      l = l->next;
      //printf("Free %p\n", old);
      csound->Free(csound, old);
    }
}

PUBLIC void csoundDeleteTree(CSOUND *csound, TREE *tree)
{
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

    csound->Message(csound, "<tree%s type=\"%d\" ", child[which], l->type);

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
//    case S_TASSIGN:
//      csound->Message(csound,"name=\"S_TASSIGN\""); break;
//    case S_TABRANGE:
//      csound->Message(csound,"name=\"S_TABRANGE\""); break;
//    case S_TABREF:
//      csound->Message(csound,"name=\"S_TABREF\""); break;
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
      csound->Message(csound, msg);
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
        printf("THIS SHOULD NOT HAPPEN -- ep NULL %s(%d)\n",
               __FILE__, __LINE__);
      }
      if (ep->intypes != NULL) {
        nreqd = argsRequired(ep->intypes);
        inArgParts = splitArgs(csound, ep->intypes);
      }

      if (PARSER_DEBUG) {
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
    }
}

char tree_argtyp(CSOUND *csound, TREE *tree) {
    if (tree->type == INTEGER_TOKEN || tree->type == NUMBER_TOKEN) {
      return 'i';
    }

    return argtyp2( tree->value->lexeme);
}
