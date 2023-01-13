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
extern OENTRIES* find_opcode2(CSOUND*, char*);
extern int is_in_optional_arg(char*);
extern int is_in_var_arg(char*);
extern char* get_expression_opcode_type(CSOUND*, TREE*);
extern char* get_boolean_expression_opcode_type(CSOUND*, TREE*);
// extern char *strsav_string(CSOUND*, ENGINE_STATE*, char*);

// static int exprIdentCnt = 100;

// static char* generateUniqueIdent(CSOUND* csound) {
//     char *ident = (char *)csound->Calloc(csound, 32);
//     snprintf(ident, 32, "__internal__%d", exprIdentCnt);
//     exprIdentCnt += 1;
//     return ident;
// }

static CSOUND_ORC_ARGUMENTS* get_arguments_from_tree(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    TREE* tree,
    TYPE_TABLE* typeTable,
    int shouldAddArgs
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
        printf("\t exprIdent: %s \n", arg->exprIdent);
        printf("\t type: %s \n", arg->cstype->varTypeName);
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

static int check_satisfies_expected_input(
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
        return strchr("aXMyx", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_K)) {
        return strchr("kxXUOJVP", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_B)) {
        return strchr("B", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_b)) {
        return strchr("Bb", *typeIdent) != NULL;
    } else if (arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_L)) {
        return strchr("l", *typeIdent) != NULL;
    } else if (
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_I) ||
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_R) ||
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_P)
    ) {
        return strchr("koXUOJVPicmI", *typeIdent) != NULL;
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
    } else if (
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_I) ||
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_R) ||
        arg->cstype == ((CS_TYPE*) &CS_VAR_TYPE_P)
    ) {
        return strchr("kzXNicmI", *typeIdent) != NULL;
    }

    return 0;

}

// as with 'resolve_opcode_get_outarg', it returns the first match
// inline binary expression tree usually has left and right branches
// for input arguments, so both lists are treated as input arguments
// from left to right
OENTRY* resolve_opcode_from_subexpression(
    CSOUND* csound,
    OENTRIES* entries,
    CSOUND_ORC_ARGUMENTS* subexpression
) {
    for (int i = 0; i < entries->count; i++) {
        OENTRY* temp = entries->entries[i];
        int matches = 0;

        int expectsInputs = temp->intypes != NULL && strlen(temp->intypes) != 0;
        if (!expectsInputs && subexpression->length == 0 && subexpression->length == 0) {
            return temp;
        }

        if (expectsInputs && subexpression->length > 0) {
            char* current = temp->intypes;
            CONS_CELL* cdr = subexpression->list;
            CSOUND_ORC_ARGUMENT* currentArg = cdr->value;
            while (cdr != NULL && *current != '\0') {
                if (check_satisfies_expected_input(currentArg, current)) {
                    matches = 1;
                } else {
                    matches = 0;
                    break;
                }
                cdr = cdr->next;
                currentArg = cdr == NULL ? NULL : cdr->value;
                current++;
            }
        }

        if (matches) {
            return temp;
        }
    }
    return NULL;
}

OENTRY* resolve_opcode_with_orc_args(
    CSOUND* csound,
    OENTRIES* entries,
    CSOUND_ORC_ARGUMENTS* inlist,
    CSOUND_ORC_ARGUMENTS* outlist
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

        if (outlist->length == 0 && !expectsOutputs) {
            outArgsMatch = 1;
        }

        if (!inArgsMatch && expectsInputs && inlist->length > 0) {
            char* current = temp->intypes;
            CONS_CELL* cdr = inlist->list;
            CSOUND_ORC_ARGUMENT* currentArg = cdr->value;
            while (currentArg != NULL && *current != '\0' ) {
                if (check_satisfies_expected_input(currentArg, current)) {
                    inArgsMatch = 1;
                } else {
                    inArgsMatch = 0;
                    break;
                }
                cdr = cdr->next;
                currentArg = cdr == NULL ? NULL : (CSOUND_ORC_ARGUMENT*) cdr->value;
                current++;
            }
        }

        if (!outArgsMatch && expectsOutputs && outlist->length > 0) {
            char* current = temp->outypes;
            CONS_CELL* cdr = outlist->list;
            CSOUND_ORC_ARGUMENT* currentArg = cdr->value;
            while (currentArg != NULL && *current != '\0' ) {
                if (check_satisfies_expected_output(currentArg, current)) {
                    outArgsMatch = 1;
                } else {
                    outArgsMatch = 0;
                    break;
                }
                cdr = cdr->next;
                currentArg = cdr == NULL ? NULL : (CSOUND_ORC_ARGUMENT*) cdr->value;
                current++;
            }
        }

        if (inArgsMatch && outArgsMatch) {
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
    if (tree != NULL) {
        arg->type = tree->type;
        arg->linenum = tree->line;
    }

    arg->dimensions = 0;
    arg->isGlobal = 0;
    arg->isPfield = 0;
    arg->isExpression = 0;
    arg->SubExpression = NULL;
    arg->exprIdent = NULL;
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

// void *add_to_constants_pool(
//     CSOUND *csound,
//     CS_HASH_TABLE *constantsPool,
//     const char *name,
//     MYFLT value
// ) {
//   void *retVal = cs_hash_table_get(csound, constantsPool, (char *)name);
//   if (retVal == NULL) {
//     CS_VAR_MEM *memValue = csound->Calloc(csound, sizeof(CS_VAR_MEM));
//     memValue->varType = (CS_TYPE *)&CS_VAR_TYPE_C;
//     memValue->value = value;
//     cs_hash_table_put(csound, constantsPool, (char *)name, memValue);
//     retVal = cs_hash_table_get(csound, constantsPool, (char *)name);
//   }
//   return retVal;
// }


// static CSOUND_ORC_ARGUMENTS* concat_orc_arguments(
//     CSOUND* csound,
//     CSOUND_ORC_ARGUMENTS* args1,
//     CSOUND_ORC_ARGUMENTS* args2
// ) {

//     if (args1->length > 0) {
//         int pos = args1->length;
//         CONS_CELL* car = args1->list;
//         while(pos--) {
//             if (car->next != NULL) {
//                 car = car->next;
//             }
//         }
//         if (args2->length > 0) {
//             args1->length += args2->length;
//             car->next = args2->list;
//         }
//     }

//     return args1;
// }

static CSOUND_ORC_ARGUMENT* resolve_single_argument_from_tree(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    TREE* tree,
    TYPE_TABLE* typeTable,
    int shouldAddArgs
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
        if (tree->left != NULL && tree->left->value != NULL) {
            isPreparedTree = tree->left->value->lexeme[0] == '#';
            arg->exprIdent = tree->left->value->lexeme;
        }
        arg->isExpression = 1;

        CSOUND_ORC_ARGUMENTS* subExpr = new_csound_orc_arguments(csound);

        arg->SubExpression = get_arguments_from_tree(
            csound,
            subExpr,
            tree->right,
            typeTable,
            shouldAddArgs
        );

        //printOrcArgs(arg->SubExpression);

        char* opname = isBool ? \
            get_boolean_expression_opcode_type(csound, tree) : \
            get_expression_opcode_type(csound, tree);

        OENTRIES* entries = find_opcode2(csound, opname);
        OENTRY* oentry = resolve_opcode_from_subexpression(
            csound,
            entries,
            arg->SubExpression
        );
        // TODO: handle no match
        if (oentry == NULL) {
            printf("NO matching oentry in expr %s\n", opname);
            return 0;
        }

        tree->markup = oentry;
        tree->inlist = arg->SubExpression;
        tree->outlist = arg;
        arg->text = opname; // FIXME: human readable
        if (!isPreparedTree) {
            arg->exprIdent = opname;
        }
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
                arg->exprIdent,
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
        case T_ARRAY_IDENT:
        case T_IDENT: {
            ident = csound->Strdup(csound, tree->value->lexeme);
            if (is_reserved(ident)) {
                arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_R;
                break;
            }
            if (is_label(ident, typeTable->labelList)) {
                arg->cstype = (CS_TYPE*) &CS_VAR_TYPE_L;
                break;
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
                        0,
                        NULL
                    )
                );
                break;
            }
            if (shouldAddArgs && ident[0] != '#') {
                char* annotation = NULL;
                if (tree->value->optype != NULL) {
                    annotation = tree->value->optype;
                }
                var = add_arg_to_pool(
                    csound,
                    typeTable,
                    ident,
                    annotation,
                    0
                );
                arg->cstype = var->varType;
                break;
            }

            var = csoundFindVariableWithName(
                csound,
                typeTable->localPool,
                ident
            );

            if (var != NULL) {
                arg->cstype = var->varType;
                break;
            }

            if (*ident == 'g' || is_reserved(ident)) {
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
                    if (var != NULL) {
                        arg->isGlobal = 1;
                    }
                }
            } else {
                var = csoundFindVariableWithName(
                    csound,
                    typeTable->localPool,
                    tree->value->lexeme
                );
            }
            if (UNLIKELY(var == NULL)) {
                synterr(
                    csound,
                    Str("Variable '%s' used before defined\n"
                        "Line %d\n"
                    ),
                    tree->value->lexeme,
                    tree->line - 1
                );
                return NULL;
            }
        }
    }

    return arg;
}

static CSOUND_ORC_ARGUMENTS* get_arguments_from_tree(
    CSOUND* csound,
    CSOUND_ORC_ARGUMENTS* args,
    TREE* tree,
    TYPE_TABLE* typeTable,
    int shouldAddArgs
) {
    TREE* current = tree;

    while (current != NULL) {
        resolve_single_argument_from_tree(
            csound,
            args,
            current,
            typeTable,
            shouldAddArgs
        );

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

int verify_opcode_2(
    CSOUND* csound,
    TREE* root,
    TYPE_TABLE* typeTable
) {
    if (root->value == NULL) return 0;
    OENTRIES* entries = find_opcode2(csound, root->value->lexeme);
    OENTRY* oentry = entries->entries[0];

    if (UNLIKELY(entries == NULL || entries->count == 0)) {
        synterr(csound, Str("Unable to find opcode with name: %s\n"),
                root->value->lexeme);
        if (entries != NULL) {
            csound->Free(csound, entries);
        }
        return 0;
    }

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

    CSOUND_ORC_ARGUMENTS* rightSideArgs = get_arguments_from_tree(
        csound,
        argsRight,
        right,
        typeTable,
        0
    );
    // printOrcArgs(leftSideArgs);
    // printOrcArgs(rightSideArgs);

    oentry = resolve_opcode_with_orc_args(
        csound,
        entries,
        rightSideArgs,
        leftSideArgs
    );

    if (oentry == NULL) {
        printf("NO matching oentry %s\n", root->value->lexeme);
        return 0;
    }

    root->markup = oentry;
    root->inlist = rightSideArgs;
    root->outlist = leftSideArgs;

    return 1;

}

