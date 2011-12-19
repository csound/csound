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
      if (root->left) {
        root->left= verify_tree(csound, root->left);
        if ((root->left->type  == INTEGER_TOKEN || root->left->type  == NUMBER_TOKEN) &&
            (root->right->type == INTEGER_TOKEN || root->right->type == NUMBER_TOKEN)) {
          lval = (root->left->type == INTEGER_TOKEN ?
                  (double)root->left->value->value :root->left->value->fvalue);
          rval = (root->right->type == INTEGER_TOKEN ?
                  (double)root->right->value->value :root->left->value->fvalue);
          ans = root->left;
          ans->type = ans->value->type = NUMBER_TOKEN;
          /* **** Something wrong here -- subtractuon confuses memory **** */
          switch (root->type) {
          case '+':
            ans->value->fvalue = lval+rval;
            ans->value->lexeme =
              (char*)mrealloc(csound, ans->value->lexeme, 24);
            sprintf(ans->value->lexeme, "%f", ans->value->fvalue);
            //Memory leak!! 
            //mfree(csound, root); mfree(csound root->right);
            return ans;
          case '-':
            ans->value->fvalue = lval-rval;
            ans->value->lexeme =
              (char*)mrealloc(csound, ans->value->lexeme, 24);
            sprintf(ans->value->lexeme, "%f", ans->value->fvalue);
            //Memory leak!! 
            //mfree(csound, root); mfree(csound, root->right);
            return ans;
          case '*':
            ans->value->fvalue = lval*rval;
            ans->value->lexeme =
              (char*)mrealloc(csound, ans->value->lexeme, 24);
            sprintf(ans->value->lexeme, "%f", ans->value->fvalue);
            //Memory leak!! 
            //mfree(csound, root); mfree(csound, root->right);
            return ans;
          case '/':
            ans->value->fvalue = lval/rval;
            ans->value->lexeme =
              (char*)mrealloc(csound, ans->value->lexeme, 24);
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
      else if (root->right->type == INTEGER_TOKEN ||
               root->right->type == NUMBER_TOKEN) {
        switch (root->type) {
        case S_UMINUS:
          print_tree(csound, "root", root);
          ans = root->right;
          ans->value->fvalue = -(ans->type==INTEGER_TOKEN ? ans->value->value
                                 : ans->value->fvalue);
          ans->value->lexeme =
            (char*)mrealloc(csound, ans->value->lexeme, 24);
          sprintf(ans->value->lexeme, "%f", ans->value->fvalue);
          ans->type = ans->value->type = NUMBER_TOKEN;
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
    printf("\n === END OF INPUT ===\n");
    return (1);
}

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
    /*     	     csound_orcget_lineno(yyscanner)); */
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
TREE* make_node(CSOUND *csound, int line, int type, TREE* left, TREE* right)
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
    ans->line = line;
    csound->DebugMsg(csound, "%s(%d) line = %d\n", __FILE__, __LINE__, line);
    return ans;
}

TREE* make_leaf(CSOUND *csound, int line, int type, ORCTOKEN *v)
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
    csound->DebugMsg(csound, "%s(%d) line = %d\n", __FILE__, __LINE__, line);
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
      csound->Message(csound,"%c:(%d)\n", l->type, l->line); break;
    case NEWLINE:
      csound->Message(csound,"NEWLINE:(%d)\n", l->line); break;
    case S_NEQ:
      csound->Message(csound,"S_NEQ:(%d)\n", l->line); break;
    case S_AND:
      csound->Message(csound,"S_AND:(%d)\n", l->line); break;
    case S_OR:
      csound->Message(csound,"S_OR:(%d)\n", l->line); break;
    case S_LT:
      csound->Message(csound,"S_LT:(%d)\n", l->line); break;
    case S_LE:
      csound->Message(csound,"S_LE:(%d)\n", l->line); break;
    case S_EQ:
      csound->Message(csound,"S_EQ:(%d)\n", l->line); break;
    case S_TASSIGN:
      csound->Message(csound,"S_TASSIGN:(%d)\n", l->line); break;
    case S_TABREF:
      csound->Message(csound,"S_TABREF:(%d)\n", l->line); break;
    case S_GT:
      csound->Message(csound,"S_GT:(%d)\n", l->line); break;
    case S_GE:
      csound->Message(csound,"S_GE:(%d)\n", l->line); break;
    case LABEL_TOKEN:
      csound->Message(csound,"LABEL_TOKEN: %s\n", l->value->lexeme); break;
    case IF_TOKEN:
      csound->Message(csound,"IF_TOKEN:(%d)\n", l->line); break;
    case THEN_TOKEN:
          csound->Message(csound,"THEN_TOKEN:(%d)\n", l->line); break;
    case ITHEN_TOKEN:
          csound->Message(csound,"ITHEN_TOKEN:(%d)\n", l->line); break;
    case KTHEN_TOKEN:
          csound->Message(csound,"KTHEN_TOKEN:(%d)\n", l->line); break;
    case ELSEIF_TOKEN:
          csound->Message(csound,"ELSEIF_TOKEN:(%d)\n", l->line); break;
    case ELSE_TOKEN:
          csound->Message(csound,"ELSE_TOKEN:(%d)\n", l->line); break;
    case UNTIL_TOKEN:
          csound->Message(csound,"UNTIL_TOKEN:(%d)\n", l->line); break;
    case DO_TOKEN:
          csound->Message(csound,"DO_TOKEN:(%d)\n", l->line); break;
    case OD_TOKEN:
          csound->Message(csound,"OD_TOKEN:(%d)\n", l->line); break;
    case GOTO_TOKEN:
      csound->Message(csound,"GOTO_TOKEN:(%d)\n", l->line); break;
    case IGOTO_TOKEN:
      csound->Message(csound,"IGOTO_TOKEN:(%d)\n", l->line); break;
    case KGOTO_TOKEN:
      csound->Message(csound,"KGOTO_TOKEN:(%d)\n", l->line); break;
    case SRATE_TOKEN:
      csound->Message(csound,"SRATE_TOKEN:(%d)\n", l->line); break;
    case KRATE_TOKEN:
      csound->Message(csound,"KRATE_TOKEN:(%d)\n", l->line); break;
    case ZERODBFS_TOKEN:
      csound->Message(csound,"ZERODFFS_TOKEN:(%d)\n", l->line); break;
    case KSMPS_TOKEN:
      csound->Message(csound,"KSMPS_TOKEN:(%d)\n", l->line); break;
    case NCHNLS_TOKEN:
      csound->Message(csound,"NCHNLS_TOKEN:(%d)\n", l->line); break;
    case NCHNLSI_TOKEN:
      csound->Message(csound,"NCHNLSI_TOKEN:(%d)\n", l->line); break;
    case INSTR_TOKEN:
      csound->Message(csound,"INSTR_TOKEN:(%d)\n", l->line); break;
    case STRING_TOKEN:
      csound->Message(csound,"STRING_TOKEN: %s\n", l->value->lexeme); break;
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
    case INTEGER_TOKEN:
      csound->Message(csound,"INTEGER_TOKEN: %d\n", l->value->value); break;
    case NUMBER_TOKEN:
      csound->Message(csound,"NUMBER_TOKEN: %f\n", l->value->fvalue); break;
    case S_ANDTHEN:
      csound->Message(csound,"S_ANDTHEN:(%d)\n", l->line); break;
    case S_APPLY:
      csound->Message(csound,"S_APPLY:(%d)\n", l->line); break;
    case T_OPCODE0:
      csound->Message(csound,"T_OPCODE0: %s\n", l->value->lexeme); break;
    case T_OPCODE:
      csound->Message(csound,"T_OPCODE: %s\n", l->value->lexeme); break;
    case T_FUNCTION:
      csound->Message(csound,"T_FUNCTION: %s\n", l->value->lexeme); break;
    case S_UMINUS:
        csound->Message(csound,"S_UMINUS:(%d)\n", l->line); break;
    case T_INSTLIST:
        csound->Message(csound,"T_INSTLIST:(%d)\n", l->line); break;
    default:
      csound->Message(csound,"unknown:%d(%d)\n", l->type, l->line);
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
    case S_TABREF:
      csound->Message(csound,"name=\"S_TABREF\""); break;
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
    case INTEGER_TOKEN:
      csound->Message(csound,"name=\"INTEGER_TOKEN\" value=\"%d\"",
                      l->value->value); break;
    case NUMBER_TOKEN:
      csound->Message(csound,"name=\"NUMBER_TOKEN\" value=\"%f\" type=%d",
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
    case UDO_TOKEN:
        csound->Message(csound,"name=\"UDO_TOKEN\""); break;
    case UDO_ANS_TOKEN:
        csound->Message(csound,"name=\"UDO_ANS_TOKEN\" signature=\"%s\"",
                        l->value->lexeme); break;
    case UDO_ARGS_TOKEN:
        csound->Message(csound,"name=\"UDO_ARGS_TOKEN\" signature=\"%s\"",
                        l->value->lexeme); break;
    default:
      csound->Message(csound,"name=\"unknown\"(%d)", l->type);
    }

    csound->Message(csound, " (%d)>\n", l->line);

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
    if (l == NULL || l->type == LABEL_TOKEN) return;

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
          temp = make_leaf(csound, l->line, INTEGER_TOKEN, make_int(csound, "0"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'P':
        case 'p':
          temp = make_leaf(csound, l->line, INTEGER_TOKEN, make_int(csound, "1"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'q':
          temp = make_leaf(csound, l->line, INTEGER_TOKEN, make_int(csound, "10"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;

        case 'V':
        case 'v':
          temp = make_leaf(csound, l->line, NUMBER_TOKEN, make_num(csound, ".5"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'h':
          temp = make_leaf(csound, l->line, INTEGER_TOKEN, make_int(csound, "127"));
          if (l->right==NULL) l->right = temp;
          else appendToTree(csound, l->right, temp);
          break;
        case 'J':
        case 'j':
          temp = make_leaf(csound, l->line, INTEGER_TOKEN, make_int(csound, "-1"));
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
                  Str("insufficient required arguments for opcode %s on line %d\n"),
                  ep->opname, l->line);
        }
        incnt++;
      } while (incnt < nreqd);
    }
}

char tree_argtyp(CSOUND *csound, TREE *tree) {
    if (tree->type == INTEGER_TOKEN || tree->type == NUMBER_TOKEN) {
      return 'i';
    }

    return argtyp2(csound, tree->value->lexeme);
}

void handle_polymorphic_opcode(CSOUND* csound, TREE * tree) {
    if (tree->type == '=') {
      /* BUG: tree->right->value may be NULL */
      /* if (tree->right->value) */
      tree->value->lexeme =
        get_assignment_type(csound,
                            tree->left->value->lexeme,
                            tree->right/*->value->lexeme*/);
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
