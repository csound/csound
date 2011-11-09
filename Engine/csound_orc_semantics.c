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
#include "csoundCore.h"
#include "csound_orc.h"
#include "namedins.h"
#include "parse_param.h"

char *csound_orcget_text ( void *scanner );

extern  char argtyp2(CSOUND*, char*);
extern  int tree_arg_list_count(TREE *);
void print_tree(CSOUND *, char *, TREE *);
/* TREE* force_rate(TREE* a, char t) */
/* {                               /\* Ensure a is of type t *\/ */
/*     return a; */
/* } */


/** Verifies and optimise; constant fold and opcodes and args are correct*/
TREE * verify_tree(CSOUND *csound, TREE *root) 
{
    TREE* ans;
    double lval, rval;
    //csound->Message(csound, "Verifying AST (NEED TO IMPLEMENT)\n");
    //    print_tree(csound, "Verify", root);
    if (root==NULL) return NULL;
    if (root->right) {
      root->right = verify_tree(csound, root->right);
      if (root->left)  {
        root->left= verify_tree(csound, root->left);
        if ((root->left->type  == T_INTGR || root->left->type  == T_NUMBER) &&
            (root->right->type == T_INTGR || root->right->type == T_NUMBER)) {
          lval = (root->left->type == T_INTGR ?
                  (double)root->left->value->value :root->left->value->fvalue);
          rval = (root->right->type == T_INTGR ?
                  (double)root->right->value->value :root->left->value->fvalue);
          ans = root->left;
          ans->type = ans->value->type = T_NUMBER;
          /* **** Something wrong here -- subtractuon confuses memory **** */
          switch (root->type) {
          case S_PLUS:
            ans->value->fvalue = lval+rval;
            ans->value->lexeme = (char*)mrealloc(csound, ans->value->lexeme, 24);
            sprintf(ans->value->lexeme, "%f", ans->value->fvalue);
            //Memory leak!! 
            //mfree(csound, root); mfree(csound root->right);
            return ans;
          case S_MINUS:
            ans->value->fvalue = lval-rval;
            ans->value->lexeme = (char*)mrealloc(csound, ans->value->lexeme, 24);
            sprintf(ans->value->lexeme, "%f", ans->value->fvalue);
            //Memory leak!! 
            //mfree(csound, root); mfree(csound, root->right);
            return ans;
          case S_TIMES:
            ans->value->fvalue = lval*rval;
            ans->value->lexeme = (char*)mrealloc(csound, ans->value->lexeme, 24);
            sprintf(ans->value->lexeme, "%f", ans->value->fvalue);
            //Memory leak!! 
            //mfree(csound, root); mfree(csound, root->right);
            return ans;
          case S_DIV:
            ans->value->fvalue = lval/rval;
            ans->value->lexeme = (char*)mrealloc(csound, ans->value->lexeme, 24);
            sprintf(ans->value->lexeme, "%f", ans->value->fvalue);
            //Memory leak!! 
            //mfree(csound, root); mfree(csound, root->right);
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
      else if (root->right->type == T_INTGR || root->right->type == T_NUMBER) {
        switch (root->type) {
        case S_UMINUS:
          print_tree(csound, "root", root);
          ans = root->right;
          ans->value->fvalue = -(ans->type == T_INTGR ? ans->value->value
                                 : ans->value->fvalue);
          ans->value->lexeme = (char*)mrealloc(csound, ans->value->lexeme, 24);
          sprintf(ans->value->lexeme, "%f", ans->value->fvalue);
          ans->type = ans->value->type = T_NUMBER;
          print_tree(csound, "ans", ans);
          return ans;
        default:
          break;
        }
      }
    }
    return root;
}


/* BISON PARSER FUNCTION */
int csound_orcwrap()
{
    printf("END OF INPUT\n");
    return (1);
}

/* BISON PARSER FUNCTION */
void csound_orcerror(PARSE_PARM *pp, void *yyscanner,
                     CSOUND *csound, TREE *astTree, char *str)
{
    //??    extern int yyline;
    //    extern char* buffer;

    csound->Message(csound, Str("error: %s (token \"%s\")"),
                    str, csound_orcget_text(yyscanner));
    csound->Message(csound, Str(" line %d: %s"),
                    csound_orcget_lineno(yyscanner), pp->buffer); // buffer has \n at end
}

/**
 * Appends TREE * node to TREE * node using ->next field in struct; walks
 * down the linked list to append at end; checks for NULL's and returns
 * appropriate nodes
 */
TREE* appendToTree(CSOUND * csound, TREE *first, TREE *newlast) {
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
     * useful; the number 400 is arbitrary, chosen as it seemed to be a value
     * higher than all the type numbers that were being printed out
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
TREE* make_node(CSOUND *csound, int type, TREE* left, TREE* right)
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
    ans->next = NULL;
    ans->len = 2;
    ans->rate = -1;
    return ans;
}

TREE* make_leaf(CSOUND *csound, int type, ORCTOKEN *v)
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
    return ans;
}

/** Utility function to create assignment statements
 *  Replaces = with correct version for args
 */
char* get_assignment_type(CSOUND *csound, char * ans, TREE* arg1) {
    char c = argtyp2(csound, ans);
    char* str = (char*)mcalloc(csound, 65);

    switch (c) {
    case 'S':
      strcpy(str, "strcpy");
      break;
    case 'a':
      c = argtyp2(csound, arg1->value->lexeme);
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
    case S_COM:
      csound->Message(csound,"S_COM:\n"); break;
    case S_Q:
      csound->Message(csound,"S_Q:\n"); break;
    case S_COL:
      csound->Message(csound,"S_COL:\n"); break;
    case S_NOT:
      csound->Message(csound,"S_NOT:\n"); break;
    case S_PLUS:
      csound->Message(csound,"S_PLUS:\n"); break;
    case S_MINUS:
      csound->Message(csound,"S_MINUS:\n"); break;
    case S_TIMES:
      csound->Message(csound,"S_TIMES:\n"); break;
    case S_DIV:
      csound->Message(csound,"S_DIV:\n"); break;
    case S_MOD:
      csound->Message(csound,"S_MOD:\n"); break;
    case S_POW:
      csound->Message(csound,"S_POW:\n"); break;
    case S_NL:
      csound->Message(csound,"S_NL:\n"); break;
    case S_LB:
      csound->Message(csound,"S_LB:\n"); break;
    case S_RB:
      csound->Message(csound,"S_RB:\n"); break;
    case S_NEQ:
      csound->Message(csound,"S_NEQ:\n"); break;
    case S_AND:
      csound->Message(csound,"S_AND:\n"); break;
    case S_OR:
      csound->Message(csound,"S_OR:\n"); break;
    case S_LT:
      csound->Message(csound,"S_LT:\n"); break;
    case S_LE:
      csound->Message(csound,"S_LE:\n"); break;
    case S_EQ:
      csound->Message(csound,"S_EQ:\n"); break;
    case S_ASSIGN:
      csound->Message(csound,"S_ASSIGN:\n"); break;
    case S_TASSIGN:
      csound->Message(csound,"S_TASSIGN:\n"); break;
    case S_TABREF:
      csound->Message(csound,"S_TABREF:\n"); break;
    case S_GT:
      csound->Message(csound,"S_GT:\n"); break;
    case S_GE:
      csound->Message(csound,"S_GE:\n"); break;
    case T_LABEL:
      csound->Message(csound,"T_LABEL: %s\n", l->value->lexeme); break;
    case T_IF:
      csound->Message(csound,"T_IF:\n"); break;
    case T_THEN:
          csound->Message(csound,"T_THEN:\n"); break;
    case T_ITHEN:
          csound->Message(csound,"T_ITHEN:\n"); break;
    case T_KTHEN:
          csound->Message(csound,"T_KTHEN:\n"); break;
    case T_ELSEIF:
          csound->Message(csound,"T_ELSEIF:\n"); break;
    case T_ELSE:
          csound->Message(csound,"T_ELSE:\n"); break;
    case T_UNTIL:
          csound->Message(csound,"T_UNTIL:\n"); break;
    case T_DO:
          csound->Message(csound,"T_DO:\n"); break;
    case T_OD:
          csound->Message(csound,"T_OD:\n"); break;
    case T_GOTO:
      csound->Message(csound,"T_GOTO:\n"); break;
    case T_IGOTO:
      csound->Message(csound,"T_IGOTO:\n"); break;
    case T_KGOTO:
      csound->Message(csound,"T_KGOTO:\n"); break;
    case T_SRATE:
      csound->Message(csound,"T_SRATE:\n"); break;
    case T_KRATE:
      csound->Message(csound,"T_KRATE:\n"); break;
    case T_KSMPS:
      csound->Message(csound,"T_KSMPS:\n"); break;
    case T_NCHNLS:
      csound->Message(csound,"T_NCHNLS:\n"); break;
    case T_NCHNLSI:
      csound->Message(csound,"T_NCHNLSI:\n"); break;
    case T_INSTR:
      csound->Message(csound,"T_INSTR:\n"); break;
    case T_STRCONST:
      csound->Message(csound,"T_STRCONST: %s\n", l->value->lexeme); break;
    case T_IDENT:
      csound->Message(csound,"T_IDENT: %s\n", l->value->lexeme); break;
    case T_IDENT_I:
      csound->Message(csound,"IDENT_I: %s\n", l->value->lexeme); break;
    case T_IDENT_GI:
      csound->Message(csound,"IDENT_GI: %s\n", l->value->lexeme); break;
    case T_IDENT_K:
      csound->Message(csound,"IDENT_K: %s\n", l->value->lexeme); break;
    case T_IDENT_GK:
      csound->Message(csound,"IDENT_GK: %s\n", l->value->lexeme); break;
    case T_IDENT_A:
      csound->Message(csound,"IDENT_A: %s\n", l->value->lexeme); break;
    case T_IDENT_GA:
      csound->Message(csound,"IDENT_GA: %s\n", l->value->lexeme); break;
    case T_IDENT_S:
      csound->Message(csound,"IDENT_S: %s\n", l->value->lexeme); break;
    case T_IDENT_GS:
      csound->Message(csound,"IDENT_GS: %s\n", l->value->lexeme); break;
    case T_IDENT_T:
      csound->Message(csound,"IDENT_T: %s\n", l->value->lexeme); break;
    case T_IDENT_GT:
      csound->Message(csound,"IDENT_GT: %s\n", l->value->lexeme); break;
    case T_IDENT_W:
      csound->Message(csound,"IDENT_W: %s\n", l->value->lexeme); break;
    case T_IDENT_GW:
      csound->Message(csound,"IDENT_GW: %s\n", l->value->lexeme); break;
    case T_IDENT_F:
      csound->Message(csound,"IDENT_F: %s\n", l->value->lexeme); break;
    case T_IDENT_GF:
      csound->Message(csound,"IDENT_GF: %s\n", l->value->lexeme); break;
    case T_IDENT_P:
      csound->Message(csound,"IDENT_P: %s\n", l->value->lexeme); break;
    case T_IDENT_B:
      csound->Message(csound,"IDENT_B: %s\n", l->value->lexeme); break;
    case T_IDENT_b:
      csound->Message(csound,"IDENT_b: %s\n", l->value->lexeme); break;
    case T_INTGR:
      csound->Message(csound,"T_INTGR: %d\n", l->value->value); break;
    case T_NUMBER:
      csound->Message(csound,"T_NUMBER: %f\n", l->value->fvalue); break;
    case S_ANDTHEN:
      csound->Message(csound,"S_ANDTHEN:\n"); break;
    case S_APPLY:
      csound->Message(csound,"S_APPLY:\n"); break;
    case T_OPCODE0:
      csound->Message(csound,"T_OPCODE0: %s\n", l->value->lexeme); break;
    case T_OPCODE:
      csound->Message(csound,"T_OPCODE: %s\n", l->value->lexeme); break;
    case T_FUNCTION:
      csound->Message(csound,"T_FUNCTION: %s\n", l->value->lexeme); break;
    case S_UMINUS:
        csound->Message(csound,"S_UMINUS:\n"); break;
    case T_INSTLIST:
        csound->Message(csound,"T_INSTLIST:\n"); break;
    default:
      csound->Message(csound,"t:%d\n", l->type);
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
    case S_COM:
      csound->Message(csound,"name=\"S_COM\""); break;
    case S_Q:
      csound->Message(csound,"name=\"S_Q\""); break;
    case S_COL:
      csound->Message(csound,"name=\"S_COL\""); break;
    case S_NOT:
      csound->Message(csound,"name=\"S_NOT\""); break;
    case S_PLUS:
      csound->Message(csound,"name=\"S_PLUS\""); break;
    case S_MINUS:
      csound->Message(csound,"name=\"S_MINUS\""); break;
    case S_TIMES:
      csound->Message(csound,"name=\"S_TIMES\""); break;
    case S_DIV:
      csound->Message(csound,"name=\"S_DIV\""); break;
    case S_MOD:
      csound->Message(csound,"name=\"S_MOD\""); break;
    case S_POW:
      csound->Message(csound,"name=\"S_POW\""); break;
    case S_NL:
      csound->Message(csound,"name=\"S_NL\""); break;
    case S_LB:
      csound->Message(csound,"name=\"S_LB\""); break;
    case S_RB:
      csound->Message(csound,"name=\"S_RB\""); break;
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
    case S_ASSIGN:
      csound->Message(csound,"name=\"S_ASSIGN\""); break;
    case S_TASSIGN:
      csound->Message(csound,"name=\"S_TASSIGN\""); break;
    case S_TABREF:
      csound->Message(csound,"name=\"S_TABREF\""); break;
    case S_GT:
      csound->Message(csound,"name=\"S_GT\""); break;
    case S_GE:
      csound->Message(csound,"name=\"S_GE\""); break;
    case S_BITOR:
      csound->Message(csound,"name=\"S_BITOR\""); break;
    case S_BITAND:
      csound->Message(csound,"name=\"S_BITAND\""); break;
    case S_BITSHR:
      csound->Message(csound,"name=\"S_BITSHR\""); break;
    case S_BITSHL:
      csound->Message(csound,"name=\"S_BITSHL\""); break;
    case S_NEQV:
      csound->Message(csound,"name=\"S_NEQV\""); break;
    case S_BITNOT:
      csound->Message(csound,"name=\"S_BITNOT\""); break;
    case T_LABEL:
      csound->Message(csound,"name=\"T_LABEL\" label=\"%s\"",
                      l->value->lexeme); break;
    case T_IF:
      csound->Message(csound,"name=\"T_IF\""); break;
    case T_THEN:
          csound->Message(csound,"name=\"T_THEN\""); break;
    case T_ITHEN:
          csound->Message(csound,"name=\"T_ITHEN\""); break;
    case T_KTHEN:
          csound->Message(csound,"name=\"T_KTHEN\""); break;
    case T_ELSEIF:
          csound->Message(csound,"name=\"T_ELSEIF\""); break;
    case T_ELSE:
          csound->Message(csound,"name=\"T_ELSE\""); break;
    case T_UNTIL:
          csound->Message(csound,"name=\"T_UNTIL\""); break;
    case T_DO:
          csound->Message(csound,"name=\"T_DO\""); break;
    case T_OD:
          csound->Message(csound,"name=\"T_OD\""); break;
    case T_GOTO:
      csound->Message(csound,"name=\"T_GOTO\""); break;
    case T_IGOTO:
      csound->Message(csound,"name=\"T_IGOTO\""); break;
    case T_KGOTO:
      csound->Message(csound,"name=\"T_KGOTO\""); break;
    case T_SRATE:
      csound->Message(csound,"name=\"T_SRATE\""); break;
    case T_KRATE:
      csound->Message(csound,"name=\"T_KRATE\""); break;
    case T_KSMPS:
      csound->Message(csound,"name=\"T_KSMPS\""); break;
    case T_NCHNLS:
      csound->Message(csound,"name=\"T_NCHNLS\""); break;
    case T_NCHNLSI:
      csound->Message(csound,"name=\"T_NCHNLSI\""); break;
    case T_INSTR:
      csound->Message(csound,"name=\"T_INSTR\""); break;
    case T_STRCONST:
      csound->Message(csound,"name=\"T_STRCONST\" str=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT:
      csound->Message(csound,"name=\"T_IDENT\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_I:
      csound->Message(csound,"name=\"IDENT_I\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_GI:
      csound->Message(csound,"name=\"IDENT_GI\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_K:
      csound->Message(csound,"name=\"IDENT_K\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_GK:
      csound->Message(csound,"name=\"IDENT_GK\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_A:
      csound->Message(csound,"name=\"IDENT_A\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_GA:
      csound->Message(csound,"name=\"IDENT_GA\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_S:
      csound->Message(csound,"name=\"IDENT_S\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_GS:
      csound->Message(csound,"name=\"IDENT_GS\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_T:
      csound->Message(csound,"name=\"IDENT_T\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_GT:
      csound->Message(csound,"name=\"IDENT_GT\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_W:
      csound->Message(csound,"name=\"IDENT_W\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_GW:
      csound->Message(csound,"name=\"IDENT_GW\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_F:
      csound->Message(csound,"name=\"IDENT_F\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_GF:
      csound->Message(csound,"name=\"IDENT_GF\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_P:
      csound->Message(csound,"name=\"IDENT_P\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_B:
      csound->Message(csound,"name=\"IDENT_B\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_IDENT_b:
      csound->Message(csound,"name=\"IDENT_b\" varname=\"%s\"",
                      l->value->lexeme); break;
    case T_INTGR:
      csound->Message(csound,"name=\"T_INTGR\" value=\"%d\"",
                      l->value->value); break;
    case T_NUMBER:
      csound->Message(csound,"name=\"T_NUMBER\" value=\"%f\" type=%d",
                      l->value->fvalue, l->value->type); break;
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
    case T_UDO:
        csound->Message(csound,"name=\"T_UDO\""); break;
    case T_UDO_ANS:
        csound->Message(csound,"name=\"T_UDO_ANS\" signature=\"%s\"",
                        l->value->lexeme); break;
    case T_UDO_ARGS:
        csound->Message(csound,"name=\"T_UDO_ARGS\" signature=\"%s\"",
                        l->value->lexeme); break;
    default:
      csound->Message(csound,"name=\"unknown\"");
    }

    csound->Message(csound, " >\n");

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
    /*if (PARSER_DEBUG)*/ {
      if (msg)
        csound->Message(csound, msg);
      else
        csound->Message(csound, "Printing Tree\n");
      csound->Message(csound, "<ast>\n");
      print_tree_xml(csound, l, 0, TREE_NONE);
      csound->Message(csound, "</ast>\n");
    }
}

void handle_optional_args(CSOUND *csound, TREE *l)
{
    if (l == NULL || l->type == T_LABEL) return;

    int opnum = find_opcode(csound, l->value->lexeme);
    OENTRY *ep = csound->opcodlst + opnum;
    int nreqd = 0;
    int incnt = tree_arg_list_count(l->right);
    TREE * temp;

    if (ep->intypes != NULL) {
      nreqd = strlen(ep->intypes);
    }

    if (PARSER_DEBUG) {
      csound->Message(csound, "Handling Optional Args for opcode %s, %d, %d",
                      ep->opname, incnt, nreqd);
      //      csound->Message(csound, "ep->intypes = >%s<\n", ep->intypes);
    }
    if (incnt < nreqd) {         /*  or set defaults: */
      do {
        switch (ep->intypes[incnt]) {
        case 'O':             /* Will this work?  Doubtful code.... */
        case 'o':
          temp = make_leaf(csound, T_INTGR, make_int(csound, "0"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'P':
        case 'p':
          temp = make_leaf(csound, T_INTGR, make_int(csound, "1"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'q':
          temp = make_leaf(csound, T_INTGR, make_int(csound, "10"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;

        case 'V':
        case 'v':
          temp = make_leaf(csound, T_NUMBER, make_num(csound, ".5"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'h':
          temp = make_leaf(csound, T_INTGR, make_int(csound, "127"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'J':
        case 'j':
          temp = make_leaf(csound, T_INTGR, make_int(csound, "-1"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'M':
        case 'N':
        case 'm':
          nreqd--;
          break;
        default:
          synterr(csound, Str("insufficient required arguments"));
        }
        incnt++;
      } while (incnt < nreqd);
    }
}

char tree_argtyp(CSOUND *csound, TREE *tree) {
    if (tree->type == T_INTGR || tree->type == T_NUMBER) {
      return 'i';
    }

    return argtyp2(csound, tree->value->lexeme);
}

void handle_polymorphic_opcode(CSOUND* csound, TREE * tree) {
    if (tree->type == S_ASSIGN) {
      /* BUG: tree->right->value may be NULL */
      /* if (tree->right->value) */
        tree->value->lexeme = get_assignment_type(csound,
                                                  tree->left->value->lexeme,
                                                  tree->right/*->value->lexeme*/);
      /* else {                    /\* Conditional expression so broken  *\/ */
      /*   print_tree(csound, "Odd case\n", tree); */
      /* } */
      return;
    }
    else if (tree->type==0) {
      csound->Message(csound, "Null type in tree -- aborting\n");
      exit(2);
    }
    else {
      int opnum = find_opcode(csound, tree->value->lexeme);
      OENTRY *ep = csound->opcodlst + opnum;

/*     int incnt = tree_arg_list_count(tree->right); */

      char * str = (char *)mcalloc(csound, strlen(ep->opname) + 4);
      char c, d;

      if (ep->dsblksiz >= 0xfffb) {

        switch (ep->dsblksiz) {

        case 0xffff:
          /* use outype to modify some opcodes flagged as translating */
          if (PARSER_DEBUG)
            csound->Message(csound, "[%s]\n", tree->left->value->lexeme);

          c = tree_argtyp(csound, tree->left);
          if (c == 'p')   c = 'i';
          if (c == '?')   c = 'a';                   /* tmp */
          sprintf(str, "%s.%c", ep->opname, c);

          if (PARSER_DEBUG) csound->Message(csound, "New Value: %s\n", str);

          /*if (find_opcode(csound, str) == 0) {*/
          /* synterr(csound, Str("failed to find %s, output arg '%s' illegal type"),
             str, ST(group)[ST(nxtest)]);*/    /* report syntax error     */
          /*ST(nxtest) = 100; */                    /* step way over this line */
          /*goto tstnxt;*/                          /* & go to next            */
          /*break;*/
          /*}*/
          tree->value->lexeme = (char *)mrealloc(csound, tree->value->lexeme,
                                                 strlen(str) + 1);
          strcpy(tree->value->lexeme, str);
          csound->DebugMsg(csound, Str("modified opcod: %s"), str);
          break;
        case 0xfffe:                              /* Two tags for OSCIL's    */
          if (PARSER_DEBUG)
            csound->Message(csound, "POLYMORPHIC 0xfffe\n");
          c = tree_argtyp(csound, tree->right);
          if (c != 'a') c = 'k';
          if ((d = tree_argtyp(csound, tree->right->next)) != 'a') d = 'k';
          sprintf(str, "%s.%c%c", ep->opname, c, d);
          if (PARSER_DEBUG) csound->Message(csound, "New Value: %s\n", str);
          tree->value->lexeme = (char *)mrealloc(csound, tree->value->lexeme,
                                                 strlen(str) + 1);
          strcpy(tree->value->lexeme, str);
          break;
        case 0xfffd:                              /* For peak, etc.          */
          c = tree_argtyp(csound, tree->right);
          if (PARSER_DEBUG)
            csound->Message(csound, "POLYMORPHIC 0xfffd\n");
          if (c != 'a') c = 'k';
          sprintf(str, "%s.%c", ep->opname, c);
          if (PARSER_DEBUG) csound->Message(csound, "New Value: %s\n", str);
          tree->value->lexeme = (char *)mrealloc(csound, tree->value->lexeme,
                                                 strlen(str) + 1);
          strcpy(tree->value->lexeme, str);
          break;
        case 0xfffc:                              /* For divz types          */
          if (PARSER_DEBUG)
            csound->Message(csound, "POLYMORPHIC 0xfffc\n");
          c = tree_argtyp(csound, tree->right);
          d = tree_argtyp(csound, tree->right->next);
          if ((c=='i' || c=='c') && (d=='i' || d=='c'))
            c = 'i', d = 'i';
          else {
            if (c != 'a') c = 'k';
            if (d != 'a') d = 'k';
          }
          sprintf(str, "%s.%c%c", ep->opname, c, d);
          if (PARSER_DEBUG) csound->Message(csound, "New Value: %s\n", str);
          tree->value->lexeme = (char *)mrealloc(csound, tree->value->lexeme,
                                                 strlen(str) + 1);
          strcpy(tree->value->lexeme, str);
          break;
        case 0xfffb:          /* determine opcode by type of first input arg */
          if (PARSER_DEBUG)
            csound->Message(csound, "POLYMORPHIC 0xfffb\n");
          c = tree_argtyp(csound, tree->right);
           /* allows a, k, and i types (e.g. Inc, Dec), but not constants */
          if (c=='p') c = 'i';
          /*if (ST(typemask_tabl)[(unsigned char) c] & (ARGTYP_i | ARGTYP_p))
            c = 'i';
            sprintf(str, "%s.%c", ST(linopcod), c);*/
          sprintf(str, "%s.%c", ep->opname, c);
          if (PARSER_DEBUG) csound->Message(csound, "New Value: %s\n", str);
          tree->value->lexeme = (char *)mrealloc(csound, tree->value->lexeme,
                                                 strlen(str) + 1);
          strcpy(tree->value->lexeme, str);
          break;
        default:
          csound->Message(csound, "Impossible case\n");
          break;
          /*strcpy(str, ST(linopcod));*/  /* unknown code: use original opcode   */
        }

        /*if (!(isopcod(csound, str))) {*/
        /* if opcode is not found: report syntax error     */
        /*synterr(csound, Str("failed to find %s, input arg illegal type"), str);*/
        /*ST(nxtest) = 100;*/                       /* step way over this line */
        /*goto tstnxt;*/                            /* & go to next            */
        /*}
          ST(linopnum) = ST(opnum);
          ST(linopcod) = ST(opcod);
          csound->DebugMsg(csound, Str("modified opcod: %s"), ST(opcod));*/
      }

      /* free(str); */
    }
}
