 /*
    csound_orc_expressions.c:

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

extern char argtyp2(CSOUND *, char *);
extern void print_tree(CSOUND *, char *, TREE *);
extern void handle_polymorphic_opcode(CSOUND*, TREE *);
extern void handle_optional_args(CSOUND *, TREE *);
extern ORCTOKEN *make_token(CSOUND *, char *);
extern ORCTOKEN *make_label(CSOUND *, char *);

TREE* create_boolean_expression(CSOUND*, TREE*);
TREE * create_expression(CSOUND *, TREE *);

static int genlabs = 300;

char *create_out_arg(CSOUND *csound, char outype)
{
    char* s = (char *)csound->Malloc(csound, 8);

    switch(outype) {
    case 'a': sprintf(s, "#a%d", csound->acount++); break;
    case 'K':
    case 'k': sprintf(s, "#k%d", csound->kcount++); break;
    case 'B': sprintf(s, "#B%d", csound->Bcount++); break;
    case 'b': sprintf(s, "#b%d", csound->bcount++); break;
    default:  sprintf(s, "#i%d", csound->icount++); break;
    }

    return s;
}

/**
 * Handles expression opcode type, appending to passed in opname
 * returns outarg type
 */
char *set_expression_type(CSOUND *csound, char * op, char arg1, char arg2)
{
    char outype, *s;

    if (arg1 == 'a') {
      if (arg2 == 'a') {
        strncat(op,".aa",80);
      }
      else {
        strncat(op,".ak",80);
      }
      outype = 'a';
    }
    else if (arg2 == 'a') {
      strncat(op,".ka",80);
      outype = 'a';
    }
    else if (arg1 == 'k' || arg2 == 'k') {
      strncat(op,".kk",80);
      outype = 'k';
    }
    else {
      strncat(op,".ii",80);
      outype = 'i';
    }

    s = create_out_arg(csound, outype);

    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "SET_EXPRESSION_TYPE: %s : %s\n", op, s);

    return s;
}

char * get_boolean_arg(CSOUND *csound, int type)
{
    char* s = (char *)csound->Malloc(csound, 8);
    //    type = 1;
    sprintf(s, "#%c%d", type?'B':'b',csound->Bcount++);

    return s;
}

int get_expression_ans_type(CSOUND * csound, char * ans)
{
    char * t = ans;
    t++;

    switch(*t) {
    case 'a':
      return T_IDENT_A;
    case 'k':
      return T_IDENT_K;
    case 'B':
      return T_IDENT_B;
    case 'b':
      return T_IDENT_b;
    default:
      return T_IDENT_I;
    }
}

TREE *create_empty_token(CSOUND *csound)
{
    TREE *ans;
    ans = (TREE*)mmalloc(csound, sizeof(TREE));
    if (UNLIKELY(ans==NULL)) {
      /* fprintf(stderr, "Out of memory\n"); */
      exit(1);
    }
    ans->type = -1;
    ans->left = NULL;
    ans->right = NULL;
    ans->next = NULL;
    ans->len = 0;
    ans->rate = -1;
    ans->value = NULL;
    return ans;
}

TREE *create_minus_token(CSOUND *csound)
{
    TREE *ans;
    ans = (TREE*)mmalloc(csound, sizeof(TREE));
    if (UNLIKELY(ans==NULL)) {
      /* fprintf(stderr, "Out of memory\n"); */
      exit(1);
    }
    ans->type = T_INTGR;
    ans->left = NULL;
    ans->right = NULL;
    ans->next = NULL;
    ans->len = 0;
    ans->rate = -1;
    ans->value = make_int(csound, "-1");
    return ans;
}

TREE * create_opcode_token(CSOUND *csound, char* op)
{
    TREE *ans = create_empty_token(csound);

    ans->type = T_OPCODE;
    ans->value = make_token(csound, op);
    ans->value->type = T_OPCODE;

    return ans;
}

TREE * create_ans_token(CSOUND *csound, char* var)
{
    TREE *ans = create_empty_token(csound);

    ans->type = get_expression_ans_type(csound, var);
    ans->value = make_token(csound, var);
    ans->value->type = ans->type;

    return ans;

}

TREE * create_goto_token(CSOUND *csound, char * booleanVar,
                         TREE * gotoNode, int type)
{
/*     TREE *ans = create_empty_token(csound); */
    char* op = (char *)csound->Malloc(csound, 7); /* Unchecked */
    TREE *opTree, *bVar;

    switch(gotoNode->type) {
    case T_KGOTO:
      sprintf(op, "ckgoto");
      break;
    case T_IGOTO:
      sprintf(op, "cigoto");
      break;
    case T_ITHEN:
      sprintf(op, "cngoto");
      break;
    case T_THEN:
    case T_KTHEN:
      sprintf(op, "cngoto");
      break;
    default:
      if (type) sprintf(op, "ckgoto");
      else sprintf(op, "cggoto");
    }

    opTree = create_opcode_token(csound, op);
    bVar = create_empty_token(csound);
    bVar->type = (type ? T_IDENT_B : T_IDENT_b);
    bVar->value = make_token(csound, booleanVar);
    bVar->value->type = bVar->type;

    opTree->left = NULL;
    opTree->right = bVar;
    opTree->right->next = gotoNode->right;

    return opTree;
}

/* THIS PROBABLY NEEDS TO CHANGE TO RETURN DIFFERENT GOTO TYPES LIKE IGOTO, ETC */
TREE *create_simple_goto_token(CSOUND *csound, TREE *label, int type)
{
    char* op = (char *)csound->Calloc(csound, 6);
    TREE * opTree;
    char *gt[3] = {"kgoto", "igoto", "goto"};
    sprintf(op, gt[type]);       /* kgoto, igoto, goto ?? */
    opTree = create_opcode_token(csound, op);
    opTree->left = NULL;
    opTree->right = label;

    return opTree;
}

/* Returns true if passed in TREE node is a numerical expression */
static int is_expression_node(TREE *node)
{
    if (node == NULL) {
      return 0;
    }

    switch(node->type) {
    case S_PLUS:
    case S_MINUS:
    case S_TIMES:
    case S_DIV:
    case S_MOD:
    case S_POW:
    case T_FUNCTION:
    case S_UMINUS:
    case S_BITOR:
    case S_BITAND:
    case S_BITSHR:
    case S_BITSHL:
    case S_NEQV:
    case S_BITNOT:
    case S_Q:
    case S_TABREF:
      return 1;
    }
   return 0;
}

/* Returns if passed in TREE node is a boolean expression */
static int is_boolean_expression_node(TREE *node)
{
    if (node == NULL) {
      return 0;
    }

    switch(node->type) {
    case S_EQ:
    case S_NEQ:
    case S_GE:
    case S_LE:
    case S_GT:
    case S_LT:
    case S_AND:
    case S_OR:
      return 1;
    }
    return 0;
}

static TREE *create_cond_expression(CSOUND *csound, TREE *root)
{
    char *op = (char*)mmalloc(csound, 4), arg1, arg2, *outarg = NULL;
    char outype, *s;
    TREE *anchor = create_boolean_expression(csound, root->left), *last;
    TREE * opTree;
    TREE *b= create_ans_token(csound, anchor->left->value->lexeme);;
    TREE *c = root->right->left, *d = root->right->right;
    last = anchor;
    while (last->next != NULL) {
      last = last->next;
    }
    if (is_expression_node(c)) {
      last->next = create_expression(csound, c);
      /* TODO - Free memory of old left node
         freetree */
      last = last->next;
      while (last->next != NULL) {
        last = last->next;
      }
      c = create_ans_token(csound, last->left->value->lexeme);
    }
    if (is_expression_node(d)) {
      last->next = create_expression(csound, d);
      /* TODO - Free memory of old left node
         freetree */
      last = last->next;
      while (last->next != NULL) {
        last = last->next;
      }
      d = create_ans_token(csound, last->left->value->lexeme);
    }

    arg1 = argtyp2(csound, c->value->lexeme);
    arg2 = argtyp2(csound, d->value->lexeme);
    if (arg1 == 'a' || arg2 == 'a') {
      strcpy(op,":a");
      outype = 'a';
    }
    else if (arg1 == 'k' || arg2 == 'k') {
      strcpy(op,":k");
      outype = 'k';
    }
    else {
      strcpy(op,":i");
      outype = 'i';
      }
    outarg = create_out_arg(csound, outype);
    opTree = create_opcode_token(csound, op);
    opTree->left = create_ans_token(csound, outarg);
    opTree->right = b;
    opTree->right->next = c;
    opTree->right->next->next = d;
    /* should recycle memory for root->right */
    //mfree(csound, root->right); root->right = NULL;
    last->next = opTree;
    //    print_tree(csound, "Answer:\n", anchor);
    return anchor;
}
/**
 * Create a chain of Opcode (OPTXT) text from the AST node given. Called from
 * create_opcode when an expression node has been found as an argument
 */
TREE * create_expression(CSOUND *csound, TREE *root)
{
    char *op, arg1, arg2, c, *outarg = NULL;
    TREE *anchor = NULL, *last;
    TREE * opTree;
    /* HANDLE SUB EXPRESSIONS */

    if (root->type==S_Q) return create_cond_expression(csound, root);

    if (is_expression_node(root->left)) {
      anchor = create_expression(csound, root->left);

      /* TODO - Free memory of old left node
         freetree */
      last = anchor;
      while (last->next != NULL) {
        last = last->next;
      }
      root->left = create_ans_token(csound, last->left->value->lexeme);
    }

    if (is_expression_node(root->right)) {
      TREE * newRight = create_expression(csound, root->right);
      if (anchor == NULL) {
        anchor = newRight;
      }
      else {
        last = anchor;
        while (last->next != NULL) {
          last = last->next;
        }
        last->next = newRight;
      }
      last = newRight;

      while (last->next != NULL) {
        last = last->next;
      }
        /* TODO - Free memory of old right node
           freetree */
      root->right = create_ans_token(csound, last->left->value->lexeme);
    }
    arg1 = '\0';
    if (root->left != NULL) {
      arg1 = argtyp2(csound, root->left->value->lexeme);
    }
    arg2 = argtyp2(csound, root->right->value->lexeme);

    op = mcalloc(csound, 80);

     switch(root->type) {
    case S_PLUS:
      strncpy(op, "add", 80);
      outarg = set_expression_type(csound, op, arg1, arg2);
      break;
    case S_MINUS:
      strncpy(op, "sub", 80);
      outarg = set_expression_type(csound, op, arg1, arg2);
      break;
    case S_TIMES:
      strncpy(op, "mul", 80);
      outarg = set_expression_type(csound, op, arg1, arg2);
      break;
    case S_MOD:
      strncpy(op, "mod", 80);
      outarg = set_expression_type(csound, op, arg1, arg2);
      break;
    case S_DIV:
      strncpy(op, "div", 80);
      outarg = set_expression_type(csound, op, arg1, arg2);
      break;
    case S_POW:
      { int outype = 'i';
        strncpy(op, "pow.", 80);
        if (arg1 == 'a') {
          strncat(op, "a", 80);
          outype = arg1;
        }
        else if (arg1 == 'k') {
          strncat(op, "k", 80);
          outype = arg1;
        }
        else
          strncat(op, "i", 80);
        outarg = create_out_arg(csound, outype);
      }
      break;
    case S_TABREF:
      strncpy(op, "##tabref", 80);
      if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "Found TABREF: %s\n", op);
      outarg = create_out_arg(csound, 'k');
      break;
    case T_FUNCTION: /* assumes on single arg input */
      c = arg2;
      if (c == 'p' || c == 'c')   c = 'i';
      sprintf(op, "%s.%c", root->value->lexeme, c);
      if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "Found OP: %s\n", op);
      outarg = create_out_arg(csound, c);
      break;
    case S_UMINUS:
      if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "HANDLING UNARY MINUS!");
      root->left = create_minus_token(csound);
      arg1 = 'i';
      strncpy(op, "mul", 80);
      outarg = set_expression_type(csound, op, arg1, arg2);
      break;
    case S_BITOR:
      strncpy(op, "or", 80);
      outarg = set_expression_type(csound, op, arg1, arg2);
      break;
    case S_BITAND:
      strncpy(op, "and", 80);
      outarg = set_expression_type(csound, op, arg1, arg2);
      break;
    case S_BITSHR:
      strncpy(op, "shr", 80);
      outarg = set_expression_type(csound, op, arg1, arg2);
      break;
    case S_BITSHL:
      strncpy(op, "shl", 80);
      outarg = set_expression_type(csound, op, arg1, arg2);
      break;
    case S_NEQV:
      strncpy(op, "xor", 80);
      outarg = set_expression_type(csound, op, arg1, arg2);
      break;
    case S_BITNOT:
      strncpy(op, "not", 80);
      outarg = set_expression_type(csound, op, arg1, '\0');
      break;
    }
    opTree = create_opcode_token(csound, op);
    if (root->left != NULL) {
      opTree->right = root->left;
      opTree->right->next = root->right;
      opTree->left = create_ans_token(csound, outarg);
      //print_tree(csound, "making expression", opTree);
    }
    else {
      opTree->right = root->right;
      opTree->left = create_ans_token(csound, outarg);
    }

    if (anchor == NULL) {
      anchor = opTree;
    }
    else {
      last = anchor;
      while (last->next != NULL) {
        last = last->next;
      }
      last->next = opTree;
    }
    mfree(csound, op);
    return anchor;
}

/**
 * Create a chain of Opcode (OPTXT) text from the AST node given. Called from
 * create_opcode when an expression node has been found as an argument
 */
TREE * create_boolean_expression(CSOUND *csound, TREE *root)
{
    char *op, *outarg;
    TREE *anchor = NULL, *last;
    TREE * opTree;

    /*   if (UNLIKELY(PARSER_DEBUG)) */csound->Message(csound, "Creating boolean expression\n");
    /* HANDLE SUB EXPRESSIONS */
    if (is_boolean_expression_node(root->left)) {
        anchor = create_boolean_expression(csound, root->left);
        last = anchor;
        while (last->next != NULL) {
            last = last->next;
        }
        /* TODO - Free memory of old left node
           freetree */
        root->left = create_ans_token(csound, anchor->left->value->lexeme);
    } else if (is_expression_node(root->left)) {
        anchor = create_expression(csound, root->left);
        
        /* TODO - Free memory of old left node
         freetree */
        last = anchor;
        while (last->next != NULL) {
            last = last->next;
        }
        root->left = create_ans_token(csound, last->left->value->lexeme);
    }

    
    if (is_boolean_expression_node(root->right)) {
      TREE * newRight = create_boolean_expression(csound, root->right);
      if (anchor == NULL) {
        anchor = newRight;
      }
      else {
        last = anchor;
        while (last->next != NULL) {
          last = last->next;
        }
        last->next = newRight;
      }
      /* TODO - Free memory of old right node
         freetree */
      root->right = create_ans_token(csound, newRight->left->value->lexeme);
    } else if (is_expression_node(root->right)) {
        TREE * newRight = create_expression(csound, root->right);
        if (anchor == NULL) {
            anchor = newRight;
        }
        else {
            last = anchor;
            while (last->next != NULL) {
                last = last->next;
            }
            last->next = newRight;
        }
        last = newRight;
        
        while (last->next != NULL) {
            last = last->next;
        }
        
        /* TODO - Free memory of old right node
         freetree */
        root->right = create_ans_token(csound, last->left->value->lexeme);
    }

    op = mcalloc(csound, 80);
    switch(root->type) {
    case S_EQ:
      strncpy(op, "==", 80);
      break;
    case S_NEQ:
      strncpy(op, "!=", 80);
      break;
    case S_GE:
      strncpy(op, ">=", 80);
      break;
    case S_LE:
      strncpy(op, "<=", 80);
      break;
    case S_GT:
      strncpy(op, ">", 80);
      break;
    case S_LT:
      strncpy(op, "<", 80);
      break;
    case S_AND:
      strncpy(op, "&&", 80);
      break;
    case S_OR:
      strncpy(op, "||", 80);
      break;
    }

    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "Operator Found: %s (%c %c)\n", op,
                      argtyp2(csound, root->left->value->lexeme),
                      argtyp2(csound, root->right->value->lexeme));

    outarg = get_boolean_arg(csound,
                             argtyp2(csound, root->left->value->lexeme) =='k' ||
                             argtyp2(csound, root->right->value->lexeme)=='k' ||
                             argtyp2(csound, root->left->value->lexeme) =='B' ||
                             argtyp2(csound, root->right->value->lexeme)=='B');

    opTree = create_opcode_token(csound, op);
    opTree->right = root->left;
    opTree->right->next = root->right;
    opTree->left = create_ans_token(csound, outarg);

    if (anchor == NULL) {
      anchor = opTree;
    }
    else {
      last = anchor;
      while (last->next != NULL) {
        last = last->next;
      }
      last->next = opTree;
    }
    mfree(csound, op);
    return anchor;
}


static TREE *create_synthetic_ident(CSOUND *csound, int32 count)
{
    char *label = (char *)csound->Calloc(csound, 20);
    ORCTOKEN *token;

    sprintf(label, "__synthetic_%ld", count);
    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "Creating Synthetic T_IDENT: %s\n", label);
    token = make_token(csound, label);
    token->type = T_IDENT;
    return make_leaf(csound, T_IDENT, token);
}

TREE *create_synthetic_label(CSOUND *csound, int32 count)
{
    char *label = (char *)csound->Calloc(csound, 20);

    sprintf(label, "__synthetic_%ld:", count);
    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "Creating Synthetic label: %s\n", label);
    return make_leaf(csound, T_LABEL, make_label(csound, label));
}

/* Expands expression nodes into opcode calls
 *
 *
 * for if-goto, expands to:
 *   1. Expression nodes - all of the expressions that lead to the boolean var
 *   2. goto node - a conditional goto to evals the boolean var and goes to a
 *      label
 *
 * for if-then-elseif-else, expands to:
 *   1. for each conditional, converts to a set of:
 *      -expression nodes
 *      -conditional not-goto that goes to block end label if condition does
 *       not pass (negative version of conditional is used to conditional skip
 *       contents of block)
 *      -statements (body of within conditional block)
 *      -goto complete block end (this signifies that at the end of these
 *       statements, skip all other elseif or else branches and go to very end)
 *      -block end label
 *   2. for else statements found, do no conditional and just link in statements
 *
 * */

TREE *csound_orc_expand_expressions(CSOUND * csound, TREE *root)
{
    //    int32 labelCounter = 300L;

    TREE *anchor = NULL;
    TREE * expressionNodes = NULL;

    TREE *current = root;
    TREE *previous = NULL;

    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "[Begin Expanding Expressions in AST]\n");

    while (current != NULL) {
      switch(current->type) {
      case T_INSTR:
        if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "Instrument found\n");
        current->right = csound_orc_expand_expressions(csound, current->right);
        //        print_tree(csound, "AFTER", current);
        break;
      case T_UDO:
        if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "UDO found\n");
        current->right = csound_orc_expand_expressions(csound, current->right);
        break;
      case T_IF:
        if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "Found IF statement\n");
        {
          TREE * left = current->left;
          TREE * right = current->right;
          TREE* last;
          TREE * gotoToken;

          if (right->type == T_IGOTO ||
              right->type == T_KGOTO ||
              right->type == T_GOTO) {
            if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "Found if-goto\n");
            expressionNodes = create_boolean_expression(csound, left);
            /* Set as anchor if necessary */
            if (anchor == NULL) {
              anchor = expressionNodes;
            }
            /* reconnect into chain */
            last = expressionNodes;
            while (last->next != NULL) {
              last = last->next;
            }
            if (previous != NULL) {
              previous->next = expressionNodes;
            }
            gotoToken = create_goto_token(csound,
                                          last->left->value->lexeme,
                                          right,
                                          last->left->type == 'k' ||
                                          right->type =='k');
            last->next = gotoToken;
            gotoToken->next = current->next;

            current = gotoToken;
            previous = last;
          }
          else if (right->type == T_THEN ||
                   right->type == T_ITHEN ||
                   right->type == T_KTHEN) {
            int endLabelCounter = -1;
            TREE *tempLeft;
            TREE *tempRight;
            TREE *ifBlockStart = NULL;
            TREE *ifBlockCurrent = current;
            TREE *ifBlockLast = NULL;

            if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "Found if-then\n");
            if (right->next != NULL) {
              endLabelCounter = genlabs++;
            }

            while (ifBlockCurrent != NULL) {
              tempLeft = ifBlockCurrent->left;
              tempRight = ifBlockCurrent->right;
              if (ifBlockCurrent->type == T_ELSE) {
                //                print_tree(csound, "ELSE case\n", ifBlockCurrent);
                ifBlockLast->next =
                  csound_orc_expand_expressions(csound, tempRight);
                while (ifBlockLast->next != NULL) {
                  ifBlockLast = ifBlockLast->next;
                }
                //                print_tree(csound, "ELSE transformed\n", ifBlockCurrent);
                break;
              }
              else if (ifBlockCurrent->type == T_ELSEIF) { /* JPff code */
                //                print_tree(csound, "ELSEIF case\n", ifBlockCurrent);
                ifBlockCurrent->type = T_IF;
                ifBlockCurrent = make_node(csound, T_ELSE, NULL, ifBlockCurrent);
                //tempLeft = NULL;
                /*   ifBlockLast->next = */
                /*     csound_orc_expand_expressions(csound, ifBlockCurrent); */
                /* while (ifBlockLast->next != NULL) { */
                /*   ifBlockLast = ifBlockLast->next; */
                /* } */
                //                print_tree(csound, "ELSEIF transformed\n", ifBlockCurrent);
                //break;
              }

              expressionNodes = create_boolean_expression(csound, tempLeft);
                            /* Set as anchor if necessary */
              if (ifBlockStart == NULL) {
                ifBlockStart = expressionNodes;
              }
              /* reconnect into chain */
              {
                TREE* last = expressionNodes;
                TREE *statements, *label, *labelEnd, *gotoToken;
                int gotoType;

                while (last->next != NULL) {
                  last = last->next;
                }
                if (ifBlockLast != NULL) {
                  ifBlockLast->next = expressionNodes;
                }

                statements = tempRight->right;
                label = create_synthetic_ident(csound, genlabs);
                labelEnd = create_synthetic_label(csound, genlabs++);
                tempRight->right = label;
//                printf("goto types %c %c %c %c %d\n",
//                       expressionNodes->left->type, tempRight->type,
//                       argtyp2(csound, last->left->value->lexeme),
//                       argtyp2(csound, tempRight->value->lexeme),
//                       (argtyp2(csound, last->left->value->lexeme) == 'k') ||
//                       (argtyp2(csound, tempRight->value->lexeme) == 'k'));
//                print_tree(csound, "expression nodes", expressionNodes);
                gotoType = (argtyp2(csound, last->left->value->lexeme) == 'B') ||
                  (argtyp2(csound, tempRight->value->lexeme) == 'k');
                gotoToken =
                  create_goto_token(csound,
                   last->left->value->lexeme,
                   tempRight,
                   gotoType);
                /* relinking */
                last->next = gotoToken;
                gotoToken->next = statements;
                while (statements->next != NULL) {
                  statements = statements->next;
                }
                if (endLabelCounter > 0) {
                  TREE *endLabel = create_synthetic_ident(csound,
                                                          endLabelCounter);
                  int type = (gotoType == 1) ? 0 : 1;
                  TREE *gotoEndLabelToken =
                    create_simple_goto_token(csound, endLabel, type);
                  if (UNLIKELY(PARSER_DEBUG))
                    csound->Message(csound, "Creating simple goto token\n");

                  statements->next = gotoEndLabelToken;
                  gotoEndLabelToken->next = labelEnd;
                }
                else {
                  statements->next = labelEnd;
                }

                ifBlockLast = labelEnd;
                ifBlockCurrent = tempRight->next;
              }
            }

            if (endLabelCounter > 0) {
              TREE *endLabel = create_synthetic_label(csound,
                                                      endLabelCounter);
              endLabel->next = ifBlockLast->next;
              ifBlockLast->next = endLabel;
              ifBlockLast = endLabel;
            }
            ifBlockLast->next = current->next;

            /* Connect in all of the TREE nodes that were flattened from
             * the if-else-else block
             */
            /* Set as anchor if necessary */
            if (anchor == NULL) {
              anchor = ifBlockStart;
            }

            /* reconnect into chain */
            if (previous != NULL) {
              previous->next = ifBlockStart;
            }
            current = ifBlockStart;
          }
          else {
            csound->Message(csound, "ERROR: Neither if-goto or if-then found!!!");
          }
        }
        break;
      case T_UNTIL:
        if (UNLIKELY(PARSER_DEBUG))
          csound->Message(csound, "Found UNTIL statement\n");
        {
          TREE * left = current->left;
          TREE * right = current->right;
          TREE* last;
          TREE * gotoToken;

          int32 topLabelCounter = genlabs++;
          int32 endLabelCounter = genlabs++;
          TREE *tempLeft = current->left;
          TREE *tempRight = current->right;
          TREE *ifBlockStart = current;
          TREE *ifBlockCurrent = current;
          TREE *ifBlockLast = current;
          TREE *next = current->next;
          TREE *statements, *label, *labelEnd;
          int type;
          /* *** Stage 1: Create a top label (overwriting *** */
          {                           /* Change UNTIL to label and add IF */
            TREE* t=create_synthetic_label(csound, topLabelCounter);
            current->type = t->type; current->left = current->right = NULL;
            current->value = t->value;
            ifBlockCurrent = t;
            ifBlockCurrent->left = tempLeft;
            ifBlockCurrent->right = tempRight;
            ifBlockCurrent->next = current->next;
            ifBlockCurrent->type = T_IF;
            current->next = t;
          }
          /* *** Stage 2: Boolean expression *** */
          /* Deal with the boolean expression to variable */
          tempRight = ifBlockCurrent->right;
          expressionNodes =
            ifBlockLast->next = create_boolean_expression(csound,
                                                          ifBlockCurrent->left);
          ifBlockLast = ifBlockLast->next;
          /* *** Stage 3: Create the goto *** */
          statements = tempRight;     /* the body of the loop */
          label    = create_synthetic_ident(csound, topLabelCounter);
          labelEnd = create_synthetic_label(csound, endLabelCounter);
          gotoToken =
            create_goto_token(csound,
                              expressionNodes->left->value->lexeme,
                              labelEnd,
                              type =
                              ((argtyp2(csound,
                                        expressionNodes->left->value->lexeme)=='B')
                               ||
                               (argtyp2(csound,
                                        tempRight->value->lexeme) == 'k')));
          /* relinking */
          tempRight = ifBlockLast->next;
          ifBlockLast->next = gotoToken;
          ifBlockLast->next->next = tempRight;
          gotoToken->right->next = labelEnd;
          gotoToken->next = statements;
          labelEnd = create_synthetic_label(csound, endLabelCounter);
          while (statements->next != NULL) { /* To end of body */
            statements = statements->next;
          }
          {
            TREE *topLabel = create_synthetic_ident(csound,
                                                    topLabelCounter);
            TREE *gotoTopLabelToken =
              create_simple_goto_token(csound, topLabel, (type==1 ? 0 : 1));
            if (UNLIKELY(PARSER_DEBUG))
              csound->Message(csound, "Creating simple goto token\n");
            statements->next = gotoTopLabelToken;
            gotoTopLabelToken->next = labelEnd;
          }
          ifBlockLast = labelEnd;
          ifBlockCurrent = tempRight->next;
        }
        break;
      case T_LABEL:
        break;
      default:
        { /* This is WRONG in optional argsq */
          TREE* previousArg = NULL;
          TREE* currentArg = current->right;
          if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "Found Statement.\n");

          /* if (current->type == S_ASSIGN) { */
          /*   //csound->Message(csound, "Assignment Statement.\n"); */
          /* } */
          while (currentArg != NULL) {
            TREE* last;
            TREE *nextArg;
            TREE *newArgTree;
            if (is_expression_node(currentArg)) {
              char * newArg;
              if (UNLIKELY(PARSER_DEBUG))
                csound->Message(csound, "Found Expression.\n");
              expressionNodes = create_expression(csound, currentArg);

              /* Set as anchor if necessary */
              if (anchor == NULL) {
                anchor = expressionNodes;
              }

              /* reconnect into chain */
              last = expressionNodes;
              while (last->next != NULL) {
                last = last->next;
              }
              last->next = current;
              if (previous == NULL) {
                previous = last;
              }
              else {
                previous->next = expressionNodes;
                previous = last;
              }

              newArg = last->left->value->lexeme;

              if (UNLIKELY(PARSER_DEBUG))
                csound->Message(csound, "New Arg: %s\n", newArg);

              /* handle arg replacement of currentArg here */
              nextArg = currentArg->next;
              newArgTree = create_ans_token(csound, newArg);

              if (previousArg == NULL) {
                current->right = newArgTree;
              }
              else {
                previousArg->next = newArgTree;
              }

              newArgTree->next = nextArg;
              currentArg = newArgTree;
              /* TODO - Delete the expression nodes here */
            }

            previousArg = currentArg;
            currentArg = currentArg->next;
          }
          handle_polymorphic_opcode(csound, current);
          handle_optional_args(csound, current);
        }
      }

      if (anchor == NULL) {
        anchor = current;
      }
      previous = current;
      current = current->next;
    }

    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "[End Expanding Expressions in AST]\n");

    return anchor;
}

