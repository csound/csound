/*
    cs_par_orc_semantic_analysis.c:

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

#include "interlocks.h"

/***********************************************************************
 * static function prototypes
 */
/* static int csp_thread_index_get(CSOUND *csound); */
static INSTR_SEMANTICS *instr_semantics_alloc(CSOUND *csound, char *name);

/***********************************************************************
 * helper functions
 */

static INSTR_SEMANTICS *instr_semantics_alloc(CSOUND *csound, char *name)
{
    INSTR_SEMANTICS *instr =
      csound->Malloc(csound, sizeof(INSTR_SEMANTICS));
    memset(instr, 0, sizeof(INSTR_SEMANTICS));
    strncpy(instr->hdr, INSTR_SEMANTICS_HDR, HDR_LEN);
    instr->name = name;
    instr->insno = -1;
    /* always check for greater than 0 in optimisation
       so this is a good default
     */
    instr->weight = 1;

    csp_set_alloc_string(csound, &(instr->read_write));
    csp_set_alloc_string(csound, &(instr->write));
    csp_set_alloc_string(csound, &(instr->read));

    return instr;
}

/***********************************************************************
 * parse time support
 */

//static INSTR_SEMANTICS *curr;
//static INSTR_SEMANTICS *root;

void csp_orc_sa_cleanup(CSOUND *csound)
{
    INSTR_SEMANTICS *current = csound->instRoot, *h = NULL;
    while (current != NULL) {

      csp_set_dealloc(csound, &(current->read));
      csp_set_dealloc(csound, &(current->write));
      csp_set_dealloc(csound, &(current->read_write));

      h = current;
      current = current->next;
      csound->Free(csound, h);
    }

    csound->instCurr = NULL;
    csound->instRoot = NULL;
}

void csp_orc_sa_print_list(CSOUND *csound)
{
    csound->Message(csound, "Semantic Analysis\n");
    INSTR_SEMANTICS *current = csound->instRoot;
    while (current != NULL) {
      csound->Message(csound, "Instr: %s\n", current->name);
      csound->Message(csound, "  read: ");
      csp_set_print(csound, current->read);

      csound->Message(csound, "  write: ");
      csp_set_print(csound, current->write);

      csound->Message(csound, "  read_write: ");
      csp_set_print(csound, current->read_write);

      csound->Message(csound, "  weight: %u\n", current->weight);

      current = current->next;
    }
    csound->Message(csound, "Semantic Analysis Ends\n");
}

void csp_orc_sa_global_read_write_add_list(CSOUND *csound,
                                           struct set_t *write,
                                           struct set_t *read)
{
    if (csound->instCurr == NULL) {
      csound->Message(csound,
                      "Add global read, write lists without any instruments\n");
    }
    else if (UNLIKELY(write == NULL  || read == NULL)) {
      csound->Die(csound,
                  "Invalid NULL parameter set to add to global read, "
                  "write lists\n");
    }
    else {
      struct set_t *new = NULL;
      csp_set_union(csound, write, read, &new);
      if (write->count == 1 && read->count == 1 && new->count == 1) {
        /* this is a read_write list thing */
        struct set_t *new_read_write = NULL;
        csp_set_union(csound, csound->instCurr->read_write, new, &new_read_write);
        csp_set_dealloc(csound, &csound->instCurr->read_write);
        csound->instCurr->read_write = new_read_write;
      }
      else {
        csp_orc_sa_global_write_add_list(csound, write);
        csp_orc_sa_global_read_add_list(csound, read);
      }

      csp_set_dealloc(csound, &new);
    }
}

void csp_orc_sa_global_write_add_list(CSOUND *csound, struct set_t *set)
{
    if (csound->instCurr == NULL) {
      csound->Message(csound,
                      "Add a global write_list without any instruments\n");
    }
    else if (UNLIKELY(set == NULL)) {
      csound->Die(csound,
                  "Invalid NULL parameter set to add to a global write_list\n");
    }
    else {
      struct set_t *new = NULL;
      csp_set_union(csound, csound->instCurr->write, set, &new);

      csp_set_dealloc(csound, &csound->instCurr->write);
      csp_set_dealloc(csound, &set);

      csound->instCurr->write = new;
    }
}

void csp_orc_sa_global_read_add_list(CSOUND *csound, struct set_t *set)
{
    if (csound->instCurr == NULL) {
      csound->Message(csound, "add a global read_list without any instruments\n");
    }
    else if (UNLIKELY(set == NULL)) {
      csound->Die(csound,
                  "Invalid NULL parameter set to add to a global read_list\n");
    }
    else {
      struct set_t *new = NULL;
      csp_set_union(csound, csound->instCurr->read, set, &new);

      csp_set_dealloc(csound, &csound->instCurr->read);
      csp_set_dealloc(csound, &set);

      csound->instCurr->read = new;
    }
}

void csp_orc_sa_interlocksf(CSOUND *csound, int code)
{
    if (code&0xfff8) {
      /* zak etc */
      struct set_t *rr = NULL;
      struct set_t *ww = NULL;
      csp_set_alloc_string(csound, &ww);
      csp_set_alloc_string(csound, &rr);
      switch (code&0xfff8) {
      case ZR:
        csp_set_add(csound, rr, "##zak");
        break;
      case ZW:
        csp_set_add(csound, ww, "##zak");
        break;
      case ZB:
        csp_set_add(csound, rr, "##zak");
        csp_set_add(csound, ww, "##zak");
        break;
      case TR:
        csp_set_add(csound, rr, "##tab");
        break;
      case TW:
        csp_set_add(csound, ww, "##tab");
        break;
      case TB:
        csp_set_add(csound, rr, "##tab");
        csp_set_add(csound, ww, "##tab");
        break;
      case CR:
        csp_set_add(csound, rr, "##chn");
        break;
      case CW:
        csp_set_add(csound, ww, "##chn");
        break;
      case CB:
        csp_set_add(csound, rr, "##chn");
        csp_set_add(csound, ww, "##chn");
        break;
      }
      csp_orc_sa_global_read_write_add_list(csound, ww, rr);
    }
}

void csp_orc_sa_interlocks(CSOUND *csound, ORCTOKEN *opcode)
{
    char *name = opcode->lexeme;
    int32 opnum = find_opcode(csound, name);
    OENTRY *ep = csound->opcodlst + opnum;
    csp_orc_sa_interlocksf(csound, ep->thread);
}

static int inInstr = 0;

void csp_orc_sa_instr_add(CSOUND *csound, char *name)
{
    csound->inInstr = 1;
    if (csound->instRoot == NULL) {
      csound->instRoot = instr_semantics_alloc(csound, name);
      csound->instCurr = csound->instRoot;
    }
    else if (csound->instCurr == NULL) {
      INSTR_SEMANTICS *prev = csound->instRoot;
      csound->instCurr = prev->next;
      while (csound->instCurr != NULL) {
        prev = csound->instCurr;
        csound->instCurr = csound->instCurr->next;
      }
      prev->next = instr_semantics_alloc(csound, name);
      csound->instCurr = prev->next;
    }
    else {
      csound->instCurr->next = instr_semantics_alloc(csound, name);
      csound->instCurr = csound->instCurr->next;
    }
    // csound->instCurr->insno = named_instr_find(name);
}

/* New code to deal with lists of integer instruments -- JPff */
void csp_orc_sa_instr_add_tree(CSOUND *csound, TREE *x)
{
    while (x) {
      if (x->type == T_INTGR) {
        csp_orc_sa_instr_add(csound, x->value->lexeme);
        return;
      }
      if (x->type == T_IDENT) {
        csp_orc_sa_instr_add(csound, x->value->lexeme);
        return;
      }
      if (UNLIKELY(x->type != T_INSTLIST)) {
        printf("type %d not T_INSTLIST\n", x->type);
        csound->Die(csound, "Not a proper list of ints");
      }
      csp_orc_sa_instr_add(csound, x->left->value->lexeme);
      x = x->right;
    }
}

void csp_orc_sa_instr_finalize(CSOUND *csound)
{
    csound->instCurr = NULL;
    csound->inInstr = 0;
}

struct set_t *csp_orc_sa_globals_find(CSOUND *csound, TREE *node)
{
    struct set_t *left, *right;
    struct set_t *current_set = NULL;

    if (node == NULL) {
      struct set_t *set = NULL;
      csp_set_alloc_string(csound, &set);
      return set;
    }

    left  = csp_orc_sa_globals_find(csound, node->left);
    right = csp_orc_sa_globals_find(csound, node->right);
    csp_set_union(csound, left, right, &current_set);

    csp_set_dealloc(csound, &left);
    csp_set_dealloc(csound, &right);

    switch (node->type) {
    case T_IDENT_GI:
    case T_IDENT_GK:
    case T_IDENT_GF:
    case T_IDENT_GW:
    case T_IDENT_GS:
    case T_IDENT_GA:
      csp_set_add(csound, current_set, node->value->lexeme);
      break;
    default:
      /* no globals */
      break;
    }

    if (node->next != NULL) {
      struct set_t *prev_set = current_set;
      struct set_t *next = csp_orc_sa_globals_find(csound, node->next);
      csp_set_union(csound, prev_set, next, &current_set);

      csp_set_dealloc(csound, &prev_set);
      csp_set_dealloc(csound, &next);
    }

    return current_set;
}

INSTR_SEMANTICS *csp_orc_sa_instr_get_by_name(CSOUND *csound, char *instr_name)
{
    INSTR_SEMANTICS *current_instr = csound->instRoot;
    while (current_instr != NULL) {
      if (strcmp(current_instr->name, instr_name) == 0) {
        return current_instr;
      }
      current_instr = current_instr->next;
    }
    return NULL;
}

INSTR_SEMANTICS *csp_orc_sa_instr_get_by_num(CSOUND *csound, int16 insno)
{
#define BUF_LENGTH 8
    INSTR_SEMANTICS *current_instr = csound->instRoot;
    char buf[BUF_LENGTH];
    while (current_instr != NULL) {
      if (current_instr->insno != -1 && current_instr->insno == insno) {
        return current_instr;
      }
      current_instr = current_instr->next;
    }

    snprintf(buf, BUF_LENGTH, "%i", insno);

    current_instr = csp_orc_sa_instr_get_by_name(csound, buf);
    if (current_instr != NULL) {
      current_instr->insno = insno;
    }
    return current_instr;
}
