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
  int32_t                         index;
  LOCK_TYPE                   lock;
  struct global_var_lock_t    *next;
};

inline void csp_locks_lock(CSOUND * csound, int32_t global_index)
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

inline void csp_locks_unlock(CSOUND * csound, int32_t global_index)
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
                                                       char *name, int32_t index)
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
      int32_t ctr = 0;
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
    return anchor;
}

void csp_locks_cache_build(CSOUND *csound)
{
    int32_t ctr = 0;
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
int32_t globallock(CSOUND *csound, GLOBAL_LOCK_UNLOCK *p)
{
    /* csound->Message(csound, "Locking:   %i\n", (int)*p->gvar_ix); */
    csp_locks_lock(csound, (int)*p->gvar_ix);
    return OK;
}

int32_t globalunlock(CSOUND *csound, GLOBAL_LOCK_UNLOCK *p)
{
    /* csound->Message(csound, "UnLocking: %i\n", (int)*p->gvar_ix); */
    csp_locks_unlock(csound, (int)*p->gvar_ix);
    return OK;
}


