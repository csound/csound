 /*
    csound_orc_optimize.c:

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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include "csoundCore.h"
#include "csound_orc.h"
extern void print_tree(CSOUND *csound, char*, TREE *l);
extern void delete_tree(CSOUND *csound, TREE *l);

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
    //print_tree(csound, "optimize_ifun: before", root);
    switch(root->right->type) {
    case INTEGER_TOKEN:
    case NUMBER_TOKEN:               /* i(num)    -> num      */
      // FIXME - reinstate optimization after implementing get_type(varname)
      //    case T_IDENT_I:          /* i(ivar)   -> ivar     */
      //    case T_IDENT_GI:         /* i(givar)  -> givar    */
      //    case T_IDENT_P:          /* i(pN)     -> pN       */
      root = root->right;
      break;
      //    case T_IDENT_K:          /* i(kvar)   -> i(kvar)  */
      //    case T_IDENT_GK:         /* i(gkvar)  -> i(gkvar) */
      //      break;
    case T_FUNCTION:                 /* i(fn(x))  -> fn(i(x)) */
      {
        TREE *funTree = root->right;
        if (LIKELY(strcmp(funTree->value->lexeme,"i")!=0))
          funTree->right = create_fun_token(csound, funTree->right, "i");
        root = funTree;
      }
      break;
    default:                         /* i(A op B) -> i(A) op i(B) */
      if (root->right->left != NULL)
        root->right->left = create_fun_token(csound,
                                             root->right->left, "i");
      if (root->right->right != NULL)
        root->right->right = create_fun_token(csound,
                                              root->right->right, "i");
      root->right->next = root->next;
      root = root->right;
      break;
    }
    /* print_tree(csound, "optimize_ifun: after", root); */
    return root;
}

/** Verifies opcodes and args are correct*/
/* The wrong place to fold constants so done in parser -- JPff */
static TREE * verify_tree1(CSOUND *csound, TREE *root)
{
    TREE *last;
    //print_tree(csound, "Verify", root);
    if (root->right && root->right->type != T_INSTLIST) {
      if (root->type == T_OPCODE || root->type == T_OPCODE0) {
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
      }
    }
    return root;
}

//#ifdef JPFF
static inline int same_type(char *var, char ty)
{
    if (var[0]=='g') return var[1]==ty;
    else return var[0]==ty;
}

static TREE* remove_excess_assigns(CSOUND *csound, TREE* root)
{
    TREE* current = root;
    while (current) {
      //if (PARSER_DEBUG) printf("in loop: current->type = %d\n", current->type);
      if ((current->type == T_OPCODE || current->type == '=') &&
          current->left != NULL &&
          //current->right != NULL &&  no one looks at current->right
          current->left->value->lexeme[0]=='#') {
        TREE *nxt = current->next;
        /* if (PARSER_DEBUG) { */
        /*   printf("passes test1 %s type =%d\n", */
        /*          current->left->value->lexeme, current->type); */
        /*   printf("next type = %d; lexeme %s\n", */
        /*          nxt->type, nxt->right->value->lexeme); */
        /* } */
        /* if (PARSER_DEBUG) printf("test3: %c%c %c\n", */
        /*            nxt->left->value->lexeme[0], nxt->left->value->lexeme[1], */
        /*            nxt->right->value->lexeme[1]); */
        if (nxt && nxt->type == '=' &&
            nxt->left != NULL &&
            !strcmp(current->left->value->lexeme,nxt->right->value->lexeme) &&
            same_type(nxt->left->value->lexeme, nxt->right->value->lexeme[1])) {
          if (PARSER_DEBUG) {
            printf("passes test2\n");
            print_tree(csound, "optimise assignment\n", current);
          }
          csound->Free(csound, current->left->value);
          current->left->value = nxt->left->value;
          current->next = nxt->next;
          csound->Free(csound,nxt);
          if (PARSER_DEBUG) print_tree(csound, "change to\n", current);
        }
      }
      else {                    /* no need to check for NULL */
          current->right = remove_excess_assigns(csound, current->right);
          current->left = remove_excess_assigns(csound, current->left);
      }
      current = current->next;
    }
    return root;
}
//#endif

/* Called directly from the parser; constant fold and some alebraic identities */
TREE* constant_fold(CSOUND *csound, TREE* root)
{
    extern MYFLT MOD(MYFLT, MYFLT);
    TREE* current = root;
    while (current) {
      switch (current->type) {
      case '+':
      case '-':
      case '*':
      case '/':
      case '^':
      case '%':
      case '|':
      case '&':
      case '#':
      case S_BITSHIFT_LEFT:
      case S_BITSHIFT_RIGHT:
        /* First check for syntax errors */
        if (current->left==NULL || current->right==NULL) return root;
        current->left = constant_fold(csound, current->left);
        current->right = constant_fold(csound, current->right);
        //print_tree(csound, "Folding case??\n", current);
        if ((current->left->type == INTEGER_TOKEN ||
             current->left->type == NUMBER_TOKEN) &&
            (current->right->type == INTEGER_TOKEN ||
             current->right->type == NUMBER_TOKEN)) {
          MYFLT lval, rval;
          char buf[64];
          lval = (current->left->type == INTEGER_TOKEN ?
                  (double)current->left->value->value :
                  current->left->value->fvalue);
          rval = (current->right->type == INTEGER_TOKEN ?
                  (double)current->right->value->value :
                  current->right->value->fvalue);
          //printf("lval = %g  rval = %g\n", lval, rval);
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
          case '^':
            lval = POWER(lval,rval);
            break;
          case '%':
            lval = MOD(lval,rval);
            break;
          case '|':
            lval = (MYFLT)(((int)lval)|((int)rval));
            break;
          case '&':
            lval = (MYFLT)(((int)lval)&((int)rval));
            break;
          case '#':
            lval = (MYFLT)(((int)lval)^((int)rval));
            break;
          case S_BITSHIFT_LEFT:
            lval = (MYFLT)(((int)lval)<<((int)rval));
            break;
          case S_BITSHIFT_RIGHT:
            lval = (MYFLT)(((int)lval)>>((int)rval));
            break;
          }
          //printf("ans = %g\n", lval);
          current->value = current->left->value;
          current->type = NUMBER_TOKEN;
          current->value->fvalue = lval;
          snprintf(buf, 60, "%.20g", lval);
          csound->Free(csound, current->value->lexeme);
          current->value->lexeme = cs_strdup(csound, buf);
          csound->Free(csound, current->left);
          csound->Free(csound, current->right->value);
          csound->Free(csound, current->right);
          current->right = current->left = NULL;
        }
        else                    /* X op 0 */
          if ((current->right->type == INTEGER_TOKEN &&
               current->right->value->value==0) ||
              (current->right->type == NUMBER_TOKEN &&
               current->right->value->fvalue==0)) {
            //print_tree(csound, "X op 0\n", current);
            switch (current->type) {
            case '+':
            case '-':
            case '|':
            case S_BITSHIFT_LEFT:
            case S_BITSHIFT_RIGHT: /* return X */
              {
                TREE* tmp = current->right;
                current->type = current->left->type;
                current->value = current->left->value;
                current->right = current->left->right;
                current->left = current->left->left;
                csound->Free(csound, tmp);
                //print_tree(csound, "X op 0 -> X\n", current);
              }
              break;
#ifdef somefineday
            case '*':
            case '&':           /* return zero */
              {
                TREE* tmp = current->left;
                current->type = current->right->type;
                current->value = current->right->value;
                current->right = NULL;
                current->left = NULL;
                delete_tree(csound, tmp);
                //print_tree(csound, "X op 0 -> 0\n", current);
                break;
              }
#endif
              //case '^':  /* 0 op X -> 1 */
            }
          }
          else                    /* 0 op X */
          if ((current->left->type == INTEGER_TOKEN &&
               current->left->value->value==0) ||
              (current->left->type == NUMBER_TOKEN &&
               current->left->value->fvalue==0)) {
            //print_tree(csound, "0 op X\n", current);
            switch (current->type) {
            case '+':
            case '|':
                delete_tree(csound,current->left);
                current->type = current->right->type;
                current->value = current->right->value;
                current->left = current->right->left;
                current->right = current->right->right;
                //print_tree(csound, "0 op X -> X\n", current);
              break;
#ifdef somefineday
            case '*':
            case '/':
            case '^':
#endif
            case '&':
            case S_BITSHIFT_LEFT:
            case S_BITSHIFT_RIGHT: /* return 0 */
              { TREE *tmp = current->right;
                current->type = current->left->type;
                current->value = current->left->value;
                current->right = NULL;
                current->left = NULL;
                delete_tree(csound, tmp);
                //print_tree(csound, "0 op X -> 0\n", current);
                break;
              }
            }
          }
          else                    /* X op 1 */
            if ((current->right->type == INTEGER_TOKEN &&
                 current->right->value->value==1) ||
                (current->right->type == NUMBER_TOKEN &&
                 current->right->value->fvalue==FL(1.0))) {
              //print_tree(csound, "X op 1\n", current);
              switch (current->type) {
              case '*':
              case '/':
              case '^':
                {
                  TREE* tmp = current->right;
                  current->type = current->left->type;
                  current->value = current->left->value;
                  current->right = current->left->right;
                  current->left = current->left->left;
                  csound->Free(csound, tmp);
                  //print_tree(csound, "X op 1 -> X\n", current);
                }
              }
            }
          else                    /* 1 op X */
            if ((current->left->type == INTEGER_TOKEN &&
                 current->left->value->value==1) ||
                (current->left->type == NUMBER_TOKEN &&
                 current->left->value->fvalue==FL(1.0))) {
              //print_tree(csound, "1 op X\n", current);
              switch (current->type) {
              case'^':
              { TREE *tmp = current->right;
                current->type = current->left->type;
                current->value = current->left->value;
                current->right = NULL;
                current->left = NULL;
                delete_tree(csound, tmp);
                //print_tree(csound, "1 op X -> 1\n", current);
                break;
              }
              case '*':
                delete_tree(csound,current->left);
                current->type = current->right->type;
                current->value = current->right->value;
                current->left = current->right->left;
                current->right = current->right->right;
                //print_tree(csound, "1 op X -> X\n", current);
                break;
              }
            }
        break;
      case S_UMINUS:
      case '~':
        //print_tree(csound, "Folding case?\n", current);
        current->right = constant_fold(csound, current->right);
        //print_tree(csound, "Folding case??\n", current);
        if (current->right->type == INTEGER_TOKEN ||
             current->right->type == NUMBER_TOKEN) {
          MYFLT lval;
          char buf[64];
          lval = (current->right->type == INTEGER_TOKEN ?
                  (double)current->right->value->value :
                  current->right->value->fvalue);
          switch (current->type) {
          case S_UMINUS:
            lval = -lval;
            break;
          case '~':
            lval = (MYFLT)(~(int)lval);
            break;
          }
          current->value = current->right->value;
          current->type = NUMBER_TOKEN;
          current->value->fvalue = lval;
          snprintf(buf, 60, "%.20g", lval);
          csound->Free(csound, current->value->lexeme);
          current->value->lexeme = cs_strdup(csound, buf);
          csound->Free(csound, current->right);
          current->right = NULL;
        }
        break;
      default:
        current->left = constant_fold(csound, current->left);
        current->right = constant_fold(csound, current->right);
      }
      current = current->next;
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
    //#ifdef JPFF
    return remove_excess_assigns(csound,original);
    //#else
    //return original;
    //#endif
}
