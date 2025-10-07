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
#include "csound_orc_compile.h"
#include "csound_orc_structs.h"
#include <inttypes.h>



// Return primary rate class from identifier token name after skipping '#' and 'g' prefixes
static char rate_hint_from_ident(const TREE* t) {
  if (!t || t->type != T_IDENT || !t->value || !t->value->lexeme) return 0;
  const char* s = t->value->lexeme;
  if (*s == '#') s++;
  if (*s == 'g') s++;
  return *s;
}

ORCTOKEN *make_token(CSOUND *, char *);
ORCTOKEN *make_label(CSOUND *, char *);
char* create_array_arg_type(CSOUND* csound, CS_VARIABLE* arrayVar);

static TREE *create_boolean_expression(CSOUND*, TREE*, int32_t,  uint64_t,
                                       TYPE_TABLE*);
static TREE* flatten_struct_array_chain(CSOUND* csound, TREE* root, int line, int locn,
                                        TYPE_TABLE* typeTable, TREE** anchor);
static TREE *create_expression(CSOUND *, TREE *, int32_t,  uint64_t,
                               TYPE_TABLE*);
static TREE *create_synthetic_label(CSOUND *csound, int32 count);


static TREE* lower_expr_to_temp(CSOUND* csound, TREE* expr, int32_t line, uint64_t locn, TYPE_TABLE* typeTable, const char** outTempLexeme);

static TREE* lower_expr_to_temp(
  CSOUND* csound,
  TREE* expr,
  int32_t line,
  uint64_t locn,
  TYPE_TABLE* typeTable,
  const char** outTempLexeme
) {
  if (outTempLexeme) *outTempLexeme = NULL;
  if (!expr) return NULL;
  TREE* chain = create_expression(csound, expr, line, locn, typeTable);
  if (!chain) return NULL;
  // Prefer the opcall for this function (by name) to get its real out temp
  const char* outLex = NULL;
  const char* fnName = (expr && expr->value) ? expr->value->lexeme : NULL;
  for (TREE* t = chain; t != NULL; t = t->next) {
    if (t->type == T_OPCALL && t->value && t->value->lexeme && fnName && strcmp(t->value->lexeme, fnName) == 0) {
      if (t->left && t->left->value && t->left->value->lexeme) {
        outLex = t->left->value->lexeme;
      }
    }
  }
  // Fallback: use tail with a concrete left
  if (!outLex) {
    TREE* tail = chain; while (tail && tail->next) tail = tail->next;
    if (tail && tail->left && tail->left->value && tail->left->value->lexeme) {
      outLex = tail->left->value->lexeme;
    }
  }
  if (outTempLexeme) *outTempLexeme = outLex;
  return chain;
}


// Simple structural equivalence for binary ops with identifier leaves
static int trees_equiv_simple(TREE* a, TREE* b) {
  if (!a || !b) return 0;
  if ((a->type == '*' || a->type == '+') && (b->type == '*' || b->type == '+')) {
    if (a->type != b->type) return 0;
    if (!a->left || !a->right || !b->left || !b->right) return 0;
    if (a->left->type != T_IDENT || a->right->type != T_IDENT ||
        b->left->type != T_IDENT || b->right->type != T_IDENT) return 0;
    const char* aL = (a->left->value && a->left->value->lexeme) ? a->left->value->lexeme : "";
    const char* aR = (a->right->value && a->right->value->lexeme) ? a->right->value->lexeme : "";
    const char* bL = (b->left->value && b->left->value->lexeme) ? b->left->value->lexeme : "";
    const char* bR = (b->right->value && b->right->value->lexeme) ? b->right->value->lexeme : "";
    if ((!strcmp(aL, bL) && !strcmp(aR, bR)) || (!strcmp(aL, bR) && !strcmp(aR, bL))) return 1;
  }
  return 0;
}

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
      snprintf(s, 16, "#%c%d[]", type[1], argCount);
      if(*type == '[') // [type]
      add_array_arg(csound, s,  NULL, 1, typeTable);
      else // type[]
      add_array_arg(csound, s,  type, 1, typeTable);
    }
    else {
      snprintf(s, 256, "#%s%d", type, argCount);
      add_arg(csound, s, type, typeTable, NULL);
    }
    csound->Free(csound, type);
  }
  return s;
}

#if 0
char *create_out_arg(CSOUND *csound, char* outype, int32_t argCount,
                     TYPE_TABLE* typeTable)
{


  // Always allocate a sufficiently large buffer for user-defined names
  char* s = (char *)csound->Calloc(csound, 256);

  size_t L = strlen(outype);

  // Single-letter scalar types (fast path)
  if (L == 1 && outype[0] != '[') {
    switch(*outype) {
    case 'a': snprintf(s, 256, "#a%d", argCount); break;
    case 'K':
    case 'k': snprintf(s, 256, "#k%d", argCount); break;
    case 'B': snprintf(s, 256, "#B%d", argCount); break;
    case 'b': snprintf(s, 256, "#b%d", argCount); break;
    case 'f': snprintf(s, 256, "#f%d", argCount); break;
    case 't': snprintf(s, 256, "#k%d", argCount); break;
    case 'S': snprintf(s, 256, "#S%d", argCount); break;
    default:  snprintf(s, 256, "#i%d", argCount); break;
    }
    add_arg(csound, s, NULL, typeTable, NULL);
    return s;
  }

  // Normalize new-type spellings by stripping optional : ; quoting
  char *type = remove_type_quoting(csound, outype);

  // Detect array forms in either internal ("[[k") or external ("Type[]")
  int32_t dims = 0;
  char *baseName = NULL;

  if (type[0] == '[') {
    // Internal representation: [[k (dims = leading '[' count), base = next char)
    const char *p = type;
    while (*p == '[') { dims++; p++; }
    // base letter (k,i,a,S, etc.)
    char baseLetter[2] = { *p ? *p : 'i', '\0' };
    // Name result token as #[letter]N for variable storage; add_array_arg will handle the array type
    snprintf(s, 256, "#%s%d", baseLetter, argCount);
    add_array_arg(csound, s, baseLetter, dims, typeTable);
  } else if (find_brace(type)) {
    // External representation: e.g., Person[][], k[], Complex[]
    size_t n = strlen(type);
    // Count trailing [] pairs
    const char *p = type + n;
    while (p - type >= 2 && *(p-2) == '[' && *(p-1) == ']') { dims++; p -= 2; }
    size_t nameLen = (size_t)(p - type);
    baseName = (char*)csound->Calloc(csound, nameLen + 1);
    memcpy(baseName, type, nameLen);

    // Compose a descriptive temp var name like #Person3 for variable storage; add_array_arg will handle the array type
    snprintf(s, 256, "#%s%d", baseName, argCount);
    add_array_arg(csound, s, baseName, dims > 0 ? dims : 1, typeTable);
  } else {
    // Non-array or multi-char types
    // If 'type' is a pure sequence of primitive single-letter types (e.g., "aa", "ak"),
    // treat it as a multi-output. Create one temp per letter and return the first.
    int isMultiPrim = 0;
    if (type && type[0]) {
      size_t nmp = strlen(type);
      if (nmp > 1) {
        isMultiPrim = 1;
        for (size_t i = 0; i < nmp; ++i) {
          char c = type[i];
          if (!(c=='i'||c=='k'||c=='K'||c=='a'||c=='S'||c=='s'||c=='B'||c=='b'||c=='f'||c=='t')) {
            isMultiPrim = 0; break;
          }
        }
      }
    }
    if (isMultiPrim) {
      size_t nout = strlen(type);
      for (size_t i = 0; i < nout; ++i) {
        char c = type[i];
        char nameBuf[64];
        snprintf(nameBuf, sizeof nameBuf, "#%c%d", c, (int)(argCount + i));
        add_arg(csound, nameBuf, NULL, typeTable, NULL);
      }
      if (nout > 1) { typeTable->localPool->synthArgCount += (int)(nout - 1); }
      snprintf(s, 256, "#%c%d", type[0], argCount);
    } else {
      // Non-array user-defined or multi-char built-ins (e.g., Complex)
      snprintf(s, 256, "#%s%d", type, argCount);
      add_arg(csound, s, type, typeTable, NULL);
    }
  }

  if (baseName) csound->Free(csound, baseName);
  csound->Free(csound, type);

  return s;
}
#endif

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
TREE * create_opcode_token(CSOUND *csound, const char* op)
{
  TREE *ans = create_empty_token(csound);
  ans->type = T_OPCALL;
  ans->value = make_token(csound, (char*)op);
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
  case STRUCT_EXPR:
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
  case S_EQT:
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
            cs_strdup(csound, L1->value->lexeme), typeTable->labelList);
  typeTable->labelList =
    cs_cons(csound,
            cs_strdup(csound, L2->value->lexeme), typeTable->labelList);
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
    TREE *C = create_opcode_token(csound, cs_strdup(csound, eq));
    C->left = create_ans_token(csound, right); C->right = c;
    c = C;
  }
  {
    TREE *D = create_opcode_token(csound, cs_strdup(csound, eq));
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
  last->next = create_opcode_token(csound, cs_strdup(csound, eq));
  last->next->left = create_ans_token(csound, right);
  last->next->right = create_ans_token(csound, right);
  return b;
}

static char* create_out_arg_for_expression(CSOUND* csound, char* op, TREE* left,
                                           TREE* right, TYPE_TABLE* typeTable) {
  char* outType;

  OENTRIES* opentries = find_opcode2(csound, op);

  if (opentries == NULL || opentries->count == 0) {
    csound->Message(csound, "ERROR: Opcode '%s' not found\n", op);
    return NULL;
  }


  char* leftArgType = get_arg_string_from_tree(csound, left, typeTable);
  char* rightArgType = get_arg_string_from_tree(csound, right, typeTable);
  char* argString = csound->Calloc(csound, 80);

  strNcpy(argString, leftArgType, 80);
  strlcat(argString, rightArgType, 80);

  if (csound->GetDebug(csound)) {
    csound->Message(csound, "DEBUG(expr outarg): op=%s left=%s right=%s args=%s\n",
                    op ? op : "(null)",
                    leftArgType ? leftArgType : "(null)",
                    rightArgType ? rightArgType : "(null)",
                    argString ? argString : "(null)");
  }

  outType = resolve_opcode_get_outarg(csound, opentries, argString);

  csound->Free(csound, argString);
  csound->Free(csound, leftArgType);
  csound->Free(csound, rightArgType);
  csound->Free(csound, opentries);

  if (outType == NULL) return NULL;

  outType = convert_external_to_internal(csound, outType);
  if (csound->GetDebug(csound)) {
    csound->Message(csound, "DEBUG: create_out_arg_for_expression calling create_out_arg with outType='%s'\n",
                    outType ? outType : "(null)");
  }
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
  int skipOpCreation = 0;
  // Track multi-output primitive info for T_FUNCTION (e.g., outtype "aa")
  int multiPrimCount = 0;
  int multiPrimBaseIndex = -1;
  char multiPrimTypes[16] = {0};

  // Debug: Print the AST structure for all expressions to see what struct member access looks like
  if (root && root->value && root->value->lexeme) {
    if (strstr(root->value->lexeme, "john") || strstr(root->value->lexeme, "relativeList") || strstr(root->value->lexeme, "relativeCount")) {
      printf("DEBUG: create_expression called with node type=%d lexeme='%s'\n", root->type, root->value->lexeme);
      print_tree(csound, "DEBUG: Struct-related expression AST:\n", root);
    }
  }

  /* HANDLE SUB EXPRESSIONS */
  if (root->type=='?') return create_cond_expression(csound, root, line,
                                                     locn, typeTable);
  memset(op, 0, 80);
  current = root->left;
  newArgList = NULL;
  while (current != NULL) {
    if (current->type == T_OPCALL || is_expression_node(current)) {
      // Special case 1: do NOT pre-evaluate a STRUCT_EXPR that is the base of
      // a T_ARRAY. We want the T_ARRAY case below to see the original
      // STRUCT_EXPR so it can emit a dedicated member-array getter instead of
      // lowering it to an ANS token here.
      if (root->type == T_ARRAY && current == root->left && current->type == STRUCT_EXPR) {
        newArgList = append_to_tree(csound, newArgList, current);
        // Advance and continue without transforming this child
        current = current->next;
        continue;
      }
      // Special case 2: when building a STRUCT_EXPR, if its left child is a T_ARRAY
      // (e.g., john.relativeList[0].field), do NOT pre-evaluate that T_ARRAY here.
      // We want the STRUCT_EXPR case to see the intact T_ARRAY to emit the chain:
      // member_get(array) -> array_get_struct -> member_get(scalar).
      if (root->type == STRUCT_EXPR && current == root->left && current->type == T_ARRAY) {
        newArgList = append_to_tree(csound, newArgList, current);
        current = current->next;
        continue;
      }
      TREE* newArg;
      TREE* expr = create_expression(csound, current, line, locn, typeTable);
      anchor = append_to_tree(csound, anchor, expr);
      last = tree_tail(expr);
      if (last == NULL) {
        return NULL;
      }
      /* Prefer the last node in the expression chain that has a concrete LHS temp.
         This is important for ternary lowering which ends with a label; the actual
         result temp is on the last assignment before the label. */
      TREE* pickWithLeft = NULL;
      for (TREE* tscan = expr; tscan != NULL; tscan = tscan->next) {
        if (tscan->left && tscan->left->value && tscan->left->value->lexeme) {
          pickWithLeft = tscan;
        }
      }
      const char* pickedLex = NULL;
      if (pickWithLeft) {
        pickedLex = pickWithLeft->left->value->lexeme;
      } else if (last->value && last->value->lexeme) {
        pickedLex = last->value->lexeme; /* fallback: direct value on tail node */
      }
      if (!pickedLex) {
        return NULL;
      }
      newArg = create_ans_token(csound, (char*)pickedLex);
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
  // Normalize arglist anomalies: merge a leading unary '+' on a binary term
  // with a following '+' that supplies the missing left operand.
  // Pattern: [S_UPLUS(* x y)], [+(L, * x y)]  =>  [+(L, * x y)]
  if (root && root->type == T_OPCALL && root->right) {
    TREE* cur = root->right;
    while (cur && cur->next) {
      TREE* n = cur->next;

      if (cur->type == S_UPLUS && cur->right && n->type == '+' && n->left && n->right && trees_equiv_simple(cur->right, n->right)) {
        // transform cur into '+' with left operand taken from next
        cur->type = '+';
        if (!cur->value) cur->value = make_token(csound, "+");
        else cur->value->lexeme = cs_strdup(csound, "+");
        // make a separate copy of the missing left operand
        cur->left = copy_node(csound, n->left);
        // keep the next arg (n) intact so both args become identical sums
        // continue without advancing to avoid skipping
        continue;
      }
      cur = cur->next;
    }
  }

  newArgList = NULL;
  while (current != NULL) {
    if (current->type == T_OPCALL || is_expression_node(current)) {
      TREE* newArg;



      if (csound->GetDebug(csound) && current->type == T_FUNCTION &&
          current->value && strcmp(current->value->lexeme, "fillarray") == 0) {
        csound->Message(csound, "DEBUG: Processing fillarray T_FUNCTION in argument list\n");
      }
      TREE* expr = create_expression(csound, current, line, locn, typeTable);
      if (csound->GetDebug(csound) && current->type == T_FUNCTION &&
          current->value && strcmp(current->value->lexeme, "fillarray") == 0) {
        csound->Message(csound, "DEBUG: create_expression returned expr=%p for fillarray\n", (void*)expr);
      }
      anchor = append_to_tree(csound, anchor, expr);
      last = tree_tail(expr);
      if (last == NULL) {
        return NULL;
      }
      /* Special case: full-array initialization like `SArr[] = [ ... ]`
         If LHS is an array identifier (no index) and RHS expression tail is `fillarray`,
         retarget the fillarray output temp to the actual LHS variable so it initializes
         the declared array directly, and skip the enclosing assignment later. */
      if (current && current->type == T_ASSIGNMENT && current->left &&
          current->left->type == T_ARRAY_IDENT && last->value && last->value->lexeme &&
          strncmp(last->value->lexeme, "fillarray", 9) == 0) {
        if (csound->GetDebug(csound))
          csound->Message(csound, "[orc] retarget fillarray output to LHS array '%s' (was %s)\n",
                          current->left->value && current->left->value->lexeme ? current->left->value->lexeme : "(null)",
                          last->left->value && last->left->value->lexeme ? last->left->value->lexeme : "(null)");
        if (current->left->value && current->left->value->lexeme) {
          /* Replace the output token node entirely to ensure binding targets the real var */
          last->left = create_ans_token(csound, current->left->value->lexeme);
        }
      }
      /* Prefer the last node with a concrete LHS temp; fallback to tail value */
      TREE* pickWithLeft2 = NULL;
      for (TREE* tscan2 = expr; tscan2 != NULL; tscan2 = tscan2->next) {
        if (tscan2->left && tscan2->left->value && tscan2->left->value->lexeme) {
          pickWithLeft2 = tscan2;
        }
      }
      const char* pickedLex2 = NULL;
      if (pickWithLeft2) {
        pickedLex2 = pickWithLeft2->left->value->lexeme;
      } else if (last->value && last->value->lexeme) {
        pickedLex2 = last->value->lexeme;
      }
      if (!pickedLex2) {
        return NULL;
      }
      newArg = create_ans_token(csound, (char*)pickedLex2);
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
    {


      // Map to existing scalar/audio variants only. Mixed-rate ki/ik variants do not exist.
      char* ltype = get_arg_string_from_tree(csound, root->left, typeTable);
      char* rtype = get_arg_string_from_tree(csound, root->right, typeTable);
      int l_is_array = (ltype && strchr(ltype, '[') != NULL);
      int r_is_array = (rtype && strchr(rtype, '[') != NULL);
      if (!l_is_array && !r_is_array) {
        char lc = (ltype && ltype[0]) ? ltype[0] : rate_hint_from_ident(root->left);
        char rc = (rtype && rtype[0]) ? rtype[0] : rate_hint_from_ident(root->right);
        // Treat constants as i for selection purposes here; resolver accepts i/c where k is required.
        if (lc == 'c') lc = 'i';
        if (rc == 'c') rc = 'i';
        if (lc == 'a' || rc == 'a') {
          if (lc == 'a' && rc == 'a')      strNcpy(op, "##add.aa", 80);
          else if (lc == 'a')              strNcpy(op, "##add.ak", 80);
          else                              strNcpy(op, "##add.ka", 80);
        } else if (lc == 'k' || rc == 'k') {
          // Any k involvement -> use kk;
          strNcpy(op, "##add.kk", 80);
        } else {
          // Pure i/c -> ii
          strNcpy(op, "##add.ii", 80);
        }
      } else {
        // Arrays handled by dedicated table ops; keep short name and let resolver select bracketed variant
        strNcpy(op, "##add", 80);
      }
      if (ltype) csound->Free(csound, ltype);
      if (rtype) csound->Free(csound, rtype);
      outarg = create_out_arg_for_expression(csound, op, root->left,
                                             root->right, typeTable);
    }
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
    {
      // Prefer explicit scalar variant (i/k/a) to avoid wrong first-match fallback
      char* ltype = get_arg_string_from_tree(csound, root->left, typeTable);
      char* rtype = get_arg_string_from_tree(csound, root->right, typeTable);
      int l_is_array = (ltype && strchr(ltype, '[') != NULL);
      int r_is_array = (rtype && strchr(rtype, '[') != NULL);
      if (!l_is_array && !r_is_array) {
        char lc = (ltype && ltype[0]) ? ltype[0] : rate_hint_from_ident(root->left);
        char rc = (rtype && rtype[0]) ? rtype[0] : rate_hint_from_ident(root->right);
        // normalize 'c' constants as i-rate for op selection
        if (lc == 'c') lc = 'i';
        if (rc == 'c') rc = 'i';
        if (lc == 'a' || rc == 'a')      strNcpy(op, "##pow.a", 80);
        else if (lc == 'k' || rc == 'k') strNcpy(op, "##pow.k", 80);
        else                              strNcpy(op, "##pow.i", 80);
      } else {
        // Array cases resolved by resolver using precise in/out arg strings
        strNcpy(op, "##pow", 80);
      }
      if (ltype) csound->Free(csound, ltype);
      if (rtype) csound->Free(csound, rtype);
      outarg = create_out_arg_for_expression(csound, op, root->left,
                                             root->right, typeTable);
    }
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
      if (csound->GetDebug(csound)) {
        csound->Message(csound, "DEBUG: T_FUNCTION calling create_out_arg with outtype_internal='%s'\n",
                        outtype_internal ? outtype_internal : "(null)");
      }
      // Capture base index before allocating temps so we can build multi-output LHS
      int multiPrimBaseIndex_local = typeTable->localPool->synthArgCount;
      outarg = create_out_arg(csound, outtype_internal,
                              typeTable->localPool->synthArgCount++, typeTable);
      // Record multi-primitive info for later (after op creation)
      multiPrimBaseIndex = multiPrimBaseIndex_local;
      if (outtype_internal && outtype_internal[0] && outtype_internal[1]) {
        // Detect pure sequence of primitive single-letter outputs (e.g., "aa", "ak")
        size_t nmp = strlen(outtype_internal);
        int allPrim = 1;
        for (size_t i = 0; i < nmp; ++i) {
          char c = outtype_internal[i];
          if (!(c=='i'||c=='k'||c=='K'||c=='a'||c=='S'||c=='s'||c=='B'||c=='b'||c=='f'||c=='t')) { allPrim = 0; break; }
        }
        if (allPrim) {
          multiPrimCount = (int)nmp;
          if (multiPrimCount > (int)sizeof(multiPrimTypes)) multiPrimCount = (int)sizeof(multiPrimTypes);
          for (int i = 0; i < multiPrimCount; ++i) multiPrimTypes[i] = outtype_internal[i];
        }
      }

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

      if (csound->GetDebug(csound)) {
        csound->Message(csound, "DEBUG: T_OPCALL calling create_out_arg with outype='%s'\n",
                        outype ? outype : "(null)");
      }
      outarg = create_out_arg(csound, outype,
                              typeTable->localPool->synthArgCount++, typeTable);

    }
    break;
  case T_ARRAY:
    {
      char* outype = NULL;
      strNcpy(op, "##array_get", 80);
      // Track if base started as a struct expression before rewriting
      int leftWasStructExpr = (root->left && root->left->type == STRUCT_EXPR);
      // If the base is a struct expression (e.g., users.names[2])
      // emit member_get(array) followed by array_get/array_get_struct here.
      if (leftWasStructExpr) {
        // Determine array member type (e.g., S[] or :Person;[])
        char* arrTypeName = get_arg_type2(csound, root->left, typeTable);
        if (arrTypeName == NULL) return NULL;
        // Determine element type for the array_get op (e.g., S or :Person;)
        outype = get_arg_type2(csound, root, typeTable);
        if (outype == NULL) { csound->Free(csound, arrTypeName); return NULL; }
        // Build member_get to extract the array member from the struct
        TREE* baseStructExpr = root->left; // STRUCT_EXPR for users.names
        // Resolve struct input (could itself be complex)
        TREE* structInputNode = NULL;
        if (baseStructExpr->left && (baseStructExpr->left->type == T_ARRAY || baseStructExpr->left->type == STRUCT_EXPR)) {
          TREE* pre = create_expression(csound, baseStructExpr->left, line, locn, typeTable);
          if (pre == NULL) { csound->Free(csound, arrTypeName); csound->Free(csound, outype); return NULL; }
          anchor = append_to_tree(csound, anchor, pre);
          TREE* preLast = tree_tail(pre);
          if (preLast && preLast->left && preLast->left->value && preLast->left->value->lexeme) {
            structInputNode = create_ans_token(csound, preLast->left->value->lexeme);
          } else {
            structInputNode = copy_node(csound, baseStructExpr->left);
          }
        } else {
          structInputNode = copy_node(csound, baseStructExpr->left);
        }
        // Compute member index from struct type and member name
        char* structTypeName = get_arg_type2(csound, baseStructExpr->left, typeTable);
        CS_TYPE* st = csoundGetTypeWithVarTypeName(csound->typePool, structTypeName);
        csound->Free(csound, structTypeName);
        int mIndex = 0; if (st) {
          const char* mname = baseStructExpr->right && baseStructExpr->right->value ? baseStructExpr->right->value->lexeme : "";
          CONS_CELL* cc = st->members; int ii = 0; while (cc) { CS_VARIABLE* mv=(CS_VARIABLE*)cc->value; if (!strcmp(mv->varName, mname)) { mIndex = ii; break; } ii++; cc = cc->next; }
        }
        TREE* op_member_get_array = create_opcode_token(csound, "##member_get");
        char* outArrName = create_out_arg(csound, arrTypeName, typeTable->localPool->synthArgCount++, typeTable);
        op_member_get_array->left = create_ans_token(csound, outArrName);
        char ibufA[32];
        snprintf(ibufA, sizeof ibufA, "%d", mIndex);
        TREE* idxNodeA = make_leaf(csound, baseStructExpr->right ? baseStructExpr->right->line : line,
                                   baseStructExpr->right ? baseStructExpr->right->locn : locn,
                                   INTEGER_TOKEN, make_int(csound, ibufA));
        op_member_get_array->right = structInputNode;
        op_member_get_array->right->next = idxNodeA;

        // Now emit array_get (or array_get_struct for UDT elements)
        const CS_TYPE* elemT = csoundGetTypeWithVarTypeName(csound->typePool, outype);
        int useStructGetLocal = (elemT && elemT->userDefinedType) ? 1 : 0;
        TREE* op_array_get = create_opcode_token(csound, useStructGetLocal ? "##array_get_struct" : "##array_get");
        char* outElemName = create_out_arg(csound, outype, typeTable->localPool->synthArgCount++, typeTable);
        op_array_get->left = create_ans_token(csound, outElemName);
        op_array_get->right = create_ans_token(csound, outArrName);
        op_array_get->right->next = copy_node(csound, root->right);

        // Chain and return the two-op sequence
        op_member_get_array->next = op_array_get;
        return op_member_get_array;
      } else if (root->left && root->left->type != T_IDENT &&
                 root->left->type != T_ARRAY_IDENT &&
                 root->left->type != STRUCT_EXPR) {
        // Case: array index applied to a non-ident expression (e.g., UDO call)
        // Emit the left expression first to materialize the array temp with sizes set,
        // then emit array_get using that temp as the base.
        char* elemType = get_arg_type2(csound, root, typeTable);
        if (elemType == NULL) return NULL;
        // 1) Lower the left expression producing the array value and get its ANS temp
        const char* baseOutName = NULL;
        TREE* pre = lower_expr_to_temp(csound, root->left, line, locn, typeTable, &baseOutName);
        if (pre == NULL) { return NULL; }
        anchor = append_to_tree(csound, anchor, pre);
        // Prefer the concrete last LHS temp from the emitted left chain as base array
        {
          TREE* preLast = tree_tail(pre);
          if (preLast && preLast->left && preLast->left->value && preLast->left->value->lexeme) {
            baseOutName = preLast->left->value->lexeme;
          }
        }


        // Retarget the left-expression's final output to a fresh array temp so
        // the UDO writes directly into an initialised base we can index safely.
        char* arrTypeNameNI = get_arg_type2(csound, root->left, typeTable);
        if (arrTypeNameNI) {
          char* baseArrOutName = create_out_arg(csound, arrTypeNameNI, typeTable->localPool->synthArgCount++, typeTable);
          // Find the last opcall/function node in 'pre' that actually produces the array
          TREE* cursor = pre; TREE* lastWithLeft = NULL;
          while (cursor) { if (cursor->left) lastWithLeft = cursor; cursor = cursor->next; }
          if (!lastWithLeft) lastWithLeft = tree_tail(pre);
          if (lastWithLeft) {
            lastWithLeft->left = create_ans_token(csound, baseArrOutName);
            baseOutName = baseArrOutName;
            if (csound->GetDebug(csound)) {
              csound->Message(csound, "[orc] retarget inline array result to %s (type=%s)\n",
                              baseArrOutName ? baseArrOutName : "(null)", arrTypeNameNI ? arrTypeNameNI : "(null)");
            }
          }
          csound->Free(csound, arrTypeNameNI);
        }

        // 2) Select array_get variant based on element type
        int useStructGetLocal = 0;
        {
          const CS_TYPE* et = csoundGetTypeWithVarTypeName(csound->typePool, elemType);
          if (et && et->userDefinedType) useStructGetLocal = 1;
        }
        TREE* op_array_get = create_opcode_token(csound, useStructGetLocal ? "##array_get_struct" : "##array_get");
        if (!useStructGetLocal && elemType && strlen(elemType) == 1) {
          switch (elemType[0]) {
            case 'S': strNcpy(op, "##array_get.S", 80); break;
            case 'k': strNcpy(op, "##array_get.k", 80); break;
            case 'a': strNcpy(op, "##array_get.a", 80); break;
            case 'i': strNcpy(op, "##array_get.i", 80); break;
            default: break;
          }
        }
        char* outElemName = create_out_arg(csound, elemType, typeTable->localPool->synthArgCount++, typeTable);
        op_array_get->left = create_ans_token(csound, outElemName);
        // Use the function result temp deterministically; fall back to copying the base
        if (baseOutName) {
          // Use the concrete ANS temp name of the function result as the array base
          op_array_get->right = create_ans_token(csound, (char*)baseOutName);
        } else {
          op_array_get->right = copy_node(csound, root->left);
        }
        if (csound->GetDebug(csound)) {
          csound->Message(csound, "[orc] T_ARRAY non-ident: baseOutName=%s elemType=%s\n", baseOutName?baseOutName:"(null)", elemType?elemType:"(null)");
        }

        op_array_get->right->next = copy_node(csound, root->right);
        // 3) Emit as a distinct statement after the lowered left expression
        anchor = append_to_tree(csound, anchor, op_array_get);
        // Return the full statement chain
        return anchor;


      } else {
        // Regular array variable case: infer from variable subtype
        char *varBaseName = root->left->value->lexeme;
        var = find_var_from_pools(csound, varBaseName, varBaseName, typeTable);
        if (var == NULL) {
          // Array base not yet declared in symbol table. Infer element type from
          // identifier prefix so we can still materialize an array_get for uses like
          // 'printks(..., kArr[0])'. Default to 'i' if unknown; treat 'a' bases as
          // signal-as-array views yielding k-rate on indexing.
          const char* nm = varBaseName;
          char base = (nm && nm[0]) ? nm[0] : 'i';
          if (!(base=='i' || base=='k' || base=='a' || base=='S' || base=='B')) base = 'i';
          char inferred[2]; inferred[0] = (base=='a') ? 'k' : base; inferred[1] = '\0';
          outype = cs_strdup(csound, inferred);
        } else if (var->varType == &CS_VAR_TYPE_ARRAY) {
          outype = strdup(var->subType->varTypeName);
          // Adjust i->k if any k-rate indices present
          if (outype[0] == 'i') {
            TREE* inds = root->right;
            while (inds) {
              char *xx = get_arg_string_from_tree(csound, inds, typeTable);
              if (xx && xx[0] == 'k') { outype[0] = 'k'; }
              if (xx) csound->Free(csound, xx);
              inds = inds->next;
            }
          }
        } else if (var->varType == &CS_VAR_TYPE_A) {
          outype = "k";
        } else {
          // Fallback: try semantic type inference for array element type
          outype = get_arg_type2(csound, root, typeTable);
          if (outype == NULL) {
            synterr(csound,
                    Str("invalid array type %s line %d\n"),
                    var->varType && var->varType->varTypeName ? var->varType->varTypeName : "(null)",
                    line);
            return NULL;
          }
        }
      }

      if (outype == NULL) return NULL;

	      // Ensure base argument is typed as an array when symbol table lags:
	      // if left is a plain identifier not known as ARRAY, wrap it as T_ARRAY_IDENT
	      // with an explicit optype suffix like "i[]" so argument typing becomes "i[]".
	      if (root->left && root->left->type == T_IDENT) {
	        const char* baseNm = (root->left->value && root->left->value->lexeme) ? root->left->value->lexeme : NULL;
	        CS_VARIABLE* vbase = (baseNm) ? find_var_from_pools(csound, (char*)baseNm, (char*)baseNm, typeTable) : NULL;
	        if (baseNm && (!vbase)) {
	          ORCTOKEN* tok = make_token(csound, (char*)baseNm);
	          tok->type = T_ARRAY_IDENT;
	          // Build explicit optype like "i[]" or "k[]" based on element outype
	          if (outype && strlen(outype) >= 1) {
	            char buf[8]; size_t n = strlen(outype); if (n > 3) n = 3;
	            memcpy(buf, outype, n); buf[n] = '\0';
	            strncat(buf, "[]", sizeof(buf)-strlen(buf)-1);
	            tok->optype = cs_strdup(csound, buf);
	          } else {
	            tok->optype = cs_strdup(csound, "i[]");
	          }
	          TREE* arrIdent = make_leaf(csound, root->left->line, root->left->locn, T_ARRAY_IDENT, tok);
	          root->left = arrIdent;
	        }
	      }


	      // If symbol exists but is not yet registered as ARRAY, hint its argument type
	      // to the resolver by converting to a T_TYPED_IDENT with an explicit array type.
	      if (root->left && root->left->type == T_IDENT) {
	        const char* baseNm2 = (root->left->value && root->left->value->lexeme) ? root->left->value->lexeme : NULL;
	        CS_VARIABLE* vbase2 = (baseNm2) ? find_var_from_pools(csound, (char*)baseNm2, (char*)baseNm2, typeTable) : NULL;
	        if (baseNm2 && vbase2 && vbase2->varType != &CS_VAR_TYPE_ARRAY) {
	          ORCTOKEN* tok2 = make_token(csound, (char*)baseNm2);
	          // Build "i[]" optype from element outype, fallback to 'i'
	          if (outype && strlen(outype) >= 1) {
	            char buf2[8]; size_t n2 = strlen(outype); if (n2 > 3) n2 = 3;
	            memcpy(buf2, outype, n2); buf2[n2] = '\0';
	            strncat(buf2, "[]", sizeof(buf2)-strlen(buf2)-1);
	            tok2->optype = cs_strdup(csound, buf2);
	          } else {
	            tok2->optype = cs_strdup(csound, "i[]");
	          }
	          TREE* typed = make_leaf(csound, root->left->line, root->left->locn, T_TYPED_IDENT, tok2);
	          root->left = typed;
	        }
	      }


      // If the element is a user-defined struct, or the base WAS a STRUCT_EXPR,
      int useStructGet = 0;
      if (leftWasStructExpr) {
        useStructGet = 1;
      } else if (root->left && (root->left->type == T_IDENT || root->left->type == T_ARRAY_IDENT)) {
        char *varBaseName = root->left->value->lexeme;
        CS_VARIABLE* v = find_var_from_pools(csound, varBaseName, varBaseName, typeTable);
        if (v && v->varType == &CS_VAR_TYPE_ARRAY && v->subType && v->subType->userDefinedType) {
          useStructGet = 1;
        }
      } else {
        // Fall back: parse outype to a base name and check the type pool
        char *typ = remove_type_quoting(csound, outype);
        // Strip trailing []
        size_t n = strlen(typ);
        while (n >= 2 && typ[n-2] == '[' && typ[n-1] == ']') { typ[n-2] = '\0'; n -= 2; }
        const CS_TYPE* et = csoundGetTypeWithVarTypeName(csound->typePool, typ);
        if (et && et->userDefinedType) useStructGet = 1;
        csound->Free(csound, typ);
      }
      if (useStructGet) {
        strNcpy(op, "##array_get_struct", 80);
      } else {
        // Special-case: signal-as-array view a[k] -> use vaops entry "##array_get" with intypes "ak"
        int pinnedAK = 0;
        if (root->left) {
          char* baseType = get_arg_type2(csound, root->left, typeTable);
          if (baseType && outype && strlen(baseType)==1 && baseType[0]=='a' && strlen(outype)==1 && outype[0]=='k') {
            strNcpy(op, "##array_get", 80);
            pinnedAK = 1;
          }
          if (baseType) csound->Free(csound, baseType);
        }
        // Otherwise, select type-specific array_get opcode based on outype
        if (!pinnedAK && outype && strlen(outype) == 1) {
          switch (outype[0]) {
            case 'S':
              strNcpy(op, "##array_get.S", 80);
              break;
            case 'k':
              strNcpy(op, "##array_get.k", 80);
              break;
            case 'a':
              strNcpy(op, "##array_get.a", 80);
              break;
            case 'i':
              strNcpy(op, "##array_get.i", 80);
              break;
            default:
              // Keep default "##array_get"
              break;
          }
        }
      }
      outarg = create_out_arg(csound, outype, typeTable->localPool->synthArgCount++, typeTable);
    }

    break;
  case STRUCT_EXPR:
    {
      // For STRUCT_EXPR, handle nested struct+array chains robustly.
      // We emit an explicit left-to-right chain:
      //   (member_get array) -> (array_get_struct) [repeated] -> (member_get scalar)

      // Full chain flattener: parse entire struct+array access chain and emit contiguous operations
      TREE* flattenResult = flatten_struct_array_chain(csound, root, line, locn, typeTable, &anchor);
      if (flattenResult) {
        return flattenResult;
      }

      // If left is an array, we might have deeper nesting (e.g., a.b[0].c[1].d).
      if (root->left && root->left->type == T_ARRAY) {
        // baseStructExpr is the STRUCT_EXPR for the member just before the index on the left
        TREE* baseStructExpr = root->left->left; // e.g., john.relativeList
        if (baseStructExpr == NULL || baseStructExpr->type != STRUCT_EXPR) {
          return NULL;
        }

        // If baseStructExpr->left itself is complex (STRUCT_EXPR or T_ARRAY),
        // recursively lower it first and use its ANS as the struct input.
        TREE* structInputNode = NULL;
        if (baseStructExpr->left && (baseStructExpr->left->type == T_ARRAY || baseStructExpr->left->type == STRUCT_EXPR)) {
          TREE* pre = create_expression(csound, baseStructExpr->left, line, locn, typeTable);
          if (pre == NULL) return NULL;
          anchor = append_to_tree(csound, anchor, pre);
          TREE* preLast = tree_tail(pre);
          if (preLast && preLast->left && preLast->left->value && preLast->left->value->lexeme) {
            structInputNode = create_ans_token(csound, preLast->left->value->lexeme);
          } else {
            // Fallback to a copy if we cannot find the ANS
            structInputNode = copy_node(csound, baseStructExpr->left);
          }
        } else {
          structInputNode = copy_node(csound, baseStructExpr->left); // simple ident
        }

        // 1) member_get array member from current struct input
        char* arrayMemberType = get_arg_type2(csound, baseStructExpr, typeTable);
        if (arrayMemberType == NULL) return NULL;
        TREE* op_member_get_array = create_opcode_token(csound, "##member_get");

        char* outArrName = create_out_arg(csound, arrayMemberType, typeTable->localPool->synthArgCount++, typeTable);
        op_member_get_array->left = create_ans_token(csound, outArrName);
        // Compute member index from struct type and member name
        char* structTypeName = get_arg_type2(csound, baseStructExpr->left, typeTable);
        if (structInputNode && structInputNode->value && structInputNode->value->lexeme) {
          // Try to prefer the actual input's type when possible
          char* t2 = get_arg_type2(csound, structInputNode, typeTable);
          if (t2) { csound->Free(csound, structTypeName); structTypeName = t2; }
        }
        CS_TYPE* st = csoundGetTypeWithVarTypeName(csound->typePool, structTypeName);
        csound->Free(csound, structTypeName);
        int mIndex = 0; if (st) {
          const char* mname = baseStructExpr->right && baseStructExpr->right->value ? baseStructExpr->right->value->lexeme : "";
          CONS_CELL* cc = st->members; int ii = 0;
          while (cc) { CS_VARIABLE* mv=(CS_VARIABLE*)cc->value; if (!strcmp(mv->varName, mname)) { mIndex = ii; break; } ii++; cc = cc->next; }
        }
        char ibufA[32]; snprintf(ibufA, sizeof ibufA, "%d", mIndex);
        TREE* idxNodeA = make_leaf(csound, baseStructExpr->right ? baseStructExpr->right->line : line,
                                   baseStructExpr->right ? baseStructExpr->right->locn : locn,
                                   INTEGER_TOKEN, make_int(csound, ibufA));
        op_member_get_array->right = structInputNode;
        op_member_get_array->right->next = idxNodeA;

        // 2) array_get_struct for the index on the immediate left
        char* arrayElemType = get_arg_type2(csound, root->left, typeTable); // element type (e.g., Person)
        if (arrayElemType == NULL) return NULL;
        TREE* op_array_get_struct = create_opcode_token(csound, "##array_get_struct");
        char* outStructName = create_out_arg(csound, arrayElemType, typeTable->localPool->synthArgCount++, typeTable);
        op_array_get_struct->left = create_ans_token(csound, outStructName);
        op_array_get_struct->right = create_ans_token(csound, outArrName);
        op_array_get_struct->right->next = copy_node(csound, root->left->right);

        // 3) Nested array access on the right? Inline the next level to preserve chaining
        if (root->right && root->right->type == STRUCT_EXPR && root->right->left && root->right->left->type == T_ARRAY) {
          if (csound->GetDebug(csound)) csound->Message(csound, "DEBUG: STRUCT_EXPR(left=T_ARRAY): inlining nested right array access\n");
          TREE* inner = root->right;               // STRUCT_EXPR for next member/index
          TREE* innerArray = inner->left;          // T_ARRAY for next index
          TREE* innerBase = innerArray->left;      // STRUCT_EXPR for inner member name

          // 3a) member_get array from current struct element (outStructName)
          char* innerArrayType = get_arg_type2(csound, innerBase, typeTable);
          if (innerArrayType == NULL) return NULL;
          TREE* op_member_get_array2 = create_opcode_token(csound, "##member_get");
          char* outArrName2 = create_out_arg(csound, innerArrayType, typeTable->localPool->synthArgCount++, typeTable);
          op_member_get_array2->left = create_ans_token(csound, outArrName2);
          // Compute inner member index
          char* elemStructTypeName2 = get_arg_type2(csound, root->left, typeTable);
          CS_TYPE* elemStructType2 = csoundGetTypeWithVarTypeName(csound->typePool, elemStructTypeName2);
          csound->Free(csound, elemStructTypeName2);
          int mIndex2 = 0; if (elemStructType2) {
            const char* mname2 = (innerBase && innerBase->right && innerBase->right->value) ? innerBase->right->value->lexeme : "";
            CONS_CELL* cc2 = elemStructType2->members; int jj = 0; while (cc2) { CS_VARIABLE* mv=(CS_VARIABLE*)cc2->value; if (!strcmp(mv->varName, mname2)) { mIndex2 = jj; break; } jj++; cc2 = cc2->next; }
          }
          char ibufC[32]; snprintf(ibufC, sizeof ibufC, "%d", mIndex2);
          TREE* idxNodeMember2 = make_leaf(csound, innerBase ? innerBase->line : line,
                                           innerBase ? innerBase->locn : locn,
                                           INTEGER_TOKEN, make_int(csound, ibufC));
          TREE* structElemTemp2 = create_ans_token(csound, outStructName);
          op_member_get_array2->right = structElemTemp2;
          op_member_get_array2->right->next = idxNodeMember2;

          // 3b) array_get_struct for the inner index
          char* innerElemType = get_arg_type2(csound, innerArray, typeTable);
          if (innerElemType == NULL) return NULL;
          TREE* op_array_get_struct2 = create_opcode_token(csound, "##array_get_struct");
          char* outStructName2 = create_out_arg(csound, innerElemType, typeTable->localPool->synthArgCount++, typeTable);
          op_array_get_struct2->left = create_ans_token(csound, outStructName2);
          op_array_get_struct2->right = create_ans_token(csound, outArrName2);
          op_array_get_struct2->right->next = copy_node(csound, innerArray->right);

          // 3c) Final scalar on the right? Emit last member_get now
          if (inner->right && inner->right->value) {
            char* finalOutType = get_arg_type2(csound, inner, typeTable);
            if (finalOutType == NULL) return NULL;
            outarg = create_out_arg(csound, finalOutType, typeTable->localPool->synthArgCount++, typeTable);
            csound->Free(csound, finalOutType);

            TREE* op_member_get_scalar = create_opcode_token(csound, "##member_get");
            op_member_get_scalar->left = create_ans_token(csound, outarg);
            TREE* structElemTemp3 = create_ans_token(csound, outStructName2);
            op_member_get_scalar->right = structElemTemp3;
            // Compute index of the scalar member from element struct type
            int scalarIdx2 = 0; if (elemStructType2) {
              const char* scalarName2 = inner->right && inner->right->value ? inner->right->value->lexeme : "";
              CONS_CELL* cc3 = elemStructType2->members; int kk = 0; while (cc3) { CS_VARIABLE* mv=(CS_VARIABLE*)cc3->value; if (!strcmp(mv->varName, scalarName2)) { scalarIdx2 = kk; break; } kk++; cc3 = cc3->next; }
            }
            char ibufD[32]; snprintf(ibufD, sizeof ibufD, "%d", scalarIdx2);
            TREE* scalarIdxNode2 = make_leaf(csound, inner->right ? inner->right->line : line,
                                             inner->right ? inner->right->locn : locn,
                                             INTEGER_TOKEN, make_int(csound, ibufD));
            structElemTemp3->next = scalarIdxNode2;

            // Chain and return head (five ops total)
            op_member_get_array->next = op_array_get_struct;
            op_array_get_struct->next = op_member_get_array2;
            op_member_get_array2->next = op_array_get_struct2;
            op_array_get_struct2->next = op_member_get_scalar;
            return op_member_get_array;
          }

          // No final scalar; return four-op chain
          op_member_get_array->next = op_array_get_struct;
          op_array_get_struct->next = op_member_get_array2;
          op_member_get_array2->next = op_array_get_struct2;
          return op_member_get_array;
        }

        // 3) If the right side of the root is a scalar member, emit final member_get now.
        // Otherwise, return the chain so outer STRUCT_EXPR levels can continue.
        if (root->right && root->right->value) {
          // Determine precise member type for correct typing
          char* scalarOutType = get_arg_type2(csound, root, typeTable);
          if (scalarOutType == NULL) return NULL;
          outarg = create_out_arg(csound, scalarOutType, typeTable->localPool->synthArgCount++, typeTable);
          csound->Free(csound, scalarOutType);

          TREE* op_member_get_scalar = create_opcode_token(csound, "##member_get");
          op_member_get_scalar->left = create_ans_token(csound, outarg);
          TREE* structElemTemp = create_ans_token(csound, outStructName);
          op_member_get_scalar->right = structElemTemp;
          // Compute index of the scalar member from element struct type
          char* elemStructTypeName = get_arg_type2(csound, root->left, typeTable);
          CS_TYPE* elemStructType = csoundGetTypeWithVarTypeName(csound->typePool, elemStructTypeName);
          csound->Free(csound, elemStructTypeName);
          int scalarIdx = 0; if (elemStructType) {
            const char* scalarName = (root->right && root->right->value) ? root->right->value->lexeme : "";
            CONS_CELL* cell = elemStructType->members; int i = 0;
            while (cell) { CS_VARIABLE* mv = (CS_VARIABLE*)cell->value; if (!strcmp(mv->varName, scalarName)) { scalarIdx = i; break; } i++; cell = cell->next; }
          }
          char ibufB[32]; snprintf(ibufB, sizeof ibufB, "%d", scalarIdx);
          TREE* scalarIdxNode = make_leaf(csound, root->right->line, root->right->locn,
                                          INTEGER_TOKEN, make_int(csound, ibufB));
          structElemTemp->next = scalarIdxNode;

          // Chain and return head
          op_member_get_array->next = op_array_get_struct;
          op_array_get_struct->next = op_member_get_scalar;
          return op_member_get_array;
        } else {
          // Return the two-op chain; outer STRUCT_EXPR levels can continue
          op_member_get_array->next = op_array_get_struct;
          return op_member_get_array;
        }
      } else {
        // Case: left is a struct temp/ident. If the right is a nested array access, inline it.
        if (root->right && root->right->type == STRUCT_EXPR && root->right->left && root->right->left->type == T_ARRAY) {
          if (csound->GetDebug(csound)) csound->Message(csound, "DEBUG: STRUCT_EXPR(left=struct): inlining nested right array access\n");
          TREE* inner = root->right;          // STRUCT_EXPR for c[1].d
          TREE* innerArray = inner->left;     // T_ARRAY for c[1]
          TREE* innerBase = innerArray->left; // STRUCT_EXPR for c

          // 1) member_get array from current struct (root->left)
          char* innerArrayType2 = get_arg_type2(csound, innerBase, typeTable);
          if (innerArrayType2 == NULL) return NULL;
          TREE* op_member_get_array2 = create_opcode_token(csound, "##member_get");
          char* outArrName2 = create_out_arg(csound, innerArrayType2, typeTable->localPool->synthArgCount++, typeTable);
          op_member_get_array2->left = create_ans_token(csound, outArrName2);
          // Resolve struct input for the current left; if complex, lower it first and use its ANS
          TREE* structVarNode2 = NULL;
          if (root->left && (root->left->type == T_ARRAY || root->left->type == STRUCT_EXPR)) {
            TREE* pre2 = create_expression(csound, root->left, line, locn, typeTable);
            if (pre2 == NULL) return NULL;
            anchor = append_to_tree(csound, anchor, pre2);
            TREE* preLast2 = tree_tail(pre2);
            if (preLast2 && preLast2->left && preLast2->left->value && preLast2->left->value->lexeme) {
              structVarNode2 = create_ans_token(csound, preLast2->left->value->lexeme);
            } else {
              structVarNode2 = copy_node(csound, root->left);
            }
          } else {
            structVarNode2 = copy_node(csound, root->left);
          }
          op_member_get_array2->right = structVarNode2;
          // Compute member index on the current struct type
          char* typeName2 = get_arg_type2(csound, root->left, typeTable);
          CS_TYPE* structType2 = csoundGetTypeWithVarTypeName(csound->typePool, typeName2);
          csound->Free(csound, typeName2);
          int idx2 = 0; if (structType2) {
            const char* mname2 = innerBase && innerBase->right && innerBase->right->value ? innerBase->right->value->lexeme : "";
            CONS_CELL* cell2 = structType2->members; int i2 = 0;
            while (cell2) { CS_VARIABLE* mv2 = (CS_VARIABLE*)cell2->value; if (!strcmp(mv2->varName, mname2)) { idx2 = i2; break; } i2++; cell2 = cell2->next; }
          }
          char ibuf2[32]; snprintf(ibuf2, sizeof ibuf2, "%d", idx2);
          TREE* memberIdxNode2 = make_leaf(csound, innerBase ? innerBase->line : line,
                                           innerBase ? innerBase->locn : locn,
                                           INTEGER_TOKEN, make_int(csound, ibuf2));
          structVarNode2->next = memberIdxNode2;

          // 2) array_get_struct for the provided index
          char* innerElemType2 = get_arg_type2(csound, innerArray, typeTable);
          if (innerElemType2 == NULL) return NULL;
          TREE* op_array_get_struct2 = create_opcode_token(csound, "##array_get_struct");
          char* outStructName2 = create_out_arg(csound, innerElemType2, typeTable->localPool->synthArgCount++, typeTable);
          op_array_get_struct2->left = create_ans_token(csound, outStructName2);
          op_array_get_struct2->right = create_ans_token(csound, outArrName2);
          op_array_get_struct2->right->next = copy_node(csound, innerArray->right);

          // 3) Final scalar?
          if (inner->right && inner->right->value) {
            char* finalOutType2 = get_arg_type2(csound, inner, typeTable);
            if (finalOutType2 == NULL) return NULL;
            outarg = create_out_arg(csound, finalOutType2, typeTable->localPool->synthArgCount++, typeTable);
            csound->Free(csound, finalOutType2);

            TREE* op_member_get_scalar2 = create_opcode_token(csound, "##member_get");
            op_member_get_scalar2->left = create_ans_token(csound, outarg);
            TREE* structElemTemp4 = create_ans_token(csound, outStructName2);
            op_member_get_scalar2->right = structElemTemp4;
            // Scalar index from inner element struct type
            CS_TYPE* elemSt2 = csoundGetTypeWithVarTypeName(csound->typePool, innerElemType2);
            int sIdx2 = 0; if (elemSt2) {
              const char* sname2 = inner->right && inner->right->value ? inner->right->value->lexeme : "";
              CONS_CELL* cc = elemSt2->members; int k = 0; while (cc) { CS_VARIABLE* mv=(CS_VARIABLE*)cc->value; if (!strcmp(mv->varName, sname2)) { sIdx2 = k; break; } k++; cc = cc->next; }
            }
            char ibufS[32]; snprintf(ibufS, sizeof ibufS, "%d", sIdx2);
            TREE* scalarIdxNode3 = make_leaf(csound, inner->right ? inner->right->line : line,
                                             inner->right ? inner->right->locn : locn,
                                             INTEGER_TOKEN, make_int(csound, ibufS));
            structElemTemp4->next = scalarIdxNode3;

            // Chain and return
            op_member_get_array2->next = op_array_get_struct2;
            op_array_get_struct2->next = op_member_get_scalar2;
            return op_member_get_array2;
          }

          // Return chain without final scalar
          op_member_get_array2->next = op_array_get_struct2;
          return op_member_get_array2;
        }

        // Fallback: simple struct member
        char* memberType = get_arg_type2(csound, root, typeTable);
        if (memberType == NULL) return NULL;
        if (csound->GetDebug(csound)) {
          csound->Message(csound, "DEBUG: STRUCT_EXPR calling create_out_arg with memberType='%s'\n",
                          memberType ? memberType : "(null)");
        }
        outarg = create_out_arg(csound, memberType, typeTable->localPool->synthArgCount++, typeTable);
        csound->Free(csound, memberType);

        // Create type-specific ##member_get variant based on target variable rate
        char memberGetOpName[32];
        // Determine the rate of the target variable from the output argument
        char targetRate = 'k'; // default to k-rate
        if (outarg && strlen(outarg) > 1) {
          // Extract rate from output argument name (e.g., "#i0" -> 'i', "#k1" -> 'k')
          // The rate character is the second character in synthetic argument names
          targetRate = outarg[1];
        }

        printf("[DEBUG] outarg='%s', targetRate='%c'\n", outarg ? outarg : "NULL", targetRate);

        if (strchr("ikaSab", targetRate)) {
          // Use rate-specific variant like ##member_get.i, ##member_get.k, etc.
          snprintf(memberGetOpName, sizeof(memberGetOpName), "##member_get.%c", targetRate);
        } else {
          // For unknown rates, use generic variant
          strcpy(memberGetOpName, "##member_get");
        }

        printf("[DEBUG] Using opcode: %s\n", memberGetOpName);



        TREE* memberGetOp = create_opcode_token(csound, memberGetOpName);
        memberGetOp->left = create_ans_token(csound, outarg);

        // Normalize left input: if complex (array/struct expr), lower it and use its ANS
        TREE* structVarNode2b = NULL;
        if (root->left && (root->left->type == T_ARRAY || root->left->type == STRUCT_EXPR)) {
          TREE* preL = create_expression(csound, root->left, line, locn, typeTable);
          if (preL == NULL) return NULL;
          anchor = append_to_tree(csound, anchor, preL);
          TREE* preLastL = tree_tail(preL);
          if (preLastL && preLastL->left && preLastL->left->value && preLastL->left->value->lexeme) {
            structVarNode2b = create_ans_token(csound, preLastL->left->value->lexeme);
          } else {
            structVarNode2b = copy_node(csound, root->left);
          }
        } else {
          structVarNode2b = copy_node(csound, root->left);
        }
        memberGetOp->right = structVarNode2b;
        // Compute member index from struct type
        char* typeName2b = get_arg_type2(csound, root->left, typeTable);
        CS_TYPE* structType2b = csoundGetTypeWithVarTypeName(csound->typePool, typeName2b);
        csound->Free(csound, typeName2b);
        int idx2b = 0; // Initialize to 0 (first member) as default
        if (structType2b) {
          const char* mname2b = root->right && root->right->value ? root->right->value->lexeme : "";
          CONS_CELL* cell2b = structType2b->members; int i2b = 0;
          int found = 0;
          while (cell2b) { 
            CS_VARIABLE* mv2b = (CS_VARIABLE*)cell2b->value; 
            if (!strcmp(mv2b->varName, mname2b)) { 
              idx2b = i2b; 
              found = 1;
              break; 
            } 
            i2b++; 
            cell2b = cell2b->next; 
          }
          if (!found) {
            csound->Message(csound, Str("Warning: member '%s' not found in struct type, using index 0\n"), mname2b);
          }
        }
        char ibuf2b[32]; snprintf(ibuf2b, sizeof ibuf2b, "%d", idx2b);
        ORCTOKEN* idxTok2b = make_int(csound, ibuf2b);
        TREE* memberIdxNode2b = make_leaf(csound, root->right ? root->right->line : line, root->right ? root->right->locn : locn,
                                        INTEGER_TOKEN, idxTok2b);
        structVarNode2b->next = memberIdxNode2b;
        return memberGetOp;
      }
    }
    break;
   default:
    /* it should not get here, but if it does, return NULL */
    return NULL;
  }
  if (skipOpCreation) {
    // Defer codegen to STRUCT_EXPR for struct-array access; just surface the outarg
    return create_ans_token(csound, outarg);
  }

  opTree = create_opcode_token(csound, op);
  if (root->value) opTree->value->optype = root->value->optype;
  // Pin array_get exact entry to avoid fallback to wrong variant
  if (!strncmp(op, "##array_get", 12)) {
    int wantString = (outarg && outarg[0] == '#' && outarg[1] == 'S');
    int wantAK = 0;
    // Heuristic: if base is 'a' and out is 'k', prefer "ak" entry (signal-as-array view)
    if (root->type == T_ARRAY && root->left) {
      char* baseType = get_arg_type2(csound, root->left, typeTable);
      if (baseType && outarg && strlen(baseType)==1 && baseType[0]=='a' && outarg[0]=='#' && outarg[1]=='k') wantAK = 1;
      if (baseType) csound->Free(csound, baseType);
    }
    OENTRIES* ents = NULL;
    if (wantString || wantAK) ents = find_opcode2(csound, "##array_get");
    if (ents && ents->count > 0) {
      for (int i = 0; i < ents->count; i++) {
        OENTRY* e = ents->entries[i];
        if (e && e->intypes) {
          if (wantString && strcmp(e->intypes, "S[]m") == 0) { opTree->markup = e; break; }
          if (wantAK && strcmp(e->intypes, "ak") == 0) { opTree->markup = e; break; }
        }
      }
      if (opTree->markup && csound->GetDebug(csound)) {
        OENTRY* pinned = (OENTRY*)opTree->markup;
        csound->Message(csound, "[orc]   pin array_get markup -> intypes %s\n", pinned && pinned->intypes ? pinned->intypes : "(null)");
      }
      csound->Free(csound, ents);
    }
  }
  if (root->left != NULL) {
    opTree->right = root->left;
    opTree->right->next = root->right;
  } else {
    opTree->right = root->right;
  }
  // Build left outputs: multi-primitive (e.g., "aa") -> multiple temps; else single temp
  if (multiPrimCount > 0 && multiPrimBaseIndex >= 0) {
    TREE* head = NULL; TREE* tail = NULL;
    for (int i = 0; i < multiPrimCount; ++i) {
      char nameBuf[64];
      snprintf(nameBuf, sizeof nameBuf, "#%c%d", multiPrimTypes[i], multiPrimBaseIndex + i);
      TREE* n = create_ans_token(csound, nameBuf);
      if (!head) { head = tail = n; }
      else { tail->next = n; tail = n; }
    }
    opTree->left = head;
  } else {
    opTree->left = create_ans_token(csound, outarg);
  }
  opTree->line = line;
  opTree->locn = locn;
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
  case S_EQT:
    strNcpy(op, "=t", 80);
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
  // Return base variable name without brackets; array-ness is conveyed by token type (T_ARRAY_IDENT)
  // and by explicit dimensions passed to add_array_arg(). Embedding "[]" confuses symbol registration.
  char *name = (char *)csound->Calloc(csound, 36);
  snprintf(name, 36, "%c__synthetic_%"PRIi32, prefix, count);
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

/* Expand struct (UDT) initialization:
   - current must be an 'init' opcall
   - LHS must be a single struct variable (NOT an array)
   - emits ##member_set / ##member_array_assign into *anchor
   Returns 1 if it performed any lowering, 0 otherwise. */
static int expand_structs(CSOUND* csound,
                          TREE* current,
                          TYPE_TABLE* typeTable,
                          TREE** anchor)
{
  if (!current || current->type != T_OPCALL || !current->value || !current->value->lexeme)
    return 0;
  if (strcmp(current->value->lexeme, "init") != 0)
    return 0;

  /* Accept legacy nested LHS form: left=T_OPCALL(decl, args...) */
  TREE* effectiveLhs   = current->left;
  TREE* effectiveArgs  = current->right;
  if (effectiveLhs && effectiveLhs->type == T_OPCALL && effectiveLhs->left) {
    TREE* inner    = effectiveLhs;
    TREE* declNode = inner->left;          /* base decl: T_TYPED_IDENT/T_IDENT/T_ARRAY_IDENT */
    TREE* argsHead = inner->right;         /* (may be NULL) */
    TREE* argsTail = tree_tail(argsHead);
    if (argsTail)  argsTail->next = inner->next;
    else           argsHead       = inner->next;
    effectiveLhs  = declNode;
    if (!effectiveArgs) effectiveArgs = argsHead;
  }

  if (!effectiveLhs || !effectiveLhs->value || !effectiveLhs->value->lexeme)
    return 0;

  const char* structVarName = effectiveLhs->value->lexeme;

  /* Skip if LHS is array-typed (we only handle scalar struct here) */
  int lhsIsArrayTyped = 0;
  if (effectiveLhs->value->optype) {
    for (const char* p = effectiveLhs->value->optype; *p; ++p)
      if (*p == '[') { lhsIsArrayTyped = 1; break; }
  }
  if (!lhsIsArrayTyped) {
    char* lhsTypeProbe = get_arg_type2(csound, effectiveLhs, typeTable);
    if (lhsTypeProbe) {
      for (char* p = lhsTypeProbe; *p; ++p)
        if (*p == '[') { lhsIsArrayTyped = 1; break; }
      csound->Free(csound, lhsTypeProbe);
    }
  }
  if (lhsIsArrayTyped) return 0;

  /* Ensure destination variable exists in the current pool */
  CS_VARIABLE* sVar = find_var_from_pools(csound, (char*)structVarName, (char*)structVarName, typeTable);
  if (!sVar && effectiveLhs->value->optype)
    add_arg(csound, (char*)structVarName, effectiveLhs->value->optype, typeTable, effectiveLhs);

  /* Resolve struct type: prefer pool (typedef-aware), else annotation, else RHS inference */
  const CS_TYPE* structType = NULL;
  sVar = find_var_from_pools(csound, (char*)structVarName, (char*)structVarName, typeTable);
  if (sVar && sVar->varType && sVar->varType->userDefinedType)
    structType = sVar->varType;

  if (!structType && effectiveLhs->value->optype) {
    const char* ann = effectiveLhs->value->optype;
    const char* lookup = ann;
    char tmp[256];
    if (ann[0] == ':') {                         /* strip leading ':' and trailing ';' if present */
      size_t L = strlen(ann);
      if (L >= 2 && ann[L-1] == ';') {
        size_t n = L - 2; if (n >= sizeof(tmp)) n = sizeof(tmp)-1;
        memcpy(tmp, ann+1, n); tmp[n] = '\0';
        lookup = tmp;
      }
    }
    const CS_TYPE* st = csoundGetTypeWithVarTypeName(csound->typePool, lookup);
    if (st && st->userDefinedType) structType = st;
  }

  if (!structType) {
    /* Fallback: infer from first RHS arg (handles array-of-UDT → element type) */
    for (TREE* probe = effectiveArgs; probe && !structType; probe = probe->next) {
      char* at = get_arg_type2(csound, probe, typeTable);
      if (at) {
        size_t n = strlen(at);
        int isArr = (n >= 2 && at[n-2] == '[' && at[n-1] == ']');
        if (isArr) at[n-2] = '\0';
        const CS_TYPE* st = csoundGetTypeWithVarTypeName(csound->typePool, at);
        if (st && st->userDefinedType) structType = st;
        csound->Free(csound, at);
      }
    }
  }

  if (!structType || !structType->userDefinedType)
    return 0; /* not a struct init */

  /* Emit per-member assignments */
  TREE* argNode = effectiveArgs;
  CONS_CELL* cell = structType->members;
  int midx = 0;
  while (cell && argNode) {
    CS_VARIABLE* mvar = (CS_VARIABLE*)cell->value;

    if (mvar && mvar->varType == &CS_VAR_TYPE_ARRAY) {
      /* ##member_array_assign(struct, idx, arrayVar) */
      TREE* op = create_opcode_token(csound, "##member_array_assign");
      op->right = make_leaf(csound, effectiveLhs->line, effectiveLhs->locn,
                            T_IDENT, make_token(csound, (char*)structVarName));
      if (structType->varTypeName)
        op->right->value->optype = cs_strdup(csound, structType->varTypeName);

      char nbuf[32]; snprintf(nbuf, sizeof nbuf, "%d", midx);
      TREE* midxNode = make_leaf(csound, argNode->line, argNode->locn,
                                 INTEGER_TOKEN, make_int(csound, nbuf));
      op->right->next = midxNode;

      TREE* arrArg = copy_node(csound, argNode);     /* single node only */
      if (arrArg) arrArg->next = NULL;
      op->right->next->next = arrArg;

      /* Pin exact variant if available */
      OENTRIES* es = find_opcode2(csound, "##member_array_assign");
      if (es && es->count > 0) {
        for (int i = 0; i < es->count; i++) {
          OENTRY* e = es->entries[i];
          if (e && e->intypes && strcmp(e->intypes, ".c.[]") == 0) { op->markup = e; break; }
        }
        csound->Free(csound, es);
      }
      *anchor = append_to_tree(csound, *anchor, op);
    } else {
      /* ##member_set(struct, idx, value) */
      TREE* op = create_opcode_token(csound, "##member_set");
      op->right = make_leaf(csound, effectiveLhs->line, effectiveLhs->locn,
                            T_IDENT, make_token(csound, (char*)structVarName));
      if (structType->varTypeName)
        op->right->value->optype = cs_strdup(csound, structType->varTypeName);

      char nbuf[32]; snprintf(nbuf, sizeof nbuf, "%d", midx);
      TREE* midxNode = make_leaf(csound, argNode->line, argNode->locn,
                                 INTEGER_TOKEN, make_int(csound, nbuf));
      op->right->next = midxNode;

      TREE* valArg = copy_node(csound, argNode);     /* single node only */
      if (valArg) valArg->next = NULL;
      op->right->next->next = valArg;

      OENTRIES* es = find_opcode2(csound, "##member_set");
      if (es && es->count > 0) {
        for (int i = 0; i < es->count; i++) {
          OENTRY* e = es->entries[i];
          if (e && e->intypes && strcmp(e->intypes, ".c.") == 0) { op->markup = e; break; }
        }
        csound->Free(csound, es);
      }
      *anchor = append_to_tree(csound, *anchor, op);
    }

    cell = cell->next; midx++;
    argNode = argNode->next;
  }

  return 1;
}


/* Forward declarations */
static int expand_nested_struct_member_assignment(CSOUND* csound,
                                                 TREE* current,
                                                 TYPE_TABLE* typeTable,
                                                 TREE** anchor);

/* Handle struct member assignment (e.g., struct.member = value) */
int32_t expand_struct_member_assignment(CSOUND* csound,
                                        TREE* current,
                                        TYPE_TABLE* typeTable,
                                          TREE** anchor)
{
  if (!current || !current->left || current->left->type != STRUCT_EXPR) {
    return 0; // Not a struct member assignment
  }

  TREE* structExpr = current->left;
  TREE* valueExpr = current->right;

  // Check for nested struct member assignment (e.g., shape1.center.x = value)
  if (structExpr->left && structExpr->left->type == STRUCT_EXPR) {
    // This is a nested struct member assignment like shape1.center.x = 42
    // We need to handle this by creating a chain of member accesses
    return expand_nested_struct_member_assignment(csound, current, typeTable, anchor);
  }

  // Create ##member_set opcode for simple struct member assignment
  TREE* op = create_opcode_token(csound, "##member_set");

  // Set up arguments: ##member_set struct, memberIndex, value
  op->right = copy_node(csound, structExpr->left);  // struct variable (point1)

  // Get member index
  if (structExpr->right && structExpr->right->value && structExpr->right->value->lexeme) {
    const char* memberName = structExpr->right->value->lexeme;

    // Look up member index in the struct type
    char* structTypeName = get_arg_type2(csound, structExpr->left, typeTable);
    if (!structTypeName) return 0;

    const CS_TYPE* structType = csoundGetTypeWithVarTypeName(csound->typePool, structTypeName);
    if (!structType || !structType->userDefinedType) {
      csound->Free(csound, structTypeName);
      return 0;
    }

    // Find member index by iterating through the struct members
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
      return 0; // Member not found
    }

    // Add member index as argument
    char indexBuf[32];
    snprintf(indexBuf, sizeof(indexBuf), "%d", memberIndex);
    TREE* indexNode = make_leaf(csound, current->line, current->locn,
                                INTEGER_TOKEN, make_int(csound, indexBuf));
    op->right->next = indexNode;

    // Add value as argument
    indexNode->next = copy_node(csound, valueExpr);

    csound->Free(csound, structTypeName);
  } else {
    return 0; // No member name
  }

  *anchor = append_to_tree(csound, *anchor, op);
  return 1; // Success
}

/* Handle nested struct member assignment (e.g., shape1.center.x = value) */
static int expand_nested_struct_member_assignment(CSOUND* csound,
                                                 TREE* current,
                                                 TYPE_TABLE* typeTable,
                                                 TREE** anchor)
{
  if (!current || !current->left || current->left->type != STRUCT_EXPR) {
    return 0; // Not a struct member assignment
  }

  TREE* structExpr = current->left;  // e.g., shape1.center.x
  TREE* valueExpr = current->right;  // e.g., 42

  // For nested assignments like shape1.center.x = 42, we need to:
  // 1. Get the intermediate struct member (shape1.center)
  // 2. Set the final member on that intermediate struct (center.x = 42)

  // This is complex and requires temporary variables, so for now we'll
  // generate a sequence of operations that flattens the nested access

  // Create ##member_get to get the intermediate struct (shape1.center)
  TREE* getOp = create_opcode_token(csound, "##member_get");
  getOp->right = copy_node(csound, structExpr->left->left);  // shape1

  // Get the intermediate member index (center)
  if (structExpr->left->right && structExpr->left->right->value &&
      structExpr->left->right->value->lexeme) {
    const char* intermediateMemberName = structExpr->left->right->value->lexeme;

    // Look up intermediate member index
    char* structTypeName = get_arg_type2(csound, structExpr->left->left, typeTable);
    if (!structTypeName) return 0;

    const CS_TYPE* structType = csoundGetTypeWithVarTypeName(csound->typePool, structTypeName);
    if (!structType || !structType->userDefinedType) {
      csound->Free(csound, structTypeName);
      return 0;
    }

    // Find intermediate member index
    int intermediateMemberIndex = -1;
    CONS_CELL* cell = structType->members;
    int i = 0;
    while (cell) {
      CS_VARIABLE* member = (CS_VARIABLE*)cell->value;
      if (member && strcmp(member->varName, intermediateMemberName) == 0) {
        intermediateMemberIndex = i;
        break;
      }
      cell = cell->next;
      i++;
    }

    if (intermediateMemberIndex == -1) {
      csound->Free(csound, structTypeName);
      return 0;
    }

    // Add intermediate member index
    char indexBuf[32];
    snprintf(indexBuf, sizeof(indexBuf), "%d", intermediateMemberIndex);
    TREE* indexNode = make_leaf(csound, current->line, current->locn,
                                INTEGER_TOKEN, make_int(csound, indexBuf));
    getOp->right->next = indexNode;

    csound->Free(csound, structTypeName);
  } else {
    return 0;
  }

  // Add the get operation to the tree
  *anchor = append_to_tree(csound, *anchor, getOp);

  // Now create ##member_set to set the final member (__temp_struct_N.x = 42)
  TREE* setOp = create_opcode_token(csound, "##member_set");
  // Determine the intermediate struct type and use a synthetic out arg
  {
    char* tmpTypeName = get_arg_type2(csound, structExpr->left, typeTable);
    if (!tmpTypeName) return 0;
    char* tmpOutLex = create_out_arg(csound, tmpTypeName,
                                     typeTable->localPool->synthArgCount++, typeTable);
    csound->Free(csound, tmpTypeName);
    // LHS of member_get is the synthetic out arg
    getOp->left = create_ans_token(csound, tmpOutLex);
    // Pass the same temp to member_set as its struct input
    setOp->right = create_ans_token(csound, tmpOutLex);
  }

  // Get the final member index (x)
  if (structExpr->right && structExpr->right->value && structExpr->right->value->lexeme) {
    char indexBuf[32];
    snprintf(indexBuf, sizeof(indexBuf), "0");  // Simplified - assume first member
    TREE* finalIndexNode = make_leaf(csound, current->line, current->locn,
                                    INTEGER_TOKEN, make_int(csound, indexBuf));
    setOp->right->next = finalIndexNode;

    // Add value as argument
    finalIndexNode->next = copy_node(csound, valueExpr);
  } else {
    return 0;
  }

  // Add the set operation to the tree
  *anchor = append_to_tree(csound, *anchor, setOp);

  return 1; // Success
}

/* Handle struct array member assignment (e.g., structArray[0].member = value) */
static int expand_struct_array_member_assignment(CSOUND* csound,
                                                TREE* current,
                                                TYPE_TABLE* typeTable,
                                                TREE** anchor)
{
  if (!current || !current->left || current->left->type != STRUCT_EXPR) {
    return 0; // Not a struct member assignment
  }

  TREE* structExpr = current->left;  // e.g., structArray[0].member
  TREE* valueExpr = current->right;  // e.g., 42

  // Check if this is a struct array member (structArray[0].member)
  if (!structExpr->left || structExpr->left->type != T_ARRAY) {
    return 0; // Not a struct array member
  }

  TREE* arrayExpr = structExpr->left;  // e.g., structArray[0]

  // Create ##member_array_assign opcode
  TREE* op = create_opcode_token(csound, "##member_array_assign");

  // Set up arguments: ##member_array_assign structArray, index, memberIndex, value
  op->right = copy_node(csound, arrayExpr->left);  // struct array variable (structArray)

  // Add array index as argument
  op->right->next = copy_node(csound, arrayExpr->right);  // array index [0]

  // Get member index
  if (structExpr->right && structExpr->right->value && structExpr->right->value->lexeme) {
    const char* memberName = structExpr->right->value->lexeme;

    // Look up member index in the struct type
    char* structTypeName = get_arg_type2(csound, arrayExpr->left, typeTable);
    if (!structTypeName) return 0;

    // Remove array suffix from type name (e.g., "Point[]" -> "Point")
    char* bracketPos = strchr(structTypeName, '[');
    if (bracketPos) *bracketPos = '\0';

    const CS_TYPE* structType = csoundGetTypeWithVarTypeName(csound->typePool, structTypeName);
    if (!structType || !structType->userDefinedType) {
      csound->Free(csound, structTypeName);
      return 0;
    }

    // Find member index by iterating through the struct members
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
      return 0; // Member not found
    }

    // Add member index as argument
    char indexBuf[32];
    snprintf(indexBuf, sizeof(indexBuf), "%d", memberIndex);
    TREE* memberIndexNode = make_leaf(csound, current->line, current->locn,
                                     INTEGER_TOKEN, make_int(csound, indexBuf));
    op->right->next->next = memberIndexNode;

    // Add value as argument
    memberIndexNode->next = copy_node(csound, valueExpr);

    csound->Free(csound, structTypeName);
  } else {
    return 0; // No member name
  }

  *anchor = append_to_tree(csound, *anchor, op);
  return 1; // Success
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

  // Check for struct member assignment BEFORE expression expansion
  // We need to detect this early because expand_expression will flatten STRUCT_EXPR nodes
  if (current->type == '=' || current->type == T_ASSIGNMENT || current->type == S_ADDIN ||
      current->type == S_SUBIN || current->type == S_MULIN || current->type == S_DIVIN) {

    // Check for STRUCT_EXPR before it gets flattened by expand_expression
    if (current->left && current->left->type == STRUCT_EXPR) {
      csound->Message(csound, "[expand_statement] DEBUG: FOUND STRUCT_EXPR assignment - handling specially\n");

      // The right side will be expanded later in the normal flow

      // Check if this is a struct array member assignment (structArray[0].member = value)
      if (current->left->left && current->left->left->type == T_ARRAY) {
        // This is a struct array member assignment like structArray[0].member = 42
        if (expand_struct_array_member_assignment(csound, current, typeTable, &anchor)) {
          append_to_tree(csound, anchor, originalNext);
          return anchor;
        }
      } else {
        // This is a simple struct member assignment like point1.x = 42
        // Generate ##member_set opcode instead of using normal assignment processing
        if (expand_struct_member_assignment(csound, current, typeTable, &anchor)) {
          append_to_tree(csound, anchor, originalNext);
          return anchor;
        }
      }
    }

    // NEW: Check for struct-to-struct assignment (var2:MyType2 = var1)
    else if (current->left && current->right) {
      csound->Message(csound, "[expand_statement] DEBUG: Checking assignment types: left=%d, right=%d (T_IDENT=%d, T_TYPED_IDENT=%d)\n",
                      current->left->type, current->right->type, T_IDENT, T_TYPED_IDENT);

      if ((current->left->type == T_IDENT || current->left->type == T_TYPED_IDENT) &&
          current->right->type == T_IDENT) {
      // Check if both sides are struct types
      char* leftVarName = current->left->value ? current->left->value->lexeme : NULL;
      char* rightVarName = current->right->value ? current->right->value->lexeme : NULL;

      if (leftVarName && rightVarName) {
        csound->Message(csound, "[expand_statement] DEBUG: Checking struct-to-struct assignment: %s = %s\n", leftVarName, rightVarName);

        // Get the types of both variables
        CS_VARIABLE* leftVar = find_var_from_pools(csound, leftVarName, leftVarName, typeTable);
        CS_VARIABLE* rightVar = find_var_from_pools(csound, rightVarName, rightVarName, typeTable);

        if (leftVar && rightVar && leftVar->varType && rightVar->varType &&
            leftVar->varType->userDefinedType && rightVar->varType->userDefinedType &&
            leftVar->varType == rightVar->varType) {

          csound->Message(csound, "[expand_statement] DEBUG: Found struct-to-struct assignment! Converting to ##struct_alias opcode\n");

          // Convert assignment to ##struct_alias opcode: var2:MyType2 = var1 -> ##struct_alias var2, var1
          current->value->lexeme = cs_strdup(csound, "##struct_alias");
          current->type = T_OPCALL;

          // The left side becomes the output argument
          // The right side becomes the input argument
          // No need to change the tree structure, just the opcode name and type

          csound->Message(csound, "[expand_statement] DEBUG: Converted to ##struct_alias opcode\n");
        }
      }
    }
    }

    // Also check for any assignment that might be a struct assignment
    csound->Message(csound, "[expand_statement] DEBUG: Assignment details - left type=%d, right type=%d\n",
                    current->left ? current->left->type : -1, current->right ? current->right->type : -1);
    if (current->left && current->left->value && current->left->value->lexeme) {
      csound->Message(csound, "[expand_statement] DEBUG: Left lexeme='%s'\n", current->left->value->lexeme);
    }
    if (current->right && current->right->value && current->right->value->lexeme) {
      csound->Message(csound, "[expand_statement] DEBUG: Right lexeme='%s'\n", current->right->value->lexeme);
    } else {
      csound->Message(csound, "[expand_statement] DEBUG: Not a struct member assignment - left type=%d\n",
                      current->left ? current->left->type : -1);
    }
  }

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

      /* Set as anchor if necessary */
      anchor = append_to_tree(csound, anchor, expressionNodes);

      /* reconnect into chain */
      last = tree_tail(expressionNodes);
      if (last == NULL || last->left == NULL || last->left->value == NULL || last->left->value->lexeme == NULL) {
        csound->Message(csound, "[expand_statement] ERROR: Invalid tree structure - last=%p", (void*)last);
        if (last) {
          csound->Message(csound, ", last->left=%p", (void*)last->left);
          if (last->left) {
            csound->Message(csound, ", last->left->value=%p", (void*)last->left->value);
          }
        }
        csound->Message(csound, "\n");
        return 0;
      }
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

  if (current->type == T_OPCALL &&
      current->left && current->left->type == T_IDENT &&
      current->value && current->value->lexeme &&
      strcmp(current->value->lexeme, current->left->value->lexeme) == 0 &&
      current->right) {
    current->value = make_token(csound, "init");
  }

  // Run struct lowering before appending `current`
  if (current->type == T_OPCALL &&
      ((current->value && current->value->lexeme &&
        strcmp(current->value->lexeme, "init") == 0) ||
       (current->left && current->left->type == T_OPCALL))) { // legacy nested form
    if (expand_structs(csound, current, typeTable, &anchor)) {
      append_to_tree(csound, anchor, originalNext);
      return anchor; // skip LHS array-set pass and the original node
    }
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
        if (var->varType == &CS_VAR_TYPE_ARRAY) {
          outType = strdup(var->subType->varTypeName);
        } else if (var->varType == &CS_VAR_TYPE_A) {
          outType = "k";
        } else {
          synterr(csound,
                  Str("invalid array type %s line %d\n"),
                  var->varType->varTypeName, current->line);
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
                  cs_strdup(csound,
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
                                     cs_strdup(csound,
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
  // Jump when condition is true: use cingoto (i-rate) and ckgoto (k-rate)
  TREE* cgotoOperator = create_opcode_token(csound, isPerfRate ? "ckgoto" : "cingoto");
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
    cs_strdup(csound, endLabel->value->lexeme),
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
          cs_strdup(csound, caseLabel->value->lexeme),
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

        if (caseNode->right != NULL) {
          tempNext = caseNode->right->next;
          gotoChainTailAnchor = append_to_tree(csound, gotoChainTailAnchor, caseNode->right);
          gotoChainTailAnchor->next->next = NULL;
          gotoChainTailAnchor = append_to_tree(
            csound,
            gotoChainTailAnchor,
            copy_node(csound, endGoto)
          );
        } else {
          tempNext = NULL;
        }
    } else if (caseNode->type == DEFAULT_TOKEN && gotoChainHeadDefaultCase == NULL) {
      gotoChainHeadDefaultCase = create_goto_node(csound, isPerfRate);
      defaultCaseLabel = create_synthetic_label(csound, csound->genlabs++);
      typeTable->labelList = cs_cons(
        csound,
        cs_strdup(csound, defaultCaseLabel->value->lexeme),
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
  append_to_tree(
    csound,
    gotoChainHeadAnchor,
    copy_node(csound, endLabel)
  );

  return gotoChainHead;
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

  int32 topLabelCounter = csound->genlabs++;
  int32 endLabelCounter = csound->genlabs++;
  TREE* tempRight = current->right;
  TREE* last = NULL;
  TREE* labelEnd;
  int32_t gotoType;

  anchor = create_synthetic_label(csound, topLabelCounter);
  typeTable->labelList = cs_cons(csound,
                                 cs_strdup(csound, anchor->value->lexeme),
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
                                 cs_strdup(csound, labelEnd->value->lexeme),
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
  TREE *topLabel = create_synthetic_ident(csound,
                                          topLabelCounter);
  TREE *gotoTopLabelToken = create_simple_goto_token(csound,
                                                     topLabel,
                                                     (gotoType==1 ? 0 : 1));

  append_to_tree(csound, last, gotoTopLabelToken);
  gotoTopLabelToken->next = labelEnd;


  labelEnd->next = current->next;
  return anchor;
}

TREE* expand_for_statement(CSOUND* csound, TREE* current, TYPE_TABLE* typeTable,
                           char* arrayArgType) {

  const CS_TYPE *iType = &CS_VAR_TYPE_I;
  const CS_TYPE *kType = &CS_VAR_TYPE_K;
  const CS_TYPE *aType = &CS_VAR_TYPE_A;
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
    cs_cons(csound, cs_strdup(csound, loopLabel->value->lexeme),
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
  // VL: need to set the next statement after loop
  loopLtStatement->next = current->next;

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


// Full chain flattener for struct+array access chains
static TREE* flatten_struct_array_chain(CSOUND* csound, TREE* root, int line, int locn,
                                        TYPE_TABLE* typeTable, TREE** anchor) {

  if (!root || root->type != STRUCT_EXPR) return NULL;

  printf("[FLATTEN] DEBUG: Processing STRUCT_EXPR node\n");
  if (root->left && root->left->value && root->left->value->lexeme) {
    printf("[FLATTEN] DEBUG: Left side (struct): '%s'\n", root->left->value->lexeme);
  }
  if (root->right && root->right->value && root->right->value->lexeme) {
    printf("[FLATTEN] DEBUG: Right side (member): '%s'\n", root->right->value->lexeme);
  }

  // Step 1: Normalize the base (leftmost) expression to a struct temp
  TREE* baseInput = NULL;
  char* baseTypeName = NULL;

  if (root->left) {
    // Avoid mutual recursion: do NOT lower the left subtree here.
    // Determine the base input and type directly from the original subtree.
    baseInput = copy_node(csound, root->left);
    baseTypeName = get_arg_type2(csound, root->left, typeTable);
    if (!baseTypeName) return NULL;
  } else {
    return NULL;
  }

  printf("DEBUG: FLATTEN base normalized, type=%s\n", baseTypeName);
  if (baseTypeName && strstr(baseTypeName, "::")) {
    printf("DEBUG: FLATTEN DOUBLE-ENCODING DETECTED! baseTypeName='%s'\n", baseTypeName);
  }

  // Step 2: Parse the chain and emit operations
  TREE* chainHead = NULL;
  TREE* chainTail = NULL;
  char* currentStructOut = NULL; // ANS name of current struct temp
  char* currentTypeName = baseTypeName; // current struct type name
  TREE* cursor = root;

  while (cursor) {
    printf("DEBUG: FLATTEN processing node type=%d\n", cursor->type);

    if (cursor->type != STRUCT_EXPR) {
      // If cursor is not STRUCT_EXPR, it might be the final member token
      printf("DEBUG: FLATTEN non-STRUCT_EXPR node, checking if final member\n");
      break;
    }

    // Check if this is an array member access.
    // Two shapes we support:
    //  (A) left = T_ARRAY with base STRUCT_EXPR (e.g., (john.relativeList)[0].field)
    //  (B) left = T_ARRAY with base IDENT and right is member token (e.g., john.relativeList[0].field)
    if (cursor->left && cursor->left->type == T_ARRAY) {

      TREE* arrayNode = cursor->left;

      const char* memberName = NULL;
      TREE* memberExprNode = NULL; // Optional STRUCT_EXPR describing the member

      if (arrayNode->left && arrayNode->left->type == STRUCT_EXPR) {
        // Shape A
        memberExprNode = arrayNode->left;
        memberName = (memberExprNode->right && memberExprNode->right->value) ?
                     memberExprNode->right->value->lexeme : "";
      } else if (cursor->right && cursor->right->value && cursor->right->value->lexeme) {
        // Shape B
        memberName = cursor->right->value->lexeme;
      }

      if (memberName == NULL) break;

      // printf("DEBUG: FLATTEN array member access: %s\n", memberName);

      // Emit member_get(array) to get the array member
      char* arrayMemberType = NULL;
      if (memberExprNode) {
        arrayMemberType = get_arg_type2(csound, memberExprNode, typeTable);
      } else {
        // Directly query the struct type for the member type instead of creating fake tokens
        const CS_TYPE* structType = csoundGetTypeWithVarTypeName(csound->typePool, currentTypeName);
        if (structType && structType->members) {
          CONS_CELL* cell = structType->members;
          while (cell) {
            CS_VARIABLE* member = (CS_VARIABLE*)cell->value;
            if (member && member->varName && !strcmp(member->varName, memberName)) {
              if (member->varType == &CS_VAR_TYPE_ARRAY) {
                arrayMemberType = create_array_arg_type(csound, member);
              } else {
                arrayMemberType = cs_strdup(csound, member->varType->varTypeName);
              }
              break;
            }
            cell = cell->next;
          }
        }
      }
      if (arrayMemberType) {
        // printf("DEBUG: arrayMemberType for '%s' = %s\n", memberName, arrayMemberType);
      }
      if (!arrayMemberType) break;

      TREE* memberGetOp = create_opcode_token(csound, "##member_get");
      char* arrayOutName = create_out_arg(csound, arrayMemberType,
                                         typeTable->localPool->synthArgCount++, typeTable);
      memberGetOp->left = create_ans_token(csound, arrayOutName);

      // Find member index in current struct type
      const char* tname = currentTypeName;
      char tbufTrim[256];
      if (tname && tname[0] == ':') {
        size_t len = strlen(tname);
        if (len >= 2 && tname[len-1] == ';') {
          size_t n = len - 2; if (n >= sizeof(tbufTrim)) n = sizeof(tbufTrim)-1;
          memcpy(tbufTrim, tname+1, n); tbufTrim[n] = '\0';
          tname = tbufTrim;
        }
      }
      CS_TYPE* structType = csoundGetTypeWithVarTypeName(csound->typePool, tname);
      int memberIndex = 0;

      if (structType) {
        csound->Message(csound, "DEBUG: structType members for %s: ", currentTypeName ? currentTypeName : "(null)");
        CONS_CELL* cc = structType->members;
        int i = 0;
        int printed = 0;
        while (cc) {
          CS_VARIABLE* mv = (CS_VARIABLE*)cc->value;
          csound->Message(csound, "%s%s", printed ? ", " : "", mv && mv->varName ? mv->varName : "(null)");
          printed = 1;
          if (!strcmp(mv->varName, memberName)) {
            memberIndex = i;

          }
          i++;
          cc = cc->next;
        }
        csound->Message(csound, "\n");
      }

      printf("DEBUG: FLATTEN memberIndex for '%s' in type %s = %d\n", memberName, currentTypeName ? currentTypeName : "(null)", memberIndex);
      char indexBuf[32];
      snprintf(indexBuf, sizeof(indexBuf), "%d", memberIndex);
      TREE* indexNode = make_leaf(csound, line, locn, INTEGER_TOKEN, make_int(csound, indexBuf));

      // Decide if the member itself is an array
      int isArrayMember = (arrayMemberType && strstr(arrayMemberType, "[]") != NULL);
      printf("DEBUG: isArrayMember decision for '%s': isArrayMember=%d (arrayMemberType=%s)\n",
             memberName ? memberName : "(null)", isArrayMember,
             arrayMemberType ? arrayMemberType : "(null)");


      char* elemTypeName = get_arg_type2(csound, arrayNode, typeTable);
      if (!elemTypeName) {
        // Fallback: derive element type from the array variable itself
        const char* arrName = (arrayNode->left && arrayNode->left->value) ? arrayNode->left->value->lexeme : NULL;
        if (arrName) {
          CS_VARIABLE* aVar = find_var_from_pools(csound, (char*)arrName, (char*)arrName, typeTable);
          if (aVar && aVar->subType && aVar->subType->varTypeName) {
            char tbuf[256];
            cs_sprintf(tbuf, ":%s;", aVar->subType->varTypeName);
            elemTypeName = csound->Strdup(csound, tbuf);
          }
        }
      }
      if (!elemTypeName) break;

      if (isArrayMember) {
        // Set up arguments: struct input, member index
        TREE* structInTok = currentStructOut ? create_ans_token(csound, currentStructOut) :
                           copy_node(csound, baseInput);
        memberGetOp->right = structInTok;
        structInTok->next = indexNode;

        // Emit array_get_struct to get the struct element from the array member
        TREE* arrayGetOp = create_opcode_token(csound, "##array_get_struct");
        char* structOutName = create_out_arg(csound, elemTypeName,
                                            typeTable->localPool->synthArgCount++, typeTable);
        arrayGetOp->left = create_ans_token(csound, structOutName);
        arrayGetOp->right = create_ans_token(csound, arrayOutName);
        arrayGetOp->right->next = copy_node(csound, arrayNode->right); // array index



        // Add to chain: member_get then array_get_struct
        if (!chainHead) {
          chainHead = memberGetOp;
          chainTail = memberGetOp;
        } else {
          chainTail->next = memberGetOp;
          chainTail = memberGetOp;
        }
        chainTail->next = arrayGetOp;
        chainTail = arrayGetOp;

        // Update state for next iteration
        if (currentStructOut) csound->Free(csound, currentStructOut);
        currentStructOut = csound->Strdup(csound, structOutName);
        if (currentTypeName) csound->Free(csound, currentTypeName);
        currentTypeName = elemTypeName; // struct element type name

        // Move to the right side for next step
        cursor = cursor->right;
        printf("DEBUG: FLATTEN moved to cursor->right, type=%d\n", cursor ? cursor->type : -1);
        continue;
      } else {
        // Scalar member: direct struct element fetch from the base array
        TREE* arrayGetOp = create_opcode_token(csound, "##array_get_struct");
        char* structOutName = create_out_arg(csound, elemTypeName,
                                            typeTable->localPool->synthArgCount++, typeTable);
        arrayGetOp->left = create_ans_token(csound, structOutName);
        arrayGetOp->right = copy_node(csound, arrayNode->left); // the array variable (e.g., relatives)
        arrayGetOp->right->next = copy_node(csound, arrayNode->right); // array index

        printf("DEBUG: FLATTEN emitted array_get_struct (scalar member path)\n");

        // Add to chain: only array_get_struct now
        if (!chainHead) {
          chainHead = arrayGetOp;
          chainTail = arrayGetOp;
        } else {
          chainTail->next = arrayGetOp;
          chainTail = arrayGetOp;
        }

        // Update state for next iteration
        if (currentStructOut) csound->Free(csound, currentStructOut);
        currentStructOut = csound->Strdup(csound, structOutName);
        if (currentTypeName) csound->Free(csound, currentTypeName);
        currentTypeName = elemTypeName; // struct element type name

        // Move to the right side for next step
        cursor = cursor->right;
        printf("DEBUG: FLATTEN moved to cursor->right, type=%d\n", cursor ? cursor->type : -1);
        continue;
      }
    }

    // Prefer descending into left STRUCT_EXPR first (handles cases like (a.b[0].c).d)
    if (cursor->left && cursor->left->type == STRUCT_EXPR) {
      printf("DEBUG: FLATTEN descending into left STRUCT_EXPR\n");
      cursor = cursor->left;
      continue;
    }

    // If right is another STRUCT_EXPR, descend into it
    if (cursor->right && cursor->right->type == STRUCT_EXPR) {
      printf("DEBUG: FLATTEN descending into right STRUCT_EXPR\n");
      cursor = cursor->right;
      continue;
    }

    printf("DEBUG: FLATTEN breaking - no more processing possible\n");
    break; // No more processing possible
  }

  // Check for final scalar member access after breaking out of the loop
  const char* memberName = NULL;
  if (cursor && cursor->right && cursor->right->value) {
    memberName = cursor->right->value->lexeme;
  } else if (cursor && cursor->value) {
    // cursor itself is the final member name token
    memberName = cursor->value->lexeme;
  }

  printf("DEBUG: FLATTEN checking memberName: cursor=%p cursor->right=%p cursor->value=%p memberName=%s\n",
         cursor, cursor ? cursor->right : NULL, cursor ? cursor->value : NULL, memberName ? memberName : "(null)");

  if (memberName) {
    printf("DEBUG: FLATTEN final scalar member: %s\n", memberName);

    // Emit final member_get(scalar)
    char* finalOutType = get_arg_type2(csound, root, typeTable);
    if (finalOutType) {
      char* outarg = create_out_arg(csound, finalOutType,
                                   typeTable->localPool->synthArgCount++, typeTable);
      csound->Free(csound, finalOutType);

      // Create rate-specific ##member_get variant based on target variable rate
      char memberGetOpName[32];
      char targetRate = 'k'; // default to k-rate
      if (outarg && strlen(outarg) > 1) {
        // Extract rate from output argument name (e.g., "#i0" -> 'i', "#k1" -> 'k')
        // The rate character is the second character in synthetic argument names
        targetRate = outarg[1];
      }

      printf("[FLATTEN DEBUG] outarg='%s', targetRate='%c'\n", outarg ? outarg : "NULL", targetRate);

      if (strchr("ikaSab", targetRate)) {
        // Use rate-specific variant like ##member_get.i, ##member_get.k, etc.
        snprintf(memberGetOpName, sizeof(memberGetOpName), "##member_get.%c", targetRate);
      } else {
        // For unknown rates, use generic variant
        strcpy(memberGetOpName, "##member_get");
      }

      printf("[FLATTEN DEBUG] Using opcode: %s\n", memberGetOpName);

      TREE* finalMemberGet = create_opcode_token(csound, memberGetOpName);
      finalMemberGet->left = create_ans_token(csound, outarg);

      // Find member index
      CS_TYPE* structType = csoundGetTypeWithVarTypeName(csound->typePool, currentTypeName);
      int memberIndex = 0;
      if (structType) {
        CONS_CELL* cc = structType->members;
        int i = 0;
        while (cc) {
          CS_VARIABLE* mv = (CS_VARIABLE*)cc->value;
          if (!strcmp(mv->varName, memberName)) {
            memberIndex = i;
            break;
          }
          i++;
          cc = cc->next;
        }
      }

      char indexBuf[32];
      snprintf(indexBuf, sizeof(indexBuf), "%d", memberIndex);
      TREE* indexNode = make_leaf(csound,
                                 (cursor && cursor->right) ? cursor->right->line : line,
                                 (cursor && cursor->right) ? cursor->right->locn : locn,
                                 INTEGER_TOKEN, make_int(csound, indexBuf));

      // Ensure we have a struct value (not an array) as input to member_get
      if (!currentStructOut) {
        TREE* tryArray = NULL;
        if (root->left) {
          if (root->left->type == T_ARRAY) tryArray = root->left;
          else if (root->left->type == STRUCT_EXPR && root->left->left && root->left->left->type == T_ARRAY)
            tryArray = root->left->left;
        }
        if (tryArray) {
          char* elemTypeName2 = get_arg_type2(csound, tryArray, typeTable);
          if (!elemTypeName2) {
            const char* arrName2 = (tryArray->left && tryArray->left->value) ? tryArray->left->value->lexeme : NULL;
            if (arrName2) {
              CS_VARIABLE* aVar2 = find_var_from_pools(csound, (char*)arrName2, (char*)arrName2, typeTable);
              if (aVar2 && aVar2->subType && aVar2->subType->varTypeName) {
                char tbuf2[256];
                cs_sprintf(tbuf2, ":%s;", aVar2->subType->varTypeName);
                elemTypeName2 = csound->Strdup(csound, tbuf2);
              }
            }
          }
          if (elemTypeName2) {
            TREE* arrayGetOp2 = create_opcode_token(csound, "##array_get_struct");
            char* structOutName2 = create_out_arg(csound, elemTypeName2,
                                                  typeTable->localPool->synthArgCount++, typeTable);
            arrayGetOp2->left = create_ans_token(csound, structOutName2);
            arrayGetOp2->right = copy_node(csound, tryArray->left);
            arrayGetOp2->right->next = copy_node(csound, tryArray->right);
            if (!chainHead) { chainHead = arrayGetOp2; chainTail = arrayGetOp2; }
            else { chainTail->next = arrayGetOp2; chainTail = arrayGetOp2; }
            if (currentTypeName) csound->Free(csound, currentTypeName);
            currentTypeName = elemTypeName2;
            currentStructOut = csound->Strdup(csound, structOutName2);

          }
        }
      }

      TREE* structInTok = currentStructOut ? create_ans_token(csound, currentStructOut) :
                         copy_node(csound, baseInput);
      finalMemberGet->right = structInTok;
      structInTok->next = indexNode;

      // Add to chain and return
      if (!chainHead) {

        return finalMemberGet;
      } else {
        chainTail->next = finalMemberGet;
        printf("DEBUG: FLATTEN returning full chain\n");
        return chainHead;
      }
    }
  }

  printf("DEBUG: FLATTEN failed to complete chain\n");
  // Cleanup on failure
  if (currentStructOut) csound->Free(csound, currentStructOut);
  if (currentTypeName != baseTypeName) csound->Free(csound, currentTypeName);
  if (baseTypeName) csound->Free(csound, baseTypeName);
  return NULL;
}
