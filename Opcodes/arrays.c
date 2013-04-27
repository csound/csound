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
    ARRAYDAT* ans;
    MYFLT   *iargs[VARGMAX];
} TABFILL;

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

static inline void tabensure(CSOUND *csound, ARRAYDAT *p, int size)
{
    if (p->data==NULL || (p->dimensions==1 && p->sizes[0] < size)) {
      uint32_t ss = sizeof(MYFLT)*size;
      if (p->data==NULL) p->data = (MYFLT*)mmalloc(csound, ss);
      else p->data = (MYFLT*) mrealloc(csound, p->data, ss);
      p->dimensions = 1;
      p->arrayMemberSize = sizeof(MYFLT);
      p->sizes = (int*)mmalloc(csound, sizeof(int));
      p->sizes[0] = size;
    }
}

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

static int tabfill(CSOUND *csound, TABFILL *p)
{
    int    nargs = p->INOCOUNT;
    int i;
    MYFLT  **valp = p->iargs;
    tabensure(csound, p->ans, nargs);
    for (i=0; i<nargs; i++) p->ans->data[i] = *valp[i];
    return OK;
}

static int array_set(CSOUND* csound, ARRAY_SET *p) {
    ARRAYDAT* dat = p->arrayDat;
    MYFLT* mem = dat->data;
    int i;
    int end, index, incr;

    int indefArgCount = p->INOCOUNT - 2;

    if (UNLIKELY(indefArgCount == 0)) {
      csoundErrorMsg(csound, Str("Error: no indexes set for array set\n"));
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
    /* printf("array_set: mem = %p, incr = %d, value = %f\n", */
    /*        mem, incr, *((MYFLT*)p->value)); */
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

typedef struct {
    OPDS h;
    ARRAYDAT *ans, *left, *right;
} TABARITH;

typedef struct {
    OPDS h;
    ARRAYDAT *ans, *left;
    MYFLT *right;
} TABARITH1;

typedef struct {
    OPDS h;
    ARRAYDAT *ans;
    MYFLT *left;
    ARRAYDAT *right;
} TABARITH2;

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

static int tabarithset(CSOUND *csound, TABARITH *p)
{
    if (LIKELY(p->left->data && p->right->data)) {
      int size;
      if (p->left->dimensions!=1 || p->right->dimensions!=1)
        return
          csound->InitError(csound,
                            Str("Dimensions do not match in array arithmetic"));
      /* size is the smallest of the two */
      size = p->left->sizes[0] < p->right->sizes[0] ?
                     p->left->sizes[0] : p->right->sizes[0];
      tabensure(csound, p->ans, size);
      p->ans->sizes[0] = size;
      return OK;
    }
    else return csound->InitError(csound, Str("t-variable not initialised"));
}

// For cases with array as first arg
static int tabarithset1(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *left = p->left;
    if (LIKELY(left->data)) {
      int size;
      if (left->dimensions!=1)
        return
          csound->InitError(csound,
                        Str("Dimension does not match in array arithmetic"));
      size = left->sizes[0];
      tabensure(csound, p->ans, size);
      p->ans->sizes[0] = size;
      return OK;
    }
    else return csound->InitError(csound, Str("t-variable not initialised"));
}

// For cases with array as second arg
static int tabarithset2(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *right = p->right;
    if (LIKELY(right->data)) {
      int size;
      if (right->dimensions!=1)
        return
          csound->InitError(csound,
                        Str("Dimension does not match in array arithmetic"));
      size = right->sizes[0];
      tabensure(csound, p->ans, size);
      p->ans->sizes[0] = size;
      return OK;
    }
    else return csound->InitError(csound, Str("t-variable not initialised"));
}

static int tabadd(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(p->ans->data == NULL ||
          p->left->data==NULL || p->right->data==NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (r->sizes[0]<size) size = r->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] + r->data[i];
    return OK;
}

static int tabsub(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(p->ans->data == NULL ||
          p->left->data==NULL || p->right->data==NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (r->sizes[0]<size) size = r->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] - r->data[i];
    return OK;
}

static int tabneg(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(p->ans->data == NULL || p->left->data==NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = - l->data[i];
    return OK;
}

static int tabmult(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(p->ans->data == NULL ||
          p->left->data== NULL || p->right->data==NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    //printf("sizes %d %d %d\n", l->sizes[0], r->sizes[0], size);
    if (l->sizes[0]<size) size = l->sizes[0];
    if (r->sizes[0]<size) size = r->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] * r->data[i];
    return OK;
}

static int tabdiv(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(p->ans->data == NULL ||
          p->left->data== NULL || p->right->data==NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (r->sizes[0]<size) size = r->sizes[0];
    for (i=0; i<size; i++)
      if (LIKELY(r->data[i]!=0))
        ans->data[i] = l->data[i] / r->data[i];
      else return csound->PerfError(csound, Str("division by zero in t-var"));
    return OK;
}

static int tabrem(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(p->ans->data == NULL ||
          p->left->data== NULL || p->right->data==NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (r->sizes[0]<size) size = r->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = MOD(l->data[i], r->data[i]);
    return OK;
}

// Add array and scalar
static int tabiadd(CSOUND *csound, ARRAYDAT *ans, ARRAYDAT *l, MYFLT r)
{
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (ans->sizes[0]<size) size = ans->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] + r;
    return OK;
}

// K[]+K
static int tabaiadd(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r     = *p->right;
    return tabiadd(csound, ans, l, r);
}

// K+K[]
static int tabiaadd(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r     = *p->left;
    return tabiadd(csound, ans, l, r);
}

// Subtract K[]-K
static int tabaisub(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r     = *p->right;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(p->ans->data == NULL || l->data== NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (ans->sizes[0]<size) size = ans->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] - r;
    return OK;
}

// Subtract K-K[]
static int tabiasub(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r     = *p->left;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(p->ans->data == NULL || l->data== NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (ans->sizes[0]<size) size = ans->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = r - l->data[i];
    return OK;
}

// Multiply scalar by array
static int tabimult(CSOUND *csound, ARRAYDAT *ans, ARRAYDAT *l, MYFLT r)
{
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (ans->sizes[0]<size) size = ans->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] * r;
    return OK;
}

// K[] * K
static int tabaimult(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r     = *p->right;
    return tabimult(csound, ans, l, r);
}

// K * K[]
static int tabiamult(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r     = *p->left;
    return tabimult(csound, ans, l, r);
}

// K[] / K
static int tabaidiv(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r     = *p->right;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(r==FL(0.0)))
      return csound->PerfError(csound, Str("division by zero in t-var"));
    if (UNLIKELY(ans->data == NULL || l->data== NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (ans->sizes[0]<size) size = ans->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] / r;
    return OK;
}

// K / K[]
static int tabiadiv(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r     = *p->left;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(r==FL(0.0)))
      return csound->PerfError(csound, Str("division by zero in t-var"));
    if (UNLIKELY(ans->data == NULL || l->data== NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (ans->sizes[0]<size) size = ans->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = r / l->data[i];
    return OK;
}

// K[] % K
static int tabairem(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r     = *p->right;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(r==FL(0.0)))
      return csound->PerfError(csound, Str("division by zero in t-var"));
    if (UNLIKELY(p->ans->data == NULL || p->left->data== NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (ans->sizes[0]<size) size = ans->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = MOD(l->data[i], r);
    return OK;
}

// K % K[]
static int tabiarem(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r     = *p->left;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
         return csound->PerfError(csound, Str("t-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (ans->sizes[0]<size) size = ans->sizes[0];
    for (i=0; i<size; i++) {
      if (UNLIKELY(l->data[i]==FL(0.0)))
        return csound->PerfError(csound, Str("division by zero in t-var"));
      else
        ans->data[i] = MOD(r,l->data[i]);
    }
    return OK;
}

static int tabqset(CSOUND *csound, TABQUERY *p)
{
   if (LIKELY(p->tab->data)) return OK;
   return csound->InitError(csound, Str("t-variable not initialised"));
}

static int tabmax(CSOUND *csound, TABQUERY *p)
{
   ARRAYDAT *t = p->tab;
   int i, size = 0;
   MYFLT ans;

   if (UNLIKELY(t->data == NULL))
        return csound->PerfError(csound, Str("t-variable not initialised"));
   /* if (UNLIKELY(t->dimensions!=1)) */
   /*      return csound->PerfError(csound, Str("t-variable not vector")); */

   for (i=0; i<t->dimensions; i++) size += t->sizes[i];
   ans = t->data[0];
   for (i=1; i<size; i++)
     if (t->data[i]>ans) ans = t->data[i];
   *p->ans = ans;
   return OK;
}

static int tabmin(CSOUND *csound, TABQUERY *p)
{
   ARRAYDAT *t = p->tab;
   int i, size = 0;
   MYFLT ans;

   if (UNLIKELY(t->data == NULL))
     return csound->PerfError(csound, Str("t-variable not initialised"));
   /* if (UNLIKELY(t->dimensions!=1)) */
   /*   return csound->PerfError(csound, Str("t-variable not a vector")); */

   for (i=0; i<t->dimensions; i++) size += t->sizes[i];
   ans = t->data[0];
   for (i=1; i<size; i++)
     if (t->data[i]<ans) ans = t->data[i];
   *p->ans = ans;
   return OK;
}

static int tabsum(CSOUND *csound, TABQUERY *p)
{
   ARRAYDAT *t = p->tab;
   int i, size = 0;
   MYFLT ans;

   if (UNLIKELY(t->data == NULL))
        return csound->PerfError(csound, Str("t-variable not initialised"));
   if (UNLIKELY(t->dimensions!=1))
        return csound->PerfError(csound, Str("t-variable not a vector"));
   ans = t->data[0];
   for (i=0; i<t->dimensions; i++) size += t->sizes[i];
   for (i=1; i<size; i++)
     ans += t->data[i];
   *p->ans = ans;
   return OK;
}

static int tabscaleset(CSOUND *csound, TABSCALE *p)
{
   if (LIKELY(p->tab->data && p->tab->dimensions==1)) return OK;
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

   tmin = t->data[strt];
   tmax = tmin;

   // Correct start and ending points
   if (end<0) end = t->sizes[0]-1;
   else if (end>t->sizes[0]) end = t->sizes[0]-1;
   if (strt<0) strt = 0;
   else if (strt>t->sizes[0]) strt = t->sizes[0]-1;
   if (end<strt) {
     int x = end; end = strt; strt = x;
   }
   // get data range
   for (i=strt+1; i<end; i++) {
     if (t->data[i]<tmin) tmin = t->data[i];
     if (t->data[i]>tmax) tmax = t->data[i];
   }
   /* printf("start/end %d/%d max/min = %g/%g tmax/tmin = %g/%g range=%g\n",  */
   /*        strt, end, max, min, tmax, tmin, range); */
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
    ARRAYDAT *t = p->tab;
    int i, tlen = 0;

    if (UNLIKELY(p->tab->data==NULL))
      return csound->PerfError(csound, Str("t-var not initialised"));
    if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL))
        return csound->PerfError(csound, Str("No table for copy2ftab"));
    for (i=0; i<t->dimensions; i++) tlen += t->sizes[i];
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

    //printf("start=%f end=%f incr=%f size=%d\n", start, end, incr, size);
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
    //printf("size=%d\n[", size);
    for (i=0; i < size; i++) {
      data[i] = start;
      //printf("%f ", start);
      start += incr;
    }
    //printf("]\n");

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
    int size = end - start + 1;
    if (UNLIKELY(size < 0))
      csound->InitError(csound, Str("inconsistent start, end parameters"));
    if (UNLIKELY(p->tabin->dimensions!=1 || size > p->tabin->sizes[0])) {
      //printf("size=%d old tab size = %d\n", size, p->tabin->sizes[0]);
      csound->InitError(csound, Str("slice larger than original size"));
    }
    tabensure(csound, p->tab, size);
    memcpy(p->tab->data, tabin+start,sizeof(MYFLT)*size);
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

    strncpy(func,  (char *)p->str, 64);
    strncat(func, ".k", 64);
    opc = csound->opcodlst;
    for (n=0; opc < csound->oplstend; opc++, n++)
      if(!strcmp(func, opc->opname)) break;

    p->opc = opc;
    return OK;
}

static int tabmap_perf(CSOUND *csound, TABMAP *p)
{
    MYFLT *data =  p->tab->data, *tabin = p->tabin->data;
    int n, size;
    OENTRY *opc  = p->opc;
    EVAL  eval;

    if (UNLIKELY(p->tabin->data == NULL) || p->tabin->dimensions !=1)
      return csound->PerfError(csound, Str("tvar not initialised"));
    if (UNLIKELY(p->tab->data==NULL) || p->tab->dimensions !=1)
      return csound->PerfError(csound, Str("tvar not initialised"));
    size = p->tab->sizes[0];

    if (UNLIKELY(opc == csound->oplstend))
      return csound->PerfError(csound, Str("map fn not found at k rate"));
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
    { "fillarray", sizeof(TABFILL), 0, 1, "[k]", "m", (SUBR)tabfill },
    { "##array_set.i", sizeof(ARRAY_SET), 0, 1, "", "[i]im", (SUBR)array_set },
    { "##array_set.i2", sizeof(ARRAY_SET), 0, 3, "", "[.].m",
                                            (SUBR)array_set, (SUBR)array_set },
    { "##array_set.k", sizeof(ARRAY_SET), 0, 2, "", "[.].z",
                                              NULL, (SUBR)array_set },
    { "##array_get.i", sizeof(ARRAY_GET), 0, 1, "i", "[i]m", (SUBR)array_get },
    { "##array_get.i2", sizeof(ARRAY_GET), 0, 3, ".", "[.]m",
                                       (SUBR)array_get, (SUBR)array_get },
    { "##array_get.k", sizeof(ARRAY_GET), 0, 2, ".", "[.]z",
      NULL, (SUBR)array_get },
    /* ******************************************** */
    {"##add.[]", sizeof(TABARITH), 0, 3, "[k]", "[k][k]",
                                         (SUBR)tabarithset, (SUBR)tabadd},
    /* ******************************************** */
    {"##sub.[]",  sizeof(TABARITH), 0, 3, "[k]", "[k][k]",
                                         (SUBR)tabarithset, (SUBR)tabsub},
    //    {"##neg.[]",  sizeof(TABARITH), 0, 3, "[k]", "[k]",
    //                                         (SUBR)tabarithset1, (SUBR)tabneg},
    {"##mul.[]", sizeof(TABARITH), 0, 3, "[k]", "[k][k]",
                                         (SUBR)tabarithset,(SUBR)tabmult},
    {"##div.[]",  sizeof(TABARITH), 0, 3, "[k]", "[k][k]",
                                          (SUBR)tabarithset,(SUBR)tabdiv },
    {"##rem.[]",  sizeof(TABARITH), 0, 3, "[k]", "[k][k]",
                                          (SUBR)tabarithset, (SUBR)tabrem},
    {"##add.[i", sizeof(TABARITH1), 0, 3, "[k]", "[k]i",
                                          (SUBR)tabarithset1, (SUBR)tabaiadd },
    {"##add.i[", sizeof(TABARITH2), 0, 3, "[k]", "i[k]",
                                          (SUBR)tabarithset2, (SUBR)tabiaadd },
    {"##sub.[i", sizeof(TABARITH1), 0, 3, "[k]", "[k]i",
                                          (SUBR)tabarithset1, (SUBR)tabaisub },
    {"##sub.i[", sizeof(TABARITH2), 0, 3, "[k]", "[k]i",
                                          (SUBR)tabarithset2, (SUBR)tabiasub },
    {"##mul.[i", sizeof(TABARITH1), 0, 3, "[k]", "[k]i",
                                          (SUBR)tabarithset1, (SUBR)tabaimult },
    {"##mul.i[", sizeof(TABARITH2), 0, 3, "[k]", "i[k]",
                                          (SUBR)tabarithset2, (SUBR)tabiamult },
    {"##div.[i",  sizeof(TABARITH1), 0, 3, "[k]", "[k]i",
                                          (SUBR)tabarithset1, (SUBR)tabaidiv },
    {"##div.i[",  sizeof(TABARITH2), 0, 3, "[k]", "i[k]",
                                          (SUBR)tabarithset2, (SUBR)tabiadiv },
    {"##rem.[i",  sizeof(TABARITH1),0,  3, "[k]", "[k]i",
                                          (SUBR)tabarithset1, (SUBR)tabairem },
    {"##rem.i[",  sizeof(TABARITH2),0,  3, "[k]", "i[k]",
                                          (SUBR)tabarithset2, (SUBR)tabiarem },
    {"##add.[k", sizeof(TABARITH1), 0, 3, "[k]", "[k]k",
                                          (SUBR)tabarithset1, (SUBR)tabaiadd },
    {"##add.k[", sizeof(TABARITH2), 0, 3, "[k]", "k[k]",
                                          (SUBR)tabarithset2, (SUBR)tabiaadd },
    {"##sub.[k", sizeof(TABARITH1), 0, 3, "[k]", "[k]k",
                                          (SUBR)tabarithset1, (SUBR)tabiasub },
    {"##sub.k[", sizeof(TABARITH2), 0, 3, "[k]", "k[k]",
                                          (SUBR)tabarithset2, (SUBR)tabaisub },
    {"##mul.[k", sizeof(TABARITH1), 0, 3, "[k]", "[k]k",
                                          (SUBR)tabarithset1, (SUBR)tabaimult },
    {"##mul.k[", sizeof(TABARITH2), 0, 3, "[k]", "k[k]",
                                          (SUBR)tabarithset2, (SUBR)tabiamult },
    {"##div.[k",  sizeof(TABARITH1), 0, 3, "[k]", "[k]k",
                                          (SUBR)tabarithset1, (SUBR)tabaidiv },
    {"##div.k[",  sizeof(TABARITH2), 0, 3, "[k]", "k[k]",
                                          (SUBR)tabarithset2, (SUBR)tabiadiv },
    {"##rem.[k",  sizeof(TABARITH1),0,  3, "[k]", "[k]k",
                                          (SUBR)tabarithset1, (SUBR)tabairem },
    {"##rem.k[",  sizeof(TABARITH2),0,  3, "[k]", "k[k]",
                                          (SUBR)tabarithset2, (SUBR)tabiarem },
    { "maxtab",sizeof(TABQUERY),_QQ, 3, "k", "[k]", (SUBR) tabqset, (SUBR) tabmax },
    { "maxarray", sizeof(TABQUERY), 0, 3, "k", "[k]",(SUBR) tabqset,(SUBR) tabmax },
    { "mintab", sizeof(TABQUERY),_QQ, 3, "k", "[k]",(SUBR) tabqset, (SUBR) tabmin },
    { "minarray", sizeof(TABQUERY),0, 3, "k", "[k]",(SUBR) tabqset, (SUBR) tabmin },
    { "sumtab", sizeof(TABQUERY),_QQ, 3, "k", "[k]",(SUBR) tabqset, (SUBR) tabsum },
    { "sumarray", sizeof(TABQUERY),0, 3, "k", "[k]",(SUBR) tabqset, (SUBR) tabsum },
    { "scalet", sizeof(TABSCALE), _QQ, 3, "",  "[k]kkOJ",
                                               (SUBR) tabscaleset,(SUBR) tabscale },
    { "scalearray", sizeof(TABSCALE), 0, 3, "",  "[k]kkOJ",
                                               (SUBR) tabscaleset,(SUBR) tabscale },
    { "=.t", sizeof(TABCPY), 0, 2, "[k]", "[k]", NULL, (SUBR)tabcopy },
    { "tabgen", sizeof(TABGEN), _QQ, 1, "[k]", "iip", (SUBR) tabgen_set, NULL    },
    { "tabmap_i", sizeof(TABMAP), _QQ, 1, "[k]", "[k]S", (SUBR) tabmap_set       },
    { "tabmap", sizeof(TABMAP), _QQ, 3, "[k]", "[k]S", (SUBR) tabmap_set,
                                                 (SUBR) tabmap_perf},
    { "genarray", sizeof(TABGEN),0, 1, "[k]", "iip", (SUBR) tabgen_set, NULL, NULL},
    { "maparray_i", sizeof(TABMAP),0, 1, "[k]", "[k]S", (SUBR) tabmap_set         },
    { "maparray", sizeof(TABMAP), 0, 3, "[k]", "[k]S", (SUBR) tabmap_set,
                                                 (SUBR) tabmap_perf},
    { "tabslice", sizeof(TABSLICE), _QQ, 2, "[k]", "[k]ii",
                                                 NULL, (SUBR) tabslice, NULL },
    { "slicearray", sizeof(TABSLICE), 0, 2, "[k]", "[k]ii",
                                                 NULL, (SUBR) tabslice, NULL },
    { "copy2ftab", sizeof(TABCOPY), TW|_QQ, 2, "", "[k]k", NULL, (SUBR) tab2ftab },
    { "copy2ttab", sizeof(TABCOPY), TR|_QQ, 2, "", "[k]k", NULL, (SUBR) ftab2tab },
    { "copya2ftab", sizeof(TABCOPY), TW, 2, "", "[k]k", NULL, (SUBR) tab2ftab },
    { "copyf2array", sizeof(TABCOPY), TR, 2, "", "[k]k", NULL, (SUBR) ftab2tab },
    { "lentab.i", sizeof(TABQUERY), _QQ, 1, "i", "[k]", (SUBR) tablength },
    { "lentab.k", sizeof(TABQUERY), _QQ, 1, "k", "[k]", NULL, (SUBR) tablength },
    { "lenarray.i", sizeof(TABQUERY), 0, 1, "i", "[k]", (SUBR) tablength },
    { "lenarray.k", sizeof(TABQUERY), 0, 1, "k", "[k]", NULL, (SUBR) tablength }
};

LINKAGE_BUILTIN(arrayvars_localops)
