 /*
    csound_orc_optimizee.c:

    Copyright (C) 2006
    Steven Yi

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

#include "csoundCore.h"
#include "csound_orc.h"

static TREE * create_fun_token(CSOUND *csound, TREE *right, char *fname)
{
    TREE *ans;
    ans = (TREE*)csound->Malloc(csound, sizeof(TREE));
    if (UNLIKELY(ans == NULL)) exit(1);
    ans->type = T_FUNCTION;
    ans->value = make_token(csound, fname);
    ans->value->type = T_FUNCTION;
    ans->left = NULL;
    ans->right = right;
    ans->next = NULL;
    ans->len = 0;
    ans->markup = NULL;
    ans->rate = -1;
    return ans;
}

static TREE * optimize_ifun(CSOUND *csound, TREE *root)
{
    /* print_tree(csound, "optimize_ifun: before", root); */
    switch(root->right->type) {
        case INTEGER_TOKEN:
        case NUMBER_TOKEN:       /* i(num)    -> num      */
            // FIXME - reinstate optimization after implementing get_type(varname)
            //    case T_IDENT_I:          /* i(ivar)   -> ivar     */
            //    case T_IDENT_GI:         /* i(givar)  -> givar    */
            //    case T_IDENT_P:          /* i(pN)     -> pN       */
            root = root->right;
            break;
            //    case T_IDENT_K:          /* i(kvar)   -> i(kvar)  */
            //    case T_IDENT_GK:         /* i(gkvar)  -> i(gkvar) */
            //      break;
        case T_FUNCTION:         /* i(fn(x))  -> fn(i(x)) */
        {
            TREE *funTree = root->right;
            funTree->right = create_fun_token(csound, funTree->right, "i");
            root = funTree;
        }
            break;
        default:                 /* i(A op B) -> i(A) op i(B) */
            if(root->right->left != NULL)
                root->right->left = create_fun_token(csound,
                                                     root->right->left, "i");
            if(root->right->right != NULL)
                root->right->right = create_fun_token(csound,
                                                      root->right->right, "i");
            root->right->next = root->next;
            root = root->right;
            break;
    }
    /* print_tree(csound, "optimize_ifun: after", root); */
    return root;
}

/** Verifies and optimise; constant fold and opcodes and args are correct*/
static TREE * verify_tree1(CSOUND *csound, TREE *root)
{
    TREE *ans, *last;
    double lval, rval;
    //csound->Message(csound, "Verifying AST (NEED TO IMPLEMENT)\n");
    //print_tree(csound, "Verify", root);
//    if (root->right && root->right->type != T_INSTLIST) {
    if (root->right) {
        if (root->type == T_OPCALL || root->type == T_OPCALL) {
            last = root->right;
            while (last->next) {
                /* we optimize the i() functions in the opcode */
                if (last->next->type == T_FUNCTION &&
                    (strcmp(last->next->value->lexeme, "i") == 0)) {
                    TREE *temp = optimize_ifun(csound, last->next);
                    temp->next = last->next->next;
                    last->next = temp;
                }
                last = last->next;
            }
        }
        if (root->right->type == T_FUNCTION &&
            (strcmp(root->right->value->lexeme, "i") == 0)) {  /* i() function */
            root->right = optimize_ifun(csound, root->right);
        }
        last = root->right;
        while (last->next) {
            last->next = verify_tree1(csound, last->next);
            last = last->next;
        }
        root->right = verify_tree1(csound, root->right);
        if (root->left) {
            if (root->left->type == T_FUNCTION &&
                (strcmp(root->left->value->lexeme, "i") == 0)) {  /* i() function */
                root->left = optimize_ifun(csound, root->left);
            }
            root->left= verify_tree1(csound, root->left);
            if ((root->left->type  == INTEGER_TOKEN ||
                 root->left->type  == NUMBER_TOKEN) &&
                (root->right->type == INTEGER_TOKEN ||
                 root->right->type == NUMBER_TOKEN)) {
                    //print_tree(csound, "numerical case\n", root);
                    lval = (root->left->type == INTEGER_TOKEN ?
                            (double)root->left->value->value :
                            root->left->value->fvalue);
                    rval = (root->right->type == INTEGER_TOKEN ?
                            (double)root->right->value->value :
                            root->right->value->fvalue);
                    ans = root->left;
                    /* **** Something wrong here --
                       subtraction confuses memory **** */
                    switch (root->type) {
                        case '+':
                            ans->type = ans->value->type = NUMBER_TOKEN;
                            ans->value->fvalue = lval+rval;
                            ans->value->lexeme =
                              (char*)csound->
                              ReAlloc(csound, ans->value->lexeme, 24);
                            CS_SPRINTF(ans->value->lexeme, "%f", ans->value->fvalue);
                            ans->next = root->next;
                            //Memory leak!!
                            //csound->Free(csound, root); mfree(csound root->right);
                            return ans;
                        case '-':
                            ans->type = ans->value->type = NUMBER_TOKEN;
                            ans->value->fvalue = lval-rval;
                            ans->value->lexeme =
                              (char*)csound->
                              ReAlloc(csound, ans->value->lexeme, 24);
                            CS_SPRINTF(ans->value->lexeme, "%f", ans->value->fvalue);
                            ans->next = root->next;
                            //Memory leak!!
                            //csound->Free(csound, root); mfree(csound, root->right);
                            return ans;
                        case '*':
                            ans->type = ans->value->type = NUMBER_TOKEN;
                            ans->value->fvalue = lval*rval;
                            ans->value->lexeme =
                            (char*)csound->ReAlloc(csound, ans->value->lexeme, 24);
                            CS_SPRINTF(ans->value->lexeme, "%f", ans->value->fvalue);
                            ans->next = root->next;
                            //Memory leak!!
                            //csound->Free(csound, root); mfree(csound, root->right);
                            return ans;
                        case '/':
                            ans->type = ans->value->type = NUMBER_TOKEN;
                            ans->value->fvalue = lval/rval;
                            ans->value->lexeme =
                            (char*)csound->ReAlloc(csound, ans->value->lexeme, 24);
                            CS_SPRINTF(ans->value->lexeme, "%f", ans->value->fvalue);
                            ans->next = root->next;
                            //Memory leak!!
                            //csound->Free(csound, root); mfree(csound, root->right);
                            return ans;
                            /* case S_NEQ: */
                            /*   break; */
                            /* case S_AND: */
                            /*   break; */
                            /* case S_OR: */
                            /*   break; */
                            /* case S_LT: */
                            /*   break; */
                            /* case S_LE: */
                            /*   break; */
                            /* case S_EQ: */
                            /*   break; */
                            /* case S_GT: */
                            /*   break; */
                            /* case S_GE: */
                            /*   break; */
                        default: break;
                    }
                }
        }
        else if (root->right->type == INTEGER_TOKEN ||
                 root->right->type == NUMBER_TOKEN) {
            switch (root->type) {
                case S_UMINUS:
                    /*print_tree(csound, "root", root);*/
                    ans = root->right;
                    ans->value->fvalue =
                    -(ans->type==INTEGER_TOKEN ? (double)ans->value->value
                      : ans->value->fvalue);
                    ans->value->lexeme =
                    (char*)csound->ReAlloc(csound, ans->value->lexeme, 24);
                    CS_SPRINTF(ans->value->lexeme, "%f", ans->value->fvalue);
                    ans->type = ans->value->type = NUMBER_TOKEN;
                    //print_tree(csound, "ans", ans);
                    ans->next = root->next;
                    return ans;
                default:
                    break;
            }
        }
    }
    return root;
}

/* Optimizes tree (expressions, etc.) */
TREE * csound_orc_optimize(CSOUND *csound, TREE *root)
{
    TREE *original=root, *last = NULL;
    while (root) {
        TREE *xx = verify_tree1(csound, root);
        if (xx != root) {
            xx->next = root->next;
            if (last) last->next = xx;
            else original = xx;
        }
        last = root;
        root = root->next;
    }
    return original;
}
