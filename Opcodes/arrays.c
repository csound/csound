/*
    arrays.c:

    Copyright (C) 2011,2012 John ffitch, Steven Yi

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
    OPDS    h;
    ARRAYDAT  *arrayDat;
} ARRAYDEL;

typedef struct {
    OPDS    h;
    ARRAYDAT* arrayDat;
    MYFLT   *isizes[VARGMAX];
} ARRAYINIT;

typedef struct {
    OPDS    h;
    ARRAYDAT* arrayDat;
    void* value;
    MYFLT   *indexes[VARGMAX];
} ARRAY_SET;

typedef struct {
    OPDS    h;
    MYFLT*   out;
    ARRAYDAT* arrayDat;
    MYFLT   *indexes[VARGMAX];
} ARRAY_GET;

#ifdef SOME_FINE_DAY
static int array_del(CSOUND *csound, void *p)
{
    ARRAYDAT *t = ((ARRAYDEL*)p)->arrayDat;
    t->arrayType = NULL; // types cleaned up later
    mfree(csound, t->data);
    mfree(csound, p);           /* Unlikely to free the p */
    return OK;
}
#endif

static int array_init(CSOUND *csound, ARRAYINIT *p)
{
    ARRAYDAT* arrayDat = p->arrayDat;
    int i, size;

    int inArgCount = p->INOCOUNT;

    if (UNLIKELY(inArgCount == 0))
      return csound->InitError(csound,
                               Str("Error: no sizes set for array initialization"));

    arrayDat->dimensions = inArgCount;
    arrayDat->sizes = mcalloc(csound, sizeof(int) * inArgCount);
    for (i = 0; i < inArgCount; i++) {
      arrayDat->sizes[i] = MYFLT2LRND(*p->isizes[0]);
    }

    size = arrayDat->sizes[0];

    if (inArgCount > 1) {
      for (i = 1; i < inArgCount; i++) {
        size *= arrayDat->sizes[i];
      }
      size = MYFLT2LRND(size);
    }

    CS_VARIABLE* var = arrayDat->arrayType->createVariable(csound, NULL);

//    if(arrayDat->data != NULL) {
//        mfree(csound, arrayDat->data);
//    }
    arrayDat->arrayMemberSize = var->memBlockSize;
    int memSize = var->memBlockSize*size;
    arrayDat->data = mcalloc(csound, memSize);
//    for (i=0; i<size; i++) t->data[i] = val;
//    { // Need to recover space eventually
//        TABDEL *op = (TABDEL*) mmalloc(csound, sizeof(TABDEL));
//        op->h.insdshead = ((OPDS*) p)->insdshead;
//        op->tab = t;
//        csound->RegisterDeinitCallback(csound, op, tabdel);
//    }
    return OK;
}

static int array_set(CSOUND* csound, ARRAY_SET *p) {
    ARRAYDAT* dat = p->arrayDat;
    MYFLT* mem = dat->data;
    int i;
    int end, index, incr;

    int indefArgCount = p->INOCOUNT - 2;

    if (UNLIKELY(indefArgCount == 0)) {
      csoundErrorMsg(csound, "Error: no indexes set for array set\n");
      return CSOUND_ERROR;
    }
    if (UNLIKELY(indefArgCount>dat->dimensions))
      return csound->PerfError(csound, 
                               Str("Array dimension %d out of range "
                                   "for dimensions %d\n"),
                               indefArgCount, dat->dimensions);
    end = indefArgCount - 1;
    index = MYFLT2LRND(*p->indexes[end]);
    if (UNLIKELY(index >= dat->sizes[end] || index<0))
      return csound->PerfError(csound, 
                               Str("Array index %d out of range (0,%d) "
                                   "for dimension %d\n"),
                               index, dat->sizes[end]-1, indefArgCount);

    if (indefArgCount > 1) {
      for (i = end - 1; i >= 0; i--) {
        int ind = MYFLT2LRND(*p->indexes[i]);
        if (UNLIKELY(ind >= dat->sizes[i] || ind<0))
          return csound->PerfError(csound, 
                                   Str("Array index %d out of range (0,%d) "
                                       "for dimension %d\n"), ind, 
                                   dat->sizes[i]-1, i+1);
        index += ind * dat->sizes[i + 1];
      }
    }

    incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
    mem += incr;
    memcpy(mem, p->value, dat->arrayMemberSize);
    return OK;
}

static int array_get(CSOUND* csound, ARRAY_GET *p) {
    ARRAYDAT* dat = p->arrayDat;
    MYFLT* mem = dat->data;
    int i;
    int incr;
    int end;
    int index;
    int indefArgCount = p->INOCOUNT - 1;

    if (UNLIKELY(indefArgCount == 0))
      csound->PerfError(csound, Str("Error: no indexes set for array get"));
    if (UNLIKELY(indefArgCount>dat->dimensions))
      return csound->PerfError(csound, 
                               Str("Array dimension %d out of range "
                                   "for dimensions %d\n"),
                               indefArgCount, dat->dimensions);
    end = indefArgCount - 1;
    index = MYFLT2LRND(*p->indexes[end]);
    if (UNLIKELY(index >= dat->sizes[end] || index<0))
      return csound->PerfError(csound, 
                               Str("Array index %d out of range (0,%d) "
                                   "for dimension %d\n"),
                               index, dat->sizes[end]-1, end+1);
    if (indefArgCount > 1) {
        for (i = end - 1; i >= 0; i--) {
          int ind = MYFLT2LRND(*p->indexes[i]);
          if (UNLIKELY(ind >= dat->sizes[i] || ind<0))
            return csound->PerfError(csound, 
                                     Str("Array index %d out of range (0,%d) "
                                         "for dimension %d\n"), ind,
                                     dat->sizes[i]-1, i+1);
          index += ind * dat->sizes[i + 1];
        }
    }

    incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
    mem += incr;
    memcpy(p->out, mem, dat->arrayMemberSize);
    return OK;
}

//
//int tassign(CSOUND *csound, ASSIGNT *p)
//{
//    ARRAYDAT *t = p->tab;
//    int ind = MYFLT2LRND(*p->ind);
//    if (ind<0 || ind>t->size)
//        return csound->PerfError(csound,
//                                 Str("Index %d out of range [0,%d] in t[]\n"),
//                                 ind, t->size);
//    t->data[ind] = *p->val;
//    return OK;
//}
//
//int tabref_check(CSOUND *csound, TABREF *p)
//{
//    if (UNLIKELY(p->tab->data==NULL))
//        return csound->InitError(csound, Str("Vector not initialised\n"));
//    return OK;
//}
//
//int tabref(CSOUND *csound, TABREF *p)
//{
//    int ind = MYFLT2LRND(*p->ind);
//    TABDAT *t = p->tab;
//    if (ind<0 || ind>t->size)
//        return csound->PerfError(csound,
//                                 Str("Index %d out of range [0,%d] in t[]\n"),
//                                 ind, t->size);
//    *p->ans = t->data[ind];
//    return OK;
//}


//typedef struct {
//    OPDS h;
//    TABDAT *ans, *left, *right;
//} TABARITH;
//
//typedef struct {
//    OPDS h;
//    TABDAT *ans, *left;
//    MYFLT *right;
//} TABARITH1;
//
typedef struct {
    OPDS h;
    MYFLT  *ans;
    ARRAYDAT *tab;
} TABQUERY;

typedef struct {
    OPDS h;
    ARRAYDAT *tab;
    MYFLT  *kfn;
} TABCOPY;

typedef struct {
   OPDS h;
   ARRAYDAT *tab;
   MYFLT  *kmin, *kmax;
   MYFLT  *kstart, *kend;
} TABSCALE;

static inline void tabensure(CSOUND *csound, ARRAYDAT *p, int size)
{
    if (p->data && p->dimensions==1 && p->sizes[0]>= size) {
      uint32_t ss = sizeof(MYFLT)*size;
      p->data = (MYFLT*)mmalloc(csound, ss);
      p->dimensions = 1;
      p->arrayMemberSize = sizeof(MYFLT);
      p->sizes = (int*)mmalloc(csound, sizeof(int));
      p->sizes[0] = size;
    }
}

//static int tabarithset(CSOUND *csound, TABARITH *p)
//{
//    if (LIKELY(p->left->data && p->right->data)) {
//      /* size is the smallest of the two */
//      int size = p->left->size < p->right->size ? p->left->size : p->right->size;
//      tabensure(csound, p->ans, size);
//      p->ans->size = size;
//      return OK;
//    }
//    else return csound->InitError(csound, Str("t-variable not initialised"));
//}
//
//static int tabarithset1(CSOUND *csound, TABARITH *p)
//{
//    if (LIKELY(p->left->data)) {
//      /* size is the smallest of the two */
//      int size = p->left->size;
//      tabensure(csound, p->ans, size);
//      p->ans->size = size;
//      return OK;
//    }
//    else return csound->InitError(csound, Str("t-variable not initialised"));
//}
//
//static int tabadd(CSOUND *csound, TABARITH *p)
//{
//    TABDAT *ans = p->ans;
//    TABDAT *l   = p->left;
//    TABDAT *r   = p->right;
//    int size    = ans->size;
//    int i;
//
//    if (UNLIKELY(p->ans->data == NULL ||
//          p->left->data==NULL || p->right->data==NULL))
//         return csound->PerfError(csound, Str("t-variable not initialised"));
//
//    if (l->size<size) size = l->size;
//    if (r->size<size) size = r->size;
//    if (ans->size<size) size = ans->size;
//    for (i=0; i<size; i++)
//      ans->data[i] = l->data[i] + r->data[i];
//    return OK;
//}
//
//static int tabsub(CSOUND *csound, TABARITH *p)
//{
//    TABDAT *ans = p->ans;
//    TABDAT *l   = p->left;
//    TABDAT *r   = p->right;
//    int size    = ans->size;
//    int i;
//
//    if (UNLIKELY(p->ans->data == NULL ||
//          p->left->data==NULL || p->right->data==NULL))
//         return csound->PerfError(csound, Str("t-variable not initialised"));
//
//    if (l->size<size) size = l->size;
//    if (r->size<size) size = r->size;
//    if (ans->size<size) size = ans->size;
//    for (i=0; i<size; i++)
//      ans->data[i] = l->data[i] - r->data[i];
//    return OK;
//}
//
//static int tabneg(CSOUND *csound, TABARITH *p)
//{
//    TABDAT *ans = p->ans;
//    TABDAT *l   = p->left;
//    int size    = ans->size;
//    int i;
//
//    if (UNLIKELY(p->ans->data == NULL || p->left->data==NULL))
//         return csound->PerfError(csound, Str("t-variable not initialised"));
//
//    if (l->size<size) size = l->size;
//    if (ans->size<size) size = ans->size;
//    for (i=0; i<size; i++)
//      ans->data[i] = - l->data[i];
//    return OK;
//}
//
//static int tabmult(CSOUND *csound, TABARITH *p)
//{
//    TABDAT *ans = p->ans;
//    TABDAT *l   = p->left;
//    TABDAT *r   = p->right;
//    int size    = ans->size;
//    int i;
//
//    if (UNLIKELY(p->ans->data == NULL ||
//          p->left->data== NULL || p->right->data==NULL))
//         return csound->PerfError(csound, Str("t-variable not initialised"));
//
//    //printf("sizes %d %d %d\n", l->size, r->size, size);
//    if (l->size<size) size = l->size;
//    if (r->size<size) size = r->size;
//    if (ans->size<size) size = ans->size;
//    if (ans->data==NULL)
//    for (i=0; i<size; i++)
//      ans->data[i] = l->data[i] * r->data[i];
//    return OK;
//}
//
//static int tabdiv(CSOUND *csound, TABARITH *p)
//{
//    TABDAT *ans = p->ans;
//    TABDAT *l   = p->left;
//    TABDAT *r   = p->right;
//    int size    = ans->size;
//    int i;
//
//    if (UNLIKELY(p->ans->data == NULL ||
//          p->left->data== NULL || p->right->data==NULL))
//         return csound->PerfError(csound, Str("t-variable not initialised"));
//
//    if (l->size<size) size = l->size;
//    if (r->size<size) size = r->size;
//    if (ans->size<size) size = ans->size;
//    for (i=0; i<size; i++)
//      if (LIKELY(r->data[i]!=0))
//        ans->data[i] = l->data[i] / r->data[i];
//      else return csound->PerfError(csound, Str("division by zero in t-var"));
//    return OK;
//}
//
//static int tabrem(CSOUND *csound, TABARITH *p)
//{
//    TABDAT *ans = p->ans;
//    TABDAT *l   = p->left;
//    TABDAT *r   = p->right;
//    int size    = ans->size;
//    int i;
//
//    if (UNLIKELY(p->ans->data == NULL ||
//          p->left->data== NULL || p->right->data==NULL))
//         return csound->PerfError(csound, Str("t-variable not initialised"));
//
//    if (l->size<size) size = l->size;
//    if (r->size<size) size = r->size;
//    if (ans->size<size) size = ans->size;
//    for (i=0; i<size; i++)
//      ans->data[i] = MOD(l->data[i], r->data[i]);
//    return OK;
//}
//
//static int tabimult(CSOUND *csound, TABARITH1 *p)
//{
//    TABDAT *ans = p->ans;
//    TABDAT *l   = p->left;
//    MYFLT r     = *p->right;
//    int size    = ans->size;
//    int i;
//
//    if (UNLIKELY(p->ans->data == NULL || p->left->data== NULL))
//         return csound->PerfError(csound, Str("t-variable not initialised"));
//
//    if (l->size<size) size = l->size;
//    if (ans->size<size) size = ans->size;
//    for (i=0; i<size; i++)
//      ans->data[i] = l->data[i] * r;
//    return OK;
//}
//
//static int tabidiv(CSOUND *csound, TABARITH1 *p)
//{
//    TABDAT *ans = p->ans;
//    TABDAT *l   = p->left;
//    MYFLT r     = *p->right;
//    int size    = ans->size;
//    int i;
//
//    if (UNLIKELY(r==FL(0.0)))
//      return csound->PerfError(csound, Str("division by zero in t-var"));
//    if (UNLIKELY(p->ans->data == NULL || p->left->data== NULL))
//         return csound->PerfError(csound, Str("t-variable not initialised"));
//
//    if (l->size<size) size = l->size;
//    if (ans->size<size) size = ans->size;
//    for (i=0; i<size; i++)
//      ans->data[i] = l->data[i] / r;
//    return OK;
//}
//
//static int tabirem(CSOUND *csound, TABARITH1 *p)
//{
//    TABDAT *ans = p->ans;
//    TABDAT *l   = p->left;
//    MYFLT r     = *p->right;
//    int size    = ans->size;
//    int i;
//
//    if (UNLIKELY(r==FL(0.0)))
//      return csound->PerfError(csound, Str("division by zero in t-var"));
//    if (UNLIKELY(p->ans->data == NULL || p->left->data== NULL))
//         return csound->PerfError(csound, Str("t-variable not initialised"));
//
//    if (l->size<size) size = l->size;
//    if (ans->size<size) size = ans->size;
//    for (i=0; i<size; i++)
//      ans->data[i] = MOD(l->data[i], r);
//    return OK;
//}
//
static int tabqset(CSOUND *csound, TABQUERY *p)
{
   if (LIKELY(p->tab->data)) return OK;
   return csound->InitError(csound, Str("t-variable not initialised"));
}

static int tabmax(CSOUND *csound, TABQUERY *p)
{
   ARRAYDAT *t = p->tab;
   int i, size = t->sizes[0];
   MYFLT ans;

   if (UNLIKELY(t->data == NULL))
        return csound->PerfError(csound, Str("t-variable not initialised"));
   if (UNLIKELY(t->dimensions!=1))
        return csound->PerfError(csound, Str("t-variable not vector"));

   ans = t->data[0];
   for (i=1; i<size; i++)
     if (t->data[i]>ans) ans = t->data[i];
   *p->ans = ans;
   return OK;
}

static int tabmin(CSOUND *csound, TABQUERY *p)
{
   ARRAYDAT *t = p->tab;
   int i, size = t->sizes[0];
   MYFLT ans;

   if (UNLIKELY(t->data == NULL))
     return csound->PerfError(csound, Str("t-variable not initialised"));
   if (UNLIKELY(t->dimensions!=1))
     return csound->PerfError(csound, Str("t-variable not a vector"));
   ans = t->data[0];
   for (i=1; i<size; i++)
     if (t->data[i]<ans) ans = t->data[i];
   *p->ans = ans;
   return OK;
}

static int tabsum(CSOUND *csound, TABQUERY *p)
{
   ARRAYDAT *t = p->tab;
   int i, size = t->sizes[0];
   MYFLT ans;

   if (UNLIKELY(t->data == NULL))
        return csound->PerfError(csound, Str("t-variable not initialised"));
   if (UNLIKELY(t->dimensions!=1))
        return csound->PerfError(csound, Str("t-variable not a vector"));
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
   ARRAYDAT *t = p->tab;
   MYFLT tmin;
   MYFLT tmax;
   int i;
   MYFLT range;

   if (UNLIKELY(t->data == NULL || t->dimensions!=1))
     return csound->PerfError(csound, Str("t-variable not initialised"));
   tmin = t->data[strt];
   tmax = tmin;

   // Correct start and ending points
   if (end<0) end = t->sizes[0];
   else if (end>t->sizes[0]) end = t->sizes[0];
   else if (end<0) end = 0;
   if (strt<0) strt = 0;
   else if (strt>t->sizes[0]) strt = t->sizes[0];
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
    OPDS     h;
    ARRAYDAT *dst;
    ARRAYDAT *src;
    int      len;
} TABCPY;

//static int tabcopy_set(CSOUND *csound, TABCPY *p)
//{
//    //int sizes,sized;
//    if (UNLIKELY(p->src->data==NULL) || p->src->dimensions!=1)
//      return csound->InitError(csound, Str("t-variable not initialised"));
//    tabensure(csound, p->dst, p->src->sizes[0]);
//    memmove(p->dst->data, p->src->data, sizeof(MYFLT)*p->src->sizes[0]);
//    return OK;
//}

static int tabcopy(CSOUND *csound, TABCPY *p)
{
    if (UNLIKELY(p->src->data==NULL) || p->src->dimensions!=1)
      return csound->InitError(csound, Str("t-variable not initialised"));
    tabensure(csound, p->dst, p->src->sizes[0]);
    memmove(p->dst->data, p->src->data, sizeof(MYFLT)*p->src->sizes[0]);
    return OK;
}

static int tab2ftab(CSOUND *csound, TABCOPY *p)
{
    FUNC        *ftp;
    int fsize;
    MYFLT *fdata;
    int tlen = p->tab->sizes[0];
    if (UNLIKELY(p->tab->data==NULL) || p->tab->dimensions!=1)
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
    ARRAYDAT *tab;
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
      p->tab->sizes[0] = size;
    }
    else
      size = p->tab->sizes[0];
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
      p->tab->sizes[0] = fsize;
    }
    tlen = p->tab->sizes[0];
    fdata = ftp->ftable;
    if (fsize<tlen) tlen = fsize;
    memcpy(p->tab->data, fdata, sizeof(MYFLT)*tlen);
    return OK;
}

typedef struct {
    OPDS h;
    ARRAYDAT *tab, *tabin;
    MYFLT *start, *end;
    int    len;
} TABSLICE;


static int tabslice(CSOUND *csound, TABSLICE *p){

    MYFLT *tabin = p->tabin->data;
    int start = (int) *p->start;
    int end   = (int) *p->end;
    int size = end - start;
    if (UNLIKELY(size < 0))
      csound->InitError(csound, Str("inconsistent start, end parameters"));
    if (UNLIKELY(p->tabin->dimensions!=1 || size > p->tabin->sizes[0])) {
      printf("size=%d old tab size = %d\n", size, p->tabin->sizes[0]);
      csound->InitError(csound, Str("slice larger than original size"));
    }
    if (UNLIKELY(p->tab->data==NULL)) {
      tabensure(csound, p->tab, size);
      p->tab->sizes[0] = size;
    }
    else size = p->tab->sizes[0];
    memcpy(p->tab->data, tabin+start*sizeof(MYFLT),sizeof(MYFLT)*size);
    return OK;
}

typedef struct {
    OPDS h;
    ARRAYDAT *tab, *tabin;
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

    if (UNLIKELY(p->tabin->data == NULL)||p->tabin->dimensions!=1)
       return csound->InitError(csound, Str("tvar not initialised"));

    size = p->tabin->sizes[0];
    if (UNLIKELY(p->tab->data==NULL)) {
      tabensure(csound, p->tab, size);
      p->tab->sizes[0] = size;
    }
    else size = size < p->tab->sizes[0] ? size : p->tab->sizes[0];
    data =  p->tab->data;

    opc = csound->opcodlst;
    for (n=0; opc < csound->oplstend; opc++, n++)
      if(!strcmp(func, opc->opname)) break;

    if (UNLIKELY(opc == csound->oplstend))
      return csound->InitError(csound, Str("%s not found, %d opcodes"), func, n);
    p->opc = opc;
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

    if (UNLIKELY(p->tabin->data == NULL) || p->tabin->dimensions !=1)
      return csound->PerfError(csound, Str("tvar not initialised"));
    if (UNLIKELY(p->tab->data==NULL) || p->tab->dimensions !=1)
      return csound->PerfError(csound, Str("tvar not initialised"));
    size = p->tab->sizes[0];

    for (n=0; n < size; n++) {
      eval.a = &tabin[n];
      eval.r = &data[n];
      opc->kopadr(csound, (void *) &eval);
    }

    return OK;
}

int tablength(CSOUND *csound, TABQUERY *p)
{
   if (UNLIKELY(p->tab==NULL || p->tab->dimensions!=1)) *p->ans = -FL(1.0);
   else *p->ans = p->tab->sizes[0];
   return OK;
}



// reverse, scramble, mirror, stutter, rotate, ...
// jpff: stutter is an interesting one (very musical). It basically
//          randomly repeats (holds) values based on a probability parameter

static OENTRY arrayvars_localops[] =
{
    { "init.0", sizeof(ARRAYINIT), 0, 1, "[.]", "m", (SUBR)array_init },
    { "##array_set.i", sizeof(ARRAY_SET), 0, 1, "", "[.].m", (SUBR)array_set },
    { "##array_set.k", sizeof(ARRAY_SET), 0, 2, "", "[.].z",
                                              NULL, (SUBR)array_set },
    { "##array_get.i", sizeof(ARRAY_GET), 0, 1, ".", "[.].m", (SUBR)array_get },
    { "##array_get.k", sizeof(ARRAY_GET), 0, 2, ".", "[.].z",
      NULL, (SUBR)array_get },
//{"##plustab", sizeof(TABARITH), 0, 3, "t", "tt", (SUBR)tabarithset, (SUBR)tabadd},
//{"##suntab",  sizeof(TABARITH), 0, 3, "t", "tt", (SUBR)tabarithset, (SUBR)tabsub},
//{"##negtab",  sizeof(TABARITH), 0, 3, "t", "t", (SUBR)tabarithset1, (SUBR)tabneg},
//{"##multtab", sizeof(TABARITH), 0, 3, "t", "tt", (SUBR)tabarithset,(SUBR)tabmult},
//{"##divtab",  sizeof(TABARITH), 0, 3, "t", "tt", (SUBR)tabarithset,(SUBR)tabdiv },
//{"##remtab",  sizeof(TABARITH), 0, 3, "t", "tt", (SUBR)tabarithset, (SUBR)tabrem},
//{"##multitab", sizeof(TABARITH1), 0, 3, "t", "ti",
//                                         (SUBR)tabarithset1, (SUBR)tabimult },
//{"##divitab",  sizeof(TABARITH1), 0, 3, "t", "ti",
//                                         (SUBR)tabarithset1, (SUBR)tabidiv },
//{"##remitab",  sizeof(TABARITH1),0,  3, "t", "ti",
//                                         (SUBR)tabarithset1, (SUBR)tabirem },
    { "maxtab", sizeof(TABQUERY), 0, 3, "k", "[k]", (SUBR) tabqset, (SUBR) tabmax },
    { "mintab", sizeof(TABQUERY), 0, 3, "k", "[k]", (SUBR) tabqset, (SUBR) tabmin },
    { "sumtab", sizeof(TABQUERY), 0, 3, "k", "[k]", (SUBR) tabqset, (SUBR) tabsum },
    { "scalet", sizeof(TABSCALE), 0, 3, "",  "[k]kkOJ",
                                               (SUBR) tabscaleset,(SUBR) tabscale },
    { "=.t", sizeof(TABCPY), 0, 2, "[k]", "[k]", NULL, (SUBR)tabcopy },
    { "tabgen", sizeof(TABGEN), 0, 1, "[k]", "iip", (SUBR) tabgen_set, NULL, NULL},
    { "tabmap_i", sizeof(TABMAP), 0, 1, "t", "tS", (SUBR) tabmap_set, NULL, NULL},
    { "tabmap", sizeof(TABMAP), 0, 3, "t", "tS", (SUBR) tabmap_set,
                                                 (SUBR) tabmap_perf},
    { "tabslice", sizeof(TABSLICE), 0, 1, "[k]", "[k]ii",
                                                 NULL, (SUBR) tabslice, NULL },
    { "copy2ftab", sizeof(TABCOPY), TW, 2, "", "[k]k", NULL, (SUBR) tab2ftab },
    { "copy2ttab", sizeof(TABCOPY), TR, 2, "", "[k]k", NULL, (SUBR) ftab2tab },
    { "lentab.i", sizeof(TABQUERY), 0, 1, "i", "[k]", (SUBR) tablength },
    { "lentab.k", sizeof(TABQUERY), 0, 1, "k", "[k]", NULL, (SUBR) tablength }

};

LINKAGE_BUILTIN(arrayvars_localops)
