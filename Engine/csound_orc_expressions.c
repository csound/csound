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
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "csoundCore.h"
#include "csound_orc.h"
#include "csound_orc_expressions.h"
#include "csound_type_system.h"
#include "csound_orc_semantics.h"
#include "csound_standard_types.h"
#include "csound_orc_compile.h"
#include <inttypes.h>


ORCTOKEN *make_token(CSOUND *, char *);
ORCTOKEN *make_label(CSOUND *, char *);

static TREE *create_boolean_expression(CSOUND*, TREE*, int32_t,  uint64_t,
                                       TYPE_TABLE*);
static TREE *create_expression(CSOUND *, TREE *, int32_t,  uint64_t,
                               TYPE_TABLE*);
static TREE *create_synthetic_label(CSOUND *csound, int32 count);

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

char *remove_type_quoting(CSOUND *csound, const char *outype) {
     char *type, c;
     int32_t n = 0, i = 0;
     type = csound->Calloc(csound, strlen(outype) + 1);
     // remove any : or ; leftover in typename
     do  {
         c = outype[n++];
         if(c == ':' || c == ';') continue;
         type[i++] = c;
      } while (c);
     return type;
}

int32_t find_brace(char *s) {
  while(*s != '\0') {
    if(*s++ == '[') return 1;
  }
  return 0;
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
    add_arg(csound, s, NULL, typeTable, NULL);
  } else {
     // VL 15.10.24
     // at this point new types defined with string type names
     // still have : prepended and ; appended to name
     // we need to remove these for the type system to recognise the type
    char *type = remove_type_quoting(csound, outype);
    if (find_brace(type)) {
      if(*type == '[') { // [type]
        snprintf(s, 16, "#%c%d[]", type[1], argCount);
        add_array_arg(csound, s,  NULL, 1, typeTable);
      } else { // type[]
        snprintf(s, 16, "#%c%d[]", type[0], argCount);
        add_array_arg(csound, s,  type, 1, typeTable);
      }
    }
    else {
      snprintf(s, 256, "#%s%d", type, argCount);
      add_arg(csound, s, type, typeTable, NULL);
    }
    csound->Free(csound, type);
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

static TREE *create_unary_token(CSOUND *csound, char *sym)
{
  TREE *ans;
  ans = (TREE*)csound->Malloc(csound, sizeof(TREE));
  if (UNLIKELY(ans==NULL)) {
   if(csoundGetDebug(csound) & DEBUG_EXPRESSIONS)
    csoundMessage(csound, "Out of memory\n");
    exit(1);
  }
  ans->type = INTEGER_TOKEN;
  ans->left = NULL;
  ans->right = NULL;
  ans->next = NULL;
  ans->len = 0;
  ans->rate = -1;
  ans->markup = NULL;
  ans->value = make_int(csound, sym);
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
      strNcpy(op,csound->inZero? "cingoto":"cngoto", 8);
      break;
    default: printf("Whooops %d\n", type);
    }
  }

  opTree = create_opcode_token(csound, op);
  bVar = create_empty_token(csound);
  bVar->type = T_IDENT;
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

static TREE *create_cond_expression(CSOUND *csound,
                                    TREE *root, int32_t line, uint64_t locn,
                                    TYPE_TABLE* typeTable)
{
  TREE *last = NULL;
  int32 ln1 = csound->genlabs++, ln2 = csound->genlabs++;
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
            csoundStrdup(csound, L1->value->lexeme), typeTable->labelList);
  typeTable->labelList =
    cs_cons(csound,
            csoundStrdup(csound, L2->value->lexeme), typeTable->labelList);
  left = get_arg_type2(csound, c, typeTable);
  right  = get_arg_type2(csound, d, typeTable);
  if (left[0]=='c') left[0] = 'i';
  if (right[0]=='c') right[0] = 'i';
  last = b;
  while (last->next != NULL) {
    last = last->next;
  }

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
      (left[0]=='k' || right[0]=='k' || last->left->value->lexeme[1]=='B') ?2 :   1;
    if (type==2) left[0] = right[0] = 'k';
    eq = "=";
  }

  last->next = create_opcode_token(csound, type==1?"cigoto":"ckgoto");
  xx = create_empty_token(csound);
  xx->type = T_IDENT;
  xx->value = make_token(csound, last->left->value->lexeme);
  xx->value->type = T_IDENT;
  last = last->next;
  last->left = NULL;
  last->right = xx;
  last->right->next = L1;
  last->line = line; root->locn = locn;
  // Need to get type of expression for newvariable
  right = create_out_arg(csound,left,
                         typeTable->localPool->synthArgCount++, typeTable);
  {
    TREE *C = create_opcode_token(csound, csoundStrdup(csound, eq));
    C->left = create_ans_token(csound, right); C->right = c;
    c = C;
  }
  {
    TREE *D = create_opcode_token(csound, csoundStrdup(csound, eq));
    D->left = create_ans_token(csound, right); D->right = d;
    d = D;
  }
  last = b;
  while (last->next != NULL) {
    last = last->next;
  }
  last->next = d;
  while (last->next != NULL) last = last->next;
  //Last is now last assignment
  last->next = create_simple_goto_token(csound, L2, type==2?0:type);
  while (last->next != NULL) last = last->next;
  last->next = create_synthetic_label(csound,ln1);
  while (last->next != NULL) last = last->next;

  last->next = c;
  while (last->next != NULL) last = last->next;
  while (last->next != NULL) last = last->next;
  last->next = create_synthetic_label(csound,ln2);
  while (last->next != NULL) last = last->next;
  last->next = create_opcode_token(csound, csoundStrdup(csound, eq));
  last->next->left = create_ans_token(csound, right);
  last->next->right = create_ans_token(csound, right);
  return b;
}

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
static TREE *create_expression(CSOUND *csound, TREE *root, int32_t line,
                               uint64_t locn,
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

      anchor = append_to_tree(csound, anchor,
                            create_expression(csound, current, line, locn,
                                              typeTable));
      last = tree_tail(anchor);
      newArg = create_ans_token(csound, last->left->value->lexeme);
      newArgList = append_to_tree(csound, newArgList, newArg);
      current = current->next;
    } else {
      TREE* temp;
      newArgList = append_to_tree(csound, newArgList, current);
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

      anchor = append_to_tree(csound, anchor,
                            create_expression(csound, current, line,
                                              locn, typeTable));
      last = tree_tail(anchor);

      newArg = create_ans_token(csound, last->left->value->lexeme);
      newArgList = append_to_tree(csound, newArgList, newArg);
      current = current->next;
    }
    else {
      TREE* temp;
      newArgList = append_to_tree(csound, newArgList, current);
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
      if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
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
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
      csound->Message(csound, "HANDLING UNARY MINUS!");
    root->left = create_unary_token(csound, "-1");
    strNcpy(op, "##mul", 80);
    outarg = create_out_arg_for_expression(csound, op, root->left,
                                           root->right, typeTable);

    break;
   case S_UPLUS:
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
      csound->Message(csound, "HANDLING UNARY PLUS!");
    root->left = create_unary_token(csound, "1");
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

      // Handle struct member access or other complex left expressions
      if (root->left == NULL || root->left->value == NULL ||
          root->left->value->lexeme == NULL) {
        // This could be a struct member access - delegate to get_arg_string_from_tree
        // Note: get_arg_string_from_tree returns the element type for T_ARRAY nodes
        char* elementType = get_arg_string_from_tree(csound, root, typeTable);
        if (elementType == NULL) {
          return NULL;
        }

        // Use the element type directly (it's already the type of array[index])
        outype = elementType;

        // Set the operation to array_get (struct members are always plain arrays)
        strNcpy(op, "##array_get", 80);

        outarg = create_out_arg(csound, outype,
                               typeTable->localPool->synthArgCount++, typeTable);
        csound->Free(csound, elementType);
        break;
      }

      char *varBaseName = root->left->value->lexeme;
      // search for the array variable in all pools
      var = find_var_from_pools(csound, varBaseName,
                                varBaseName, typeTable);
      if (var == NULL) {
        synterr(csound,
                Str("create_expression: unable to find array sub-type "
                    "for var %s line %d\n"),
                varBaseName, current->line);
        return NULL;
      } else {
        // Check if it's an array
        // For typed arrays like k[], i[], S[], the varType is the element type
        // and subType is NULL. For generic arrays, subType contains the element type.
        if (var->subType) {
          // Generic array or struct array
          if (var->subType->userDefinedType) {
            strNcpy(op, "##array_get_struct", 80);
          } else {
            strNcpy(op, "##array_get", 80);
          }
          outype = strdup(var->subType->varTypeName);
	  /* VL: 9.2.22 pulled code from 6.x to check for array index type
             to provide the correct outype. Works with explicit types
	  */
         if (outype[0]== 'i') {
          TREE* inds = root->right;
          while (inds) {
            char *xx = get_arg_string_from_tree(csound, inds, typeTable);
            if (xx[0]=='k') {
              outype[0] = 'k';
              break;
            }
            inds = inds->next;
          }
        }
        } else if (var->dimensions > 0 || var->varType == &CS_VAR_TYPE_ARRAY) {
          // Generic array with dimensions (but no subType set yet)
          strNcpy(op, "##array_get", 80);
          outype = strdup(var->subType->varTypeName);
        } else if (var->varType == &CS_VAR_TYPE_A) {
          strNcpy(op, "##array_get", 80);
          outype = "k";
        } else {
          // Typed array like k[], varType is the element type
          strNcpy(op, "##array_get", 80);
          outype = strdup(var->varType->varTypeName);
        }
      }
      if (outype == NULL) {
        return NULL;
      }

      outarg = create_out_arg(csound, outype,
                              typeTable->localPool->synthArgCount++, typeTable);
    }

    break;
   default:
    /* it should not get here, but if it does, return NULL */
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

  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
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


  if (root->type == T_IDENT) {
    return root;
  }

  if(root->type == TRUE_TOKEN)
    return create_ans_token(csound, "true");

  if(root->type == FALSE_TOKEN)
    return create_ans_token(csound, "false");

  if(root->type == TRUEK_TOKEN)
    return create_ans_token(csound, "truek");

  if(root->type == FALSEK_TOKEN)
    return create_ans_token(csound, "falsek");


  if(root->type == T_FUNCTION) {
    return create_expression(csound, root, line,
                             locn, typeTable);
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

  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS)) {
    if (root->type == S_UNOT)
      csound->Message(csound, "Operator Found: %s (%s)\n", op,
                      get_arg_type2(csound, root->left, typeTable));
    else
      csound->Message(csound, "Operator Found: %s (%s %s)\n", op,
                      get_arg_type2(csound, root->left, typeTable),
                      get_arg_type2(csound, root->right, typeTable));
  }


  if (root->type == S_UNOT)
    outarg = get_boolean_arg(csound,
                             typeTable,
                             *get_arg_type2(csound, root->left, typeTable) =='k' ||
                             *get_arg_type2(csound, root->left, typeTable) =='B');
  else
    outarg = get_boolean_arg(csound,
                             typeTable,
                             *get_arg_type2(csound, root->left, typeTable) =='k' ||
                             *get_arg_type2(csound, root->right, typeTable) == 'k' ||
                             *get_arg_type2(csound, root->left, typeTable) =='B' ||
                             *get_arg_type2(csound, root->right, typeTable) =='B');

  add_arg(csound, outarg, NULL, typeTable, NULL);
  opTree = create_opcode_token(csound, op);
  opTree->right = root->type == T_IDENT ? root : root->left;
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
  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
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
  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
    csound->Message(csound, "Creating Synthetic label: %s\n", label);
  token = make_label(csound, label);
  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
    csound->Message(csound, "**** label lexeme >>%s<<\n", token->lexeme);
  csound->Free(csound, label);
  return make_leaf(csound, -1, 0, LABEL_TOKEN, token);
}

void handle_negative_number(CSOUND* csound, TREE* root)
{
  if (root->type == S_UMINUS &&
      (root->right->type == INTEGER_TOKEN ||
       root->right->type == NUMBER_TOKEN)) {
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
  if ((b->type == '=') &&
      (!strcmp(a->left->value->lexeme, b->right->value->lexeme)) &&
      (!strcmp(tmp1, tmp2))) {
    a->left = b->left;
    a->next = NULL;
    csound->Free(csound, b);
  }
  csound->Free(csound, tmp1);
  csound->Free(csound, tmp2);
}

/* Expand struct array member assignment: array[index].member = value
 * Transforms into:
 *   1. temp = ##array_get_struct(array, index)
 *   2. ##member_set(temp, memberIndex, value)
 *   3. array[index] = temp (standard assignment, becomes ##array_set_struct)
 */
int expand_struct_array_member_assignment(CSOUND* csound,
                                         TREE* current,
                                         TYPE_TABLE* typeTable,
                                         TREE** anchor)
{
  if (!current || !current->left || current->left->type != STRUCT_EXPR) {
    return 0;
  }

  TREE* structExpr = current->left;
  TREE* valueExpr = current->right;

  if (!structExpr->left || structExpr->left->type != T_ARRAY) {
    return 0;
  }

  TREE* arrayExpr = structExpr->left;
  TREE* memberExpr = structExpr->right;

  if (!memberExpr || !memberExpr->value || !memberExpr->value->lexeme) {
    return 0;
  }

  const char* memberName = memberExpr->value->lexeme;

  char* structTypeName = get_arg_type2(csound, arrayExpr->left, typeTable);
  if (!structTypeName) {
    return 0;
  }

  // Handle both ":MyType;[]" format and "[:MyType;]" format
  char* bracketPos = strchr(structTypeName, '[');
  if (bracketPos) *bracketPos = '\0';

  char* cleanTypeName = structTypeName;
  if (cleanTypeName[0] == '\0' && bracketPos) {
    // Format was "[:MyType;]", extract from inside brackets
    cleanTypeName = bracketPos + 1;
    char* endBracket = strchr(cleanTypeName, ']');
    if (endBracket) *endBracket = '\0';
  }

  const CS_TYPE* structType = csoundGetTypeWithVarTypeName(csound->typePool, cleanTypeName);
  if (!structType || !structType->userDefinedType) {
    csound->Free(csound, structTypeName);
    return 0;
  }

  // Find member index
  int memberIndex = -1;
  CONS_CELL* cell = structType->members;
  int i = 0;
  while (cell) {
    CS_VARIABLE* member = (CS_VARIABLE*)cell->value;
    if (member && strcmp(member->varName, memberName) == 0) {
      memberIndex = i;
      break;
    }
    cell = cell->next;
    i++;
  }

  if (memberIndex == -1) {
    csound->Free(csound, structTypeName);
    return 0;
  }

  // Generate unique temp variable name
  char tempVarName[64];
  snprintf(tempVarName, sizeof(tempVarName), "#structArrayTemp%d#", csound->struct_array_temp_counter++);

  // Step 1: temp:Type = array[index]
  ORCTOKEN* tempToken = make_token(csound, tempVarName);
  tempToken->type = T_TYPED_IDENT;
  tempToken->optype = csoundStrdup(csound, cleanTypeName);

  TREE* tempVar = make_leaf(csound, current->line, current->locn, T_TYPED_IDENT, tempToken);

  TREE* getOp = create_opcode_token(csound, "##array_get_struct");
  getOp->type = T_OPCALL;
  getOp->left = tempVar;
  getOp->right = copy_node(csound, arrayExpr->left);  // array variable
  getOp->right->next = copy_node(csound, arrayExpr->right); // index

  // Step 2: ##member_set(temp, memberIndex, value)
  TREE* setMemberOp = create_opcode_token(csound, "##member_set");
  setMemberOp->type = T_OPCALL;
  setMemberOp->right = make_leaf(csound, current->line, current->locn, T_IDENT,
                                 make_token(csound, tempVarName));

  char indexBuf[32];
  snprintf(indexBuf, sizeof(indexBuf), "%d", memberIndex);
  TREE* memberIndexNode = make_leaf(csound, current->line, current->locn,
                                   INTEGER_TOKEN, make_int(csound, indexBuf));
  setMemberOp->right->next = memberIndexNode;
  memberIndexNode->next = copy_node(csound, valueExpr);

  // Step 3: array[index] = temp (will be processed as standard assignment)
  TREE* arraySetAssignment = make_node(csound, current->line, current->locn, T_ASSIGNMENT,
                                      copy_node(csound, arrayExpr),
                                      make_leaf(csound, current->line, current->locn, T_IDENT,
                                               make_token(csound, tempVarName)));

  // T_ASSIGNMENT needs a value token for verify_opcode
  arraySetAssignment->value = make_token(csound, "=");
  arraySetAssignment->value->type = T_ASSIGNMENT;

  // Chain the operations
  getOp->next = setMemberOp;
  setMemberOp->next = arraySetAssignment;

  *anchor = append_to_tree(csound, *anchor, getOp);

  csound->Free(csound, structTypeName);
  return 1;
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

  if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
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
      if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
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

      if (expressionNodes == NULL) {
        csound->Message(csound, "Error: create_expression returned NULL\n");
        return NULL;
      }

      /* Set as anchor if necessary */
      anchor = append_to_tree(csound, anchor, expressionNodes);

      /* reconnect into chain */
      last = tree_tail(expressionNodes);
      newArg = last->left->value->lexeme;

      if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
        csound->Message(csound, "New Arg: %s\n", newArg);

      /* handle arg replacement of currentArg here */
      /* **** was a bug as currentArg could be freed above **** */
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

  anchor = append_to_tree(csound, anchor, current);


  // handle LHS expressions (i.e. array-set's)
  previousArg = NULL;
  currentArg = current->left;
  int32_t init = 0;
  if (strcmp("init", current->value->lexeme)==0) {
    init = 1;
  }
  while (currentArg != NULL) {
    TREE* temp;

    if (currentArg->type == T_ARRAY) {
      char *outType;
      CS_VARIABLE* var;

      char *varBaseName = currentArg->left->value->lexeme;
      // search for the array variable in all pools
      var = find_var_from_pools(csound, varBaseName,
                                varBaseName, typeTable);
      if (var == NULL) {
        synterr(csound,
                Str("expand_statement: unable to find array sub-type "
                    "for var %s line %d\n"),
                varBaseName, current->line);
        return NULL;
      } else {
        // Check if it's an array
        // For LHS array assignment, the temporary variable should have the element type
        // (e.g., "k" for k[], "S" for S[]) because it represents the value being assigned
        if (var->subType) {
          // Generic array, use subType (element type)
          outType = strdup(var->subType->varTypeName);
        } else if (var->dimensions > 0 || var->varType == &CS_VAR_TYPE_ARRAY) {
          // Generic array with dimensions
          outType = strdup(var->subType->varTypeName);
        } else if (var->varType == &CS_VAR_TYPE_A) {
          outType = "k";
        } else {
          // Typed array like k[], varType is the element type
          outType = strdup(var->varType->varTypeName);
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

      // Choose the appropriate array set opcode based on element type
      char* opcodeNameBase;
      if (init) {
        opcodeNameBase = "##array_init";
      } else if (var->subType && var->subType->userDefinedType) {
        opcodeNameBase = "##array_set_struct";
      } else {
        opcodeNameBase = "##array_set";
      }

      TREE* arraySet = create_opcode_token(csound, opcodeNameBase);
      arraySet->right = currentArg->left;
      arraySet->right->next =
        make_leaf(csound, temp->line, temp->locn,
                  T_IDENT, make_token(csound,
                                      temp->value->lexeme));
      arraySet->right->next->next =
        currentArg->right; // TODO - check if this handles expressions

      anchor = append_to_tree(csound, anchor, arraySet);
      currentArg = temp;
    }
    previousArg = currentArg;
    currentArg = currentArg->next;
  }

  handle_optional_args(csound, current);
  collapse_last_assigment(csound, anchor, typeTable);
  append_to_tree(csound, anchor, originalNext);
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
    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
      csound->Message(csound, "Found if-goto\n");
    expressionNodes =
      create_boolean_expression(csound, left, right->line,
                                right->locn, typeTable);


    anchor = append_to_tree(csound, anchor, expressionNodes);

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

    if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
      csound->Message(csound, "Found if-then\n");
    if (right->next != NULL) {
      endLabelCounter = csound->genlabs++;
    }

    while (ifBlockCurrent != NULL) {
      tempLeft = ifBlockCurrent->left;
      tempRight = ifBlockCurrent->right;

      if (ifBlockCurrent->type == ELSE_TOKEN) {
        append_to_tree(csound, anchor, tempRight);
        break;
      }

      expressionNodes =
        create_boolean_expression(csound, tempLeft,
                                  tempLeft->line, tempLeft->locn,
                                  typeTable);

      anchor = append_to_tree(csound, anchor, expressionNodes);

      last = tree_tail(expressionNodes);

      /* reconnect into chain */
      {
        TREE *statements, *label, *labelEnd, *gotoToken;
        int32_t gotoType;

        statements = tempRight->right;
        label = create_synthetic_ident(csound, csound->genlabs);
        labelEnd = create_synthetic_label(csound, csound->genlabs++);
        tempRight->right = label;

        typeTable->labelList =
          cs_cons(csound,
                  csoundStrdup(csound,
                            labelEnd->value->lexeme),
                  typeTable->labelList);
        // checking for #B... var name
        if (last->type == T_IDENT) {
          gotoType = (last->value->lexeme[1] == 'B');
          gotoToken = create_goto_token(csound,
            last->value->lexeme,
            tempRight,
            gotoType
          );
        } else {
          gotoType = (last->left->value->lexeme[1] == 'B');
          gotoToken = create_goto_token(csound,
            last->left->value->lexeme,
            tempRight,
            gotoType
          );
        }
        gotoToken->next = statements;
        anchor = append_to_tree(csound, anchor, gotoToken);

        /* relinking */
        last = tree_tail(last);

        if (endLabelCounter > 0) {
          TREE *endLabel = create_synthetic_ident(csound,
                                                  endLabelCounter);
          int32_t type = (gotoType == 1) ? 0 : 2;
          TREE *gotoEndLabelToken =
            create_simple_goto_token(csound, endLabel, type);
          if (UNLIKELY(csoundGetDebug(csound) & DEBUG_EXPRESSIONS))
            csound->Message(csound, "Creating simple goto token\n");

          append_to_tree(csound, last, gotoEndLabelToken);

          gotoEndLabelToken->next = labelEnd;
        }
        else {
          append_to_tree(csound, last, labelEnd);
        }

        ifBlockCurrent = tempRight->next;
      }
    }

    if (endLabelCounter > 0) {
      TREE *endLabel = create_synthetic_label(csound,
                                              endLabelCounter);
      anchor = append_to_tree(csound, anchor, endLabel);

      typeTable->labelList = cs_cons(csound,
                                     csoundStrdup(csound,
                                               endLabel->value->lexeme),
                                     typeTable->labelList);
    }
    anchor = append_to_tree(csound, anchor, current->next);
  }
  else {
    csound->Message(csound,
                    Str("ERROR: Neither if-goto or if-then found on line %d!!!"),
                    right->line);
  }

  return anchor->type == T_IDENT ? anchor->next : anchor;
}

TREE* create_equality_statement(
  CSOUND* csound,
  TREE* left,
  TREE* right
) {
  TREE *equalityNode = create_empty_token(csound);
  equalityNode->value = make_token(csound, "==");
  equalityNode->type = S_EQ;
  equalityNode->value->type = S_EQ;
  equalityNode->left = left;
  equalityNode->right = right;
  return equalityNode;
}

static TREE* create_goto_node(
  CSOUND* csound,
  int isPerfRate
) {
  TREE* gotoOperator = create_opcode_token(csound, isPerfRate ? "kgoto" : "igoto");
  gotoOperator->type = isPerfRate ? KGOTO_TOKEN : IGOTO_TOKEN;
  gotoOperator->value->type = isPerfRate ? KGOTO_TOKEN : IGOTO_TOKEN;
  return gotoOperator;
}

static TREE* create_cgoto_node(
  CSOUND* csound,
  int isPerfRate
) {
  TREE* cgotoOperator = create_opcode_token(csound, isPerfRate ? "ckgoto" : "cigoto");
  cgotoOperator->type = T_OPCALL;
  cgotoOperator->value->type = T_OPCALL;
  return cgotoOperator;
}


TREE* expand_switch_statement(
  CSOUND* csound,
  TREE* current,
  TYPE_TABLE* typeTable,
  char* switchArgType
) {
  int isPerfRate = switchArgType[0] == 'k';
  // TODO: assign to synthetic variable
  TREE* switchExpression = current->left;

  TREE* endGoto = create_goto_node(csound, isPerfRate);
  TREE* endLabel = create_synthetic_label(csound, csound->genlabs++);
  typeTable->labelList = cs_cons(
    csound,
    csoundStrdup(csound, endLabel->value->lexeme),
    typeTable->labelList
  );
  endGoto->right = endLabel;

  TREE* tempNext = NULL;
  TREE* defaultCaseLabel;
  TREE* defaultCaseBody;
  TREE* gotoChainHead = NULL;
  TREE* gotoChainHeadAnchor = NULL;
  TREE* gotoChainHeadDefaultCase = NULL;
  TREE* gotoChainTail = NULL;
  TREE* gotoChainTailAnchor;

  TREE* caseNode = current->right;
  TREE* caseLabel;

  tempNext = caseNode->right != NULL ? caseNode->right->next : NULL;

  while (caseNode) {
    if (caseNode->type == CASE_TOKEN) {
        caseLabel = create_synthetic_label(csound, csound->genlabs++);
        typeTable->labelList = cs_cons(
          csound,
          csoundStrdup(csound, caseLabel->value->lexeme),
          typeTable->labelList
        );

        gotoChainTailAnchor = copy_node(csound, caseLabel);


        if (gotoChainTail == NULL) {
          gotoChainTail = gotoChainTailAnchor;
        } else {
          append_to_tree(csound, gotoChainTail, gotoChainTailAnchor);
        }

        TREE* caseArg = caseNode->left;
        while (caseArg != NULL) {
          if (gotoChainHeadAnchor == NULL) {
            gotoChainHeadAnchor = create_cgoto_node(csound, isPerfRate);
            if (gotoChainHead == NULL) {
              gotoChainHead = gotoChainHeadAnchor;
            }
          } else {
            gotoChainHeadAnchor = create_cgoto_node(csound, isPerfRate);
            append_to_tree(csound, gotoChainHead, gotoChainHeadAnchor);
          }

          gotoChainHeadAnchor->right = create_equality_statement(
            csound,
            copy_node(csound, switchExpression),
            copy_node_shallow(csound, caseArg)
          );

          gotoChainHeadAnchor->right->next = copy_node(csound, caseLabel);
          caseArg = caseArg->next;
        }

        // Check if we have a real statement list (not NULL and not empty node)
        // Empty nodes have type == 0 with NULL left and right
        if (caseNode->right != NULL &&
            !(caseNode->right->type == 0 &&
              caseNode->right->left == NULL &&
              caseNode->right->right == NULL)) {
          tempNext = caseNode->right->next;
          gotoChainTailAnchor = append_to_tree(csound, gotoChainTailAnchor, caseNode->right);
          gotoChainTailAnchor->next->next = NULL;
          gotoChainTailAnchor = append_to_tree(
            csound,
            gotoChainTailAnchor,
            copy_node(csound, endGoto)
          );
        } else {
          // Empty case - add a goto to end to prevent fall-through
          gotoChainTailAnchor = append_to_tree(
            csound,
            gotoChainTailAnchor,
            copy_node(csound, endGoto)
          );
          // Still need to get the next node
          if (caseNode->right != NULL) {
            tempNext = caseNode->right->next;
          } else {
            tempNext = NULL;
          }
        }
    } else if (caseNode->type == DEFAULT_TOKEN && gotoChainHeadDefaultCase == NULL) {
      gotoChainHeadDefaultCase = create_goto_node(csound, isPerfRate);
      defaultCaseLabel = create_synthetic_label(csound, csound->genlabs++);
      typeTable->labelList = cs_cons(
        csound,
        csoundStrdup(csound, defaultCaseLabel->value->lexeme),
        typeTable->labelList
      );
      gotoChainHeadDefaultCase->right = defaultCaseLabel;
      defaultCaseBody = caseNode->right;
      defaultCaseBody->next = copy_node(csound, endGoto);
    } else {
      if (caseNode->right != NULL) {
        tempNext = caseNode->right->next;
      } else {
        tempNext = NULL;
      }
    }

    caseNode = tempNext;
  }

  if (gotoChainHeadDefaultCase != NULL) {
    gotoChainHeadAnchor = append_to_tree(
      csound,
      gotoChainHeadAnchor,
      gotoChainHeadDefaultCase
    );
    gotoChainTailAnchor = append_to_tree(
      csound,
      gotoChainTailAnchor,
      copy_node(csound, defaultCaseLabel)
    );
    gotoChainTailAnchor = append_to_tree(
      csound,
      gotoChainTailAnchor,
      defaultCaseBody
    );
  }

  if (gotoChainHeadDefaultCase == NULL) {
    gotoChainHeadAnchor = append_to_tree(
      csound,
      gotoChainHeadAnchor,
      endGoto
    );
  }

  gotoChainHeadAnchor = append_to_tree(
    csound,
    gotoChainHeadAnchor,
    gotoChainTail
  );
  TREE* endLabelNode = copy_node(csound, endLabel);
  append_to_tree(
    csound,
    gotoChainHeadAnchor,
    endLabelNode
  );

  // Link code after endsw
  endLabelNode->next = current->next;

  return gotoChainHead;
}

/* 1. create top label to loop back to
   2. do boolean expression
   3. do goto token that checks boolean and goes to end label
   4. insert statements
   5. add goto token that goes to top label
   6. end label */
TREE* expand_until_statement(CSOUND* csound, TREE* current,
                             TYPE_TABLE* typeTable, int32_t dowhile,
                             LOOP_JUMP_TARGETS* targets)
{
  TREE* anchor = NULL;
  TREE* expressionNodes = NULL;

  TREE* gotoToken;

  int32 topLabelCounter = csound->genlabs++;
  int32 endLabelCounter = csound->genlabs++;
  TREE* tempRight = current->right;
  TREE* last = NULL;
  TREE* labelEnd;
  int32_t gotoType;

  anchor = create_synthetic_label(csound, topLabelCounter);
  typeTable->labelList = cs_cons(csound,
                                 csoundStrdup(csound, anchor->value->lexeme),
                                 typeTable->labelList);

  if (current->left->type == T_IDENT) {
    last = tree_tail(anchor);
  } else {
    expressionNodes = create_boolean_expression(
      csound,
      current->left,
      current->line,
      current->locn,
      typeTable
    );
    anchor = append_to_tree(csound, anchor, expressionNodes);
    last = tree_tail(anchor);
  }

  // checking for #B... var name
  if (current->left->type == T_IDENT) {
    gotoType = current->left->value->lexeme[1] == 'B';
  } else {
    gotoType = last->left->value->lexeme[1] == 'B';
  }

  labelEnd = create_synthetic_label(csound, endLabelCounter);
  typeTable->labelList = cs_cons(csound,
                                 csoundStrdup(csound, labelEnd->value->lexeme),
                                 typeTable->labelList);
  gotoToken =
    create_goto_token(csound,
                      current->left->type == T_IDENT ?
                        current->left->value->lexeme :
                        last->left->value->lexeme,
                      labelEnd,
                      gotoType+0x8000*dowhile);
  gotoToken->next = tempRight;
  gotoToken->right->next = labelEnd;


  last = append_to_tree(csound, last, gotoToken);
  last = tree_tail(last);


  labelEnd = create_synthetic_label(csound, endLabelCounter);
  TREE *labelEndIdent = create_synthetic_ident(csound,
                                               endLabelCounter);
  TREE *topLabel = create_synthetic_ident(csound,
                                          topLabelCounter);
  TREE *gotoTopLabelToken = create_simple_goto_token(csound,
                                                     topLabel,
                                                     (gotoType==1 ? 0 : 1));

  append_to_tree(csound, last, gotoTopLabelToken);
  gotoTopLabelToken->next = labelEnd;


  labelEnd->next = current->next;
  targets->continueTargetIdent = topLabel;
  targets->breakTargetIdent = labelEndIdent;
  targets->breakTargetLabel = labelEnd;
  targets->gotoType = (gotoType==1 ? 0 : 1);
  return anchor;
}

TREE* expand_for_statement(CSOUND* csound, TREE* current, TYPE_TABLE* typeTable,
                           char* arrayArgType, LOOP_JUMP_TARGETS* targets) {

  const CS_TYPE *iType = &CS_VAR_TYPE_I;
  const CS_TYPE *kType = &CS_VAR_TYPE_K;
  const CS_TYPE *aType = &CS_VAR_TYPE_A;
  const CS_TYPE *sType = &CS_VAR_TYPE_S;
  const CS_TYPE *xType = &CS_VAR_TYPE_COMPLEX;
  const CS_TYPE *arrayType =
    csoundGetTypeWithVarTypeName(csound->typePool, arrayArgType);
  int32_t isPerfRate = 0;

  // these array types generated perf-time loops
  if(arrayType == aType || arrayType == kType ||
     arrayType == xType) isPerfRate = 1;
  else isPerfRate = 0;

  char* op = (char *)csound->Malloc(csound, 10);
  // create index counter
  TREE *indexAssign = create_empty_token(csound);
  indexAssign->value = make_token(csound, "=");
  indexAssign->type = T_ASSIGNMENT;
  indexAssign->value->type = T_ASSIGNMENT;
  char *indexName = create_synthetic_var_name(csound,csound->genlabs++,
                                              isPerfRate ? 'k' : 'i');
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

  // this array holds the data for each iteration
  // the array type generally matches the loop var type
  // with the exception of 'i' and 'k' which may be used interchangeably
  char *arrayName = create_synthetic_array_var_name(csound,csound->genlabs++,'x');
  TREE *arrayIdent = create_empty_token(csound);
  arrayIdent->value = make_token(csound, arrayName);
  arrayIdent->type = T_ARRAY_IDENT;
  arrayIdent->value->type = T_ARRAY_IDENT;
  add_array_arg(csound, arrayName, arrayArgType, 1, typeTable);

  arrayAssign->left = arrayIdent;
  arrayAssign->right = current->right->left;
  indexAssign->next = arrayAssign;

  TREE *arrayLength = create_empty_token(csound);
  arrayLength->value = make_token(csound, "=");
  arrayLength->type = T_ASSIGNMENT;
  arrayLength->value->type = T_ASSIGNMENT;
  char *arrayLengthName = create_synthetic_var_name(csound,csound->genlabs++,
                                                    isPerfRate ? 'k' : 'i');
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

  TREE* loopLabel = create_synthetic_label(csound, csound->genlabs++);
  loopLabel->type = LABEL_TOKEN;
  loopLabel->value->type = LABEL_TOKEN;
  CS_VARIABLE *loopLabelVar =
    csoundCreateVariable(csound, csound->typePool, isPerfRate ? kType : iType,
                         loopLabel->value->lexeme, NULL);
  csoundAddVariable(csound, typeTable->localPool, loopLabelVar);
  typeTable->labelList =
    cs_cons(csound, csoundStrdup(csound, loopLabel->value->lexeme),
                                 typeTable->labelList);
  arrayLength->next = loopLabel;

  // handle case where user provided an index identifier
  int32_t hasOptionalIndex = 0;
  if (current->left->next != NULL) {
    CS_VARIABLE* var = find_var_from_pools(csound, current->left->next->value->lexeme,
                                        current->left->next->value->lexeme, typeTable);
    // variable will replace any existing variable
    if(var != NULL)
    csound->Warning(csound, "redefining variable %s in loop (type: %s)\n"
		            "\t - now using %s type, line %d",
		              var->varName,  var->varType->varTypeName,
		              isPerfRate ? "k" : "i", current->line);
    add_arg(csound, current->left->next->value->lexeme, isPerfRate ? "k" : "i", typeTable, NULL);
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

  char* array_get = arrayType != sType ? "##array_get" : "##array_geti";
  TREE* arrayGetStatement = create_opcode_token(csound, array_get);
  arrayGetStatement->left = current->left;

  arrayGetStatement->right = copy_node(csound, arrayIdent);
  arrayGetStatement->right->next = copy_node(csound, indexIdent);
  if (hasOptionalIndex) {
    loopLabel->next->next = arrayGetStatement;
  } else {
    loopLabel->next = arrayGetStatement;
  }
  arrayGetStatement->next = current->right->right;

  int32_t continueTargetCounter = csound->genlabs++;
  TREE* continueTargetLabel = create_synthetic_label(csound, continueTargetCounter);
  typeTable->labelList = cs_cons(csound,
                                 csoundStrdup(csound, continueTargetLabel->value->lexeme),
                                 typeTable->labelList);
  TREE* continueTargetIdent = create_synthetic_ident(csound, continueTargetCounter);

  int32_t breakTargetCounter = csound->genlabs++;
  TREE* breakTargetLabel = create_synthetic_label(csound, breakTargetCounter);
  typeTable->labelList = cs_cons(csound,
                                 csoundStrdup(csound, breakTargetLabel->value->lexeme),
                                 typeTable->labelList);
  TREE* breakTargetIdent = create_synthetic_ident(csound, breakTargetCounter);

  TREE* tail = tree_tail(current->right->right);
  tail->next = continueTargetLabel;

  strNcpy(op, isPerfRate ? "looplt.k" : "looplt.i", 10);
  TREE* loopLtStatement = create_opcode_token(csound, op);
  continueTargetLabel->next = loopLtStatement;

  TREE* indexArgToken = copy_node(csound, indexIdent);
  loopLtStatement->right = indexArgToken;
  // VL: need to set the next statement after loop
  loopLtStatement->next = breakTargetLabel;
  breakTargetLabel->next = current->next;

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

  targets->continueTargetIdent = continueTargetIdent;
  targets->breakTargetIdent = breakTargetIdent;
  targets->breakTargetLabel = breakTargetLabel;
  targets->gotoType = (isPerfRate == 1 ? 0 : 1);

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

TREE* convert_break_to_goto(CSOUND* csound, LOOP_JUMP_TARGETS* targets) {
  return create_simple_goto_token(csound, copy_node(csound, targets->breakTargetIdent), targets->gotoType);
}

TREE* convert_continue_to_goto(CSOUND* csound, LOOP_JUMP_TARGETS* targets) {
  return create_simple_goto_token(csound, copy_node(csound, targets->continueTargetIdent), targets->gotoType);
}
