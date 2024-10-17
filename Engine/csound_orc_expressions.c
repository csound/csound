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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

#include "csoundCore.h"
#include "csound_orc.h"
#include "csound_orc_expressions.h"
#include "csound_type_system.h"
#include "csound_orc_semantics.h"
#include "csound_standard_types.h"
#include <inttypes.h>

extern char argtyp2(char *);
extern void print_tree(CSOUND *, char *, TREE *);
extern void handle_optional_args(CSOUND *, TREE *);
extern ORCTOKEN *make_token(CSOUND *, char *);
extern ORCTOKEN *make_label(CSOUND *, char *);
extern OENTRIES* find_opcode2(CSOUND *, char*);
extern char* resolve_opcode_get_outarg(CSOUND* , OENTRIES* , char*);
extern TREE* appendToTree(CSOUND * csound, TREE *first, TREE *newlast);
extern  char* get_arg_string_from_tree(CSOUND* csound, TREE* tree,
                                       TYPE_TABLE* typeTable);
extern void add_arg(CSOUND* csound, char* varName, char* annotation, TYPE_TABLE* typeTable);
extern void add_array_arg(CSOUND* csound, char* varName, char* annotation, int32_t dimensions,
                          TYPE_TABLE* typeTable);

extern char* get_array_sub_type(CSOUND* csound, char*);

extern char* convert_external_to_internal(CSOUND* csound, char* arg);


static TREE *create_boolean_expression(CSOUND*, TREE*, int32_t,  uint64_t,  TYPE_TABLE*);
static TREE *create_expression(CSOUND *, TREE *, int32_t,  uint64_t,  TYPE_TABLE*);
char *check_annotated_type(CSOUND* csound, OENTRIES* entries,
                           char* outArgTypes);
static TREE *create_synthetic_label(CSOUND *csound, int32 count);
extern void do_baktrace(CSOUND *csound, uint64_t files);



static int32_t genlabs = 300;

TREE* tree_tail(TREE* node) {
  TREE* t = node;
  if (t == NULL) {
    return NULL;
  }
  while (t->next != NULL) {
    t = t->next;
  }
  return t;
}

char *create_out_arg(CSOUND *csound, char* outype, int32_t argCount,
                     TYPE_TABLE* typeTable)
{
  char* s = (char *)csound->Malloc(csound, 256);
  if (strlen(outype) == 1) {
    switch(*outype) {
    case 'a': snprintf(s, 16, "#a%d", argCount); break;
    case 'K':
    case 'k': snprintf(s, 16, "#k%d", argCount); break;
    case 'B': snprintf(s, 16, "#B%d", argCount); break;
    case 'b': snprintf(s, 16, "#b%d", argCount); break;
    case 'f': snprintf(s, 16, "#f%d", argCount); break;
    case 't': snprintf(s, 16, "#k%d", argCount); break;
    case 'S': snprintf(s, 16, "#S%d", argCount); break;
    case '[': snprintf(s, 16, "#%c%d[]", outype[1], argCount);
      break;
    default:  snprintf(s, 16, "#i%d", argCount); break;
    }
    add_arg(csound, s, NULL, typeTable);
  } else {
     // VL 15.10.24
     // at this point new types defined with string type names
     // still have : prepended and ; appended to name
     // we need to remove these for the type system to recognise the type
     char type[64] = {0}, c;
     int32_t n = 0, i = 0;
     // remove any : or ; leftover in typename
     do  {
         c = outype[n++];
         if(c == ':' || c == ';') continue;  
         type[i++] = c;
      } while (c);
     
    // FIXME - struct arrays
    if (*type == '[') {
      snprintf(s, 16, "#%c%d[]", type[1], argCount);
      add_array_arg(csound, s, NULL, 1, typeTable);
    }
    else {
      //            char* argType = cs_strndup(csound, outype + 1, strlen(outype) - 2);
      snprintf(s, 256, "#%s%d", type, argCount);
      add_arg(csound, s, type, typeTable);
    }
    //        } else if(*outype == ':') {
    //            char* argType = cs_strndup(csound, outype + 1, strlen(outype) - 2);
    //            snprintf(s, 256, "#%s%d", argType, argCount);
    //            add_arg(csound, s, argType, typeTable);
    //        } else {
    //            csound->Warning(csound, "ERROR: unknown outype found for out arg synthesis: %s\n", outype);
    //            return NULL;
    //        }
  }
  return s;
}

/**
 * Handles expression opcode type, appending to passed in opname
 * returns outarg type
 */

char * get_boolean_arg(CSOUND *csound, TYPE_TABLE* typeTable, int32_t type)
{
  char* s = (char *)csound->Malloc(csound, 8);
  snprintf(s, 8, "#%c%d", type?'B':'b', typeTable->localPool->synthArgCount++);

  return s;
}

static TREE *create_empty_token(CSOUND *csound)
{
  TREE *ans;
  ans = (TREE*)csound->Malloc(csound, sizeof(TREE));
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
  ans->line = 0;
  ans->locn  = 0;
  ans->value = NULL;
  ans->markup = NULL;
  return ans;
}

static TREE *create_minus_token(CSOUND *csound)
{
  TREE *ans;
  ans = (TREE*)csound->Malloc(csound, sizeof(TREE));
  if (UNLIKELY(ans==NULL)) {
    /* fprintf(stderr, "Out of memory\n"); */
    exit(1);
  }
  ans->type = INTEGER_TOKEN;
  ans->left = NULL;
  ans->right = NULL;
  ans->next = NULL;
  ans->len = 0;
  ans->rate = -1;
  ans->markup = NULL;
  ans->value = make_int(csound, "-1");
  return ans;
}

// also used in csound_orc_semantics.c
TREE * create_opcode_token(CSOUND *csound, char* op)
{
  TREE *ans = create_empty_token(csound);

  ans->type = T_OPCALL;
  ans->value = make_token(csound, op);
  ans->value->type = T_OPCALL;

  return ans;
}

static TREE * create_ans_token(CSOUND *csound, char* var)
{
  TREE *ans = create_empty_token(csound);

  ans->type = T_IDENT;
  ans->value = make_token(csound, var);
  ans->value->type = ans->type;

  return ans;

}

static TREE * create_goto_token(CSOUND *csound, char * booleanVar,
                                TREE * gotoNode, int32_t type)
{
  /*     TREE *ans = create_empty_token(csound); */
  char* op = (char *)csound->Malloc(csound, 8); /* Unchecked */
  TREE *opTree, *bVar;

  switch(gotoNode->type) {
  case KGOTO_TOKEN:
    strNcpy(op, "ckgoto", 8);
    break;
  case IGOTO_TOKEN:
    strNcpy(op, "cigoto", 8);
    break;
  case ITHEN_TOKEN:
    // *** yi ***
  icase:
    strNcpy(op, "cingoto", 8);
    break;
  case THEN_TOKEN:
    // *** yi ***
    if (csound->inZero) goto icase;
    /* if (csound->inZero) { */
    /*   printf("**** Odd case in instr0 %d\n", csound->inZero); */
    /*   print_tree(csound, "goto token\n", gotoNode); */
    /* } */
    /* fall through */
  case KTHEN_TOKEN:
    strNcpy(op, "cngoto", 8);
    break;
  default:
    switch (type) {
    case 1: strNcpy(op, "ckgoto", 8); break;
    case 0x8001: strNcpy(op, "cnkgoto", 8); break;
    case 0: strNcpy(op, "cggoto", 8); break;
    case 0x8000:
      // *** yi ***
      strNcpy(op,csound->inZero?"cingoto":"cngoto", 8);
      //strNcpy(op,"cngoto", 8);
      break;
    default: printf("Whooops %d\n", type);
    }
  }

  opTree = create_opcode_token(csound, op);
  bVar = create_empty_token(csound);
  bVar->type = T_IDENT; //(type ? T_IDENT_B : T_IDENT_b);
  bVar->value = make_token(csound, booleanVar);
  bVar->value->type = bVar->type;

  opTree->left = NULL;
  opTree->right = bVar;
  opTree->right->next = gotoNode->right;
  csound->Free(csound, op);
  return opTree;
}

/* THIS PROBABLY NEEDS TO CHANGE TO RETURN DIFFERENT GOTO
   TYPES LIKE IGOTO, ETC */
static TREE *create_simple_goto_token(CSOUND *csound, TREE *label, int32_t type)
{
  char* op = (char *)csound->Calloc(csound, 6);
  TREE * opTree;
  char *gt[3] = {"kgoto", "igoto", "goto"};
  if (csound->inZero && type==2) type = 1;
  strNcpy(op, gt[type],6);       /* kgoto, igoto, goto ?? */
  opTree = create_opcode_token(csound, op);
  opTree->left = NULL;
  opTree->right = label;
  csound->Free(csound, op);
  return opTree;
}

/* Returns true if passed in TREE node is a numerical expression */
int32_t is_expression_node(TREE *node)
{
  if (node == NULL) {
    return 0;
  }

  switch(node->type) {
  case '+':
  case '-':
  case '*':
  case '/':
  case '%':
  case '^':
  case T_FUNCTION:
  case S_UMINUS:
  case S_UPLUS:
  case '|':
  case '&':
  case S_BITSHIFT_RIGHT:
  case S_BITSHIFT_LEFT:
  case '#':
  case '~':
  case '?':
  case T_ARRAY:
    return 1;
  }
  return 0;
}

/* Returns if passed in TREE node is a boolean expression */
int32_t is_boolean_expression_node(TREE *node)
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
  case S_UNOT:
    return 1;
  }
  return 0;
}

//#ifdef JPFF

static TREE *create_cond_expression(CSOUND *csound,
                                    TREE *root, int32_t line, uint64_t locn,
                                    TYPE_TABLE* typeTable)
{
  TREE *last = NULL;
  int32 ln1 = genlabs++, ln2 = genlabs++;
  TREE *L1 = create_synthetic_label(csound, ln1);
  TREE *L2 = create_synthetic_label(csound, ln2);
  TREE *b = create_boolean_expression(csound, root->left, line, locn,
                                      typeTable);
  TREE *c = root->right->left, *d = root->right->right;
  char *left, *right;
  int32_t type;
  TREE *xx;
  char *eq;

  typeTable->labelList =
    cs_cons(csound,
            cs_strdup(csound, L1->value->lexeme), typeTable->labelList);
  typeTable->labelList =
    cs_cons(csound,
            cs_strdup(csound, L2->value->lexeme), typeTable->labelList);
  //print_tree(csound, "***B\n", b);
  //print_tree(csound, "***C\n", c); print_tree(csound,"***D\n", d);
  left = get_arg_type2(csound, c, typeTable);
  right  = get_arg_type2(csound, d, typeTable);
  //printf("***types %s %s\n", left, right);
  if (left[0]=='c') left[0] = 'i';
  if (right[0]=='c') right[0] = 'i';
  //printf("***type = %c %c\n",left[0], right[0]);
  last = b;
  while (last->next != NULL) {
    last = last->next;
  }
  //p{rintf("type = %s , %s\n", left, right);
  if (left[0]=='S' || right[0]=='S') {
    type = (last->left->value->lexeme[1]=='B') ?2 : 1;
    eq = (last->left->value->lexeme[1]=='B') ?"#=.S" : "=.S";
  }
  else if (left[0] == 'a' && right[0] == 'a') {
    type = 0;
    eq = "=";
  }
  else if (left[0]=='a' || right[0]=='a') {
    csound->Warning(csound, Str("Unbanced rates in conditional expression"));
    return NULL;
  }
  else {
    type =
      (left[0]=='k' || right[0]=='k' || last->left->value->lexeme[1]=='B') ?2 : 1;
    if (type==2) left[0] = right[0] = 'k';
    eq = "=";
  }
  //printf("***boolvalr = %s, type=%d\n", last->left->value->lexeme, type);
  //print_tree(csound, "***\nL1\n", L1);

  last->next = create_opcode_token(csound, type==1?"cigoto":"ckgoto");
  //print_tree(csound, "first jump\n", last->next);
  xx = create_empty_token(csound);
  xx->type = T_IDENT;
  xx->value = make_token(csound, last->left->value->lexeme);
  xx->value->type = T_IDENT;
  last = last->next;
  last->left = NULL;
  last->right = xx;
  last->right->next = L1;
  last->line = line; root->locn = locn;
  //print_tree(csound, "***IF node\n", b);
  // Need to get type of expression for newvariable
  right = create_out_arg(csound,left,
                         typeTable->localPool->synthArgCount++, typeTable);
  //printf("right = %s\n", right);
  {
    TREE *C = create_opcode_token(csound, cs_strdup(csound, eq));
    C->left = create_ans_token(csound, right); C->right = c;
    c = C;
  }
  //print_tree(csound, "\n\nc\n", c);
  {
    TREE *D = create_opcode_token(csound, cs_strdup(csound, eq));
    D->left = create_ans_token(csound, right); D->right = d;
    d = D;
  }
  //print_tree(csound, "\n\nc\n", c);
  //print_tree(csound, "\n\nd\n", d);
  last = b;
  while (last->next != NULL) {
    last = last->next;
  }
  last->next = d;
  while (last->next != NULL) last = last->next;
  //Last is now last assignment
  //print_tree(csound, "\n\nlast assignment\n", last);
  //printf("=======type = %d\n", type);
  last->next = create_simple_goto_token(csound, L2, type==2?0:type);
  //print_tree(csound, "second goto\n", last->next);
  //print_tree(csound, "\n\nafter goto\n", b);
  while (last->next != NULL) last = last->next;
  last->next = create_synthetic_label(csound,ln1);
  while (last->next != NULL) last = last->next;
  //print_tree(csound, "\n\nafter label\n", b);

  last->next = c;
  while (last->next != NULL) last = last->next;
  //print_tree(csound, "n\nAfter c\n", b);
  while (last->next != NULL) last = last->next;
  last->next = create_synthetic_label(csound,ln2);
  //print_tree(csound, "\n\nafter secondlabel\n", b);
  while (last->next != NULL) last = last->next;
  last->next = create_opcode_token(csound, cs_strdup(csound, eq));
  //print_tree(csound, "\n\nafter secondlabel\n", b);
  last->next->left = create_ans_token(csound, right);
  last->next->right = create_ans_token(csound, right);

  //printf("\n\n*** create_cond_expression ends\n");

  //print_tree(csound, "ANSWER\n", b);
  return b;
}

/* static TREE *create_cond_expression(CSOUND *csound, */
/*                                     TREE *root, int32_t line, int32_t locn, */
/*                                     TYPE_TABLE* typeTable) */
/* { */
/*     char *arg1, *arg2, *ans, *outarg = NULL; */
/*     char* outype; */
/*     TREE *anchor = create_boolean_expression(csound, root->left, line, locn, */
/*                                              typeTable); */
/*     TREE *last; */
/*     TREE * opTree; */
/*     TREE *b; */
/*     TREE *c = root->right->left, *d = root->right->right; */
/*     last = anchor; */
/*     char condInTypes[64]; */

/*     while (last->next != NULL) { */
/*       last = last->next; */
/*     } */
/*     b= create_ans_token(csound, last->left->value->lexeme); */
/*     if (is_expression_node(c)) { */
/*       last->next = create_expression(csound, c, line, locn, typeTable); */
/*       /\* TODO - Free memory of old left node */
/*          freetree *\/ */
/*       last = last->next; */
/*       while (last->next != NULL) { */
/*         last = last->next; */
/*       } */
/*       c = create_ans_token(csound, last->left->value->lexeme); */
/*     } */
/*     if (is_expression_node(d)) { */
/*       last->next = create_expression(csound, d, line, locn, typeTable); */
/*       /\* TODO - Free memory of old left node */
/*          freetree *\/ */
/*       last = last->next; */
/*       while (last->next != NULL) { */
/*         last = last->next; */
/*       } */
/*       d = create_ans_token(csound, last->left->value->lexeme); */
/*     } */

/*     arg1 = get_arg_type2(csound, c, typeTable); */
/*     arg2 = get_arg_type2(csound, d, typeTable); */
/*     ans  = get_arg_type2(csound, b, typeTable); */

/*     snprintf(condInTypes, 64, "%s%s%s", ans, arg1, arg2); */

/*     OENTRIES* entries = find_opcode2(csound, ":cond"); */
/*     outype = resolve_opcode_get_outarg(csound, entries, condInTypes); */

/*     if (outype == NULL) { */
/*       csound->Free(csound, entries); */
/*       return NULL; */
/*     } */

/*     outarg = create_out_arg(csound, outype, */
/*                             typeTable->localPool->synthArgCount++, typeTable); */
/*     opTree = create_opcode_token(csound, cs_strdup(csound, ":cond")); */
/*     opTree->left = create_ans_token(csound, outarg); */
/*     opTree->right = b; */
/*     opTree->right->next = c; */
/*     opTree->right->next->next = d; */
/*     /\* should recycle memory for root->right *\/ */
/*     //csound->Free(csound, root->right); root->right = NULL; */
/*     last->next = opTree; */
/*     csound->Free(csound, entries); */
/*     return anchor; */
/* } */

static char* create_out_arg_for_expression(CSOUND* csound, char* op, TREE* left,
                                           TREE* right, TYPE_TABLE* typeTable) {
  char* outType;

  OENTRIES* opentries = find_opcode2(csound, op);

  char* leftArgType = get_arg_string_from_tree(csound, left, typeTable);
  char* rightArgType = get_arg_string_from_tree(csound, right, typeTable);
  char* argString = csound->Calloc(csound, 80);

  strNcpy(argString, leftArgType, 80);
  strlcat(argString, rightArgType, 80);
  
  outType = resolve_opcode_get_outarg(csound, opentries, argString);

  csound->Free(csound, argString);
  csound->Free(csound, leftArgType);
  csound->Free(csound, rightArgType);
  csound->Free(csound, opentries);

  if (outType == NULL) return NULL;

  outType = convert_external_to_internal(csound, outType);
  return create_out_arg(csound, outType,
                        typeTable->localPool->synthArgCount++, typeTable);
}

/**
 * Create a chain of Opcode (OPTXT) text from the AST node given. Called from
 * create_opcode when an expression node has been found as an argument
 */
static TREE *create_expression(CSOUND *csound, TREE *root, int32_t line, uint64_t locn,
                               TYPE_TABLE* typeTable)
{
  char op[80], *outarg = NULL;
  TREE *anchor = NULL, *last;
  TREE * opTree, *current, *newArgList;
  OENTRIES* opentries;
  CS_VARIABLE* var;

  /* HANDLE SUB EXPRESSIONS */

  if (root->type=='?') return create_cond_expression(csound, root, line,
                                                     locn, typeTable);
  memset(op, 0, 80);
  current = root->left;
  newArgList = NULL;
  while (current != NULL) {
    if (is_expression_node(current)) {
      TREE* newArg;

      anchor = appendToTree(csound, anchor,
                            create_expression(csound, current, line, locn,
                                              typeTable));
      last = tree_tail(anchor);
      newArg = create_ans_token(csound, last->left->value->lexeme);
      newArgList = appendToTree(csound, newArgList, newArg);
      current = current->next;
    } else {
      TREE* temp;
      newArgList = appendToTree(csound, newArgList, current);
      temp = current->next;
      current->next = NULL;
      current = temp;
    }

  }
  root->left = newArgList;

  current = root->right;
  newArgList = NULL;
  while (current != NULL) {
    if (is_expression_node(current)) {
      TREE* newArg;

      anchor = appendToTree(csound, anchor,
                            create_expression(csound, current, line,
                                              locn, typeTable));
      last = tree_tail(anchor);

      newArg = create_ans_token(csound, last->left->value->lexeme);
      newArgList = appendToTree(csound, newArgList, newArg);
      current = current->next;
    }
    else {
      TREE* temp;
      newArgList = appendToTree(csound, newArgList, current);
      temp = current->next;
      current->next = NULL;
      current = temp;
    }
  }
  root->right = newArgList;

  switch(root->type) {
  case '+':
    strNcpy(op, "##add", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);
    break;
  case '-':
    strNcpy(op, "##sub", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);
    break;
  case '*':
    strNcpy(op, "##mul", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);
    break;
  case '%':
    strNcpy(op, "##mod", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);
    break;
  case '/':
    strNcpy(op, "##div", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);
    break;
  case '^':
    strNcpy(op, "##pow", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);
    break;
  case T_FUNCTION:
    {
      char *outtype, *outtype_internal;
      int32_t len = (int32_t) strlen(root->value->lexeme);
      strNcpy(op, root->value->lexeme, len+1);
      if (UNLIKELY(PARSER_DEBUG))
        csound->Message(csound, "Found OP: %s\n", op);

      opentries = find_opcode2(csound, root->value->lexeme);
      if (UNLIKELY(opentries->count == 0)) {
        csound->Warning(csound,
                        Str("error: function %s not found, "
                            "line %d\n"),
                        root->value->lexeme, line);
        outtype = "i";
      }
      else {
        char* inArgTypes =
          get_arg_string_from_tree(csound, root->right, typeTable);
        if (root->value->optype != NULL)
          outtype =
            check_annotated_type(csound, opentries, root->value->optype);
        /* if there are type annotations */
        else outtype =
               resolve_opcode_get_outarg(csound, opentries, inArgTypes);
        csound->Free(csound, inArgTypes);
      }

      csound->Free(csound, opentries);

      if (UNLIKELY(outtype == NULL)) {
        csound->Warning(csound,
                        Str("error: opcode %s with output type %s not found, "
                            "line %d"),
                        root->value->lexeme, root->value->optype, line);
        outtype = "i";
      }

      outtype_internal = convert_external_to_internal(csound, outtype);
      outarg = create_out_arg(csound, outtype_internal,
                              typeTable->localPool->synthArgCount++, typeTable);

    }
    break;
  case S_UMINUS:
    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "HANDLING UNARY MINUS!");
    root->left = create_minus_token(csound);
    //      arg1 = 'i';
    strNcpy(op, "##mul", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);

    break;
   case S_UPLUS:
    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "HANDLING UNARY PLUS!");
    root->left = create_minus_token(csound);
    //      arg1 = 'i';
    strNcpy(op, "##mul", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);

    break;

  case '|':
    strNcpy(op, "##or", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);
    break;
  case '&':
    strNcpy(op, "##and", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);
    break;
  case S_BITSHIFT_RIGHT:
    strNcpy(op, "##shr", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);
    break;
  case S_BITSHIFT_LEFT:
    strNcpy(op, "##shl", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);
    break;
  case '#':
    strNcpy(op, "##xor", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);
    break;
  case '~':
    {
      strNcpy(op, "##not", 80);

      opentries = find_opcode2(csound, op);

      char* rightArgType = get_arg_string_from_tree(csound, root->right,
                                                    typeTable);

      if (rightArgType == NULL) {
        return NULL;
      }

      char* outype = resolve_opcode_get_outarg(csound, opentries,
                                               rightArgType);
      csound->Free(csound, rightArgType);
      csound->Free(csound, opentries);

      if (outype == NULL) {
        return NULL;
      }

      outarg = create_out_arg(csound, outype,
                              typeTable->localPool->synthArgCount++, typeTable);

    }
    break;
  case T_ARRAY:
    {
      char* outype;
      strNcpy(op, "##array_get", 80);

      char *varBaseName = root->left->value->lexeme;

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
                Str("unable to find array sub-type for var %s line %d\n"), varBaseName, current->line);
        return NULL;
      } else {
        if (var->varType == &CS_VAR_TYPE_ARRAY) {
          outype = strdup(var->subType->varTypeName);
	  /* VL: 9.2.22 pulled code from 6.x to check for array index type
             to provide the correct outype. Works with explicity types
	  */
         if (outype[0]== 'i') {
          TREE* inds = root->right;
          while (inds) {
            char *xx = get_arg_string_from_tree(csound, inds, typeTable);
            //printf("****array type:%s index type=%s right %s\n", outype, xx, inds->value->lexeme);
            if (xx[0]=='k') {
              outype[0] = 'k';
              break;
            }
            inds = inds->next;
          }
        }
        } else if (var->varType == &CS_VAR_TYPE_A) {
          outype = "k";
        } else {
          synterr(csound,
                  Str("invalid array type %s line %d\n"), var->varType->varTypeName, current->line);
          return NULL;
        }
      }
      if (outype == NULL) {
        return NULL;
      }

      outarg = create_out_arg(csound, outype,
                              typeTable->localPool->synthArgCount++, typeTable);
    }

    break;
    /* it should not get here, but if it does,
       return NULL */
  default:
    return NULL;
  }
  opTree = create_opcode_token(csound, op);
  if (root->value) opTree->value->optype = root->value->optype;
  if (root->left != NULL) {
    opTree->right = root->left;
    opTree->right->next = root->right;
    opTree->left = create_ans_token(csound, outarg);
    opTree->line = line;
    opTree->locn = locn;
    //print_tree(csound, "making expression", opTree);
  }
  else {
    opTree->right = root->right;
    opTree->left = create_ans_token(csound, outarg);
    opTree->line = line;
    opTree->locn = locn;

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
  csound->Free(csound, outarg);
  return anchor;
}

/**
 * Create a chain of Opcode (OPTXT) text from the AST node given. Called from
 * create_opcode when an expression node has been found as an argument
 */
static TREE *create_boolean_expression(CSOUND *csound, TREE *root,
                                       int32_t line, uint64_t locn, TYPE_TABLE* typeTable)
{
  char *op, *outarg;
  TREE *anchor = NULL, *last;
  TREE * opTree;

  if (UNLIKELY(PARSER_DEBUG))
    csound->Message(csound, "Creating boolean expression\n");
  /* HANDLE SUB EXPRESSIONS */
  if (is_boolean_expression_node(root->left)) {
    anchor = create_boolean_expression(csound, root->left,
                                       line, locn, typeTable);
    last = anchor;
    while (last->next != NULL) {
      last = last->next;
    }
    /* TODO - Free memory of old left node
       freetree */
    root->left = create_ans_token(csound, last->left->value->lexeme);
  } else if (is_expression_node(root->left)) {
    anchor = create_expression(csound, root->left, line, locn, typeTable);

    /* TODO - Free memory of old left node
       freetree */
    last = anchor;
    while (last->next != NULL) {
      last = last->next;
    }
    root->left = create_ans_token(csound, last->left->value->lexeme);
  }


  if (is_boolean_expression_node(root->right)) {
    TREE * newRight = create_boolean_expression(csound,
                                                root->right, line, locn,
                                                typeTable);
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
  else if (is_expression_node(root->right)) {
    TREE * newRight = create_expression(csound, root->right, line,
                                        locn, typeTable);

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
    root->line = line;
    root->locn = locn;
  }

  op = csound->Calloc(csound, 80);
  switch(root->type) {
  case S_UNOT:
    strNcpy(op, "!", 80);
    break;
  case S_EQ:
    strNcpy(op, "==", 80);
    break;
  case S_NEQ:
    strNcpy(op, "!=", 80);
    break;
  case S_GE:
    strNcpy(op, ">=", 80);
    break;
  case S_LE:
    strNcpy(op, "<=", 80);
    break;
  case S_GT:
    strNcpy(op, ">", 80);
    break;
  case S_LT:
    strNcpy(op, "<", 80);
    break;
  case S_AND:
    strNcpy(op, "&&", 80);
    break;
  case S_OR:
    strNcpy(op, "||", 80);
    break;
  }

  if (UNLIKELY(PARSER_DEBUG)) {
    if (root->type == S_UNOT)
      csound->Message(csound, "Operator Found: %s (%c)\n", op,
                      argtyp2( root->left->value->lexeme));
    else
      csound->Message(csound, "Operator Found: %s (%c %c)\n", op,
                      argtyp2( root->left->value->lexeme),
                      argtyp2( root->right->value->lexeme));
  }
  if (root->type == S_UNOT) {
    outarg = get_boolean_arg(csound,
                             typeTable,
                             argtyp2( root->left->value->lexeme) =='k' ||
                             argtyp2( root->left->value->lexeme) =='B');
  }
  else
    outarg = get_boolean_arg(csound,
                             typeTable,
                             argtyp2( root->left->value->lexeme) =='k' ||
                             argtyp2( root->right->value->lexeme)=='k' ||
                             argtyp2( root->left->value->lexeme) =='B' ||
                             argtyp2( root->right->value->lexeme)=='B');

  add_arg(csound, outarg, NULL, typeTable);
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
  csound->Free(csound, outarg);
  csound->Free(csound, op);
  return anchor;
}

static char* create_synthetic_var_name(CSOUND* csound, int32 count, int32_t prefix)
{
  char *name = (char *)csound->Calloc(csound, 36);
  snprintf(name, 36, "%c__synthetic_%"PRIi32, prefix, count);
  return name;
}

static char* create_synthetic_array_var_name(CSOUND* csound, int32 count, int32_t prefix)
{
  char *name = (char *)csound->Calloc(csound, 36);
  snprintf(name, 36, "%c__synthetic_%"PRIi32"[]", prefix, count);
  return name;
}

static TREE *create_synthetic_ident(CSOUND *csound, int32 count)
{
  char *label = (char *)csound->Calloc(csound, 32);
  ORCTOKEN *token;
  snprintf(label, 32, "__synthetic_%"PRIi32, count);
  if (UNLIKELY(PARSER_DEBUG))
    csound->Message(csound, "Creating Synthetic T_IDENT: %s\n", label);
  token = make_token(csound, label);
  token->type = T_IDENT;
  csound->Free(csound, label);
  return make_leaf(csound, -1, 0, T_IDENT, token);
}

static TREE *create_synthetic_label(CSOUND *csound, int32 count)
{
  char *label = (char *)csound->Calloc(csound, 32);
  ORCTOKEN *token;
  snprintf(label, 32, "__synthetic_%"PRIi32":", count);
  if (UNLIKELY(PARSER_DEBUG))
    csound->Message(csound, "Creating Synthetic label: %s\n", label);
  token = make_label(csound, label);
  if (UNLIKELY(PARSER_DEBUG))
    printf("**** label lexeme >>%s<<\n", token->lexeme);
  csound->Free(csound, label);
  return make_leaf(csound, -1, 0, LABEL_TOKEN, token);
}

void handle_negative_number(CSOUND* csound, TREE* root)
{
  if (root->type == S_UMINUS &&
      (root->right->type == INTEGER_TOKEN || root->right->type == NUMBER_TOKEN)) {
    int32_t len = (int32_t) strlen(root->right->value->lexeme);
    char* negativeNumber = csound->Malloc(csound, len + 3);
    negativeNumber[0] = '-';
    strcpy(negativeNumber + 1, root->right->value->lexeme);
    negativeNumber[len + 2] = '\0';
    root->type = root->right->type;
    root->value = root->right->type == INTEGER_TOKEN ?
      make_int(csound, negativeNumber) : make_num(csound, negativeNumber);
    root->value->lexeme = negativeNumber;
  }
}


static void collapse_last_assigment(CSOUND* csound, TREE* anchor,
                                    TYPE_TABLE* typeTable)
{
  TREE *a, *b, *temp;
  temp = anchor;

  if (temp == NULL || temp->next == NULL) {
    return;
  }

  while (temp->next != NULL) {
    a = temp;
    b = temp->next;
    temp = temp->next;
  }

  if (b == NULL || a->left == NULL ||
      b->left == NULL || b->right == NULL) {
    return;
  }
  char *tmp1 = get_arg_type2(csound, b->left, typeTable);
  char *tmp2 = get_arg_type2(csound, b->right, typeTable);
  //print_tree(csound, "b", b);
  //print_tree(csound, "a", a);
  //printf("b->type = %d, tmp`1 %s tmp2 %s\n", b->type, tmp1, tmp2);
  if ((b->type == '=') &&
      (!strcmp(a->left->value->lexeme, b->right->value->lexeme)) &&
      (!strcmp(tmp1, tmp2))) {
    a->left = b->left;
    a->next = NULL;
    csound->Free(csound, b);
  }
  csound->Free(csound, tmp1);
  csound->Free(csound, tmp2);
  //print_tree(csound, "returns\n", a);
}

/* returns the head of a list of TREE* nodes, expanding all RHS
   expressions into statements prior to the original statement line,
   and LHS expressions (array sets) after the original statement
   line */
TREE* expand_statement(CSOUND* csound, TREE* current, TYPE_TABLE* typeTable)
{
  /* This is WRONG in optional argsq */
  TREE* anchor = NULL;
  TREE* originalNext = current->next;

  TREE* previousArg = NULL;
  TREE* currentArg = current->right;

  current->next = NULL;

  if (UNLIKELY(PARSER_DEBUG))
    csound->Message(csound, "Found Statement.\n");
  while (currentArg != NULL) {
    TREE* last;
    TREE *nextArg;
    TREE *newArgTree;
    TREE *expressionNodes;
    int32_t is_bool = 0;
    handle_negative_number(csound, currentArg);
    if (is_expression_node(currentArg) ||
        (is_bool = is_boolean_expression_node(currentArg))) {
      char * newArg;
      if (UNLIKELY(PARSER_DEBUG))
        csound->Message(csound, "Found Expression.\n");
      if (is_bool == 0) {
        expressionNodes =
          create_expression(csound, currentArg,
                            currentArg->line, currentArg->locn, typeTable);
        // free discarded node
      }
      else {
        expressionNodes =
          create_boolean_expression(csound, currentArg,
                                    currentArg->line, currentArg->locn,
                                    typeTable);
      }
      nextArg = currentArg->next;
      csound->Free(csound, currentArg);

      /* Set as anchor if necessary */

      anchor = appendToTree(csound, anchor, expressionNodes);

      /* reconnect into chain */
      last = tree_tail(expressionNodes);
      newArg = last->left->value->lexeme;

      if (UNLIKELY(PARSER_DEBUG))
        csound->Message(csound, "New Arg: %s\n", newArg);

      /* handle arg replacement of currentArg here */
      /* **** was a bug as currentArg could be freed above **** */
      //nextArg = currentArg->next;
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

  anchor = appendToTree(csound, anchor, current);


  // handle LHS expressions (i.e. array-set's)
  previousArg = NULL;
  currentArg = current->left;
  int32_t init = 0;
  if (strcmp("init", current->value->lexeme)==0) {
    //print_tree(csound, "init",current);
    init = 1;
  }
  while (currentArg != NULL) {
    TREE* temp;

    if (currentArg->type == T_ARRAY) {
      char *outType;
      CS_VARIABLE* var;

      char *varBaseName = currentArg->left->value->lexeme;

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
                Str("unable to find array sub-type for var %s line %d\n"), varBaseName, current->line);
        return NULL;
      } else {
        if (var->varType == &CS_VAR_TYPE_ARRAY) {
          outType = strdup(var->subType->varTypeName);
        } else if (var->varType == &CS_VAR_TYPE_A) {
          outType = "k";
        } else {
          synterr(csound,
                  Str("invalid array type %s line %d\n"), var->varType->varTypeName, current->line);
          return NULL;
        }
      }

      temp =
        create_ans_token(csound,
                         create_out_arg(csound, outType,
                                        typeTable->localPool->synthArgCount++,
                                        typeTable));

      if (previousArg == NULL) {
        current->left = temp;
      }
      else {
        previousArg->next = temp;
      }
      temp->next = currentArg->next;

      TREE* arraySet = create_opcode_token(csound,
                                           (init ? "##array_init":
                                            "##array_set"));
      arraySet->right = currentArg->left;
      arraySet->right->next =
        make_leaf(csound, temp->line, temp->locn,
                  T_IDENT, make_token(csound,
                                      temp->value->lexeme));
      arraySet->right->next->next =
        currentArg->right; // TODO - check if this handles expressions

      anchor = appendToTree(csound, anchor, arraySet);
      //print_tree(csound, "anchor", anchor);
      currentArg = temp;
    }
    previousArg = currentArg;
    currentArg = currentArg->next;
  }

  handle_optional_args(csound, current);

  collapse_last_assigment(csound, anchor, typeTable);

  appendToTree(csound, anchor, originalNext);

  return anchor;
}

/* Flattens one level of if-blocks, sub-if-blocks should get flattened
   when the expander goes through statements */
TREE* expand_if_statement(CSOUND* csound,
                          TREE* current, TYPE_TABLE* typeTable) {

  TREE* anchor = NULL;
  TREE* expressionNodes = NULL;

  TREE* left = current->left;
  TREE* right = current->right;
  TREE* last;
  TREE* gotoToken;

  if (right->type == IGOTO_TOKEN ||
      right->type == KGOTO_TOKEN ||
      right->type == GOTO_TOKEN) {
    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "Found if-goto\n");
    expressionNodes =
      create_boolean_expression(csound, left, right->line,
                                right->locn, typeTable);


    anchor = appendToTree(csound, anchor, expressionNodes);

    /* reconnect into chain */
    last = tree_tail(expressionNodes);

    gotoToken = create_goto_token(csound,
                                  last->left->value->lexeme,
                                  right,
                                  last->left->type == 'k' ||
                                  right->type =='k');
    last->next = gotoToken;
    gotoToken->next = current->next;
  }
  else if (LIKELY(right->type == THEN_TOKEN ||
                  right->type == ITHEN_TOKEN ||
                  right->type == KTHEN_TOKEN)) {
    int32_t endLabelCounter = -1;
    TREE *tempLeft;
    TREE *tempRight;
    TREE* last;

    TREE *ifBlockCurrent = current;

    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "Found if-then\n");
    if (right->next != NULL) {
      endLabelCounter = genlabs++;
    }

    while (ifBlockCurrent != NULL) {
      tempLeft = ifBlockCurrent->left;
      tempRight = ifBlockCurrent->right;

      if (ifBlockCurrent->type == ELSE_TOKEN) {
        appendToTree(csound, anchor, tempRight);
        break;
      }

      expressionNodes =
        create_boolean_expression(csound, tempLeft,
                                  tempLeft->line, tempLeft->locn,
                                  typeTable);

      anchor = appendToTree(csound, anchor, expressionNodes);

      last = tree_tail(expressionNodes);

      /* reconnect into chain */
      {
        TREE *statements, *label, *labelEnd, *gotoToken;
        int32_t gotoType;

        statements = tempRight->right;
        label = create_synthetic_ident(csound, genlabs);
        labelEnd = create_synthetic_label(csound, genlabs++);
        tempRight->right = label;

        typeTable->labelList =
          cs_cons(csound,
                  cs_strdup(csound,
                            labelEnd->value->lexeme),
                  typeTable->labelList);
        //printf("allocate label %s\n", typeTable->labelList->value );

        gotoType = // checking for #B... var name
          (last->left->value->lexeme[1] == 'B');
        gotoToken =
          create_goto_token(csound,
                            last->left->value->lexeme,
                            tempRight,
                            gotoType);
        gotoToken->next = statements;
        anchor = appendToTree(csound, anchor, gotoToken);

        /* relinking */
        last = tree_tail(last);

        if (endLabelCounter > 0) {
          TREE *endLabel = create_synthetic_ident(csound,
                                                  endLabelCounter);
          int32_t type = (gotoType == 1) ? 0 : 2;
          /* csound->DebugMsg(csound, "%s(%d): type = %d %d\n", */
          /*        __FILE__, __LINE__, type, gotoType); */
          TREE *gotoEndLabelToken =
            create_simple_goto_token(csound, endLabel, type);
          if (UNLIKELY(PARSER_DEBUG))
            csound->Message(csound, "Creating simple goto token\n");

          appendToTree(csound, last, gotoEndLabelToken);

          gotoEndLabelToken->next = labelEnd;
        }
        else {
          appendToTree(csound, last, labelEnd);
        }

        ifBlockCurrent = tempRight->next;
      }
    }

    if (endLabelCounter > 0) {
      TREE *endLabel = create_synthetic_label(csound,
                                              endLabelCounter);
      anchor = appendToTree(csound, anchor, endLabel);

      typeTable->labelList = cs_cons(csound,
                                     cs_strdup(csound,
                                               endLabel->value->lexeme),
                                     typeTable->labelList);
      //printf("allocate label %s\n", typeTable->labelList->value );
    }

    anchor = appendToTree(csound, anchor, current->next);

  }
  else {
    csound->Message(csound,
                    Str("ERROR: Neither if-goto or if-then found on line %d!!!"),
                    right->line);
  }

  return anchor;
}

/* 1. create top label to loop back to
   2. do boolean expression
   3. do goto token that checks boolean and goes to end label
   4. insert statements
   5. add goto token that goes to top label
   6. end label */
TREE* expand_until_statement(CSOUND* csound, TREE* current,
                             TYPE_TABLE* typeTable, int32_t dowhile)
{
  TREE* anchor = NULL;
  TREE* expressionNodes = NULL;

  TREE* gotoToken;

  int32 topLabelCounter = genlabs++;
  int32 endLabelCounter = genlabs++;
  TREE* tempRight = current->right;
  TREE* last = NULL;
  TREE* labelEnd;
  int32_t gotoType;

  anchor = create_synthetic_label(csound, topLabelCounter);
  typeTable->labelList = cs_cons(csound,
                                 cs_strdup(csound, anchor->value->lexeme),
                                 typeTable->labelList);

  expressionNodes = create_boolean_expression(csound,
                                              current->left,
                                              current->line,
                                              current->locn,
                                              typeTable);
  anchor = appendToTree(csound, anchor, expressionNodes);
  last = tree_tail(anchor);

  labelEnd = create_synthetic_label(csound, endLabelCounter);
  typeTable->labelList = cs_cons(csound,
                                 cs_strdup(csound, labelEnd->value->lexeme),
                                 typeTable->labelList);

  gotoType =
    last->left->value->lexeme[1] == 'B'; // checking for #B... var name

  // printf("%s\n", last->left->value->lexeme);
  //printf("gottype = %d ; dowhile = %d\n", gotoType, dowhile);
  gotoToken =
    create_goto_token(csound,
                      last->left->value->lexeme,
                      labelEnd,
                      gotoType+0x8000*dowhile);
  gotoToken->next = tempRight;
  gotoToken->right->next = labelEnd;


  last = appendToTree(csound, last, gotoToken);
  last = tree_tail(last);


  labelEnd = create_synthetic_label(csound, endLabelCounter);
  TREE *topLabel = create_synthetic_ident(csound,
                                          topLabelCounter);
  TREE *gotoTopLabelToken = create_simple_goto_token(csound,
                                                     topLabel,
                                                     (gotoType==1 ? 0 : 1));

  appendToTree(csound, last, gotoTopLabelToken);
  gotoTopLabelToken->next = labelEnd;


  labelEnd->next = current->next;
  return anchor;
}

TREE* expand_for_statement(
  CSOUND* csound,
  TREE* current,
  TYPE_TABLE* typeTable,
  char* arrayArgType
) {

  CS_TYPE *iType = (CS_TYPE *)&CS_VAR_TYPE_I;
  CS_TYPE *kType = (CS_TYPE *)&CS_VAR_TYPE_K;

  int32_t isPerfRate = arrayArgType[1] == 'k';
  char* op = (char *)csound->Malloc(csound, 10);
  // create index counter
  TREE *indexAssign = create_empty_token(csound);
  indexAssign->value = make_token(csound, "=");
  indexAssign->type = T_ASSIGNMENT;
  indexAssign->value->type = T_ASSIGNMENT;
  char *indexName = create_synthetic_var_name(
    csound,
    genlabs++,
    isPerfRate ? 'k' : 'i'
  );
  TREE *indexIdent = create_empty_token(csound);
  indexIdent->value = make_token(csound, indexName);
  indexIdent->type = T_IDENT;
  indexIdent->value->type = T_IDENT;
  TREE *zeroToken = create_empty_token(csound);
  zeroToken->value = make_token(csound, "0");
  zeroToken->value->value = 0;
  zeroToken->type = INTEGER_TOKEN;
  zeroToken->value->type = INTEGER_TOKEN;
  indexAssign->left = indexIdent;
  indexAssign->right = zeroToken;

  TREE *arrayAssign = create_empty_token(csound);
  arrayAssign->value = make_token(csound, "=");
  arrayAssign->type = T_ASSIGNMENT;
  arrayAssign->value->type = T_ASSIGNMENT;
  char *arrayName = create_synthetic_array_var_name(
    csound,
    genlabs++,
    isPerfRate ? 'k' : 'i'
  );
  TREE *arrayIdent = create_empty_token(csound);
  arrayIdent->value = make_token(csound, arrayName);
  arrayIdent->type = T_ARRAY_IDENT;
  arrayIdent->value->type = T_ARRAY_IDENT;
  add_array_arg(csound, arrayName, NULL, 1, typeTable);

  arrayAssign->left = arrayIdent;
  arrayAssign->right = current->right->left;
  indexAssign->next = arrayAssign;

  TREE *arrayLength = create_empty_token(csound);
  arrayLength->value = make_token(csound, "=");
  arrayLength->type = T_ASSIGNMENT;
  arrayLength->value->type = T_ASSIGNMENT;
  char *arrayLengthName = create_synthetic_var_name(
    csound,
    genlabs++,
    isPerfRate ? 'k' : 'i'
  );
  TREE *arrayLengthIdent = create_empty_token(csound);
  arrayLengthIdent->value = make_token(csound, arrayLengthName);
  arrayLengthIdent->type = T_IDENT;
  arrayLengthIdent->value->type = T_IDENT;
  arrayLength->left = arrayLengthIdent;
  TREE *arrayLengthFn = create_empty_token(csound);
  arrayLengthFn->value = make_token(csound, "lenarray");
  arrayLengthFn->type = T_FUNCTION;
  arrayLengthFn->value->type = T_FUNCTION;
  TREE *arrayLengthArrayIdent = copy_node(csound, arrayIdent);
  arrayLengthFn->right = arrayLengthArrayIdent;
  arrayLength->right = arrayLengthFn;
  arrayAssign->next = arrayLength;


  TREE* loopLabel = create_synthetic_label(csound, genlabs++);
  loopLabel->type = LABEL_TOKEN;
  loopLabel->value->type = LABEL_TOKEN;
  CS_VARIABLE *loopLabelVar = csoundCreateVariable(
      csound, csound->typePool, isPerfRate ? kType : iType, loopLabel->value->lexeme, NULL);
  csoundAddVariable(csound, typeTable->localPool, loopLabelVar);
  typeTable->labelList = cs_cons(csound,
                                 cs_strdup(csound, loopLabel->value->lexeme),
                                 typeTable->labelList);

  arrayLength->next = loopLabel;

  // handle case where user provided an index identifier
  int32_t hasOptionalIndex = 0;
  if (current->left->next != NULL) {
    hasOptionalIndex = 1;
    TREE *optionalUserIndexAssign = create_empty_token(csound);
    optionalUserIndexAssign->value = make_token(csound, "=");
    optionalUserIndexAssign->type = T_ASSIGNMENT;
    optionalUserIndexAssign->value->type = T_ASSIGNMENT;
    optionalUserIndexAssign->left = current->left->next;
    optionalUserIndexAssign->right = copy_node(csound, indexIdent);
    current->left->next = NULL;
    loopLabel->next = optionalUserIndexAssign;
  }

  TREE* arrayGetStatement = create_opcode_token(csound, "##array_get");
  arrayGetStatement->left = current->left;
  arrayGetStatement->right = copy_node(csound, arrayIdent);
  arrayGetStatement->right->next = copy_node(csound, indexIdent);
  if (hasOptionalIndex) {
    loopLabel->next->next = arrayGetStatement;
  } else {
    loopLabel->next = arrayGetStatement;
  }
  arrayGetStatement->next = current->right->right;

  strNcpy(op, isPerfRate ? "loop_lt.k" : "loop_lt.i", 10);

  TREE* loopLtStatement = create_opcode_token(csound, op);
  TREE* tail = tree_tail(current->right->right);
  tail->next = loopLtStatement;

  TREE* indexArgToken = copy_node(csound, indexIdent);
  loopLtStatement->right = indexArgToken;

  // loop less-than arg1: increment by 1
  TREE *oneToken = create_empty_token(csound);
  oneToken->value = make_token(csound, "1");
  oneToken->value->value = 1;
  oneToken->type = INTEGER_TOKEN;
  oneToken->value->type = INTEGER_TOKEN;
  indexArgToken->next = oneToken;

  // loop less-than arg2: max iterations (length of the array)
  TREE* arrayLengthArgToken = copy_node(csound, arrayLengthIdent);

  oneToken->next = arrayLengthArgToken;

  // loop less-than arg3: goto label
  TREE *labelGotoIdent = create_empty_token(csound);
  labelGotoIdent->value = make_token(csound, loopLabel->value->lexeme);
  labelGotoIdent->type = T_IDENT;
  labelGotoIdent->value->type = T_IDENT;
  arrayLengthArgToken->next = labelGotoIdent;


  csound->Free(csound, indexName);
  csound->Free(csound, arrayName);
  csound->Free(csound, arrayLengthName);
  csound->Free(csound, op);

  return indexAssign;
}

int32_t is_statement_expansion_required(TREE* root) {
  TREE* current = root->right;
  while (current != NULL) {
    if (is_boolean_expression_node(current) || is_expression_node(current)) {
      return 1;
    }
    current = current->next;
  }

  /*  VL: do we  need  to always expand  ARRAY expressions?
      would this lead to unecessary copying at times?
   */
  current = root->left;
  while (current != NULL) {
    if (current->type == T_ARRAY) {
      return 1;
    }
    current = current->next;
  }
  return 0;
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

//TREE *csound_orc_expand_expressions(CSOUND * csound, TREE *root)
//{
//    //    int32 labelCounter = 300L;
//
//    TREE *anchor = NULL;
//    TREE * expressionNodes = NULL;
//
//    TREE *current = root;
//    TREE *previous = NULL;
//
//    if (UNLIKELY(PARSER_DEBUG))
//      csound->Message(csound, "[Begin Expanding Expressions in AST]\n");
//
//    while (current != NULL) {
//      switch(current->type) {
//      case INSTR_TOKEN:
//        if (UNLIKELY(PARSER_DEBUG))
//          csound->Message(csound, "Instrument found\n");
//        current->right = csound_orc_expand_expressions(csound, current->right);
//        //print_tree(csound, "AFTER", current);
//        break;
//
//      case UDO_TOKEN:
//        if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "UDO found\n");
//        current->right = csound_orc_expand_expressions(csound, current->right);
//        break;
//
//      case IF_TOKEN:
//        if (UNLIKELY(PARSER_DEBUG))
//          csound->Message(csound, "Found IF statement\n");
//
//        current = expand_if_statement(csound, current);
//
//        if (previous != NULL) {
//            previous->next = current;
//        }
//
//        continue;
//      case UNTIL_TOKEN:
//        if (UNLIKELY(PARSER_DEBUG))
//          csound->Message(csound, "Found UNTIL statement\n");
//
//        current = expand_until_statement(csound, current);
//
//        if (previous != NULL) {
//          previous->next = current;
//        }
//
//        continue;
//
//      case LABEL_TOKEN:
//        break;
//
//      default:
//        //maincase:
//        if (is_statement_expansion_required(current)) {
//            current = expand_statement(csound, current);
//
//            if (previous != NULL) {
//                previous->next = current;
//            }
//            continue;
//        } else {
//            handle_optional_args(csound, current);
//        }
//      }
//
//      if (anchor == NULL) {
//        anchor = current;
//      }
//      previous = current;
//      current = current->next;
//    }
//
//    if (UNLIKELY(PARSER_DEBUG))
//      csound->Message(csound, "[End Expanding Expressions in AST]\n");
//
//    return anchor;
//}
