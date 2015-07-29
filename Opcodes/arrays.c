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
#include "csound_orc_semantics.h"

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
    csound->Free(csound, t->data);
    csound->Free(csound, p);           /* Unlikely to free the p */
    return OK;
}
#endif

static inline void tabensure(CSOUND *csound, ARRAYDAT *p, int size)
{
    if (p->data==NULL || p->dimensions == 0 ||
        (p->dimensions==1 && p->sizes[0] < size)) {
      size_t ss;
      if (p->data == NULL) {
        CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL);
        p->arrayMemberSize = var->memBlockSize;
      }

      ss = p->arrayMemberSize*size;
      if (p->data==NULL) p->data = (MYFLT*)csound->Calloc(csound, ss);
      else p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
      p->dimensions = 1;
      p->sizes = (int*)csound->Malloc(csound, sizeof(int));
      p->sizes[0] = size;
  }
}

static int array_init(CSOUND *csound, ARRAYINIT *p)
{
    ARRAYDAT* arrayDat = p->arrayDat;
    int i, size;

    int inArgCount = p->INOCOUNT;

    if (UNLIKELY(inArgCount == 0))
      return
        csound->InitError(csound,
                          Str("Error: no sizes set for array initialization"));

    for (i = 0; i < inArgCount; i++) {
      if(MYFLT2LRND(*p->isizes[i]) <= 0) {
        return
          csound->InitError(csound,
                          Str("Error: sizes must be > 0 for array initialization"));
      }
    }

    arrayDat->dimensions = inArgCount;
    arrayDat->sizes = csound->Calloc(csound, sizeof(int) * inArgCount);
    for (i = 0; i < inArgCount; i++) {
      arrayDat->sizes[i] = MYFLT2LRND(*p->isizes[i]);
    }

    size = arrayDat->sizes[0];

    if (inArgCount > 1) {
      for (i = 1; i < inArgCount; i++) {
        size *= arrayDat->sizes[i];
      }
      size = MYFLT2LRND(size);
    }

    CS_VARIABLE* var = arrayDat->arrayType->createVariable(csound, NULL);

    arrayDat->arrayMemberSize = var->memBlockSize;
    arrayDat->data = csound->Calloc(csound, var->memBlockSize*size);
  //    for (i=0; i<size; i++) t->data[i] = val;
  //    { // Need to recover space eventually
  //        TABDEL *op = (TABDEL*) csound->Malloc(csound, sizeof(TABDEL));
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
    size_t memMyfltSize;
    MYFLT  **valp = p->iargs;
    tabensure(csound, p->ans, nargs);
    if (p->ans->dimensions > 2) {
      return
        csound->InitError(csound,
                          Str("fillarrray: arrays with dim > 2 not "
                              "currently supported\n"));
    }
    memMyfltSize = p->ans->arrayMemberSize / sizeof(MYFLT);
    for (i=0; i<nargs; i++) {
      p->ans->arrayType->copyValue(csound,
                                   p->ans->data + (i * memMyfltSize),
                                   valp[i]);
    }
    return OK;
}

static int array_set(CSOUND* csound, ARRAY_SET *p)
{
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
      return csound->PerfError(csound, p->h.insdshead,
                               Str("Array dimension %d out of range "
                                   "for dimensions %d\n"),
                               indefArgCount, dat->dimensions);
    end = indefArgCount - 1;
    index = MYFLT2LRND(*p->indexes[end]);
    if (UNLIKELY(index >= dat->sizes[end] || index<0))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("Array index %d out of range (0,%d) "
                                   "for dimension %d\n"),
                               index, dat->sizes[end]-1, indefArgCount);

    if (indefArgCount > 1) {
      for (i = end - 1; i >= 0; i--) {
        int ind = MYFLT2LRND(*p->indexes[i]);
        if (UNLIKELY(ind >= dat->sizes[i] || ind<0))
          return csound->PerfError(csound, p->h.insdshead,
                                   Str("Array index %d out of range (0,%d) "
                                       "for dimension %d\n"), ind,
                                   dat->sizes[i]-1, i+1);
        index += ind * dat->sizes[i + 1];
      }
    }

    incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
    mem += incr;
    //memcpy(mem, p->value, dat->arrayMemberSize);
    dat->arrayType->copyValue(csound, mem, p->value);
    /* printf("array_set: mem = %p, incr = %d, value = %f\n", */
    /*        mem, incr, *((MYFLT*)p->value)); */
    return OK;
}

static int array_get(CSOUND* csound, ARRAY_GET *p)
{
    ARRAYDAT* dat = p->arrayDat;
    MYFLT* mem = dat->data;
    int i;
    int incr;
    int end;
    int index;
    int indefArgCount = p->INOCOUNT - 1;

    if (UNLIKELY(indefArgCount == 0))
      csound->PerfError(csound, p->h.insdshead,
                        Str("Error: no indexes set for array get"));
    if (UNLIKELY(indefArgCount>dat->dimensions))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("Array dimension %d out of range "
                                   "for dimensions %d\n"),
                               indefArgCount, dat->dimensions);
    end = indefArgCount - 1;
    index = MYFLT2LRND(*p->indexes[end]);
    if (UNLIKELY(index >= dat->sizes[end] || index<0))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("Array index %d out of range (0,%d) "
                                   "for dimension %d\n"),
                               index, dat->sizes[end]-1, end+1);
    if (indefArgCount > 1) {
      for (i = end - 1; i >= 0; i--) {
        int ind = MYFLT2LRND(*p->indexes[i]);
        if (UNLIKELY(ind >= dat->sizes[i] || ind<0))
          return csound->PerfError(csound, p->h.insdshead,
                                   Str("Array index %d out of range (0,%d) "
                                       "for dimension %d\n"), ind,
                                   dat->sizes[i]-1, i+1);
        index += ind * dat->sizes[i + 1];
      }
    }

    incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
    mem += incr;
    //    memcpy(p->out, &mem[incr], dat->arrayMemberSize);
    dat->arrayType->copyValue(csound, (void*)p->out, (void*)mem);
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
  MYFLT  *ans, *pos;
  ARRAYDAT *tab;
} TABQUERY;

typedef struct {
  OPDS h;
  MYFLT  *ans;
  ARRAYDAT *tab;
  MYFLT  *opt;
} TABQUERY1;

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
    else return csound->InitError(csound, Str("array-variable not initialised"));
}

static int tabiadd(CSOUND *csound, ARRAYDAT *ans, ARRAYDAT *l, MYFLT r, void *p);

// For cases with array as first arg
static int tabarithset1(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *left = p->left;
    if(p->ans->data == left->data) {
      printf("same ptr \n");
      return OK;
    }

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
    else return csound->InitError(csound, Str("array-variable not initialised"));
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
    else return csound->InitError(csound, Str("array-variable not initialised"));
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
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

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
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (r->sizes[0]<size) size = r->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = l->data[i] - r->data[i];
    return OK;
}

/* static int tabneg(CSOUND *csound, TABARITH *p) */
/* { */
/*     ARRAYDAT *ans = p->ans; */
/*     ARRAYDAT *l   = p->left; */
/*     int size    = ans->sizes[0]; */
/*     int i; */

/*     if (UNLIKELY(p->ans->data == NULL || p->left->data==NULL)) */
/*          return csound->PerfError(csound, p->h.insdshead, */
/*                                   Str("array-variable not initialised")); */

/*     if (l->sizes[0]<size) size = l->sizes[0]; */
/*     for (i=0; i<size; i++) */
/*       ans->data[i] = - l->data[i]; */
/*     return OK; */
/* } */

static int tabmult(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(p->ans->data == NULL ||
                 p->left->data== NULL || p->right->data==NULL))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

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
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (r->sizes[0]<size) size = r->sizes[0];
    for (i=0; i<size; i++)
      if (LIKELY(r->data[i]!=0))
        ans->data[i] = l->data[i] / r->data[i];
      else
        return
          csound->PerfError(csound, p->h.insdshead,
                            Str("division by zero in array-var at index %d"), i);
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
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (r->sizes[0]<size) size = r->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = MOD(l->data[i], r->data[i]);
    return OK;
}

static int tabpow(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int   size    = ans->sizes[0];
    int   i;
    MYFLT tmp;

    if (UNLIKELY(p->ans->data == NULL ||
                 p->left->data== NULL || p->right->data==NULL))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (r->sizes[0]<size) size = r->sizes[0];
    for (i=0; i<size; i++)
      if (LIKELY(l->data[i]>=0) || MODF(r->data[i],&tmp)==FL(0.0))
        ans->data[i] = POWER(l->data[i],r->data[i]);
      else
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("undefined power in array-var at index %d"),
                                    i);
    return OK;
}


#define IIARRAY(opcode,fn)                              \
  static int opcode(CSOUND *csound, TABARITH *p)        \
  {                                                     \
    if (!tabarithset(csound, p)) return fn(csound, p);  \
    else return NOTOK;                                  \
  }

IIARRAY(tabaddi,tabadd)
IIARRAY(tabsubi,tabsub)
IIARRAY(tabmulti,tabmult)
IIARRAY(tabdivi,tabdiv)
IIARRAY(tabremi,tabrem)
IIARRAY(tabpowi,tabpow)

// Add array and scalar
static int tabiadd(CSOUND *csound, ARRAYDAT *ans, ARRAYDAT *l, MYFLT r, void *p)
{
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, ((TABARITH *) p)->h.insdshead,
                               Str("array-variable not initialised"));

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
    MYFLT r       = *p->right;
    return tabiadd(csound, ans, l, r, p);
}

// K+K[]
static int tabiaadd(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r       = *p->left;
    return tabiadd(csound, ans, l, r, p);
}

// Subtract K[]-K
static int tabaisub(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r       = *p->right;
    int size      = ans->sizes[0];
    int i;

    if (UNLIKELY(p->ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

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
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (ans->sizes[0]<size) size = ans->sizes[0];
    for (i=0; i<size; i++)
      ans->data[i] = r - l->data[i];
    return OK;
}

// Multiply scalar by array
static int tabimult(CSOUND *csound, ARRAYDAT *ans, ARRAYDAT *l, MYFLT r, void *p)
{
    int size    = ans->sizes[0];
    int i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, ((TABARITH1 *)p)->h.insdshead,
                               Str("array-variable not initialised"));

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
    MYFLT r       = *p->right;
    return tabimult(csound, ans, l, r, p);
}

// K * K[]
static int tabiamult(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r       = *p->left;
    return tabimult(csound, ans, l, r, p);
}

// K[] / K
static int tabaidiv(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r       = *p->right;
    int size      = ans->sizes[0];
    int i;

    if (UNLIKELY(r==FL(0.0)))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("division by zero in array-var"));
    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

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
      return csound->PerfError(csound, p->h.insdshead,
                               Str("division by zero in array-var"));
    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

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
      return csound->PerfError(csound, p->h.insdshead,
                               Str("division by zero in array-var"));
    if (UNLIKELY(p->ans->data == NULL || p->left->data== NULL))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

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
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (ans->sizes[0]<size) size = ans->sizes[0];
    for (i=0; i<size; i++) {
      if (UNLIKELY(l->data[i]==FL(0.0)))
        return
          csound->PerfError(csound, p->h.insdshead,
                            Str("division by zero in array-var at index %d"), i);
      else
        ans->data[i] = MOD(r,l->data[i]);
    }
    return OK;
}

// K[] pow K
static int tabaipow(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r     = *p->right;
    int size    = ans->sizes[0];
    int i;
    MYFLT tmp;
    int intcase = (MODF(r,&tmp)==FL(0.0));

    if (UNLIKELY(p->ans->data == NULL || p->left->data== NULL))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (ans->sizes[0]<size) size = ans->sizes[0];
    for (i=0; i<size; i++)
      if (intcase || LIKELY(l->data[i]>=0))
        ans->data[i] = POWER(l->data[i], r);
      else
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("undefined power in array-var at index %d"),
                                    i);
    return OK;
}

// K ^ K[]
static int tabiapow(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r     = *p->left;
    int size    = ans->sizes[0];
    int i;
    MYFLT tmp;
    int poscase = (r>=FL(0.0));

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));

    if (l->sizes[0]<size) size = l->sizes[0];
    if (ans->sizes[0]<size) size = ans->sizes[0];
    for (i=0; i<size; i++) {
      if (LIKELY(poscase || MODF(l->data[i],&tmp)==FL(0.0)))
        ans->data[i] = POWER(r,l->data[i]);
      else
        return csound->PerfError(csound, p->h.insdshead,
                                 Str("undefined power in array-var at index %d"),
                                    i);
    }
    return OK;
}

#define IiARRAY(opcode,fn)                              \
  static int opcode(CSOUND *csound, TABARITH1 *p)       \
  {                                                     \
    if (!tabarithset1(csound, p)) return fn(csound, p); \
    else return NOTOK;                                  \
  }

IiARRAY(tabaiaddi,tabaiadd)
IiARRAY(tabaisubi,tabaisub)
IiARRAY(tabaimulti,tabaimult)
IiARRAY(tabaidivi,tabaidiv)
IiARRAY(tabairemi,tabairem)
IiARRAY(tabaipowi,tabaipow)

#define iIARRAY(opcode,fn)                              \
  static int opcode(CSOUND *csound, TABARITH2 *p)       \
  {                                                     \
    if (!tabarithset2(csound, p)) return fn(csound, p); \
    else return NOTOK;                                  \
  }

iIARRAY(tabiaaddi,tabiaadd)
iIARRAY(tabiasubi,tabiasub)
iIARRAY(tabiamulti,tabiamult)
iIARRAY(tabiadivi,tabiadiv)
iIARRAY(tabiaremi,tabiarem)
iIARRAY(tabiapowi,tabiapow)




static int tabqset(CSOUND *csound, TABQUERY *p)
{
    if (LIKELY(p->tab->data)) return OK;
    return csound->InitError(csound, Str("array-variable not initialised"));
}

static int tabqset1(CSOUND *csound, TABQUERY1 *p)
{
    if (LIKELY(p->tab->data)) return OK;
    return csound->InitError(csound, Str("array-variable not initialised"));
}

static int tabmax(CSOUND *csound, TABQUERY *p)
{
    ARRAYDAT *t = p->tab;
    int i, size = 0, pos = 0;;
    MYFLT ans;

    if (UNLIKELY(t->data == NULL))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));
    /* if (UNLIKELY(t->dimensions!=1)) */
    /*      return csound->PerfError(csound, p->h.insdshead, */
    /*      Str("array-variable not vector")); */

    for (i=0; i<t->dimensions; i++) size += t->sizes[i];
    ans = t->data[0];
    for (i=1; i<size; i++)
      if (t->data[i]>ans) {
        ans = t->data[i];
        pos = i;
      }
    *p->ans = ans;
    if (p->OUTOCOUNT>1) *p->pos = (MYFLT)pos;
    return OK;
}

static int tabmax1(CSOUND *csound, TABQUERY *p)
{
    if (tabqset(csound, p) == OK) return tabmax(csound, p);
    else return NOTOK;
}

static int tabmin(CSOUND *csound, TABQUERY *p)
{
    ARRAYDAT *t = p->tab;
    int i, size = 0, pos = 0;
    MYFLT ans;

    if (UNLIKELY(t->data == NULL))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));
    /* if (UNLIKELY(t->dimensions!=1)) */
    /*   return csound->PerfError(csound,
         p->h.insdshead, Str("array-variable not a vector")); */

    for (i=0; i<t->dimensions; i++) size += t->sizes[i];
    ans = t->data[0];
    for (i=1; i<size; i++)
      if (t->data[i]<ans) {
        ans = t->data[i];
        pos = i;
      }
    *p->ans = ans;
    if (p->OUTOCOUNT>1) *p->pos = (MYFLT)pos;
    return OK;
}

static int tabmin1(CSOUND *csound, TABQUERY *p)
{
    if (tabqset(csound, p) == OK) return tabmin(csound, p);
    else return NOTOK;
}

static int tabsum(CSOUND *csound, TABQUERY1 *p)
{
    ARRAYDAT *t = p->tab;
    int i, size = 0;
    MYFLT ans;

    if (UNLIKELY(t->data == NULL))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not initialised"));
    if (UNLIKELY(t->dimensions!=1))
      return csound->PerfError(csound, p->h.insdshead,
                               Str("array-variable not a vector"));
    ans = t->data[0];
    for (i=0; i<t->dimensions; i++) size += t->sizes[i];
    for (i=1; i<size; i++)
      ans += t->data[i];
    *p->ans = ans;
    return OK;
}

static int tabsum1(CSOUND *csound, TABQUERY1 *p)
{
    if (tabqset1(csound, p) == OK) return tabsum(csound, p);
    else return NOTOK;
}

static int tabscaleset(CSOUND *csound, TABSCALE *p)
{
    if (LIKELY(p->tab->data && p->tab->dimensions==1)) return OK;
    return csound->InitError(csound, Str("array-variable not initialised"));
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
    if (end<0) end = t->sizes[0];
    else if (end>t->sizes[0]) end = t->sizes[0];
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
    /* printf("start/end %d/%d max/min = %g/%g tmax/tmin = %g/%g range=%g\n",  */
    /*        strt, end, max, min, tmax, tmin, range); */
    range = (max-min)/(tmax-tmin);
    for (i=strt; i<end; i++) {
      t->data[i] = (t->data[i]-tmin)*range + min;
    }
    return OK;
}

static int tabscale1(CSOUND *csound, TABSCALE *p)
{
    if (tabscaleset(csound, p) == OK) return tabscale(csound, p);
    else return NOTOK;
}

typedef struct {
  OPDS     h;
  ARRAYDAT *dst;
  ARRAYDAT *src;
  int      len;
} TABCPY;

static int get_array_total_size(ARRAYDAT* dat)
{
    int i;
    int size;

    if (dat->sizes == NULL) {
      return -1;
    }

    size = dat->sizes[0];
    for (i = 1; i < dat->dimensions; i++) {
      size *= dat->sizes[i];
    }
    return size;
}

static int tabcopy(CSOUND *csound, TABCPY *p)
{
    int i, arrayTotalSize, memMyfltSize;

    if (UNLIKELY(p->src->data==NULL) || p->src->dimensions <= 0 )
      return csound->InitError(csound, Str("array-variable not initialised"));
    if(p->dst->dimensions > 0 && p->src->dimensions != p->dst->dimensions)
      return csound->InitError(csound,
                               Str("array-variable dimensions do not match"));
    if(p->src->arrayType != p->dst->arrayType)
      return csound->InitError(csound, Str("array-variable types do not match"));

    if (p->src == p->dst) return OK;

    arrayTotalSize = get_array_total_size(p->src);
    memMyfltSize = p->src->arrayMemberSize / sizeof(MYFLT);
    p->dst->arrayMemberSize = p->src->arrayMemberSize;

    if (arrayTotalSize != get_array_total_size(p->dst)) {
      p->dst->dimensions = p->src->dimensions;

      p->dst->sizes = csound->Malloc(csound, sizeof(int) * p->src->dimensions);
      memcpy(p->dst->sizes, p->src->sizes, sizeof(int) * p->src->dimensions);

      if (p->dst->data == NULL) {
        p->dst->data = csound->Calloc(csound,
                                      p->src->arrayMemberSize * arrayTotalSize);
      } else {
        csound->ReAlloc(csound, p->dst->data,
                        p->src->arrayMemberSize * arrayTotalSize);
        memset(p->dst->data, 0, p->src->arrayMemberSize * arrayTotalSize);
      }
    }


    for (i = 0; i < arrayTotalSize; i++) {
      int index = (i * memMyfltSize);
      p->dst->arrayType->copyValue(csound,
                                   (void*)(p->dst->data + index),
                                   (void*)(p->src->data + index));
    }

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
      return csound->PerfError(csound,
                               p->h.insdshead, Str("array-var not initialised"));
    if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL))
      return csound->PerfError(csound,
                               p->h.insdshead, Str("No table for copy2ftab"));
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


static int tabgen(CSOUND *csound, TABGEN *p)
{
    MYFLT *data =  p->tab->data;
    MYFLT start = *p->start;
    MYFLT end   = *p->end;
    MYFLT incr  = *p->incr;
    int i, size =  (end - start)/incr + 1;

    //printf("start=%f end=%f incr=%f size=%d\n", start, end, incr, size);
    if (UNLIKELY(size < 0))
      return
        csound->InitError(csound,
                          Str("inconsistent start, end and increment parameters"));
    tabensure(csound, p->tab, size);
    if (UNLIKELY(p->tab->data==NULL)) {
      tabensure(csound, p->tab, size);
      p->tab->sizes[0] = size;
    }
    //else /* This is wrong if array exists only write to specified part */
    //size = p->tab->sizes[0];
    data =  p->tab->data;
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
      return csound->PerfError(csound,
                               p->h.insdshead, Str("No table for copy2ftab"));
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
    int i;
    int memMyfltSize = p->tabin->arrayMemberSize / sizeof(MYFLT);

    if (UNLIKELY(size < 0))
      return csound->InitError(csound, Str("inconsistent start, end parameters"));
    if (UNLIKELY(p->tabin->dimensions!=1 || end >= p->tabin->sizes[0])) {
      //printf("size=%d old tab size = %d\n", size, p->tabin->sizes[0]);
      return csound->InitError(csound, Str("slice larger than original size"));
    }
    tabensure(csound, p->tab, size);

    for (i = start; i < end + 1; i++) {
      int destIndex = i - start;
      p->tab->arrayType->copyValue(csound,
                                   p->tab->data + (destIndex * memMyfltSize),
                                   tabin + (memMyfltSize * i));
    }

    return OK;
}

//#include "str_ops.h"
//// This cheats using strcpy opcode fake
//static int tabsliceS(CSOUND *csound, TABSLICE *p){
//
//    MYFLT *tabin = p->tabin->data;
//    int start = (int) *p->start;
//    int end   = (int) *p->end;
//    int size = end - start + 1, i;
//    STRCPY_OP xx;
//    if (UNLIKELY(size < 0))
//      return csound->InitError(csound,
//                               Str("inconsistent start, end parameters"));
//    if (UNLIKELY(p->tabin->dimensions!=1 || size > p->tabin->sizes[0])) {
//      //printf("size=%d old tab size = %d\n", size, p->tabin->sizes[0]);
//      return csound->InitError(csound, Str("slice larger than original size"));
//    }
//    tabensure(csound, p->tab, size);
//    for (i=0; i<size; i++) {
//      xx.r = p->tab->data +i;
//      xx.str = tabin+start+i;
//      strcpy_opcode_S(csound, &xx);
//    }
//    //memcpy(p->tab->data, tabin+start,sizeof(MYFLT)*size);
//    return OK;
//}

typedef struct {
  OPDS h;
  ARRAYDAT *tab, *tabin;
  STRINGDAT *str;
  int    len;
  OENTRY *opc;
} TABMAP;



static int tabmap_set(CSOUND *csound, TABMAP *p)
{
    MYFLT *data, *tabin = p->tabin->data;
    int n, size;
    OENTRY *opc  = NULL;
    EVAL  eval;

    if (UNLIKELY(p->tabin->data == NULL)||p->tabin->dimensions!=1)
      return csound->InitError(csound, Str("array-var not initialised"));

    size = p->tabin->sizes[0];
    if (UNLIKELY(p->tab->data==NULL)) {
      tabensure(csound, p->tab, size);
      p->tab->sizes[0] = size;
    }
    else size = size < p->tab->sizes[0] ? size : p->tab->sizes[0];
    data =  p->tab->data;

    opc = find_opcode_new(csound, p->str->data, "i", "i");

    if (UNLIKELY(opc == NULL))
      return csound->InitError(csound, Str("%s not found"), p->str->data);
    p->opc = opc;
    for (n=0; n < size; n++) {
      eval.a = &tabin[n];
      eval.r = &data[n];
      opc->iopadr(csound, (void *) &eval);
    }

    opc = find_opcode_new(csound, p->str->data, "k", "k");

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
      return csound->PerfError(csound,
                               p->h.insdshead, Str("array-var not initialised"));
    if (UNLIKELY(p->tab->data==NULL) || p->tab->dimensions !=1)
      return csound->PerfError(csound,
                               p->h.insdshead, Str("array-var not initialised"));
    size = p->tab->sizes[0];

    if (UNLIKELY(opc == NULL))
      return csound->PerfError(csound,
                               p->h.insdshead, Str("map fn not found at k rate"));
    for (n=0; n < size; n++) {
      eval.a = &tabin[n];
      eval.r = &data[n];
      opc->kopadr(csound, (void *) &eval);
    }

    return OK;
}

int tablength(CSOUND *csound, TABQUERY1 *p)
{
    int opt = (int)*p->opt;
    if (UNLIKELY(p->tab==NULL || opt>p->tab->dimensions))
      *p->ans = -FL(1.0);
    else if (UNLIKELY(opt<=0)) *p->ans = p->tab->dimensions;
    else *p->ans = p->tab->sizes[opt-1];
    return OK;
}

typedef struct {
  OPDS h;
  ARRAYDAT *tabin;
  unsigned int    len;
} OUTA;


static int outa_set(CSOUND *csound, OUTA *p)
{
    int len = (p->tabin->dimensions==1?p->tabin->sizes[0]:-1);
    if (len>(int)csound->nchnls) len = csound->nchnls;
    if (len<=0) return NOTOK;
    p->len = len;
    if (p->tabin->arrayMemberSize != (int)(CS_KSMPS*sizeof(MYFLT)))
      return NOTOK;
    return OK;
}

static int outa(CSOUND *csound, OUTA *p)
{
    unsigned int n, m=0, nsmps = CS_KSMPS;
    unsigned int l, pl = p->len;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = nsmps - p->h.insdshead->ksmps_no_end;
    MYFLT       *data = p->tabin->data;
    MYFLT       *sp= CS_SPOUT;
    if (!csound->spoutactive) {
      for (n=0; n<nsmps; n++) {
        for (l=0; l<pl; l++) {
          sp[m++] = (n>=offset && n<early ? data[l+n*nsmps] :FL(0.0)) ;
        }
      }
      csound->spoutactive = 1;
    }
    else {
      for (n=0; n<nsmps; n++) {
        for (l=0; l<p->len; l++) {
          if (n>=offset && n<early)
            sp[m] += data[l+n*nsmps];
          m++;
        }
      }
    }
    return OK;
}

static int ina_set(CSOUND *csound, OUTA *p)
{
    ARRAYDAT *aa = p->tabin;
    // should call ensure here but it is a-rate
    aa->dimensions = 1;
    if (aa->sizes) csound->Free(csound, aa->sizes);
    if (aa->data) csound->Free(csound, aa->data);
    aa->sizes = (int*)csound->Malloc(csound, sizeof(int));
    aa->sizes[0] = p->len = csound->inchnls;
    aa->data = (MYFLT*)
      csound->Malloc(csound, CS_KSMPS*sizeof(MYFLT)*p->len);
    aa->arrayMemberSize = CS_KSMPS*sizeof(MYFLT);
    return OK;
}

static int ina(CSOUND *csound, OUTA *p)
{
    ARRAYDAT *aa = p->tabin;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, l, nsmps = CS_KSMPS;
    MYFLT       *data = aa->data;
    MYFLT       *sp= CS_SPIN;
    uint32_t len = (uint32_t)p->len;
    for (l=0; l<len; l++) {
      sp = CS_SPIN + l;
      memset(data, '\0', nsmps*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        nsmps -= early;
      }
      for (n = 0; n < nsmps; n++) {
        if (n<offset) data[n] = FL(0.0);
        else          data[n] = *sp;
        //printf("chn %d n=%d data=%f (%p)\n", l, n, data[n], &data[n]);
        sp += len;
      }
      data += CS_KSMPS;
    }
    return OK;
}

/*
  transform operations
*/

typedef struct _fft {
  OPDS h;
  ARRAYDAT *out;
  ARRAYDAT *in, *in2;
  MYFLT *f;
  MYFLT b;
  int n;
  AUXCH mem;
} FFT;


static unsigned int isPowerOfTwo (unsigned int x) {
    return ((x != 0) && !(x & (x - 1)));
}


int init_rfft(CSOUND *csound, FFT *p){
  int   N = p->in->sizes[0];
  if(p->in->dimensions > 1)
    return csound->InitError(csound,
     "rfft: only one-dimensional arrays allowed");
  if (isPowerOfTwo(N))
    tabensure(csound, p->out,N);
  else
    tabensure(csound, p->out, N+2);
  return OK;
}

int perf_rfft(CSOUND *csound, FFT *p){
    int N = p->out->sizes[0];
    memcpy(p->out->data,p->in->data,N*sizeof(MYFLT));
    if (isPowerOfTwo(N))
      csound->RealFFT(csound,p->out->data,N);
    else{
      p->out->data[N] = FL(0.0);
      csound->RealFFTnp2(csound,p->out->data,N);
    }
    return OK;
}

int rfft_i(CSOUND *csound, FFT *p){
  if(init_rfft(csound,p) == OK)
    return perf_rfft(csound, p);
  else return NOTOK; 
}

int init_rifft(CSOUND *csound, FFT *p){
  int   N = p->in->sizes[0];
 if(p->in->dimensions > 1)
    return csound->InitError(csound,
       "rifft: only one-dimensional arrays allowed");
  if (isPowerOfTwo(N))
    tabensure(csound, p->out, N);
  else
    tabensure(csound, p->out, N+2);
  return OK;
}

int perf_rifft(CSOUND *csound, FFT *p){
    int N = p->out->sizes[0];
    memcpy(p->out->data,p->in->data,N*sizeof(MYFLT));
    if (isPowerOfTwo(N))
      csound->InverseRealFFT(csound,p->out->data,N);
    else{
      p->out->data[N] = FL(0.0);
      csound->InverseRealFFTnp2(csound,p->out->data,N);
    }
    return OK;
}

int rifft_i(CSOUND *csound, FFT *p){
  if(init_rifft(csound,p) == OK)
    return perf_rifft(csound, p);
  else return NOTOK; 
}

int init_rfftmult(CSOUND *csound, FFT *p){
    int   N = p->in->sizes[0];
    if(N != p->in2->sizes[0])
      return csound->InitError(csound, "array sizes do not match\n");
    /*if(isPowerOfTwo(N))*/
    tabensure(csound, p->out, N);
    /* else
       return
         csound->InitError(csound, "non-pow-of-two case not implemented yet \n");*/
    return OK;
}

int perf_rfftmult(CSOUND *csound, FFT *p){
    int N = p->out->sizes[0];
    csound->RealFFTMult(csound,p->out->data,p->in->data,p->in2->data,N,1);
    return OK;
}

/* these should have been in the CSOUND struct, but are not */
void csoundComplexFFTnp2(CSOUND *csound, MYFLT *buf, int FFTsize);
void csoundInverseComplexFFTnp2(CSOUND *csound, MYFLT *buf, int FFTsize);

int init_fft(CSOUND *csound, FFT *p){
  int   N2 = p->in->sizes[0];
 if(p->in->dimensions > 1)
    return csound->InitError(csound,
      "fft: only one-dimensional arrays allowed");
  tabensure(csound,p->out,N2);
  return OK;
}

int perf_fft(CSOUND *csound, FFT *p){
    int N2 = p->in->sizes[0];
    memcpy(p->out->data,p->in->data,N2*sizeof(MYFLT));
    if (isPowerOfTwo(N2))
      csound->ComplexFFT(csound,p->out->data,N2/2);
    else {
      csoundComplexFFTnp2(csound,p->out->data,N2/2);
    }
    return OK;
}

int fft_i(CSOUND *csound, FFT *p){
  if(init_fft(csound,p) == OK)
    return perf_fft(csound, p);
  else return NOTOK; 
}


int init_ifft(CSOUND *csound, FFT *p){
  int   N2 = p->in->sizes[0];
   if(p->in->dimensions > 1)
    return csound->InitError(csound,
       "fftinv: only one-dimensional arrays allowed");
  tabensure(csound, p->out, N2);
  return OK;
}

int perf_ifft(CSOUND *csound, FFT *p){
    int N2 = p->out->sizes[0];
    memcpy(p->out->data,p->in->data,N2*sizeof(MYFLT));
    if (isPowerOfTwo(N2)){
      csound->InverseComplexFFT(csound,p->out->data,N2/2);
    }
    else {
      csoundInverseComplexFFTnp2(csound,p->out->data,N2/2);
    }
    return OK;
}

int ifft_i(CSOUND *csound, FFT *p){
  if(init_ifft(csound,p) == OK)
    return perf_ifft(csound, p);
  else return NOTOK; 
}

int init_recttopol(CSOUND *csound, FFT *p){
    int   N = p->in->sizes[0];
    tabensure(csound, p->out, N);
    return OK;
}

int perf_recttopol(CSOUND *csound, FFT *p){
    int i, end = p->out->sizes[0];
    MYFLT *in, *out, mag, ph;
    in = p->in->data;
    out = p->out->data;
    for (i=2;i<end;i+=2 ){
      mag = sqrt(in[i]*in[i] + in[i+1]*in[i+1]);
      ph = atan2(in[i+1],in[i]);
      out[i] = mag; out[i+1] = ph;
    }
    return OK;
}

int perf_poltorect(CSOUND *csound, FFT *p){
    int i, end = p->out->sizes[0];
    MYFLT *in, *out, re, im;
    in = p->in->data;
    out = p->out->data;
    for(i=2;i<end;i+=2){
      re = in[i]*cos(in[i+1]);
      im = in[i]*sin(in[i+1]);
      out[i] = re; out[i+1] = im;
    }
    return OK;
}

int init_poltorect2(CSOUND *csound, FFT *p){
    if(p->in2->sizes[0] == p->in->sizes[0]){
      int   N = p->in2->sizes[0]-1;
      tabensure(csound, p->out, N*2);
      return OK;
    } else return csound->InitError(csound,
                                    "in array sizes do not match: %d and %d\n",
                                    p->in2->sizes[0],p->in->sizes[0]);
}


int perf_poltorect2(CSOUND *csound, FFT *p){
    int i,j, end = p->in->sizes[0]-1;
    MYFLT *mags, *phs, *out, re, im;
    mags = p->in->data;
    phs = p->in2->data;
    out = p->out->data;
    for(i=2,j=1;j<end;i+=2, j++){
      re = mags[j]*cos(phs[j]);
      im = mags[j]*sin(phs[j]);
      out[i] = re; out[i+1] = im;
    }
    out[0] = mags[0];
    out[1] = mags[end];
    return OK;
}

int init_mags(CSOUND *csound, FFT *p){
    int   N = p->in->sizes[0];
    tabensure(csound, p->out, N/2+1);
    return OK;
}

int perf_mags(CSOUND *csound, FFT *p){
    int i,j, end = p->out->sizes[0];
    MYFLT *in, *out;
    in = p->in->data;
    out = p->out->data;
    for(i=2,j=1;j<end-1;i+=2,j++)
      out[j] = sqrt(in[i]*in[i] + in[i+1]*in[i+1]);
    out[0] = in[0];
    out[end-1] = in[1];
    return OK;
}

int perf_phs(CSOUND *csound, FFT *p){
    int i,j, end = p->out->sizes[0];
    MYFLT *in, *out;
    in = p->in->data;
    out = p->out->data;
    for(i=0,j=0;j<end;i+=2,j++)
      out[j] = atan2(in[i+1],in[i]);
    return OK;
}

int init_logarray(CSOUND *csound, FFT *p){
    tabensure(csound, p->out, p->out->sizes[0]);
    if(*((MYFLT *)p->in2))
      p->b = 1/log(*((MYFLT *)p->in2));
    else
      p->b = FL(0.0);
    return OK;
}

int perf_logarray(CSOUND *csound, FFT *p){
    int i, end = p->out->sizes[0];
    MYFLT bas = p->b;
    MYFLT *in, *out;
    in = p->in->data;
    out = p->out->data;
    if(bas)
      for(i=0;i<end;i++)
        out[i] = log(in[i])*bas;
    else
      for(i=0;i<end;i++)
        out[i] = log(in[i]);
    return OK;
}

int init_rtoc(CSOUND *csound, FFT *p){
    int   N = p->in->sizes[0];
    tabensure(csound, p->out, N*2);
    return OK;
}

int perf_rtoc(CSOUND *csound, FFT *p){
    int i,j, end = p->out->sizes[0];
    MYFLT *in, *out;
    in = p->in->data;
    out = p->out->data;
    for(i=0,j=0;i<end;i+=2,j++){
      out[i] = in[j];
      out[i+1] = FL(0.0);
    }
    return OK;
}

int rtoc_i(CSOUND *csound, FFT *p){
  if(init_rtoc(csound,p) == OK)
    return perf_rtoc(csound, p);
  else return NOTOK; 
}

int init_ctor(CSOUND *csound, FFT *p){
    int   N = p->in->sizes[0];
    tabensure(csound, p->out, N/2);
    return OK;
}


int perf_ctor(CSOUND *csound, FFT *p){
    int i,j, end = p->out->sizes[0];
    MYFLT *in, *out;
    in = p->in->data;
    out = p->out->data;
    for(i=0,j=0;j<end;i+=2,j++)
      out[j] = in[i];
    return OK;
}

int ctor_i(CSOUND *csound, FFT *p){
  if(init_ctor(csound,p) == OK)
    return perf_ctor(csound, p);
  else return NOTOK; 
}


int init_window(CSOUND *csound, FFT *p){
    int   N = p->in->sizes[0];
    int   i,type = (int) *p->f;
    MYFLT *w;
    tabensure(csound, p->out, N);
    if(p->mem.auxp == 0 || p->mem.size < N*sizeof(MYFLT))
      csound->AuxAlloc(csound, N*sizeof(MYFLT), &p->mem);
    w = (MYFLT *) p->mem.auxp;
    switch(type){
    case 0:
      for(i=0; i<N; i++) w[i] = 0.54 - 0.46*cos(i*2*PI/N);
      break;
    case 1:
    default:
      for(i=0; i<N; i++) w[i] = 0.5 - 0.5*cos(i*2*PI/N);
    }
    return OK;
}

int perf_window(CSOUND *csound, FFT *p){
    int i,end = p->out->sizes[0], off = *((MYFLT *)p->in2);
    MYFLT *in, *out, *w;
    in = p->in->data;
    out = p->out->data;
    w = (MYFLT *) p->mem.auxp;
    if(off) off = end - off;
    for(i=0;i<end;i++)
      out[i] = in[i]*w[(i+off)%end];
    return OK;
}

#include "pstream.h"

typedef struct _pvsceps {
  OPDS    h;
  ARRAYDAT  *out;
  PVSDAT  *fin;
  MYFLT   *coefs;
  uint32_t  lastframe;
} PVSCEPS;

int pvsceps_init(CSOUND *csound, PVSCEPS *p){
    int N = p->fin->N;
    if(isPowerOfTwo(N))
      tabensure(csound, p->out, N);
    else
      return csound->InitError(csound,
                               Str("non-pow-of-two case not implemented yet\n"));
    p->lastframe = 0;
    return OK;
}

int pvsceps_perf(CSOUND *csound, PVSCEPS *p){

    if (p->lastframe < p->fin->framecount) {
      int N = p->fin->N;
      int i;
      MYFLT *ceps = p->out->data;
      MYFLT coefs = *p->coefs;
      float *fin = (float *) p->fin->frame.auxp;
      for(i=0; i < N; i+=2){
        ceps[i] = log(fin[i] > 0.0 ? fin[i] : 1e-20);
        ceps[i+1] = 0.f;
      }
      csound->InverseComplexFFT(csound, ceps, N/2);
      if(coefs)
        // lifter coefs
        for (i=coefs; i < N-coefs; i++) ceps[i] = 0.0;
      p->lastframe = p->fin->framecount;
    }
    return OK;
}


int init_ceps(CSOUND *csound, FFT *p){
    int N = p->in->sizes[0]-1;
    if(N < 64)
      return csound->InitError(csound,
                               Str("FFT size too small (min 64 samples)\n"));
    if(isPowerOfTwo(N))
      tabensure(csound, p->out, 2*N+2);
    else
      return csound->InitError(csound,
                               Str("non-pow-of-two case not implemented yet\n"));
    
    return OK;
}


int perf_ceps(CSOUND *csound, FFT *p){
    int siz = p->out->sizes[0], i, j;
    MYFLT *ceps = p->out->data;
    MYFLT coefs = *((MYFLT *)p->in2);
    MYFLT *mags = (MYFLT *) p->in->data;
    for(j=i=0; i < siz; i++, j+=2){
      ceps[j] = log(mags[i] > 0.0 ? mags[i] : 1e-20);
      ceps[j+1] = 0.f;
    }
    csound->InverseComplexFFT(csound, ceps, siz/2-1);
    if(coefs)
      // lifter coefs
      for (i=coefs; i < siz-coefs; i++) ceps[i] = 0.0;
    return OK;
}

int init_iceps(CSOUND *csound, FFT *p){
    int N = p->in->sizes[0]-2;
    if(isPowerOfTwo(N))
      tabensure(csound, p->out, N/2+1);
    else
      return csound->InitError(csound,
                               Str("non-pow-of-two case not implemented yet\n"));
    if(p->mem.auxp == NULL || p->mem.size < (N+2)*sizeof(MYFLT))
      csound->AuxAlloc(csound, (N+2)*sizeof(MYFLT), &p->mem);
    return OK;
}

int perf_iceps(CSOUND *csound, FFT *p){
    int siz = p->in->sizes[0], i, j;
    MYFLT *spec = (MYFLT *)p->mem.auxp;
    MYFLT *out = p->out->data;
    memcpy(spec, p->in->data, siz*sizeof(MYFLT));
    csound->ComplexFFT(csound,spec,siz/2-1);
    for(i=j=0; i < siz; i+=2, j++){
      out[j] = exp(spec[i]);
    }
    return OK;
}

int rows_init(CSOUND *csound, FFT *p){
    if(p->in->dimensions == 2){
      int siz = p->in->sizes[1];
      tabensure(csound, p->out, siz);
      return OK;
    }
    else
      return csound->InitError(csound,
                               "in array not 2-dimensional\n");
}

int rows_perf(CSOUND *csound, FFT *p){
    int start = *((MYFLT *)p->in2);
    if(start < p->in->sizes[0]) {
      int bytes =  p->in->sizes[1]*sizeof(MYFLT);
      start *= p->in->sizes[1];
      memcpy(p->out->data,p->in->data+start,bytes);
      return OK;
    }
    else return csound->PerfError(csound,  p->h.insdshead,
                                  "requested row is out of range\n");
}

int cols_init(CSOUND *csound, FFT *p){
    if(p->in->dimensions == 2){
      int siz = p->in->sizes[0];
      tabensure(csound, p->out, siz);
      return OK;
    }
    else
      return csound->InitError(csound,
                               "in array not 2-dimensional\n");
}

int cols_perf(CSOUND *csound, FFT *p){
    int start = *((MYFLT *)p->in2);
    if(start < p->in->sizes[1]) {
      int j,i,len =  p->in->sizes[0];
      for(j=0,i=start; j < len; i+=len, j++)
        p->out->data[j] = p->in->data[i];
      return OK;
    }
    else return csound->PerfError(csound,  p->h.insdshead,
                                  "requested col is out of range\n");
}

static inline void tabensure2D(CSOUND *csound, ARRAYDAT *p, int rows, int columns)
{
    if (p->data==NULL || p->dimensions == 0 ||
        (p->dimensions==2 && (p->sizes[0] < rows || p->sizes[1] < columns))) {
      size_t ss;
      if(p->data == NULL) {
        CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL);
        p->arrayMemberSize = var->memBlockSize;
      }
      ss = p->arrayMemberSize*rows*columns;
      if (p->data==NULL) {
        p->data = (MYFLT*)csound->Calloc(csound, ss);
        p->dimensions = 2;
        p->sizes = (int*)csound->Malloc(csound, sizeof(int)*2);
      }
      else p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
      p->sizes[0] = rows;  p->sizes[1] = columns;
    }
}

int set_rows_init(CSOUND *csound, FFT *p){
    int sizs = p->in->sizes[0];
    int row = *((MYFLT *)p->in2);
    tabensure2D(csound, p->out, row+1, sizs);
    return OK;
}

int set_rows_perf(CSOUND *csound, FFT *p){
    int start = *((MYFLT *)p->in2);
    int bytes =  p->in->sizes[0]*sizeof(MYFLT);
    start *= p->out->sizes[1];
    memcpy(p->out->data+start,p->in->data,bytes);
    return OK;
}

int set_cols_init(CSOUND *csound, FFT *p){
    int siz = p->in->sizes[0];
    int col = *((MYFLT *)p->in2);
    tabensure2D(csound, p->out, siz, col+1);
    return OK;
}

int set_cols_perf(CSOUND *csound, FFT *p){
    int start = *((MYFLT *)p->in2);
    int j,i,len =  p->out->sizes[0];
    for(j=0,i=start; j < len; i+=len, j++)
      p->out->data[i] = p->in->data[j];
    return OK;
}

int shiftin_init(CSOUND *csound, FFT *p){
    int sizs = CS_KSMPS;
    tabensure(csound, p->out, sizs);
    p->n = 0;
    return OK;
}

int shiftin_perf(CSOUND *csound, FFT *p){
    uint32_t  siz =  p->out->sizes[0], n = p->n;
    MYFLT *in = ((MYFLT *) p->in);
    if(n + CS_KSMPS < siz) {
      memcpy(p->out->data+n,in,CS_KSMPS*sizeof(MYFLT));
    }
    else {
      int num = siz - n;
      memcpy(p->out->data+n,in,num*sizeof(MYFLT));
      memcpy(p->out->data,in+num,(CS_KSMPS-num)*sizeof(MYFLT));
    }
    p->n = (n + CS_KSMPS)%siz;
    return OK;
}


int shiftout_init(CSOUND *csound, FFT *p){
    int siz = p->in->sizes[0];
    p->n = ((int)*((MYFLT *)p->in2) % siz);
    if((uint32_t) siz < CS_KSMPS)
      return csound->InitError(csound, "input array too small\n");
    return OK;
}

int shiftout_perf(CSOUND *csound, FFT *p){
    uint32_t siz =  p->in->sizes[0], n = p->n;
    MYFLT *out = ((MYFLT *) p->out);
    if(n + CS_KSMPS < siz) {
      memcpy(out,p->in->data+n,CS_KSMPS*sizeof(MYFLT));
    }
    else {
      int num = siz - n;
      memcpy(out,p->in->data+n,num*sizeof(MYFLT));
      memcpy(out+num,p->in->data,(CS_KSMPS-num)*sizeof(MYFLT));
    }
    p->n = (n + CS_KSMPS)%siz;
    return OK;
}

int scalarset(CSOUND *csound, FFT *p){
    uint32_t siz = 0 , dim = p->out->dimensions, i;
    MYFLT val = *((MYFLT *)p->in);
    for(i=0; i < dim; i++)
      siz += p->out->sizes[i];
    for(i=0; i < siz; i++)
      p->out->data[i] = val;
    return OK;
}

int unwrap(CSOUND *csound, FFT *p){
    int i,siz = p->in->sizes[0];
    MYFLT *phs = p->out->data;
    for(i=0; i < siz; i++){
      while(phs[i] >= PI) phs[i] -= 2*PI;
      while(phs[i] < -PI) phs[i] += 2*PI;
    }
    return OK;
}


// reverse, scramble, mirror, stutter, rotate, ...
// jpff: stutter is an interesting one (very musical). It basically
//          randomly repeats (holds) values based on a probability parameter

static OENTRY arrayvars_localops[] =
  {
    { "init.0", sizeof(ARRAYINIT), 0, 1, ".[]", "m", (SUBR)array_init },
    { "fillarray", 0xffff },
    { "fillarray.k", sizeof(TABFILL), 0, 1, "k[]", "m", (SUBR)tabfill },
    { "fillarray.i", sizeof(TABFILL), 0, 1, "i[]", "m", (SUBR)tabfill },
    { "fillarray.s", sizeof(TABFILL), 0, 1, "S[]", "W", (SUBR)tabfill },
    { "array", 0xffff },
    { "array.k", sizeof(TABFILL), _QQ, 1, "k[]", "m", (SUBR)tabfill     },
    { "array.i", sizeof(TABFILL), _QQ, 1, "i[]", "m", (SUBR)tabfill     },
    { "##array_set.i", sizeof(ARRAY_SET), 0, 1, "", "i[]im", (SUBR)array_set },
    { "##array_init", sizeof(ARRAY_SET), 0, 1, "", ".[]im", (SUBR)array_set },
    { "##array_set.k0", sizeof(ARRAY_SET), 0, 2, "", "k[]kz",
      NULL, (SUBR)array_set },
    { "##array_set.i2", sizeof(ARRAY_SET), 0, 3, "", ".[].m",
      (SUBR)array_set, (SUBR)array_set },
    { "##array_set.k", sizeof(ARRAY_SET), 0, 2, "", ".[].z",
      NULL, (SUBR)array_set },
    { "##array_get.i", sizeof(ARRAY_GET), 0, 1, "i", "i[]m", (SUBR)array_get },
    { "##array_get.k0", sizeof(ARRAY_GET), 0, 3, "k", "k[]z",
      (SUBR)array_get, (SUBR)array_get },
    { "##array_get.i2", sizeof(ARRAY_GET), 0, 3, ".", ".[]m",
      (SUBR)array_get, (SUBR)array_get },
    { "##array_get.k", sizeof(ARRAY_GET), 0, 3, ".", ".[]z",
      (SUBR)array_get, (SUBR)array_get },
    /* ******************************************** */
    {"##add.[]", sizeof(TABARITH), 0, 3, "k[]", "k[]k[]",
     (SUBR)tabarithset, (SUBR)tabadd},
    {"##add.[i]", sizeof(TABARITH), 0, 1, "i[]", "i[]i[]",
     (SUBR)tabaddi},
    /* ******************************************** */
    {"##sub.[]", sizeof(TABARITH), 0, 3, "k[]", "k[]k[]",
     (SUBR)tabarithset, (SUBR)tabsub},
    {"##sub.[i]", sizeof(TABARITH), 0, 1, "i[]", "i[]i[]",
     (SUBR)tabsubi},
    //    {"##neg.[]",  sizeof(TABARITH), 0, 3, "k[]", "k[]",
    //                                         (SUBR)tabarithset1, (SUBR)tabneg},
    {"##mul.[]", sizeof(TABARITH), 0, 3, "k[]", "k[]k[]",
     (SUBR)tabarithset,(SUBR)tabmult},
    {"##mul.[i]", sizeof(TABARITH), 0, 1, "i[]", "i[]i[]",
     (SUBR)tabmulti},
    {"##div.[]",  sizeof(TABARITH), 0, 3, "k[]", "k[]k[]",
     (SUBR)tabarithset,(SUBR)tabdiv },
    {"##div.[i]",  sizeof(TABARITH), 0, 1, "i[]", "i[]i[]",
     (SUBR)tabdivi },
    {"##rem.[]",  sizeof(TABARITH), 0, 3, "k[]", "k[]k[]",
     (SUBR)tabarithset, (SUBR)tabrem},
    {"##rem.[i]",  sizeof(TABARITH), 0, 1, "i[]", "i[]i[]", (SUBR)tabremi},
    {"##add.[i", sizeof(TABARITH1), 0, 3, "k[]", "k[]i",
     (SUBR)tabarithset1, (SUBR)tabaiadd },
    {"##add.i[", sizeof(TABARITH2), 0, 3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiaadd },
    {"##add.[p", sizeof(TABARITH1), 0, 1, "i[]", "i[]i", (SUBR)tabaiaddi },
    {"##add.p[", sizeof(TABARITH2), 0, 1, "i[]", "ii[]", (SUBR)tabiaaddi },
    {"##sub.[i", sizeof(TABARITH1), 0, 3, "k[]", "k[]i",
     (SUBR)tabarithset1, (SUBR)tabaisub },
    {"##sub.i[", sizeof(TABARITH2), 0, 3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiasub },
    {"##sub.[p", sizeof(TABARITH1), 0, 1, "i[]", "i[]i", (SUBR)tabaisubi },
    {"##sub.p[", sizeof(TABARITH2), 0, 1, "i[]", "ii[]", (SUBR)tabiasubi },
    {"##mul.[i", sizeof(TABARITH1), 0, 3, "k[]", "k[]i",
     (SUBR)tabarithset1, (SUBR)tabaimult },
    {"##mul.i[", sizeof(TABARITH2), 0, 3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiamult },
    {"##mul.[p", sizeof(TABARITH1), 0, 1, "i[]", "i[]i", (SUBR)tabaimulti },
    {"##mul.p[", sizeof(TABARITH2), 0, 1, "i[]", "ii[]",  (SUBR)tabiamulti },
    {"##div.[i",  sizeof(TABARITH1), 0, 3, "k[]", "k[]i",
     (SUBR)tabarithset1, (SUBR)tabaidiv },
    {"##div.i[",  sizeof(TABARITH2), 0, 3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiadiv },
    {"##div.[p",  sizeof(TABARITH1), 0, 1, "i[]", "i[]i", (SUBR)tabaidivi },
    {"##div.p[",  sizeof(TABARITH2), 0, 1, "i[]", "ii[]", (SUBR)tabiadivi },
    {"##rem.[i",  sizeof(TABARITH1),0,  3, "k[]", "k[]i",
     (SUBR)tabarithset1, (SUBR)tabairem },
    {"##rem.i[",  sizeof(TABARITH2),0,  3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiarem },
    {"##rem.[p",  sizeof(TABARITH1),0,  1, "i[]", "i[]i", (SUBR)tabairemi },
    {"##rem.p[",  sizeof(TABARITH2),0,  1, "i[]", "ii[]", (SUBR)tabiaremi },
    {"##add.[k", sizeof(TABARITH1), 0, 3, "k[]", "k[]k",
     (SUBR)tabarithset1, (SUBR)tabaiadd },
    {"##add.k[", sizeof(TABARITH2), 0, 3, "k[]", "kk[]",
     (SUBR)tabarithset2, (SUBR)tabiaadd },
    {"##sub.[k", sizeof(TABARITH1), 0, 3, "k[]", "k[]k",
     (SUBR)tabarithset1, (SUBR)tabiasub },
    {"##sub.k[", sizeof(TABARITH2), 0, 3, "k[]", "kk[]",
     (SUBR)tabarithset2, (SUBR)tabaisub },
    {"##mul.[k", sizeof(TABARITH1), 0, 3, "k[]", "k[]k",
     (SUBR)tabarithset1, (SUBR)tabaimult },
    {"##mul.k[", sizeof(TABARITH2), 0, 3, "k[]", "kk[]",
     (SUBR)tabarithset2, (SUBR)tabiamult },
    {"##div.[k",  sizeof(TABARITH1), 0, 3, "k[]", "k[]k",
     (SUBR)tabarithset1, (SUBR)tabaidiv },
    {"##div.k[",  sizeof(TABARITH2), 0, 3, "k[]", "kk[]",
     (SUBR)tabarithset2, (SUBR)tabiadiv },
    {"##rem.[k",  sizeof(TABARITH1),0,  3, "k[]", "k[]k",
     (SUBR)tabarithset1, (SUBR)tabairem },
    {"##rem.k[",  sizeof(TABARITH2),0,  3, "k[]", "kk[]",
     (SUBR)tabarithset2, (SUBR)tabiarem },
    {"##pow.[]",  sizeof(TABARITH), 0, 3, "k[]", "k[]k[]",
     (SUBR)tabarithset,(SUBR)tabpow },
    {"##pow.[i]",  sizeof(TABARITH), 0, 1, "i[]", "i[]i[]",
     (SUBR)tabpowi },
    {"##pow.[i",  sizeof(TABARITH1), 0, 3, "k[]", "k[]i",
     (SUBR)tabarithset1, (SUBR)tabaipow },
    {"##pow.i[",  sizeof(TABARITH2), 0, 3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiapow },
    {"##pow.[p",  sizeof(TABARITH1), 0, 1, "i[]", "i[]i", (SUBR)tabaipowi },
    {"##pow.p[",  sizeof(TABARITH2), 0, 1, "i[]", "ii[]", (SUBR)tabiapowi },
    {"##pow.[k",  sizeof(TABARITH1), 0, 3, "k[]", "k[]k",
     (SUBR)tabarithset1, (SUBR)tabaipow },
    {"##pow.k[",  sizeof(TABARITH2), 0, 3, "k[]", "kk[]",
     (SUBR)tabarithset2, (SUBR)tabiapow },
    { "maxtab", 0xffff},
    { "maxtab.k",sizeof(TABQUERY),_QQ, 3, "kz", "k[]",
      (SUBR) tabqset, (SUBR) tabmax },
    { "maxarray", 0xffff},
    { "maxarray.k", sizeof(TABQUERY), 0, 3, "kz", "k[]",
      (SUBR) tabqset,(SUBR) tabmax },
    { "maxarray.i", sizeof(TABQUERY), 0, 1, "iI", "i[]",(SUBR) tabmax1, NULL  },
    { "mintab", 0xffff},
    { "minarray", 0xffff},
    { "mintab.k", sizeof(TABQUERY),_QQ, 3, "kz", "k[]",
      (SUBR) tabqset, (SUBR) tabmin },
    { "minarray.k", sizeof(TABQUERY),0, 3, "kz", "k[]",(SUBR) tabqset,
      (SUBR) tabmin },
    { "minarray.i", sizeof(TABQUERY),0, 1, "iI", "i[]",(SUBR) tabmin1 },
    { "sumarray", 0xffff},
    { "sumtab", sizeof(TABQUERY1),_QQ, 3, "k", "k[]",
      (SUBR) tabqset1, (SUBR) tabsum },
    { "sumarray.k", sizeof(TABQUERY1),0, 3, "k", "k[]",
      (SUBR) tabqset1, (SUBR) tabsum },
    { "sumarray.i", sizeof(TABQUERY1),0, 1, "i", "i[]", (SUBR) tabsum1   },
    { "scalet", sizeof(TABSCALE), _QQ, 3, "",  "k[]kkOJ",
      (SUBR) tabscaleset,(SUBR) tabscale },
    { "scalearray", 0xffff},
    { "scalearray.k", sizeof(TABSCALE), 0, 3, "",  "k[]kkOJ",
      (SUBR) tabscaleset,(SUBR) tabscale },
    { "scalearray.1", sizeof(TABSCALE), 0, 1, "",  "i[]iiOJ",   (SUBR) tabscale1 },
    { "=.I", sizeof(TABCPY), 0, 1, "i[]", "i[]", (SUBR)tabcopy, NULL },
    { "=._", sizeof(TABCPY), 0, 3, ".[]", ".[]", (SUBR)tabcopy, (SUBR)tabcopy },
    { "tabgen", sizeof(TABGEN), _QQ, 1, "k[]", "iip", (SUBR) tabgen, NULL    },
    { "tabmap_i", sizeof(TABMAP), _QQ, 1, "k[]", "k[]S", (SUBR) tabmap_set   },
    { "tabmap", sizeof(TABMAP), _QQ, 3, "k[]", "k[]S", (SUBR) tabmap_set,
      (SUBR) tabmap_perf},
    { "tabmap", sizeof(TABMAP), _QQ, 3, "k[]", "k[]S", (SUBR) tabmap_set,
      (SUBR) tabmap_perf},
    { "genarray.i", sizeof(TABGEN),0, 1, "i[]", "iip", (SUBR) tabgen, NULL   },
    { "genarray_i", sizeof(TABGEN),0, 1, "k[]", "iip", (SUBR) tabgen, NULL, NULL},
    { "genarray.k", sizeof(TABGEN),0, 2, "k[]", "kkp", NULL, (SUBR)tabgen    },
    { "maparray_i", sizeof(TABMAP),0, 1, "k[]", "k[]S", (SUBR) tabmap_set    },
    { "maparray.k", sizeof(TABMAP), 0, 3, "k[]", "k[]S", (SUBR) tabmap_set,
      (SUBR) tabmap_perf          },
    { "maparray.i", sizeof(TABMAP), 0, 1, "i[]", "i[]S", (SUBR) tabmap_set },
    /*  { "maparray.s", sizeof(TABMAP), 0, 3, "S[]", "S[]S", (SUBR) tabmap_set, */
    /*                                               (SUBR) tabmap_perf     }, */
    { "tabslice", sizeof(TABSLICE), _QQ, 2, "k[]", "k[]ii",
      NULL, (SUBR) tabslice, NULL },

    { "slicearray.i", sizeof(TABSLICE), 0, 1, "i[]", "i[]ii",
      (SUBR) tabslice, NULL, NULL },
    { "slicearray.x", sizeof(TABSLICE), 0, 3, ".[]", ".[]ii",
      (SUBR) tabslice, (SUBR) tabslice, NULL },
    //    { "slicearray.s", sizeof(TABSLICE), 0, 3, "S[]", "[]ii",
    //                                  (SUBR) tabsliceS, (SUBR) tabsliceS, NULL },
    { "copy2ftab", sizeof(TABCOPY), TW|_QQ, 2, "", "k[]k", NULL, (SUBR) tab2ftab },
    { "copy2ttab", sizeof(TABCOPY), TR|_QQ, 2, "", "k[]k", NULL, (SUBR) ftab2tab },
    { "copya2ftab.k", sizeof(TABCOPY), TW, 3, "", "k[]k",
      (SUBR) tab2ftab, (SUBR) tab2ftab },
    { "copyf2array.k", sizeof(TABCOPY), TR, 3, "", "k[]k",
      (SUBR) ftab2tab, (SUBR) ftab2tab },
    { "copya2ftab.i", sizeof(TABCOPY), TW, 1, "", "i[]i", (SUBR) tab2ftab },
    { "copyf2array.i", sizeof(TABCOPY), TR, 1, "", "i[]i", (SUBR) ftab2tab },
    /* { "lentab", 0xffff}, */
    { "lentab.i", sizeof(TABQUERY1), _QQ, 1, "i", "k[]p", (SUBR) tablength },
    { "lentab.k", sizeof(TABQUERY1), _QQ, 1, "k", "k[]p", NULL, (SUBR) tablength },
    { "lenarray.ix", sizeof(TABQUERY1), 0, 1, "i", ".[]p", (SUBR) tablength },
    { "lenarray.kx", sizeof(TABQUERY1), 0, 2, "k", ".[]p", NULL, (SUBR)tablength },
    { "out.A", sizeof(OUTA), 0, 5,"", "a[]", (SUBR)outa_set, NULL, (SUBR)outa},
    { "in.A", sizeof(OUTA), 0, 5, "a[]", "", (SUBR)ina_set, NULL, (SUBR)ina},
    {"rfft", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_rfft, (SUBR) perf_rfft, NULL},
    {"rfft", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) rfft_i, NULL, NULL},
    {"rifft", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_rifft, (SUBR) perf_rifft, NULL},
    {"rifft", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) rifft_i, NULL, NULL},
    {"cmplxprod", sizeof(FFT), 0, 3, "k[]","k[]k[]",
     (SUBR) init_rfftmult, (SUBR) perf_rfftmult, NULL},
    {"fft", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_fft, (SUBR) perf_fft, NULL},
    {"fft", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) fft_i, NULL, NULL},
    {"fftinv", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_ifft, (SUBR) perf_ifft, NULL},
    {"fftinv", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) ifft_i, NULL, NULL},
    {"rect2pol", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_recttopol, (SUBR) perf_recttopol, NULL},
    {"pol2rect", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_recttopol, (SUBR) perf_poltorect, NULL},
    {"pol2rect", sizeof(FFT), 0, 3, "k[]","k[]k[]",
     (SUBR) init_poltorect2, (SUBR) perf_poltorect2, NULL},
    {"mags", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_mags, (SUBR) perf_mags, NULL},
    {"phs", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_mags, (SUBR) perf_phs, NULL},
    {"log", sizeof(FFT), 0, 3, "k[]","k[]o",
     (SUBR) init_logarray, (SUBR) perf_logarray, NULL},
    {"r2c", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) rtoc_i, NULL, NULL},
    {"r2c", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_rtoc, (SUBR) perf_rtoc, NULL},
    {"c2r", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) ctor_i, NULL, NULL},
    {"c2r", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_ctor, (SUBR) perf_ctor, NULL},
    {"window", sizeof(FFT), 0, 3, "k[]","k[]Op",
     (SUBR) init_window, (SUBR) perf_window, NULL},
    {"pvsceps", sizeof(PVSCEPS), 0, 3, "k[]","fo",
     (SUBR) pvsceps_init, (SUBR) pvsceps_perf, NULL},
    {"iceps", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_iceps, (SUBR) perf_iceps, NULL},
    {"ceps", sizeof(FFT), 0, 3, "k[]","k[]k",
     (SUBR) init_ceps, (SUBR) perf_ceps, NULL},
    {"getrow", sizeof(FFT), 0, 3, "k[]","k[]k",
     (SUBR) rows_init, (SUBR) rows_perf, NULL},
    {"getcol", sizeof(FFT), 0, 3, "k[]","k[]k",
     (SUBR) cols_init, (SUBR) cols_perf, NULL},
    {"setrow", sizeof(FFT), 0, 3, "k[]","k[]k",
     (SUBR) set_rows_init, (SUBR) set_rows_perf, NULL},
    {"setcol", sizeof(FFT), 0, 3, "k[]","k[]k",
     (SUBR) set_cols_init, (SUBR) set_cols_perf, NULL},
    {"shiftin", sizeof(FFT), 0, 5, "k[]","a",
     (SUBR) shiftin_init, NULL, (SUBR) shiftin_perf},
    {"shiftout", sizeof(FFT), 0, 5, "a","k[]o",
     (SUBR) shiftout_init, NULL, (SUBR) shiftout_perf},
    {"unwrap", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_recttopol, (SUBR) unwrap},
    {"=.k", sizeof(FFT), 0, 3, "k[]","k", (SUBR) scalarset, (SUBR) scalarset}
  };

LINKAGE_BUILTIN(arrayvars_localops)
