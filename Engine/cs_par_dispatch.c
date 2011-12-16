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
#include "tok.h"
#include "cs_par_base.h"
#include "cs_par_orc_semantics.h"
#include "cs_par_dispatch.h"

#include "cs_par_ops.h"
#include "cs_par_structs.h"

/***********************************************************************
 * external prototypes not in headers
 */
extern ORCTOKEN *lookup_token(CSOUND *csound, char *);

/***********************************************************************
 * static function prototypes
 */
static uint32_t inline hash( uint32_t a );
static uint32_t hash_chain(INSDS *chain, uint32_t hash_size);
static uint32_t hash_string(char *str, uint32_t hash_size);

/***********************************************************************
 * helper functions
 */

/* Robert Jenkins' 32 bit integer hash function from
 * http://www.concentric.net/~Ttwang/tech/inthash.htm
 */
static uint32_t inline hash( uint32_t a )
{
    a = (a+0x7ed55d16) + (a<<12);
    a = (a^0xc761c23c) ^ (a>>19);
    a = (a+0x165667b1) + (a<<5);
    a = (a+0xd3a2646c) ^ (a<<9);
    a = (a+0xfd7046c5) + (a<<3);
    a = (a^0xb55a4f09) ^ (a>>16);
    return a;
}

static uint32_t hash_chain(INSDS *chain, uint32_t hash_size)
{
    uint32_t val = 0;
    while (chain != NULL) {
      val = val ^ chain->insno;
      val = hash( val );
      chain = chain->nxtact;
    }
    return val % hash_size;
}

static uint32_t hash_string(char *str, uint32_t hash_size)
{
    uint32_t ctr = 0;
    uint32_t val = 0;
    uint32_t len = strlen(str);
    while (ctr < len) {
      val = val ^ str[ctr];
      val = hash( val );
      ctr++;
    }
    return val % hash_size;
}

/***********************************************************************
 * Global Var Lock Inserts
 */
#pragma mark -
#pragma mark Global Var Lock Inserts

/* global variables lock support */
struct global_var_lock_t {
  char                        hdr[HDR_LEN];
  char                        *name;
  int                         index;
  LOCK_TYPE                   lock;
  struct global_var_lock_t    *next;
};

//static struct global_var_lock_t *global_var_lock_root;
//static int global_var_lock_count;
//static struct global_var_lock_t **global_var_lock_cache;


void inline csp_locks_lock(CSOUND * csound, int global_index)
{
    if (UNLIKELY(global_index >= csound->global_var_lock_count)) {
      csound->Die(csound,
                  Str("Poorly specified global lock index: %i [max: %i]\n"),
                  global_index, csound->global_var_lock_count);
    }
    TRACE_2("Locking:   %i [%p %s]\n", global_index,
            csound->global_var_lock_cache[global_index],
            csound->global_var_lock_cache[global_index]->name);
    TAKE_LOCK(&(csound->global_var_lock_cache[global_index]->lock));
}

void inline csp_locks_unlock(CSOUND * csound, int global_index)
{
    if (UNLIKELY(global_index >= csound->global_var_lock_count)) {
      csound->Die(csound,
                  Str("Poorly specified global lock index: %i [max: %i]\n"),
                  global_index, csound->global_var_lock_count);
    }
    RELS_LOCK(&(csound->global_var_lock_cache[global_index]->lock));
    TRACE_2("UnLocking: %i [%p %s]\n",
            global_index, csound->global_var_lock_cache[global_index],
            csound->global_var_lock_cache[global_index]->name);
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

/* static void locks_print(CSOUND *csound)
   {
   csound->Message(csound, Str("Current Global Locks\n"));
   struct global_var_lock_t *cg = csound->global_var_lock_root;
   while (cg != NULL) {
   csound->Message(csound, "[%i] %s [%p]\n", cg->index,
                   cg->name, cg);
   cg = cg->next;
   }
   } */

TREE *csp_locks_insert(CSOUND *csound, TREE *root)
{
    csound->Message(csound,
                    "Inserting Parallelism Constructs into AST\n");

    TREE *anchor = NULL;

    TREE *current = root;
    TREE *previous = NULL;
    INSTR_SEMANTICS *instr = NULL;

    while(current != NULL) {
      switch(current->type) {
      case INSTR_TOKEN:
        instr = csp_orc_sa_instr_get_by_name(csound,
                                             current->left->value->lexeme);
        if (instr->read_write->count > 0 &&
            instr->read->count == 0 &&
            instr->write->count == 0) {
          current->right = csp_locks_insert(csound, current->right);
        }
        break;

      case UDO_TOKEN:
      case IF_TOKEN:
        break;

      default:
        if (current->type == '=') {
          struct set_t *left = NULL, *right  = NULL;
          left = csp_orc_sa_globals_find(csound, current->left);
          right = csp_orc_sa_globals_find(csound, current->right);

          struct set_t *new = NULL;
          csp_set_union(csound, left, right, &new);
          /* add locks if this is a read-write global variable
           * that is same global read and written in this operation */
          if (left->count == 1 && right->count == 1 && new->count == 1) {
            char *global_var = NULL;
            csp_set_get_num(csound, new, 0, (void **)&global_var);

            struct global_var_lock_t *gvar =
              global_var_lock_find(csound, global_var);

            /* add_token(csound, "str", A_TYPE); */

            char buf[8];
            snprintf(buf, 8, "%i", gvar->index);

            ORCTOKEN *lock_tok   = lookup_token(csound, "##globallock");
            ORCTOKEN *unlock_tok = lookup_token(csound, "##globalunlock");
            ORCTOKEN *var_tok    = make_int(csound, buf);

            TREE *lock_leaf = make_leaf(csound, current->line, T_OPCODE, lock_tok);
            lock_leaf->right = make_leaf(csound, current->line, INTEGER_TOKEN, var_tok);
            TREE *unlock_leaf = make_leaf(csound, current->line, T_OPCODE, unlock_tok);
            unlock_leaf->right = make_leaf(csound, current->line, INTEGER_TOKEN, var_tok);

            if (previous == NULL) {
              TREE *old_current = lock_leaf;
              lock_leaf->next = current;
              unlock_leaf->next = current->next;
              current->next = unlock_leaf;
              current = old_current;
            }
            else {
              previous->next = lock_leaf;
              lock_leaf->next = current;
              unlock_leaf->next = current->next;
              current->next = unlock_leaf;
            }
          }

          csp_set_dealloc(csound, &new);
          csp_set_dealloc(csound, &left);
          csp_set_dealloc(csound, &right);
        }
        break;
      }

      if (anchor == NULL) {
        anchor = current;
      }

      previous = current;
      current = current->next;

    }

    csound->Message(csound,
                    "[End Inserting Parallelism Constructs into AST]\n");

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

/***********************************************************************
 * weighting
 */
#pragma mark -
#pragma mark Instr weightings

/* static struct instr_weight_info_t 
   *instr_weight_info_alloc(CSOUND *csound)
   {
   struct instr_weight_info_t *ret =
   csound->Malloc(csound, sizeof(struct instr_weight_info_t));
   memset(ret, 0, sizeof(struct instr_weight_info_t));
   strncpy(ret->hdr, INSTR_WEIGHT_INFO_HDR, HDR_LEN);

   return ret;
   } */

#define WEIGHT_UNKNOWN_NODE     1
#define WEIGHT_S_ASSIGN_NODE    1
#define WEIGHT_OPCODE_NODE      5

static void csp_weights_calculate_instr(CSOUND *csound, TREE *root,
                                        INSTR_SEMANTICS *instr)
{
    csound->Message(csound, "Calculating Instrument weight from AST\n");

    TREE *current = root;
    INSTR_SEMANTICS *nested_instr = NULL;

    while(current != NULL) {
      switch(current->type) {
      case INSTR_TOKEN:
        nested_instr = 
          csp_orc_sa_instr_get_by_name(csound,
                                       current->left->value->lexeme);
        /* if (nested_instr->weight == NULL) {
           nested_instr->weight = instr_weight_info_alloc(csound);
           } */
        csp_weights_calculate_instr(csound,
                                    current->right, nested_instr);
        break;

#ifdef LOOKUP_WEIGHTS
      case T_OPCODE:
      case T_OPCODE0:
        instr->weight += csp_opcode_weight_fetch(csound,
                                                 current->value->lexeme);
        break;
#else
      case T_OPCODE:
      case T_OPCODE0:
        instr->weight += WEIGHT_OPCODE_NODE;
        break;
      case '=':
        instr->weight += WEIGHT_S_ASSIGN_NODE;
        break;
#endif

      default:
        csound->Message(csound,
                        "WARNING: Unexpected node type in weight "
                        "calculation walk %i\n", current->type);
        instr->weight += WEIGHT_UNKNOWN_NODE;
        break;
      }

      current = current->next;
    }

    csound->Message(csound,
                    "[End Calculating Instrument weight from AST]\n");
}

void csp_weights_calculate(CSOUND *csound, TREE *root)
{
    csound->Message(csound, "Calculating Instrument weights from AST\n");

    TREE *current = root;
    INSTR_SEMANTICS *instr = NULL;

    while(current != NULL) {
      switch(current->type) {
      case INSTR_TOKEN:
        instr = csp_orc_sa_instr_get_by_name(csound,
                                             current->left->value->lexeme);
        /* if (instr->weight == NULL) {
           instr->weight = instr_weight_info_alloc(csound);
           } */
        csp_weights_calculate_instr(csound, current->right, instr);
        break;

      default:
        break;
      }

      current = current->next;
    }

    csound->Message(csound,
                    "[End Calculating Instrument weights from AST]\n");
}

static void csp_orc_sa_opcode_dump_instr(CSOUND *csound, TREE *root)
{
    TREE *current = root;

    while(current != NULL) {
      switch(current->type) {
      case INSTR_TOKEN:
        break;

      case T_OPCODE:
      case T_OPCODE0:
        csound->Message(csound, "OPCODE: %s\n", current->value->lexeme);
        break;

      case '=':
        break;

      default:
        csound->Message(csound, "WARNING: Unexpected node type in weight "
                        "calculation walk %i\n", current->type);
        break;
      }

      current = current->next;
    }
}

void csp_orc_sa_opcode_dump(CSOUND *csound, TREE *root)
{
    csound->Message(csound, "Opcode List from AST\n");

    TREE *current = root;

    while(current != NULL) {
      switch(current->type) {
      case INSTR_TOKEN:
        csp_orc_sa_opcode_dump_instr(csound, current->right);
        break;

      default:
        break;
      }

      current = current->next;
    }

    csound->Message(csound, "[End Opcode List from AST]\n");
}


/***********************************************************************
 * weights structure
 */
#pragma mark -
#pragma mark weights structure

struct opcode_weight_cache_entry_t {
  uint32_t                            hash_val;
  struct opcode_weight_cache_entry_t  *next;

  char                                *name;
  double                              play_time;
  uint32_t                            weight;
};

//#define OPCODE_WEIGHT_CACHE_SIZE     128

//static int opcode_weight_cache_ctr;
//static struct opcode_weight_cache_entry_t *
//       opcode_weight_cache[OPCODE_WEIGHT_CACHE_SIZE];

//static int opcode_weight_have_cache;

static void opcode_weight_entry_add(CSOUND *csound,
                                    char *name, uint32_t weight);

static int 
  opcode_weight_entry_alloc(CSOUND *csound,
                            struct opcode_weight_cache_entry_t **entry,
                            char *name, uint32_t weight, uint32_t hash_val)
{
#ifdef CAUTIOUS
    if (UNLIKELY(entry == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter entry"));
    if (UNLIKELY(name == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter name"));
#endif

    *entry = csound->Malloc(csound,
                            sizeof(struct opcode_weight_cache_entry_t));
    if (UNLIKELY(*entry == NULL)) {
      csound->Die(csound,
                  Str("Failed to allocate Opcode Weight cache entry"));
    }
    memset(*entry, 0, sizeof(struct opcode_weight_cache_entry_t));

    (*entry)->hash_val = hash_val;
    (*entry)->name     = name;
    (*entry)->weight   = weight;

    return CSOUND_SUCCESS;
}

#if 0
static int 
  opcode_weight_entry_dealloc(CSOUND *csound,
                              struct opcode_weight_cache_entry_t **entry)
{
#ifdef CAUTIOUS
    if (UNLIKELY(entry == NULL || *entry == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter entry"));
#endif

    csound->Free(csound, *entry);
    *entry = NULL;

    return CSOUND_SUCCESS;
}
#endif

uint32_t csp_opcode_weight_fetch(CSOUND *csound, char *name)
{
#ifdef CAUTIOUS
    if (UNLIKELY(name == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter name"));
#endif

    if (csound->opcode_weight_have_cache == 0) {
      return WEIGHT_OPCODE_NODE;
    }
    else {
      uint32_t hash_val = hash_string(name, OPCODE_WEIGHT_CACHE_SIZE);
      struct opcode_weight_cache_entry_t *curr =
        csound->opcode_weight_cache[hash_val];
      while (curr != NULL) {
        if (UNLIKELY(strcmp(curr->name, name) == 0)) {
          return curr->weight;
        }
        curr = curr->next;
      }
      /* no weight for this opcode use default */
      csound->Message(csound,
                      "WARNING: no weight found for opcode: %s\n", name);
      return WEIGHT_OPCODE_NODE;
    }
}

void csp_opcode_weight_set(CSOUND *csound, char *name, double play_time)
{
#ifdef CAUTIOUS
    if (UNLIKELY(name == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter name"));
#endif

    if (csound->opcode_weight_have_cache == 0) {
      return;
    }
    else {
      uint32_t hash_val = hash_string(name, OPCODE_WEIGHT_CACHE_SIZE);
      struct opcode_weight_cache_entry_t *curr =
        csound->opcode_weight_cache[hash_val];
      TRACE_0("Adding %s [%u]\n", name, hash_val);

      while (curr != NULL) {
        TRACE_0("Looking at: %s\n", curr->name);
        if (strcmp(curr->name, name) == 0) {
          if (UNLIKELY(curr->play_time == 0)) {
            curr->play_time = play_time;
          }
          else {
            curr->play_time = 0.9 * curr->play_time + 0.1 * play_time;
          }
        return;
        }
        curr = curr->next;
      }

      TRACE_0("Not Found Adding\n");

      /* not here add it and then set the play time */
      opcode_weight_entry_add(csound, strdup(name), WEIGHT_OPCODE_NODE);
      TRACE_0("Not Found Added\n");
      csp_opcode_weight_set(csound, name, play_time);
      TRACE_0("Not Found Done\n");
    }
}

static void opcode_weight_entry_add(CSOUND *csound,
                                    char *name, uint32_t weight)
{
#ifdef CAUTIOUS
    if (UNLIKELY(name == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter name"));
#endif

    uint32_t hash_val = hash_string(name, OPCODE_WEIGHT_CACHE_SIZE);
    struct opcode_weight_cache_entry_t *curr =
      csound-> opcode_weight_cache[hash_val];
    int found = 0;
    TRACE_0("entry_add %s [%u]\n", name, hash_val);
    while (curr != NULL) {
      if (strcmp(curr->name, name) == 0) {
        found = 1;
        break;
      }
      curr = curr->next;
    }
    if (found == 0) {
      TRACE_0("Allocing %s\n", name);
      opcode_weight_entry_alloc(csound, &curr, name, weight, hash_val);
      csound->opcode_weight_cache_ctr++;

      curr->next = csound->opcode_weight_cache[hash_val];
      csound->opcode_weight_cache[hash_val] = curr;
    }
}

void csp_weights_dump(CSOUND *csound)
{
    if (UNLIKELY(csound->opcode_weight_have_cache == 0)) {
          csound->Message(csound,
                          Str("No Weights to Dump (Using Defaults)\n"));
      return;
    }
    else {
      uint32_t bin_ctr = 0;
      csound->Message(csound, "Weights Dump\n");
      while (bin_ctr < OPCODE_WEIGHT_CACHE_SIZE) {
        struct opcode_weight_cache_entry_t *entry =
          csound->opcode_weight_cache[bin_ctr];

        while (entry != NULL) {
          csound->Message(csound, "%s => %u\n",
                          entry->name, entry->weight);
          entry = entry->next;
        }

        bin_ctr++;
      }
      csound->Message(csound, "[Weights Dump end]\n");
    }
}

void csp_weights_dump_file(CSOUND *csound)
{
    char *path;
    FILE *f;
    uint32_t bin_ctr = 0;
    double min = 0, max = 0;
    if (UNLIKELY(csound->opcode_weight_have_cache == 0)) {
      csound->Message(csound, "No Weights to Dump to file\n");
      return;
    }

    path = csound->weights;
    if (path == NULL) {
      return;
    }

    f = fopen(path, "w+");
    if (UNLIKELY(f == NULL)) {
      csound->Die(csound,
                  Str("Opcode Weight Spec File not found at: %s"), path);
    }

    while (bin_ctr < OPCODE_WEIGHT_CACHE_SIZE) {
      struct opcode_weight_cache_entry_t *entry =
        csound->opcode_weight_cache[bin_ctr];

      while (entry != NULL) {
        if (min == 0) {
          min = entry->play_time;
        }

        if (entry->play_time != 0 && entry->play_time < min) {
          min = entry->play_time;
        }
        else if (entry->play_time != 0 && entry->play_time > max) {
          max = entry->play_time;
        }

        entry = entry->next;
      }

      bin_ctr++;
    }

    {
      double scale = 99 / (max - min);

      bin_ctr = 0;
      while (bin_ctr < OPCODE_WEIGHT_CACHE_SIZE) {
        struct opcode_weight_cache_entry_t *entry =
          csound->opcode_weight_cache[bin_ctr];

        while (entry != NULL) {
          uint32_t weight = floor((entry->play_time - min) * scale) + 1;
          fprintf(f, "%s, %u, %g\n", entry->name, weight, entry->play_time);
          entry = entry->next;
        }

        bin_ctr++;
      }
    }
    fclose(f);
}

void csp_weights_dump_normalised(CSOUND *csound)
{
    uint32_t bin_ctr = 0;
    double min = 0, max = 0;
    if (UNLIKELY(csound->opcode_weight_have_cache == 0)) {
      csound->Message(csound, Str("No Weights to Dump (Using Defaults)\n"));
      return;
    }

    csound->Message(csound, Str("Weights Dump\n"));
    while (bin_ctr < OPCODE_WEIGHT_CACHE_SIZE) {
      struct opcode_weight_cache_entry_t *entry =
        csound->opcode_weight_cache[bin_ctr];

      while (entry != NULL) {
        if (min == 0) {
          min = entry->play_time;
        }

        if (entry->play_time != 0 && entry->play_time < min) {
          min = entry->play_time;
        }
        else if (entry->play_time != 0 && entry->play_time > max) {
          max = entry->play_time;
        }

        entry = entry->next;
      }

      bin_ctr++;
    }

    csound->Message(csound, "min: %g\n", min);
    csound->Message(csound, "max: %g\n", max);

    {
      double scale = 99 / (max - min);

      csound->Message(csound, "scale: %g\n", scale);

      bin_ctr = 0;
      while (bin_ctr < OPCODE_WEIGHT_CACHE_SIZE) {
        struct opcode_weight_cache_entry_t *entry =
          csound->opcode_weight_cache[bin_ctr];

        while (entry != NULL) {
          uint32_t weight = floor((entry->play_time - min) * scale) + 1;
          csound->Message(csound, "%s => %u, %g\n",
                          entry->name, weight, entry->play_time);
          entry = entry->next;
        }

        bin_ctr++;
      }
    }
    if (csound->oparms->calculateWeights) {
      csp_weights_dump_file(csound);
    }

    csound->Message(csound, "[Weights Dump end]\n");
}

void csp_weights_load(CSOUND *csound)
{
    char *path = csound->weights;
    FILE *f;
    char buf[1024];
    char *opcode_name = NULL;
    int weight = 0;
    int ctr = 0;
    int col = 0;
    int c;
    if (path == NULL) {
      csound->opcode_weight_have_cache = 0;
      return;
    }
    csound->opcode_weight_have_cache = 1;

    memset(csound->opcode_weight_cache, 0,
           sizeof(struct opcode_weight_cache_entry_t *) *
           OPCODE_WEIGHT_CACHE_SIZE);

    f = fopen(path, "r");
    if (UNLIKELY(f == NULL)) {
      csound->Die(csound,
                  Str("Opcode Weight Spec File not found at: %s"), path);
    }

    while ((c = fgetc(f)) != EOF) {
      if (col == 0 && c == ',') {
        col = 1;
        buf[ctr] = '\0';
        opcode_name = strdup(buf);
        ctr = -1;
      }
      else if (col == 1 && c == '\n') {
        col = 0;
        buf[ctr] = '\0';
        weight = atoi(buf);
        opcode_weight_entry_add(csound, opcode_name, weight);
        opcode_name = NULL; weight = 0;
        ctr = -1;
      }
      else if (col == 1 && c == ',') {
        col = 2;
        buf[ctr] = '\0';
        weight = atoi(buf);
        ctr = -1;
      }
      else if (col == 2 && c == '\n') {
        double play_time;
        col = 0;
        buf[ctr] = '\0';
        play_time = atof(buf);
        opcode_weight_entry_add(csound, opcode_name, weight);
        csp_opcode_weight_set(csound, opcode_name, play_time);
        opcode_name = NULL; weight = 0;
        ctr = -1;
      }
      else {
        buf[ctr] = c;
      }
      ctr++;

      if (ctr > 1024-1) {
        // do something about buffer overrun
      }
    }

    fclose(f);
}



/***********************************************************************
 * weighting decision
 */
#pragma mark -
#pragma mark dag weighting decision

/* static struct instr_weight_info_t *weight_info; */

struct weight_decision_info_t {
  uint32_t    weight_min;
  uint32_t    weight_max;
  int         roots_avail_min;
  int         roots_avail_max;
};

static struct weight_decision_info_t global_weight_info = {0, 0, 0, 0};
/* {1125, 0, 2, 0}; */

void csp_orc_sa_parallel_compute_spec_read(CSOUND *csound, const char *path)
{
    FILE *f = fopen(path, "r");
    int rc = 0;
    if (UNLIKELY(f == NULL)) {
      csound->Die(csound, Str("Parallel Spec File not found at: %s"), path);
    }

    rc = fscanf(f, "%u\n", &(global_weight_info.weight_min));
    if (UNLIKELY(rc != 0 || rc == EOF))
      csound->Die(csound,
                  Str("Parallel Spec File invalid format expected "
                      "weight_min parameter"));
    rc = fscanf(f, "%u\n", &(global_weight_info.weight_max));
    if (UNLIKELY(rc != 0 || rc == EOF))
      csound->Die(csound,
                  Str("Parallel Spec File invalid format expected "
                      "weight_max parameter"));
    rc = fscanf(f, "%i\n", &(global_weight_info.roots_avail_min));
    if (UNLIKELY(rc != 0 || rc == EOF))
      csound->Die(csound, Str("Parallel Spec File invalid format "
                              "expected roots_avail_min parameter"));
    rc = fscanf(f, "%i\n", &(global_weight_info.roots_avail_max));
    if (UNLIKELY(rc != 0 || rc == EOF))
      csound->Die(csound, Str("Parallel Spec File invalid format expected"
                              " roots_avail_max parameter"));

    /* todo fix this */
    fclose(f);
}

void csp_parallel_compute_spec_setup(CSOUND *csound)
{
    char *path = "Default";

    if (csound->weight_info != NULL) {
      path = csound->weight_info;
      csp_orc_sa_parallel_compute_spec_read(csound, path);
    }

    csound->Message(csound, "InstrWeightInfo: [%s]\n"
                    "  weight_min:      %u\n"
                    "  weight_max:      %u\n"
                    "  roots_avail_min: %i\n"
                    "  roots_avail_max: %i\n",
                    path,
                    global_weight_info.weight_min,
                    global_weight_info.weight_max,
                    global_weight_info.roots_avail_min,
                    global_weight_info.roots_avail_max);
}

#if 0
int inline csp_parallel_compute_should(CSOUND *csound, DAG *dag)
{
    return (dag->weight >= global_weight_info.weight_min &&
            dag->max_roots >= global_weight_info.roots_avail_min);
    /* return (dag->ratio > 0.15 && dag->ratio < 0.85); */
    /* return (dag->weight->weight > weight_info->weight); */
    /* return 1; */
}
#endif

/***********************************************************************
 * dag2 data structure
 */
#pragma mark -
#pragma mark Dag2

/* prototypes for dag2 */
static void dag_node_2_alloc(CSOUND *csound, DAG_NODE **dag_node,
                             INSTR_SEMANTICS *instr, INSDS *insds);
static void dag_node_2_alloc_list(CSOUND *csound, DAG_NODE **dag_node,
                                  int count);
static void dag_node_2_dealloc(CSOUND *csound, DAG_NODE **dag_node);

/* add all the instruments to a DAG found in an insds chain */
/* static DAG *csp_dag_build_initial(CSOUND *csound, INSDS *chain); */
/* allocate the dynamic structures which depend on the number of instruments
   in the dag */
/* static void csp_dag_build_prepare(CSOUND *csound, DAG *dag); */
/* build the edges (fill out the table) */
/* static void csp_dag_build_edges(CSOUND *csound, DAG *dag); */
/* find the roots in the DAG and setup the root countdowns */
static void csp_dag_build_roots(CSOUND *csound, DAG *dag);
/* perpare DAG after building */
/* static void csp_dag_prepare_use_first(CSOUND *csound, DAG *dag); */
/* perpare DAG after getting out of the cache */
static void csp_dag_prepare_use(CSOUND *csound, DAG *dag);
/* follow insds chain in nodes to update insds pointers */
static inline void csp_dag_prepare_use_insds(CSOUND *csound, DAG *dag,
                                             INSDS *chain);

#define DAG_NO_LINK     0
#define DAG_STRONG_LINK 1
#define DAG_WEAK_LINK   2

void csp_dag_alloc(CSOUND *csound, DAG **dag)
{
#ifdef CAUTIOUS
    if (UNLIKELY(dag == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter dag"));
#endif

    *dag = (DAG*)csound->Malloc(csound, sizeof(DAG));
    if (UNLIKELY(*dag == NULL)) {
      csound->Die(csound, Str("Failed to allocate dag"));
    }
    memset(*dag, 0, sizeof(DAG));
    strncpy((*dag)->hdr.hdr, DAG_2_HDR, HDR_LEN);
    (*dag)->hdr.type = DAG_NODE_DAG;
    INIT_LOCK((*dag)->spinlock);
    INIT_LOCK((*dag)->consume_spinlock);
    (*dag)->count          = 0;
    (*dag)->first_root_ori = -1;
    /* (*dag)->weight = instr_weight_info_alloc(csound); */
    (*dag)->weight = 0;
    (*dag)->max_roots = 0;
}

void csp_dag_dealloc(CSOUND *csound, DAG **dag)
{
    int ctr = 0;
#ifdef CAUTIOUS
    if (UNLIKELY(dag == NULL || *dag == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter dag"));
#endif

    while (ctr < (*dag)->count) {
      dag_node_2_dealloc(csound, &((*dag)->all[ctr]));
      (*dag)->all[ctr] = NULL;
      ctr++;
    }

    if ((*dag)->all          != NULL) csound->Free(csound, (*dag)->all);
    if ((*dag)->roots_ori    != NULL) csound->Free(csound, (*dag)->roots_ori);
    if ((*dag)->roots        != NULL) csound->Free(csound, (*dag)->roots);
    if ((*dag)->root_seen_ori!= NULL)
      csound->Free(csound, (*dag)->root_seen_ori);
    if ((*dag)->root_seen    != NULL) csound->Free(csound, (*dag)->root_seen);
    if ((*dag)->table_ori    != NULL) csound->Free(csound, (*dag)->table_ori);
    if ((*dag)->table        != NULL) csound->Free(csound, (*dag)->table);
    if ((*dag)->remaining_count_ori != NULL)
      csound->Free(csound, (*dag)->remaining_count_ori);
    if ((*dag)->remaining_count!= NULL)
      csound->Free(csound, (*dag)->remaining_count);

    /* csound->Free(csound, (*dag)->weight); */

    csound->Free(csound, *dag);
    *dag = NULL;
}

static void dag_node_2_alloc(CSOUND *csound, DAG_NODE **dag_node,
                             INSTR_SEMANTICS *instr, INSDS *insds)
{
#ifdef CAUTIOUS
    if (UNLIKELY(dag_node == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter dag_node"));
    if (UNLIKELY(instr == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter instr"));
    if (UNLIKELY(insds == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter insds"));
#endif

    *dag_node = csound->Malloc(csound, sizeof(DAG_NODE));
    if (UNLIKELY(*dag_node == NULL)) {
      csound->Die(csound, Str("Failed to allocate dag_node"));
    }
    memset(*dag_node, 0, sizeof(DAG_NODE));
    strncpy((*dag_node)->hdr.hdr, DAG_NODE_2_HDR, HDR_LEN);
    (*dag_node)->hdr.type = DAG_NODE_INDV;
    (*dag_node)->instr = instr;
    (*dag_node)->insds = insds;
}

static void dag_node_2_alloc_list(CSOUND *csound,
                                  DAG_NODE **dag_node, int count)
{
#ifdef CAUTIOUS
    if (dag_node == NULL)
      csound->Die(csound, Str("Invalid NULL Parameter dag_node"));
    if (count <= 0)
      csound->Die(csound,
                  Str("Invalid Parameter count must be greater than 0"));
#endif

    *dag_node = csound->Malloc(csound, sizeof(DAG_NODE));
    if (*dag_node == NULL) {
      csound->Die(csound, Str("Failed to allocate dag_node"));
    }
    memset(*dag_node, 0, sizeof(DAG_NODE));
    strncpy((*dag_node)->hdr.hdr, DAG_NODE_2_HDR, HDR_LEN);
    (*dag_node)->hdr.type = DAG_NODE_LIST;
    (*dag_node)->nodes = csound->Malloc(csound,
                                        sizeof(DAG_NODE *) * count);
    (*dag_node)->count = count;
}

static void dag_node_2_dealloc(CSOUND *csound, DAG_NODE **dag_node)
{
#ifdef CAUTIOUS
    if (UNLIKELY(dag_node == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter dag_node"));
    if (UNLIKELY(*dag_node == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter dag_node"));
#endif

    if ((*dag_node)->hdr.type == DAG_NODE_LIST) {
      csound->Free(csound, (*dag_node)->nodes);
    }

    csound->Free(csound, *dag_node);
    *dag_node = NULL;
}

void csp_dag_add(CSOUND *csound, DAG *dag,
                 INSTR_SEMANTICS *instr, INSDS *insds)
{
    DAG_NODE *dag_node = NULL;
    DAG_NODE **old = dag->all;
    //int ctr = 0;
    dag_node_2_alloc(csound, &dag_node, instr, insds);

    TRACE_1("dag->count = %d\n", dag->count);
    //    dag->all = 
    //     (DAG_NODE **)csound->Malloc(csound,
    //                                 sizeof(DAG_NODE *) * (dag->count + 1));
    /* Can not this be done with memcpy or Realloc */
    //    while (ctr < dag->count) {
    //      dag->all[ctr] = old[ctr];
    //      ctr++;
    //    }
    //    dag->all[ctr] = dag_node;
    //    if (old != NULL) csound->Free(csound, old);
    dag->all =
      (DAG_NODE **)csound->ReAlloc(csound, old,
                                   sizeof(DAG_NODE *) * (dag->count + 1));
    dag->all[dag->count++] = dag_node;
    //dag->count++;

    if (dag->count == 1) {
      dag->insds_chain_start = dag->all[0];
    }
    else if (dag->count > 1) {
      dag->all[dag->count - 2]->insds_chain_next = dag->all[dag->count - 1];
    }
}

inline static void csp_dag_build_prepare(CSOUND *csound, DAG *dag)
{
#ifdef CAUTIOUS
    if (dag == NULL) csound->Die(csound,
                                 Str("Invalid NULL Parameter dag"));
#endif

    if (dag->roots_ori       != NULL) csound->Free(csound, dag->roots_ori);
    if (dag->roots           != NULL) csound->Free(csound, dag->roots);
    if (dag->root_seen_ori   != NULL)
      csound->Free(csound, dag->root_seen_ori);
    if (dag->root_seen       != NULL) csound->Free(csound, dag->root_seen);
    if (dag->remaining_count_ori != NULL)
      csound->Free(csound, dag->remaining_count_ori);
    if (dag->remaining_count != NULL)
      csound->Free(csound, dag->remaining_count);
    if (dag->table_ori       != NULL) csound->Free(csound, dag->table_ori);
    if (dag->table           != NULL) csound->Free(csound, dag->table);

    if (dag->count == 0) {
      dag->roots_ori         = NULL;
      dag->roots             = NULL;
      dag->root_seen_ori     = NULL;
      dag->root_seen         = NULL;
      dag->remaining_count_ori = NULL;
      dag->remaining_count   = NULL;
      dag->table_ori         = NULL;
      dag->table             = NULL;
      return;
    }

    dag->roots_ori =
      csound->Malloc(csound, sizeof(DAG_NODE *) * dag->count);
    dag->roots = csound->Malloc(csound, sizeof(DAG_NODE *) * dag->count);
    dag->root_seen_ori = csound->Malloc(csound, sizeof(uint8_t) * dag->count);
    dag->root_seen     = csound->Malloc(csound, sizeof(uint8_t) * dag->count);
    dag->remaining_count_ori =
      csound->Malloc(csound, sizeof(int) * dag->count);
    dag->remaining_count     =
      csound->Malloc(csound, sizeof(int) * dag->count);
    {
      long int ss = (sizeof(uint8_t *) * dag->count) +
                    (sizeof(uint8_t) * dag->count * dag->count);
      dag->table_ori = csound->Malloc(csound, ss);
      ss = (sizeof(uint8_t *) * dag->count) +
           (sizeof(uint8_t) * dag->count * dag->count);
    dag->table = csound->Malloc(csound, ss);
    }
    /* set up pointers for 2D array */
    {
      int ctr = 0;
      while (ctr < dag->count) {
        dag->table_ori[ctr] = ((uint8_t *)dag->table_ori) +
          (sizeof(uint8_t *) * dag->count) +
          (sizeof(uint8_t) * dag->count * ctr);
        dag->table[ctr]     = ((uint8_t *)dag->table) +
          (sizeof(uint8_t *) * dag->count) +
          (sizeof(uint8_t) * dag->count * ctr);
        ctr++;
      }
    }
    memset(dag->roots_ori, 0,        sizeof(DAG_NODE *) * dag->count);
    memset(dag->roots,     0,        sizeof(DAG_NODE *) * dag->count);
    memset(dag->root_seen_ori, 0,    sizeof(uint8_t) * dag->count);
    memset(dag->root_seen, 0,        sizeof(uint8_t) * dag->count);
    memset(dag->remaining_count_ori, 0, sizeof(int) * dag->count);
    memset(dag->remaining_count,     0, sizeof(int) * dag->count);
    memset(dag->table_ori[0], DAG_NO_LINK,
           sizeof(uint8_t) * dag->count * dag->count);
    memset(dag->table[0], DAG_NO_LINK,
           sizeof(uint8_t) * dag->count * dag->count);
}

inline static DAG *csp_dag_build_initial(CSOUND *csound, INSDS *chain)
{
    DAG *dag = NULL;
#ifdef CAUTIOUS
    if (UNLIKELY(chain == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter chain"));
#endif
    csp_dag_alloc(csound, &dag);
    while (chain != NULL) {
      INSTR_SEMANTICS *current_instr =
        csp_orc_sa_instr_get_by_num(csound, chain->insno);
      if (current_instr == NULL)
        csound->Die(csound,
                    Str("Failed to find semantic information"
                        " for instrument '%i'"),
                        chain->insno);
      csp_dag_add(csound, dag, current_instr, chain);
      dag->weight += current_instr->weight;
      chain = chain->nxtact;
    }
    return dag;
}

inline static void csp_dag_build_edges(CSOUND *csound, DAG *dag)
{
#ifdef CAUTIOUS
    if (UNLIKELY(dag == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter dag"));
#endif

    int dag_root_ctr = 0;
    while (dag_root_ctr < dag->count) {
      int dag_curr_ctr = dag_root_ctr + 1;
      while (dag_curr_ctr < dag->count) {

        /* csound->Message(csound, "=== %s <> %s ===\n",
            dag->all[dag_root_ctr]->instr->name,
            dag->all[dag_curr_ctr]->instr->name); */

        int depends = DAG_NO_LINK;
        struct set_t *write_intersection = NULL;
        csp_set_intersection(csound, dag->all[dag_root_ctr]->instr->write,
                             dag->all[dag_curr_ctr]->instr->read,
                             &write_intersection);
        if (csp_set_count(csound, write_intersection) != 0) {
          depends |= DAG_STRONG_LINK;
        }
        csp_set_dealloc(csound, &write_intersection);

        /* csound->Message(csound, 
                           "write_intersection depends: %i\n", depends);
           csp_set_print(csound, dag->all[dag_root_ctr]->instr->write);
           csp_set_print(csound, dag->all[dag_curr_ctr]->instr->read); */

        struct set_t *read_intersection = NULL;
        csp_set_intersection(csound, dag->all[dag_root_ctr]->instr->read,
                             dag->all[dag_curr_ctr]->instr->write,
                             &read_intersection);
        if (csp_set_count(csound, read_intersection) != 0) {
          depends |= DAG_STRONG_LINK;
        }
        csp_set_dealloc(csound, &read_intersection);

        /* csound->Message(csound,
                           "read_intersection depends: %i\n", depends);
           csp_set_print(csound, dag->all[dag_root_ctr]->instr->read);
           csp_set_print(csound, dag->all[dag_curr_ctr]->instr->write); */

        struct set_t *double_write_intersection = NULL;
        csp_set_intersection(csound, dag->all[dag_root_ctr]->instr->write,
                             dag->all[dag_curr_ctr]->instr->write,
                             &double_write_intersection);
        if (csp_set_count(csound, double_write_intersection) != 0) {
          depends |= DAG_STRONG_LINK;
        }
        csp_set_dealloc(csound, &double_write_intersection);

        /* csound->Message(csound, "double_write_intersection depends: %i\n",
                           depends);
           csp_set_print(csound, dag->all[dag_root_ctr]->instr->read);
           csp_set_print(csound, dag->all[dag_curr_ctr]->instr->write); */

        struct set_t *readwrite_write_intersection = NULL;
        csp_set_intersection(csound,
                             dag->all[dag_root_ctr]->instr->read_write,
                             dag->all[dag_curr_ctr]->instr->write,
                             &readwrite_write_intersection);
        if (csp_set_count(csound, readwrite_write_intersection) != 0) {
          depends |= DAG_STRONG_LINK;
        }
        csp_set_dealloc(csound, &readwrite_write_intersection);

        /* csound->Message(csound, 
                           "readwrite_write_intersection depends: %i\n",
                           depends);
           csp_set_print(csound, dag->all[dag_root_ctr]->instr->read_write);
           csp_set_print(csound, dag->all[dag_curr_ctr]->instr->write); */

        struct set_t *readwrite_read_intersection = NULL;
        csp_set_intersection(csound, 
                             dag->all[dag_root_ctr]->instr->read_write,
                             dag->all[dag_curr_ctr]->instr->read,
                             &readwrite_read_intersection);
        if (csp_set_count(csound, readwrite_read_intersection) != 0) {
          depends |= DAG_STRONG_LINK;
        }
        csp_set_dealloc(csound, &readwrite_read_intersection);

        /* csound->Message(csound,
                           "readwrite_read_intersection depends: %i\n",
                           depends);
           csp_set_print(csound, 
                         dag->all[dag_root_ctr]->instr->read_write);
           csp_set_print(csound, dag->all[dag_curr_ctr]->instr->write); */

        struct set_t *read_readwrite_intersection = NULL;
        csp_set_intersection(csound, dag->all[dag_root_ctr]->instr->read,
                             dag->all[dag_curr_ctr]->instr->read_write,
                             &read_readwrite_intersection);
        if (csp_set_count(csound, read_readwrite_intersection) != 0) {
          depends |= DAG_STRONG_LINK;
        }
        csp_set_dealloc(csound, &read_readwrite_intersection);

        /* csound->Message(csound, 
                           "read_readwrite_intersection depends: %i\n",
                           depends);
           csp_set_print(csound, dag->all[dag_root_ctr]->instr->read);
           csp_set_print(csound, 
                         dag->all[dag_curr_ctr]->instr->read_write); */

        struct set_t *write_readwrite_intersection = NULL;
        csp_set_intersection(csound, dag->all[dag_root_ctr]->instr->write,
                             dag->all[dag_curr_ctr]->instr->read_write,
                             &write_readwrite_intersection);
        if (csp_set_count(csound, write_readwrite_intersection) != 0) {
          depends |= DAG_STRONG_LINK;
        }
        csp_set_dealloc(csound, &write_readwrite_intersection);

        /* csound->Message(csound,
                           "write_readwrite_intersection depends: %i\n",
                           depends);
           csp_set_print(csound, dag->all[dag_root_ctr]->instr->write);
           csp_set_print(csound,
                          dag->all[dag_curr_ctr]->instr->read_write); */

        struct set_t *readwrite_readwrite_intersection = NULL;
        csp_set_intersection(csound,
                             dag->all[dag_root_ctr]->instr->read_write,
                             dag->all[dag_curr_ctr]->instr->read_write,
                             &readwrite_readwrite_intersection);
        if (csp_set_count(csound,
                          readwrite_readwrite_intersection) != 0) {
          depends |= DAG_WEAK_LINK;
        }
        csp_set_dealloc(csound, &readwrite_readwrite_intersection);

        /* csound->Message(csound,
                           "readwrite_readwrite_intersection depends: %i\n",
                           depends);
           csp_set_print(csound, dag->all[dag_root_ctr]->instr->read_write);
           csp_set_print(csound, 
                         dag->all[dag_curr_ctr]->instr->read_write); */

        if (depends & DAG_STRONG_LINK) {
          dag->table_ori[dag_root_ctr][dag_curr_ctr] = DAG_STRONG_LINK;
        }
        else if (depends & DAG_WEAK_LINK) {
          dag->table_ori[dag_root_ctr][dag_curr_ctr] = DAG_WEAK_LINK;
        }
        dag_curr_ctr++;
      }
      dag_root_ctr++;
    }
}

static void csp_dag_build_roots(CSOUND *csound, DAG *dag)
{
    int col = 0;
#ifdef CAUTIOUS
    if (dag == NULL) csound->Die(csound, Str("Invalid NULL Parameter dag"));
#endif

    while (col < dag->count) {
      int exists_in = 0;
      int row = 0;
      while (row < dag->count) {
        if (dag->table_ori[row][col] == DAG_STRONG_LINK) {
          exists_in = 1;
          dag->remaining_count_ori[col]++;
        }
        row++;
      }
      if (exists_in == 0) {
        dag->roots_ori[col] = dag->all[col];
        dag->root_seen_ori[col] = 1;
        if (dag->first_root_ori == -1) {
          dag->first_root_ori = col;
        }
      }
      col++;
    }
}

inline static void csp_dag_prepare_use_first(CSOUND *csound, DAG *dag)
{

    memcpy(dag->table[0], dag->table_ori[0],
           sizeof(uint8_t) * dag->count * dag->count);
}

static void csp_dag_prepare_use(CSOUND *csound, DAG *dag)
{
 #ifdef CAUTIOUS
    if (dag == NULL) csound->Die(csound, Str("Invalid NULL Parameter dag"));
#endif

    memcpy(dag->roots, dag->roots_ori, sizeof(DAG_NODE *) * dag->count);
    memcpy(dag->root_seen, dag->root_seen_ori, sizeof(uint8_t) * dag->count);
    memcpy(dag->remaining_count,
           dag->remaining_count_ori, sizeof(int) * dag->count);
    dag->remaining   = dag->count;
    dag->first_root  = dag->first_root_ori;
    dag->root_seen[dag->first_root] = 2;
}

static inline void csp_dag_prepare_use_insds(CSOUND *csound,
                                             DAG *dag, INSDS *chain)
{
    DAG_NODE *node;
#ifdef CAUTIOUS
    if (dag == NULL) csound->Die(csound, Str("Invalid NULL Parameter dag"));
#endif

    TRACE_2("updating insds\n");
    node = dag->insds_chain_start;
    INSDS *current_insds = chain;
    while (node != NULL && current_insds != NULL) {
      TRACE_2("  node: %p, insds: %p\n", node, current_insds);
      node->insds = current_insds;
      current_insds = current_insds->nxtact;
      node = node->insds_chain_next;
    }
}

static void csp_dag_calculate_max_roots(CSOUND *csound, DAG *dag)
{
    INSTR_SEMANTICS *instr = NULL;
    INSDS *insds = NULL;
    DAG_NODE *node;
    int update_hdl = -1;

#ifdef CAUTIOUS
    if (dag == NULL) csound->Die(csound, Str("Invalid NULL Parameter dag"));
#endif

    while (!csp_dag_is_finished(csound, dag)) {
      int ctr = 0, roots_avail = 0;
      while (ctr < dag->count) {
        if (dag->roots[ctr] != NULL) {
          roots_avail++;
        }
        ctr++;
      }
      if (roots_avail > dag->max_roots) {
        dag->max_roots = roots_avail;
      }
      csp_dag_consume(csound, dag, &node, &update_hdl);
      instr = node->instr;
      insds = node->insds;

      if (insds != NULL) {
        csp_dag_consume_update(csound, dag, update_hdl);
      }
    }
}

void csp_dag_build(CSOUND *csound, DAG **dag, INSDS *chain)
{
#ifdef CAUTIOUS
    if (dag == NULL)
      csound->Die(csound, Str("Invalid NULL Parameter dag"));
    if (chain == NULL)
      csound->Die(csound, Str("Invalid NULL Parameter chain"));
#endif

    TRACE_5("DAG BUILD\n");
    *dag = csp_dag_build_initial(csound, chain);
    csp_dag_build_prepare(csound, *dag);
    csp_dag_build_edges(csound, *dag);
    csp_dag_build_roots(csound, *dag);
    csp_dag_prepare_use_first(csound, *dag);
    csp_dag_prepare_use(csound, *dag);

    csp_dag_calculate_max_roots(csound, *dag);
    csp_dag_prepare_use(csound, *dag);
    TRACE_5("DAG BUILT\n");
}

int inline csp_dag_is_finished(CSOUND *csound, DAG *dag)
{
#ifdef CAUTIOUS
    if (dag == NULL)
      csound->Die(csound, Str("Invalid NULL Parameter dag"));
#endif
    /* TAKE_LOCK(&(dag->spinlock));
       int res = (dag->remaining <= 0);
       RELS_LOCK(&(dag->spinlock));
       return res;
    */
    TRACE_5("DAG is finished %d\n", dag->remaining <= 0);
    return (dag->remaining <= 0);
}

/*
 * consume an instr and update the first root cache
 */
//static int waiting_for_ending=0;
void csp_dag_consume(CSOUND *csound, DAG *dag,
                     DAG_NODE **node, int *update_hdl)
{
    DAG_NODE *dag_node = NULL;
    int ctr, first_root;

    TRACE_5("DAG consume\n");
#ifdef CAUTIOUS
    if (UNLIKELY(dag == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter dag"));
    if (UNLIKELY(node == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter node"));
    if (UNLIKELY(update_hdl == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter update_hdl"));
#endif

    TRACE_2("[%i] Consuming PreLock [%i, %i] +++++\n",
            csp_thread_index_get(csound),
            dag->first_root, dag->consume_spinlock);

    TAKE_LOCK(&(dag->consume_spinlock));

    TRACE_2("[%i] Consuming Have Consume_Spinlock [%i]\n",
            csp_thread_index_get(csound), dag->first_root);

    if (dag->remaining <= 0) {
      TRACE_5("[%i] Consuming Nothing [%i, %i]\n",
              csp_thread_index_get(csound), dag->first_root,
              dag->consume_spinlock);
      RELS_LOCK(&(dag->consume_spinlock));
      /* RELS_LOCK(&(dag->spinlock)); */
      *node = NULL;
      *update_hdl = -1;
      return;
    }
    if (UNLIKELY(dag->first_root == -1)) {

      //      csp_dag_print(csound, dag);

      /* csound->Die(csound, */
      /*     Str("Expected a root to perform. Found none (%i remaining)"), */
      /*     dag->remaining); */
      RELS_LOCK(&(dag->consume_spinlock));
      /* RELS_LOCK(&(dag->spinlock)); */
      *node = NULL;
      *update_hdl = -1;
      //{ struct timespec tt = {0, 100};
        //        nanosleep(&tt, NULL);
      //}
      /* Really ought to wait until someone leaves comsume_dag_update */
      return;
    }

    first_root = dag->first_root;

    TRACE_5("[%i] Consuming root:%i\n",
            csp_thread_index_get(csound), first_root);

    dag_node = dag->roots[first_root];
    dag->roots[first_root] = NULL;
    ctr = 0;
    dag->remaining--;
    dag->first_root = -1;

    if (dag->remaining > 0) {
      while (ctr < dag->count) {
        if (dag->roots[ctr] != NULL) {
          dag->first_root = ctr;
          if (dag->root_seen[ctr] == 1) {
            dag->root_seen[ctr] = 2;
            TRACE_5("[%i] Consuming Unlock [%i] -----\n",
                    csp_thread_index_get(csound), dag->first_root);
            //            RELS_LOCK(&(dag->consume_spinlock));
          }
          break;
        }
        ctr++;
      }
    }
    else {
      //      RELS_LOCK(&(dag->consume_spinlock));
    }

    RELS_LOCK(&(dag->consume_spinlock));

    *node = dag_node;
    *update_hdl = first_root;

    TRACE_5("[%i] Consuming Leave [%i]\n",
            csp_thread_index_get(csound), dag->first_root);
}

/*
 * update the roots countdown and roots after consumeing a node
 */
void csp_dag_consume_update(CSOUND *csound, DAG *dag, int update_hdl)
{
    int col = 0;
    TRACE_5("DAG update\n");
#ifdef CAUTIOUS
    if (UNLIKELY(dag == NULL))
      csound->Die(csound,Str( "Invalid NULL Parameter dag"));
    if (UNLIKELY(update_hdl < 0 || update_hdl >= dag->count))
      csound->Die(csound,
                  Str("Invalid Parameter update_hdl is outside the DAG range"));
#endif

    TAKE_LOCK(&(dag->consume_spinlock));
    TRACE_2("[%i] Consuming Update [%i, %i]\n",
            csp_thread_index_get(csound), dag->first_root, dag->consume_spinlock);

    while (col < dag->count) {
      if (dag->table[update_hdl][col] == DAG_STRONG_LINK) {
        dag->remaining_count[col]--;
        TRACE_5("[%i] Consuming Remaining (%i, %i) [%i, %i]\n",
                csp_thread_index_get(csound), col, dag->remaining_count[col],
                dag->first_root, dag->consume_spinlock);

        if (dag->remaining_count[col] == 0) {
          //          TAKE_LOCK(&(dag->spinlock));
          TRACE_5("[%i] Consuming Found Root [%i]\n",
                  csp_thread_index_get(csound), dag->first_root);

          if (dag->root_seen[col] == 0) {
            dag->root_seen[col] = 1;
            dag->roots[col] = dag->all[col];
            TRACE_5("[%i] Consuming First Root Set [%i]\n",
                    csp_thread_index_get(csound), dag->first_root);
          }

          if (dag->root_seen[col] == 1 && dag->first_root == -1) {
            dag->first_root = col;

            dag->root_seen[col] = 2;
            TRACE_3("[%i] Consuming Unlock [%i] -----\n",
                    csp_thread_index_get(csound), dag->first_root);
            //            RELS_LOCK(&(dag->consume_spinlock));
          }
          //RELS_LOCK(&(dag->spinlock));
        }
      }
      col++;
    }
    RELS_LOCK(&(dag->consume_spinlock));

    TRACE_2("[%i] Consuming Update Leave [%i, %i]\n",
            csp_thread_index_get(csound), dag->first_root, dag->consume_spinlock);
}

static char *csp_dag_string(CSOUND *csound, DAG *dag)
{
#define DAG_2_BUF 8196
    char buf[DAG_2_BUF];
    char *bufp = buf;

#ifdef CAUTIOUS
    if (dag == NULL) csound->Die(csound, Str("Invalid NULL Parameter dag"));
#endif

    bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "Dag2:\n");
    int ctr = 0;
    while (ctr < dag->count) {
      if (dag->all[ctr]->hdr.type == DAG_NODE_INDV) {
        bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "  %s [%p]\n",
                               dag->all[ctr]->instr->name, dag->all[ctr]);
      }
      else if (dag->all[ctr]->hdr.type == DAG_NODE_LIST) {
        bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "  ");
        int inner_ctr = 0;
        while (inner_ctr < dag->all[ctr]->count) {
          bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "%s [%p] ",
                                 dag->all[ctr]->nodes[inner_ctr]->instr->name,
                                 dag->all[ctr]->nodes[inner_ctr]);
          inner_ctr++;
        }
        bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "\n");
      }
      ctr++;
    }

    bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "roots:\n");
    ctr = 0;
    while (ctr < dag->count) {
      if (dag->roots[ctr] != NULL) {
        if (dag->all[ctr]->hdr.type == DAG_NODE_INDV) {
          bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "  %s [%p]\n",
                                 dag->roots[ctr]->instr->name, dag->roots[ctr]);
        }
        else if (dag->all[ctr]->hdr.type == DAG_NODE_LIST) {
          bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "  ");
          int inner_ctr = 0;
          while (inner_ctr < dag->roots[ctr]->count) {
            bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "%s [%p] ",
                                   dag->roots[ctr]->nodes[inner_ctr]->instr->name,
                                   dag->roots[ctr]->nodes[inner_ctr]);
            inner_ctr++;
          }
          bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "\n");
        }
      }
      ctr++;
    }

    bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "root_seen:\n ");
    ctr = 0;
    while (ctr < dag->count) {
      bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), " %hhu ",
                             dag->root_seen[ctr]);
      ctr++;
    }
    bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "\n");

    bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "remaining:\n");
    ctr = 0;
    while (ctr < dag->count) {
      bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "  %i\n",
                             dag->remaining_count[ctr]);
      ctr++;
    }

    bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf),
                           "remaining:      %i\n",  dag->remaining);
    bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf),
                           "first_root:     %i\n", dag->first_root);

    bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "table:\n");
    ctr = 0;
    while (ctr < dag->count) {
      int inner_ctr = 0;
      while (inner_ctr < dag->count) {
        bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "%hhi ",
                               dag->table[ctr][inner_ctr]);
        inner_ctr++;
      }
      bufp = bufp + snprintf(bufp, DAG_2_BUF - (bufp - buf), "\n");
      ctr++;
    }

    return strdup(buf);
}

void csp_dag_print(CSOUND *csound, DAG *dag)
{
    char *str = csp_dag_string(csound, dag);
    if (str != NULL) {
      csound->Message(csound, "%s", str);
      free(str);
    }
#if 0
    if (dag == NULL) csound->Die(csound,Str( "Invalid NULL Parameter dag"));

    csound->Message(csound, "Dag2:\n");
    int ctr = 0;
    while (ctr < dag->count) {
      csound->Message(csound, "  %s\n", dag->all[ctr]->instr->name);
      ctr++;
    }

    csound->Message(csound, "roots:\n");
    ctr = 0;
    while (ctr < dag->count) {
      if (dag->roots[ctr] != NULL) {
        csound->Message(csound, "  %s\n", dag->roots[ctr]->instr->name);
      }
      ctr++;
    }

    csound->Message(csound, "remaining:      %i\n",  dag->remaining);
    csound->Message(csound, "first_root:     %i\n", dag->first_root);
    /* csound->Message(csound, "consume_locked: %i\n", dag->consume_locked); */

    csound->Message(csound, "table:\n");
    ctr = 0;
    while (ctr < dag->count) {
      int inner_ctr = 0;
      while (inner_ctr < dag->count) {
        csound->Message(csound, "%hhi ", dag->table[ctr][inner_ctr]);
        inner_ctr++;
      }
      csound->Message(csound, "\n");
      ctr++;
    }
#endif
}

/**********************************************************************************************
 * dag2 optimization structure
 */
#pragma mark -
#pragma mark Dag2 optimization

static uint64_t dag_opt_counter;

/* attempt to optimize the dag
 * this is basis of the combined design */
void csp_dag_optimization(CSOUND *csound, DAG *dag)
{
    int starting_row = 0;
    int threads = csound->oparms->numThreads;
    int end_point = dag->count - threads;
    int dag_table_original_size = dag->count;

#ifdef CAUTIOUS
    if (UNLIKELY(dag == NULL))
      csound->Die(csound,Str( "Invalid NULL Parameter dag"));
#endif

#if TRACE > 1
    TRACE_0("========== Start ==========\n");
    csp_dag_print(csound, dag);
#endif

    /* loop through from start to the end of all the rows */
    while (starting_row < end_point) {
      TRACE_2("Start Loop\n");
      /* DAG_NODE *readwrite_group[dag->count];
         memset(readwrite_group, 0, sizeof(DAG_NODE *) * dag->count); */
      int readwrite_group[dag->count];

      /* try and find sqaures of target side length
       * decrease target until we find one or its smaller than
       * the number of threads
       *
       * the target square will have instruments performable at the same time
       * we place the indexes of the instruments in readwrite_group */
      int found_block = 0;
      int target = dag->count - starting_row;
      while (target >= threads && found_block == 0) {
        TRACE_2("Start Loop Target target: %i\n", target);
        TRACE_2("end_point: %i\n", end_point);
        TRACE_2("starting_row: %i\n", starting_row);

        int count_matching_cols = 0;
        memset(readwrite_group, -1, sizeof(int) * dag->count);

        TRACE_2("readwrite_group done\n");

        int row = starting_row, col = starting_row;
        while (col < dag->count) {
          row = starting_row;
          while (row < dag->count &&
                 (dag->table_ori[row][col] == DAG_WEAK_LINK ||
                  dag->table_ori[row][col] == DAG_NO_LINK)) {
            row++;
          }
          TRACE_2("row: %i\n", row - starting_row);
          if (UNLIKELY((row - starting_row) >= target)) {
            readwrite_group[count_matching_cols] = col;
            count_matching_cols++;
          }
          col++;
        }
        TRACE_2("count_matching_cols: %i\n", count_matching_cols);
        if (UNLIKELY(count_matching_cols >= target)) {
          /* we have a target x target sized square somewhere inside */
          found_block = 1;
        }
        else {    /* go around again looking for a smaller target */
          target--;
        }
      }

      TRACE_2("Found Target? %i (%i)\n", target > threads, target);

      /* we've found a target square */
      if (target > threads) {
        dag_opt_counter++;

        /* replace starting_row..starting_row+target with a new row */
        int ctr = 0;

        /* copy the original dependency table */
        uint8_t table_copy[dag_table_original_size][dag_table_original_size];
        memcpy(table_copy, dag->table_ori[0],
               sizeof(uint8_t) * dag_table_original_size * dag_table_original_size);

        TRACE_2("Table Copy\n");
#if TRACE > 1
        ctr = 0;
        while (ctr < dag->count) {
          int col_ctr = 0;
          while (col_ctr < dag->count) {
            csound->Message(csound, "%i ", table_copy[ctr][col_ctr]);
            col_ctr++;
          }
          csound->Message(csound, "\n");
          ctr++;
        }
#endif

        /* stream data structure */
        int streams[threads][dag->count];
        memset(streams, -1, sizeof(int) * threads * dag->count);
        int streams_ix[threads];
        memset(streams_ix, 0, sizeof(int) * threads);
        int readwrite_group_copy[target];
        memcpy(readwrite_group_copy, readwrite_group, sizeof(int) * target);

        TRACE_2("Copied Everything\n");
#if TRACE > 1
        ctr = 0;
        while (ctr < target) {
          csound->Message(csound, "%i: %i\n", ctr, readwrite_group_copy[ctr]);
          ctr++;
        }
#endif

        /* fill out the streams data structure which we use as
           instructions to build the new nodes */
        ctr = 0;
        while (ctr < target) {
          int current_thread = 0;
          while (ctr < target && current_thread < threads) {
            int inner_ctr = 0, max_ix = 0;
            uint32_t max_weight = 0;
            while (inner_ctr < target) {
              if (readwrite_group_copy[inner_ctr] >= 0 &&
                  dag->all[readwrite_group_copy[inner_ctr]]->instr->weight
                  > max_weight) {
                max_weight =
                  dag->all[readwrite_group_copy[inner_ctr]]->instr->weight;
                max_ix = inner_ctr;
              }
              inner_ctr++;
            }
            streams[current_thread][streams_ix[current_thread]] =
              readwrite_group_copy[max_ix];
            readwrite_group_copy[max_ix] = -1;
            ctr++;
            streams_ix[current_thread]++;
            current_thread++;
          }
        }

        TRACE_2("Done Stream Instructions\n");
#if TRACE > 1
        ctr = 0;
        while (ctr < threads) {
          csound->Message(csound, "Stream %i: [%i]\n  ", ctr, streams_ix[ctr]);
          int stream_ix = 0;
          while (stream_ix < streams_ix[ctr]) {
            csound->Message(csound, "%i ", streams[ctr][stream_ix]);
            stream_ix++;
          }
          csound->Message(csound, "\n");
          ctr++;
        }
#endif

        /* allocate the replacement list dag_nodes */
        ctr = 0;
        DAG_NODE *new_nodes[threads];
        memset(new_nodes, 0, sizeof(DAG_NODE *) * threads);
        while (ctr < threads) {
          dag_node_2_alloc_list(csound, &(new_nodes[ctr]), streams_ix[ctr]);
          ctr++;
        }

        TRACE_2("Allocated all nodes\n");

        /* copy nodes from dag into array inside dag_node */
        ctr = 0;
        while (ctr < threads) {
          int inner_ctr = 0;
          while (inner_ctr < new_nodes[ctr]->count) {
            new_nodes[ctr]->nodes[inner_ctr] = dag->all[streams[ctr][inner_ctr]];
            dag->all[streams[ctr][inner_ctr]] = NULL;
            inner_ctr++;
          }
          ctr++;
        }

        TRACE_2("Copied all old node references into new nodes\n");

        int map_old_to_new_locations[dag->count];
        memset(map_old_to_new_locations, -1, sizeof(int) * dag->count);

        /* shuffle up all the nodes into the gaps left */
        ctr = 0;
        int streams_remaining = 0;
        while (ctr < dag->count) {
          /* we only move things if necessary ie we're in a spot where a
             node was removed for merging before */
          if (dag->all[ctr] == NULL) {
            if (streams_remaining < threads) {  /* slot new nodes in first */
              TRACE_2("Is merged node\n");
              dag->all[ctr] = new_nodes[streams_remaining];

              int stream_ctr = 0;
              while (stream_ctr < streams_ix[streams_remaining]) {
                map_old_to_new_locations[streams[streams_remaining][stream_ctr]] =
                  ctr;
                stream_ctr++;
              }

              streams_remaining++;
            }
            else {            /* slide down the rest of the nodes */
              int next = ctr + 1;
              TRACE_2("No more merged nodes\n");
              while (next < dag->count && dag->all[next] == NULL) {
                next++;
              }
              if (LIKELY(next < dag->count)) {
                TRACE_2("Normal Node\n");
                dag->all[ctr] = dag->all[next];
                dag->all[next] = NULL;

                map_old_to_new_locations[next] = ctr;
              }
              else {
                TRACE_2("Done copying normal nodes\n");
                /* we're done at this point no more nodes to copy */
                break;
              }
            }
          }
          else {
            map_old_to_new_locations[ctr] = ctr;
          }
          ctr++;
        }

        TRACE_2("Shuffled up all nodes\n");

#if TRACE > 1
        ctr = 0;
        while (ctr < dag->count) {
          csound->Message(csound, "%i -> %i\n", ctr, map_old_to_new_locations[ctr]);
          ctr++;
        }
#endif

          /* empty the table of dependencies */
        memset(dag->table_ori[0], 0,
               sizeof(uint8_t) * dag_table_original_size * dag_table_original_size);

        /* copy across the new dependency info */
        int update_col = 0, update_row = 0;
        while (update_col < dag->count) {
          update_row = 0;
          int working_col = map_old_to_new_locations[update_col];
          while (update_row < dag->count) {
            int working_row = map_old_to_new_locations[update_row];

            TRACE_2("(%i, %i) -> (%i, %i)\n", update_row, update_col,
                                              working_row, working_col);
            TRACE_2("%i -> %i\n", table_copy[update_row][update_col],
                                  dag->table_ori[working_row][working_col]);

            switch (table_copy[update_row][update_col]) {
            case DAG_STRONG_LINK:
              dag->table_ori[working_row][working_col] = DAG_STRONG_LINK;
              break;
            case DAG_WEAK_LINK:
              if (dag->table_ori[working_row][working_col] != DAG_STRONG_LINK) {
                dag->table_ori[working_row][working_col] = DAG_WEAK_LINK;
              }
              break;
            case DAG_NO_LINK:
              if (dag->table_ori[working_row][working_col] != DAG_STRONG_LINK &&
                  dag->table_ori[working_row][working_col] != DAG_WEAK_LINK) {
                dag->table_ori[working_row][working_col] = DAG_NO_LINK;
              }
              break;
            }
            TRACE_2("-> %i\n", dag->table_ori[working_row][working_col]);
            update_row++;
          }
          update_col++;
        }

        TRACE_2("Completed dependencies merge\n");

        /* update the dag->count */
        dag->count = dag->count - target + threads;
        starting_row = starting_row + threads;
        end_point = dag->count - threads;
      }
      else {
        /* not enough parallelism after this instrument */
        starting_row++;
      }
    }

    /* reset ready to redo roots and similar things */
    memcpy(dag->table[0], dag->table_ori[0],
           sizeof(uint8_t) * dag_table_original_size * dag_table_original_size);
    memset(dag->roots_ori, 0, sizeof(DAG_NODE *) * dag_table_original_size);
    dag->first_root_ori = -1;

    memset(dag->root_seen_ori, 0, sizeof(uint8_t) * dag_table_original_size);
    memset(dag->root_seen, 0, sizeof(uint8_t) * dag_table_original_size);
    memset(dag->remaining_count_ori, 0, sizeof(int) * dag_table_original_size);
    dag->max_roots = 0;

    csp_dag_build_roots(csound, dag);
    /* csp_dag_prepare_use_first(csound, dag); */
    csp_dag_prepare_use(csound, dag);

    csp_dag_calculate_max_roots(csound, dag);
    csp_dag_prepare_use(csound, dag);
    //    csp_dag_print(csound, dag);


#if TRACE > 1
    TRACE_0("========== New ==========\n");
    csp_dag_print(csound, dag);
#endif
}

/**********************************************************************************************
 * dag2 cache structure
 */
#pragma mark -
#pragma mark Dag2 Cache

/* #ifdef LINEAR_CACHE */

/* struct dag_cache_entry_t { */
/*   DAG              *dag; */
/*   uint32_t                    uses; */
/*   uint32_t                    age; */
/*   struct dag_cache_entry_t  *next; */
/*   int16                       instrs; */
/*   int16                       chain[]; */
/* }; */

/* static int cache_ctr; */
/* static struct dag_cache_entry_t *cache; */

/* static uint32_t update_ctr; */

/* #define DAG_2_CACHE_SIZE     100 */
/* #define DAG_2_DECAY_COMP     1 */
/* #define DAG_2_MIN_USE_LIMIT  5000 */
/* /\* aiming for 8 passes of the cache update before a new entry must exist solely on its usage *\/ */
/* #define DAG_2_MIN_AGE_LIMIT  256 */
/* #define DAG_2_AGE_START      131072 */

/* static int csp_dag_cache_entry_dealloc(CSOUND *csound, */
/*                                        struct dag_cache_entry_t **entry); */
/* static int csp_dag_cache_entry_alloc(CSOUND *csound, */
/*                                      struct dag_cache_entry_t **entry, */
/*                                      INSDS *chain); */
/* static int csp_dag_cache_compare(CSOUND *csound */
/*                                  , struct dag_cache_entry_t *entry, INSDS *chain); */
/* static void csp_dag_cache_update(CSOUND *csound); */

/* void csp_dag_cache_print(CSOUND *csound) */
/* { */
/*     csound->Message(csound, "Dag2 Cache Size: %i\n", cache_ctr); */
/*     uint32_t sum = 0, max = 0, min = UINT32_MAX, sum_age = 0; */
/*     struct dag_cache_entry_t *entry = cache, *prev = NULL; */
/*     while (entry != NULL) { */
/*       if (entry->uses > max) max = entry->uses; */
/*       else if (entry->uses < min) min = entry->uses; */
/*       sum = sum + entry->uses; */
/*       sum_age = sum_age + entry->age; */
/*       entry = entry->next; */
/*     } */
/*     csound->Message(csound, "Dag2 Avg Uses: %u\n", sum / cache_ctr); */
/*     csound->Message(csound, "Dag2 Min Uses: %u\n", min); */
/*     csound->Message(csound, "Dag2 Max Uses: %u\n", max); */
/*     csound->Message(csound, "Dag2 Avg Age: %u\n", sum_age / cache_ctr); */
/*     csound->Message(csound, "Dag2 Fetches:  %u\n", update_ctr); */
/* } */

/* static int csp_dag_cache_entry_alloc(CSOUND *csound, */
/*                                      struct dag_cache_entry_t **entry, INSDS *chain) */
/* { */
/*     int ctr = 0; */
/*     INSDS *current_insds = chain; */

/* #ifdef CAUTIOUS */
/*     if (UNLIKELY(entry == NULL)) */
/*       csound->Die(csound, Str("Invalid NULL Parameter entry")); */
/*     if (UNLIKELY(chain == NULL)) */
/*       csound->Die(csound, Str("Invalid NULL Parameter chain")); */
/* #endif */

/*     while (current_insds != NULL) { */
/*       ctr++; */
/*       current_insds = current_insds->nxtact; */
/*     } */

/*     *entry = csound->Malloc(csound, */
/*                             sizeof(struct dag_cache_entry_t) + sizeof(int16) * ctr); */
/*     if (UNLIKELY(*entry == NULL)) { */
/*       csound->Die(csound, Str("Failed to allocate Dag2 cache")); */
/*     } */
/*     memset(*entry, 0, sizeof(struct dag_cache_entry_t) + sizeof(int16) * ctr); */
/*     (*entry)->uses   = 1; */
/*     (*entry)->age    = DAG_2_AGE_START; */
/*     (*entry)->instrs = ctr; */

/*     ctr = 0; */
/*     current_insds = chain; */
/*     while (current_insds != NULL) { */
/*       (*entry)->chain[ctr] = current_insds->insno; */
/*       ctr++; */
/*       current_insds = current_insds->nxtact; */
/*     } */

/*     DAG *dag = NULL; */
/*     csp_dag_build(csound, &dag, chain); */
/*     (*entry)->dag = dag; */

/*     return CSOUND_SUCCESS; */
/* } */

/* static int csp_dag_cache_entry_dealloc(CSOUND *csound, */
/*                                        struct dag_cache_entry_t **entry) */
/* { */
/* #ifdef CAUTIOUS */
/*     if (UNLIKELY(entry == NULL)) */
/*       csound->Die(csound, Str("Invalid NULL Parameter entry")); */
/*     if (UNLIKELY(*entry == NULL)) */
/*       csound->Die(csound, Str("Invalid NULL Parameter entry")); */
/* #endif */

/*     csp_dag_dealloc(csound, &((*entry)->dag)); */

/*     csound->Free(csound, *entry); */
/*     *entry = NULL; */

/*     return CSOUND_SUCCESS; */
/* } */

/* static void csp_dag_cache_update(CSOUND *csound) */
/* { */
/*     if (cache_ctr < DAG_2_CACHE_SIZE) { */
/*       return; */
/*     } */
/*     //    csound->Message(csound, Str("Cache Update\n")); */
/*     struct dag_cache_entry_t *entry = cache, *prev = NULL; */
/*     while (entry != NULL) { */
/*       entry->uses = entry->uses >> DAG_2_DECAY_COMP; */
/*       entry->age  = entry->age  >> DAG_2_DECAY_COMP; */
/*       if (entry->uses < DAG_2_MIN_USE_LIMIT && */
/*           entry->age < DAG_2_MIN_AGE_LIMIT && */
/*           prev != NULL) { */
/*         prev->next = entry->next; */
/*         csp_dag_cache_entry_dealloc(csound, &entry); */
/*         entry = prev->next; */
/*         cache_ctr--; */
/*       } */
/*       else { */
/*         prev = entry; */
/*         entry = entry->next; */
/*       } */
/*     } */
/* } */

/* static int csp_dag_cache_compare(CSOUND *csound, */
/*                                  struct dag_cache_entry_t *entry, INSDS *chain) */
/* { */
/*     INSDS *current_insds = chain; */
/*     int32_t ctr = 0; */

/* #ifdef CAUTIOUS */
/*     if (entry == NULL) csound->Die(csound, Str("Invalid NULL Parameter entry")); */
/*     if (chain == NULL) csound->Die(csound, Str("Invalid NULL Parameter chain")); */
/* #endif */

/*     while (current_insds != NULL && ctr < entry->instrs) { */
/*       if (current_insds->insno != entry->chain[ctr]) { */
/*         return 0; */
/*       } */
/*       current_insds = current_insds->nxtact; */
/*       ctr++; */
/*     } */
/*     if (ctr >= entry->instrs && current_insds != NULL) { */
/*       return 0; */
/*     } */
/*     else if (ctr < entry->instrs && current_insds == NULL) { */
/*       return 0; */
/*     } */
/*     else { */
/*       return 1; */
/*     } */
/* } */

/* void csp_dag_cache_fetch(CSOUND *csound, DAG **dag, INSDS *chain) */
/* { */
/*     struct dag_cache_entry_t *curr; */
/* #ifdef CAUTIOUS */
/* if (UNLIKELY(dag == NULL)) */
/*       csound->Die(csound, Str("Invalid NULL Parameter dag")); */
/*     if (UNLIKELY(chain == NULL)) */
/*       csound->Die(csound, Str("Invalid NULL Parameter chain")); */
/* #endif */

/*     update_ctr++;               /\* What does this do? *\/ */
/*     if (update_ctr == 10000) { */
/*       csp_dag_cache_update(csound); */
/*       update_ctr = 0; */
/*     } */

/*     curr = cache; */
/*     while (curr != NULL) { */
/*       if (UNLIKELY(csp_dag_cache_compare(csound, curr, chain))) { */
/*         TRACE_2("Cache Hit [%i]\n", cache_ctr); */
/*         *dag = curr->dag; */

/*         curr->uses++; */

/*         csp_dag_prepare_use_insds(csound, curr->dag, chain); */
/*         csp_dag_prepare_use(csound, curr->dag); */
/*         break; */
/*       } */
/*       curr = curr->next; */
/*     } */
/*     if (*dag == NULL) { */
/*       csp_dag_cache_entry_alloc(csound, &curr, chain); */
/*       cache_ctr++; */
/*       *dag = curr->dag; */
/*       curr->next = cache; */
/*       cache = curr; */

/*       TRACE_2("Cache Miss [%i]\n", cache_ctr); */
/*     } */
/* } */

/* #endif */


#ifdef HASH_CACHE

struct dag_cache_entry_t {
  uint32_t                    hash_val;
  struct dag_cache_entry_t   *next;
  DAG                        *dag;
  uint32_t                    uses;
  uint32_t                    age;

  int16                       instrs;
  int16                       chain[];
};

//#define DAG_2_CACHE_SIZE     128
#define DAG_2_DECAY_COMP     1
#define DAG_2_MIN_USE_LIMIT  5000
/* aiming for 8 passes of the cache update before a new entry
   must exist solely on its usage */
#define DAG_2_MIN_AGE_LIMIT  256
#define DAG_2_AGE_START      131072

static int cache_ctr;
//static struct dag_cache_entry_t *cache[DAG_2_CACHE_SIZE];

/* #ifdef HYBRID_HASH_CACHE */
/* static struct dag_cache_entry_t *cache_last; */
/* #endif */

static uint32_t update_ctr;

static int csp_dag_cache_entry_alloc(CSOUND *csound,
                                     struct dag_cache_entry_t **entry,
                                     INSDS *chain, uint32_t hash_val);
static int csp_dag_cache_compare(CSOUND *csound,
                                 struct dag_cache_entry_t *entry, INSDS *chain);
#if 0
static int csp_dag_cache_entry_dealloc(CSOUND *csound,
                                       struct dag_cache_entry_t **entry);
static void csp_dag_cache_update(CSOUND *csound);
#endif
static void csp_dag_cache_print_weights_dump(CSOUND *csound);

void csp_dag_cache_print(CSOUND *csound)
{
    uint32_t sum = 0, max = 0, min = UINT32_MAX, sum_age = 0;
    uint32_t weight_sum = 0, weight_max = 0, weight_min = UINT32_MAX;
    uint32_t instr_num_sum = 0, instr_num_max = 0, instr_num_min = UINT32_MAX;
    uint32_t root_avail_sum = 0, root_avail_max = 0;
    uint32_t root_avail_min = UINT32_MAX;
    uint32_t bin_ctr = 0, bins_empty = 0, bins_used = 0, bin_max = 0;

    csound->Message(csound, "Dag2 Cache Size: %i\n", cache_ctr);
    while (bin_ctr < DAG_2_CACHE_SIZE) {
      struct dag_cache_entry_t *entry = csound->cache[bin_ctr];

      if (entry == NULL) bins_empty++;
      else bins_used++;

      uint32_t entry_ctr = 0;
      while (entry != NULL) {
        entry_ctr++;
        if (entry->uses > max) max = entry->uses;
        else if (entry->uses < min) min = entry->uses;
        sum = sum + entry->uses;
        sum_age = sum_age + entry->age;

        weight_sum += entry->dag->weight;
        if (entry->dag->weight > weight_max) weight_max = entry->dag->weight;
        else if (entry->dag->weight < weight_min) weight_min = entry->dag->weight;

        instr_num_sum += entry->instrs;
        if (entry->instrs > instr_num_max) instr_num_max = entry->instrs;
        else if (entry->instrs < instr_num_min) instr_num_min = entry->instrs;

        root_avail_sum += entry->dag->max_roots;
        if (entry->dag->max_roots > root_avail_max)
          root_avail_max = entry->dag->max_roots;
        else if (entry->dag->max_roots < root_avail_min)
          root_avail_min = entry->dag->max_roots;

        entry = entry->next;
      }

      if (entry_ctr > bin_max) bin_max = entry_ctr;

      bin_ctr++;
    }
    csound->Message(csound, "Dag2 Avg Uses: %u\n", sum / cache_ctr);
    csound->Message(csound, "Dag2 Min Uses: %u\n", min);
    csound->Message(csound, "Dag2 Max Uses: %u\n", max);
    csound->Message(csound, "Dag2 Avg Age: %u\n", sum_age / cache_ctr);
    csound->Message(csound, "Dag2 Fetches:  %u\n", update_ctr);

    csound->Message(csound, "Dag2 Empty Bins:  %u\n", bins_empty);
    csound->Message(csound, "Dag2 Used Bins:  %u\n", bins_used);
    csound->Message(csound, "Dag2 Bins Max:  %u\n", bin_max);
    csound->Message(csound, "Dag2 Bins Avg:  %u\n", cache_ctr / bins_used);

    csound->Message(csound, "Weights Avg: %u\n", weight_sum / cache_ctr);
    csound->Message(csound, "Weights Min: %u\n", weight_min);
    csound->Message(csound, "Weights Max: %u\n", weight_max);
    csound->Message(csound, "Weights InstrNum Avg: %u\n",
                    instr_num_sum / cache_ctr);
    csound->Message(csound, "Weights InstrNum Min: %u\n", instr_num_min);
    csound->Message(csound, "Weights InstrNum Max: %u\n", instr_num_max);
    csound->Message(csound, "Roots Available Avg: %u\n",
                    root_avail_sum / cache_ctr);
    csound->Message(csound, "Roots Available Min: %u\n", root_avail_min);
    csound->Message(csound, "Roots Available Max: %u\n", root_avail_max);

    csound->Message(csound, "Number Optimized: %llu\n", dag_opt_counter);


    if (csound->weight_dump != NULL) {
      csp_dag_cache_print_weights_dump(csound);
    }
}

static void csp_dag_cache_print_weights_dump(CSOUND *csound)
{
    char *path = csound->weight_dump;
    FILE *f = fopen(path, "w+");
    uint32_t bin_ctr = 0;

#ifdef CAUTIOUS
    if (UNLIKELY(f == NULL)) {
      csound->Die(csound,
                  Str("Parallel Dump File not found at: %s for writing"), path);
    }
#endif

    while (bin_ctr < DAG_2_CACHE_SIZE) {
      struct dag_cache_entry_t *entry = csound->cache[bin_ctr];
      char *dag_str;

      while (entry != NULL) {
        DAG *dag = entry->dag;
        int ctr = 0;
        while (ctr < entry->instrs) {
          fprintf(f, "%hi", entry->chain[ctr]);
          if (ctr != entry->instrs - 1) {
            fprintf(f, ", ");
          }
          ctr++;
        }
        fprintf(f, "\n");
        fprintf(f, "%u\n", dag->weight);
        fprintf(f, "%u\n", dag->max_roots);

        dag_str = csp_dag_string(csound, dag);
        if (dag_str != NULL) {
          fprintf(f, "%s", dag_str);
          free(dag_str);
        }
        fprintf(f, "\n");
        entry = entry->next;
      }

      bin_ctr++;
    }

    fclose(f);
}

static int csp_dag_cache_entry_alloc(CSOUND *csound,
                                     struct dag_cache_entry_t **entry,
                                     INSDS *chain, uint32_t hash_val)
{
    int ctr = 0;
    INSDS *current_insds = chain;

#ifdef CAUTIOUS
    if (UNLIKELY(entry == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter entry"));
    if (UNLIKELY(chain == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter chain"));
#endif

    while (current_insds != NULL) {
      ctr++;
      current_insds = current_insds->nxtact;
    }

    *entry = csound->Malloc(csound,
                            sizeof(struct dag_cache_entry_t) +
                            sizeof(int16) * ctr);
    if (UNLIKELY(*entry == NULL)) {
      csound->Die(csound, Str("Failed to allocate Dag2 cache entry"));
    }
    memset(*entry, 0, sizeof(struct dag_cache_entry_t) + sizeof(int16) * ctr);
    (*entry)->uses   = 1;
    (*entry)->age    = DAG_2_AGE_START;
    (*entry)->instrs = ctr;
    (*entry)->hash_val = hash_val;

    ctr = 0;
    current_insds = chain;
    while (current_insds != NULL) {
      (*entry)->chain[ctr] = current_insds->insno;
      ctr++;
      current_insds = current_insds->nxtact;
    }

    {
      DAG *dag = NULL;
      csp_dag_build(csound, &dag, chain);
      (*entry)->dag = dag;
      csp_dag_optimization(csound, dag);
    }
    return CSOUND_SUCCESS;
}

#if 0
static int csp_dag_cache_entry_dealloc(CSOUND *csound,
                                       struct dag_cache_entry_t **entry)
{
#ifdef CAUTIOUS
    if (UNLIKELY(entry == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter entry"));
    if (UNLIKELY(*entry == NULL))
      csound->Die(csound, Str("Invalid NULL Parameter entry"));
#endif

    csp_dag_dealloc(csound, &((*entry)->dag));

    csound->Free(csound, *entry);
    *entry = NULL;

    return CSOUND_SUCCESS;
}
#endif

#if 0
static void csp_dag_cache_update(CSOUND *csound)
{
    uint32_t bin_ctr = 0;

    if (cache_ctr < DAG_2_CACHE_SIZE) {
      return;
    }
    csound->Message(csound, Str("Cache Update\n"));

    while (bin_ctr < DAG_2_CACHE_SIZE) {
      struct dag_cache_entry_t *entry = csound->cache[bin_ctr], *prev = NULL;

      if (entry == NULL) {
        bin_ctr++;
        continue;
      }

      while (entry != NULL) {
        entry->uses = entry->uses >> DAG_2_DECAY_COMP;
        entry->age  = entry->age  >> DAG_2_DECAY_COMP;
        if (entry->uses < DAG_2_MIN_USE_LIMIT &&
            entry->age < DAG_2_MIN_AGE_LIMIT) {
          if (prev == NULL) {
            csound->cache[bin_ctr] = entry->next;
          }
          else {
            prev->next = entry->next;
          }
          csp_dag_cache_entry_dealloc(csound, &entry);
          if (prev == NULL) {
            entry = csound->cache[bin_ctr];
          }
          else {
            entry = prev->next;
          }
          cache_ctr--;
        }
        else {
          prev = entry;
          entry = entry->next;
        }
      }
      bin_ctr++;
    }
}
#endif

static int csp_dag_cache_compare(CSOUND *csound,
                                 struct dag_cache_entry_t *entry, INSDS *chain)
{
#ifdef CAUTIOUS
    if (entry == NULL) csound->Die(csound, Str("Invalid NULL Parameter entry"));
    if (chain == NULL) csound->Die(csound, Str("Invalid NULL Parameter chain"));
#endif

    INSDS *current_insds = chain;
    int32_t ctr = 0;
    while (current_insds != NULL && ctr < entry->instrs) {
      if (current_insds->insno != entry->chain[ctr]) {
        return 0;
      }
      current_insds = current_insds->nxtact;
      ctr++;
    }
    if (ctr >= entry->instrs && current_insds != NULL) {
      return 0;
    }
    else if (ctr < entry->instrs && current_insds == NULL) {
      return 0;
    }
    else {
      return 1;
    }
}

void csp_dag_cache_fetch(CSOUND *csound, DAG **dag, INSDS *chain)
{
#ifdef CAUTIOUS
    if (dag == NULL) csound->Die(csound, Str("Invalid NULL Parameter dag"));
    if (chain == NULL) csound->Die(csound, Str("Invalid NULL Parameter chain"));
#endif

    update_ctr++;
    // WHY does this code exist?
/*     if (update_ctr == 10000) { */
/*       csound->Message(csound, "Reloading because of count\n"); */
/*       csp_dag_cache_update(csound); */
/*       update_ctr = 0; */
/* #ifdef HYBRID_HASH_CACHE */
/*       cache_last = NULL; */
/* #endif */
/*     } */

/* #ifdef HYBRID_HASH_CACHE */
/*     if (cache_last != NULL && */
/*         csp_dag_cache_compare(csound, cache_last, chain)) { */
/*       struct dag_cache_entry_t *curr = cache_last; */
/* #if TRACE > 4 */
/*       csound->Message(csound, "Cache Hit (Last) [%i]\n", cache_ctr); */
/* #endif */
/*       *dag = curr->dag; */
/*       TAKE_LOCK(&(curr->dag->consume_spinlock)); */
/*       curr->uses++; */

/*       csp_dag_prepare_use_insds(csound, curr->dag, chain); */
/*       csp_dag_prepare_use(csound, curr->dag); */
/*       RELS_LOCK(&(curr->dag->consume_spinlock)); */
/*       return; */
/*     } */
/* #endif */

    uint32_t hash_val = hash_chain(chain, DAG_2_CACHE_SIZE);
    struct dag_cache_entry_t *curr = csound->cache[hash_val];
    while (curr != NULL) {
      if (csp_dag_cache_compare(csound, curr, chain)) {
        TRACE_2("Cache Hit [%i]\n", cache_ctr);

        *dag = curr->dag;

        TAKE_LOCK(&(curr->dag->consume_spinlock));
        curr->uses++;

        csp_dag_prepare_use_insds(csound, curr->dag, chain);
        csp_dag_prepare_use(csound, curr->dag);
/* #ifdef HYBRID_HASH_CACHE */
/*         cache_last = curr; */
/* #endif */
        RELS_LOCK(&(curr->dag->consume_spinlock));
        break;
      }
      curr = curr->next;
    }
    if (*dag == NULL) {
      TRACE_2("Cache Miss [%i]\n", cache_ctr);
      csp_dag_cache_entry_alloc(csound, &curr, chain, hash_val);
      cache_ctr++;
      *dag = curr->dag;

      curr->next = csound->cache[hash_val];
      csound->cache[hash_val] = curr;
/* #ifdef HYBRID_HASH_CACHE */
/*       cache_last = curr; */
/* #endif */
    }
}

#endif

static long x = 0;

int waste_time(CSOUND* csound, WASTE *p)
{
    int k, n = (int)*p->icnt;
    for (k=0; k<n; k++) x = (x+1)^1234;
    for (k=0; k<n; k++) x = (x+1)^1234;
    for (k=0; k<n; k++) x = (x+1)^1234;
    return OK;
}
