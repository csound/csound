/*
  csound_orc_arguments.c:

  Copyright (C) 2023

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
#include "csound_orc.h"
#include "csound_orc_arguments.h"
#include "csound_orc_expressions.h"
#include "csound_orc_semantics.h"
#include "csound_orc_structs.h"
#include "csound_orc_verify.h"

extern int pnum(char*);
void print_tree(CSOUND *, char *, TREE *);
void do_baktrace(CSOUND *csound, uint64_t files);
extern OENTRIES* find_opcode2(CSOUND*, char*);
extern int is_in_optional_arg(char*);
extern int is_in_var_arg(char*);
extern char* get_expression_opcode_type(CSOUND*, TREE*);
extern char* get_boolean_expression_opcode_type(CSOUND*, TREE*);
extern char** splitArgs(CSOUND*, char*);
extern char* convert_internal_to_external(CSOUND*, char*);

static CSOUND_ORC_ARGUMENT* new_csound_orc_argument(
    CSOUND*,
    CSOUND_ORC_ARGUMENTS*,
    TREE*,
    TYPE_TABLE*
);

static int identCounter = 100;

static char* generateUniqueIdent(CSOUND* csound) {
    char *ident = (char *)csound->Calloc(csound, 32);
    snprintf(ident, 32, "__internal__%d", identCounter);
    identCounter += 1;
    return ident;
}

static CSOUND_ORC_ARGUMENTS* get_arguments_from_tree(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    TREE* tree,
    TYPE_TABLE* typeTable,
    int isAssignee
);


// static void printOrcArgs(
//     CSOUND_ORC_ARGUMENTS* args
// ) {
//     if (args->length == 0) {
//         printf("== ARGUMENT LIST EMPTY ==\n");
//         return;
//     }
//     CONS_CELL* car = args->list;
//     CSOUND_ORC_ARGUMENT* arg;
//     int n = 0;
//     while(car != NULL) {
//         arg = (CSOUND_ORC_ARGUMENT*) car->value;
//         printf("== ARGUMENT LIST INDEX %d ==\n", n);
//         printf("\t text: %s \n", arg->text);
//         printf("\t uid: %s \n", arg->uid);
//         printf("\t type: %s \n", arg->cstype == NULL ? arg->text :
//          arg->cstype->varTypeName);
//         printf("\t isGlobal: %d \n", arg->isGlobal);
//         printf("\t dimensions: %d \n", arg->dimensions);
//         printf("\t isExpression: %d \n", arg->isExpression);
//         car = car->next;
//         n += 1;
//     }
// }

void printNode(TREE* tree) {
    printf("== BEG ==\n");
    if (tree == NULL) {
        return;
    }

    if (tree->value) {
        if (tree->value->lexeme != NULL) {
            printf("tree->value->lexeme: %s\n", tree->value->lexeme);
        } else {
            printf("missing: tree->value->lexeme\n");
        }
        if (tree->value->optype != NULL) {
            printf("tree->value->optype (annotation): %s\n", tree->value->optype);
        }

    } else {
        printf("missing: tree->value\n");
    }
    if (tree->type) {
        printf("tree->type: %d\n", tree->type);
    }
    if (tree->line) {
        printf("tree->line: %d\n", tree->line);
    }

    printf("== END ==\n");
}

// for pretty printing the given arguments
static char* make_arglist_string(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    int displayType
) {
    char pretty[256];
    CONS_CELL* car = args->list;
    CSOUND_ORC_ARGUMENT* arg;

    char* p = pretty;
    while (car != NULL) {
        if (car->value == NULL) {
            p += sprintf(p, "%s", "NULL");
            break;
        }
        arg = car->value;

        p += sprintf(
            p,
            arg->cstype != NULL &&
                arg->cstype->userDefinedType ?
                 ":%s" : "%s",
            displayType ?
                arg->cstype != NULL ?
                    arg->cstype->varTypeName :
                    "@" :
                    arg->text
            );
        int dimensions = arg->dimensions;
        while (dimensions > 0) {
            p += sprintf(p, "%s", "[]");
            dimensions -= 1;
        }
        if (arg->cstype != NULL && arg->cstype->userDefinedType) {
            p += sprintf(p, "%s", ";");
        }

        car = car->next;
    }
    *p = '\0';
    return csound->Strdup(csound, pretty);
}

// static void debugPrintArglist(
//     CSOUND* csound,
//     CSOUND_ORC_ARGUMENTS* expr,
//     CSOUND_ORC_ARGUMENT* parent,
//     TREE* tree
// ) {
//     if (tree->value == NULL && parent->isExpression) {
//         printf("<expr %d> arglist: %s\n",
//           tree->type,
//           make_comma_sep_arglist(csound, expr, 0, 0));
//     } else if (tree->value == NULL) {
//         printf("%s = opchar: %c arglist: %s\n",
//           parent->text,
//           tree->type,
//           make_comma_sep_arglist(csound, expr, 0, 0));
//     } else {
//         printf("%s = opname: %s arglist: %s\n",
//           parent->text,
//           tree->value->lexeme,
//           make_comma_sep_arglist(csound, expr, 0, 0));
//     }
// }


static int fill_optional_inargs(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    TYPE_TABLE* typeTable,
    OENTRY* candidate
) {
    char temp[8];
    char* current = candidate->intypes;
    CONS_CELL* car = args->list;

    // fast forward to the end
    while (car != NULL && *current != '\0') {
        if (*current == ':') {
            while (*current != '\0' && *current != ';') {
                current += 1;
            }
        }
        car = car->next;
        current++;
        while (*current == '[' || *current == ']') {
            current += 1;
        }
    }

    while (car == NULL && *current != '\0') {
        // end of explicit input
        // handle optional args
        CSOUND_ORC_ARGUMENT* optArg;
        memset(temp, '\0', 8);
        MYFLT optargValue = 0.0;

        switch(*current) {
            case 'O':
            case 'o': {
                optargValue = 0.0;
                break;
            }
            case 'P':
            case 'p': {
                optargValue = 1.0;
                break;
            }
            case 'q': {
                optargValue = 10.0;
                break;
            }
            case 'V':
            case 'v': {
                optargValue = 0.5;
                break;
            }
            case 'h': {
                optargValue = 127.0;
                break;
            }
            case 'J':
            case 'j': {
                optargValue = -1.0;
                break;
            }
            default: {
                // not a mismatch if it's a vararg at the end
                if (is_vararg_input_type(current)) {
                    return 1;
                } else {
                    return 0;
                }
            }
        }

        snprintf(temp, 8, "%d", (int) optargValue);
        optArg = new_csound_orc_argument(
            csound,
            args,
            NULL,
            typeTable
        );

        optArg->text = csound->Strdup(csound, temp);
        optArg->cstype = (CS_TYPE*) &CS_VAR_TYPE_C;
        args->append(csound, args, optArg);
        current += 1;
    }

    return 1;
}

// as with 'resolve_opcode_get_outarg', it returns the first match
// inline binary expression tree usually has left and right branches
// for input arguments, so both lists are treated as input arguments
// from left to right
OENTRY* resolve_opcode_with_orc_args(
    CSOUND* csound,
    OENTRIES* entries,
    CSOUND_ORC_ARGUMENTS* inlist,
    CSOUND_ORC_ARGUMENTS* outlist,
    TYPE_TABLE* typeTable,
    int skipOutargs // for subexpressions
) {

    for (int i = 0; i < entries->count; i++) {
        OENTRY* temp = entries->entries[i];

        // if (strncmp(temp->opname, "##array_get", 10) == 0) {
        //     printf("yo\n");
        // }
        int inArgsMatch = 0;
        int outArgsMatch = 0;
        int expectsInputs = temp->intypes != NULL && strlen(temp->intypes) != 0;
        int expectsOutputs = temp->outypes != NULL && strlen(temp->outypes) != 0;
        int isInitOpcode = is_init_opcode(temp->opname);
        int alternatingInputListCount = 0; // needs to be even number

        if ((inlist->length == 0 && !expectsInputs) ||
            (expectsInputs && *temp->intypes == '*')) {
            inArgsMatch = 1;
        }

        // handle cases where inlist begins with optarg
        if (
            inlist->length == 0 &&
            expectsInputs &&
            is_optarg_input_type(temp->intypes)
        ) {
            inArgsMatch = 1;
        }

        if (
            skipOutargs || (outlist->length == 0 && !expectsOutputs) ||
            (expectsOutputs && *temp->outypes == '*')
        ) {
            outArgsMatch = 1;
        }

        if (!inArgsMatch && expectsInputs && inlist->length > 0) {
            char* current = temp->intypes;
            CONS_CELL* car = inlist->list;
            CSOUND_ORC_ARGUMENT* currentArg = car->value;

            while (currentArg != NULL && *current != '\0') {
                int isUserDefinedType = currentArg->cstype != NULL &&
                    currentArg->cstype->userDefinedType;
                int currentArgTextLength = 1;
                int dimensionsNeeded = currentArg->dimensions;
                int dimensionsFound = 0;

                if (
                    dimensionsNeeded > 0 &&
                    is_internal_array_opcode(temp->opname)
                ) {
                    // ##array_get and ##array_set can work
                    // on any dimension and its
                    // intypes are non-specifc about it
                    dimensionsNeeded = 1;
                }

                if (isUserDefinedType) {
                    if (*current == '.') {
                        inArgsMatch = 1;
                    } else {
                        currentArgTextLength = strlen(
                            currentArg->cstype->varTypeName
                        );
                        inArgsMatch = strncmp(
                            current + 1,
                            currentArg->cstype->varTypeName,
                            currentArgTextLength
                        ) == 0;
                        if (inArgsMatch) {
                            char* delimitStruct = strchr(current, ';');
                            int extraCharLen = delimitStruct > 0 ? 1 + delimitStruct - current : 2;
                            currentArgTextLength += extraCharLen;
                            current += currentArgTextLength;
                        } else {
                            break;
                        }
                    }
                } else {
                    if (*current == 'Z') {
                        inArgsMatch = check_satisfies_alternating_z_input(
                            currentArg->cstype->varTypeName,
                            alternatingInputListCount
                        );
                        alternatingInputListCount += 1;
                        if (!inArgsMatch) {
                            // at this point we dont compare if it's even
                            alternatingInputListCount = 0;
                            break;
                        }
                    } else if (check_satisfies_expected_input(
                        currentArg->cstype,
                        current,
                        dimensionsNeeded > 0)
                    ) {
                        inArgsMatch = 1;
                        if (isInitOpcode) break;
                    } else {
                        inArgsMatch = 0;
                        break;
                    }
                }

                if (!is_vararg_input_type(current)) {
                    current += currentArgTextLength;
                    while (*current == '[' || *current == ']') {
                        if (*current == '[') {
                            dimensionsFound += 1;
                        }
                        current += 1;
                    }

                    if (dimensionsNeeded != dimensionsFound) {
                        inArgsMatch = 0;
                        break;
                    }
                }

                car = car == NULL ? NULL : car->next;
                currentArg = car == NULL ? NULL : (CSOUND_ORC_ARGUMENT*) car->value;
            }
        }

        if (
            alternatingInputListCount > 0 &&
            alternatingInputListCount % 2 != 0
        ) {
            inArgsMatch = 0;
        }

        if (inArgsMatch && !outArgsMatch && expectsOutputs && outlist->length > 0) {
            char* current = temp->outypes;
            CONS_CELL* car = outlist->list;
            CSOUND_ORC_ARGUMENT* currentArg = car->value;

            while (currentArg != NULL && *current != '\0' ) {
                int currentArgTextLength = 1;
                int isUserDefinedType = currentArg->cstype != NULL ?
                    currentArg->cstype->userDefinedType :
                    0;
                int dimensionsNeeded = currentArg->dimensions;
                int isInitOpcode = is_init_opcode(temp->opname);

                if (dimensionsNeeded > 0 && isInitOpcode) {
                    // init family of opcodes intypes are non-specific
                    // about the level of dimensions
                    dimensionsNeeded = 1;
                }

                int dimensionsFound = 0;
                if (isUserDefinedType && *current == '.') {
                    outArgsMatch = 1;
                } else if (isUserDefinedType) {
                    // compare MyVar with :MyVar;
                    currentArgTextLength = strlen(
                        currentArg->cstype->varTypeName
                    );
                    outArgsMatch = strncmp(
                        current + 1,
                        currentArg->cstype->varTypeName,
                        currentArgTextLength
                    ) == 0;
                    currentArgTextLength += 2;
                } else if (
                    check_satisfies_expected_output(currentArg->cstype, current)
                ) {
                    outArgsMatch = 1;
                    if (isInitOpcode && isUserDefinedType) break;
                } else {
                    outArgsMatch = 0;
                    break;
                }

                current += currentArgTextLength;
                while (*current == '[' || *current == ']') {
                    if (*current == '[') {
                        dimensionsFound += 1;
                    }
                    current += 1;
                }
                if (
                    (dimensionsNeeded > dimensionsFound &&
                    dimensionsFound > 0)
                ) {
                    // some opcodes like fillarray allow operating
                    // a pointer-like reference on the array
                    // therfore we allow each opcode to decide what
                    // they want to do with non specific dimensions
                    // this can only happen if the array already exists
                    // in memory
                    if (
                        csoundFindVariableWithName(
                            csound,
                            typeTable->localPool,
                            currentArg->text
                        ) != NULL
                    ) {
                        outArgsMatch = 1;
                    } else {
                        outArgsMatch = 0;
                        break;
                    }
                } else if (dimensionsNeeded != dimensionsFound) {
                    outArgsMatch = 0;
                    break;
                }
                car = car->next;
                currentArg = car == NULL ? NULL : (CSOUND_ORC_ARGUMENT*) car->value;
            }
        }

        if (
            inArgsMatch &&
            outArgsMatch &&
            (isInitOpcode || fill_optional_inargs(
                csound,
                inlist,
                typeTable,
                temp
            ))
        ) {
            return temp;
        }
    }

    return NULL;
}

static CSOUND_ORC_ARGUMENT* new_csound_orc_argument(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    TREE* tree,
    TYPE_TABLE* typeTable
) {
    CSOUND_ORC_ARGUMENT* arg = csound->Malloc(
        csound,
        sizeof(CSOUND_ORC_ARGUMENT)
    );
    arg->text = NULL;

    if (tree != NULL && tree->value != NULL) {
        arg->text = csound->Strdup(csound, tree->value->lexeme);
    }
    if (
        tree != NULL &&
        arg->text == NULL &&
        tree->type == T_ARRAY &&
        tree->left != NULL
    ) {
        arg->text = tree->left->value->lexeme;
    }

    if (tree != NULL) {
        arg->type = tree->type;
        arg->linenum = tree->line;
    }

    arg->dimensions = 0;
    arg->memberType = NULL;
    arg->memberTypeDimensions = 0;
    arg->isGlobal = 0;
    arg->isPfield = 0;
    arg->isExpression = 0;
    arg->uid = generateUniqueIdent(csound);

    if (arg->text == NULL) {
        arg->text = arg->uid;
    }
    return arg;
}

static CS_VARIABLE* add_arg_to_pool(
    CSOUND* csound,
    TYPE_TABLE* typeTable,
    char* varName,
    char* annotation,
    int dimensions,
    CS_TYPE* maybeType
) {
    CS_TYPE* type = maybeType;
    CS_VAR_POOL* pool;
    void* typeArg = NULL;
    char* varName_ = varName;
    int isGlobal = varName[0] == 'g';
    pool = isGlobal ? typeTable->globalPool : typeTable->localPool;

    CS_VARIABLE* var = csoundFindVariableWithName(
        csound,
        pool,
        varName
    );

    if (var != NULL) {
        var->refCount += 1;
    } else if (type == NULL) {
        if (annotation != NULL) {
            type = csoundGetTypeWithVarTypeName(
                csound->typePool,
                annotation
            );
            typeArg = type;
        } else {
            if (*varName_ == 'g') {
                varName_ += 1;
            }
            type = csoundFindStandardTypeWithChar(varName_[0]);
        }

        if (type == NULL) {
            return NULL;
        }

        var = csoundCreateVariable(
            csound,
            csound->typePool,
            type,
            varName,
            dimensions,
            typeArg
        );

        csoundAddVariable(csound, pool, var);
    } else {
        var = csoundCreateVariable(
            csound,
            csound->typePool,
            type,
            varName,
            dimensions,
            typeArg
        );

        csoundAddVariable(csound, pool, var);
    }

    return var;

}

static CS_TYPE* resolve_cstype_from_string(
    CSOUND* csound,
    char* argString
) {
    int len = strlen(argString);
    int isUserDefinedType = *argString == ':';
    char* arrayPos = strchr(argString, '[');
    char* tmp = NULL;
    CS_TYPE* cstype = NULL;
    if (arrayPos != NULL) {
        len = arrayPos - argString;
    }
    if (isUserDefinedType) {
        if (arrayPos != NULL) {
            len -= 1;
        } else {
            len -= 2;
        }
    }
    tmp = csound->Calloc(
        csound,  len + 1
    );
    memcpy(tmp, argString + isUserDefinedType, len);
    if (strlen(tmp) == 1) {
        cstype = csoundFindStandardTypeWithChar(
            tmp[0]
        );
    } else {
        cstype = csoundGetTypeWithVarTypeName(
            csound->typePool, tmp);
    }

    if (tmp != NULL) {
        csound->Free(csound, tmp);
    }

    return cstype;
}

static int count_dimensions_from_string(char* str) {
    char* pos = str;
    int dimensions = 0;
    while (*pos != '\0') {
        if (*pos == '[') {
            dimensions += 1;
        }
        pos += 1;
    }
    return dimensions;
}

static CSOUND_ORC_ARGUMENT* resolve_single_argument_from_tree(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    TREE* tree,
    TYPE_TABLE* typeTable,
    int isAssignee
) {
    char* ident = NULL;
    char* annotation = NULL;
    CS_VARIABLE* var = NULL;
    CSOUND_ORC_ARGUMENT* arg = new_csound_orc_argument(
        csound,
        args,
        tree,
        typeTable
    );

    if (tree->value != NULL && tree->value->optype != NULL) {
        annotation = tree->value->optype;
    }

    args->append(csound, args, arg);

    switch(tree->type) {
        case NUMBER_TOKEN:
        case INTEGER_TOKEN: {
            arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_C;
            break;
        }
        case STRING_TOKEN: {
            arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_S;
            break;
        }
        case LABEL_TOKEN: {
            arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_L;
            break;
        }
        case T_MEMBER_IDENT: {

            arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_C;
            tree->type = INTEGER_TOKEN;

            CS_VARIABLE* structVar = csoundFindVariableWithName(
                csound,
                typeTable->localPool,
                tree->value->optype == NULL ?
                    tree->value->lexeme : tree->value->optype
            );
            if (structVar == NULL) {
                structVar = csoundFindVariableWithName(
                    csound,
                    csound->engineState.varPool,
                    tree->value->optype
                );
            }
            if (structVar == NULL) {
                synterr(
                    csound,
                    Str("Line %d failed to find member '%s' of "
                        "non existing struct variable '%s'"),
                    tree->line,
                    tree->value->lexeme,
                    tree->value->optype
                );
                return NULL;
            }

            int memberIndex = findStructMemberIndex(
                structVar->varType->members,
                tree->value->lexeme
            );

            if (memberIndex > -1) {
                char* numStr = csound->Calloc(csound, 16 * sizeof(char));
                sprintf(numStr, "%d", memberIndex);
                csound->Free(csound, tree->value->lexeme);
                csound->Free(csound, arg->text);
                arg->text = csound->Strdup(csound, numStr);
                tree->value->lexeme = numStr;
                tree->value->value = memberIndex;
                int index = 0;
                CONS_CELL* memCar = structVar->varType->members;
                while (index < memberIndex) {
                    memCar = memCar->next;
                    index += 1;
                }
                CS_VARIABLE* memberVar = memCar->value;
                arg->memberType = memberVar->varType;
                arg->memberTypeDimensions = memberVar->dimensions;
            } else {
                synterr(
                    csound,
                    Str("Line %d member '%s' doesn't exist in struct '%s'"),
                    tree->line,
                    tree->value->lexeme,
                    structVar->varType->varTypeName
                );
                return NULL;
            }

            break;
        }
        case T_ARRAY_IDENT: {
            // check if the dimensions are known from
            // the xin arguments of an opcode
            CS_VARIABLE* tmpVar = csoundFindVariableWithName(
                csound,
                typeTable->instr0LocalPool,
                tree->value->lexeme
            );
            if (tmpVar != NULL) {
                arg->dimensions = tmpVar->dimensions;
                arg->cstype = tmpVar->varType;
                add_arg_to_pool(
                    csound,
                    typeTable,
                    csound->Strdup(csound, tree->value->lexeme),
                    NULL,
                    arg->dimensions,
                    arg->cstype
                );
                break;
            } else if (
                tree->right != NULL &&
                tree->right->value != NULL &&
                *tree->right->value->lexeme == '['
            ) {
                TREE* currentBracketNode = tree->right;

                while (currentBracketNode != NULL) {
                    arg->dimensions += 1;
                    currentBracketNode = currentBracketNode->next;
                }
            }
            // lack of break is intended
        }
        case T_TYPED_IDENT:
        case T_IDENT: {
            ident = csound->Strdup(csound, tree->value->lexeme);
            // let's first check if it already exists
            var = csoundFindVariableWithName(
                csound,
                typeTable->localPool,
                ident
            );
            if (var == NULL) {
                var = csoundFindVariableWithName(
                    csound,
                    typeTable->globalPool,
                    ident
                );
                arg->isGlobal = var != NULL;
            }

            if (var != NULL) {
                // already defined
                arg->cstype = var->varType;
                arg->dimensions = var->dimensions;
                break;
            }

            if (is_reserved(ident)) {
                arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_R;
                break;
            }
            if (is_label(ident, typeTable->labelList)) {
                arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_L;
                return arg;
            }
            if (
                annotation != NULL &&
                strlen(annotation) == 1 &&
                *annotation == 'l')
            {
                // label that needs to be added
                arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_L;
                isAssignee = 1;
            }

            if (
                (*ident >= '1' && *ident <= '9') ||
                *ident == '.' || *ident == '-' || *ident == '+' ||
                (*ident == '0' && strcmp(ident, "0dbfs") != 0)
            ) {
                arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_C;
                break;
            }
            if (*ident == '"') {
                arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_S;
                break;
            }
            if (pnum(ident) >= 0) {
                arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_P;
                arg->isPfield = 1;

                csoundAddVariable(
                    csound,
                    typeTable->localPool,
                    csoundCreateVariable(
                        csound,
                        csound->typePool,
                        arg->cstype,
                        ident,
                        arg->dimensions,
                        NULL
                    )
                );
                break;
            }
            if (*ident == 'g' || is_reserved(ident)) {
                arg->isGlobal = 1;
            }

            if (isAssignee) {
                if (
                    annotation == NULL &&
                    arg->cstype == NULL &&
                    is_legacy_t_rate_ident(ident)
                ) {
                    arg->cstype = (CS_TYPE*)&CS_VAR_TYPE_K;
                    arg->dimensions = 1;
                    csound->Warning(
                        csound,
                        Str("Using t-rate variables is deprecated; use k[] instead\n")
                    );
                } else if (
                    annotation != NULL
                ) {
                    arg->cstype = resolve_cstype_from_string(
                        csound, annotation
                    );
                    if (UNLIKELY(arg->cstype == NULL)) {
                        synterr(
                            csound,
                            Str(
                                "\nLine %d type '%s' "
                                "doesn't exist!\n"
                            ),
                            tree->line,
                            annotation
                        );
                        return NULL;
                    }
                    if (arg->dimensions == 0) {
                        // if it's already set (ex. array-idents)
                        // it's probably correct already
                        arg->dimensions = count_dimensions_from_string(
                            annotation
                        );
                    }


                } else {
                    arg->cstype = csoundFindStandardTypeWithChar(
                        *ident == 'g' ? ident[1] : ident[0]
                    );
                }

                var = add_arg_to_pool(
                    csound,
                    typeTable,
                    ident,
                    annotation,
                    arg->dimensions,
                    arg->cstype
                );

                if (var == NULL) {
                    // inference
                    return arg;
                }
                arg->cstype = var->varType;
                arg->dimensions = var->dimensions;

                if (arg->type == T_ARRAY_IDENT) break;
                goto count_dim;
            }

            if (var != NULL) {
                arg->cstype = var->varType;
                arg->dimensions = var->dimensions;
                goto count_dim;
            }

            if (arg->isGlobal) {
                var = csoundFindVariableWithName(
                    csound,
                    csound->engineState.varPool,
                    ident
                );

                if (var == NULL) {
                    var = csoundFindVariableWithName(
                        csound,
                        typeTable->globalPool,
                        ident
                    );
                }
            } else {
                var = csoundFindVariableWithName(
                    csound,
                    typeTable->localPool,
                    tree->value->lexeme
                );
            }

            if (var == NULL) {
                // last attempt, find the var from
                // UDO xin scope
                var = csoundFindVariableWithName(
                    csound,
                    typeTable->instr0LocalPool,
                    tree->value->lexeme
                );
            }

            count_dim:

            if (UNLIKELY(var == NULL)) {
                synterr(
                    csound,
                    Str("\nLine %d variable '%s' used before defined\n"),
                    tree->line - 1,
                    tree->value->lexeme
                );
                return NULL;
            }

            if (arg->cstype == NULL) {
                arg->cstype = var->varType;
            }

            break;
        }
    }

    return arg;
}

static CSOUND_ORC_ARGUMENTS* get_arguments_from_tree(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    TREE* tree,
    TYPE_TABLE* typeTable,
    int isAssignee
) {
    CSOUND_ORC_ARGUMENT* result;
    TREE* current = tree;

    while (current != NULL) {
        result = resolve_single_argument_from_tree(
            csound,
            args,
            current,
            typeTable,
            isAssignee
        );

        if (result == NULL) {
            return NULL;
        }

        current = current->next;
    }
    return args;
}

// CSOUND_ORC_ARGUMENTS helper function
// get the nth index from argument list
static CSOUND_ORC_ARGUMENT* arglist_nth(
    CSOUND_ORC_ARGUMENTS* args,
    int nth
) {
    if (nth < 0 || args->length <= nth) {
        return NULL;
    }
    CONS_CELL* car = args->list;
    while(nth > 0 && nth--) {
        car = car->next;
    }
    if (car == NULL) {
        return NULL;
    } else {
        return (CSOUND_ORC_ARGUMENT*) car->value;
    }
}

// CSOUND_ORC_ARGUMENTS helper function
static void arglist_append(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    CSOUND_ORC_ARGUMENT* arg
) {
    CONS_CELL* tail = csound->Calloc(csound, sizeof(CONS_CELL));
    tail->value = arg;
    tail->next = NULL;

    if (args->list == NULL) {
        args->list = tail;
    } else {
        int pos = args->length;
        CONS_CELL* car = args->list;
        while(pos--) {
            if (car->next != NULL) {
                car = car->next;
            }
        }
        car->next = tail;
    }
    args->length += 1;
}

static void arglist_remove_nth(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    int nth
) {
    if (nth < 0 || args->length <= nth) {
        printf("Internal error: tried to remove non-existing arglist entry\n");
        return;
    }
    CONS_CELL* lastCar = NULL;
    CONS_CELL* car = args->list;
    int index = 0;
    while(index < args->length) {
        if (index == nth) {
            args->length -= 1;
            if (lastCar != NULL) {
                lastCar->next = car->next;
            } else {
                args->list = car->next;
            }
            csound->Free(csound, car);
            return;
        }
        lastCar = car;
        car = car == NULL ? NULL : car->next;
        index += 1;
    }
}

// generic
CSOUND_ORC_ARGUMENTS* new_csound_orc_arguments(CSOUND* csound) {
    CSOUND_ORC_ARGUMENTS* args = csound->Calloc(
        csound,
        sizeof(CSOUND_ORC_ARGUMENTS)
    );
    args->list = NULL;
    args->length = 0;
    args->nth = &arglist_nth;
    args->append = &arglist_append;
    args->remove_nth = &arglist_remove_nth;

    return args;
}



// checks if the output type needs to be inferred
static int has_unknown_output_type(
    CSOUND_ORC_ARGUMENTS* leftSideArgs
) {
    CONS_CELL* car = leftSideArgs->list;
    CSOUND_ORC_ARGUMENT* currentArg = car->value;

    while(currentArg != NULL) {
        if (
            currentArg->cstype == NULL &&
            currentArg->memberType == NULL
        ) {
            return 1;
        }
        car = car->next;
        currentArg = car == NULL ? NULL : car->value;
    }

    return 0;
}

static void initialize_inferred_variables(
    CSOUND* csound,
    TYPE_TABLE* typeTable,
    CSOUND_ORC_ARGUMENTS* leftSideArgs,
    CSOUND_ORC_ARGUMENTS* rightSideArgs,
    OENTRY* oentry
) {
    char* oentryOutypes = oentry->outypes;
    CONS_CELL* car = leftSideArgs->list;
    CSOUND_ORC_ARGUMENT* currentArg = car->value;
    CSOUND_ORC_ARGUMENT* firstRightArg =
      rightSideArgs->length > 0 ?
        rightSideArgs->list->value : NULL;
    CSOUND_ORC_ARGUMENT* secondRightArg;
    char** outtypeList = splitArgs(csound, oentryOutypes);
    int dimensions = -1;
    int index = 0;

    while(currentArg != NULL) {
        if (currentArg->cstype == NULL) {
            char* currentOutArg = convert_internal_to_external(
                csound, outtypeList[index]
            );
            if (*currentOutArg == '.') {
                if (
                    is_internal_array_opcode(oentry->opname) ||
                    !firstRightArg->cstype->userDefinedType
                ) {
                    // array_get/set
                    currentArg->cstype = firstRightArg->cstype;
                    if (strncmp(oentry->opname, "##array_get", 10) == 0) {
                        // array get output is always 1 dimension less
                        currentArg->dimensions = firstRightArg->dimensions - 1;
                    } else {
                        currentArg->dimensions = firstRightArg->dimensions;
                    }


                } else {
                    // member_get/set
                    secondRightArg = rightSideArgs->list->next->value;
                    currentArg->cstype = secondRightArg->memberType;
                    currentArg->dimensions = secondRightArg->memberTypeDimensions;
                }
                dimensions = currentArg->dimensions;
            } else {
                currentArg->cstype = resolve_cstype_from_string(
                    csound,
                    currentOutArg
                );
                if (currentArg->cstype == NULL) {
                    csound->Warning(
                        csound,
                        "Internal: couldn't find type from oentry\n"
                    );
                    csound->Free(csound, currentOutArg);
                    return;
                }
            }

            if (dimensions < 0) {
                dimensions = count_dimensions_from_string(currentOutArg);
            }

            add_arg_to_pool(
                csound,
                typeTable,
                currentArg->text,
                NULL,
                dimensions,
                currentArg->cstype
            );
            csound->Free(csound, currentOutArg);
        }
        car = car->next;
        currentArg = car == NULL ? NULL : car->value;
        index += 1;
    }
}

int verify_opcode_2(
    CSOUND* csound,
    TREE* root,
    TYPE_TABLE* typeTable
) {

    OENTRIES* entries = find_opcode2(csound, root->value->lexeme);

    if (UNLIKELY(entries == NULL || entries->count == 0)) {
        synterr(
            csound,
            Str("Line %d unable to find opcode '%s'"),
            root->line,
            root->value->lexeme
        );

        if (entries != NULL) {
            csound->Free(csound, entries);
        }
        return 0;
    }

    CSOUND_ORC_ARGUMENTS* leftSideArgs = new_csound_orc_arguments(csound);
    CSOUND_ORC_ARGUMENTS* rightSideArgs = new_csound_orc_arguments(csound);

    TREE* left = root->left;
    TREE* right = root->right;
    // print_tree(csound, "before", root);
    rightSideArgs = get_arguments_from_tree(
        csound,
        rightSideArgs,
        right,
        typeTable,
        0
    );

    if (rightSideArgs == NULL) {
        return 0;
    }

    leftSideArgs = get_arguments_from_tree(
        csound,
        leftSideArgs,
        left,
        typeTable,
        1
    );

    if (leftSideArgs == NULL) {
        return 0;
    }

    int needsInference = 0;

    if (
        leftSideArgs->length > 0 &&
        has_unknown_output_type(leftSideArgs)
    ) {
        needsInference = 1;
    }

    if (
        strcmp(root->value->lexeme, "##array_get") == 0 &&
        rightSideArgs->nth(rightSideArgs, 0)->cstype->userDefinedType
    ) {
        csound->Free(csound, root->value->lexeme);
        root->value->lexeme = csound->Strdup(csound, "##array_get_struct");
        entries = find_opcode2(csound, root->value->lexeme);
    }

    OENTRY* oentry = NULL;



    if (strcmp(root->value->lexeme, "##array_set") == 0) {
        CSOUND_ORC_ARGUMENT* firstRightArg =
            rightSideArgs->nth(rightSideArgs, 0);
        if (
            firstRightArg->cstype == (CS_TYPE*) &CS_VAR_TYPE_A &&
            firstRightArg->dimensions == 0
        ) {
            for (int i = 0; i < entries->count; i++) {
                oentry = entries->entries[i];
                if (strcmp(oentry->opname, "##array_set") == 0) {
                    break;
                }
            }
        }
    }

    if (oentry == NULL) {
        oentry = resolve_opcode_with_orc_args(
            csound,
            entries,
            rightSideArgs,
            leftSideArgs,
            typeTable,
            needsInference
        );
    }


    if (UNLIKELY(oentry == NULL)) {
        synterr(
            csound,
            Str("Unable to find opcode entry for \'%s\' "
                "with matching argument types:\n"),
            root->value->lexeme
        );

        char* leftArglist = make_arglist_string(csound, leftSideArgs, 1);
        char* rightArglist = make_arglist_string(csound, rightSideArgs, 1);

        csoundMessage(
            csound,
            Str("Found:\n  %s %s %s\n"),
            leftArglist,
            root->value->lexeme,
            rightArglist
        );

        csound->Free(csound, leftArglist);
        csound->Free(csound, rightArglist);

        csoundMessage(csound, Str("\nCandidates:\n"));

        for (int i = 0; i < entries->count; i++) {
            OENTRY *entry = entries->entries[i];
            csoundMessage(
                csound,
                "  %s %s %s\n",
                entry->outypes,
                entry->opname,
                entry->intypes
            );
        }

        csoundMessage(
            csound,
            Str("\nLine: %d\n"),
            root->line
        );
        // printOrcArgs(rightSideArgs );
        do_baktrace(csound, root->locn);

        return 0;
    }

    if (needsInference) {
        initialize_inferred_variables(
            csound,
            typeTable,
            leftSideArgs,
            rightSideArgs,
            oentry
        );
    }

    root->markup = oentry;
    root->inlist = rightSideArgs;
    root->outlist = leftSideArgs;


    return 1;

}
