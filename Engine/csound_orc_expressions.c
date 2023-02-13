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
#include <inttypes.h>

extern char argtyp2(char *);
extern void print_tree(CSOUND *, char *, TREE *);
extern void handle_optional_args(CSOUND *, TREE *);
extern ORCTOKEN *make_token(CSOUND *, char *);
extern ORCTOKEN *make_label(CSOUND *, char *);
extern OENTRIES* find_opcode2(CSOUND *, char*);
extern char* resolve_opcode_get_outarg(CSOUND* , OENTRIES* , char*);
extern TREE* appendToTree(CSOUND * csound, TREE *first, TREE *newlast);
extern  char* get_arg_string_from_tree(CSOUND*, TREE*, TYPE_TABLE*);
extern void add_arg(CSOUND* csound, char* varName, char* annotation, TYPE_TABLE* typeTable);
extern void add_array_arg(CSOUND* csound, char* varName, char* annotation, int dimensions,
                          TYPE_TABLE* typeTable);

extern char* get_array_sub_type(CSOUND* csound, char*);

extern char* convert_external_to_internal(CSOUND* csound, char* arg);


static TREE *create_boolean_expression(CSOUND*, TREE*, int, int, TYPE_TABLE*);
static TREE *create_expression(CSOUND *, TREE *, TREE *, int, int, TYPE_TABLE*);
char *check_annotated_type(CSOUND* csound, OENTRIES* entries,
                           char* outArgTypes);
static TREE *create_synthetic_label(CSOUND *csound, int32 count);
static TREE *create_synthetic_ident(CSOUND*, int32);
extern void do_baktrace(CSOUND *csound, uint64_t files);
extern CS_TYPE* csoundGetTypeWithVarTypeName(TYPE_POOL*, char*);




static int genlabs = 300;

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

char *create_out_arg(CSOUND *csound, char* outype, int argCount,
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
    // FIXME - struct arrays
    if (*outype == '[') {
      snprintf(s, 16, "#%c%d[]", outype[1], argCount);
      add_array_arg(csound, s, NULL, 1, typeTable);
    } else {
      //            char* argType = cs_strndup(csound, outype + 1, strlen(outype) - 2);
      snprintf(s, 256, "#%s%d", outype, argCount);
      add_arg(csound, s, outype, typeTable);
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

char * get_boolean_arg(CSOUND *csound, TYPE_TABLE* typeTable, int type)
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

/* Returns true if passed in TREE node is a numerical expression */
int is_expression_node(TREE *node)
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
  case STRUCT_EXPR:
    return 1;
  }
  return 0;
}

int is_boolean_tree_type(int treeType) {
  switch(treeType) {
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

/* Returns if passed in TREE node is a boolean expression */
int is_boolean_expression_node(TREE *node)
{
  if (node == NULL) {
    return 0;
  }
  return is_boolean_tree_type(node->type);
}

extern int check_satisfies_expected_input(
    CS_TYPE*, char* , int
);

static TREE *create_ternay_expression(
  CSOUND *csound,
  TREE *root,
  int line,
  int locn,
  TYPE_TABLE* typeTable
) {
  TREE *last = NULL;
  int32 ln1 = genlabs++, ln2 = genlabs++;
  TREE *L1 = create_synthetic_label(csound, ln1);
  TREE *L2 = create_synthetic_label(csound, ln2);
  TREE *anchorBool = create_boolean_expression(
    csound, root->left, line, locn, typeTable
  );
  TREE *truthyTree = root->right->left;
  TREE *falsyTree = root->right->right;
  TREE* left;
  TREE *boolIdent;

  typeTable->labelList =
    cs_cons(csound,
            cs_strdup(csound, L1->value->lexeme), typeTable->labelList);
  typeTable->labelList =
    cs_cons(csound,
            cs_strdup(csound, L2->value->lexeme), typeTable->labelList);

  last = anchorBool;
  while (last->next != NULL) {
    last = last->next;
  }

  last->next = create_opcode_token(csound, "##ternary");
  left = create_synthetic_ident(csound, genlabs++);
  boolIdent = create_ans_token(csound, last->left->value->lexeme);

  last = last->next;
  last->left = NULL;
  last->right = boolIdent;
  last->right->next = L1;
  last->line = line;
  root->locn = locn;

  {
    TREE *C = create_opcode_token(csound, cs_strdup(csound, "="));
    C->left = copy_node(csound, left);
    C->right = truthyTree;
    truthyTree = C;
  }

  {

    TREE *D = create_opcode_token(csound, cs_strdup(csound, "##ternary-assign"));
    D->left = left;
    D->right = falsyTree;
    falsyTree = D;
  }

  appendToTree(csound, last, truthyTree);
  last = tree_tail(last);
  appendToTree(csound, last,
    create_opcode_token(csound, "##endternary")
  );
  last = tree_tail(last);
  last->value->optype = csound->Strdup(csound, boolIdent->value->lexeme);
  last->right = L2;
  last->next = create_synthetic_label(csound,ln1);
  last = last->next;
  last->next = falsyTree;
  last = tree_tail(last);
  last->next = create_synthetic_label(csound, ln2);
  return anchorBool;
}

/**
 * Create a chain of Opcode (OPTXT) text from the AST node given. Called from
 * create_opcode when an expression node has been found as an argument
 */
static TREE *create_expression(
  CSOUND *csound, TREE *root, TREE *parent,
  int line, int locn,
  TYPE_TABLE* typeTable
) {
  char op[80];
  TREE *anchor = NULL, *last;
  TREE * opTree, *current, *newArgList, *previous;

  /* HANDLE SUB EXPRESSIONS */

  if (root->type=='?') {
    return create_ternay_expression(
      csound, root, line,
      locn, typeTable
    );
  }

  memset(op, 0, 80);
  current = root->left;
  previous = root;
  newArgList = NULL;
  while (current != NULL) {
    if (current->type == STRUCT_EXPR) {
      TREE* opcodeCallNode = create_opcode_token(csound, "##member_get");
      TREE* syntheticIdent = create_synthetic_ident(csound, genlabs++);
      if (current->left->type == STRUCT_EXPR) {
        anchor = appendToTree(csound, anchor,
                            create_expression(
                              csound, current->left,
                              previous, line, locn,
                              typeTable
                            ));
        last = tree_tail(anchor);
        opcodeCallNode->right = copy_node(csound, last->left);
        opcodeCallNode->right->next = current->right;
        if (opcodeCallNode->right->next->value->optype == NULL) {
          opcodeCallNode->right->next->value->optype = csound->Strdup(
            csound, opcodeCallNode->right->value->lexeme
          );
        }
      } else if (current->left->type == T_ARRAY) {
        anchor = appendToTree(csound, anchor,
                            create_expression(
                              csound, current->left,
                              previous, line, locn,
                              typeTable
                            ));
        last = tree_tail(anchor);
        opcodeCallNode->right = copy_node(csound, last->left);
        opcodeCallNode->right->next = current->right;
      } else {
        opcodeCallNode->right = current->left;
        opcodeCallNode->right->next = current->right;
      }

      current->right = NULL;
      opcodeCallNode->left = syntheticIdent;
      if (newArgList == NULL) {
        newArgList = copy_node(csound, syntheticIdent);
      } else {
        appendToTree(csound, newArgList, copy_node(csound, syntheticIdent));
      }
      anchor = appendToTree(csound, anchor, opcodeCallNode);
      previous = current;
      current = current->next;

    }
    else
    if (is_expression_node(current)) {
      TREE* newArg;

      anchor = appendToTree(csound, anchor,
                            create_expression(
                              csound, current,
                              previous, line, locn,
                              typeTable
                            ));
      last = tree_tail(anchor);
      newArg = create_ans_token(csound, last->left->value->lexeme);
      newArgList = appendToTree(csound, newArgList, newArg);
      previous = current;
      current = current->next;
    } else {
      TREE* temp;
      newArgList = appendToTree(csound, newArgList, current);
      temp = current->next;
      current->next = NULL;
      previous = current;
      current = temp;
    }

  }
  root->left = newArgList;

  current = root->right;
  previous = root;
  newArgList = NULL;
  while (current != NULL) {
    if (is_expression_node(current)) {
      TREE* newArg;

      anchor = appendToTree(csound, anchor,
                            create_expression(csound, current, previous, line,
                                              locn, typeTable));
      last = tree_tail(anchor);

    if (last->type == LABEL_TOKEN) {
        // second last will do it
        TREE* secondLast = anchor;
        while (secondLast->next != last) {
          secondLast = secondLast->next;
        }
        newArg = create_ans_token(csound, secondLast->left->value->lexeme);
      } else {
        newArg = create_ans_token(csound, last->left->value->lexeme);
      }


      newArgList = appendToTree(csound, newArgList, newArg);
      previous = current;
      current = current->next;
    }
    else {
      TREE* temp;
      newArgList = appendToTree(csound, newArgList, current);
      temp = current->next;
      current->next = NULL;
      previous = current;
      current = temp;
    }
  }
  root->right = newArgList;

  switch(root->type) {
  case STRUCT_EXPR: {
    strNcpy(op, "##member_get", 80);
    break;
  }
  case '+':
    strNcpy(op, "##add", 80);
    break;
  case '-':
    strNcpy(op, "##sub", 80);
    break;
  case '*':
    strNcpy(op, "##mul", 80);
    break;
  case '%':
    strNcpy(op, "##mod", 80);
    break;
  case '/':
    strNcpy(op, "##div", 80);
    break;
  case '^':
    strNcpy(op, "##pow", 80);
    break;
  case T_FUNCTION:
    strNcpy(op, root->value->lexeme, strlen(root->value->lexeme) + 1);
    break;
  case S_UMINUS:
    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "HANDLING UNARY MINUS!");
    root->left = create_minus_token(csound);
    //      arg1 = 'i';
    strNcpy(op, "##mul", 80);
    break;
   case S_UPLUS:
    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "HANDLING UNARY PLUS!");
    root->left = create_minus_token(csound);
    //      arg1 = 'i';
    strNcpy(op, "##mul", 80);
    break;

  case '|':
    strNcpy(op, "##or", 80);
    break;
  case '&':
    strNcpy(op, "##and", 80);
    break;
  case S_BITSHIFT_RIGHT:
    strNcpy(op, "##shr", 80);
    break;
  case S_BITSHIFT_LEFT:
    strNcpy(op, "##shl", 80);
    break;
  case '#':
    strNcpy(op, "##xor", 80);
    break;
  case '~':
    strNcpy(op, "##not", 80);
    break;
  case T_ARRAY:
    {
      int isStructArray = parent != NULL &&
        (parent->type == STRUCT_EXPR ||
          (parent->left != NULL && parent->left->type == STRUCT_EXPR));
      strNcpy(
        op,
        isStructArray ? "##array_get_struct" : "##array_get",
        80);
      char* outype = csound->Calloc(csound, 2 * sizeof(char));
      outype[0] = '.';
      outype[1] = '\0';
      break;
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
    opTree->left = create_synthetic_ident(csound, genlabs++);
    opTree->line = line;
    opTree->locn = locn;
    //print_tree(csound, "making expression", opTree);
  }
  else {
    opTree->right = root->right;
    opTree->left = create_synthetic_ident(csound, genlabs++);
    opTree->line = line;
    opTree->locn = locn;
    if (
      opTree->value != NULL &&
      opTree->value->optype != NULL
    ) {
      opTree->left->value->optype = csound->Strdup(
        csound, opTree->value->optype
      );
    }

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

  return anchor;
}

/**
 * Create a chain of Opcode (OPTXT) text from the AST node given. Called from
 * create_opcode when an expression node has been found as an argument
 */
static TREE *create_boolean_expression(CSOUND *csound, TREE *root,
                                       int line, int locn, TYPE_TABLE* typeTable)
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
    anchor = create_expression(csound, root->left, root, line, locn, typeTable);

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
    TREE * newRight = create_expression(csound, root->right, root, line,
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
  opTree->value->type = root->type;
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

static TREE *create_synthetic_ident(CSOUND *csound, int32 count)
{
  char *label = (char *)csound->Calloc(csound, 32);
  ORCTOKEN *token;
  snprintf(label, 32, "#_%"PRIi32, count);
  if (UNLIKELY(PARSER_DEBUG))
    csound->Message(csound, "Creating Synthetic T_IDENT: %s\n", label);
  token = make_token(csound, label);
  token->type = T_IDENT;
  csound->Free(csound, label);
  return make_leaf(csound, -1, 0, T_IDENT, token);
}

static TREE *create_synthetic_label_ident(CSOUND *csound, int32 count)
{
  char *label = (char *)csound->Calloc(csound, 32);
  ORCTOKEN *token;
  snprintf(label, 32, "__synthetic_%"PRIi32, count);
  if (UNLIKELY(PARSER_DEBUG))
    csound->Message(csound, "Creating Synthetic label T_IDENT: %s\n", label);
  token = make_token(csound, label);
  token->optype = csound->Strdup(csound, "l");
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
    int len = strlen(root->right->value->lexeme);
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

/* create new assignment statement,
   it creates new unique synthetic ident
   and assigns it to a given var-name,
   assumes inputVarName to be T_IDENT */
TREE* create_assignment_statement(
  CSOUND* csound,
  TYPE_TABLE* typeTable,
  char* inputVarName
) {
  TREE *statement = create_empty_token(csound);
  statement->value = make_token(csound, "=");
  statement->type = T_ASSIGNMENT;
  statement->value->type = T_ASSIGNMENT;
  TREE *assigneeNode = create_synthetic_ident(csound, genlabs++);
  TREE *inputVarNode = create_empty_token(csound);
  inputVarNode->value = make_token(csound, inputVarName);
  inputVarNode->type = T_IDENT;
  inputVarNode->value->type = T_IDENT;
  statement->left = assigneeNode;
  statement->right = inputVarNode;
  return statement;
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
    int is_bool = 0;
    handle_negative_number(csound, currentArg);

    if (currentArg->type == STRUCT_EXPR) {
        TREE* syntheticIdent = create_synthetic_ident(csound, genlabs++);
        TREE* opcodeCallNode = create_opcode_token(csound, "##member_get");
        TREE* subExpr = NULL;

        if (currentArg->left->type == STRUCT_EXPR) {
          subExpr = create_expression(
            csound,
            currentArg->left,
            currentArg,
            currentArg->line,
            currentArg->locn,
            typeTable
          );
          currentArg->left = NULL;
          opcodeCallNode->right = copy_node(
            csound, subExpr->left
          );
          anchor = appendToTree(csound, anchor, subExpr);
        } else if (currentArg->left->type == T_ARRAY) {
          subExpr = create_expression(
            csound,
            currentArg->left,
            currentArg,
            currentArg->line,
            currentArg->locn,
            typeTable
          );
          currentArg->left = NULL;
          TREE* subExprTail = tree_tail(subExpr);
          opcodeCallNode->right = copy_node(
            csound, subExprTail->left
          );
          anchor = appendToTree(csound, anchor, subExpr);
        } else {
          opcodeCallNode->right = currentArg->left;
        }
        if (currentArg->right->value->optype == NULL) {
          currentArg->right->value->optype = csound->Strdup(
            csound, opcodeCallNode->right->value->lexeme
          );
        }
        opcodeCallNode->right->next = currentArg->right;
        opcodeCallNode->left = copy_node(csound, syntheticIdent);
        if (previousArg == NULL) {
          current->right = syntheticIdent;
          syntheticIdent->next = currentArg->next;
        } else {
          previousArg->next = syntheticIdent;
          syntheticIdent->next = currentArg->next;
        }
        currentArg = syntheticIdent;
        anchor = appendToTree(csound, anchor, opcodeCallNode);
    } else if (is_expression_node(currentArg) ||
        (is_bool = is_boolean_expression_node(currentArg))) {

      if (UNLIKELY(PARSER_DEBUG))
        csound->Message(csound, "Found Expression.\n");
      if (is_bool == 0) {
        expressionNodes =
          create_expression(csound, currentArg,
                            previousArg == NULL ? current : previousArg,
                            currentArg->line, currentArg->locn, typeTable);
        // free discarded node
      } else {
        expressionNodes =
          create_boolean_expression(csound, currentArg,
                                    currentArg->line, currentArg->locn,
                                    typeTable);

      }
      if (expressionNodes == NULL) {
        // error
        return NULL;
      }

      nextArg = currentArg->next;
      csound->Free(csound, currentArg);

      /* Set as anchor if necessary */
      anchor = appendToTree(csound, anchor, expressionNodes);

      /* reconnect into chain */
      last = tree_tail(expressionNodes);

      /* handle arg replacement of currentArg here */
      /* **** was a bug as currentArg could be freed above **** */
      if (last->type == LABEL_TOKEN) {
        // second last will do it
        TREE* secondLast = expressionNodes;
        while (secondLast->next != last) {
          secondLast = secondLast->next;
        }
        newArgTree = create_ans_token(csound, secondLast->left->value->lexeme);
      } else {
        newArgTree = create_ans_token(csound, last->left->value->lexeme);
      }

      if (UNLIKELY(PARSER_DEBUG))
        csound->Message(csound, "New Arg: %s\n", newArgTree->value->lexeme);

      if (previousArg == NULL) {
        current->right = newArgTree;
      }
      else {
        previousArg->next = newArgTree;
      }

      newArgTree->next = nextArg;
      currentArg = newArgTree;
    }

    previousArg = currentArg;
    currentArg = currentArg->next;
  }

  anchor = appendToTree(csound, anchor, current);

  // handle LHS expressions (i.e. array-set's)
  previousArg = NULL;
  currentArg = current->left;
  int init = 0;
  if (strcmp("init", current->value->lexeme)==0) {
    //print_tree(csound, "init",current);
    init = 1;
  }

  TREE* nextAnchor = NULL;
  TREE* nextLeft = NULL;
  while (currentArg != NULL) {
    if (currentArg->type == STRUCT_EXPR) {
        TREE* opcodeCallNode = create_opcode_token(csound, "##member_set");
        TREE* currentMember = currentArg->right;
        nextAnchor = appendToTree(csound, nextAnchor, opcodeCallNode);

        if (currentArg->left->type == STRUCT_EXPR) {
          TREE* structSubExpr = create_expression(
            csound,
            currentArg->left,
            currentArg,
            currentArg->line,
            currentArg->locn,
            typeTable
          );
          currentArg->left = NULL;
          opcodeCallNode->right = copy_node(
            csound, structSubExpr->left
          );
          structSubExpr->next = nextAnchor;
          nextAnchor = structSubExpr;
        } else if (currentArg->left->type == T_ARRAY) {
          TREE* arraySubExpr = create_expression(
            csound,
            currentArg->left,
            currentArg,
            currentArg->line,
            currentArg->locn,
            typeTable
          );
          currentArg->left = NULL;
          opcodeCallNode->right = copy_node(
            csound, arraySubExpr->left
          );
          arraySubExpr->next = nextAnchor;
          nextAnchor = arraySubExpr;
        } else if (nextLeft != NULL) {
          opcodeCallNode->right = nextLeft;
        } else {
          opcodeCallNode->right = currentArg->left;
        }

        if (currentMember->value->optype == NULL) {
          currentMember->value->optype = csound->Strdup(
            csound,
            opcodeCallNode->right->value->lexeme
          );
        }

        opcodeCallNode->right->next = currentMember;
        opcodeCallNode->right->next->next = current->right;

        // replacing T_ASSIGN '=' for member_set
        TREE* oldCurrent = current;
        current = opcodeCallNode;
        opcodeCallNode->next = oldCurrent->next;
        oldCurrent->left = NULL;
        oldCurrent->right = NULL;
        oldCurrent->next = NULL;
        if (anchor != NULL) {
          TREE* nextTail = anchor;
          while (nextTail != NULL && nextTail->next != oldCurrent) {
            nextTail = nextTail->next;
          }
          if (nextTail == NULL) {
            anchor = anchor->next;
          } else {
            nextTail->next = NULL;
          }
        }
        csound->Free(csound, oldCurrent);
    } else if (currentArg->type == T_ARRAY) {
      int isStructArray = currentArg->next != NULL &&
        currentArg->next->type == STRUCT_EXPR;
      int isArrayInStruct = currentArg->left->type == STRUCT_EXPR;

      if (isStructArray) {
        TREE* arrayIdent = create_synthetic_ident(csound, genlabs++);
        TREE* arrayGet = create_opcode_token(csound, "##array_get_struct");
        arrayGet->right = currentArg->left;
        arrayGet->right->next = currentArg->right;
        arrayGet->left = copy_node(csound, arrayIdent);

        nextLeft = arrayIdent;
        if (nextAnchor != NULL) {
          arrayGet->next = nextAnchor;
        }

        nextAnchor = arrayGet;

      } else {
        if (isArrayInStruct) {
          TREE* structSubExpr = create_expression(
            csound,
            currentArg->left,
            currentArg,
            currentArg->left->line,
            currentArg->left->locn,
            typeTable
          );
          structSubExpr->next = appendToTree(csound, structSubExpr->next, nextAnchor);
          nextAnchor = structSubExpr;
          currentArg->left = copy_node(csound, structSubExpr->left);
        }


        TREE* arraySet = create_opcode_token(
          csound, init ? "##array_init" : "##array_set"
        );
        arraySet->right = nextAnchor != NULL ?
          copy_node(csound, tree_tail(nextAnchor)->left) : currentArg->left;

        if (current->type == T_OPCALL && current->value->type == T_IDENT) {
          // handling non-assignment opcall
          TREE* opcallIdent = create_synthetic_ident(csound, genlabs++);
          current->left = opcallIdent;
          arraySet->right->next = copy_node(csound, opcallIdent);
          arraySet->right->next->next = currentArg->right;
          current->next = arraySet;
        } else {
          // replacing T_ASSIGN '=/init' for array_set/init
          arraySet->right->next = current->right;
          arraySet->right->next->next = currentArg->right;
          nextAnchor = appendToTree(csound, nextAnchor, arraySet);
          TREE* oldCurrent = current;
          current = arraySet;
          arraySet->next = oldCurrent->next;
          oldCurrent->left = NULL;
          oldCurrent->right = NULL;
          oldCurrent->next = NULL;
          if (anchor != NULL) {
            TREE* nextTail = anchor;
            while (nextTail != NULL && nextTail->next != oldCurrent) {
              nextTail = nextTail->next;
            }
            if (nextTail == NULL) {
              anchor = anchor->next;
            } else {
              nextTail->next = NULL;
            }
          }
          csound->Free(csound, oldCurrent);
        }
      }
    }
    previousArg = currentArg;
    currentArg = currentArg->next;
  }

  if (nextLeft != NULL) {
    current->left = nextLeft;
  }


  if (nextAnchor != NULL) {
    anchor = appendToTree(csound, anchor, nextAnchor);
  }
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

    gotoToken = create_opcode_token(csound, "##if");
    gotoToken->right = copy_node(csound, last->left);
    gotoToken->right->next = right->right;
    last->next = gotoToken;
    gotoToken->next = current->next;
  }
  else if (LIKELY(right->type == THEN_TOKEN ||
                  right->type == ITHEN_TOKEN ||
                  right->type == KTHEN_TOKEN)) {
    int endLabelCounter = -1;
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
        TREE *statements, *label, *boolNode, *labelEnd, *gotoToken;

        statements = tempRight->right;
        label = create_synthetic_label_ident(csound, genlabs);
        labelEnd = create_synthetic_label(csound, genlabs++);
        tempRight->right = label;

        typeTable->labelList =
          cs_cons(csound,
                  cs_strdup(csound,
                            labelEnd->value->lexeme),
                  typeTable->labelList);

        gotoToken = create_opcode_token(csound, "##if");
        boolNode = copy_node(csound, last->left);
        gotoToken->right = boolNode;
        gotoToken->right->next = label;

        gotoToken->next = statements;
        anchor = appendToTree(csound, anchor, gotoToken);

        /* relinking */
        last = tree_tail(last);

        if (endLabelCounter > 0) {
          TREE *endLabel = create_synthetic_label_ident(
            csound,
            endLabelCounter
          );

          TREE *gotoEndLabelToken = create_opcode_token(csound, "##endif");
          // assign the corresponding boolean var-name to optype
          // in order for the verification to resolve correct rate
          gotoEndLabelToken->value->optype = csound->Strdup(
            csound, boolNode->value->lexeme
          );
          gotoEndLabelToken->right = endLabel;
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
                             TYPE_TABLE* typeTable, int dowhile)
{
  TREE* anchor = NULL;
  TREE* expressionNodes = NULL;

  TREE* gotoToken;

  int32 topLabelCounter = genlabs++;
  int32 endLabelCounter = genlabs++;
  TREE* tempRight = current->right;
  TREE* last = NULL;
  TREE* labelEnd;

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

  gotoToken = create_opcode_token(
    csound, dowhile ? "##while" : "##until"
  );
  gotoToken->next = tempRight;
  gotoToken->right = copy_node(csound, last->left);
  gotoToken->right->next = labelEnd;


  anchor = appendToTree(csound, anchor, gotoToken);
  last = tree_tail(anchor);


  labelEnd = create_synthetic_label(csound, endLabelCounter);
  TREE *topLabel = create_synthetic_label_ident(
    csound,
    topLabelCounter
  );
  TREE *gotoTopLabelToken = create_opcode_token(
    csound, dowhile ? "##endwhile" : "##enduntil"
  );
  gotoTopLabelToken->value->optype = csound->Strdup(
    csound, gotoToken->right->value->lexeme
  );
  gotoTopLabelToken->right = copy_node(csound, topLabel);
  anchor = appendToTree(csound, anchor, gotoTopLabelToken);
  // last->next = gotoTopLabelToken;
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

  // create index counter
  TREE *indexAssign = create_empty_token(csound);
  indexAssign->value = make_token(csound, "=");
  indexAssign->type = T_ASSIGNMENT;
  indexAssign->value->type = T_ASSIGNMENT;
  TREE *indexIdent = create_synthetic_ident(csound, genlabs++);
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
  TREE *arrayIdent = create_synthetic_ident(csound, genlabs++);
  TREE *arrayIdentCopy = copy_node(csound, arrayIdent);
  arrayIdent->type = T_ARRAY_IDENT;
  arrayIdent->value->type = T_ARRAY_IDENT;
  arrayAssign->left = arrayIdent;
  arrayAssign->right = current->right->left;
  indexAssign->next = arrayAssign;

  TREE *arrayLength = create_empty_token(csound);
  arrayLength->value = make_token(csound, "=");
  arrayLength->type = T_ASSIGNMENT;
  arrayLength->value->type = T_ASSIGNMENT;

  TREE *arrayLengthIdent = create_synthetic_ident(csound, genlabs++);
  arrayLength->left = arrayLengthIdent;
  TREE *arrayLengthFn = create_empty_token(csound);
  arrayLengthFn->value = make_token(csound, "##for-in-lenarray");
  arrayLengthFn->type = T_FUNCTION;
  arrayLengthFn->value->type = T_FUNCTION;
  arrayLengthFn->value->optype = csound->Strdup(
    csound, indexIdent->value->lexeme
  );
  TREE *arrayLengthArrayIdent = arrayIdentCopy;
  arrayLengthFn->right = arrayLengthArrayIdent;
  arrayLength->right = arrayLengthFn;
  arrayAssign->next = arrayLength;


  TREE* loopLabel = create_synthetic_label(csound, genlabs++);
  loopLabel->type = LABEL_TOKEN;
  loopLabel->value->type = LABEL_TOKEN;
  typeTable->labelList = cs_cons(csound,
                                 cs_strdup(csound, loopLabel->value->lexeme),
                                 typeTable->labelList);

  arrayLength->next = loopLabel;

  // handle case where user provided an index identifier
  int hasOptionalIndex = 0;
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

  TREE* arrayGetStatement = create_opcode_token(
    csound, csound->Strdup(csound, "##array_get")
  );
  arrayGetStatement->left = current->left;
  arrayGetStatement->right = copy_node(csound, arrayIdent);
  arrayGetStatement->right->next = copy_node(csound, indexIdent);
  if (hasOptionalIndex) {
    loopLabel->next->next = arrayGetStatement;
  } else {
    loopLabel->next = arrayGetStatement;
  }
  arrayGetStatement->next = current->right->right;

  TREE* loopLtStatement = create_opcode_token(
    csound, csound->Strdup(csound, "##for-in")
  );
  loopLtStatement->value->optype = csound->Strdup(
    csound, arrayIdent->value->lexeme
  );
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

  return indexAssign;
}

int is_statement_expansion_required(TREE* root) {
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
    if (current->type == T_ARRAY || current->type == STRUCT_EXPR) {
      return 1;
    }
    current = current->next;
  }
  return 0;
}
