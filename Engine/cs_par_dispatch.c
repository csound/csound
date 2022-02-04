/*
    cs_par_dispatch.c:

    Copyright (C) 2009: Chris Wilson and John ffitch

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
#include "cs_par_base.h"
#include "cs_par_orc_semantics.h"
#include "cs_par_dispatch.h"

#include "cs_par_ops.h"
#include "cs_par_structs.h"

/***********************************************************************
 * external prototypes not in headers
 */
extern ORCTOKEN *lookup_token(CSOUND *csound, char *);
extern void print_tree(CSOUND *, char *, TREE *);

/***********************************************************************
 * Global Var Lock Inserts
 */

/* global variables lock support */
struct global_var_lock_t {
  char                        hdr[HDR_LEN];
  char                        *name;
  int                         index;
  LOCK_TYPE                   lock;
  struct global_var_lock_t    *next;
};

inline void csp_locks_lock(CSOUND * csound, int global_index)
{
    if (UNLIKELY(global_index >= csound->global_var_lock_count)) {
      csound->Die(csound,
                  Str("Poorly specified global lock index: %i [max: %i]\n"),
                  global_index, csound->global_var_lock_count);
    }
    /* TRACE_2("Locking:   %i [%p %s]\n", global_index, */
    /*         csound->global_var_lock_cache[global_index], */
    /*         csound->global_var_lock_cache[global_index]->name); */
    TAKE_LOCK(&(csound->global_var_lock_cache[global_index]->lock));
}

inline void csp_locks_unlock(CSOUND * csound, int global_index)
{
    if (UNLIKELY(global_index >= csound->global_var_lock_count)) {
      csound->Die(csound,
                  Str("Poorly specified global lock index: %i [max: %i]\n"),
                  global_index, csound->global_var_lock_count);
    }
    RELS_LOCK(&(csound->global_var_lock_cache[global_index]->lock));
    /* TRACE_2("UnLocking: %i [%p %s]\n", */
    /*         global_index, csound->global_var_lock_cache[global_index], */
    /*         csound->global_var_lock_cache[global_index]->name); */
}

static struct global_var_lock_t *global_var_lock_alloc(CSOUND *csound,
                                                       char *name, int index)
{
    if (UNLIKELY(name == NULL))
      csound->Die(csound,
                  Str("Invalid NULL parameter name for a global variable\n"));

    struct global_var_lock_t *ret =
      csound->Malloc(csound, sizeof(struct global_var_lock_t));
    memset(ret, 0, sizeof(struct global_var_lock_t));
    INIT_LOCK(ret->lock);
    strncpy(ret->hdr, GLOBAL_VAR_LOCK_HDR, HDR_LEN);
    ret->name = name;
    ret->index = index;

    csound->global_var_lock_count++;

    return ret;
}

static struct global_var_lock_t
  *global_var_lock_find(CSOUND *csound, char *name)
{
    if (UNLIKELY(name == NULL))
      csound->Die(csound,
                  Str("Invalid NULL parameter name for a global variable\n"));

    if (csound->global_var_lock_root == NULL) {
      csound->global_var_lock_root = global_var_lock_alloc(csound, name, 0);
      return csound->global_var_lock_root;
    }
    else {
      struct global_var_lock_t *current = csound->global_var_lock_root,
        *previous = NULL;
      int ctr = 0;
      while (current != NULL) {
        if (strcmp(current->name, name) == 0) {
          break;
        }
        previous = current;
        current = current->next;
        ctr++;
      }
      if (current == NULL) {
        previous->next = global_var_lock_alloc(csound, name, ctr);
        return previous->next;
      }
      else {
        return current;
      }
    }
}

TREE *csp_locks_insert(CSOUND *csound, TREE *root)
{
    csound->Message(csound,
                    Str("Inserting Parallelism Constructs into AST\n"));

    TREE *anchor = NULL;
//
//    TREE *current = root;
//    TREE *previous = NULL;
//    INSTR_SEMANTICS *instr = NULL;
//
//    while (current != NULL) {
//      switch(current->type) {
//      case INSTR_TOKEN:
//        if (current->left->type == T_INSTLIST) {
//          instr =
//            csp_orc_sa_instr_get_by_name(csound,
//                                               current->left->left->value->lexeme);
//        }
//        else {
//          instr = csp_orc_sa_instr_get_by_name(csound,
//                                               current->left->value->lexeme);
//        }
//        if (instr->read_write->count > 0 &&
//            instr->read->count == 0 &&
//            instr->write->count == 0) {
//          csound->Message(csound, Str("Instr %d needs locks"), instr->insno);
//          //print_tree(csound, "before locks", root);
//          current->right = csp_locks_insert(csound, current->right);
//          //print_tree(csound, "after locks", root);
//        }
//        break;
//
//      case UDO_TOKEN:
//      case IF_TOKEN:
//        break;
//
//      case '=':
//        /*if (current->type == '=')*/
//        {
//          struct set_t *left = csp_orc_sa_globals_find(csound, current->left);
//          struct set_t *right = csp_orc_sa_globals_find(csound, current->right);
//          struct set_t *new = NULL;
//          csp_set_union(csound, left, right, &new);
//          /* add locks if this is a read-write global variable
//           * that is same global read and written in this operation */
//          if (left->count == 1 && right->count == 1 && new->count == 1) {
//            char *global_var = NULL;
//            struct global_var_lock_t *gvar;
//            char buf[8];
//            ORCTOKEN *lock_tok, *unlock_tok, *var_tok, *var0_tok;;
//            TREE *lock_leaf, *unlock_leaf;
//
//            csp_set_get_num(new, 0, (void **)&global_var);
//            gvar       = global_var_lock_find(csound, global_var);
//            lock_tok   = lookup_token(csound, "##globallock");
//            unlock_tok = lookup_token(csound, "##globalunlock");
//            snprintf(buf, 8, "%i", gvar->index);
//            var_tok    = make_int(csound, buf);
//            var0_tok   = make_int(csound, buf);
//
//            lock_leaf  = make_leaf(csound, current->line, current->locn,
//                                   T_OPCODE, lock_tok);
//            lock_leaf->right = make_leaf(csound, current->line, current->locn,
//                                         INTEGER_TOKEN, var_tok);
//            unlock_leaf = make_leaf(csound, current->line, current->locn,
//                                    T_OPCODE, unlock_tok);
//            unlock_leaf->right = make_leaf(csound, current->line, current->locn,
//                                           INTEGER_TOKEN, var0_tok);
//
//            if (previous == NULL) {
//              //TREE *old_current = lock_leaf;
//              lock_leaf->next = current;
//              unlock_leaf->next = current->next;
//              current->next = unlock_leaf;
//              current = unlock_leaf;
//              //print_tree(csound, "changed to\n", lock_leaf);
//            }
//            else {
//              previous->next = lock_leaf;
//              lock_leaf->next = current;
//              unlock_leaf->next = current->next;
//              current->next = unlock_leaf;
//              current = unlock_leaf;
//              //print_tree(csound, "changed-1 to\n", lock_leaf);
//            }
//          }
//
//          csp_set_dealloc(csound, &new);
//          csp_set_dealloc(csound, &left);
//          csp_set_dealloc(csound, &right);
//       }
//      default:
//        break;
//      }
//
//      if (anchor == NULL) {
//        anchor = current;
//      }
//
//      previous = current;
//      current = current->next;
//
//    }
//
//    csound->Message(csound,
//                    Str("[End Inserting Parallelism Constructs into AST]\n"));

    return anchor;
}

void csp_locks_cache_build(CSOUND *csound)
{
    int ctr = 0;
    struct global_var_lock_t *glob;
    if (UNLIKELY(csound->global_var_lock_count < 1)) {
      return;
    }

    csound->global_var_lock_cache =
      csound->Malloc(csound,
                     sizeof(struct global_var_lock_t *) *
                     csound->global_var_lock_count);

    glob = csound->global_var_lock_root;
    while (glob != NULL && ctr < csound->global_var_lock_count) {
      csound->global_var_lock_cache[ctr] = glob;
      glob = glob->next;
      ctr++;
    }

    /* csound->Message(csound, "Global Locks Cache\n");
       ctr = 0;
       while (ctr < csound->global_var_lock_count) {
       csound->Message(csound, "[%i] %s\n",
                       csound->global_var_lock_cache[ctr]->index,
       csound->global_var_lock_cache[ctr]->name);
       ctr++;
       } */
}

/* The opcodes that implement local global locks */
int globallock(CSOUND *csound, GLOBAL_LOCK_UNLOCK *p)
{
    /* csound->Message(csound, "Locking:   %i\n", (int)*p->gvar_ix); */
    csp_locks_lock(csound, (int)*p->gvar_ix);
    return OK;
}

int globalunlock(CSOUND *csound, GLOBAL_LOCK_UNLOCK *p)
{
    /* csound->Message(csound, "UnLocking: %i\n", (int)*p->gvar_ix); */
    csp_locks_unlock(csound, (int)*p->gvar_ix);
    return OK;
}


