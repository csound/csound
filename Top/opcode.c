/*  
    opcode.c:

    Copyright (C) 1997 John ffitch

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

                                /* OPCODE.C */
                                /* Print opcodes in system */

                                /* John ffitch -- 26 Jan 97 */
                                /*  4 april 02 -- ma++ */
                                /*  restructure to retrieve externally  */
#include "cs.h"
#include "H/opcode.h"

static int mystrcmp(const void *v1, const void *v2)
{
    sortable *s1 = (sortable *)v1;
    sortable *s2 = (sortable *)v2;
    int ans;
/*     printf("Compare %s and %s =>", s1, s2); */
/* Make this stop at _ would improve things */
    ans = strcmp(s1->name, s2->name);
/*     printf(" %d\n", ans); */
    return ans;
}

/* create a list of all the opcodes */
/* caller is responsible for disposing the*/
/* returned list! - use dispose_opcode_list() */
opcodelist *new_opcode_list(void)
{
    OENTRY *ops = opcodlst;
    opcodelist *list = (opcodelist *)mmalloc(sizeof(opcodelist));

                                /* All this hassle 'cos of MAC */
    long n = (long)((char*)oplstend-(char *)opcodlst);
    n /= sizeof(OENTRY);
    n *= sizeof(sortable);

    list->size = 0;
    list->table = (sortable*)mmalloc(n);
                                /* Skip first entry */
    while (++ops<oplstend) {
      char *x = mmalloc(strlen(ops->opname)+1);
      strcpy(x, ops->opname);
      list->table[list->size].name = x;
      if ((x=strchr(x,'_'))) *x = '\0';
      list->table[list->size].ans = ops->outypes;
      list->table[list->size].args = ops->intypes;
      if (ops->outypes == NULL && ops->intypes == NULL)
        n--;
      else
        list->size ++;
    }
                                /* Sort into alphabetical order */
    printf(Str(X_37,"%d opcodes\n"), list->size);
    qsort(list->table, list->size, sizeof(sortable), mystrcmp);

   return list;
}

void dispose_opcode_list(opcodelist *list)
{
    if (list) {
      while (list->size--) {
        mfree(list->table[list->size].name);
      }

      mfree(list->table);
      mfree(list);
    }
}

void list_opcodes(int level)
{
    int j, k;
    int len = 0;

    opcodelist *list = new_opcode_list();

                                /* Print in 4 columns */
    for (j = 0, k = 0; j< list->size; j++) {
      if (level == 0) {
        if (j>0 && strcmp( list->table[j-1].name,  list->table[j].name)==0) continue;
        k++;
        if ((k%3)==0) {
          printf("\n"); len = 0;
        }
        else {
          do {
            printf(" ");
            len++;
          } while (len<20*(k%3));
        }
        printf("%s", list->table[j].name);
        len += strlen(list->table[j].name);
      }
      else {
        char *ans = list->table[j].ans, *arg = list->table[j].args;
        printf("%s", list->table[j].name);
        len = strlen(list->table[j].name);
        do {
          printf(" ");
          len++;
        } while (len<10);
        if (ans==NULL || *ans=='\0') ans = "(null)";
        if (arg==NULL || *arg=='\0') arg = "(null)";
        printf("%s", ans);
        len = strlen(ans);
        do {
          printf(" ");
          len++;
        } while (len<8);
        printf("%s\n", arg);
      }
    }
    printf("\n");
    dispose_opcode_list(list);
}
