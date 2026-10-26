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
    Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/

#include "csoundCore.h"
#include "csound_orc.h"
#include "csound_orc_semantics.h"
#include "csound_orc_structs.h"
#include "csound_standard_types.h"
#include "aops.h"
#include "csound_orc_optimize.h"

static int32_t copy_leaf(const TREE *arg)
{
    return arg && arg->value && arg->value->lexeme &&
           !arg->left && !arg->right;
}

/* Do not search parent/global pools. A local name with an explicit type need
   not start with 'i', so use its resolved type rather than its spelling. */
static int32_t copy_local_scalar(CSOUND *csound, CS_VAR_POOL *pool, TREE *arg)
{
    if (!copy_leaf(arg) || arg->type != T_IDENT || !pool) return 0;
    CS_VARIABLE *var = cs_hash_table_get(csound, pool->table, arg->value->lexeme);
    return var && var->varType == &CS_VAR_TYPE_I && var->dimensions == 0;
}

static int32_t copy_scalar_input(CSOUND *csound, CS_VAR_POOL *pool, TREE *arg)
{
    return copy_leaf(arg) && (arg->type == INTEGER_TOKEN ||
           arg->type == NUMBER_TOKEN || is_pfield_name(arg->value->lexeme) ||
           copy_local_scalar(csound, pool, arg));
}

static void count_copy_references(CSOUND *csound, CS_HASH_TABLE *uses, TREE *args)
{
    for (; args; args = args->next) {
      if (args->value && args->value->lexeme &&
          !strncmp(args->value->lexeme, "#i", 2)) {
        int32_t *count = cs_hash_table_get(csound, uses, args->value->lexeme);
        if (!count) {
          count = csound->Calloc(csound, sizeof(*count));
          cs_hash_table_put(csound, uses, args->value->lexeme, count);
        }
        ++*count;
      }
      count_copy_references(csound, uses, args->left);
      count_copy_references(csound, uses, args->right);
    }
}

/* This runs after overload resolution. The earlier expression-expansion helper
   still handles strings; extending it to scalars would bypass these checks. */
static void remove_scalar_copies(CSOUND *csound, TREE *root)
{
    for (TREE *scope = root; scope; scope = scope->next) {
      /* UDO arguments may alias caller storage. Leave UDOs and instr 0 alone. */
      if (scope->type != INSTR_TOKEN || !scope->markup) continue;
      CS_VAR_POOL *pool = scope->markup;
      CS_HASH_TABLE *uses = cs_hash_table_create(csound);
      for (TREE *node = scope->right; node; node = node->next) {
        count_copy_references(csound, uses, node->left);
        count_copy_references(csound, uses, node->right);
      }
      for (TREE *node = scope->right; node && node->next; node = node->next) {
        TREE *copy = node->next;
        OENTRY *producer = node->markup, *consumer = copy->markup;
        TREE *temp = node->left, *dest = copy->left;
        if (node->type != T_OPCALL || copy->type != T_ASSIGNMENT ||
            !producer || !consumer || producer->perf || producer->deinit ||
            consumer->perf || consumer->deinit ||
            (consumer->init != (SUBR)assign && consumer->init != (SUBR)minit) ||
            !producer->outypes || strcmp(producer->outypes, "i") ||
            !producer->intypes || strcmp(producer->intypes, "ii") ||
            (producer->init != (SUBR)addkk && producer->init != (SUBR)subkk &&
             producer->init != (SUBR)mulkk) ||
            !copy_local_scalar(csound, pool, temp) || temp->next ||
            strncmp(temp->value->lexeme, "#i", 2) ||
            !copy_local_scalar(csound, pool, dest) || dest->next ||
            dest->value->lexeme[0] == '#' ||
            !copy_leaf(copy->right) || copy->right->next ||
            strcmp(temp->value->lexeme, copy->right->value->lexeme) ||
            !copy_scalar_input(csound, pool, node->right) ||
            !copy_scalar_input(csound, pool, node->right->next) ||
            node->right->next->next) continue;
        int32_t *count = cs_hash_table_get(csound, uses, temp->value->lexeme);
        if (!count || *count != 2) continue; /* one definition, one read */

        /* These three built-ins read both operands before storing the result,
           including iValue = iValue + 1. Keep the producer's resolved callback
           and location and the destination leaf's metadata. Only its output
           binding changes; the unused temporary's pool slot stays allocated. */
        node->left = dest;
        copy->left = NULL;
        node->next = copy->next;
        copy->next = NULL;
        csoundDeleteTree(csound, temp);
        csoundDeleteTree(csound, copy);
      }
      cs_hash_table_mfree_complete(csound, uses);
    }
}
#include "entry.h"

/* Only these resolved built-ins may produce a folded value. Comparing callback
   identities, rather than names, keeps plugin and UDO overloads out. */
static int32_t literal_operator(const TREE *node)
{
    if (node->type != T_OPCALL || !node->markup) return 0;
    const OENTRY *op = node->markup;
    if (op->perf || op->deinit || !op->outypes || !op->intypes ||
        strcmp(op->outypes, "i") || strcmp(op->intypes, "ii")) return 0;
    if (op->init == (SUBR)addkk) return '+';
    if (op->init == (SUBR)subkk) return '-';
    if (op->init == (SUBR)mulkk) return '*';
    return 0;
}

/* A literal must not replace writable storage passed to an unknown opcode.
   Keep both producer and consumer in the same init-only straight-line block.
   In particular, do not move an init result into a performance-time read. */
static int32_t literal_consumer(const TREE *node)
{
    if (literal_operator(node)) return 1;
    if ((node->type != T_OPCALL && node->type != T_ASSIGNMENT) ||
        !node->markup) return 0;
    const OENTRY *op = node->markup;
    return (op->init == (SUBR)assign || op->init == (SUBR)minit) &&
           !op->perf && !op->deinit;
}

typedef struct {
    TREE *first, *last;
    int32_t count, firstBlock, lastBlock, input;
} LITERAL_USE;

static void count_literal_uses(CSOUND *csound, CS_HASH_TABLE *uses,
                               TREE *args, int32_t block, int32_t input)
{
    for (; args; args = args->next) {
      if (args->value && !strncmp(args->value->lexeme, "#i", 2)) {
        char *name = args->value->lexeme;
        LITERAL_USE *use = cs_hash_table_get(csound, uses, name);
        if (!use) {
          use = csound->Calloc(csound, sizeof(*use));
          use->first = args;
          use->firstBlock = block;
          cs_hash_table_put(csound, uses, name, use);
        }
        use->count++;
        use->last = args;
        use->lastBlock = block;
        use->input = input;
      }
      /* Count nested references too, but never replace them as plain scalars. */
      count_literal_uses(csound, uses, args->left, block, 0);
      count_literal_uses(csound, uses, args->right, block, 0);
    }
}

static int32_t literal_value(TREE *arg, cs_float *value)
{
    if (!arg || !arg->value || arg->left || arg->right ||
        (arg->type != INTEGER_TOKEN && arg->type != NUMBER_TOKEN)) return 0;
    char *end;
    /* Match the conversion used by the runtime constant pool, including large
       integer spellings that do not fit ORCTOKEN's integer value member. */
    cs_double parsed = csoundStrtod(arg->value->lexeme, &end);
    *value = (cs_float)parsed;
    if (*value == FL(0.0)) {
      /* csoundStrtod itself returns sample precision in a pure-float build.
         Accept an explicit zero, not a nonzero mantissa that underflowed. */
      const char *tail = arg->value->lexeme + strspn(arg->value->lexeme, "+-0.");
      if (*tail != '\0' && *tail != 'e' && *tail != 'E') return 0;
    }
    return *end == '\0' && isfinite(*value) &&
           (*value == FL(0.0) || isnormal(*value));
}

static TREE *fold_scalar_literals(CSOUND *csound, TREE *root, CS_VAR_POOL *pool)
{
    CS_HASH_TABLE *uses = cs_hash_table_create(csound);
    int32_t block = 0;
    for (TREE *node = root; node; node = node->next) {
      if (node->type == INSTR_TOKEN || node->type == UDO_TOKEN) {
        node->right = fold_scalar_literals(csound, node->right, node->markup);
        ++block;
        continue;
      }
      int32_t safe = literal_consumer(node);
      if (!safe) ++block;
      count_literal_uses(csound, uses, node->left, block, 0);
      count_literal_uses(csound, uses, node->right, block, safe);
      if (!safe) ++block;
    }

    TREE **link = &root;
    while (*link) {
      TREE *node = *link;
      int32_t operation = literal_operator(node);
      TREE *out = node->left, *left = node->right;
      TREE *right = left ? left->next : NULL;
      cs_float a, b, result;
      if (!operation || !out || !out->value || out->next || out->left ||
          out->right || !right || right->next ||
          !literal_value(left, &a) || !literal_value(right, &b)) goto next;
      LITERAL_USE *use = cs_hash_table_get(csound, uses, out->value->lexeme);
      if (!use || use->count != 2 || use->first != out || !use->input ||
          use->firstBlock != use->lastBlock || use->last->left ||
          use->last->right) goto next;
      CS_VARIABLE *var = pool ? csoundFindVariableWithName(csound, pool,
                                                         out->value->lexeme) : NULL;
      if (!var || var->varType != &CS_VAR_TYPE_I) goto next;

      /* Each operation rounds to sample precision, just as its runtime callback
         does. Do not reassociate a chain or combine multiply and add. */
      if (operation == '+') result = a + b;
      else if (operation == '-') result = a - b;
      else result = a * b;
      if (!isfinite(result) || (result != FL(0.0) && !isnormal(result))) goto next;
      if (operation == '*' && a != FL(0.0) && b != FL(0.0) && result == FL(0.0))
        goto next;
      char text[64];
      snprintf(text, sizeof(text), sizeof(cs_float) == 4 ? "%.9g" : "%.17g",
               result);
      cs_float roundTrip = (cs_float)csoundStrtod(text, NULL);
      if (memcmp(&result, &roundTrip, sizeof(result))) goto next;

      /* Retain the use node's source location and siblings and the consumer's
         resolved opcode. The now-unused variable slot stays for a later pass. */
      TREE *arg = use->last;
      csound->Free(csound, arg->value->lexeme);
      csound->Free(csound, arg->value);
      arg->value = make_num(csound, text, NULL);
      arg->type = NUMBER_TOKEN;
      arg->markup = NULL;
      *link = node->next;
      node->next = NULL;
      csoundDeleteTree(csound, node);
      continue;
next:
      link = &node->next;
    }
    cs_hash_table_mfree_complete(csound, uses);
    return root;
}

static TREE * create_fun_token(CSOUND *csound, TREE *right, char *fname)
{
    TREE *ans;
    ans = (TREE*)csound->Malloc(csound, sizeof(TREE));
    if (UNLIKELY(ans == NULL)) exit(1);
    ans->type = T_FUNCTION;
    ans->value = make_token(csound, fname, NULL);
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
    if (root->right) {
      if (root->type == T_OPCALL) {
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
/*
static inline int32_t same_type(char *var, char ty)
{
    if (var[0]=='g') return var[1]==ty;
    else return var[0]==ty;
}
*/


#define PARSER_DEBUG1 (0)
//static TREE* remove_excess_assigns(CSOUND *csound, TREE* root)
//{
//    TREE* current = root;
//    //print_tree(csound, "AssignTest", root);
//    while (current) {
//      if (PARSER_DEBUG1) printf("in loop: current->type = %d\n", current->type);
//      if ((current->type == T_OPCALL || current->type == T_ASSIGNMENT) &&
//          current->left != NULL &&
//          //current->right != NULL &&  no one looks at current->right
//          current->left->value->lexeme[0]=='#') {
//        TREE *nxt = current->next;
//        if (PARSER_DEBUG1) {
//          printf("****passes test1 %s type =%d\n",
//                 current->left->value->lexeme, current->type);
//          printf("next type = %d; lexeme %s\n",
//                 nxt->type, nxt->right->value->lexeme);
//        }
//        if (PARSER_DEBUG1) printf("test3: %c%c %c\n",
//                   nxt->left->value->lexeme[0], nxt->left->value->lexeme[1],
//                   nxt->right->value->lexeme[1]);
//        if (nxt && nxt->type == T_ASSIGNMENT /* '=' */ &&
//            nxt->left != NULL &&
//            !strcmp(current->left->value->lexeme,nxt->right->value->lexeme) &&
//            same_type(nxt->left->value->lexeme, nxt->right->value->lexeme[1])) {
//          if (PARSER_DEBUG1) {
//            printf("passes test2\n");
//            print_tree(csound, "optimise assignment\n", current);
//          }
//          csound->Free(csound, current->left->value);
//          current->left->value = nxt->left->value;
//          current->next = nxt->next;
//          csound->Free(csound,nxt);
//          if (PARSER_DEBUG1) print_tree(csound, "change to\n", current);
//        }
//      }
//      else {                    /* no need to check for NULL */
//          current->right = remove_excess_assigns(csound, current->right);
//          current->left = remove_excess_assigns(csound, current->left);
//      }
//      current = current->next;
//    }
//    return root;
//}

/* Historical, unused parser-level folder. Keep it disabled: unlike the resolved
   scalar pass above, it cannot check operator overloads, reads integer
   token values rather than their original spelling, and changes division
   diagnostics and bitwise evaluation through broader algebraic rewrites. */
TREE* constant_fold(CSOUND *csound, TREE* root)
{
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
          cs_float lval, rval;
          char buf[64];
          lval = (current->left->type == INTEGER_TOKEN ?
                  (cs_double)current->left->value->value :
                  current->left->value->fvalue);
          rval = (current->right->type == INTEGER_TOKEN ?
                  (cs_double)current->right->value->value :
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
            lval = (cs_float)(((int)lval)|((int)rval));
            break;
          case '&':
            lval = (cs_float)(((int)lval)&((int)rval));
            break;
          case '#':
            lval = (cs_float)(((int)lval)^((int)rval));
            break;
          case S_BITSHIFT_LEFT:
            lval = (cs_float)(((int)lval)<<((int)rval));
            break;
          case S_BITSHIFT_RIGHT:
            lval = (cs_float)(((int)lval)>>((int)rval));
            break;
          }
          //printf("ans = %g\n", lval);
          current->value = current->left->value;
          current->type = NUMBER_TOKEN;
          current->value->fvalue = lval;
          snprintf(buf, 60, "%.20g", lval);
          csound->Free(csound, current->value->lexeme);
          current->value->lexeme = csoundParserStrdup(csound, buf);
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
                csoundDeleteTree(csound, tmp);
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
                csoundDeleteTree(csound,current->left);
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
                csoundDeleteTree(csound, tmp);
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
                csoundDeleteTree(csound, tmp);
                //print_tree(csound, "1 op X -> 1\n", current);
                break;
              }
              case '*':
                csoundDeleteTree(csound,current->left);
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
      case S_UPLUS:
      case '~':
        //print_tree(csound, "Folding case?\n", current);
        current->right = constant_fold(csound, current->right);
        //print_tree(csound, "Folding case??\n", current);
        if (current->right->type == INTEGER_TOKEN ||
             current->right->type == NUMBER_TOKEN) {
          cs_float lval;
          char buf[64];
          lval = (current->right->type == INTEGER_TOKEN ?
                  (cs_double)current->right->value->value :
                  current->right->value->fvalue);
          switch (current->type) {
          case S_UMINUS:
            lval = -lval;
            break;
          case '~':
            lval = (cs_float)(~(int)lval);
            break;
          case S_UPLUS:
            break;
          }
          current->value = current->right->value;
          current->type = NUMBER_TOKEN;
          current->value->fvalue = lval;
          snprintf(buf, 60, "%.20g", lval);
          csound->Free(csound, current->value->lexeme);
          current->value->lexeme = csoundParserStrdup(csound, buf);
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

/* Historical pass. Kept separate so the reference path still performs all
   semantic lowering and later execution-rate resolution. */
static TREE * optimize_legacy(CSOUND *csound, TREE *root)
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
    // return remove_excess_assigns(csound,original);
}

/* Dumps are deliberately opt-in and never run in the performance callback.
   Names are numbered by first occurrence within a scope, without addresses or
   source locations. Types and callback phases describe the resolved overload;
   TREE phases are provisional until UDO classification finishes. */
typedef struct {
    CS_HASH_TABLE *names;
    CS_VAR_POOL *localPool, *globalPool;
    int32_t temporaries, labels, operations, init, perf, deinit;
} ORC_DUMP;

/* Read resolved metadata without rerunning semantic checking or creating ARGs. */
static CS_VARIABLE *dump_variable(CSOUND *csound, ORC_DUMP *dump, char *name)
{
    char *copy = csoundStrdup(csound, name);
    char *annotation = strstr(copy, "@global");
    if (annotation)
      memmove(annotation, annotation + 7, strlen(annotation + 7) + 1);
    char *state;
    char *base = cs_strtok_r(copy, ".", &state);
    CS_VAR_POOL *pools[] = {annotation ? NULL : dump->localPool,
                            dump->globalPool, csound->engineState.varPool};
    CS_VARIABLE *var = NULL;
    for (int32_t i = 0; base && !var && i < 3; i++)
      if (pools[i]) var = csoundFindVariableWithName(csound, pools[i], base);
    char *member;
    while (var && (member = cs_strtok_r(NULL, ".", &state)) != NULL)
      var = getStructMember(var->varType->members, member);
    csound->Free(csound, copy);
    return var;
}

static void dump_variable_type(CSOUND *csound, CS_VARIABLE *var)
{
    const CS_TYPE *actual = var && var->varType == &CS_VAR_TYPE_ARRAY
                             ? var->subType : var ? var->varType : NULL;
    char *display = csoundFormatTypeName(csound,
        actual ? actual->varTypeName : NULL, var ? var->dimensions : 0);
    csound->Message(csound, ":%s", display);
    csound->Free(csound, display);
}

static void dump_type(CSOUND *csound, ORC_DUMP *dump, char *name)
{
    const char *type = NULL;
    char c = *name;
    if (c == '"') type = "S";
    else if (((c >= '0' && c <= '9') && strcmp(name, "0dbfs")) ||
             c == '.' || c == '-' || c == '+') type = "c";
    else if (!strncmp(name, "__synthetic_", 12)) type = "l";
    if (type) {
      csound->Message(csound, ":%s", type);
      return;
    }
    CS_VARIABLE *var = dump_variable(csound, dump, name);
    if (var) dump_variable_type(csound, var);
    else csound->Message(csound, ":%s", is_pfield_name(name) ? "p" : "?");
}

static void dump_name(CSOUND *csound, ORC_DUMP *dump, char *name)
{
    char *member = *name == '#' ? strchr(name, '.') : NULL;
    if (member) {
      char *base = cs_strndup(csound, name, member - name);
      dump_name(csound, dump, base);
      csound->Message(csound, "%s", member);
      csound->Free(csound, base);
      return;
    }
    int temporary = name[0] == '#';
    int label = !strncmp(name, "__synthetic_", 12);
    if (temporary || label) {
      intptr_t id = (intptr_t)cs_hash_table_get(csound, dump->names, name);
      if (!id) {
        id = temporary ? ++dump->temporaries : ++dump->labels;
        cs_hash_table_put(csound, dump->names, name, (void *)id);
      }
      csound->Message(csound, "%s%d", temporary ? "t" : "L", (int)id);
    }
    else csound->Message(csound, "%s", name);
}

static void dump_entry(CSOUND *csound, ORC_DUMP *dump, OENTRY *entry)
{
    dump->operations++;
    dump->init += entry->init != NULL;
    dump->perf += entry->perf != NULL;
    dump->deinit += entry->deinit != NULL;
    csound->Message(csound, "  op %s out=%s in=%s phase=%s%s%s ",
                    entry->opname, entry->outypes ? entry->outypes : "",
                    entry->intypes ? entry->intypes : "",
                    entry->init ? "i" : "", entry->perf ? "p" : "",
                    entry->deinit ? "d" : "");
}

static void dump_end(CSOUND *csound, ORC_DUMP *dump)
{
    csound->Message(csound,
        "  counts ops=%d init=%d perf=%d deinit=%d temps=%d labels=%d\n",
        dump->operations, dump->init, dump->perf, dump->deinit,
        dump->temporaries, dump->labels);
    cs_hash_table_free(csound, dump->names);
}

/* Preserve the reference shape without populating cached struct markup. */
static void dump_tree_reference(CSOUND *csound, ORC_DUMP *dump, TREE *arg)
{
    if (arg->type == STRUCT_EXPR) {
      dump_tree_reference(csound, dump, arg->left);
      for (TREE *member = arg->right; member; member = member->next) {
        csound->Message(csound, ".");
        dump_tree_reference(csound, dump, member);
      }
    }
    else if (arg->type == T_ARRAY) {
      dump_tree_reference(csound, dump, arg->left);
      csound->Message(csound, "[");
      for (TREE *index = arg->right; index; index = index->next) {
        dump_tree_reference(csound, dump, index);
        if (index->next) csound->Message(csound, ",");
      }
      csound->Message(csound, "]");
    }
    else if (arg->value) dump_name(csound, dump, arg->value->lexeme);
    else csound->Message(csound, "<node:%d>", arg->type);
}

static CS_VARIABLE *dump_tree_variable(CSOUND *csound, ORC_DUMP *dump, TREE *arg)
{
    if (arg->type == STRUCT_EXPR) {
      CS_VARIABLE *var = dump_tree_variable(csound, dump, arg->left);
      const CS_TYPE *container = var ? var->varType : NULL;
      if (container == &CS_VAR_TYPE_ARRAY && arg->left->type == T_ARRAY)
        container = var->subType;
      for (TREE *member = arg->right; container && member; member = member->next) {
        var = getStructMember(container->members, member->value->lexeme);
        container = var ? var->varType : NULL;
      }
      return var;
    }
    if (arg->type == T_ARRAY) return dump_tree_variable(csound, dump, arg->left);
    return arg->value ? dump_variable(csound, dump, arg->value->lexeme) : NULL;
}

static void dump_tree_args(CSOUND *csound, ORC_DUMP *dump, TREE *args,
                           int32_t typed)
{
    csound->Message(csound, "[");
    for (; args; args = args->next) {
      dump_tree_reference(csound, dump, args);
      if (typed) {
        if (args->type == STRUCT_EXPR)
          dump_variable_type(csound, dump_tree_variable(csound, dump, args));
        else if (args->value) dump_type(csound, dump, args->value->lexeme);
        else csound->Message(csound, ":?");
      }
      if (args->next) csound->Message(csound, ",");
    }
    csound->Message(csound, "]");
}

static void dump_tree(CSOUND *csound, TREE *root, CS_VAR_POOL *localPool,
                       CS_VAR_POOL *globalPool)
{
    ORC_DUMP dump = {0};
    dump.names = cs_hash_table_create(csound);
    dump.localPool = localPool;
    dump.globalPool = globalPool;
    for (; root; root = root->next) {
      if (root->type == INSTR_TOKEN || root->type == UDO_TOKEN) {
        csound->Message(csound, " scope %s\n",
                        root->type == INSTR_TOKEN ? "instrument" : "udo");
        dump_tree_args(csound, &dump, root->left, 0);
        csound->Message(csound, "\n");
        dump_tree(csound, root->right, (CS_VAR_POOL *)root->markup, globalPool);
        csound->Message(csound, " endscope\n");
      }
      else if (root->type == LABEL_TOKEN) {
        csound->Message(csound, "  label ");
        dump_name(csound, &dump, root->value->lexeme);
        csound->Message(csound, "\n");
      }
      else if ((root->type == T_OPCALL || root->type == T_ASSIGNMENT ||
                root->type == '=' || root->type == GOTO_TOKEN ||
                root->type == IGOTO_TOKEN || root->type == KGOTO_TOKEN ||
                root->type == S_ADDIN || root->type == S_SUBIN ||
                root->type == S_MULIN || root->type == S_DIVIN) && root->markup) {
        dump_entry(csound, &dump, (OENTRY *)root->markup);
        dump_tree_args(csound, &dump, root->left, 1);
        csound->Message(csound, " <- ");
        dump_tree_args(csound, &dump, root->right, 1);
        csound->Message(csound, "\n");
      }
    }
    dump_end(csound, &dump);
}

static void dump_args(CSOUND *csound, ORC_DUMP *dump, ARGLST *args)
{
    csound->Message(csound, "[");
    if (args) for (int32_t i = 0; i < args->count; i++) {
      dump_name(csound, dump, args->arg[i]);
      dump_type(csound, dump, args->arg[i]);
      if (i + 1 < args->count) csound->Message(csound, ",");
    }
    csound->Message(csound, "]");
}

void csound_orc_dump_templates(CSOUND *csound, ENGINE_STATE *state)
{
    if (!(csoundGetDebug(csound) & DEBUG_OPTIMIZER)) return;
    csound->Message(csound, "ORC-OPT templates phase=resolved\n");
    for (INSTRTXT *ip = state->instxtanchor.nxtinstxt; ip; ip = ip->nxtinstxt) {
      ORC_DUMP dump = {0};
      dump.names = cs_hash_table_create(csound);
      dump.localPool = ip->varPool;
      dump.globalPool = state->varPool;
      csound->Message(csound, " scope %s\n", ip->insname ? ip->insname : "instrument");
      for (OPTXT *op = ip->nxtop; op; op = op->nxtop) {
        if (!op->t.oentry) continue;
        if (!strcmp(op->t.oentry->opname, "$label")) {
          csound->Message(csound, "  label ");
          dump_name(csound, &dump, op->t.opcod);
          csound->Message(csound, "\n");
          continue;
        }
        dump_entry(csound, &dump, op->t.oentry);
        dump_args(csound, &dump, op->t.outlist);
        csound->Message(csound, " <- ");
        dump_args(csound, &dump, op->t.inlist);
        csound->Message(csound, "\n");
      }
      if (ip->varPool) {
        size_t audioBytes = 0;
        int32_t audioVariables = 0, allocatedTemps = 0;
        for (CS_VARIABLE *v = ip->varPool->head; v; v = v->next) {
          if (v->varName[0] == '#') allocatedTemps++;
          if (!strcmp(v->varType->varTypeName, "a")) {
            audioVariables++;
            audioBytes += v->memBlockSize;
          }
        }
        csound->Message(csound,
            "  storage pool_bytes=%d allocated_temps=%d audio_vars=%d "
            "audio_template_bytes=%zu\n", ip->varPool->poolSize,
            allocatedTemps, audioVariables, audioBytes);
      }
      dump_end(csound, &dump);
      csound->Message(csound, " endscope\n");
    }
    csound->Message(csound, "ORC-OPT end\n");
}

TREE *csound_orc_optimize(CSOUND *csound, TREE *root, TYPE_TABLE *types)
{
    int32_t *selection = csoundQueryGlobalVariable(csound, "__orc_optimizer");
    int32_t passes = selection ? *selection : CSOUND_OPT_LEGACY;
    int32_t dump = csoundGetDebug(csound) & DEBUG_OPTIMIZER;
    if (dump) {
      csound->Message(csound, "ORC-OPT before phase=provisional\n");
      dump_tree(csound, root, types->instr0LocalPool, types->globalPool);
      csound->Message(csound, "ORC-OPT end\n");
    }
    if (passes & CSOUND_OPT_LEGACY) root = optimize_legacy(csound, root);
    if (passes & CSOUND_OPT_CONSTANT_FOLDING)
      root = fold_scalar_literals(csound, root, types->instr0LocalPool);
    if (passes & CSOUND_OPT_COPY_REMOVAL) remove_scalar_copies(csound, root);
    if (dump) {
      csound->Message(csound, "ORC-OPT after phase=provisional\n");
      dump_tree(csound, root, types->instr0LocalPool, types->globalPool);
      csound->Message(csound, "ORC-OPT end\n");
    }
    return root;
}

