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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
*/

#include <stdio.h>
#include <stdlib.h>

#include "csoundCore.h"
#include "csound_orc.h"
#include "tok.h"

#include "cs_par_base.h"
#include "cs_par_orc_semantics.h"

#include "interlocks.h"

OENTRY* find_opcode(CSOUND *, char *);
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
    memcpy(instr->hdr, INSTR_SEMANTICS_HDR, HDR_LEN);
    instr->name = name;
    instr->insno = -1;
    /* always check for greater than 0 in optimisation
       so this is a good default
     */
    instr->read_write = csp_set_alloc_string(csound);
    instr->write = csp_set_alloc_string(csound);
    instr->read = csp_set_alloc_string(csound);

    return instr;
}

/***********************************************************************
 * parse time support
 */

//static INSTR_SEMANTICS *curr;
//static INSTR_SEMANTICS *root;

/* void csp_orc_sa_cleanup(CSOUND *csound) */
/* { */
/*     INSTR_SEMANTICS *current = csound-> instRoot, *h = NULL; */
/*     csp_orc_sa_print_list(csound); */
/*     while (current != NULL) { */

/*       csp_set_dealloc(csound, &(current->read)); */
/*       csp_set_dealloc(csound, &(current->write)); */
/*       csp_set_dealloc(csound, &(current->read_write)); */

/*       h = current; */
/*       current = current->next; */
/*       csound->Free(csound, h); */
/*     } */
/*     current = csound->instCurr; */
/*     while (current != NULL) { */

/*       csp_set_dealloc(csound, &(current->read)); */
/*       csp_set_dealloc(csound, &(current->write)); */
/*       csp_set_dealloc(csound, &(current->read_write)); */

/*       h = current; */
/*       current = current->next; */
/*       csound->Free(csound, h); */
/*     } */

/*     csound->instCurr = NULL; */
/*     csound->instRoot = NULL; */
/* } */


static void sanitise_set(CSOUND *csound, struct set_t* p)
{
    struct set_element_t *ele = p->head;
    while (ele != NULL) {
      ele->data = cs_strdup(csound, (char*)ele->data);
      ele = ele->next;
    }
}

void sanitize(CSOUND*csound)
{
    INSTR_SEMANTICS *p = csound->instRoot;
    while (p) {
      if (p->sanitized==0) {
        sanitise_set(csound, p->read);
        sanitise_set(csound, p->write);
        sanitise_set(csound, p->read_write);
        p->sanitized = 1;
      }
      p = p->next;
    }
}
void csp_orc_sa_print_list(CSOUND *csound)
{
    csound->Message(csound, "Semantic Analysis\n");
    INSTR_SEMANTICS *current = csound->instRoot;
    while (current != NULL) {
      csound->Message(csound, "(%p)Instr: %s\n", current, current->name);
      csound->Message(csound, "  read(%p): ", current->read);
      csp_set_print(csound, current->read);

      csound->Message(csound, "  write:(%p) ", current->write);
      csp_set_print(csound, current->write);

      csound->Message(csound, "  read_write(%p): ", current->read_write);
      csp_set_print(csound, current->read_write);

      current = current->next;
    }
    csound->Message(csound, "Semantic Analysis Ends\n");
}

void csp_orc_sa_global_read_write_add_list(CSOUND *csound,
                                           struct set_t *write,
                                           struct set_t *read)
{
    if (UNLIKELY(csound->instCurr == NULL)) {
      csound->DebugMsg(csound,
                      "Add global read, write lists without any instruments\n");
    }
    else if (UNLIKELY(write == NULL  || read == NULL)) {
      csound->Die(csound,
                  Str("Invalid NULL parameter set to add to global read, "
                      "write lists\n"));
    }
    else {
      csp_orc_sa_global_write_add_list(csound, write);
      csp_orc_sa_global_read_add_list(csound, read);
    }
}

void csp_orc_sa_global_read_write_add_list1(CSOUND *csound,
                                           struct set_t *write,
                                           struct set_t *read)
{
    if (UNLIKELY(csound->instCurr == NULL)) {
      csound->DebugMsg(csound,
                      "Add global read, write lists without any instruments\n");
    }
    else if (UNLIKELY(write == NULL  || read == NULL)) {
      csound->Die(csound,
                  Str("Invalid NULL parameter set to add to global read, "
                      "write lists\n"));
    }
    else {
      struct set_t *new = csp_set_union(csound, write, read);
      //printf("Line: %d of cs_par_orc_semantics(%p)\n", __LINE__, *new);
      if (write->count == 1 && read->count == 1 && new->count == 1) {
        /* this is a read_write list thing */
        struct set_t *new_read_write = csp_set_union(csound,
                                                     csound->instCurr->read_write,
                                                     new);
        //printf("Line: %d of cs_par_orc_semantics(%p)\n",
        //       __LINE__, *new_read_write);
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
    if (UNLIKELY(csound->instCurr == NULL)) {
      csound->Message(csound,
                      Str("Add a global write_list without any instruments\n"));
    }
    else if (UNLIKELY(set == NULL)) {
      csound->Die(csound,
                  Str("Invalid NULL parameter set to add to a "
                      "global write_list\n"));
    }
    else {
      struct set_t *new = csp_set_union(csound, csound->instCurr->write, set);

      csp_set_dealloc(csound, &csound->instCurr->write);
      csp_set_dealloc(csound, &set);

      csound->instCurr->write = new;
    }
}

void csp_orc_sa_global_read_add_list(CSOUND *csound, struct set_t *set)
{
    if (csound->instCurr == NULL) {
      if (UNLIKELY(PARSER_DEBUG))
        csound->Message(csound,
                        "add a global read_list without any instruments\n");
    }
    else if (UNLIKELY(set == NULL)) {
      csound->Die(csound,
                  Str("Invalid NULL parameter set to add to a "
                      "global read_list\n"));
    }
    else {
      struct set_t *new = csp_set_union(csound, csound->instCurr->read, set);

      csp_set_dealloc(csound, &csound->instCurr->read);
      csp_set_dealloc(csound, &set);

      csound->instCurr->read = new;
    }
}

static void csp_orc_sa_interlocksf(CSOUND *csound, int code, char *name)
{
    if (code&0xfff8) {
      /* zak etc */
      struct set_t *rr = NULL;
      struct set_t *ww = NULL;
      ww = csp_set_alloc_string(csound);
      rr = csp_set_alloc_string(csound);
      if (code&ZR) csp_set_add(csound, rr, "##zak");
      if (code&ZW) csp_set_add(csound, ww, "##zak");
      if (code&TR) csp_set_add(csound, rr, "##tab");
      if (code&TW) csp_set_add(csound, ww, "##tab");
      if (code&_CR) csp_set_add(csound, rr, "##chn");
      if (code&_CW) csp_set_add(csound, ww, "##chn");
      if (code&WR) csp_set_add(csound, ww, "##wri");
      if (code&IR) csp_set_add(csound, rr, "##int");
      if (code&IW) csp_set_add(csound, ww, "##int");
      csp_orc_sa_global_read_write_add_list(csound, ww, rr);
      //      printf("code&qq=%4x msglevel = %4x bit=%4x\n",
      //   code&_QQ, csound->oparms_.msglevel, csound->oparms_.msglevel&CS_NOQQ );
      if (UNLIKELY((code&_QQ) && !(csound->oparms_.msglevel&CS_NOQQ))) {
        csound->Message(csound, Str("opcode %s deprecated\n"), name);
      }
    }
}

void csp_orc_sa_interlocks(CSOUND *csound, ORCTOKEN *opcode)
{
    char *name = opcode->lexeme;
    OENTRY *ep = find_opcode(csound, name);
    csp_orc_sa_interlocksf(csound, ep->flags, name);
}

//static int inInstr = 0;

void csp_orc_sa_instr_add(CSOUND *csound, char *name)
{
    name = cs_strdup(csound, name); // JPff:  leaks: necessary?? Think it is correct
    //printf("csp_orc_sa_instr_add name=%s\n", name);
    csound->inInstr = 1;
    if (csound->instRoot == NULL) {
      //printf("instRoot id NULL\n");
      csound->instRoot = instr_semantics_alloc(csound, name);
      csound->instCurr = csound->instRoot;
    }
    else if (csound->instCurr == NULL) {
      INSTR_SEMANTICS *prev = csound->instRoot;
      //printf("instCurr NULL\n");
      csound->instCurr = prev->next;
      while (csound->instCurr != NULL) {
        prev = csound->instCurr;
        csound->instCurr = csound->instCurr->next;
      }
      prev->next = instr_semantics_alloc(csound, name);
      csound->instCurr = prev->next;
    }
    else {
      //printf("othercase\n");
      csound->instCurr->next = instr_semantics_alloc(csound, name);
      csound->instCurr = csound->instCurr->next;
    }
    // csound->instCurr->insno = named_instr_find(name);
}

/* New code to deal with lists of integer instruments -- JPff */
void csp_orc_sa_instr_add_tree(CSOUND *csound, TREE *x)
{
    while (x) {
      if (x->type == INTEGER_TOKEN) {
        csp_orc_sa_instr_add(csound, x->value->lexeme);
        return;
      }
      if (x->type == T_IDENT) {
        csp_orc_sa_instr_add(csound, x->value->lexeme);
        return;
      }
      if (UNLIKELY(x->type != T_INSTLIST)) {
        csound->DebugMsg(csound,"type %d not T_INSTLIST\n", x->type);
        csound->Die(csound, Str("Not a proper list of ints"));
      }
      csp_orc_sa_instr_add(csound, x->left->value->lexeme);
      x = x->right;
    }
}

struct set_t *csp_orc_sa_globals_find(CSOUND *csound, TREE *node)
{
    struct set_t *left=NULL, *right;
    struct set_t *current_set = NULL;

    if (node == NULL) {
      return csp_set_alloc_string(csound);
    }

    left  = csp_orc_sa_globals_find(csound, node->left);
    right = csp_orc_sa_globals_find(csound, node->right);
    current_set = csp_set_union(csound, left, right);
    //printf("Line: %d of cs_par_orc_semantics(%p)\n", __LINE__, current_set);
    csp_set_dealloc(csound, &left);
    csp_set_dealloc(csound, &right);

    if ((node->type == T_IDENT || node->type == LABEL_TOKEN) &&
        node->value->lexeme[0] == 'g') {
      csp_set_add(csound, current_set, /*strdup*/(node->value->lexeme));
    }

    if (node->next != NULL) {
      struct set_t *prev_set = current_set;
      struct set_t *next = csp_orc_sa_globals_find(csound, node->next);
      current_set = csp_set_union(csound, prev_set, next);
      //printf("Line: %d of cs_par_orc_semantics(%p)\n", __LINE__, current_set);
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

/* ANALYZE TREE */

void csp_orc_analyze_tree(CSOUND* csound, TREE* root)
{
    if (UNLIKELY(PARSER_DEBUG))
      csound->Message(csound, "Performing csp analysis\n");

    TREE *current = root;
    TREE *temp;

    while (current != NULL) {
      switch (current->type) {
      case INSTR_TOKEN:
        if (PARSER_DEBUG) csound->Message(csound, "Instrument found\n");

        temp = current->left;

        // FIXME - need to figure out why csp_orc_sa_instr_add is
        // called by itself in csound_orc.y
        csp_orc_sa_instr_add_tree(csound, temp);

        csp_orc_analyze_tree(csound, current->right);

        csp_orc_sa_instr_finalize(csound);

        break;
      case UDO_TOKEN:
        if (PARSER_DEBUG) csound->Message(csound, "UDO found\n");

        csp_orc_analyze_tree(csound, current->right);

        break;

      case IF_TOKEN:
      case UNTIL_TOKEN:
        break;
      case LABEL_TOKEN:
        break;
      default:
        if (UNLIKELY(PARSER_DEBUG))
          csound->Message(csound,
                          "Statement: %s\n", current->value->lexeme);

        if (current->left != NULL) {
          csp_orc_sa_global_read_write_add_list(csound,
              csp_orc_sa_globals_find(csound, current->left),
              csp_orc_sa_globals_find(csound, current->right));

        } else {
          csp_orc_sa_global_read_add_list(csound,
                                          csp_orc_sa_globals_find(csound,
                                                                  current->right));
        }

        break;
        }

      current = current->next;

    }

    if (UNLIKELY(PARSER_DEBUG)) csound->Message(csound, "[End csp analysis]\n");

}
