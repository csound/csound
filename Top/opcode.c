/*
    opcode.c:

    Copyright (C) 1997 John ffitch
              (C) 2005 Istvan Varga

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

                                /* OPCODE.C */
                                /* Print opcodes in system */

                                /* John ffitch -- 26 Jan 97 */
                                /*  4 april 02 -- ma++ */
                                /*  restructure to retrieve externally  */
                                /* And suppressing deprecated Oct 2015 -- JPff */
#include "csoundCore.h"
#include <ctype.h>
#include "interlocks.h"

static int opcode_cmp_func(const void *a, const void *b)
{
    opcodeListEntry *ep1 = (opcodeListEntry*) a;
    opcodeListEntry *ep2 = (opcodeListEntry*) b;
    int             retval;

    if ((retval = strcmp(ep1->opname, ep2->opname)) != 0)
      return retval;
    if ((retval = strcmp(ep1->outypes, ep2->outypes)) != 0)
      return retval;
    if ((retval = strcmp(ep1->intypes, ep2->intypes)) != 0)
      return retval;
    if (ep1 < ep2)
      return -1;
    if (ep1 > ep2)
      return 1;

    return 0;
}

/**
 * Gets an alphabetically sorted list of all opcodes.
 * Should be called after externals are loaded by csoundCompile().
 * Returns the number of opcodes, or a negative error code on failure.
 * Make sure to call csoundDisposeOpcodeList() when done with the list.
 */

PUBLIC int csoundNewOpcodeList(CSOUND *csound, opcodeListEntry **lstp)
{
    void    *lst = NULL;
    OENTRY  *ep;
    char    *s;
    size_t  nBytes = (size_t) 0;
    int     i, cnt = 0;
    CONS_CELL *head, *items, *temp;

    (*lstp) = NULL;
    if (UNLIKELY(csound->opcodes == NULL))
      return -1;

    head = items = cs_hash_table_values(csound, csound->opcodes);

    /* count the number of opcodes, and bytes to allocate */
    while (items != NULL) {
      temp = items->value;
      while (temp != NULL) {
        ep = temp->value;
        if (ep->opname != NULL &&
            ep->opname[0] != '\0' && isalpha(ep->opname[0]) &&
            ep->outypes != NULL && ep->intypes != NULL) {
          cnt++;
#ifdef JPFF
          if (strchr(ep->intypes, 'x'))
            printf("%s, type %d %s -> %s\n", ep->opname, ep->thread,
                   ep->intypes, ep->outypes);
          /* else if (ep->thread==5 */
          /*   printf("%s, type 6 %s -> %s\n", ep->opname, */
          /*          ep->intypes, ep->outypes); */
#endif
          nBytes += sizeof(opcodeListEntry);
          for (i = 0; ep->opname[i] != '\0' && ep->opname[i] != '.'; i++);
          nBytes += (size_t) i;
          nBytes += strlen(ep->outypes);
          nBytes += strlen(ep->intypes);
          nBytes += 3;    /* for null characters */
        }
        temp = temp->next;
      }
      items = items->next;
    }
    nBytes += sizeof(opcodeListEntry);
    /* allocate memory for opcode list */
    lst = csound->Malloc(csound, nBytes);
    if (UNLIKELY(lst == NULL))
      return CSOUND_MEMORY;
    (*lstp) = (opcodeListEntry*) lst;
    /* store opcodes in list */
    items = head;
    s = (char*) lst + ((int) sizeof(opcodeListEntry) * (cnt + 1));
    cnt = 0;
    while (items != NULL) {
        temp = items->value;
        while (temp != NULL) {
          ep = temp->value;

          if (ep->opname != NULL &&
              ep->opname[0] != '\0' && isalpha(ep->opname[0]) &&
              ep->outypes != NULL && ep->intypes != NULL) {
            for (i = 0; ep->opname[i] != '\0' && ep->opname[i] != '.'; i++)
              s[i] = ep->opname[i];
            s[i++] = '\0';
            ((opcodeListEntry*) lst)[cnt].opname = s;
            s += i;
            strcpy(s, ep->outypes);
            ((opcodeListEntry*) lst)[cnt].outypes = s;
            s += ((int) strlen(ep->outypes) + 1);
#ifdef JPFF
            if (strlen(ep->outypes)==0) printf("***potential WI opcode %s\n", ep->opname);
#endif
            strcpy(s, ep->intypes);
            ((opcodeListEntry*) lst)[cnt].intypes = s;
            s += ((int) strlen(ep->intypes) + 1);
            ((opcodeListEntry*) lst)[cnt].flags = ep->flags;
            //if (ep->flags&_QQ) printf("DEPRICATED: %s\n", ep->opname);
            //if (ep->flags&_QQ) *deprec++;
            cnt++;
          }
          temp = temp->next;
        }
      items = items->next;
    }
    ((opcodeListEntry*) lst)[cnt].opname = NULL;
    ((opcodeListEntry*) lst)[cnt].outypes = NULL;
    ((opcodeListEntry*) lst)[cnt].intypes = NULL;
    ((opcodeListEntry*) lst)[cnt].flags = 0;

    cs_cons_free(csound, head);

    /* sort list */
    qsort(lst, (size_t) cnt, sizeof(opcodeListEntry), opcode_cmp_func);

    /* return the number of opcodes */
    return cnt;
}

PUBLIC void csoundDisposeOpcodeList(CSOUND *csound, opcodeListEntry *lst)
{
    csound->Free(csound, lst);
}

void list_opcodes(CSOUND *csound, int level)
{
    opcodeListEntry *lst;
    const char      *sp = "                    ";   /* length should be 20 */
    int             j, k;
    int             cnt, len = 0, xlen = 0;
    int             count = 0;

    cnt = csoundNewOpcodeList(csound, &lst);
    if (UNLIKELY(cnt <= 0)) {
      csound->ErrorMsg(csound, Str("Error creating opcode list"));
      csoundDisposeOpcodeList(csound, lst);
      return;
    }

    for (j = 0, k = -1; j < cnt; j++) {
      if ((level&1) == 0) {                         /* Print in 4 columns */
        if (j > 0 && strcmp(lst[j - 1].opname, lst[j].opname) == 0)
          continue;
        if ((level&2)==0 && ((lst[j].flags&_QQ) !=0)) {
          //printf("dropping %s\n", lst[j].opname);
          continue;
        }
        k++;
        xlen = 0;
        if (!(k & 3))
          csound->Message(csound, "\n");
        else {
          if (len > 19) {
            xlen = len - 19;
            len = 19;
          }
          csound->Message(csound, "%s", sp + len);
        }
        csound->Message(csound, "%s", lst[j].opname);
        len = (int) strlen(lst[j].opname) + xlen;
      }
      else {
        char *ans = lst[j].outypes, *arg = lst[j].intypes;
        if ((level&2)==0 && ((lst[j].flags&_QQ) !=0)) {
          //printf("dropping %s\n", lst[j].opname);
          continue;
        }
        csound->Message(csound, "%s", lst[j].opname);
        len = (int) strlen(lst[j].opname);
        if (len > 11) {
          xlen = len - 11;
          len = 11;
        }
        csound->Message(csound, "%s", sp + (len + 8));
        if (ans == NULL || *ans == '\0') ans = "(null)";
        if (arg == NULL || *arg == '\0') arg = "(null)";
        csound->Message(csound, "%s", ans);
        len = (int) strlen(ans) + xlen;
        len = (len < 11 ? len : 11);
        xlen = 0;
        csound->Message(csound, "%s", sp + (len + 8));
        csound->Message(csound, "%s\n", arg);
      }
      count++;
    }
    csound->Message(csound, "\n");
    csound->Message(csound, Str("%d opcodes\n\n"), count);
    csoundDisposeOpcodeList(csound, lst);
}
