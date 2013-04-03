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
//#include <stdbool.h>
#include "csoundCore.h"
#include "csound_orc.h"
#include "namedins.h"
#include "parse_param.h"
#include "csound_type_system.h"
#include "csound_standard_types.h"
#include "csound_orc_expressions.h"


char *csound_orcget_text ( void *scanner );
int is_label(char* ident, CONS_CELL* labelList);

extern  char argtyp2(char*);
extern  int tree_arg_list_count(TREE *);
void print_tree(CSOUND *, char *, TREE *);

/* from csound_orc_compile.c */
extern int argsRequired(char* arrayName);
extern char** splitArgs(CSOUND* csound, char* argString);
extern int pnum(char*);

OENTRIES* find_opcode2(CSOUND*, char*);
char resolve_opcode_get_outarg(CSOUND* csound,
                               OENTRIES* entries, char* inArgTypes);
char* get_arg_string_from_tree(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable);

char* cs_strdup(CSOUND* csound, char* str) {
    size_t len = strlen(str);
    char* retVal;

    if (len == 0) {
      return NULL;
    }

    retVal = mmalloc(csound, (len + 1) * sizeof(char));
    memcpy(retVal, str, len * sizeof(char));
    retVal[len] = '\0';

    return retVal;
}

char* cs_strndup(CSOUND* csound, char* str, size_t size) {
    size_t len = strlen(str);
    char* retVal;

    if(len == 0) {
      return NULL;
    } else if (size > len) {
      return cs_strdup(csound, str);
    }

    retVal = mmalloc(csound, (size + 1) * sizeof(char));
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
      return "pow";
    case S_TABREF:
      return "#tabref";
    case S_TABRANGE:
      return "#tabgen";
    case S_TABSLICE:
      return "#tabslice";
    case T_MAPK:
      return "##tabmap";
    case T_MAPI:
      return "##tabmapo_i";
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
    case S_A2K:
      return "vaget";
    case T_ARRAY:
      return "array_get";
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


    while (*t == '[') {
      t++;
    }
    temp[0] = *t;
    temp[1] = 0;
    return cs_strdup(csound, temp);
}

//FIXME - this needs to get a TYPE_TABLE here with a label list to
//        check if it is a LABEL
PUBLIC char* get_arg_type(CSOUND* csound, TREE* tree)
{                   /* find arg type:  d, w, a, k, i, c, p, r, S, B, b, t */
    char* s;
    char* t;
    CS_TYPE* type;

    if (is_expression_node(tree)) {
      TREE* nodeToCheck = tree;

      if (tree->type == T_ARRAY) {
        //Note: does not verify here that arg expression is valid for
        //array_get/array_set
        //This is due to csound_orc.y only allowing expressions and
        //functions and not boolean expressions
        //If a case arises where this needs to be check, code should
        //be added here
        return get_array_sub_type(csound, tree->left->value->lexeme);
      }

      if (tree->type == '?') {
        char* arg1, *arg2, *ans, out, *retVal;
        char condInTypes[4];

        ans = get_arg_type(csound, tree->left);
        if (ans == NULL || (*ans != 'b' && *ans != 'B')) {
          synterr(csound,
                  Str("non-boolean expression found for ternary operator,"
                      " line %d\n"), tree->line);
          return NULL;
        }
        arg1 = get_arg_type(csound, tree->right->left);
        arg2 = get_arg_type(csound, tree->right->right);

        condInTypes[0] = *ans;
        condInTypes[1] = *arg1;
        condInTypes[2] = *arg2;
        condInTypes[3] = 0;

        OENTRIES* entries = find_opcode2(csound, ":cond");
        out = resolve_opcode_get_outarg(csound, entries, condInTypes);

        if (out == 0) {
          synterr(csound,
                  Str("unable to find ternary operator for "
                      "types '%s ? %s : %s' line %d\n"),
                  ans, arg1, arg2, tree->line);
          return NULL;
        }

        retVal = mmalloc(csound, 2);
        retVal[0] = out;
        retVal[1] = 0;

        return retVal;
      }

      char* argTypeRight = get_arg_type(csound, nodeToCheck->right);

      if(nodeToCheck->left != NULL) {
        char* argTypeLeft = get_arg_type(csound, nodeToCheck->left);

        char* opname = get_expression_opcode_type(csound, nodeToCheck);

        if (argTypeLeft == NULL || argTypeRight == NULL) {
          synterr(csound, Str("Unable to verify arg types for expression '%s'\n"),
                  opname);
          return NULL;
        }

        OENTRIES* entries = find_opcode2(csound, opname);

        int len1 = strlen(argTypeLeft);
        int len2 = strlen(argTypeRight);
        char* inArgTypes = malloc(len1 + len2 + 1);

        strncpy(inArgTypes, argTypeLeft, len1);
        strncpy(inArgTypes + len1, argTypeRight, len2);

        inArgTypes[len1 + len2] = '\0';

        char out = resolve_opcode_get_outarg(csound, entries, inArgTypes);

        if (out == 0) {
          synterr(csound,
                  Str("error: opcode '%s' for expression with arg types "
                      "%s not found, line %d \n"),
                  opname, inArgTypes, tree->line);
          return NULL;
        }

        char c[2];
        c[0] = out;
        c[1] = 0;

        return cs_strdup(csound, c);

      } else {
        return argTypeRight;
      }

    }

    if (is_boolean_expression_node(tree)) {
      char* argTypeLeft = get_arg_type(csound, tree->left);
      char* argTypeRight = get_arg_type(csound, tree->right);

      char* opname = get_boolean_expression_opcode_type(csound, tree);

      if (argTypeLeft == NULL || argTypeRight == NULL) {
        synterr(csound,
                Str("Unable to verify arg types for boolean expression '%s'\n"),
                opname);
        return NULL;
      }

      OENTRIES* entries = find_opcode2(csound, opname);

      int len1 = strlen(argTypeLeft);
      int len2 = strlen(argTypeRight);
      char* inArgTypes = malloc(len1 + len2 + 1);

      strncpy(inArgTypes, argTypeLeft, len1);
      strncpy(inArgTypes + len1, argTypeRight, len2);

      inArgTypes[len1 + len2] = '\0';

      char out = resolve_opcode_get_outarg(csound, entries, inArgTypes);

      if (out == 0) {
        synterr(csound, Str("error: boolean expression '%s' with arg "
                            "types %s not found, line %d \n"),
                opname, inArgTypes, tree->line);
        return NULL;
      }

      char c[2];
      c[0] = out;
      c[1] = 0;

      return cs_strdup(csound, c);

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
    case ZERODBFS_TOKEN:
    case NCHNLS_TOKEN:
    case NCHNLSI_TOKEN:
      return cs_strdup(csound, "r");                              /* rsvd */
    case LABEL_TOKEN:
      //FIXME: Need to review why label token is used so much in parser,
      //for now treat as T_IDENT
    case T_IDENT:
      s = tree->value->lexeme;

      if ((*s >= '1' && *s <= '9') || *s == '.' || *s == '-' || *s == '+' ||
          (*s == '0' && strcmp(s, "0dbfs") != 0))
        return cs_strdup(csound, "c");                              /* const */
      if (*s == '"')
        return cs_strdup(csound, "S");

      if (pnum(s) >= 0)
        return cs_strdup(csound, "p");                              /* pnum */
      if (*s == '#')
        s++;
      if (*s == 'g')
        s++;

      if (*s == '[') {
        int len = 1;
        t = s;

        while (*t == '[') {
          t++;
          len++;
        }

        char* retVal = mmalloc(csound, (len + 2) * sizeof(char));
        memcpy(retVal, s, len);
        retVal[len] = ';';
        retVal[len + 1] = '\0';

        return retVal;
      }

      if (*s != 'c') { // <- FIXME: this is here because labels are not
                       //           checked correctly at the moment
        type = csoundGetTypeForVarName(csound->typePool, s);
        if (type != NULL) {
          return cs_strdup(csound, type->varTypeName);
        }
      }

      return cs_strdup(csound, "l"); // assume it is a label
    case T_ARRAY:
    case T_ARRAY_IDENT:

      s = tree->value->lexeme;

      if (*s == '#') s++;
      if (*s == 'g') s++;

      t = s;

      int len = 1;
      while (*t == '[') {
        t++;
        len++;
      }

      char* retVal = mmalloc(csound, (len + 2) * sizeof(char));
      memcpy(retVal, s, len);
      retVal[len] = ';';
      retVal[len + 1] = '\0';

      return retVal;

    default:
      csoundWarning(csound, Str("Unknown arg type: %d\n"), tree->type);
//            print_tree(csound, "Arg Tree\n", tree);
      return NULL;
    }
}

char* create_array_arg_type(CSOUND* csound, CS_VARIABLE* arrayVar) {

    int i, len = arrayVar->dimensions + 3;
    char* retVal = mmalloc(csound, len);
    retVal[len - 1] = '\0';
    retVal[len - 2] = ';';
    retVal[len - 3] = *arrayVar->subType->varTypeName;
    for (i = len - 4; i >= 0; i--) {
        retVal[i] = '[';
    }
    return retVal;
}

PUBLIC char* get_arg_type2(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable)
{
    char* s;
    char* t;
    //CS_TYPE* type;
    CS_VAR_POOL* pool;
    CS_VARIABLE* var;

    if (is_expression_node(tree)) {
      TREE* nodeToCheck = tree;

      if (tree->type == T_ARRAY) {
        return get_array_sub_type(csound, tree->left->value->lexeme);
      }

      if (tree->type == '?') {
        char* arg1, *arg2, *ans, out, *retVal;
        char condInTypes[4];

        ans = get_arg_type2(csound, tree->left, typeTable);
        if (ans == NULL || (*ans != 'b' && *ans != 'B')) {
          synterr(csound,
                  Str("non-boolean expression found for ternary operator,"
                      " line %d\n"), tree->line);
          return NULL;
        }
        arg1 = get_arg_type2(csound, tree->right->left, typeTable);
        arg2 = get_arg_type2(csound, tree->right->right, typeTable);

        condInTypes[0] = *ans;
        condInTypes[1] = *arg1;
        condInTypes[2] = *arg2;
        condInTypes[3] = 0;

        out = resolve_opcode_get_outarg(csound,
                                        find_opcode2(csound, ":cond"),
                                        condInTypes);

        if (out == 0) {
          synterr(csound,
                  Str("unable to find ternary operator for "
                      "types '%s ? %s : %s' line %d\n"),
                  ans, arg1, arg2, tree->line);
          return NULL;
        }

        retVal = mmalloc(csound, 2);
        retVal[0] = out;
        retVal[1] = 0;

        return retVal;

      }

      if (tree->type == T_FUNCTION) {
          char* argTypeRight = get_arg_string_from_tree(csound, 
                                                        tree->right, typeTable);
          char* opname = tree->value->lexeme;
          OENTRIES* entries = find_opcode2(csound, opname);

          char out = resolve_opcode_get_outarg(csound, entries, argTypeRight);

          if (out == 0) {
              synterr(csound, Str("error: opcode '%s' for expression with arg "
                                  "types %s not found, line %d \n"),
                      opname, argTypeRight, tree->line);
              return NULL;
          }

          char c[2];
          c[0] = out;
          c[1] = '\0';

          return cs_strdup(csound, c);

      }

      char* argTypeRight = get_arg_type2(csound, nodeToCheck->right, typeTable);

      if(nodeToCheck->left != NULL) {
        char* argTypeLeft = get_arg_type2(csound, nodeToCheck->left, typeTable);

        char* opname = get_expression_opcode_type(csound, nodeToCheck);
        int len1, len2;
        char* inArgTypes;
        char out;

        if (argTypeLeft == NULL || argTypeRight == NULL) {
          synterr(csound, Str("Unable to verify arg types for expression '%s'\n"),
                  opname);
          return NULL;
        }

        OENTRIES* entries = find_opcode2(csound, opname);

        len1 = strlen(argTypeLeft);
        len2 = strlen(argTypeRight);
        inArgTypes = malloc(len1 + len2 + 1);

        strncpy(inArgTypes, argTypeLeft, len1);
        strncpy(inArgTypes + len1, argTypeRight, len2);

        inArgTypes[len1 + len2] = '\0';

        out = resolve_opcode_get_outarg(csound, entries, inArgTypes);

        if (out == 0) {
          synterr(csound, Str("error: opcode '%s' for expression with arg "
                              "types %s not found, line %d \n"),
                  opname, inArgTypes, tree->line);
          return NULL;
        }

        char c[2];
        c[0] = out;
        c[1] = '\0';

        return cs_strdup(csound, c);

      } else {



      }

    }

    if (is_boolean_expression_node(tree)) {
      char* argTypeLeft = get_arg_type2(csound, tree->left, typeTable);
      char* argTypeRight = get_arg_type2(csound, tree->right, typeTable);

      char* opname = get_boolean_expression_opcode_type(csound, tree);
      int len1, len2;
      char* inArgTypes;
      char out;
      char c[2];
      OENTRIES* entries;

      if (argTypeLeft == NULL || argTypeRight == NULL) {
        synterr(csound,
                Str("Unable to verify arg types for boolean expression '%s'\n"),
                opname);
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

      if (out == 0) {
        synterr(csound, Str("error: boolean expression '%s' with arg "
                            "types %s not found, line %d \n"),
                opname, inArgTypes, tree->line);
        return NULL;
      }

      c[0] = out;
      c[1] = 0;

      return cs_strdup(csound, c);

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

            if (is_label(s, typeTable->labelList)) {
                return cs_strdup(csound, "l");
            }

            if ((*s >= '1' && *s <= '9') || *s == '.' || *s == '-' || *s == '+' ||
                (*s == '0' && strcmp(s, "0dbfs") != 0))
                return cs_strdup(csound, "c");                              /* const */
            if (*s == '"')
                return cs_strdup(csound, "S");

            if (pnum(s) >= 0)
                return cs_strdup(csound, "p");                              /* pnum */

            if (*s == '#')
                s++;

            pool = (*s == 'g') ?
            typeTable->globalPool : typeTable->localPool;
            var = csoundFindVariableWithName(pool, tree->value->lexeme);

            if (var == NULL) {
                synterr(csound, Str("Variable '%s' used before defined\n"), tree->value->lexeme);
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

            t = s;

            int len = 1;
            while (*t == '[') {
                t++;
                len++;
            }

            char* retVal = mmalloc(csound, (len + 2) * sizeof(char));
            memcpy(retVal, s, len);
            retVal[len] = ';';
            retVal[len + 1] = '\0';

            return retVal;

        default:
            csoundWarning(csound, Str("Unknown arg type: %d\n"), tree->type);
            //            print_tree(csound, "Arg Tree\n", tree);
            return NULL;
    }
}

/* Finds OENTRIES that match the given opcode name.  May return multiple
 * OENTRY*'s for each entry in a polyMorphic opcode.
 */
PUBLIC OENTRIES* find_opcode2(CSOUND* csound, char* opname) {

    if (opname == NULL) {
      return NULL;
    }

    {
      int listIndex = 0;
      int i;

      OENTRY* opc = csound->opcodlst;
      OENTRIES* retVal = mcalloc(csound, sizeof(OENTRIES));

      int opLen = strlen(opname);

      //trim opcode name if name has . in it
      char* dot = strchr(opname, '.');
      if(dot != NULL) {
        opLen = dot - opname;
      }

      for (i=0; opc < csound->oplstend; opc++, i++) {

        if (strncmp(opname, opc->opname, opLen) == 0) {
          // hack to work with how opcodes are currently defined with
          //".x" endings for polymorphism
          if (opc->opname[opLen] == 0 || opc->opname[opLen] == '.') {
            retVal->entries[listIndex] = opc;
            retVal->opnum[listIndex++] = i;
          }
        }
        retVal->count = listIndex;
      }
      return retVal;
    }
}

inline static int is_in_optional_arg(char arg) {
    return (strchr("opqvjhOJVP", arg) != NULL);
}

inline static int is_in_var_arg(char arg) {
    return (strchr("mMNnyzZ", arg) != NULL);
}

int check_array_arg(char* found, char* required) {
    char* f = found;
    char* r = required;

    while(*r == '[') r++;

    if (*r == '?') {
      return 1;
    }

    while(*f == '[') f++;

    return (*f == *r);
}

PUBLIC int check_in_arg(char* found, char* required) {
    char* t;
    int i;
    if (found == NULL || required == NULL) {
      return 0;
    }

    if(strcmp(found, required) == 0) {
      return 1;
    }

    if (*found == '[' || *required == '[') {
      if(*found != *required) {
        return 0;
      }
      return check_array_arg(found, required);
    }

    if (*required == '?') {
      return 1;
    }

    t = (char*)POLY_IN_TYPES[0];

    for(i = 0; t != NULL; i += 2) {
      if(strcmp(required, t) == 0) {
        return (strchr(POLY_IN_TYPES[i + 1], *found) != NULL);
      }
      t = (char*)POLY_IN_TYPES[i + 2];
    }

    if (is_in_optional_arg(*required)) {
      t = (char*)OPTIONAL_IN_TYPES[0];
      for(i = 0; t != NULL; i += 2) {
        if(strcmp(required, t) == 0) {
          return (strchr(OPTIONAL_IN_TYPES[i + 1], *found) != NULL);
        }
        t = (char*)OPTIONAL_IN_TYPES[i + 2];
      }
    }

    if (!is_in_var_arg(*required)) {
      return 0;
    }

    t = (char*)VAR_ARG_IN_TYPES[0];
    for(i = 0; t != NULL; i += 2) {
      if(strcmp(required, t) == 0) {
        return (strchr(VAR_ARG_IN_TYPES[i + 1], *found) != NULL);
      }
      t = (char*)VAR_ARG_IN_TYPES[i + 2];
    }
    return 0;
}

PUBLIC int check_in_args(CSOUND* csound, char* inArgsFound, char* opInArgs) {
    if((inArgsFound == NULL || strlen(inArgsFound) == 0) &&
       (opInArgs == NULL || strlen(opInArgs) == 0)) {
      return 1;
    }

    if (opInArgs == NULL) {
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
        mfree(csound, argsRequired);
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

      mfree(csound, argsFound);
      mfree(csound, argsRequired);

      return returnVal;
    }
}

inline static int is_out_var_arg(char arg) {
    return (strchr("mzIXNF", arg) != NULL);
}

PUBLIC int check_out_arg(char* found, char* required) {
    char* t;
    int i;

    if (found == NULL || required == NULL) {
      return 0;
    }

    // constants not allowed in out args
    if (strcmp(found, "c") == 0) {
      return 0;
    }

    if (*found == '[' || *required == '[') {
      if(*found != *required) {
        return 0;
      }
      return check_array_arg(found, required);
    }

    if (*required == '?') {
      return 1;
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

PUBLIC int check_out_args(CSOUND* csound, char* outArgsFound, char* opOutArgs)
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
        mfree(csound, argsRequired);
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

      mfree(csound, argsFound);
      mfree(csound, argsRequired);

      return returnVal;
    }
}


/* Given an OENTRIES list, resolve to a single OENTRY* based on the
 * found in- and out- argtypes.  Returns NULL if opcode could not be
 * resolved. If more than one entry matches, mechanism assumes there
 * are multiple opcode entries with same types and last one should
 * override previous definitions.
 */
PUBLIC OENTRY* resolve_opcode(CSOUND* csound, OENTRIES* entries,
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
        if(check_in_args(csound, inArgTypes, temp->intypes) &&
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
PUBLIC char resolve_opcode_get_outarg(CSOUND* csound, OENTRIES* entries,
                              char* inArgTypes) {
    int i;

    for (i = 0; i < entries->count; i++) {
        OENTRY* temp = entries->entries[i];
        if (temp->intypes == NULL && temp->outypes == NULL) {
            continue;
        }
        if(check_in_args(csound, inArgTypes, temp->intypes)) {
            return temp->outypes[0];
        }

    }
    return 0;
}

PUBLIC int resolve_opcode_num(CSOUND* csound, OENTRIES* entries,
                              char* outArgTypes, char* inArgTypes) {

    int i;
//    int retVal = -1;

    for (i = 0; i < entries->count; i++) {
        OENTRY* temp = entries->entries[i];
        if (temp->intypes == NULL && temp->outypes == NULL) {
            continue;
        }
        if(check_in_args(csound, inArgTypes, temp->intypes) &&
           check_out_args(csound, outArgTypes, temp->outypes)) {
//            if (retVal >= 0) {
//                return 0;
//            }
//            retVal = entries->opnum[i];
            return entries->opnum[i];
        }

    }

//    return (retVal < 0) ? 0 : retVal;
    return 0;
}

PUBLIC char* get_arg_string_from_tree(CSOUND* csound, TREE* tree,
                                       TYPE_TABLE* typeTable) {

    int len = tree_arg_list_count(tree);
    int i;

    if (len == 0) {
        return NULL;
    }

    char** argTypes = mmalloc(csound, len * sizeof(char*));
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
            argsLen += strlen(argType);
            argTypes[index++] = argType;
        }


        current = current->next;
    }

    argString = mmalloc(csound, (argsLen + 1) * sizeof(char));
    char* temp = argString;

    for (i = 0; i < len; i++) {
        int size = strlen(argTypes[i]);
        memcpy(temp, argTypes[i], size);
        temp += size;
    }

    argString[argsLen] = '\0';

    return argString;

}



PUBLIC OENTRY* find_opcode_new(CSOUND* csound, char* opname,
                               char* outArgsFound, char* inArgsFound) {

//    csound->Message(csound, "Searching for opcode: %s | %s | %s\n",
//                    outArgsFound, opname, inArgsFound);

    OENTRIES* opcodes = find_opcode2(csound, opname);

    if (opcodes->count == 0) {
      return NULL;
    }
    OENTRY* retVal = resolve_opcode(csound, opcodes, outArgsFound, inArgsFound);

    mfree(csound, opcodes);

    return retVal;
}

PUBLIC int find_opcode_num(CSOUND* csound, char* opname,
                           char* outArgsFound, char* inArgsFound) {

//    csound->Message(csound, "Searching for opcode: %s | %s | %s\n",
//                    outArgsFound, opname, inArgsFound);

    OENTRIES* opcodes = find_opcode2(csound, opname);

    if (opcodes->count == 0) {
      return 0;
    }
    int retVal = resolve_opcode_num(csound, opcodes, outArgsFound, inArgsFound);

    mfree(csound, opcodes);

    return retVal;
}


//FIXME - this needs to be updated to take into account array names
// that could clash with non-array names, i.e. kVar and kVar[]
int check_args_exist(CSOUND* csound, TREE* tree, TYPE_TABLE* typeTable) {

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

          argType = get_arg_type(csound, current);

          //FIXME - this feels like a hack
          if (*argType == 'c' || *argType == 'r' || *argType == 'p') {
            break;
          }

          pool = (*varName == 'g') ?
            typeTable->globalPool : typeTable->localPool;

          if (csoundFindVariableWithName(pool, varName) == NULL) {
            synterr(csound, Str("Variable '%s' used before defined\n"), varName);
            return 0;
//            csound->Warning(csound,
//                            Str("Variable '%s' used before defined\n"), varName);
          }
          break;
        case T_ARRAY:
          varName = current->left->value->lexeme;

          pool = (*varName == 'g') ?
                 typeTable->globalPool : typeTable->localPool;

          if (csoundFindVariableWithName(pool, varName) == NULL) {
            synterr(csound, Str("Variable '%s' used before defined\n"), varName);
//            csound->Warning(csound,
//                            Str("Variable '%s' used before defined\n"), varName);
            return 0;
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

    pool = (*varName == 'g') ? typeTable->globalPool : typeTable->localPool;

    var = csoundFindVariableWithName(pool, varName);
    if (var == NULL) {
      t = varName;
      argLetter[1] = 0;

      if (*t == '#') t++;
      if (*t == 'g') t++;

      if(*t == '[') {
        int dimensions = 1;
        CS_TYPE* varType;
        char* b = t + 1;

        while(*b == '[') {
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

      type = csoundGetTypeForVarName(csound->typePool, argLetter);

      var = csoundCreateVariable(csound, csound->typePool,
                                 type, varName, typeArg);
      csoundAddVariable(pool, var);
    } else {
      //TODO - implement reference count increment
    }

}

void add_array_arg(CSOUND* csound, char* varName, int dimensions, TYPE_TABLE* typeTable) {

    CS_VARIABLE* var;
    char *t;
    CS_VAR_POOL* pool;
    char argLetter[2];
    ARRAY_VAR_INIT varInit;
    void* typeArg = NULL;

    pool = (*varName == 'g') ? typeTable->globalPool : typeTable->localPool;

    var = csoundFindVariableWithName(pool, varName);
    if (var == NULL) {
        CS_TYPE* varType;

        t = varName;
        argLetter[1] = 0;

        if (*t == '#') t++;
        if (*t == 'g') t++;

        argLetter[0] = *t;

        varType = csoundGetTypeWithVarTypeName(csound->typePool, argLetter);

        varInit.dimensions = dimensions;
        varInit.type = varType;
        typeArg = &varInit;

        var = csoundCreateVariable(csound, csound->typePool, (CS_TYPE*) &CS_VAR_TYPE_ARRAY,
                                   varName, typeArg);
        csoundAddVariable(pool, var);
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
        add_array_arg(csound, varName, tree_arg_list_count(current->right), typeTable);

        break;

      case LABEL_TOKEN:
      case T_IDENT:
        varName = current->value->lexeme;
        add_arg(csound, varName, typeTable);
        break;

      case T_ARRAY:
        varName = current->left->value->lexeme;
        add_arg(csound, varName, typeTable);  // FIXME - this needs to work for array and a-names
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

    if (!check_args_exist(csound, root->right, typeTable)) {
      return 0;
    }

    add_args(csound, root->left, typeTable);

    leftArgString = get_arg_string_from_tree(csound, left, typeTable);
    rightArgString = get_arg_string_from_tree(csound, right, typeTable);
    opcodeName = root->value->lexeme;

    if (!strcmp(opcodeName, "xin")) {
        int nreqd = tree_arg_list_count(root->right);

        if(nreqd > OPCODENUMOUTS_LOW) {
            opcodeName = (nreqd > OPCODENUMOUTS_HIGH) ? "##xin256" : "##xin64";
        }
    }

    OENTRIES* entries = find_opcode2(csound, opcodeName);
    if (entries == NULL || entries->count == 0) {
      synterr(csound, Str("Unable to find opcode with name: %s\n"),
              root->value->lexeme);
      return 0;
    }

    OENTRY* oentry = resolve_opcode(csound, entries,
                                    leftArgString, rightArgString);

    if (oentry == NULL) {
      synterr(csound, Str("Unable to find opcode entry for \'%s\' "
                          "with matching argument types:\n"),
              opcodeName);
      csoundMessage(csound, Str("Found: %s %s %s\n"),
                    leftArgString, root->value->lexeme, rightArgString);
      csoundMessage(csound, "Line: %d Loc: %d\n",
                    root->line, root->locn);
    } else {
        root->markup = oentry;
    }

    return 1;
}

/* Walks tree and finds all label: definitions */
CONS_CELL* get_label_list(CSOUND* csound, TREE* root) {
    CONS_CELL* head = NULL;
    int len = 0;
    TREE* current = root;

    while(current != NULL) {
      if(current->type == LABEL_TOKEN) {
        char* labelText = current->value->lexeme;
        head = cs_cons(csound, cs_strdup(csound, labelText), head);
        len++;
      }
      current = current->next;
    }

    if (len == 0) {
      return NULL;
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

int verify_if_statement(CSOUND* csound, TREE* root, TYPE_TABLE* typeTable) {

    char* outArg;

    TREE* right = root->right;

    if (right->type == IGOTO_TOKEN ||
        right->type == KGOTO_TOKEN ||
        right->type == GOTO_TOKEN) {

      if(!check_args_exist(csound, root->left, typeTable)) {
        return 0;
      }

      outArg = get_arg_type(csound, root->left);

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

        if(!check_args_exist(csound, current->left, typeTable)) {
          return 0;
        }

        outArg = get_arg_type(csound, current->left);

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

    outArg = get_arg_type(csound, root->left);


    if(outArg == NULL || (*outArg != 'b' && *outArg != 'B')) {
        synterr(csound,
                Str("expression for until statement not a boolean "
                    "expression, line %d\n"),
                root->line);
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

    if (PARSER_DEBUG) csound->Message(csound, "Verifying AST\n");

    while (current != NULL) {
      switch(current->type) {
      case INSTR_TOKEN:
        if (PARSER_DEBUG) csound->Message(csound, "Instrument found\n");
        typeTable->localPool = mcalloc(csound, sizeof(CS_VAR_POOL));
        current->markup = typeTable->localPool;

        if (current->right) {
          typeTable->labelList = get_label_list(csound, current->right);

          newRight = verify_tree(csound, current->right, typeTable);

          mfree(csound, typeTable->labelList);

          typeTable->labelList = NULL;

          if (newRight == NULL) {
            return NULL;
          }

          current->right = newRight;
          newRight = NULL;
        }

        typeTable->localPool = typeTable->instr0LocalPool;

        break;
      case UDO_TOKEN:
        if (PARSER_DEBUG) csound->Message(csound, "UDO found\n");

        typeTable->localPool = mcalloc(csound, sizeof(CS_VAR_POOL));
        current->markup = typeTable->localPool;

        if (current->right != NULL) {
            typeTable->labelList = get_label_list(csound, current->right);

            newRight = verify_tree(csound, current->right, typeTable);

            mfree(csound, typeTable->labelList);

            if (newRight == NULL) {
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
        if (!verify_until_statement(csound, current, typeTable)) {
          return 0;
        }

        current = expand_until_statement(csound, current, typeTable);

        if (previous != NULL) {
          previous->next = current;
        }

        continue;

      case LABEL_TOKEN:
        break;

      default:
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
    if (*p=='\0') line--;
    csound->Message(csound, Str("\nerror: %s  (token \"%s\")"),
                    str, csound_orcget_text(yyscanner));
    csound->Message(csound, Str(" line %d:\n>>>"), line);
    /* if(!strcmp(csound_orcget_text(yyscanner), "\n")) { */
    /*  csound->Message(csound, Str("error: %s (\"\\n\")"), */
    /*                 str); */
    /*  csound->Message(csound, Str(" line %d:\n>>> "), */
    /*               csound_orcget_lineno(yyscanner)); */
    /* } */
    /* else { */
    /*  csound->Message(csound, Str("\nerror: %s  (token \"%s\")"), */
    /*                 str, csound_orcget_text(yyscanner)); */
    /* csound->Message(csound, Str(" line %d:\n>>> "), */
    /*                 csound_orcget_lineno(yyscanner)+1); */
    /* } */
    while ((ch=*--p) != '\n' && ch != '\0');
    do {
      ch = *++p;
      if(ch == '\n') break;
      csound->Message(csound, "%c", ch);
    } while (ch != '\n' && ch != '\0');
    csound->Message(csound, " <<<\n");
}


/**
 * Appends TREE * node to TREE * node using ->next field in struct; walks
 * down the linked list to append at end; checks for NULL's and returns
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
    ans = (TREE*)mmalloc(csound, sizeof(TREE));
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
    ans = (TREE*)mmalloc(csound, sizeof(TREE));
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

/** Utility function to rewrite array names from xxx to [xxx, as csound
 *  uses first letter to denote type.  Also handles if name starts with g;
 *  used by parser (csound_orc.y)
 */
PUBLIC char* convertArrayName(CSOUND* csound, char* arrayName) {
    if (arrayName == NULL) {
      return NULL;
    }
    {
      int len = strlen(arrayName);
      char* newArrayName;
      if (len == 0) {
        return NULL;
      }

      newArrayName = mmalloc(csound, (len + 3)* sizeof(char));
      if (arrayName[0] == 'g') {
        newArrayName[0] = 'g';
        newArrayName[1] = '[';
        newArrayName[2] = arrayName[1];
        newArrayName[3] = ';';
        memcpy(newArrayName + 4, arrayName + 2, len - 2);
      } else {
        newArrayName[0] = '[';
        newArrayName[1] = arrayName[0];
        newArrayName[2] = ';';
        memcpy(newArrayName + 3, arrayName + 1, len - 1);
      }
      newArrayName[len + 2] = 0;

      return newArrayName;
    }
}

PUBLIC char* addDimensionToArrayName(CSOUND* csound, char* arrayName) {
    if(arrayName == NULL) {
        return NULL;
    }
    int len = strlen(arrayName);

    if(len == 0) {
        return NULL;
    }

    char* newArrayName = mmalloc(csound, (len + 2)* sizeof(char));
    if(arrayName[0] == 'g') {
        newArrayName[0] = 'g';
        newArrayName[1] = '[';
        memcpy(newArrayName + 2, arrayName + 1, len - 1);
    } else {
        newArrayName[0] = '[';
        memcpy(newArrayName + 1, arrayName , len);
    }
    newArrayName[len + 1] = 0;

    return newArrayName;
}

/** Utility function to create assignment statements
 *  Replaces = with correct version for args
 */
char* get_assignment_type(CSOUND *csound, char * ans, TREE* arg1) {
    char c = argtyp2( ans);
    char* str = (char*)mcalloc(csound, 65);

    switch (c) {
    case 'S':
      strcpy(str, "strcpy");
      break;
    case 'a':
      c = argtyp2( arg1->value->lexeme);
      strcpy(str, (c == 'a' ? "=.a" : "upsamp"));
      /* strcpy(str, "=.a"); */
      break;
    case 'p':
      c = 'i'; /* purposefully fall through */
    default:
      sprintf(str, "=.%c", c);
    }

    if (PARSER_DEBUG)
      csound->Message(csound, "Found Assignment type: %s\n", str);

    return str;
}

void delete_tree(CSOUND *csound, TREE *l)
{
    while (1) {
      TREE *old = l;
      if (UNLIKELY(l==NULL)) {
        return;
      }
      if (l->value) {
        if (l->value->lexeme) {
          //printf("Free %p (%s)\n", l->value->lexeme, l->value->lexeme);
          mfree(csound, l->value->lexeme);
          //l->value->lexeme = NULL;
        }
        //printf("Free val %p\n", l->value);
        mfree(csound, l->value);
        //l->value = NULL;
      }
      delete_tree(csound, l->left);
      //l->left = NULL;
      delete_tree(csound, l->right);
      //l->right = NULL;
      l = l->next;
      //printf("Free %p\n", old);
      mfree(csound, old);
    }
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
      csound->Message(csound,"%c:(%d:%d)\n", l->type, l->line, l->locn); break;
    case NEWLINE:
      csound->Message(csound,"NEWLINE:(%d:%d)\n", l->line, l->locn); break;
    case S_NEQ:
      csound->Message(csound,"S_NEQ:(%d:%d)\n", l->line, l->locn); break;
    case S_AND:
      csound->Message(csound,"S_AND:(%d:%d)\n", l->line, l->locn); break;
    case S_OR:
      csound->Message(csound,"S_OR:(%d:%d)\n", l->line, l->locn); break;
    case S_LT:
      csound->Message(csound,"S_LT:(%d:%d)\n", l->line, l->locn); break;
    case S_LE:
      csound->Message(csound,"S_LE:(%d:%d)\n", l->line, l->locn); break;
    case S_EQ:
      csound->Message(csound,"S_EQ:(%d:%d)\n", l->line, l->locn); break;
    case S_TASSIGN:
      csound->Message(csound,"S_TASSIGN:(%d:%d)\n", l->line, l->locn); break;
    case S_TABRANGE:
      csound->Message(csound,"S_TABRANGE:(%d:%d)\n", l->line, l->locn); break;
    case S_TABREF:
      csound->Message(csound,"S_TABREF:(%d:%d)\n", l->line, l->locn); break;
    case T_MAPK:
      csound->Message(csound,"T_MAPK:(%d:%d)\n", l->line, l->locn); break;
    case T_MAPI:
      csound->Message(csound,"T_MAPI:(%d:%d)\n", l->line, l->locn); break;
    case S_GT:
      csound->Message(csound,"S_GT:(%d:%d)\n", l->line, l->locn); break;
    case S_GE:
      csound->Message(csound,"S_GE:(%d:%d)\n", l->line, l->locn); break;
    case LABEL_TOKEN:
      csound->Message(csound,"LABEL_TOKEN: %s\n", l->value->lexeme); break;
    case IF_TOKEN:
      csound->Message(csound,"IF_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case THEN_TOKEN:
          csound->Message(csound,"THEN_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case ITHEN_TOKEN:
          csound->Message(csound,"ITHEN_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case KTHEN_TOKEN:
          csound->Message(csound,"KTHEN_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case ELSEIF_TOKEN:
          csound->Message(csound,"ELSEIF_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case ELSE_TOKEN:
          csound->Message(csound,"ELSE_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case UNTIL_TOKEN:
          csound->Message(csound,"UNTIL_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case DO_TOKEN:
          csound->Message(csound,"DO_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case OD_TOKEN:
          csound->Message(csound,"OD_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case GOTO_TOKEN:
      csound->Message(csound,"GOTO_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case IGOTO_TOKEN:
      csound->Message(csound,"IGOTO_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case KGOTO_TOKEN:
      csound->Message(csound,"KGOTO_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case SRATE_TOKEN:
      csound->Message(csound,"SRATE_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case KRATE_TOKEN:
      csound->Message(csound,"KRATE_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case ZERODBFS_TOKEN:
      csound->Message(csound,"ZERODFFS_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case KSMPS_TOKEN:
      csound->Message(csound,"KSMPS_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case NCHNLS_TOKEN:
      csound->Message(csound,"NCHNLS_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case NCHNLSI_TOKEN:
      csound->Message(csound,"NCHNLSI_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case INSTR_TOKEN:
      csound->Message(csound,"INSTR_TOKEN:(%d:%d)\n", l->line, l->locn); break;
    case STRING_TOKEN:
      csound->Message(csound,"STRING_TOKEN: %s\n", l->value->lexeme); break;
    case T_IDENT:
      csound->Message(csound,"T_IDENT: %s\n", l->value->lexeme); break;
    case INTEGER_TOKEN:
      csound->Message(csound,"INTEGER_TOKEN: %d\n", l->value->value); break;
    case NUMBER_TOKEN:
      csound->Message(csound,"NUMBER_TOKEN: %f\n", l->value->fvalue); break;
    case S_ANDTHEN:
      csound->Message(csound,"S_ANDTHEN:(%d:%d)\n", l->line, l->locn); break;
    case S_APPLY:
      csound->Message(csound,"S_APPLY:(%d:%d)\n", l->line, l->locn); break;
    case T_OPCODE0:
      csound->Message(csound,"T_OPCODE0: %s\n", l->value->lexeme); break;
    case T_OPCODE:
      csound->Message(csound,"T_OPCODE: %s\n", l->value->lexeme); break;
    case T_FUNCTION:
      csound->Message(csound,"T_FUNCTION: %s\n", l->value->lexeme); break;
    case S_UMINUS:
        csound->Message(csound,"S_UMINUS:(%d:%d)\n", l->line, l->locn); break;
    case T_INSTLIST:
        csound->Message(csound,"T_INSTLIST:(%d:%d)\n", l->line, l->locn); break;
    case T_TADD:
      csound->Message(csound,"T_TADD:(%d:%d)\n", l->line, l->locn); break;
    case T_SUB:
      csound->Message(csound,"T_SUB:(%d:%d)\n", l->line, l->locn); break;
    case S_TUMINUS:
      csound->Message(csound,"S_TUMINUS:(%d:%d)\n", l->line, l->locn); break;
    case T_TMUL:
      csound->Message(csound,"T_TMUL:(%d:%d)\n", l->line, l->locn); break;
    case T_TDIV:
      csound->Message(csound,"T_TDIV:(%d:%d)\n", l->line, l->locn); break;
    case T_TREM:
      csound->Message(csound,"T_TREM:(%d:%d)\n", l->line, l->locn); break;
    default:
      csound->Message(csound,"unknown:%d(%d:%d)\n", l->type, l->line, l->locn);
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
    case S_TASSIGN:
      csound->Message(csound,"name=\"S_TASSIGN\""); break;
    case S_TABRANGE:
      csound->Message(csound,"name=\"S_TABRANGE\""); break;
    case S_TABREF:
      csound->Message(csound,"name=\"S_TABREF\""); break;
    case S_A2K:
      csound->Message(csound,"name=\"S_A2K\""); break;
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
    case T_MAPI:
      csound->Message(csound,"name=\"T_MAPI\""); break;
    case T_MAPK:
      csound->Message(csound,"name=\"T_MAPK\""); break;
    case T_TADD:
      csound->Message(csound,"name=\"T_TADD\""); break;
    case T_SUB:
      csound->Message(csound,"name=\"T_SUB\""); break;
    case S_TUMINUS:
      csound->Message(csound,"name=\"S_TUMINUS\""); break;
    case T_TMUL:
      csound->Message(csound,"name=\"T_TMUL\""); break;
    case T_TDIV:
      csound->Message(csound,"name=\"T_TDIV\""); break;
    case T_TREM:
      csound->Message(csound,"name=\"T_TREM\""); break;
    default:
      csound->Message(csound,"name=\"unknown\"(%d)", l->type);
    }

    csound->Message(csound, " loc=\"%d:%d\">\n", l->line, l->locn);

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
            if (l->right==NULL) l->right = temp;
            else appendToTree(csound, l->right, temp);
            break;
          case 'P':
          case 'p':
            temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                             make_int(csound, "1"));
            if (l->right==NULL) l->right = temp;
            else appendToTree(csound, l->right, temp);
            break;
          case 'q':
            temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                             make_int(csound, "10"));
            if (l->right==NULL) l->right = temp;
            else appendToTree(csound, l->right, temp);
            break;

          case 'V':
          case 'v':
            temp = make_leaf(csound, l->line, l->locn, NUMBER_TOKEN,
                             make_num(csound, ".5"));
            if (l->right==NULL) l->right = temp;
            else appendToTree(csound, l->right, temp);
            break;
          case 'h':
            temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                             make_int(csound, "127"));
            if (l->right==NULL) l->right = temp;
            else appendToTree(csound, l->right, temp);
            break;
          case 'J':
          case 'j':
            temp = make_leaf(csound, l->line, l->locn, INTEGER_TOKEN,
                             make_int(csound, "-1"));
            if (l->right==NULL) l->right = temp;
            else appendToTree(csound, l->right, temp);
            break;
          case 'M':
          case 'N':
          case 'm':
            nreqd--;
            break;
          default:
            synterr(csound,
                    Str("insufficient required arguments for opcode %s"
                        " on line %d:%d\n"),
                    ep->opname, l->line, l->locn);
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
