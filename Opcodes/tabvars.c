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
#include "aops.h"
extern MYFLT MOD(MYFLT a, MYFLT bb);

typedef struct {
    OPDS h;
    TABDAT *ans, *left, *right;
} TABARITH;

typedef struct {
    OPDS h;
    TABDAT *ans, *left;
    MYFLT *right;
} TABARITH1;

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

typedef struct {
    OPDS h;
    TABDAT *tab;
    MYFLT  *kmin, *kmax;
    MYFLT  *kstart, *kend;
} TABSCALE;

static inline void tabensure(CSOUND *csound, TABDAT *p, int size)
{
    int32 ss = sizeof(MYFLT)*size;
    if (p->aux.auxp==NULL || p->aux.size<ss) {
      csound->AuxAlloc(csound, ss, &p->aux);
      p->data = (MYFLT*)(p->aux.auxp);
      p->size = size;
    }
}

static int tabarithset(CSOUND *csound, TABARITH *p)
{
    if (LIKELY(p->left->data && p->right->data)) {
      /* size is the smallest of the two */
      int size = p->left->size < p->right->size ? p->left->size : p->right->size;
      tabensure(csound, p->ans, size);
      p->ans->size = size;
      return OK;
    }
    else return csound->InitError(csound, Str("t-variable not initialised"));
}

static int tabarithset1(CSOUND *csound, TABARITH *p)
{
    if (LIKELY(p->left->data)) {
      /* size is the smallest of the two */
      int size = p->left->size;
      tabensure(csound, p->ans, size);
      p->ans->size = size;
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

    if (UNLIKELY(p->ans->data == NULL ||
          p->left->data==NULL || p->right->data==NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->size<size) size = l->size;
    if (r->size<size) size = r->size;
    if (ans->size<size) size = ans->size;
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] + r->data[i];
    return OK;
}

static int tabsub(CSOUND *csound, TABARITH *p)
{
    TABDAT *ans = p->ans;
    TABDAT *l   = p->left;
    TABDAT *r   = p->right;
    int size    = ans->size;
    int i;

    if (UNLIKELY(p->ans->data == NULL ||
          p->left->data==NULL || p->right->data==NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->size<size) size = l->size;
    if (r->size<size) size = r->size;
    if (ans->size<size) size = ans->size;
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] - r->data[i];
    return OK;
}

static int tabneg(CSOUND *csound, TABARITH *p)
{
    TABDAT *ans = p->ans;
    TABDAT *l   = p->left;
    int size    = ans->size;
    int i;

    if (UNLIKELY(p->ans->data == NULL || p->left->data==NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->size<size) size = l->size;
    if (ans->size<size) size = ans->size;
    for (i=0; i<size; i++)
      ans->data[i] = - l->data[i];
    return OK;
}

static int tabmult(CSOUND *csound, TABARITH *p)
{
    TABDAT *ans = p->ans;
    TABDAT *l   = p->left;
    TABDAT *r   = p->right;
    int size    = ans->size;
    int i;

    if (UNLIKELY(p->ans->data == NULL ||
          p->left->data== NULL || p->right->data==NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    //printf("sizes %d %d %d\n", l->size, r->size, size);
    if (l->size<size) size = l->size;
    if (r->size<size) size = r->size;
    if (ans->size<size) size = ans->size;
    if (ans->data==NULL) 
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] * r->data[i];
    return OK;
}

static int tabdiv(CSOUND *csound, TABARITH *p)
{
    TABDAT *ans = p->ans;
    TABDAT *l   = p->left;
    TABDAT *r   = p->right;
    int size    = ans->size;
    int i;

    if (UNLIKELY(p->ans->data == NULL ||
          p->left->data== NULL || p->right->data==NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->size<size) size = l->size;
    if (r->size<size) size = r->size;
    if (ans->size<size) size = ans->size;
    for (i=0; i<size; i++)
      if (LIKELY(r->data[i]!=0))
        ans->data[i] = l->data[i] / r->data[i];
      else return csound->PerfError(csound, Str("division by zero in t-var"));
    return OK;
}

static int tabrem(CSOUND *csound, TABARITH *p)
{
    TABDAT *ans = p->ans;
    TABDAT *l   = p->left;
    TABDAT *r   = p->right;
    int size    = ans->size;
    int i;

    if (UNLIKELY(p->ans->data == NULL ||
          p->left->data== NULL || p->right->data==NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->size<size) size = l->size;
    if (r->size<size) size = r->size;
    if (ans->size<size) size = ans->size;
    for (i=0; i<size; i++)
      ans->data[i] = MOD(l->data[i], r->data[i]);
    return OK;
}

static int tabimult(CSOUND *csound, TABARITH1 *p)
{
    TABDAT *ans = p->ans;
    TABDAT *l   = p->left;
    MYFLT r     = *p->right;
    int size    = ans->size;
    int i;

    if (UNLIKELY(p->ans->data == NULL || p->left->data== NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->size<size) size = l->size;
    if (ans->size<size) size = ans->size;
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] * r;
    return OK;
}

static int tabidiv(CSOUND *csound, TABARITH1 *p)
{
    TABDAT *ans = p->ans;
    TABDAT *l   = p->left;
    MYFLT r     = *p->right;
    int size    = ans->size;
    int i;

    if (UNLIKELY(r==FL(0.0)))
      return csound->PerfError(csound, Str("division by zero i t-var"));
    if (UNLIKELY(p->ans->data == NULL || p->left->data== NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->size<size) size = l->size;
    if (ans->size<size) size = ans->size;
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] / r;
    return OK;
}

static int tabirem(CSOUND *csound, TABARITH1 *p)
{
    TABDAT *ans = p->ans;
    TABDAT *l   = p->left;
    MYFLT r     = *p->right;
    int size    = ans->size;
    int i;

    if (UNLIKELY(r==FL(0.0)))
      return csound->PerfError(csound, Str("division by zero i t-var"));
    if (UNLIKELY(p->ans->data == NULL || p->left->data== NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->size<size) size = l->size;
    if (ans->size<size) size = ans->size;
    for (i=0; i<size; i++)
      ans->data[i] = MOD(l->data[i], r);
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
    MYFLT ans;

    if (UNLIKELY(t->data == NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    ans = t->data[0];
    for (i=1; i<size; i++)
      if (t->data[i]>ans) ans = t->data[i];
    *p->ans = ans;
    return OK;
}

static int tabmin(CSOUND *csound, TABQUERY *p)
{
    TABDAT *t = p->tab;
    int i, size = t->size;
    MYFLT ans;

    if (UNLIKELY(t->data == NULL))
      return csound->PerfError(csound, Str("t-variable not initialised"));
    ans = t->data[0];
    for (i=1; i<size; i++)
      if (t->data[i]<ans) ans = t->data[i];
    *p->ans = ans;
    return OK;
}

static int tabsum(CSOUND *csound, TABQUERY *p)
{
    TABDAT *t = p->tab;
    int i, size = t->size;
    MYFLT ans;

    if (UNLIKELY(t->data == NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));
    ans = t->data[0];

    for (i=1; i<size; i++)
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
    MYFLT tmin;
    MYFLT tmax;
    int i;
    MYFLT range;

   if (UNLIKELY(t->data == NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));
    tmin = t->data[strt];
    tmax = tmin;

    // Correct start and ending points
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
    int    len;
} TABCPY;

static int tabcopy_set(CSOUND *csound, TABCPY *p)
{
    //int sizes,sized;
    if (UNLIKELY(p->src->data==NULL))
      return csound->InitError(csound, Str("t-variable not initialised"));
    tabensure(csound, p->dst, p->src->size);
    memmove(p->dst->data, p->src->data, p->len);
    return OK;
}

static int tabcopy(CSOUND *csound, TABCPY *p)
{
   if (UNLIKELY(p->dst->data==NULL || p->src->data==NULL))
       return csound->InitError(csound, Str("t-variable not initialised"));
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
    int    len;
} TABGEN;


static int tabgen_set(CSOUND *csound, TABGEN *p)
{
    MYFLT *data =  p->tab->data;
    MYFLT start = *p->start;
    MYFLT end   = *p->end;
    MYFLT incr  = *p->incr;
    int i,size =  (end - start)/incr + 1;

    printf("start=%f end=%f incr=%f size=%d\n", start, end, incr, size);
    if (UNLIKELY(size < 0))
      csound->InitError(csound,
                        Str("inconsistent start, end and increment parameters"));
    tabensure(csound, p->tab, size);
    if (UNLIKELY(p->tab->data==NULL)) {
      tabensure(csound, p->tab, size);
      p->tab->size = size;
    }
    else
      size = p->tab->size;
    data =  p->tab->data;
    printf("size=%d\n[", size);
    for (i=0; i < size; i++) {
      data[i] = start;
      printf("%f ", start);
      start += incr;
    }
    printf("]\n");

    return OK;
}

static int ftab2tab(CSOUND *csound, TABCOPY *p)
{
    FUNC        *ftp;
    int         fsize;
    MYFLT       *fdata;
    int tlen;

    if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL))
        return csound->PerfError(csound, Str("No table for copy2ftab"));
    fsize = ftp->flen;
    if (UNLIKELY(p->tab->data==NULL)) {
      tabensure(csound, p->tab, fsize);
      p->tab->size = fsize;
    }
    tlen = p->tab->size;
    fdata = ftp->ftable;
    if (fsize<tlen) tlen = fsize;
    memcpy(p->tab->data, fdata, sizeof(MYFLT)*tlen);
    return OK;
}

typedef struct {
    OPDS h;
    TABDAT *tab, *tabin;
    MYFLT *start, *end;
    int    len;
} TABSLICE;


static int tabslice_set(CSOUND *csound, TABSLICE *p){

    MYFLT *tabin = p->tabin->data;
    int start = (int) *p->start;
    int end   = (int) *p->end;
    int size = end - start;
    if (UNLIKELY(size < 0))
      csound->InitError(csound, Str("inconsistent start, end parameters"));
    if (UNLIKELY(size > p->tabin->size)) {
      printf("size=%d old tab size = %d\n", size, p->tabin->size);
      csound->InitError(csound, Str("slice larger than original size"));
    }
    if (UNLIKELY(p->tab->data==NULL)) {
      tabensure(csound, p->tab, size);
      p->tab->size = size;
    }
    else size = p->tab->size;
    memcpy(p->tab->data, tabin+start*sizeof(MYFLT),sizeof(MYFLT)*size);
    return OK;
}

typedef struct {
    OPDS h;
    TABDAT *tab, *tabin;
    MYFLT *str;
    int    len;
    OENTRY *opc;
} TABMAP;

static int tabmap_set(CSOUND *csound, TABMAP *p)
{
    MYFLT *data, *tabin = p->tabin->data;
    char func[64];
    int n, size;
    OENTRY *opc  = NULL;
    EVAL  eval;

    strncpy(func,  (char *)p->str, 64);
    strncat(func, ".i", 64);

    if (UNLIKELY(p->tabin->data == NULL))
       return csound->InitError(csound, Str("tvar not initialised"));

    size = p->tabin->size;
    if (UNLIKELY(p->tab->data==NULL)) {
      tabensure(csound, p->tab, size);
      p->tab->size = size;
    }
    else size = size < p->tab->size ? size : p->tab->size;
    data =  p->tab->data;

    opc = csound->opcodlst;
    for (n=0; opc < csound->oplstend; opc++, n++)
      if(!strcmp(func, opc->opname)) break;

    if (UNLIKELY(opc == csound->oplstend))
      return csound->InitError(csound, Str("%s not found, %d opcodes"), func, n);
    p->opc = opc;
    /* int     (*iopadr)(CSOUND *, void *p); */
    for (n=0; n < size; n++) {
      eval.a = &tabin[n];
      eval.r = &data[n];
      opc->iopadr(csound, (void *) &eval);
    }

    return OK;
}

static int tabmap_perf(CSOUND *csound, TABMAP *p)
{
    MYFLT *data =  p->tab->data, *tabin = p->tabin->data;
    char func[64];
    int n, size;
    OENTRY *opc  = p->opc;
    EVAL  eval;

    strncpy(func,  (char *)p->str, 64);
    strncat(func, ".k", 64);

    if (UNLIKELY(p->tabin->data == NULL))
      return csound->PerfError(csound, Str("tvar not initialised"));
    if (UNLIKELY(p->tab->data==NULL))
      return csound->PerfError(csound, Str("tvar not initialised"));
    size = p->tab->size;

    /* opc = csound->opcodlst; */
    /* for(n=0; opc < csound->oplstend; opc++, n++) */
    /*   if(!strcmp(func, opc->opname)) break; */

    /* if (UNLIKELY(opc == csound->oplstend)) */
    /*   return csound->PerfError(csound, Str("%s not found, %d opcodes"), 
                                  func, n); */

    for (n=0; n < size; n++) {
      eval.a = &tabin[n];
      eval.r = &data[n];
      opc->kopadr(csound, (void *) &eval);
    }

    return OK;
}

int tablength(CSOUND *csopund, TABQUERY *p)
{
    if (UNLIKELY(p->tab==NULL)) *p->ans = -FL(1.0);
    else *p->ans = p->tab->size;
    return OK;
}

static OENTRY tabvars_localops[] =
{
  { "##plustab", sizeof(TABARITH), 3, "t", "tt", (SUBR)tabarithset, (SUBR)tabadd },
  { "##suntab",  sizeof(TABARITH), 3, "t", "tt", (SUBR)tabarithset, (SUBR)tabsub },
  { "##negtab",  sizeof(TABARITH), 3, "t", "t",  (SUBR)tabarithset1, (SUBR)tabneg },
  { "##multtab", sizeof(TABARITH), 3, "t", "tt", (SUBR)tabarithset, (SUBR)tabmult },
  { "##divtab",  sizeof(TABARITH), 3, "t", "tt", (SUBR)tabarithset, (SUBR)tabdiv },
  { "##remtab",  sizeof(TABARITH), 3, "t", "tt", (SUBR)tabarithset, (SUBR)tabrem },
  { "##multitab", sizeof(TABARITH1), 3, "t", "ti",
                                              (SUBR)tabarithset1, (SUBR)tabimult },
  { "##divitab",  sizeof(TABARITH1), 3, "t", "ti",
                                               (SUBR)tabarithset1, (SUBR)tabidiv },
  { "##remitab",  sizeof(TABARITH1), 3, "t", "ti",
                                               (SUBR)tabarithset1, (SUBR)tabirem },
  { "maxtab", sizeof(TABQUERY), 3, "k", "t", (SUBR) tabqset, (SUBR) tabmax },
  { "mintab", sizeof(TABQUERY), 3, "k", "t", (SUBR) tabqset, (SUBR) tabmin },
  { "sumtab", sizeof(TABQUERY), 3, "k", "t", (SUBR) tabqset, (SUBR) tabsum },
  { "scalet", sizeof(TABSCALE), 3, "", "tkkOJ",(SUBR) tabscaleset,(SUBR) tabscale },
  { "#copytab", sizeof(TABCPY), 3, "t", "t", (SUBR) tabcopy_set, (SUBR)tabcopy },
  { "#tabgen", sizeof(TABGEN), 1, "t", "iip", (SUBR) tabgen_set, NULL, NULL},
  { "#tabmap_i", sizeof(TABMAP), 1, "t", "tS", (SUBR) tabmap_set, NULL, NULL},
  { "#tabmap", sizeof(TABMAP), 3, "t", "tS", (SUBR) tabmap_set, (SUBR) tabmap_perf},
  { "#tabslice", sizeof(TABSLICE), 1, "t", "tii", (SUBR) tabslice_set, NULL, NULL},
  { "copy2ftab", sizeof(TABCOPY), TW|2, "", "tk", NULL, (SUBR) tab2ftab },
  { "copy2ttab", sizeof(TABCOPY), TR|2, "", "tk", NULL, (SUBR) ftab2tab },
  { "lentab.i", sizeof(TABQUERY), 1, "i", "t", (SUBR) tablength },
  { "lentab.k", sizeof(TABQUERY), 1, "k", "t", NULL, (SUBR) tablength }

};
// reverse, scramble, mirror, stutter, rotate, ...
// jpff: stutter is an interesting one (very musical). It basically
//          randomly repeats (holds) values based on a probability parameter


LINKAGE1(tabvars_localops)
