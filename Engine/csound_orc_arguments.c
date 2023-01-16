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

extern int pnum(char*);
void do_baktrace(CSOUND *csound, uint64_t files);
extern OENTRIES* find_opcode2(CSOUND*, char*);
extern int is_in_optional_arg(char*);
extern int is_in_var_arg(char*);
extern char* get_expression_opcode_type(CSOUND*, TREE*);
extern char* get_boolean_expression_opcode_type(CSOUND*, TREE*);

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

static void printOrcArgs(
    CSOUND_ORC_ARGUMENTS* args
) {
    if (args->length == 0) {
        printf("== ARGUMENT LIST EMPTY ==\n");
        return;
    }
    CONS_CELL* car = args->list;
    CSOUND_ORC_ARGUMENT* arg;
    int n = 0;
    while(car != NULL) {
        arg = (CSOUND_ORC_ARGUMENT*) car->value;
        printf("== ARGUMENT LIST INDEX %d ==\n", n);
        printf("\t text: %s \n", arg->text);
        printf("\t uid: %s \n", arg->uid);
        printf("\t type: %s \n", arg->cstype == NULL ? arg->text :
         arg->cstype->varTypeName);
        printf("\t isGlobal: %d \n", arg->isGlobal);
        printf("\t dimensions: %d \n", arg->dimensions);
        printf("\t isExpression: %d \n", arg->isExpression);
        car = car->next;
        n += 1;
    }
}

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

static int is_wildcard_type(char* typeIdent) {
  return strchr("?.*", *typeIdent) != NULL;
}

static int is_vararg_input_type(char* typeIdent) {
  return strchr("MNmW", *typeIdent) != NULL;
}

static int is_unary_token_type(int tokenType) {
  return (tokenType == S_UNOT) || \
    (tokenType == S_UMINUS) || \
    (tokenType == S_UPLUS);
}

static int fill_optional_inargs(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    TYPE_TABLE* typeTable,
    OENTRY* candidate
) {
    char temp[6];
    char* current = candidate->intypes;
    CONS_CELL* cdr = args->list;
    CSOUND_ORC_ARGUMENT* currentArg = cdr != NULL ? cdr->value : NULL;


    // fast forward to the end
    while (cdr != NULL && *current != '\0') {
        cdr = cdr->next;
        currentArg = cdr == NULL ? NULL : cdr->value;
        current++;
        while (*current == '[' || *current == ']') {
            current += 1;
        }
    }

    while (cdr == NULL && *current != '\0') {
        // end of explicit input
        // handle optional args
        CSOUND_ORC_ARGUMENT* optArg;
        memset(temp, '\0', 6);
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

        cs_sprintf(temp, "%f", optargValue);
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

static int check_satisfies_expected_input(
    CSOUND_ORC_ARGUMENT* arg,
    char* typeIdent,
    int isArray
) {
    if (is_wildcard_type(typeIdent)) {
        return 1;
    }

    // printf("input: checking %c against %s\n", *typeIdent, arg->text);
    if (
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_C) &&
        strchr("Sf", *typeIdent) == NULL // fewer letters to rule out than include
    ) {
        return 1;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_A)) {
        return strchr("aXMNyx", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_K)) {
        return strchr("kmxzXUOJVPN", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_B)) {
        return strchr("B", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_b)) {
        return strchr("Bb", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_L)) {
        return strchr("l", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_S)) {
        return strchr("SN", *typeIdent) != NULL;
    } else if (
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_I) ||
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_R) ||
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_P)
    ) {
        return isArray ? \
            strchr("icoXUNcmI", *typeIdent) != NULL : \
            strchr("ickoXUOJVPNmIz", *typeIdent) != NULL;
    }

    return 0;

}

static int check_satisfies_expected_output(
    CSOUND_ORC_ARGUMENT* arg,
    char* typeIdent
) {
    if (is_wildcard_type(typeIdent)) {
        return 1;
    }

    // printf("input: checking %c against %s\n", *typeIdent, arg->text);
    if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_C)) {
        return 1;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_A)) {
        return strchr("amXN", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_K)) {
        return strchr("kzXN", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_B)) {
        return strchr("B", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_b)) {
        return strchr("Bb", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_L)) {
        return strchr("l", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_S)) {
        return strchr("SN", *typeIdent) != NULL;
    } else if (
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_I) ||
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_R) ||
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_P)
    ) {
        return strchr("zXNicmI", *typeIdent) != NULL;
    }

    return 0;

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
    int matchFirstArgOnly, // for unexpanded unary trees
    int skipOutargs // for subexpressions
) {
    for (int i = 0; i < entries->count; i++) {
        OENTRY* temp = entries->entries[i];
        int inArgsMatch = 0;
        int outArgsMatch = 0;
        int expectsInputs = temp->intypes != NULL && strlen(temp->intypes) != 0;
        int expectsOutputs = temp->outypes != NULL && strlen(temp->outypes) != 0;

        if (inlist->length == 0 && !expectsInputs) {
            inArgsMatch = 1;
        }

        if (skipOutargs || (outlist->length == 0 && !expectsOutputs)) {
            outArgsMatch = 1;
        }

        if (!inArgsMatch && expectsInputs && inlist->length > 0) {
            char* current = temp->intypes;
            CONS_CELL* cdr = inlist->list;
            CSOUND_ORC_ARGUMENT* currentArg = cdr->value;
            int index = 0;
            while (currentArg != NULL && *current != '\0') {
                while (currentArg->isExpression) {
                    // the last statement is the returned
                    currentArg = currentArg->SubExpression->nth(
                        currentArg->SubExpression,
                        currentArg->SubExpression->length - 1
                    );
                }
                int dimensionsNeeded = currentArg->dimensions - currentArg->dimension;
                int dimensionsFound = 0;

                if (check_satisfies_expected_input(
                    currentArg,
                    current,
                    dimensionsNeeded > 0)
                ) {
                    inArgsMatch = 1;
                } else {
                    inArgsMatch = 0;
                    break;
                }
                if (matchFirstArgOnly && index == 0) {
                    return inArgsMatch ? temp : NULL;
                }

                if (!is_vararg_input_type(current)) {
                    current += 1;
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
                cdr = cdr == NULL ? NULL : cdr->next;
                currentArg = cdr == NULL ? NULL : (CSOUND_ORC_ARGUMENT*) cdr->value;
                index += 1;
            }
        }

        if (inArgsMatch && !outArgsMatch && expectsOutputs && outlist->length > 0) {
            char* current = temp->outypes;
            CONS_CELL* cdr = outlist->list;
            CSOUND_ORC_ARGUMENT* currentArg = cdr->value;
            while (currentArg != NULL && *current != '\0' ) {
                int isSyntheticVar = *currentArg->text == '#';
                int dimensionsNeeded = currentArg->dimensions;
                int dimensionsFound = 0;
                if (check_satisfies_expected_output(currentArg, current)) {
                    outArgsMatch = 1;
                } else {
                    outArgsMatch = 0;
                    break;
                }
                current += 1;
                while (*current == '[' || *current == ']') {
                    if (*current == '[') {
                        dimensionsFound += 1;
                    }
                    current += 1;
                }
                if (dimensionsNeeded != dimensionsFound) {

                    if (isSyntheticVar && dimensionsNeeded < dimensionsFound) {
                        // mismatch in dimensions from synthetic vars
                        // is due to the fact that just walking the tree
                        // it's unable to determine and assign correct
                        // dimensions value. Given the rate resolved fully
                        // thus far, we are safe to fix this here.
                        // ex. `ians = iS + iT where iS and iT are arrays
                        // sadly there's no way around this.
                        currentArg->dimensions = dimensionsFound;
                        CS_VARIABLE* var = csoundFindVariableWithName(
                            csound,
                            typeTable->localPool,
                            currentArg->text
                        );

                        if (var != NULL) {
                            var->dimensions = currentArg->dimensions;
                        } else {
                            printf(Str("error: couldn't find synthetic arg"
                                        " while resolving input args\n"));
                        }
                        outArgsMatch = 1;
                    } else {
                        outArgsMatch = 0;
                        break;
                    }

                }
                cdr = cdr->next;
                currentArg = cdr == NULL ? NULL : (CSOUND_ORC_ARGUMENT*) cdr->value;
            }
        }

        if (
            inArgsMatch &&
            outArgsMatch &&
            fill_optional_inargs(
                csound,
                inlist,
                typeTable,
                temp
            )
        ) {
            return temp;
        }
        // printf("entry doesn't match in: %s out: %s\n", temp->intypes, temp->outypes);
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
    arg->dimension = 0;
    arg->isGlobal = 0;
    arg->isPfield = 0;
    arg->isExpression = 0;
    arg->SubExpression = NULL;
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
    int dimensions
) {
    CS_TYPE* type;
    CS_VAR_POOL* pool;
    void* typeArg = NULL;

    char* varName_ = varName;
    if (*varName_ == '#') {
        varName_ += 1;
    }

    int isGlobal = *varName_ == 'g';
    pool = isGlobal ? typeTable->globalPool : typeTable->localPool;

    CS_VARIABLE* var = csoundFindVariableWithName(
        csound,
        pool,
        varName
    );

    if (var != NULL) {
        var->refCount += 1;
    } else {
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

            if (type == NULL) {
                type = (CS_TYPE*) &CS_VAR_TYPE_L;
            }
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
    }

    return var;

}

static CSOUND_ORC_ARGUMENT* resolve_single_argument_from_tree(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    TREE* tree,
    TYPE_TABLE* typeTable,
    int isAssignee
) {
    char* ident;
    CS_VARIABLE* var = NULL;
    CSOUND_ORC_ARGUMENT* arg = new_csound_orc_argument(
        csound,
        args,
        tree,
        typeTable
    );

    args->append(csound, args, arg);

    if (is_expression_node(tree) || is_boolean_expression_node(tree)) {
        int isBool = is_boolean_expression_node(tree);
        int isPreparedTree = 0;
        int isGlobal = 0;
        int isFunCall = tree->type == T_FUNCTION;

        if (tree->left != NULL && tree->left->value != NULL) {
            isPreparedTree = tree->left->value->lexeme[0] == '#';
        }
        arg->isExpression = 1;

        CSOUND_ORC_ARGUMENTS* subExpr = new_csound_orc_arguments(csound);

        // unary expressions in unexpanded trees need to account
        // for the fact that it only has one node present out of actual two
        int isUnary = is_unary_token_type(tree->type);

        // T_ARRAY as expression
        int isArray = tree->type == T_ARRAY;

        if (isArray) {
            var = csoundFindVariableWithName(
                csound,
                typeTable->localPool,
                tree->left->value->lexeme
            );
            var = var == NULL ? csoundFindVariableWithName(
                csound,
                typeTable->globalPool,
                tree->left->value->lexeme
            ) : var;

            if (var == NULL) {
                synterr(
                    csound,
                    Str("Line %d array-identifier '%s' used "
                        "before it was defined."),
                    tree->line,
                    tree->left->value->lexeme
                );
                return NULL;
            }

            if (var->dimensions == 0) {
                synterr(
                    csound,
                    Str("Line %d variable '%s' isn't an array"),
                    tree->line,
                    tree->left->value->lexeme
                );
                return NULL;
            }
        }

        if (!isPreparedTree) {
            subExpr = get_arguments_from_tree(
                csound,
                subExpr,
                tree->left,
                typeTable,
                isAssignee
            );
        }

        if (!isPreparedTree) {
            arg->SubExpression = get_arguments_from_tree(
                csound,
                subExpr,
                tree->right,
                typeTable,
                isAssignee
            );
        }

        if (!isPreparedTree && isArray) {
            // attach the array-get indicies to
            // resolve which dimension we are refering to
            tree->left->right = tree->right;
            arg->SubExpression = get_arguments_from_tree(
                csound,
                subExpr,
                tree->left,
                typeTable,
                isAssignee
            );
            // remove the pointer to prevent incorrect behavior later
            tree->left->right = NULL;
        } else {
            arg->SubExpression = get_arguments_from_tree(
                csound,
                subExpr,
                tree->right,
                typeTable,
                isAssignee
            );
        }

        char* opname = NULL;

        if (isFunCall) {
            opname = tree->value->lexeme;
        } else if (isBool) {
            opname = get_boolean_expression_opcode_type(csound, tree);
        } else {
            opname = get_expression_opcode_type(csound, tree);
        }

        OENTRIES* entries = find_opcode2(csound, opname);

        if (UNLIKELY(entries == NULL || entries->count == 0)) {
            synterr(
                csound,
                Str("Line %d unable to find opcode '%s'"),
                tree->line - 1,
                opname
            );
            if (entries != NULL) {
                csound->Free(csound, entries);
            }
            return NULL;
        }

        OENTRY* oentry = resolve_opcode_with_orc_args(
            csound,
            entries,
            arg->SubExpression,
            NULL,
            typeTable,
            !isPreparedTree && (isUnary),
            1
        );

        if (oentry == NULL) {
            synterr(
                csound,
                Str("Line %d no arguments match opcode '%s'"),
                tree->line - 1,
                opname
            );
            oentry = resolve_opcode_with_orc_args(
            csound,
            entries,
            arg->SubExpression,
            NULL,
            typeTable,
            !isPreparedTree && (isUnary),
            1
        );
            return NULL;
        }

        tree->markup = oentry;
        tree->inlist = arg->SubExpression;
        tree->outlist = arg;
        arg->text = opname;
        arg->cstype = csoundFindStandardTypeWithChar(*oentry->outypes);

        if (isPreparedTree) {
            char* annotation = NULL;
            if (tree->value != NULL && tree->value->optype != NULL) {
                annotation = tree->value->optype;
            }
            var = csoundCreateVariable(
                csound,
                csound->typePool,
                arg->cstype,
                arg->text,
                0,
                annotation
            );
            // TODO handle global vars
            csoundAddVariable(csound, typeTable->localPool, var);
        }

        return arg;
    }

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
        case T_ARRAY_IDENT: {
            if (
                tree->right != NULL &&
                tree->right->value != NULL &&
                *tree->right->value->lexeme == '['
            ) {
                TREE* currentBracketNode = tree->right;

                while (currentBracketNode != NULL) {
                    arg->dimensions += 1;
                    currentBracketNode = currentBracketNode->right;
                }
            }
            // lack of break is intended
        }
        case T_IDENT: {
            ident = csound->Strdup(csound, tree->value->lexeme);
            if (is_reserved(ident)) {
                arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_R;
                break;
            }
            if (is_label(ident, typeTable->labelList)) {
                arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_L;
                return arg;
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

            if (isAssignee && ident[0] != '#') {
                char* annotation = NULL;
                if (
                    tree->value != NULL &&
                    tree->value->optype != NULL
                ) {
                    annotation = tree->value->optype;
                }

                CS_VAR_POOL* pool = arg->isGlobal ? \
                    typeTable->globalPool : typeTable->localPool;

                arg->cstype = csoundFindStandardTypeWithChar(ident[0]);
                var = add_arg_to_pool(
                    csound,
                    typeTable,
                    ident,
                    annotation,
                    arg->dimensions
                );
                arg->cstype = var->varType;
                arg->dimensions = var->dimensions;
                // count the dimension nesting
                if (arg->dimensions > 0) {
                    TREE* arrayIndexGetLeaf = tree->right;
                    while (arrayIndexGetLeaf != NULL) {
                        arg->dimension += 1;
                        arrayIndexGetLeaf = arrayIndexGetLeaf->next;
                    }
                }
                goto count_dim;
            }
            if (var == NULL) {
                var = csoundFindVariableWithName(
                    csound,
                    arg->isGlobal ? typeTable->globalPool : typeTable->localPool,
                    ident
                );
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

            count_dim:
            if (tree->value != NULL && strcmp(tree->value->lexeme, "kans") == 0) {
                printf("kans  %p!\n", &var);
            }
            // a variable can be initialized from a pre-defined
            // array at a different dimension, so we start counting
            // from where the variable left off
            arg->dimension = var->dimension;
            // count the dimension nesting
            if (arg->dimensions > 0 && !isAssignee) {
                TREE* arrayIndexGetLeaf = tree->right;
                while (arrayIndexGetLeaf != NULL) {
                    arg->dimension += 1;
                    arrayIndexGetLeaf = arrayIndexGetLeaf->next;
                }
            }

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
    CONS_CELL* tail = csound->Malloc(csound, sizeof(CONS_CELL));
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

// generic
CSOUND_ORC_ARGUMENTS* new_csound_orc_arguments(CSOUND* csound) {
    CSOUND_ORC_ARGUMENTS* args = csound->Malloc(
        csound,
        sizeof(CSOUND_ORC_ARGUMENTS)
    );
    args->list = NULL;
    args->length = 0;
    args->nth = &arglist_nth;
    args->append = &arglist_append;

    return args;
}

// for exception cases where the opcode search
// will find wrong onetry, ex. when lexeme
// str not giving all the required hints for
// correct resolution.
OENTRY* maybe_fix_opcode_resolution(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* argsLeft,
    CSOUND_ORC_ARGUMENTS* argsRight,
    TYPE_TABLE* typeTable,
    char* opcodeString
) {
    OENTRIES* entries;
    OENTRY* oentry;
    char tmp[24];

    if (
        *opcodeString == '=' &&
        argsLeft->length == 1
    ) {
        CSOUND_ORC_ARGUMENT* outarg = argsLeft->list->value;
        if (outarg->dimensions > 0) {
            return find_opcode(csound, "##array_assign");
        }
    }

    return NULL;
}

int verify_opcode_2(
    CSOUND* csound,
    TREE* root,
    TYPE_TABLE* typeTable
) {
    if (UNLIKELY(root->value == NULL)) {
        printf("SHOULD NOT HAPPEN!\n");
        return 0;
    }
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

    OENTRY* oentry = entries->entries[0];

    CSOUND_ORC_ARGUMENTS* argsLeft = new_csound_orc_arguments(csound);
    CSOUND_ORC_ARGUMENTS* argsRight = new_csound_orc_arguments(csound);
    TREE* left = root->left;
    TREE* right = root->right;

    CSOUND_ORC_ARGUMENTS* leftSideArgs = get_arguments_from_tree(
        csound,
        argsLeft,
        left,
        typeTable,
        1
    );

    if (leftSideArgs == NULL) {
        return 0;
    }

    CSOUND_ORC_ARGUMENTS* rightSideArgs = get_arguments_from_tree(
        csound,
        argsRight,
        right,
        typeTable,
        0
    );

    if (rightSideArgs == NULL) {
        return 0;
    }
    // printOrcArgs(leftSideArgs);
    // printOrcArgs(rightSideArgs);


    if ((oentry = maybe_fix_opcode_resolution(
            csound,
            leftSideArgs,
            rightSideArgs,
            typeTable,
            root->value->lexeme
        )) == NULL) {
        oentry = resolve_opcode_with_orc_args(
            csound,
            entries,
            rightSideArgs,
            leftSideArgs,
            typeTable,
            0,
            0
        );
    }

    if (UNLIKELY(oentry == NULL)) {
        // TODO: create nicer left/right strings with all args
        int i;
        synterr(
            csound,
            Str("Unable to find opcode entry for \'%s\' "
                "with matching argument types:\n"),
            root->value->lexeme
        );
        csoundMessage(
            csound,
            Str("Found:\n  %s %s %s\n"),
            root->left != NULL && root->left->value != NULL ? \
                root->left->value->lexeme : "",
            root->value->lexeme,
            root->right != NULL && root->right->value != NULL ? \
                root->right->value->lexeme : ""
        );

        csoundMessage(csound, Str("\nCandidates:\n"));

        for (i = 0; i < entries->count; i++) {
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
        do_baktrace(csound, root->locn);

        return 0;
    }

    root->markup = oentry;
    root->inlist = rightSideArgs;
    root->outlist = leftSideArgs;

    return 1;

}

