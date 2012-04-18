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

// #include "csdl.h"
#include "csoundCore.h"
#include "interlocks.h"


typedef struct {
    OPDS h;
    TABDAT *ans, *left, *right;
    AUXCH auxch;
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
    AUXCH auxch;
} TABCOPY;

typedef struct {
    OPDS h;
    TABDAT *tab;
    MYFLT  *kmin, *kmax;
    MYFLT  *kstart, *kend;
} TABSCALE;

static int tabarithset(CSOUND *csound, TABARITH *p)
{
  if (LIKELY(p->left->data && p->right->data)){
    if(p->ans->data == NULL) {
      /* size is the smallest of the two */  
      int size = p->left->size < p->right->size ? p->left->size : p->right->size;
      csound->AuxAlloc(csound,sizeof(MYFLT)*size, &p->auxch);
      p->ans->data = (MYFLT *)p->auxch.auxp;
      p->ans->size = size;
    }
    return OK;
  }
  else return csound->InitError(csound, Str("t-variable not initialised"));
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
    if (ans->size<size) size = ans->size;
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
    if (ans->size<size) size = ans->size;
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

static int tabsum(CSOUND *csound, TABQUERY *p)
{
    TABDAT *t = p->tab;
    int i, size = t->size;
    MYFLT ans = FL(0.0);

    for (i=1; i<=size; i++)
      ans += t->data[i];
    *p->ans = ans;
    return OK;
}

static int tabscaleset(CSOUND *csound, TABSCALE *p)
{
    if (LIKELY(p->tab->data)) return OK;
    return csound->InitError(csound, Str("t-variable not initialised"));
}

static int tabscale(CSOUND *csound, TABSCALE *p)
{
    MYFLT min = *p->kmin, max = *p->kmax;
    int strt = (int)MYFLT2LRND(*p->kstart), end = (int)MYFLT2LRND(*p->kend);
    TABDAT *t = p->tab;
    MYFLT tmin = t->data[strt];
    MYFLT tmax = tmin;
    int i;
    MYFLT range;

    // Correct start an dening points
    if (end<0) end = t->size;
    else if (end>t->size) end = t->size;
    else if (end<0) end = 0;
    if (strt<0) strt = 0;
    else if (strt>t->size) strt = t->size;
    if (end<strt) {
      int x = end; end = strt; strt = x;
    }
    // get data range
    for (i=strt+1; i<end; i++) {
      if (t->data[i]<tmin) tmin = t->data[i];
      if (t->data[i]>tmax) tmax = t->data[i];
    }
    range = (max-min)/(tmax-tmin);
    for (i=strt; i<end; i++) {
      t->data[i] = (t->data[i]-tmin)*range + min;
    }
    return OK;
}

typedef struct {
    OPDS h;
    TABDAT *dst;
    TABDAT *src;
    AUXCH auxch;
    int    len;
} TABCPY;

static int tabcopy_set(CSOUND *csound, TABCPY *p)
{
    int sizes,sized;
    if (UNLIKELY(p->src->data==NULL))
      return csound->InitError(csound, Str("t-variable not initialised"));
    if (UNLIKELY(p->dst->data==NULL)) {
      // return csound->InitError(csound, Str("t-variable not initialised"));
      csound->AuxAlloc(csound, sizeof(MYFLT)*p->src->size, &p->auxch);
      p->dst->data = (MYFLT *) p->auxch.auxp;
      p->len = (p->dst->size = p->src->size)*sizeof(MYFLT);
      memmove(p->dst->data, p->src->data, p->len);
    } else{
     sizes = p->src->size;
     sized = p->dst->size;
     p->len = sizeof(MYFLT)*(sizes>sized ? sized : sizes);
    }
    return OK;
}

static int tabcopy(CSOUND *csound, TABCPY *p)
{
    memmove(p->dst->data, p->src->data, p->len);
    return OK;
}

static int tab2ftab(CSOUND *csound, TABCOPY *p)
{
    FUNC        *ftp;
    int fsize;
    MYFLT *fdata;
    int tlen = p->tab->size;
    if (UNLIKELY(p->tab->data==NULL)) 
      return csound->PerfError(csound, Str("t-var not initialised"));
    if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL))
        return csound->PerfError(csound, Str("No table for copy2ftab"));
    fsize = ftp->flen;
    fdata = ftp->ftable;
    if (fsize<tlen) tlen = fsize;
    memcpy(fdata, p->tab->data, sizeof(MYFLT)*tlen);
    return OK;
}

typedef struct {
    OPDS h;
    TABDAT *tab;
    MYFLT *start, *end, *incr;
    AUXCH auxch;
    int    len;
} TABGEN;


static int tabgen_set(CSOUND *csound, TABGEN *p){

  MYFLT *data =  p->tab->data;
  MYFLT start = *p->start;
  MYFLT end   = *p->end;
  MYFLT incr  = *p->incr;
  int i,size =  (p->end - p->start)/incr + 1;
  if(size < 0) csound->InitError(csound, Str("inconsistent start, end and increment parameters"));
  
 if (UNLIKELY(p->tab->data==NULL)) {
      csound->AuxAlloc(csound, sizeof(MYFLT)*size, &p->auxch);
      data = p->tab->data = (MYFLT *) p->auxch.auxp;
      p->tab->size = size;
    }
 else size = p->tab->size;

 for(i=0; i < size; i++){
   data[i] = start; 
   start += incr;
 }

 return OK;
}

static int ftab2tab(CSOUND *csound, TABCOPY *p)
{
    FUNC        *ftp;
    int         fsize;
    MYFLT       *fdata;
    int tlen;
    fsize = ftp->flen;
    if (UNLIKELY(p->tab->data==NULL)) {
      // return csound->PerfError(csound, Str("t-var not initialised"));
      csound->AuxAlloc(csound, sizeof(MYFLT)*fsize, &p->auxch);
      p->tab->data = (MYFLT *) p->auxch.auxp;
      p->tab->size = fsize;
    }
    if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL))
        return csound->PerfError(csound, Str("No table for copy2ftab"));
    tlen = p->tab->size;
    fdata = ftp->ftable;
    if (fsize<tlen) tlen = fsize;
    memcpy(p->tab->data, fdata, sizeof(MYFLT)*tlen);
    return OK;
}





static OENTRY tabvars_localops[] =
{
  { "plustab", sizeof(TABARITH), 3, "t", "tt", (SUBR) tabarithset, (SUBR) tabadd },
  { "multtab", sizeof(TABARITH), 3, "t", "tt", (SUBR) tabarithset, (SUBR) tabmult },
  { "maxtab", sizeof(TABQUERY), 3, "k", "t", (SUBR) tabqset, (SUBR) tabmax },
  { "mintab", sizeof(TABQUERY), 3, "k", "t", (SUBR) tabqset, (SUBR) tabmin },
  { "sumtab", sizeof(TABQUERY), 3, "k", "t", (SUBR) tabqset, (SUBR) tabsum },
  { "scalet", sizeof(TABSCALE), 3, "", "tkkOJ",(SUBR) tabscaleset,(SUBR) tabscale },
  { "#copytab", sizeof(TABCPY), 3, "t", "t", (SUBR) tabcopy_set, (SUBR)tabcopy },
  { "tabgen", sizeof(TABGEN), 1, "t", "iii", (SUBR) tabgen_set, NULL, NULL},
  { "copy2ftab", sizeof(TABCOPY), TW|1, "", "tk", NULL, (SUBR) tab2ftab },  /* thread should be 2 instead of 1 ? */
  { "copy2ttab", sizeof(TABCOPY), TR|1, "", "tk", NULL, (SUBR) ftab2tab }
};
// reverse, scramble, mirror, stutter, rotate, ...
// jpff: stutter is an interesting one (very musical). It basically
//          randomly repeats (holds) values based on a probability parameter


LINKAGE1(tabvars_localops)
