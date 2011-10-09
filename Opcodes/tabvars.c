/*
    tabvars.c:

    Copyright (C) 2011 John ffitch

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

#include "csdl.h"

typedef struct {
    OPDS h;
    TABDAT *ans, *left, *right;
} TABARITH;

typedef struct {
    OPDS h;
    MYFLT  *ans;
    TABDAT *tab;
} TABQUERY;

typedef struct {
    OPDS h;
    TABDAT *tab;
    MYFLT  *kfn;
} TABCOPY;

static int tabarithset(CSOUND *csound, TABARITH *p)
{
    if (LIKELY(p->ans->data && p->left->data && p->right->data)) return OK;
    return csound->InitError(csound, Str("t-variable not initialised"));
}

static int tabadd(CSOUND *csound, TABARITH *p)
{
    TABDAT *ans = p->ans;
    TABDAT *l   = p->left;
    TABDAT *r   = p->right;
    int size    = ans->size;
    int i;

    if (l->size<size) size = l->size;
    if (r->size<size) size = r->size;
    for (i=0; i<=size; i++)
      ans->data[i] = l->data[i] + r->data[i];
    return OK;
}

static int tabmult(CSOUND *csound, TABARITH *p)
{
    TABDAT *ans = p->ans;
    TABDAT *l   = p->left;
    TABDAT *r   = p->right;
    int size    = ans->size;
    int i;

    if (l->size<size) size = l->size;
    if (r->size<size) size = r->size;
    for (i=0; i<=size; i++)
      ans->data[i] = l->data[i] * r->data[i];
    return OK;
}
 
static int tabqset(CSOUND *csound, TABQUERY *p)
{
    if (LIKELY(p->tab->data)) return OK;
    return csound->InitError(csound, Str("t-variable not initialised"));
}

static int tabmax(CSOUND *csound, TABQUERY *p)
{
    TABDAT *t = p->tab;
    int i, size = t->size;
    MYFLT ans = t->data[0];

    for (i=1; i<=size; i++)
      if (t->data[i]>ans) ans = t->data[i];
    *p->ans = ans;
    return OK;
}
  
static int tabmin(CSOUND *csound, TABQUERY *p)
{
    TABDAT *t = p->tab;
    int i, size = t->size;
    MYFLT ans = t->data[0];

    for (i=1; i<=size; i++)
      if (t->data[i]<ans) ans = t->data[i];
    *p->ans = ans;
    return OK;
}
  
static OENTRY localops[] =
{
  { "plustab", sizeof(TABARITH), 3, "", "ttt", (SUBR) tabarithset, (SUBR) tabadd },
  { "multtab", sizeof(TABARITH), 3, "", "ttt", (SUBR) tabarithset, (SUBR) tabmult },
  { "maxtab", sizeof(TABQUERY), 3, "k", "t", (SUBR) tabqset, (SUBR) tabmax },
  { "mintab", sizeof(TABQUERY), 3, "k", "t", (SUBR) tabqset, (SUBR) tabmin },
  //  { "copy2ftab", sizeof(TABCOPY), 1, "", "tk", NULL, (SUBR) tab2ftab },
  //  { "copy2ttab", sizeof(TABCOPY), 1, "", "tk", NULL, (SUBR) ftab2tab },
};

LINKAGE



