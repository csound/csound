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
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
  02110-1301 USA
*/

// #include "csdl.h"
#include "csoundCore_internal.h"
#include "interlocks.h"
#include "aops.h"
#include "find_opcode.h"

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
  ARRAYDAT* ans;
  STRINGDAT *fname;
} TABFILLF;

typedef struct {
  OPDS    h;
  ARRAYDAT* arrayDat;
  void    *value;
  MYFLT   *indexes[VARGMAX];
} ARRAY_SET;

typedef struct {
  OPDS    h;
  MYFLT*   out;
  ARRAYDAT* arrayDat;
  MYFLT   *indexes[VARGMAX];
} ARRAY_GET;

#ifdef SOME_FINE_DAY
static int32_t array_del(CSOUND *csound, void *p)
{
    ARRAYDAT *t = ((ARRAYDEL*)p)->arrayDat;
    t->arrayType = NULL; // types cleaned up later
    csound->Free(csound, t->data);
    csound->Free(csound, p);           /* Unlikely to free the p */
    return OK;
}
#endif

#include "arrays.h"

static int32_t array_init(CSOUND *csound, ARRAYINIT *p)
{
    ARRAYDAT* arrayDat = p->arrayDat;
    int32_t i, size;

    int32_t inArgCount = p->INOCOUNT;

    if (UNLIKELY(inArgCount == 0))
      return
        csound->InitError(csound, "%s",
                          Str("Error: no sizes set for array initialization"));

    for (i = 0; i < inArgCount; i++) {
      if (UNLIKELY(MYFLT2LRND(*p->isizes[i]) <= 0)) {
        return
          csound->InitError(csound, "%s",
                      Str("Error: sizes must be > 0 for array initialization"));
      }
    }

    arrayDat->dimensions = inArgCount;
    arrayDat->sizes = csound->Calloc(csound, sizeof(int32_t) * inArgCount);
    for (i = 0; i < inArgCount; i++) {
      arrayDat->sizes[i] = MYFLT2LRND(*p->isizes[i]);
    }

    size = arrayDat->sizes[0];
    if (inArgCount > 1) {
      for (i = 1; i < inArgCount; i++) {
        size *= arrayDat->sizes[i];
      }
      //size = MYFLT2LRND(size); // size is an int32_t not float
    }

    {
      CS_VARIABLE* var = arrayDat->arrayType->createVariable(csound,arrayDat->arrayType);
      char *mem;
      arrayDat->arrayMemberSize = var->memBlockSize;
      arrayDat->data = csound->Calloc(csound,
                                      arrayDat->allocated=var->memBlockSize*size);
      mem = (char *) arrayDat->data;
      for (i=0; i < size; i++) {
        var->initializeVariableMemory(csound, var,
                                      (MYFLT*)(mem+i*var->memBlockSize));
      }
    }
    return OK;
}

static int32_t tabfill(CSOUND *csound, TABFILL *p)
{
    int32_t    nargs = p->INOCOUNT;
    int32_t i, size;
    size_t memMyfltSize;
    MYFLT  **valp = p->iargs;
    tabinit(csound, p->ans, nargs);
/*     if (UNLIKELY(p->ans->dimensions > 2)) { */
/*       return */
/*         csound->InitError(csound, "%s", */
/*                           Str("fillarrray: arrays with dim > 2 not " */
/*                               "currently supported\n")); */
/*     } */
    size = p->ans->sizes[0];
    for (i=1; i<p->ans->dimensions; i++) size *= p->ans->sizes[i];
    if (size<nargs) nargs = size;
    memMyfltSize = p->ans->arrayMemberSize / sizeof(MYFLT);
    for (i=0; i<nargs; i++) {
      p->ans->arrayType->copyValue(csound,
                                   p->ans->arrayType,
                                   p->ans->data + (i * memMyfltSize),
                                   valp[i]);
    }
    return OK;
}

#include <ctype.h>

static MYFLT nextval(FILE *f)
{
    /* Read the next character; suppress multiple space and comments to a
       single space */
    int c;
 top:
    c = getc(f);
 top1:
    if (UNLIKELY(feof(f))) return NAN; /* Hope value is ignored */
    if (isdigit(c) || c=='e' || c=='E' || c=='+' || c=='-' || c=='.') {
      double d;                           /* A number starts */
      char buff[128];
      int j = 0;
      do {                                /* Fill buffer */
        buff[j++] = c;
        c = getc(f);
      } while (isdigit(c) || c=='e' || c=='E' || c=='+' || c=='-' || c=='.');
      buff[j]='\0';
      d = atof(buff);
      if (c==';' || c=='#') {             /* If extended with comment clear it now */
        while ((c = getc(f)) != '\n');
      }
      return (MYFLT)d;
    }
    while (isspace(c) || c == ',') c = getc(f);       /* Whitespace */
    if (c==';' || c=='#' || c=='<') {     /* Comment and tag*/
      while ((c = getc(f)) != '\n');
    }
    if (isdigit(c) || c=='e' || c=='E' || c=='+' || c=='-' || c=='.') goto top1;
    goto top;
}

static int32_t tabfillf(CSOUND* csound, TABFILLF* p)
{
    int32_t i = 0, flen = 0, size;
    char *fname = p->fname->data;
    FILE    *infile;
    void    *fd = csound->FileOpen2(csound, &infile, CSFILE_STD, fname, "r",
                           "SFDIR;SSDIR;INCDIR", CSFTYPE_FLOATS_TEXT, 0);
    if (UNLIKELY(fd == NULL)) {
      return csound->InitError(csound, Str("error opening ASCII file %s\n"), fname);
    }
    do {
      flen++;
      nextval(infile);
    } while (!feof(infile));
    flen--; // overshoots by 1
    tabinit(csound, p->ans, flen);
    size = p->ans->sizes[0];
    for (i=1; i<p->ans->dimensions; i++) size *= p->ans->sizes[i];
    if (size<flen) flen = size;
    if (p->ans->dimensions == 1 && (flen > p->ans->sizes[0]))
      flen = p->ans->sizes[0];
    else if (p->ans->dimensions == 2 && (flen > p->ans->sizes[0]*p->ans->sizes[1]))
      flen = p->ans->sizes[0]*p->ans->sizes[1];
    //memMyfltSize = p->ans->arrayMemberSize / sizeof(MYFLT);
    rewind(infile);
    i = 0;
    while (!feof(infile) && i < flen)
      ((MYFLT*)p->ans->data)[i++] = nextval(infile);
    return OK;
}

static MYFLT nextsval(char **ff)
{
    /* Read the next character; suppress multiple space
    * should use stdtod */
    int c;
    char *f = *ff;
 top:
    c = *f++;
 top1:
    if (UNLIKELY(c=='\0')) { *ff = f; return NAN; }
    if (isdigit(c) || c=='e' || c=='E' || c=='+' || c=='-' || c=='.') {
      double d;                           /* A number starts */
      char buff[128];
      int j = 0;
      do {                                /* Fill buffer */
        buff[j++] = c;
        c = *f++;
      } while (isdigit(c) || c=='e' || c=='E' || c=='+' || c=='-' || c=='.');
      buff[j]='\0';
      d = atof(buff);
      while (isspace(c) || c == ',') c = *f++;       /* Whitespace */
      *ff = --f;
      //printf(">> %f\n", d);
      return (MYFLT)d;
    }
    while (isspace(c) || c == ',') c = *f++;       /* Whitespace */
    if (isdigit(c) || c=='e' || c=='E' || c=='+' || c=='-' || c=='.') goto top1;
    goto top;
}

static int32_t tabsfill(CSOUND *csound, TABFILLF *p)
{
    int32_t i = 0, flen = 0, size;
    char *string = p->fname->data;
    MYFLT x;
    do {
      x = nextsval(&string);
      if (x==NAN) break;
      flen++;
    } while (*string!='\0');
    //flen--; // overshoots by 1
    tabinit(csound, p->ans, flen);
    size = p->ans->sizes[0];
    for (i=1; i<p->ans->dimensions; i++) size *= p->ans->sizes[i];
    if (size<flen) flen = size;
    if (p->ans->dimensions == 1 && (flen > p->ans->sizes[0]))
      flen = p->ans->sizes[0];
    else if (p->ans->dimensions == 2 && (flen > p->ans->sizes[0]*p->ans->sizes[1]))
      flen = p->ans->sizes[0]*p->ans->sizes[1];
    //memMyfltSize = p->ans->arrayMemberSize / sizeof(MYFLT);
    string = p->fname->data;
    i = 0;
    while ((*string!='\0') && i < flen)
      ((MYFLT*)p->ans->data)[i++] = nextsval(&string);
    return OK;
}

static int32_t array_err(CSOUND* csound, ARRAY_SET *p)
{
    IGN(p);
    return csound->InitError(csound, "%s", Str("Cannot set i-array at k-rate\n"));
}

static int32_t array_set(CSOUND* csound, ARRAY_SET *p)
{
    ARRAYDAT* dat = p->arrayDat;
    MYFLT* mem = dat->data;
    int32_t i;
    int32_t end, index, incr;

    int32_t indefArgCount = p->INOCOUNT - 2;

    //printf("*** value=%f, index = [%d][%d][%d]\n", *(double*)p->value,
    //       (int)(*p->indexes[0]), (int)(*p->indexes[1]), (int)(*p->indexes[2]));

    if (UNLIKELY(indefArgCount == 0)) {
      csoundErrorMsg(csound, Str("Error: no indexes set for array set\n"));
      return CSOUND_ERROR;
    }
    if (UNLIKELY(indefArgCount!=dat->dimensions)) {
      return csound->PerfError(csound, &(p->h),
                               Str("Array dimension %d does not match "
                                   "for dimensions %d\n"),
                               indefArgCount, dat->dimensions);
    }
    index = 0;
    for (i=0;i<indefArgCount; i++) {
      end = (int)(*p->indexes[i]);
      //printf("** dimemnsion %d end = %d\n", i, end);
      if (UNLIKELY(end>=dat->sizes[i]))
        return csound->PerfError(csound, &(p->h),
                      Str("Array index %d out of range (0,%d) "
                          "for dimension %d"),
                               end, dat->sizes[i], i+1);
      // printf("*** index %d -> ", index);
      index = (index * dat->sizes[i]) + end;
      // printf("%d : %d\n", index, dat->sizes[i]);
    }
    incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
    mem += incr;
    //memcpy(mem, p->value, dat->arrayMemberSize);
    dat->arrayType->copyValue(csound, dat->arrayType, mem, p->value);
    /* printf("array_set: mem = %p, incr = %d, value = %f\n", */
    /*         mem, incr, *((MYFLT*)p->value)); */
    return OK;
}

static int32_t array_get(CSOUND* csound, ARRAY_GET *p)
{
    ARRAYDAT* dat = p->arrayDat;
    MYFLT* mem = dat->data;
    int32_t i;
    int32_t incr;
    int32_t end;
    int32_t index;
    int32_t indefArgCount = p->INOCOUNT - 1;

    if (UNLIKELY(indefArgCount == 0))
      return csound->PerfError(csound, &(p->h),
                               Str("Error: no indexes set for array get"));
    if (UNLIKELY(indefArgCount!=dat->dimensions)) {
      return csound->PerfError(csound, &(p->h),
                               Str("Array dimension %d out of range "
                                   "for dimensions %d"),
                               indefArgCount, dat->dimensions);
    }
    index = 0;
    for (i=0;i<indefArgCount; i++) {
      end = (int)(*p->indexes[i]);
      //printf("++++ ** dimemnsion %d end = %d\n", i, end);
      if (UNLIKELY(end>=dat->sizes[i]))
        return csound->PerfError(csound, &(p->h),
                      Str("Array index %d out of range (0,%d) "
                          "for dimension %d"),
                               end, dat->sizes[i]-1, i+1);
      //printf("*** index %d -> ", index);
      index = (index * dat->sizes[i]) + end;
      //printf("%d : %d\n", index, dat->sizes[i]);
    }

    incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
    mem += incr;
    //    memcpy(p->out, &mem[incr], dat->arrayMemberSize);
    dat->arrayType->copyValue(csound, dat->arrayType, (void*)p->out, (void*)mem);
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
  ARRAYDAT *ans;
  ARRAYDAT *right;
} TABARITHIN;

typedef struct {
  OPDS h;
  ARRAYDAT *ans;
  MYFLT *right;
} TABARITHIN1;

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
  MYFLT  *kfn;
  MYFLT  *offset;
} TABCOPY2;


typedef struct {
  OPDS h;
  ARRAYDAT *tab;
  MYFLT  *kmin, *kmax;
  MYFLT  *kstart, *kend;
} TABSCALE;

typedef struct {
  OPDS h;
  ARRAYDAT *tab;
} TABCLEAR;

static int32_t tabarithset(CSOUND *csound, TABARITH *p)
{
    if (LIKELY(p->left->data && p->right->data)) {
      int i;
      if (UNLIKELY(p->left->dimensions != p->right->dimensions))
        return
          csound->InitError(csound, "%s",
                            Str("Dimensions do not match in array arithmetic"));
      for (i=0; i<p->left->dimensions; i++) {
        if (UNLIKELY(p->left->sizes[i] != p->right->sizes[i]))
          return
            csound->InitError(csound, "%s",
                              Str("Dimensions do not match in array arithmetic"));
      }
      tabinit_like(csound, p->ans, p->left);
      return OK;
    }
    else return csound->InitError(csound, "%s",
                                  Str("array-variable not initialised"));
}

static int32_t tabiadd(CSOUND *csound, ARRAYDAT *ans,
                       ARRAYDAT *l, MYFLT r, void *p);

// For cases with array as first arg
static int32_t tabarithset1(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *left = p->left;

    if (p->ans->data == left->data) {
      // printf("same ptr\n");
      return OK;
    }

    tabinit_like(csound, p->ans, left);
    return OK;
}

// For cases with array as second arg
static int32_t tabarithset2(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *right = p->right;

    if (p->ans->data == right->data) {
      // printf("same ptr\n");
      return OK;
    }

    tabinit_like(csound, p->ans, right);
    return OK;
}

static int32_t tabadd(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    int32_t i;

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel= sizer;
    for (i=0; i<sizel; i++)
      ans->data[i] = l->data[i] + r->data[i];
    return OK;
}

static int32_t tabsub(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    int32_t i;

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel= sizer;
    for (i=0; i<sizel; i++)
      ans->data[i] = l->data[i] - r->data[i];
    return OK;
}

static int32_t tabmult(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    int32_t i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel= sizer;
    for (i=0; i<sizel; i++)
      ans->data[i] = l->data[i] * r->data[i];
    return OK;
}

static int32_t tabdiv(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel = l->sizes[0];
    int32_t sizer = r->sizes[0];
    int32_t i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++)
      if (LIKELY(r->data[i]!=0))
        ans->data[i] = l->data[i] / r->data[i];
      else
        return
          csound->PerfError(csound, &(p->h),
                            Str("division by zero in array-var at index %d"), i);
    return OK;
}

static int32_t tabrem(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel = l->sizes[0];
    int32_t sizer = r->sizes[0];
    int32_t i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

   for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++)
      if (LIKELY(r->data[i]!=0))
        ans->data[i] = MOD(l->data[i], r->data[i]);
      else
        return
          csound->PerfError(csound, &(p->h),
                            Str("division by zero in array-var at index %d"), i);

    return OK;
}

static int32_t tabpow(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel = l->sizes[0];
    int32_t sizer = r->sizes[0];
    int32_t   i;
    MYFLT tmp;

    if (UNLIKELY(ans->data == NULL || l->data== NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

   for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++)
      if (LIKELY(l->data[i]>=0) || MODF(r->data[i],&tmp)==FL(0.0))
        ans->data[i] = POWER(l->data[i],r->data[i]);
      else
        return csound->PerfError(csound, &(p->h),
                                 Str("undefined power in array-var at index %d"),
                                    i);
    return OK;
}


#define IIARRAY(opcode,fn)                              \
  static int32_t opcode(CSOUND *csound, TABARITH *p)    \
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
static int32_t tabiadd(CSOUND *csound, ARRAYDAT *ans, ARRAYDAT *l, MYFLT r, void *p)
{
    int32_t sizel = l->sizes[0];
    int32_t i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, &(((TABARITH *) p)->h),
                               Str("array-variable not initialised"));

   for (i=1; i<l->dimensions; i++) {
      sizel*=l->sizes[i];
    }
    for (i=0; i<sizel; i++)
      ans->data[i] = l->data[i] + r;
    return OK;
}

// K[]+K
static int32_t tabaiadd(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r       = *p->right;
    tabinit_like(csound, ans, l);
    return tabiadd(csound, ans, l, r, p);
}

// K+K[]
static int32_t tabiaadd(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r       = *p->left;
    return tabiadd(csound, ans, l, r, p);
}

/* ****************Array versions of addin/subin*********** */
/// K[] += K
static int32_t addinAA(CSOUND *csound, TABARITHIN1 *p)
{
    ARRAYDAT *ans = p->ans;
    MYFLT r       = *p->right;
    //tabinit_like(csound, ans, l);
    return tabiadd(csound, ans, ans, r, p);
}

// K[] += K[]
static int32_t tabaddinkk(CSOUND *csound, TABARITHIN *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = ans->sizes[0];
    int32_t sizer    = r->sizes[0];
    int32_t i;

    tabinit_like(csound, ans, r);
    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=ans->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel= sizer;
    for (i=0; i<sizel; i++)
      ans->data[i] += r->data[i];
    return OK;
}

//a[]+=a[]
static int32_t tabaaddin(CSOUND *csound, TABARITHIN *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = ans->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=ans->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel= sizer;
    if (UNLIKELY(early)) {
      nsmps -= early;
    }
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] += b[n];
    }
    return OK;
}

// a[]-=a[]
static int32_t tabaasubin(CSOUND *csound, TABARITHIN *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = ans->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=ans->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel= sizer;
    if (UNLIKELY(early)) {
      nsmps -= early;
    }
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] -= b[n];
    }
    return OK;
}

//a[]+=k[]
static int32_t tabarkrddin(CSOUND *csound, TABARITHIN *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *r   = p->right;
    int32_t sizel  = ans->sizes[0];
    int32_t sizer  = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=ans->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT b,*aa;
      int32_t j = i*span;
      b = r->data[i];
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] += b;
    }
    return OK;
}

// a[]-=k[]
static int32_t tabarkrsbin(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = ans->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=ans->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT b,*aa;
      int32_t j = i*span;
      b = r->data[i];
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] -= b;
    }
    return OK;
}

//a[]+=k
static int32_t tabakaddin(CSOUND *csound, TABARITHIN1 *p)
{
    ARRAYDAT *ans   = p->ans;
    MYFLT l         = *p->right;
    int32_t sizel   = ans->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=ans->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *aa;
      int32_t j = i*span;
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] += l;
    }
    return OK;
}
//========================
//a[]-=k
static int32_t tabaksubin(CSOUND *csound, TABARITHIN1 *p)
{
    ARRAYDAT *ans   = p->ans;
    MYFLT l         = *p->right;
    int32_t sizel   = ans->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=ans->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *aa;
      int32_t j = i*span;
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] -= l;
    }
    return OK;
}

// a[] - k
static int32_t tabaksub(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans   = p->ans;
    MYFLT l         = *p->right;
    ARRAYDAT *r     = p->left;
    int32_t sizel   = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = b[n] - l;
    }
    return OK;
}


// K[] -= K
static int32_t subinAA(CSOUND *csound, TABARITHIN *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = ans->sizes[0];
    int32_t sizer    = r->sizes[0];
    int32_t i;

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=ans->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel= sizer;
    for (i=0; i<sizel; i++)
      ans->data[i] -=r->data[i];
    return OK;
}

// K[] -= K[]
static int32_t tabsubinkk(CSOUND *csound, TABARITHIN *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = ans->sizes[0];
    int32_t sizer    = r->sizes[0];
    int32_t i;

    tabinit_like(csound, ans, r);
    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=ans->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel= sizer;
    for (i=0; i<sizel; i++)
      ans->data[i] -= r->data[i];
    return OK;
}




// Subtract K[]-K
static int32_t tabaisub(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r       = *p->right;
    int32_t sizel = l->sizes[0];
    int32_t i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

   for (i=1; i<l->dimensions; i++) {
      sizel*=l->sizes[i];
    }
    for (i=0; i<sizel; i++)
      ans->data[i] = l->data[i] - r;
    return OK;
}

// Subtract K-K[]
static int32_t tabiasub(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r     = *p->left;
    int32_t sizel = l->sizes[0];
    int32_t i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

   for (i=1; i<l->dimensions; i++) {
      sizel*=l->sizes[i];
    }
    for (i=0; i<sizel; i++)
      ans->data[i] = r - l->data[i];
    return OK;
}

// Multiply scalar by array
static int32_t tabimult(CSOUND *csound, ARRAYDAT *ans, ARRAYDAT *l,
                        MYFLT r, void *p)
{
    int32_t sizel    = l->sizes[0];
    int32_t i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, &(((TABARITH1 *)p)->h),
                               Str("array-variable not initialised"));

    for (i=1; i<l->dimensions; i++) {
      sizel*=l->sizes[i];
    }
    for (i=0; i<sizel; i++)
      ans->data[i] = l->data[i] * r;
    return OK;
}

// K[] * K
static int32_t tabaimult(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r       = *p->right;
    return tabimult(csound, ans, l, r, p);
}

// K * K[]
static int32_t tabiamult(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r       = *p->left;
    return tabimult(csound, ans, l, r, p);
}

// K[] / K
static int32_t tabaidiv(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r       = *p->right;
    int32_t sizel = l->sizes[0];
    int32_t i;

    if (UNLIKELY(r==FL(0.0)))
      return csound->PerfError(csound, &(p->h),
                               Str("division by zero in array-var"));
    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<l->dimensions; i++) {
      sizel*=l->sizes[i];
    }
    for (i=0; i<sizel; i++)
      ans->data[i] = l->data[i] / r;
    return OK;
}

// K / K[]
static int32_t tabiadiv(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r     = *p->left;
    int32_t sizel    = l->sizes[0];
    int32_t i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<l->dimensions; i++) {
      sizel*=l->sizes[i];
    }
    for (i=0; i<sizel; i++) {
      if (UNLIKELY(l->data[i]==FL(0.0)))
        return csound->PerfError(csound, &(p->h),
                                 Str("division by zero in array-var"));
      ans->data[i] = r / l->data[i];
    }
    return OK;
}

// K[] % K
static int32_t tabairem(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r       = *p->right;
    int32_t sizel = l->sizes[0];
    int32_t i;

    if (UNLIKELY(r==FL(0.0)))
      return csound->PerfError(csound, &(p->h),
                               Str("division by zero in array-var"));
    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<l->dimensions; i++) {
      sizel*=l->sizes[i];
    }
    for (i=0; i<sizel; i++)
      ans->data[i] = MOD(l->data[i], r);
    return OK;
}

// K % K[]
static int32_t tabiarem(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r       = *p->left;
    int32_t sizel = l->sizes[0];
    int32_t i;

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<l->dimensions; i++) {
      sizel*=l->sizes[i];
    }
    for (i=0; i<sizel; i++) {
      if (UNLIKELY(l->data[i]==FL(0.0)))
        return
          csound->PerfError(csound, &(p->h),
                            Str("division by zero in array-var at index %d"), i);
      else
        ans->data[i] = MOD(r,l->data[i]);
    }
    return OK;
}

// K[] pow K
static int32_t tabaipow(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT r       = *p->right;
    int32_t sizel = l->sizes[0];
    int32_t i;
    MYFLT tmp;
    int32_t intcase = (MODF(r,&tmp)==FL(0.0));

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<l->dimensions; i++) {
      sizel*=l->sizes[i];
    }
    for (i=0; i<sizel; i++) {
      if (intcase || LIKELY(l->data[i]>=0))
        ans->data[i] = POWER(l->data[i], r);
      else
        return csound->PerfError(csound, &(p->h),
                                 Str("undefined power in array-var at index %d"),
                                 i);
    }
    return OK;
}

// K ^ K[]
static int32_t tabiapow(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->right;
    MYFLT r     = *p->left;
    int32_t sizel = l->sizes[0];
    int32_t i;
    MYFLT tmp;
    int32_t poscase = (r>=FL(0.0));

    if (UNLIKELY(ans->data == NULL || l->data== NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<l->dimensions; i++) {
      sizel*=l->sizes[i];
    }
    for (i=0; i<sizel; i++) {
      if (LIKELY(poscase || MODF(l->data[i],&tmp)==FL(0.0)))
        ans->data[i] = POWER(r,l->data[i]);
      else
        return csound->PerfError(csound, &(p->h),
                                 Str("undefined power in array-var at index %d"),
                                    i);
    }
    return OK;
}

#define IiARRAY(opcode,fn)                              \
  static int32_t opcode(CSOUND *csound, TABARITH1 *p)   \
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
  static int32_t opcode(CSOUND *csound, TABARITH2 *p)   \
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

//a[]+a[]
static int32_t tabaadd(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel= sizer;
    if (UNLIKELY(early)) {
      nsmps -= early;
    }
    for (i=0; i<sizel; i++) {
      MYFLT *a,*b,*aa;
      int32_t j = i*span;
      a = (MYFLT*)&(l->data[j]); b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] + b[n];
    }
    return OK;
}

// a[]-a[]
static int32_t tabasub(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel= sizer;
    if (UNLIKELY(early)) {
      nsmps -= early;
    }
    for (i=0; i<sizel; i++) {
      MYFLT *a,*b,*aa;
      int32_t j = i*span;
      a = (MYFLT*)&(l->data[j]); b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] - b[n];
    }
    return OK;
}

// a[]*a[]
static int32_t tabamul(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel= sizer;
    if (UNLIKELY(early)) {
      nsmps -= early;
    }
    for (i=0; i<sizel; i++) {
      MYFLT *a,*b,*aa;
      int32_t j = i*span;
      a = (MYFLT*)&(l->data[j]); b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] * b[n];
    }
    return OK;
}

// a[]/a[]
static int32_t tabadiv(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel= sizer;
    if (UNLIKELY(early)) {
      nsmps -= early;
    }
    for (i=0; i<sizel; i++) {
      MYFLT *a,*b,*aa;
      int32_t j = i*span;
      a = (MYFLT*)&(l->data[j]); b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++) {
        if (UNLIKELY(b[n]==FL(0.0)))
          return csound->PerfError(csound, &(p->h),
                                  Str("division by zero in array-var "
                                      "at index %d/%d"), i,n);
        aa[n] = a[n] / b[n];
      }
    }
    return OK;
}

// k * a[]
static int32_t tabkamult(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans   = p->ans;
    MYFLT l         = *p->left;
    ARRAYDAT *r     = p->right;
    int32_t sizel        = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = l * b[n];
    }
    return OK;
}

// a[] * k
static int32_t tabakmult(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans   = p->ans;
    MYFLT l         = *p->right;
    ARRAYDAT *r     = p->left;
    int32_t sizel        = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = l * b[n];
    }
    return OK;
}

// k + a[]
static int32_t tabkaadd(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans   = p->ans;
    MYFLT l         = *p->left;
    ARRAYDAT *r     = p->right;
    int32_t sizel   = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = l + b[n];
    }
    return OK;
}

// a[] + k
static int32_t tabakadd(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans   = p->ans;
    MYFLT l         = *p->right;
    ARRAYDAT *r     = p->left;
    int32_t sizel   = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = l + b[n];
    }
    return OK;
}
//========================
//k - a[]
static int32_t tabkasub(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans   = p->ans;
    MYFLT l         = *p->left;
    ARRAYDAT *r     = p->right;
    int32_t sizel   = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = l - b[n];
    }
    return OK;
}

//k / a[]
static int32_t tabkadiv(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans   = p->ans;
    MYFLT l         = *p->left;
    ARRAYDAT *r     = p->right;
    int32_t sizel   = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++) {
        if (UNLIKELY(b[n]==FL(0.0)))
          return csound->PerfError(csound, &(p->h),
                            Str("division by zero in array-var "
                                "at index %d/%d"), i,n);
        aa[n] = l / b[n];
      }
    }
    return OK;
}

// a[] / k
static int32_t tabakdiv(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans   = p->ans;
    MYFLT l         = *p->right;
    ARRAYDAT *r     = p->left;
    int32_t sizel    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));
    if (UNLIKELY(l==FL(0.0)))
      return csound->PerfError(csound, &(p->h),
                               Str("division by zero in array-var"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = b[n] / l;
    }
    return OK;
}

// a[] % k
static int32_t tabarkrem(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans   = p->ans;
    MYFLT l         = *p->right;
    ARRAYDAT *r     = p->left;
    int32_t sizel   = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));
    if (UNLIKELY(l==FL(0.0)))
      return csound->PerfError(csound, &(p->h),
                               Str("division by zero in array-var"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = MOD(b[n], l);
    }
    return OK;
}

// a[] ^ k
static int32_t tabarkpow(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans   = p->ans;
    MYFLT l         = *p->right;
    ARRAYDAT *r     = p->left;
    int32_t sizel   = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = POWER(b[n],l);
    }
    return OK;
}

//a+a[]
static int32_t tabaardd(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *r   = p->right;
    MYFLT    *a   = p->left;
    int32_t sizel = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] + b[n];
    }
    return OK;
}

//a-a[]
static int32_t tabaarsb(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *r   = p->right;
    MYFLT    *a   = p->left;
    int32_t sizel   = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] - b[n];
    }
    return OK;
}

//a*a[]
static int32_t tabaarml(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *r   = p->right;
    MYFLT    *a   = p->left;
    int32_t sizel = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i+span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] * b[n];
    }
    return OK;
}

//a/a[]
static int32_t tabaardv(CSOUND *csound, TABARITH2 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *r   = p->right;
    MYFLT    *a   = p->left;
    int32_t sizel = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=r->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *b,*aa;
      int32_t j = i*span;
      b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] / b[n];
    }
    return OK;
}

//a[]+a
static int32_t tabaradd(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT    *b   = p->right;
    int32_t sizel = l->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=l->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *a,*aa;
      int32_t j = i*span;
      a = (MYFLT*)&(l->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] + b[n];
    }
    return OK;
}

//a[]-a
static int32_t tabarasb(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT    *b   = p->right;
    int32_t sizel = l->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=l->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *a,*aa;
      int32_t j = i*span;
      a = (MYFLT*)&(l->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] - b[n];
    }
    return OK;
}

//a[]*a
static int32_t tabaraml(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT    *b   = p->right;
    int32_t sizel = l->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=l->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *a,*aa;
      int32_t j = i*span;
      a = (MYFLT*)&(l->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] * b[n];
    }
    return OK;
}

//a[]/a
static int32_t tabaradv(CSOUND *csound, TABARITH1 *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    MYFLT    *b   = p->right;
    int32_t sizel = l->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++)
      sizel*=l->sizes[i];
    for (i=0; i<sizel; i++) {
      MYFLT *a,*aa;
      int32_t j = i*span;
      a = (MYFLT*)&(l->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] / b[n];
    }
    return OK;
}


// k[]+a[]
static int32_t tabkrardd(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel = l->sizes[0];
    int32_t sizer = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT a, *b,*aa;
      int32_t j = i*span;
      a = l->data[i]; b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a + b[n];
    }
    return OK;
}

// k[]-a[]
static int32_t tabkrarsb(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel = l->sizes[0];
    int32_t sizer = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT a, *b,*aa;
      int32_t j = i*span;
      a = l->data[i]; b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a - b[n];
    }
    return OK;
}

// k[]*a[]
static int32_t tabkrarml(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT a, *b,*aa;
      int32_t j = i*span;
      a = l->data[i]; b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a * b[n];
    }
    return OK;
}

// k[]/a[]
static int32_t tabkrardv(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans       = p->ans;
    ARRAYDAT *l         = p->left;
    ARRAYDAT *r         = p->right;
    int32_t sizel       = l->sizes[0];
    int32_t sizer       = r->sizes[0];
    uint32_t offset     = p->h.insdshead->ksmps_offset;
    uint32_t early      = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span        = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT a, *b,*aa;
      int32_t j = i*span;
      a = l->data[i]; b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a / b[n];
    }
    return OK;
}

// k[]%a[]
static int32_t tabkrarmd(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT a, *b,*aa;
      int32_t j = i*span;
      a = l->data[i]; b = (MYFLT*)&(r->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = MOD(a, b[n]);
    }
    return OK;
}

// a[]+k[]
static int32_t tabarkrdd(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel  = l->sizes[0];
    int32_t sizer  = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT *a, b,*aa;
      int32_t j = i*span;
      b = r->data[i]; a = (MYFLT*)&(l->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] + b;
    }
    return OK;
}

// a[]-k[]
static int32_t tabarkrsb(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT *a, b,*aa;
      int32_t j = i*span;
      b = r->data[i]; a = (MYFLT*)&(l->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] - b;
    }
    return OK;
}

// a[]*k[]
static int32_t tabarkrml(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT *a, b,*aa;
      int32_t j = i*span;
      b = r->data[i]; a = (MYFLT*)&(l->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] * b;
    }
    return OK;
}

// a[]/k[]
static int32_t tabarkrdv(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT *a, b,*aa;
      int32_t j = i*span;
      b = r->data[i]; a = (MYFLT*)&(l->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = a[n] / b;
    }
    return OK;
}

// a[]%k[]
static int32_t tabarkrmd(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT *a, b,*aa;
      int32_t j = i*span;
      b = r->data[i]; a = (MYFLT*)&(l->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = MOD(a[n], b);
    }
    return OK;
}

// a[]^k[]
static int32_t tabarkrpw(CSOUND *csound, TABARITH *p)
{
    ARRAYDAT *ans = p->ans;
    ARRAYDAT *l   = p->left;
    ARRAYDAT *r   = p->right;
    int32_t sizel    = l->sizes[0];
    int32_t sizer    = r->sizes[0];
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t i, n, nsmps = CS_KSMPS-early;
    int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));

    for (i=1; i<ans->dimensions; i++) {
      sizel*=l->sizes[i];
      sizer*=r->sizes[i];
    }
    if (sizer<sizel) sizel = sizer;
    for (i=0; i<sizel; i++) {
      MYFLT *a, b,*aa;
      int32_t j = i*span;
      b = r->data[i]; a = (MYFLT*)&(l->data[j]);
      aa = (MYFLT*)&(ans->data[j]);
      if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
      if (UNLIKELY(early)) {
        memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
      }
      for (n=offset; n<nsmps; n++)
        aa[n] = POWER(a[n], b);
    }
    return OK;
}



////////////////// more here
//#include "arrayarith.c"




static int32_t tabqset(CSOUND *csound, TABQUERY *p)
{
    if (LIKELY(p->tab->data)) return OK;
    return csound->InitError(csound, "%s", Str("array-variable not initialised"));
}

static int32_t tabqset1(CSOUND *csound, TABQUERY1 *p)
{
    if (LIKELY(p->tab->data)) return OK;
    return csound->InitError(csound, "%s", Str("array-variable not initialised"));
}

static int32_t tabmax(CSOUND *csound, TABQUERY *p)
{
    ARRAYDAT *t = p->tab;
    int32_t i, size = 0, pos = 0;;
    MYFLT ans;

    if (UNLIKELY(t->data == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));
    /* if (UNLIKELY(t->dimensions!=1)) */
    /*      return csound->PerfError(csound, &(p->h), */
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

static int32_t tabmax1(CSOUND *csound, TABQUERY *p)
{
    if (tabqset(csound, p) == OK) return tabmax(csound, p);
    else return NOTOK;
}

static int32_t tabmin(CSOUND *csound, TABQUERY *p)
{
    ARRAYDAT *t = p->tab;
    int32_t i, size = 0, pos = 0;
    MYFLT ans;

    if (UNLIKELY(t->data == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));
    /* if (UNLIKELY(t->dimensions!=1)) */
    /*   return csound->PerfError(csound,
         &(p->h), Str("array-variable not a vector")); */

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

static int32_t tabmin1(CSOUND *csound, TABQUERY *p)
{
    if (LIKELY(tabqset(csound, p) == OK)) return tabmin(csound, p);
    else return NOTOK;
}


static int32_t tabsuma(CSOUND *csound, TABQUERY1 *p)
{
    ARRAYDAT *t = p->tab;
    int32_t i, numarrays = 0;
    MYFLT *ans = p->ans, *in0, *in1, *in2, *in3;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t nsmps = CS_KSMPS;
    int32_t span = (t->arrayMemberSize)/sizeof(MYFLT);

    if (UNLIKELY(t->data == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));
    if (UNLIKELY(t->dimensions!=1))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not a vector"));


    if (UNLIKELY(offset)) memset(ans, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
        nsmps -= early;
        memset(&ans[nsmps], '\0', early*sizeof(MYFLT));
    }

    for (i=0; i<t->dimensions; i++) numarrays += t->sizes[i];

    memset(&ans[offset], '\0', nsmps*sizeof(MYFLT));

    int numarrays4 = numarrays - (numarrays % 4);

    for (i=0; i<numarrays4; i+=4) {
        in0 = &(t->data[i*span]);
        in1 = &(t->data[(i+1)*span]);
        in2 = &(t->data[(i+2)*span]);
        in3 = &(t->data[(i+3)*span]);

        for(int j=offset; j < nsmps; j++) {
            ans[j] += in0[j] + in1[j] + in2[j] + in3[j];
        }
    }

    for (i=numarrays4;i<numarrays; i++) {
        in0 = &(t->data[i*span]);
        for(int j=offset; j < nsmps; j++) {
            ans[j] += in0[j];
        }
    }
    return OK;
}

static int32_t tabclearset(CSOUND *csound, TABCLEAR *p)
{
    if (LIKELY(p->tab->data)) return OK;
    return csound->InitError(csound, "%s", Str("array-variable not initialised"));
}


static int32_t tabclear(CSOUND *csound, TABCLEAR *p)
{
    ARRAYDAT *t = p->tab;
    int32_t i;
    int32_t nsmps = CS_KSMPS;
    int32_t size = 1;
    
    if (UNLIKELY(t->data == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));
    /* if (UNLIKELY(t->dimensions!=1)) */
    /*   return csound->PerfError(csound, &(p->h), */
    /*                            Str("array-variable not a vector")); */

    for(i = 0; i < t->dimensions; i++) size *= t->sizes[i];
    memset(t->data, 0, sizeof(MYFLT)*nsmps*size);
    
    return OK;
}


static int32_t tabcleark(CSOUND *csound, TABCLEAR *p)
{
    ARRAYDAT *t = p->tab;
    int32_t i;
    int32_t size = 1;
    
    if (UNLIKELY(t->data == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));
    /* if (UNLIKELY(t->dimensions!=1)) */
    /*   return csound->PerfError(csound, &(p->h), */
    /*                            Str("array-variable not a vector")); */

    for(i = 0; i < t->dimensions; i++) size *= t->sizes[i];
    memset(t->data, 0, sizeof(MYFLT)*size);
    
    return OK;
}



static int32_t tabsum(CSOUND *csound, TABQUERY1 *p)
{
    ARRAYDAT *t = p->tab;
    int32_t i, size = 0;
    MYFLT ans;

    if (UNLIKELY(t->data == NULL))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not initialised"));
    if (UNLIKELY(t->dimensions!=1))
      return csound->PerfError(csound, &(p->h),
                               Str("array-variable not a vector"));
    ans = t->data[0];
    for (i=0; i<t->dimensions; i++) size += t->sizes[i];
    for (i=1; i<size; i++)
      ans += t->data[i];
    *p->ans = ans;
    return OK;
}

static int32_t tabsuma1(CSOUND *csound, TABQUERY1 *p)
{
    if (LIKELY(tabqset1(csound, p) == OK)) return tabsuma(csound, p);
    else return NOTOK;
}

static int32_t tabsum1(CSOUND *csound, TABQUERY1 *p)
{
    if (LIKELY(tabqset1(csound, p) == OK)) return tabsum(csound, p);
    else return NOTOK;
}

static int32_t tabscaleset(CSOUND *csound, TABSCALE *p)
{
    if (LIKELY(p->tab->data && p->tab->dimensions==1)) return OK;
    return csound->InitError(csound, "%s", Str("array-variable not initialised"));
}

static int32_t tabscale(CSOUND *csound, TABSCALE *p)
{
    IGN(csound);
    MYFLT min = *p->kmin, max = *p->kmax;
    int32_t strt = (int32_t)MYFLT2LRND(*p->kstart),
            end = (int32_t)MYFLT2LRND(*p->kend);
    ARRAYDAT *t = p->tab;
    MYFLT tmin;
    MYFLT tmax;
    int32_t i;
    MYFLT range;

    tmin = t->data[strt];
    tmax = tmin;

    // Correct start and ending points
    if (end<0) end = t->sizes[0];
    else if (end>t->sizes[0]) end = t->sizes[0];
    if (strt<0) strt = 0;
    else if (strt>t->sizes[0]) strt = t->sizes[0];
    if (end<strt) {
      int32_t x = end; end = strt; strt = x;
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

static int32_t tabscale1(CSOUND *csound, TABSCALE *p)
{
    if (LIKELY(tabscaleset(csound, p) == OK)) return tabscale(csound, p);
    else return NOTOK;
}

typedef struct {
  OPDS     h;
  ARRAYDAT *dst;
  ARRAYDAT *src;
  int32_t      len;
} TABCPY;

static int32_t get_array_total_size(ARRAYDAT* dat)
{
    int32_t i;
    int32_t size;

    if (UNLIKELY(dat->sizes == NULL)) {
      return -1;
    }

    size = dat->sizes[0];
    for (i = 1; i < dat->dimensions; i++) {
      size *= dat->sizes[i];
    }
    return size;
}

static int32_t tabcopy(CSOUND *csound, TABCPY *p)
{
    int32_t i, arrayTotalSize, memMyfltSize;

    if (UNLIKELY(p->src->data==NULL) || p->src->dimensions <= 0 )
      return csound->InitError(csound, "%s", Str("array-variable not initialised"));
    if (UNLIKELY(p->dst->dimensions > 0 &&
                 p->src->dimensions != p->dst->dimensions))
      return csound->InitError(csound, "%s",
                               Str("array-variable dimensions do not match"));
    if (UNLIKELY(p->src->arrayType != p->dst->arrayType))
      return csound->InitError(csound, "%s",
                               Str("array-variable types do not match"));

    if (p->src == p->dst) return OK;

    arrayTotalSize = get_array_total_size(p->src);
    memMyfltSize = p->src->arrayMemberSize / sizeof(MYFLT);
    p->dst->arrayMemberSize = p->src->arrayMemberSize;

    if (arrayTotalSize != get_array_total_size(p->dst)) {
      p->dst->dimensions = p->src->dimensions;

      p->dst->sizes = csound->Malloc(csound, sizeof(int32_t) * p->src->dimensions);
      memcpy(p->dst->sizes, p->src->sizes, sizeof(int32_t) * p->src->dimensions);

      if (p->dst->data == NULL) {
        p->dst->data = csound->Calloc(csound,
                                      p->src->arrayMemberSize * arrayTotalSize);
        p->dst->allocated = p->src->arrayMemberSize * arrayTotalSize;
      } else {
        p->dst->data = csound->ReAlloc(csound, p->dst->data,
                        p->src->arrayMemberSize * arrayTotalSize);
        memset(p->dst->data, 0, p->src->arrayMemberSize * arrayTotalSize);
      }
    }

    for (i = 0; i < arrayTotalSize; i++) {
      int32_t index = (i * memMyfltSize);
      p->dst->arrayType->copyValue(csound, p->dst->arrayType,
                                   (void*)(p->dst->data + index),
                                   (void*)(p->src->data + index));
    }

    return OK;
}

static int32_t tabcopyk_init(CSOUND *csound, TABCPY *p) {
    tabinit(csound, p->dst, get_array_total_size(p->src));
    return OK;
}
static int32_t tabcopyk(CSOUND *csound, TABCPY *p)
{
    int32_t i, arrayTotalSize, memMyfltSize;

    if (UNLIKELY(p->src->data==NULL) || p->src->dimensions <= 0 )
      return csound->PerfError(csound, &(p->h), "%s", Str("array-variable not initialised"));
    if (UNLIKELY(p->dst->dimensions > 0 &&
                 p->src->dimensions != p->dst->dimensions))
      return csound->PerfError(csound,&(p->h), "%s",
                               Str("array-variable dimensions do not match"));
    if (UNLIKELY(p->src->arrayType != p->dst->arrayType))
      return csound->PerfError(csound, &(p->h), "%s",
                               Str("array-variable types do not match"));

    if (p->src == p->dst) return OK;

    arrayTotalSize = get_array_total_size(p->src);
    memMyfltSize = p->src->arrayMemberSize / sizeof(MYFLT);
    p->dst->arrayMemberSize = p->src->arrayMemberSize;

    if (arrayTotalSize != get_array_total_size(p->dst)) {
      p->dst->dimensions = p->src->dimensions;

      p->dst->sizes = csound->Malloc(csound, sizeof(int32_t) * p->src->dimensions);
      memcpy(p->dst->sizes, p->src->sizes, sizeof(int32_t) * p->src->dimensions);

      if (p->dst->data == NULL) {
        p->dst->data = csound->Calloc(csound,
                                      p->src->arrayMemberSize * arrayTotalSize);
        p->dst->allocated = p->src->arrayMemberSize * arrayTotalSize;
      } else {
        p->dst->data = csound->ReAlloc(csound, p->dst->data,
                        p->src->arrayMemberSize * arrayTotalSize);
        memset(p->dst->data, 0, p->src->arrayMemberSize * arrayTotalSize);
      }
    }
 
    for (i = 0; i < arrayTotalSize; i++) {
      int32_t index = (i * memMyfltSize);
      p->dst->arrayType->copyValue(csound, p->dst->arrayType,
                                   (void*)(p->dst->data + index),
                                   (void*)(p->src->data + index));
    }

    return OK;
}


static int32_t tabcopy1(CSOUND *csound, TABCPY *p)
{
    int32_t i, arrayTotalSize, memMyfltSize;

    if (UNLIKELY(p->src->data==NULL) || p->src->dimensions <= 0 )
      return csound->InitError(csound, "%s", Str("array-variable not initialised"));
    if (p->dst->dimensions > 0 && p->src->dimensions != p->dst->dimensions)
      return csound->InitError(csound, "%s",
                               Str("array-variable dimensions do not match"));
//    if (p->src->arrayType != p->dst->arrayType)
//      return csound->InitError(csound, "%s",
//                               Str("array-variable types do not match"));

    if (p->src == p->dst) return OK;

    arrayTotalSize = get_array_total_size(p->src);
    memMyfltSize = p->src->arrayMemberSize / sizeof(MYFLT);
    p->dst->arrayMemberSize = p->src->arrayMemberSize;

    if (arrayTotalSize != get_array_total_size(p->dst)) {
      p->dst->dimensions = p->src->dimensions;

      p->dst->sizes = csound->Malloc(csound, sizeof(int32_t) * p->src->dimensions);
      memcpy(p->dst->sizes, p->src->sizes, sizeof(int32_t) * p->src->dimensions);

      if (p->dst->data == NULL) {
        p->dst->data = csound->Calloc(csound,
                                      p->src->arrayMemberSize * arrayTotalSize);
        p->dst->allocated = p->src->arrayMemberSize * arrayTotalSize;
      } else {
        p->dst->data = csound->ReAlloc(csound, p->dst->data,
                        p->src->arrayMemberSize * arrayTotalSize);
        memset(p->dst->data, 0, p->src->arrayMemberSize * arrayTotalSize);
      }
    }


    for (i = 0; i < arrayTotalSize; i++) {
      int32_t index = (i * memMyfltSize);
      p->dst->arrayType->copyValue(csound,
                                   p->dst->arrayType,
                                   (void*)(p->dst->data + index),
                                   (void*)(p->src->data + index));
    }

    return OK;
}

static int32_t tabcopy2(CSOUND *csound, TABCPY *p)
{                               /* Like tabcopy but sample-accurate */
    int32_t i, j, arrayTotalSize;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    int32_t nsmps = CS_KSMPS;
    MYFLT * dest, *src;
    uint32_t k;

    if (UNLIKELY(p->src->data==NULL) || p->src->dimensions <= 0 )
      return csound->InitError(csound, "%s", Str("array-variable not initialised"));
    if (UNLIKELY(p->dst->dimensions > 0 &&
                 p->src->dimensions != p->dst->dimensions))
      return csound->InitError(csound, "%s",
                               Str("array-variable dimensions do not match"));
    if (UNLIKELY(p->src->arrayType != p->dst->arrayType))
      return csound->InitError(csound, "%s",
                               Str("array-variable types do not match"));

    if (p->src == p->dst) return OK;

    arrayTotalSize = get_array_total_size(p->src);
    p->dst->arrayMemberSize = p->src->arrayMemberSize;

    if (arrayTotalSize != get_array_total_size(p->dst)) {
      p->dst->dimensions = p->src->dimensions;

      p->dst->sizes = csound->Malloc(csound, sizeof(int32_t) * p->src->dimensions);
      memcpy(p->dst->sizes, p->src->sizes, sizeof(int32_t) * p->src->dimensions);

      if (p->dst->data == NULL) {
        p->dst->data = csound->Calloc(csound,
                                      p->src->arrayMemberSize * arrayTotalSize);
        p->dst->allocated = p->src->arrayMemberSize * arrayTotalSize;
      } else {
        p->dst->data = csound->ReAlloc(csound, p->dst->data,
                        p->src->arrayMemberSize * arrayTotalSize);
        memset(p->dst->data, 0, p->src->arrayMemberSize * arrayTotalSize);
      }
    }
    dest = (MYFLT*)p->dst->data;
    src = (MYFLT*)p->src->data;
    for (i=0;i<p->dst->dimensions; i++) {
      for (j=0; j<p->src->sizes[i]; j++) {
        if (offset)
          memset(dest, '\0', offset*sizeof(MYFLT));
        if (early) memset(&dest[nsmps-early], '\0', early*sizeof(MYFLT));
        for (k=offset;k<nsmps-early;k++)
          dest[k] = src[k];
        dest +=nsmps; src += nsmps;
      }
    }
    return OK;
}



static int32_t tab2ftab(CSOUND *csound, TABCOPY *p)
{
    FUNC        *ftp;
    int32_t fsize;
    MYFLT *fdata;
    ARRAYDAT *t = p->tab;
    int32_t i, tlen = 0;

    if (UNLIKELY(p->tab->data==NULL))
      return csound->PerfError(csound,
                               &(p->h), Str("array-var not initialised"));
    if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL))
      return csound->PerfError(csound,
                               &(p->h), Str("No table for copy2ftab"));
    for (i=0; i<t->dimensions; i++) tlen += t->sizes[i];
    fsize = ftp->flen;
    fdata = ftp->ftable;
    if (fsize<tlen) tlen = fsize;
    memcpy(fdata, p->tab->data, sizeof(MYFLT)*tlen);
    return OK;
}

static int32_t tab2ftabi(CSOUND *csound, TABCOPY *p)
{
    FUNC        *ftp;
    int32_t fsize;
    MYFLT *fdata;
    ARRAYDAT *t = p->tab;
    int32_t i, tlen = 0;

    if (UNLIKELY(p->tab->data==NULL))
      return csound->InitError(csound, "%s", Str("array-var not initialised"));
    if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL))
      return csound->InitError(csound, "%s", Str("No table for copy2ftab"));
    for (i=0; i<t->dimensions; i++) tlen += t->sizes[i];
    fsize = ftp->flen;
    fdata = ftp->ftable;
    if (fsize<tlen) tlen = fsize;
    memcpy(fdata, p->tab->data, sizeof(MYFLT)*tlen);
    return OK;
}

// copya2ftab k[], iftable, [, koffset=0]
static int32_t tab2ftab_offset(CSOUND *csound, TABCOPY2 *p)
{
  FUNC *ftp;
  int32_t fsize;
  MYFLT *fdata;
  ARRAYDAT *t = p->tab;
  int32_t offset = (int)(*p->offset);
  int32_t i, tlen = 0, maxitems;
  if (UNLIKELY(t->data==NULL))
    return csound->PerfError(csound, &(p->h), Str("array-var not initialised"));
  if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL))
    return csound->PerfError(csound, &(p->h), Str("No table for copy2ftab"));
  fsize = ftp->flen;
  if (UNLIKELY(offset >= fsize || offset < 0))
      return csound->PerfError(csound, &(p->h), Str("Offset is out of bounds"));
  for (i=0; i<t->dimensions; i++)
      tlen += t->sizes[i];
  fdata = ftp->ftable;
  maxitems = fsize - offset;
  if (maxitems < tlen)
      tlen = maxitems;
  memcpy(&(fdata[offset]), t->data, sizeof(MYFLT)*tlen);
  return OK;
}

// copya2ftab i[], iftable, [, ioffset=0]
static int32_t tab2ftab_offset_i(CSOUND *csound, TABCOPY2 *p)
{
    FUNC *ftp;
    int32_t fsize;
    MYFLT *fdata;
    ARRAYDAT *t = p->tab;
    int32_t offset = (int)*p->offset;;
    int32_t i, tlen = 0, maxitems;
    if (UNLIKELY(t->data==NULL))
      return csound->InitError(csound, "%s", Str("array-var not initialised"));
    if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL))
      return csound->InitError(csound, "%s", Str("No table for copy2ftab"));
    fsize = ftp->flen;
    fdata = ftp->ftable;
    if (UNLIKELY(offset >= fsize || offset < 0))
        return csound->InitError(csound, "%s", Str("Offset is out of bounds"));
    for (i=0; i<t->dimensions; i++)
        tlen += t->sizes[i];
    maxitems = fsize - offset;
    if (maxitems < tlen)
        tlen = maxitems;
    memcpy(&(fdata[offset]), t->data, sizeof(MYFLT)*tlen);
    return OK;
}


typedef struct {
    OPDS h;
    ARRAYDAT *tab;
  MYFLT *start, *end, *incr;
  int32_t    len;
} TABGEN;


static int32_t tabgen(CSOUND *csound, TABGEN *p)
{
    MYFLT *data =  p->tab->data;
    MYFLT start = *p->start;
    MYFLT end   = *p->end;
    MYFLT incr  = *p->incr;
    int32_t i, size =  (end - start)/incr + 1;

    //printf("start=%f end=%f incr=%f size=%d\n", start, end, incr, size);
    if (UNLIKELY(size < 0))
      return
        csound->InitError(csound, "%s",
                          Str("inconsistent start, end and increment parameters"));
    tabinit(csound, p->tab, size);
    if (UNLIKELY(p->tab->data==NULL)) {
      tabinit(csound, p->tab, size);
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


static int32_t ftab2tabi(CSOUND *csound, TABCOPY *p)
{
    FUNC        *ftp;
    int32_t         fsize;
    MYFLT       *fdata;
    int32_t tlen;

    if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL))
      return csound->InitError(csound, "%s", Str("No table for copy2ftab"));
    fsize = ftp->flen;
    if (UNLIKELY(p->tab->data==NULL)) {
      tabinit(csound, p->tab, fsize);
      p->tab->sizes[0] = fsize;
    }
    tlen = p->tab->sizes[0];
    fdata = ftp->ftable;
    if (fsize<tlen) tlen = fsize;
    memcpy(p->tab->data, fdata, sizeof(MYFLT)*tlen);
    return OK;
}

static int32_t ftab2tab(CSOUND *csound, TABCOPY *p)
{
    FUNC        *ftp;
    int32_t      fsize;
    MYFLT       *fdata;
    int32_t      tlen;

    if (UNLIKELY((ftp = csound->FTFindP(csound, p->kfn)) == NULL))
      return csound->PerfError(csound,
                               &(p->h), Str("No table for copy2ftab"));
    fsize = ftp->flen;
    if (UNLIKELY(p->tab->data==NULL)) {
      if (tabcheck(csound, p->tab, fsize, &(p->h))) return NOTOK;
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
  ARRAYDAT  *tab;
  MYFLT     *size;
} TRIM;

static int32_t trim_i(CSOUND *csound, TRIM *p)
{
    int size = (int)(*p->size);
    tabinit(csound, p->tab, size);
    p->tab->sizes[0] = size;
    return OK;
}
static int32_t trim(CSOUND *csound, TRIM *p)
{
    int size = (int)(*p->size);
    int n = tabcheck(csound, p->tab, size, &(p->h));
    if (n != OK) return n;
    p->tab->sizes[0] = size;
    return OK;
}


typedef struct {
  OPDS h;
  ARRAYDAT *tab, *tabin;
  MYFLT    *start, *end, *inc;
  int32_t   len;
} TABSLICE;


static int32_t tabslice(CSOUND *csound, TABSLICE *p) {

    MYFLT *tabin = p->tabin->data;
    int32_t start = (int32_t) *p->start;
    int32_t end   = (int32_t) *p->end;
    int32_t inc   = (int32_t) *p->inc;
    int32_t size = (end - start)/inc + 1;
    int32_t i, destIndex;
    int32_t memMyfltSize = p->tabin->arrayMemberSize / sizeof(MYFLT);

    if (UNLIKELY(size < 0))
      return csound->InitError(csound, "%s",
                               Str("inconsistent start, end parameters"));
    if (UNLIKELY(p->tabin->dimensions!=1 || end >= p->tabin->sizes[0])) {
      //printf("size=%d old tab size = %d\n", size, p->tabin->sizes[0]);
      return csound->InitError(csound, "%s",
                               Str("slice larger than original size"));
    }
    if (UNLIKELY(inc<=0))
      return csound->InitError(csound, "%s",
                               Str("slice increment must be positive"));
    tabinit(csound, p->tab, size);

    for (i = start, destIndex = 0; i < end + 1; i+=inc, destIndex++) {
      p->tab->arrayType->copyValue(csound, p->tab->arrayType,
                                   p->tab->data + (destIndex * memMyfltSize),
                                   tabin + (memMyfltSize * i));
    }

    return OK;
}

//#include "str_ops.h"
//// This cheats using strcpy opcode fake
//static int32_t tabsliceS(CSOUND *csound, TABSLICE *p) {
//
//    MYFLT *tabin = p->tabin->data;
//    int32_t start = (int32_t) *p->start;
//    int32_t end   = (int32_t) *p->end;
//    int32_t size = end - start + 1, i;
//    STRCPY_OP xx;
//    if (UNLIKELY(size < 0))
//      return csound->InitError(csound, "%s",
//                               Str("inconsistent start, end parameters"));
//    if (UNLIKELY(p->tabin->dimensions!=1 || size > p->tabin->sizes[0])) {
//      //printf("size=%d old tab size = %d\n", size, p->tabin->sizes[0]);
//      return csound->InitError(csound, "%s",
//                               Str("slice larger than original size"));
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
  int32_t    len;
  OENTRY *opc;
} TABMAP;



static int32_t tabmap_set(CSOUND *csound, TABMAP *p)
{
    MYFLT *data, *tabin = p->tabin->data;
    int32_t n, size;
    OENTRY *opc  = NULL;
    EVAL  eval;

    if (UNLIKELY(p->tabin->data == NULL)||p->tabin->dimensions!=1)
      return csound->InitError(csound, "%s", Str("array-var not initialised"));

    size = p->tabin->sizes[0];
    if (UNLIKELY(p->tab->data==NULL)) {
      tabinit(csound, p->tab, size);
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

static int32_t tabmap_perf(CSOUND *csound, TABMAP *p)
{
    /* FIXME; eeds check */
    MYFLT *data =  p->tab->data, *tabin = p->tabin->data;
    int32_t n, size;
    OENTRY *opc  = p->opc;
    EVAL  eval;

    if (UNLIKELY(p->tabin->data == NULL) || p->tabin->dimensions !=1)
      return csound->PerfError(csound,
                               &(p->h), Str("array-var not initialised"));
    if (UNLIKELY(p->tab->data==NULL) || p->tab->dimensions !=1)
      return csound->PerfError(csound,
                               &(p->h), Str("array-var not initialised"));
    size = p->tab->sizes[0];

    if (UNLIKELY(opc == NULL))
      return csound->PerfError(csound,
                               &(p->h), Str("map fn not found at k rate"));
    for (n=0; n < size; n++) {
      eval.a = &tabin[n];
      eval.r = &data[n];
      opc->kopadr(csound, (void *) &eval);
    }

    return OK;
}

int32_t tablength(CSOUND *csound, TABQUERY1 *p)
{
    IGN(csound);
    int32_t opt = (int32_t)*p->opt;
    if (UNLIKELY(p->tab==NULL || opt>p->tab->dimensions))
      *p->ans = -FL(1.0);
    else if (UNLIKELY(opt<=0)) *p->ans = p->tab->dimensions;
    else *p->ans = p->tab->sizes[opt-1];
    return OK;
}

typedef struct {
  OPDS h;
  ARRAYDAT *tabin;
  uint32_t    len;
} OUTA;

static int32_t ina_set(CSOUND *csound, OUTA *p)
{
    ARRAYDAT *aa = p->tabin;
    // should call ensure here but it is a-rate
    aa->dimensions = 1;
    if (aa->sizes) csound->Free(csound, aa->sizes);
    if (aa->data) csound->Free(csound, aa->data);
    aa->sizes = (int32_t*)csound->Malloc(csound, sizeof(int32_t));
    aa->sizes[0] = p->len = csound->inchnls;
    aa->data = (MYFLT*)
      csound->Malloc(csound, CS_KSMPS*sizeof(MYFLT)*p->len);
    aa->arrayMemberSize = CS_KSMPS*sizeof(MYFLT);
    return OK;
}

static int32_t ina(CSOUND *csound, OUTA *p)
{
    IGN(csound);
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

static int32_t monitora_perf(CSOUND *csound, OUTA *p)
{
    ARRAYDAT *aa = p->tabin;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t i, j, l, nsmps = CS_KSMPS;
    MYFLT       *data = aa->data;
    MYFLT       *sp= CS_SPOUT;
    uint32_t len = (uint32_t)p->len;

      for (l=0; l<len; l++) {
        sp = CS_SPOUT;
        memset(data, '\0', nsmps*sizeof(MYFLT));
        if (UNLIKELY(early)) {
          nsmps -= early;
        }
        for (i = 0; i<nsmps; i++) {
          for (j = 0; j<csound->GetNchnls(csound); j++) {
            if (i<offset)
              data[i+j*nsmps] = FL(0.0);
            else
              data[i+j*nsmps] = sp[i+j*nsmps];
          }
        }
      }
    return OK;
}

static int32_t monitora_init(CSOUND *csound, OUTA *p)
{
    ARRAYDAT *aa = p->tabin;
    // should call ensure here but it is a-rate
    aa->dimensions = 1;
    if (aa->sizes) csound->Free(csound, aa->sizes);
    if (aa->data) csound->Free(csound, aa->data);
    aa->sizes = (int32_t*)csound->Malloc(csound, sizeof(int32_t));
    aa->sizes[0] = p->len = csound->nchnls;
    aa->data = (MYFLT*)
      csound->Malloc(csound, CS_KSMPS*sizeof(MYFLT)*p->len);
    aa->arrayMemberSize = CS_KSMPS*sizeof(MYFLT);
    return OK;
}

/*
  transform operations
*/

typedef struct _autocorr {
  OPDS h;
  ARRAYDAT *out;
  ARRAYDAT *in;
  AUXCH mem;
  int32_t N;
  int32_t FN;
} AUTOCORR;

int32_t init_autocorr(CSOUND *csound, AUTOCORR *p) {
  int32_t   N = p->in->sizes[0], fn;
  for(fn=2; fn < N*2-1; fn*=2);
  if (p->mem.auxp == 0 || p->mem.size < fn*sizeof(MYFLT))
      csound->AuxAlloc(csound, fn*sizeof(MYFLT), &p->mem);
  p->N = N;
  p->FN = fn;
  tabinit(csound, p->out,N);
  return OK;
}

int32_t perf_autocorr(CSOUND *csound, AUTOCORR *p) {
  MYFLT *r = p->out->data;
  MYFLT *buf = (MYFLT *) p->mem.auxp;
  MYFLT *s = p->in->data;
  csound->AutoCorrelation(csound,r,s,p->N,buf,p->FN);
  return OK;
}

typedef struct _fft {
  OPDS h;
  ARRAYDAT *out;
  ARRAYDAT *in, *in2;
  MYFLT *f;
  MYFLT b;
  int32_t n;
  void *setup;
  AUXCH mem;
} FFT;


static uint32_t isPowerOfTwo (uint32_t x) {
  return x != 0  ? !(x & (x - 1)) : 0;
}


int32_t init_rfft(CSOUND *csound, FFT *p) {
  int32_t   N = p->in->sizes[0];
  if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("rfft: only one-dimensional arrays allowed"));
  if (isPowerOfTwo(N)) {
    tabinit(csound, p->out,N);
    p->setup = csound->RealFFT2Setup(csound, N, FFT_FWD);
  }
  else
    tabinit(csound, p->out, N+2);
  return OK;
}

int32_t perf_rfft(CSOUND *csound, FFT *p) {
    int32_t N = p->out->sizes[0];
    memcpy(p->out->data,p->in->data,N*sizeof(MYFLT));
    if (isPowerOfTwo(N)) {
      csound->RealFFT2(csound,p->setup,p->out->data);
    }
    else{
      p->out->data[N] = FL(0.0);
      csound->RealFFTnp2(csound,p->out->data,N);
    }
    return OK;
}

int32_t rfft_i(CSOUND *csound, FFT *p) {
  if (init_rfft(csound,p) == OK)
    return perf_rfft(csound, p);
  else return NOTOK;
}

int32_t init_rifft(CSOUND *csound, FFT *p) {
  int32_t   N = p->in->sizes[0];
  if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("rifft: only one-dimensional arrays allowed"));
  if (isPowerOfTwo(N)) {
    p->setup = csound->RealFFT2Setup(csound, N, FFT_INV);
    tabinit(csound, p->out, N);
  }
  else
    tabinit(csound, p->out, N+2);
  return OK;
}

int32_t perf_rifft(CSOUND *csound, FFT *p) {
    int32_t N = p->in->sizes[0];
    memcpy(p->out->data,p->in->data,N*sizeof(MYFLT));
    if (isPowerOfTwo(N))
      csound->RealFFT2(csound,p->setup,p->out->data);
    else{
      p->out->data[N] = FL(0.0);
      csound->InverseRealFFTnp2(csound,p->out->data,N);
    }
    return OK;
}

int32_t rifft_i(CSOUND *csound, FFT *p) {
  if (init_rifft(csound,p) == OK)
    return perf_rifft(csound, p);
  else return NOTOK;
}

int32_t init_rfftmult(CSOUND *csound, FFT *p) {
    int32_t   N = p->in->sizes[0];
    if (UNLIKELY(N != p->in2->sizes[0]))
      return csound->InitError(csound, "%s", Str("array sizes do not match\n"));
    /*if (isPowerOfTwo(N))*/
    tabinit(csound, p->out, N);
    /* else
       return
         csound->InitError(csound, "%s",
                           Str("non-pow-of-two case not implemented yet\n"));*/
    return OK;
}

int32_t perf_rfftmult(CSOUND *csound, FFT *p) {
    int32_t N = p->out->sizes[0];
    csound->RealFFTMult(csound,p->out->data,p->in->data,p->in2->data,N,1);
    return OK;
}

/* these should have been in the CSOUND struct, but are not */
void csoundComplexFFTnp2(CSOUND *csound, MYFLT *buf, int32_t FFTsize);
void csoundInverseComplexFFTnp2(CSOUND *csound, MYFLT *buf, int32_t FFTsize);

int32_t initialise_fft(CSOUND *csound, FFT *p) {
  int32_t   N2 = p->in->sizes[0];
  if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("fft: only one-dimensional arrays allowed"));
  tabinit(csound,p->out,N2);
  return OK;
}

int32_t perf_fft(CSOUND *csound, FFT *p) {
    int32_t N2 = p->in->sizes[0];
    memcpy(p->out->data,p->in->data,N2*sizeof(MYFLT));
    if (isPowerOfTwo(N2))
      csound->ComplexFFT(csound,p->out->data,N2/2);
    else {
      csoundComplexFFTnp2(csound,p->out->data,N2/2);
    }
    return OK;
}

int32_t fft_i(CSOUND *csound, FFT *p) {
  if (initialise_fft(csound,p) == OK)
    return perf_fft(csound, p);
  else return NOTOK;
}


int32_t init_ifft(CSOUND *csound, FFT *p) {
    int32_t   N2 = p->in->sizes[0];
    if (UNLIKELY(p->in->dimensions > 1))
      return csound->InitError(csound, "%s",
                               Str("fftinv: only one-dimensional arrays allowed"));
    tabinit(csound, p->out, N2);
    return OK;
}

int32_t perf_ifft(CSOUND *csound, FFT *p) {
    int32_t N2 = p->out->sizes[0];
    memcpy(p->out->data,p->in->data,N2*sizeof(MYFLT));
    if (isPowerOfTwo(N2)) {
      csound->InverseComplexFFT(csound,p->out->data,N2/2);
    }
    else {
      csoundInverseComplexFFTnp2(csound,p->out->data,N2/2);
    }
    return OK;
}

int32_t ifft_i(CSOUND *csound, FFT *p) {
    if (LIKELY(init_ifft(csound,p) == OK))
    return perf_ifft(csound, p);
  else return NOTOK;
}

int32_t init_recttopol(CSOUND *csound, FFT *p) {
    int32_t   N = p->in->sizes[0];
    tabinit(csound, p->out, N);
    return OK;
}

int32_t perf_recttopol(CSOUND *csound, FFT *p) {
    IGN(csound);
    int32_t i, end = p->out->sizes[0];
    MYFLT *in, *out, mag, ph;
    in = p->in->data;
    out = p->out->data;
    for (i=2;i<end;i+=2 ) {
      mag = HYPOT(in[i], in[i+1]);
      ph = ATAN2(in[i+1],in[i]);
      out[i] = mag; out[i+1] = ph;
    }
    return OK;
}

int32_t perf_poltorect(CSOUND *csound, FFT *p) {
    IGN(csound);
    int32_t i, end = p->out->sizes[0];
    MYFLT *in, *out, re, im;
    in = p->in->data;
    out = p->out->data;
    for (i=2;i<end;i+=2) {
      re = in[i]*COS(in[i+1]);
      im = in[i]*SIN(in[i+1]);
      out[i] = re; out[i+1] = im;
    }
    return OK;
}

int32_t init_poltorect2(CSOUND *csound, FFT *p) {
    if (LIKELY(p->in2->sizes[0] == p->in->sizes[0])) {
      int32_t   N = p->in2->sizes[0];
      tabinit(csound, p->out, N*2 - 2);
      return OK;
    } else return csound->InitError(csound,
                                    Str("in array sizes do not match: %d and %d\n"),
                                    p->in2->sizes[0],p->in->sizes[0]);
}


int32_t perf_poltorect2(CSOUND *csound, FFT *p) {
    IGN(csound);
    int32_t i,j, end = p->in->sizes[0]-1;
    MYFLT *mags, *phs, *out, re, im;
    mags = p->in->data;
    phs = p->in2->data;
    out = p->out->data;
    for (i=2,j=1;j<end;i+=2, j++) {
      re = mags[j]*COS(phs[j]);
      im = mags[j]*SIN(phs[j]);
      out[i] = re; out[i+1] = im;
    }
    out[0] = mags[0]*COS(phs[0]);
    out[1] = mags[end]*COS(phs[end]);
    return OK;
}

int32_t init_mags(CSOUND *csound, FFT *p) {
    int32_t   N = p->in->sizes[0];
    tabinit(csound, p->out, N/2+1);
    return OK;
}

int32_t perf_mags(CSOUND *csound, FFT *p) {
    IGN(csound);
    int32_t i,j, end = p->out->sizes[0];
    MYFLT *in, *out;
    in = p->in->data;
    out = p->out->data;
    for (i=2,j=1;j<end-1;i+=2,j++)
      out[j] = HYPOT(in[i],in[i+1]);
    out[0] = in[0];
    out[end-1] = in[1];
    return OK;
}

int32_t perf_phs(CSOUND *csound, FFT *p) {
    IGN(csound);
    int32_t i,j, end = p->out->sizes[0];
    MYFLT *in, *out;
    in = p->in->data;
    out = p->out->data;
    for (i=2,j=1;j<end-1;i+=2,j++)
      out[j] = atan2(in[i+1],in[i]);
    return OK;
}

int32_t init_logarray(CSOUND *csound, FFT *p) {
    tabinit(csound, p->out, p->in->sizes[0]);
    if (LIKELY(*((MYFLT *)p->in2)))
      p->b = 1/log(*((MYFLT *)p->in2));
    else
      p->b = FL(1.0);
    return OK;
}

int32_t perf_logarray(CSOUND *csound, FFT *p) {
    IGN(csound);
    int32_t i, end = p->out->sizes[0];
    MYFLT bas = p->b;
    MYFLT *in, *out;
    in = p->in->data;
    out = p->out->data;
    if (LIKELY(bas))
      for (i=0;i<end;i++)
        out[i] = log(in[i])*bas;
    else
      for (i=0;i<end;i++)
        out[i] = log(in[i]);
    return OK;
}

int32_t init_rtoc(CSOUND *csound, FFT *p) {
    int32_t   N = p->in->sizes[0];
    tabinit(csound, p->out, N*2);
    return OK;
}

int32_t perf_rtoc(CSOUND *csound, FFT *p) {
    IGN(csound);
    int32_t i,j, end = p->out->sizes[0];
    MYFLT *in, *out;
    in = p->in->data;
    out = p->out->data;
    for (i=0,j=0;i<end;i+=2,j++) {
      out[i] = in[j];
      out[i+1] = FL(0.0);
    }
    return OK;
}

int32_t rtoc_i(CSOUND *csound, FFT *p) {
    if (LIKELY(init_rtoc(csound,p) == OK))
    return perf_rtoc(csound, p);
  else return NOTOK;
}

int32_t init_ctor(CSOUND *csound, FFT *p) {
    int32_t   N = p->in->sizes[0];
    tabinit(csound, p->out, N/2);
    return OK;
}


int32_t perf_ctor(CSOUND *csound, FFT *p) {
    IGN(csound);
    int32_t i,j, end = p->out->sizes[0];
    MYFLT *in, *out;
    in = p->in->data;
    out = p->out->data;
    for (i=0,j=0;j<end;i+=2,j++)
      out[j] = in[i];
    return OK;
}

int32_t ctor_i(CSOUND *csound, FFT *p) {
    if (LIKELY(init_ctor(csound,p) == OK))
    return perf_ctor(csound, p);
  else return NOTOK;
}


int32_t init_window(CSOUND *csound, FFT *p) {
    int32_t   N = p->in->sizes[0];
    int32_t   i,type = (int32_t) *p->f;
    MYFLT *w;
    tabinit(csound, p->out, N);
    if (p->mem.auxp == 0 || p->mem.size < N*sizeof(MYFLT))
      csound->AuxAlloc(csound, N*sizeof(MYFLT), &p->mem);
    w = (MYFLT *) p->mem.auxp;
    switch(type) {
    case 0:
      for (i=0; i<N; i++) w[i] = 0.54 - 0.46*cos(i*TWOPI/N);
      break;
    case 1:
    default:
      for (i=0; i<N; i++) w[i] = 0.5 - 0.5*cos(i*TWOPI/N);
    }
    return OK;
}

int32_t perf_window(CSOUND *csound, FFT *p) {
    IGN(csound);
    int32_t i,end = p->out->sizes[0], off = *((MYFLT *)p->in2);
    MYFLT *in, *out, *w;
    in = p->in->data;
    out = p->out->data;
    w = (MYFLT *) p->mem.auxp;
    /*while (off < 0) off += end;
     for (i=0;i<end;i++)
      out[(i+off)%end] = in[i]*w[i];*/
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
  void *setup;
  uint32_t  lastframe;
} PVSCEPS;

int32_t pvsceps_init(CSOUND *csound, PVSCEPS *p) {
    int32_t N = p->fin->N;
    if (LIKELY(isPowerOfTwo(N))) {
      p->setup = csound->RealFFT2Setup(csound, N/2, FFT_FWD);
      tabinit(csound, p->out, N/2+1);
    }
    else
      return csound->InitError(csound, "%s",
                               Str("non-pow-of-two case not implemented yet\n"));
    p->lastframe = 0;
    return OK;
}

int32_t pvsceps_perf(CSOUND *csound, PVSCEPS *p) {

    if (p->lastframe < p->fin->framecount) {
      int32_t N = p->fin->N;
      int32_t i, j;
      MYFLT *ceps = p->out->data;
      MYFLT coefs = *p->coefs;
      float *fin = (float *) p->fin->frame.auxp;
      for (i=j=0; i < N; i+=2, j++) {
        ceps[j] = log(fin[i] > 0.0 ? fin[i] : 1e-20);
      }
      ceps[N/2] = fin[N/2];
      csound->RealFFT2(csound, p->setup, ceps);
      if (coefs) {
        // lifter coefs
       for (i=coefs*2; i < N/2; i++) ceps[i] = 0.0;
        ceps[N/2] = 0.0;
      }
      p->lastframe = p->fin->framecount;
    }
    return OK;
}


int32_t init_ceps(CSOUND *csound, FFT *p) {
    int32_t N = p->in->sizes[0]-1;
    if (UNLIKELY(N < 64))
      return csound->InitError(csound, "%s",
                               Str("FFT size too small (min 64 samples)\n"));
    if (LIKELY(isPowerOfTwo(N))) {
      p->setup = csound->RealFFT2Setup(csound, N, FFT_FWD);
      tabinit(csound, p->out, N+1);
    }
    else
      return csound->InitError(csound, "%s",
                               Str("non-pow-of-two case not implemented yet\n"));

    return OK;
}


int32_t perf_ceps(CSOUND *csound, FFT *p) {
    int32_t siz = p->out->sizes[0]-1, i;
    MYFLT *ceps = p->out->data;
    MYFLT coefs = *((MYFLT *)p->in2);
    MYFLT *mags = (MYFLT *) p->in->data;
    for (i=0; i < siz; i++) {
      ceps[i] = log(mags[i] > 0.0 ? mags[i] : 1e-20);
    }
    ceps[siz] = mags[siz];
    csound->RealFFT2(csound, p->setup, ceps);
    if (coefs) {
      // lifter coefs
      for (i=coefs*2; i < siz; i++) ceps[i] = 0.0;
      ceps[siz] = 0.0;
    }
    return OK;
}

int32_t init_iceps(CSOUND *csound, FFT *p) {
    int32_t N = p->in->sizes[0]-1;
    if (LIKELY(isPowerOfTwo(N))) {
      p->setup = csound->RealFFT2Setup(csound, N, FFT_INV);
      tabinit(csound, p->out, N+1);
    }
    else
      return csound->InitError(csound, "%s",
                               Str("non-pow-of-two case not implemented yet\n"));
    N++;
    if (p->mem.auxp == NULL || p->mem.size < N*sizeof(MYFLT))
      csound->AuxAlloc(csound, N*sizeof(MYFLT), &p->mem);
    return OK;
}

int32_t perf_iceps(CSOUND *csound, FFT *p) {
    int32_t siz = p->in->sizes[0]-1, i;
    MYFLT *spec = (MYFLT *)p->mem.auxp;
    MYFLT *out = p->out->data;
    memcpy(spec, p->in->data, siz*sizeof(MYFLT));
    csound->RealFFT2(csound,p->setup,spec);
    for (i=0; i < siz; i++) {
      out[i] = exp(spec[i]);
    }
    out[siz] = spec[siz];       /* Writes outside data allocated */
    return OK;
}

int32_t rows_init(CSOUND *csound, FFT *p) {
    if (p->in->dimensions == 2) {
      int32_t siz = p->in->sizes[1];
      tabinit(csound, p->out, siz);
      return OK;
    }
    else
      return csound->InitError(csound, "%s",
                               Str("in array not 2-dimensional\n"));
}

int32_t rows_perf(CSOUND *csound, FFT *p) {
    int32_t start = *((MYFLT *)p->in2);
    if (LIKELY(start < p->in->sizes[0])) {
      int32_t bytes =  p->in->sizes[1]*sizeof(MYFLT);
      start *= p->in->sizes[1];
      memcpy(p->out->data,p->in->data+start,bytes);
      return OK;
    }
    else return csound->PerfError(csound,  &(p->h),
                                  Str("requested row is out of range\n"));
}

/* Getrow for string arrays */
int32_t rows_perf_S(CSOUND *csound, FFT *p)
{
    ARRAYDAT* dat = p->in;      /* The data in e 2_D array */
    STRINGDAT* mem = (STRINGDAT*)dat->data;
    STRINGDAT* dest = (STRINGDAT*)p->out->data;
    int32_t i;
    int32_t index = (int32_t)(*((MYFLT *)p->in2));
    if (LIKELY(index < p->in->sizes[0])) {
      index = (index * dat->sizes[1]);
      //printf("%d : %d\n", index, dat->sizes[1]);
      mem += index;
      //incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
      //printf("*** mem = %p dst = %p\n", mem, dest);
      for (i = 0; i<p->in->sizes[1]; i++) {
        dat->arrayType->copyValue(csound, dat->arrayType, (void*)dest, (void*)mem);
        //printf("*** copies i=%d: %s -> %s\n", i,(char*)(mem->data),(char*)(dest->data));
        dest +=1;
        mem += 1;
      }
      return OK;
    }
    else return csound->PerfError(csound,  &(p->h),
                                  Str("requested row is out of range\n"));
}

int32_t set_rows_perf_S(CSOUND *csound, FFT *p)
{
    ARRAYDAT* dat = p->in;      /* The data in e 2_D array */
    STRINGDAT* mem = (STRINGDAT*)dat->data;
    STRINGDAT* dest = (STRINGDAT*)p->out->data;
    int32_t i;
    int32_t index = (int32_t)(*((MYFLT *)p->in2));
    if (LIKELY(index < p->in->sizes[0])) {
      index = (index * dat->sizes[1]);
      //printf("%d : %d\n", index, dat->sizes[1]);
      mem += index;
      //incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
      //printf("*** mem = %p dst = %p\n", mem, dest);
      for (i = 0; i<p->in->sizes[1]; i++) {
        dat->arrayType->copyValue(csound, dat->arrayType, (void*)mem, (void*)dest);
        //printf("*** copies i=%d: %s -> %s\n", i,(char*)(mem->data),(char*)(dest->data));
        dest+= 1;
        mem += 1;
      }
      return OK;
    }
    else return csound->PerfError(csound,  &(p->h),
                                  Str("requested row is out of range\n"));
}


int32_t rows_i(CSOUND *csound, FFT *p) {
  if (rows_init(csound,p) == OK) {
   int32_t start = *((MYFLT *)p->in2);
   if (LIKELY(start < p->in->sizes[0])) {
      int32_t bytes =  p->in->sizes[1]*sizeof(MYFLT);
      start *= p->in->sizes[1];
      memcpy(p->out->data,p->in->data+start,bytes);
      return OK;
    }
    else return csound->InitError(csound, "%s",
                                  Str("requested row is out of range\n"));

  }
  else return NOTOK;
}

static inline void tabensure2D(CSOUND *csound, ARRAYDAT *p,
                               int32_t rows, int32_t columns)
{
    if (p->data==NULL || p->dimensions == 0 ||
        (p->dimensions==2 && (p->sizes[0] < rows || p->sizes[1] < columns))) {
      size_t ss;
      if (p->data == NULL) {
        CS_VARIABLE* var = p->arrayType->createVariable(csound, NULL);
        p->arrayMemberSize = var->memBlockSize;
      }
      ss = p->arrayMemberSize*rows*columns;
      if (p->data==NULL) {
        p->data = (MYFLT*)csound->Calloc(csound, ss);
        p->dimensions = 2;
        p->sizes = (int32_t*)csound->Malloc(csound, sizeof(int32_t)*2);
      }
      else p->data = (MYFLT*) csound->ReAlloc(csound, p->data, ss);
      p->sizes[0] = rows;  p->sizes[1] = columns;
    }
}


int32_t set_rows_init(CSOUND *csound, FFT *p) {
    int32_t sizs = p->in->sizes[0];
    int32_t row = *((MYFLT *)p->in2);
    tabensure2D(csound, p->out, row+1, sizs);
    return OK;
}


int32_t set_rows_init_S(CSOUND *csound, FFT *p) {
    if (set_rows_init(csound, p)==0)
      return set_rows_perf_S(csound, p);
    return NOTOK;
}

int32_t rows_init_S(CSOUND *csound, FFT *p) {
    if (rows_init(csound, p)==0)
      return rows_perf_S(csound, p);
    return NOTOK;
}

int32_t cols_init(CSOUND *csound, FFT *p) {
    if (LIKELY(p->in->dimensions == 2)) {
      int32_t siz = p->in->sizes[0];
      tabinit(csound, p->out, siz);
      return OK;
    }
    else
      return csound->InitError(csound, "%s",
                               Str("in array not 2-dimensional\n"));
}

int32_t cols_perf(CSOUND *csound, FFT *p) {
    int32_t start = *((MYFLT *)p->in2);

    if (LIKELY(start < p->in->sizes[1])) {
        int32_t j,i,collen =  p->in->sizes[1], len = p->in->sizes[0];
        for (j=0,i=start; j < len; i+=collen, j++) {
            p->out->data[j] = p->in->data[i];
        }
        return OK;
    }
    else return csound->PerfError(csound,  &(p->h),
                                  Str("requested col is out of range\n"));
}

int32_t cols_i(CSOUND *csound, FFT *p) {
  if (cols_init(csound, p) == OK) {
  int32_t start = *((MYFLT *)p->in2);
    if (LIKELY(start < p->in->sizes[1])) {
        int32_t j,i,collen =  p->in->sizes[1], len = p->in->sizes[0];
        for (j=0,i=start; j < len; i+=collen, j++) {
            p->out->data[j] = p->in->data[i];
        }
        return OK;
    }
    else return csound->InitError(csound, "%s",
                                  Str("requested col is out of range\n"));
  }
  else return NOTOK;
}

int32_t cols_perf_S(CSOUND *csound, FFT *p) {
    ARRAYDAT* dat = p->in;      /* The data in e 2_D array */
    STRINGDAT* mem = (STRINGDAT*)dat->data;
    STRINGDAT* dest = (STRINGDAT*)p->out->data;
    int32_t i;
    int32_t index = (int32_t)(*((MYFLT *)p->in2));
    if (LIKELY(index < p->in->sizes[0])) {
      //index = (index * dat->sizes[1]);
      //printf("%d : %d\n", index, dat->sizes[1]);
      mem += index;
      //dest += index;
      //incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
      //printf("*** mem = %p dst = %p\n", mem, dest);
      for (i = 0; i<p->in->sizes[0]; i++) {
        dat->arrayType->copyValue(csound, dat->arrayType, (void*)dest, (void*)mem);
        //printf("*** copies i=%d: %s -> %s\n", i,(char*)(dest->data),(char*)(mem->data));
        dest+= 1;
        mem += p->in->sizes[1];
      }
      return OK;
    }
    else return csound->PerfError(csound,  &(p->h),
                                  Str("requested col is out of range\n"));
}

int32_t set_rows_perf(CSOUND *csound, FFT *p) {
    int32_t start = *((MYFLT *)p->in2);
    if (UNLIKELY(start < 0 || start >= p->out->sizes[0]))
      return csound->PerfError(csound, &(p->h),
                                 Str("Error: index out of range\n"));
    int32_t bytes =  p->in->sizes[0]*sizeof(MYFLT);
    start *= p->out->sizes[1];
    memcpy(p->out->data+start,p->in->data,bytes);
    return OK;
}

int32_t set_rows_i(CSOUND *csound, FFT *p) {
  int32_t start = *((MYFLT *)p->in2);
  set_rows_init(csound, p);
  if (UNLIKELY(start < 0 || start >= p->out->sizes[0]))
        return csound->InitError(csound, "%s",
                                 Str("Error: index out of range\n"));
  int32_t bytes =  p->in->sizes[0]*sizeof(MYFLT);
  start *= p->out->sizes[1];
  memcpy(p->out->data+start,p->in->data,bytes);
  return OK;
}


int32_t set_cols_init(CSOUND *csound, FFT *p) {
    int32_t siz = p->in->sizes[0];
    int32_t col = *((MYFLT *)p->in2);
    tabensure2D(csound, p->out, siz, col+1);
    return OK;
}

int32_t set_cols_perf(CSOUND *csound, FFT *p) {

    int32_t start = *((MYFLT *)p->in2);

    if (UNLIKELY(start < 0 || start >= p->out->sizes[1]))
      return csound->PerfError(csound, &(p->h),
                                 Str("Error: index out of range\n"));
    if (UNLIKELY(p->in->dimensions != 1 || p->in->sizes[0]<p->out->sizes[0]))
      return csound->PerfError(csound, &(p->h),
                                 Str("Error: New column too short\n"));


    int32_t j,i,row = p->out->sizes[1], col = p->out->sizes[0];
    for (j=0,i=start; j < col; i+=row, j++)
        p->out->data[i] = p->in->data[j];
    return OK;
}

int32_t set_cols_i(CSOUND *csound, FFT *p) {
    int32_t start = *((MYFLT *)p->in2);
    set_cols_init(csound,p);
    if (UNLIKELY(start < 0 || start >= p->out->sizes[1]))
        return csound->InitError(csound, "%s",
                                 Str("Error: index out of range\n"));
    if (UNLIKELY(p->in->dimensions != 1 || p->in->sizes[0]<p->out->sizes[0]))
      return csound->InitError(csound, "%s",
                                 Str("Error: New column too short\n"));
    int32_t j,i,row = p->out->sizes[1], col = p->out->sizes[0];
    for (j=0,i=start; j < col; i+=row, j++)
        p->out->data[i] = p->in->data[j];
    return OK;
}

int32_t set_cols_perf_S(CSOUND *csound, FFT *p) {
    ARRAYDAT* dat = p->in;      /* The data in 2_D array */
    STRINGDAT* mem = (STRINGDAT*)dat->data;
    STRINGDAT* dest = (STRINGDAT*)p->out->data;
    int32_t i;
    int32_t index = (int32_t)(*((MYFLT *)p->in2));
    if (LIKELY(index < p->in->sizes[0])) {
      index = (index * dat->sizes[1]);
      //printf("%d : %d\n", index, dat->sizes[1]);
      mem += index;
      //incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
      //printf("*** mem = %p dst = %p\n", mem, dest);
      for (i = 0; i<p->in->sizes[0]; i++) {
        dat->arrayType->copyValue(csound, dat->arrayType, (void*)mem, (void*)dest );
        //printf("*** copies i=%d: %s -> %s\n", i,(char*)(mem->data),(char*)(dest->data));
        dest += p->in->sizes[1];
        mem  += 1;
      }
      return OK;
    }
    else return csound->PerfError(csound,  &(p->h),
                                  Str("requested col is out of range\n"));    
}

int32_t set_cols_init_S(CSOUND *csound, FFT *p) {
    if (set_cols_i(csound,p)==0)
      return set_cols_perf_S(csound, p);
    return NOTOK;
}

int32_t cols_init_S(CSOUND *csound, FFT *p) {
    if (cols_init(csound,p)==0)
      return cols_perf_S(csound, p);
    return NOTOK;
}

int32_t shiftin_init(CSOUND *csound, FFT *p) {
    int32_t sizs = CS_KSMPS;
    if(p->out->sizes[0] < sizs)
       tabinit(csound, p->out, sizs);
    p->n = 0;
    return OK;
}

int32_t shiftin_perf(CSOUND *csound, FFT *p) {
   IGN(csound);
    uint32_t  siz =  p->out->sizes[0], n = p->n;
    MYFLT *in = ((MYFLT *) p->in);
    if (n + CS_KSMPS < siz) {
      memcpy(p->out->data+n,in,CS_KSMPS*sizeof(MYFLT));
    }
    else {
      int32_t num = siz - n;
      memcpy(p->out->data+n,in,num*sizeof(MYFLT));
      memcpy(p->out->data,in+num,(CS_KSMPS-num)*sizeof(MYFLT));
    }
    p->n = (n + CS_KSMPS)%siz;
    return OK;
}


int32_t shiftout_init(CSOUND *csound, FFT *p) {
    int32_t siz = p->in->sizes[0];
    p->n = ((int32_t)*((MYFLT *)p->in2) % siz);
    if (UNLIKELY((uint32_t) siz < CS_KSMPS))
      return csound->InitError(csound, "%s", Str("input array too small\n"));
    return OK;
}

int32_t shiftout_perf(CSOUND *csound, FFT *p) {
    IGN(csound);
    uint32_t siz =  p->in->sizes[0], n = p->n;
    MYFLT *out = ((MYFLT *) p->out);

    if (n + CS_KSMPS < siz) {
      memcpy(out,p->in->data+n,CS_KSMPS*sizeof(MYFLT));
    }
    else {
      int32_t num = siz - n;
      memcpy(out,p->in->data+n,num*sizeof(MYFLT));
      memcpy(out+num,p->in->data,(CS_KSMPS-num)*sizeof(MYFLT));
    }
    p->n = (n + CS_KSMPS)%siz;
    return OK;
}

int32_t scalarset(CSOUND *csound, TABCOPY *p) {
    IGN(csound);
    uint32_t siz = 0 , dim = p->tab->dimensions, i;
    MYFLT val = *p->kfn;
    for (i=0; i < dim; i++)
      siz += p->tab->sizes[i];
    for (i=0; i < siz; i++)
      p->tab->data[i] = val;
    return OK;
}

int32_t arrayass(CSOUND *csound, TABCOPY *p)
{
    IGN(csound);
    uint32_t siz = 0 , dim = p->tab->dimensions, i;
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t span = (p->tab->arrayMemberSize)/sizeof(MYFLT);
    MYFLT *val = p->kfn;

    for (i=0; i < dim; i++)
      siz += p->tab->sizes[i];
    for (i=0; i < siz; i++) {
      int32_t pp = i*span;
      for (n=0; n<offset; n++)
        p->tab->data[pp+n] = FL(0.0);
      for (n=offset; n<nsmps-early; n++)
        p->tab->data[pp+n] = val[n];
      for (n=nsmps-early; n<nsmps; n++)
        p->tab->data[pp+n] = FL(0.0);
    }
    return OK;
}

int32_t unwrap(CSOUND *csound, FFT *p) {
    IGN(csound);
    int32_t i,siz = p->in->sizes[0];
    MYFLT *phs = p->out->data;
    for (i=0; i < siz; i++) {
      while (phs[i] >= PI) phs[i] -= TWOPI;
      while (phs[i] < -PI) phs[i] += TWOPI;
    }
    return OK;
}

void *csoundDCTSetup(CSOUND *csound,
                     int32_t FFTsize, int32_t d);
void csoundDCT(CSOUND *csound,
               void *p, MYFLT *sig);

int32_t init_dct(CSOUND *csound, FFT *p) {
   int32_t   N = p->in->sizes[0];
   if (LIKELY(isPowerOfTwo(N))) {
     if (UNLIKELY(p->in->dimensions > 1))
    return csound->InitError(csound, "%s",
                             Str("dct: only one-dimensional arrays allowed"));
    tabinit(csound, p->out, N);
    p->setup =  csoundDCTSetup(csound,N,FFT_FWD);
    return OK;
   }
   else return
          csound->InitError(csound, "%s",
                            Str("dct: non-pow-of-two sizes not yet implemented"));
}

int32_t kdct(CSOUND *csound, FFT *p) {
    // FIXME: IF N changes value do we need a check
    int32_t N = p->out->sizes[0];
    memcpy(p->out->data,p->in->data,N*sizeof(MYFLT));
    csoundDCT(csound,p->setup,p->out->data);
    return OK;
}

int32_t dct(CSOUND *csound, FFT *p) {
    if (!init_dct(csound,p)) {
      kdct(csound,p);
      return OK;
      } else return NOTOK;
}

int32_t init_dctinv(CSOUND *csound, FFT *p) {
   int32_t   N = p->in->sizes[0];
   if (LIKELY(isPowerOfTwo(N))) {
     if (UNLIKELY(p->in->dimensions > 1))
       return csound->InitError(csound, "%s",
                                Str("dctinv: only one-dimensional arrays allowed"));
     tabinit(csound, p->out, N);
     p->setup =  csoundDCTSetup(csound,N,FFT_INV);
     return OK;
   }
   else
     return
       csound->InitError(csound, "%s",
                         Str("dctinv: non-pow-of-two sizes not yet implemented"));
}

int32_t dctinv(CSOUND *csound, FFT *p) {
    if (LIKELY(!init_dctinv(csound,p))) {
      kdct(csound,p);
      return OK;
      } else return NOTOK;
}

int32_t perf_pows(CSOUND *csound, FFT *p) {
    IGN(csound);
    int32_t i,j, end = p->out->sizes[0];
    MYFLT *in, *out;
    in = p->in->data;
    out = p->out->data;
    for (i=2,j=1;j<end-1;i+=2,j++)
      out[j] = in[i]*in[i]+in[i+1]*in[i+1];
    out[0] = in[0]*in[0];
    out[end-1] = in[1]*in[1];
    return OK;
}

typedef struct _MFB {
  OPDS h;
  ARRAYDAT *out;
  ARRAYDAT *in;
  MYFLT *low;
  MYFLT *up;
  MYFLT *len;
  AUXCH  bins;
} MFB;

static inline MYFLT f2mel(MYFLT f) {
    return 1125.*log(1.+f/700.);
}

static inline int32_t mel2bin(MYFLT m, int32_t N, MYFLT sr) {
    MYFLT f = 700.*(exp(m/1125.) - 1.);
    return  (int32_t)(f/(sr/(2*N)));
}

int32_t mfb_init(CSOUND *csound, MFB *p) {
    int32_t   L = *p->len;
    int32_t N = p->in->sizes[0];
    if (LIKELY(L < N)) {
      tabinit(csound, p->out, L);
    }
    else
      return csound->InitError(csound, "%s",
                       Str("mfb: filter bank size exceeds input array length"));
    if (p->bins.auxp == NULL || p->bins.size < (L+2)*sizeof(int32_t))
      csound->AuxAlloc(csound, (L+2)*sizeof(MYFLT), &p->bins);
    return OK;
}

int32_t mfb(CSOUND *csound, MFB *p) {
    /* FIXME: Init cals tabinit but not checked in erf? */
    int32_t i,j;
    int32_t *bin = (int32_t *) p->bins.auxp;
    MYFLT start,max,end;
    MYFLT g = FL(0.0), incr, decr;
    int32_t L = p->out->sizes[0];
    int32_t N = p->in->sizes[0];
    MYFLT sum = FL(0.0);
    MYFLT *out = p->out->data;
    MYFLT *in = p->in->data;
    MYFLT sr = csound->GetSr(csound);

    start = f2mel(*p->low);
    end = f2mel(*p->up);
    incr = (end-start)/(L+1);



    for (i=0;i<L+2;i++) {
      bin[i] = (int32_t) mel2bin(start,N-1,sr);

      if (bin[i] > N) bin[i] = N;
      start += incr;
    }

    for (i=0; i < L; i++) {
      start = bin[i];
      max = bin[i+1];
      end = bin[i+2];
      incr =  1.0/(max - start);
      decr =  1.0/(end - max);
      for (j=start; j < max; j++) {
        sum += in[j]*g;
        g += incr;
      }
      g = FL(1.0);
      for (j=max; j < end; j++) {
        sum += in[j]*g;
        g -= decr;
      }
      out[i] = sum/(end - start);

      g = FL(0.0);
      sum = FL(0.0);
    }

    return OK;
}

int32_t mfbi(CSOUND *csound, MFB *p) {
    if (LIKELY(mfb_init(csound,p) == OK))
    return mfb(csound,p);
    else return NOTOK;
}

typedef struct _centr{
  OPDS h;
  MYFLT *out;
  ARRAYDAT *in;
} CENTR;

int32_t array_centroid(CSOUND *csound, CENTR *p) {

    MYFLT *in = p->in->data,a=FL(0.0),b=FL(0.0);
    int32_t NP1 = p->in->sizes[0];
    MYFLT f = csound->GetSr(csound)/(2*(NP1 - 1)),cf;
    int32_t i;
    cf = f*FL(0.5);
    for (i=0; i < NP1-1; i++, cf+=f) {
      a += in[i];
      b += in[i]*cf;
    }
    *p->out = a > FL(0.0) ? b/a : FL(0.0);
    return OK;
}

typedef struct _inout {
  OPDS H;
  MYFLT *out, *in;
} INOUT;

int32_t nxtpow2(CSOUND *csound, INOUT *p) {
    IGN(csound);
    int32_t inval = (int32_t)*p->in;
    int32_t powtwo = 2;
    while (powtwo < inval) powtwo *= 2;
    *p->out = powtwo;
    return OK;
}


typedef struct interl{
  OPDS h;
  ARRAYDAT *a;
  ARRAYDAT *b;
  ARRAYDAT *c;
} INTERL;


int32_t interleave_i (CSOUND *csound, INTERL *p) {
    if(p->b->dimensions == 1 &&
       p->c->dimensions == 1 &&
       p->b->sizes[0] == p->c->sizes[0]) {
      int32_t len = p->b->sizes[0], i,j;
      tabinit(csound, p->a, len*2);
      for(i = 0, j = 0; i < len; i++,j+=2) {
        p->a->data[j] =  p->b->data[i];
        p->a->data[j+1] = p->c->data[i];
      }
      return OK;
    }
    return csound->InitError(csound, Str("array inputs not in correct format\n"));
}

int32_t interleave_perf (CSOUND *csound, INTERL *p) {
    int32_t len = p->b->sizes[0], i,j;
    tabcheck(csound, p->a, len*2, &(p->h));
    for(i = 0, j = 0; i < len; i++,j+=2) {
      p->a->data[j] =  p->b->data[i];
      p->a->data[j+1] = p->c->data[i];
    }
    return OK;
}

int32_t deinterleave_i (CSOUND *csound, INTERL *p) {
  if(p->c->dimensions == 1) {
    int32_t len = p->c->sizes[0]/2, i,j;
    tabinit(csound, p->a, len);
    tabinit(csound, p->b, len);
    for(i = 0, j = 0; i < len; i++,j+=2) {
      p->a->data[i] =  p->c->data[j];
      p->b->data[i] = p->c->data[j+1];
    }
    return OK;
  }
  return csound->InitError(csound, Str("array inputs not in correct format\n"));
}

int32_t deinterleave_perf (CSOUND *csound, INTERL *p) {
    int32_t len = p->c->sizes[0]/2, i,j;
    tabcheck(csound, p->a, len, &(p->h));
    tabcheck(csound, p->b, len, &(p->h));
    for(i = 0, j = 0; i < len; i++,j+=2) {
      p->a->data[i] =  p->c->data[j];
      p->b->data[i] = p->c->data[j+1];
    }
    return OK;
}

static int32 taninv2_A(CSOUND* csound, TABARITH* p)
{
    ARRAYDAT* ans = p->ans;
    ARRAYDAT* aa = p->left;
    ARRAYDAT* bb = p->right;
    int i, j, k;
    if (csound->mode == 1 && tabarithset(csound, p)!=OK)
                             return NOTOK;
    k = 0;
    for (i=0; i<ans->dimensions; i++) {
      for (j=0; j<aa->sizes[i]; j++) {
        ans->data[k] = ATAN2(aa->data[k], bb->data[k]);
        k++;
      }
    }
    return OK;
}

static int32 taninv2_Aa(CSOUND* csound, TABARITH* p)
{
    ARRAYDAT* ans = p->ans;
    ARRAYDAT* aa = p->left;
    ARRAYDAT* bb = p->right;
    int i, j, k;
    uint32_t m;
    k = 0;
    for (i=0; i<ans->dimensions; i++) {
      for (j=0; j<aa->sizes[i]; j++)
        for (m=0; m<csound->ksmps; m++) {
          ans->data[k] = ATAN2(aa->data[k], bb->data[k]);
          k++;
        }
    }
    return OK;
}


// reverse, scramble, mirror, stutter, rotate, ...
// jpff: stutter is an interesting one (very musical). It basically
//          randomly repeats (holds) values based on a probability parameter

static OENTRY arrayvars_localops[] =
  {
    { "nxtpow2", sizeof(INOUT), 0, 1, "i", "i", (SUBR)nxtpow2, NULL, NULL, NULL},
    { "init.i", sizeof(ARRAYINIT), 0, 1, "i[]", "m", (SUBR)array_init, NULL, NULL, NULL},
    { "init.k", sizeof(ARRAYINIT), 0, 1, "k[]", "m", (SUBR)array_init, NULL, NULL, NULL},
    { "init.a", sizeof(ARRAYINIT), 0, 1, "a[]", "m", (SUBR)array_init, NULL, NULL, NULL},
    { "init.S", sizeof(ARRAYINIT), 0, 1, "S[]", "m", (SUBR)array_init, NULL, NULL, NULL},
    { "init.0", sizeof(ARRAYINIT), 0, 1, ".[]", "m", (SUBR)array_init, NULL, NULL, NULL},
    { "fillarray.k", sizeof(TABFILL), 0, 1, "k[]", "m", (SUBR)tabfill, NULL, NULL, NULL},
    { "fillarray.i", sizeof(TABFILL), 0, 1, "i[]", "m", (SUBR)tabfill, NULL, NULL, NULL},
    { "fillarray.s", sizeof(TABFILL), 0, 1, "S[]", "W", (SUBR)tabfill, NULL, NULL, NULL},
    { "fillarray.K", sizeof(TABFILL), 0, 2, "k[]", "z", NULL, (SUBR)tabfill, NULL, NULL},
    { "fillarray.f", sizeof(TABFILLF), 0, 1, "k[]", "S", (SUBR)tabfillf, NULL, NULL, NULL},
    { "fillarray.F", sizeof(TABFILLF), 0, 1, "i[]", "S", (SUBR)tabfillf, NULL, NULL, NULL},
    { "string2array.S", sizeof(TABFILLF), 0, 1, "i[]", "S", (SUBR)tabsfill, NULL, NULL, NULL},
    { "string2array.s", sizeof(TABFILLF), 0, 1, "k[]", "S", (SUBR)tabsfill, NULL, NULL, NULL},
    { "array.k", sizeof(TABFILL), _QQ, 1, "k[]", "m", (SUBR)tabfill, NULL, NULL, NULL},
    { "array.i", sizeof(TABFILL), _QQ, 1, "i[]", "m", (SUBR)tabfill, NULL, NULL, NULL},
    { "##array_init", sizeof(ARRAY_SET), 0, 1, "", ".[].m", (SUBR)array_set, NULL, NULL, NULL},
    { "##array_set.k", sizeof(ARRAY_SET), 0, 2, "", "k[]km", NULL,(SUBR)array_set, NULL, NULL},
    { "##array_set.a", sizeof(ARRAY_SET), 0, 2, "", "a[]am", NULL, (SUBR)array_set, NULL, NULL},
    // VL: 11.2.22 I think array set S needs to be added running at thread 3 for parser3
    { "##array_set.S", sizeof(ARRAY_SET), 0, 3, "", "S[].m", (SUBR)array_set , (SUBR)array_set, NULL, NULL},
    { "##array_set.i", sizeof(ARRAY_SET), 0, 1, "", ".[].m", (SUBR)array_set, NULL, NULL, NULL},
    { "##array_set.e", sizeof(ARRAY_SET), 0, 1, "", "i[].z", (SUBR)array_err, NULL, NULL, NULL},
    { "##array_set.x", sizeof(ARRAY_SET), 0, 2, "", ".[].z", NULL, (SUBR)array_set, NULL, NULL},
    { "##array_get.k", sizeof(ARRAY_GET), 0, 2, "k", "k[]m", NULL,(SUBR)array_get, NULL, NULL},
    { "##array_get.a", sizeof(ARRAY_GET), 0, 2, "a", "a[]m",NULL, (SUBR)array_get, NULL, NULL},
    { "##array_get.x", sizeof(ARRAY_GET), 0, 1, ".", ".[]m",(SUBR)array_get, NULL, NULL, NULL},
    { "##array_get.K", sizeof(ARRAY_GET), 0, 2, ".", ".[]z",NULL, (SUBR)array_get, NULL, NULL},
    { "i.Ai", sizeof(ARRAY_GET),0, 1,      "i",    "k[]m", (SUBR)array_get, NULL, NULL, NULL},
    { "i.Ak", sizeof(ARRAY_GET),0, 1,      "i",    "k[]z", (SUBR)array_get, NULL, NULL, NULL},
    /* ******************************************** */
    {"##add.[s]", sizeof(TABARITH), 0, 3, "a[]", "a[]a[]",
     (SUBR)tabarithset, (SUBR)tabaadd, NULL, NULL},
    {"##add.[]", sizeof(TABARITH), 0, 3, "k[]", "k[]k[]",
     (SUBR)tabarithset, (SUBR)tabadd, NULL, NULL},
    {"##add.[i]", sizeof(TABARITH), 0, 1, "i[]", "i[]i[]",
     (SUBR)tabaddi, NULL, NULL, NULL},
    /* ******************************************** */
    {"##sub.[a]", sizeof(TABARITH), 0, 3, "a[]", "a[]a[]",
     (SUBR)tabarithset, (SUBR)tabasub, NULL, NULL},
    {"##sub.[]", sizeof(TABARITH), 0, 3, "k[]", "k[]k[]",
     (SUBR)tabarithset, (SUBR)tabsub, NULL, NULL},
    {"##sub.[i]", sizeof(TABARITH), 0, 1, "i[]", "i[]i[]",
     (SUBR)tabsubi, NULL, NULL, NULL},
    //    {"##neg.[]",  sizeof(TABARITH), 0, 3, "k[]", "k[]",
    //                                         (SUBR)tabarithset1, (SUBR)tabneg},
    {"##mul.[a]", sizeof(TABARITH), 0, 3, "a[]", "a[]a[]",
    (SUBR)tabarithset,(SUBR)tabamul, NULL, NULL},
    {"##mul.[]", sizeof(TABARITH), 0, 3, "k[]", "k[]k[]",
     (SUBR)tabarithset,(SUBR)tabmult, NULL, NULL},
    {"##mul.[i]", sizeof(TABARITH), 0, 1, "i[]", "i[]i[]",
     (SUBR)tabmulti, NULL, NULL, NULL},
    {"##div.[a]", sizeof(TABARITH), 0, 3, "a[]", "a[]a[]",
     (SUBR)tabarithset, (SUBR)tabadiv, NULL, NULL},
    {"##div.[]",  sizeof(TABARITH), 0, 3, "k[]", "k[]k[]",
     (SUBR)tabarithset,(SUBR)tabdiv, NULL, NULL},
    {"##div.[i]",  sizeof(TABARITH), 0, 1, "i[]", "i[]i[]",
     (SUBR)tabdivi, NULL, NULL, NULL},
    {"##rem.[]",  sizeof(TABARITH), 0, 3, "k[]", "k[]k[]",
     (SUBR)tabarithset, (SUBR)tabrem, NULL, NULL},
    {"##rem.[i]",  sizeof(TABARITH), 0, 1, "i[]", "i[]i[]", (SUBR)tabremi, NULL, NULL, NULL},
    {"##add.[i", sizeof(TABARITH1), 0, 3, "k[]", "k[]i",
     (SUBR)tabarithset1, (SUBR)tabaiadd, NULL, NULL},
    {"##add.i[", sizeof(TABARITH2), 0, 3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiaadd, NULL, NULL},
    {"##add.[p", sizeof(TABARITH1), 0, 1, "i[]", "i[]i", (SUBR)tabaiaddi, NULL, NULL, NULL},
    {"##add.p[", sizeof(TABARITH2), 0, 1, "i[]", "ii[]", (SUBR)tabiaaddi, NULL, NULL, NULL},
    {"##add.k[a[", sizeof(TABARITH), 0, 3, "a[]", "k[]a[]",
     (SUBR)tabarithset, (SUBR)tabkrardd, NULL, NULL},
    {"##add.a[k[", sizeof(TABARITH), 0, 3, "a[]", "a[]k[]",
     (SUBR)tabarithset, (SUBR)tabarkrdd, NULL, NULL},
    {"##addin.[i", sizeof(TABARITHIN1), 0, 1, "i[]", "i", (SUBR)addinAA, NULL , NULL, NULL},
    {"##addin.[k", sizeof(TABARITHIN1), 0, 2, "k[]", "i", NULL, (SUBR)addinAA, NULL, NULL},
    {"##addin.[", sizeof(TABARITHIN), 0, 1, "i[]", "i[]",  (SUBR)tabaddinkk, NULL, NULL, NULL},
    {"##addin.[K", sizeof(TABARITHIN), 0, 2, "k[]", "k[]", NULL, (SUBR)tabaddinkk, NULL, NULL},
    {"##addin.[a", sizeof(TABARITHIN), 0, 2, "a[]", "a[]", NULL, (SUBR)tabaaddin, NULL, NULL},
    {"##addin.[a", sizeof(TABARITHIN), 0, 2, "a[]", "a[]", NULL, (SUBR)tabaaddin, NULL, NULL},
    {"##addin.[ak", sizeof(TABARITHIN), 0, 2, "a[]", "k[]", NULL, (SUBR)tabarkrddin, NULL, NULL},
    {"##addin.[aks", sizeof(TABARITHIN), 0, 2, "a[]", "k", NULL, (SUBR)tabakaddin, NULL, NULL},
    {"##sub.[i", sizeof(TABARITH1), 0, 3, "k[]", "k[]i",
     (SUBR)tabarithset1, (SUBR)tabaisub, NULL, NULL},
    {"##sub.i[", sizeof(TABARITH2), 0, 3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiasub, NULL, NULL},
    {"##subin.[i", sizeof(TABARITHIN1), 0, 1, "i[]", "i", (SUBR)subinAA, NULL, NULL, NULL},
    {"##subin.[", sizeof(TABARITHIN), 0, 1, "i[]", "i[]",  (SUBR)tabsubinkk, NULL, NULL, NULL},
    {"##subin.[K", sizeof(TABARITHIN), 0, 2, "k[]", "k[]", NULL, (SUBR)tabsubinkk, NULL, NULL},
    {"##subin.[k", sizeof(TABARITHIN1), 0, 2, "k[]", "k", NULL, (SUBR)subinAA, NULL, NULL},
    {"##subin.[a", sizeof(TABARITHIN), 0, 2, "a[]", "a[]", NULL, (SUBR)tabaasubin, NULL, NULL},
    {"##subin.[ak", sizeof(TABARITHIN), 0, 2, "a[]", "k[]", NULL, (SUBR)tabarkrsbin, NULL, NULL},
    {"##subdin.[aks", sizeof(TABARITHIN), 0, 2, "a[]", "k", NULL, (SUBR)tabaksubin, NULL, NULL},
    {"##sub.[p", sizeof(TABARITH1), 0, 1, "i[]", "i[]i", (SUBR)tabaisubi, NULL, NULL, NULL},
    {"##sub.p[", sizeof(TABARITH2), 0, 1, "i[]", "ii[]", (SUBR)tabiasubi, NULL, NULL, NULL},
    {"##sub.k[a[", sizeof(TABARITH), 0, 3, "a[]", "k[]a[]",
     (SUBR)tabarithset, (SUBR)tabkrarsb, NULL, NULL},
    {"##sub.a[k[", sizeof(TABARITH), 0, 3, "a[]", "a[]k[]",
     (SUBR)tabarithset, (SUBR)tabarkrsb, NULL, NULL},
    {"##mul.[i", sizeof(TABARITH1), 0, 3, "k[]", "k[]i",
     (SUBR)tabarithset1, (SUBR)tabaimult, NULL, NULL},
    {"##mul.i[", sizeof(TABARITH2), 0, 3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiamult, NULL, NULL},
    {"##mul.[p", sizeof(TABARITH1), 0, 1, "i[]", "i[]i", (SUBR)tabaimulti, NULL, NULL, NULL},
    {"##mul.p[", sizeof(TABARITH2), 0, 1, "i[]", "ii[]",  (SUBR)tabiamulti, NULL, NULL, NULL},
    {"##mul.k[a[", sizeof(TABARITH), 0, 3, "a[]", "k[]a[]",
     (SUBR)tabarithset, (SUBR)tabkrarml, NULL, NULL},
    {"##mul.a[k[", sizeof(TABARITH), 0, 3, "a[]", "a[]k[]",
     (SUBR)tabarithset, (SUBR)tabarkrml, NULL, NULL},
    {"##div.[i",  sizeof(TABARITH1), 0, 3, "k[]", "k[]i",
     (SUBR)tabarithset1, (SUBR)tabaidiv, NULL, NULL},
    {"##div.i[",  sizeof(TABARITH2), 0, 3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiadiv, NULL, NULL},
    {"##div.[p",  sizeof(TABARITH1), 0, 1, "i[]", "i[]i", (SUBR)tabaidivi, NULL, NULL, NULL},
    {"##div.p[",  sizeof(TABARITH2), 0, 1, "i[]", "ii[]", (SUBR)tabiadivi, NULL, NULL, NULL},
    {"##div.k[a[", sizeof(TABARITH), 0, 3, "a[]", "k[]a[]",
     (SUBR)tabarithset, (SUBR)tabkrardv, NULL, NULL},
    {"##div.a[k[", sizeof(TABARITH), 0, 3, "a[]", "a[]k[]",
     (SUBR)tabarithset, (SUBR)tabarkrdv, NULL, NULL},
    {"##rem.[i",  sizeof(TABARITH1),0,  3, "k[]", "k[]i",
     (SUBR)tabarithset1, (SUBR)tabairem, NULL, NULL},
    {"##rem.i[",  sizeof(TABARITH2),0,  3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiarem, NULL, NULL},
    {"##rem.[p",  sizeof(TABARITH1),0,  1, "i[]", "i[]i", (SUBR)tabairemi, NULL, NULL, NULL},
    {"##rem.p[",  sizeof(TABARITH2),0,  1, "i[]", "ii[]", (SUBR)tabiaremi, NULL, NULL, NULL},
    {"##rem.a[k[", sizeof(TABARITH), 0, 3, "a[]", "a[]k[]",
     (SUBR)tabarithset, (SUBR)tabarkrmd, NULL, NULL},
    {"##re.k[a[", sizeof(TABARITH), 0, 3, "a[]", "k[]a[]",
     (SUBR)tabarithset, (SUBR)tabkrarmd, NULL, NULL},
    {"##add.[k", sizeof(TABARITH1), 0, 3, "k[]", "i[]k",
     (SUBR)tabarithset1, (SUBR)tabaiadd, NULL, NULL},
    {"##add.[k", sizeof(TABARITH1), 0, 3, "k[]", "k[]k",
     (SUBR)tabarithset1, (SUBR)tabaiadd, NULL, NULL},
    {"##add.k[", sizeof(TABARITH2), 0, 3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiaadd, NULL, NULL},
    {"##add.k[", sizeof(TABARITH2), 0, 3, "k[]", "kk[]",
     (SUBR)tabarithset2, (SUBR)tabiaadd, NULL, NULL},
    {"##add.[ak", sizeof(TABARITH1), 0, 3, "a[]", "a[]k",
     (SUBR)tabarithset1, (SUBR)tabakadd, NULL, NULL},
    {"##add.k[a", sizeof(TABARITH2), 0, 3, "a[]", "ka[]",
     (SUBR)tabarithset2, (SUBR)tabkaadd, NULL, NULL},
    {"##add.aa[", sizeof(TABARITH1), 0, 3, "a[]", "aa[]",
     (SUBR)tabarithset1, (SUBR)tabaardd, NULL, NULL},
    {"##add.a[a", sizeof(TABARITH2), 0, 3, "a[]", "a[]a",
     (SUBR)tabarithset2, (SUBR)tabaradd, NULL, NULL},
    {"##sub.[k", sizeof(TABARITH1), 0, 3, "k[]", "i[]k",
     (SUBR)tabarithset1, (SUBR)tabaisub, NULL, NULL},
    {"##sub.[k", sizeof(TABARITH1), 0, 3, "k[]", "k[]k",
     (SUBR)tabarithset1, (SUBR)tabaisub, NULL, NULL},
    {"##sub.k[", sizeof(TABARITH2), 0, 3, "k[]", "kk[]",
     (SUBR)tabarithset2, (SUBR)tabiasub, NULL, NULL},
    {"##sub.k[", sizeof(TABARITH2), 0, 3, "k[]", "ki[]",
     (SUBR)tabarithset2, (SUBR)tabiasub, NULL, NULL},
    {"##sub.[ak", sizeof(TABARITH1), 0, 3, "a[]", "a[]k",
     (SUBR)tabarithset1, (SUBR)tabaksub, NULL, NULL},
    {"##sub.k[a", sizeof(TABARITH2), 0, 3, "a[]", "ka[]",
     (SUBR)tabarithset2, (SUBR)tabkasub, NULL, NULL},
    {"##sub.aa[", sizeof(TABARITH2), 0, 3, "a[]", "a[]a",
     (SUBR)tabarithset2, (SUBR)tabaarsb, NULL, NULL},
    {"##sub.a[a", sizeof(TABARITH1), 0, 3, "a[]", "aa[]",
     (SUBR)tabarithset1, (SUBR)tabarasb, NULL, NULL},
    {"##mul.[k", sizeof(TABARITH1), 0, 3, "k[]", "k[]k",
     (SUBR)tabarithset1, (SUBR)tabaimult, NULL, NULL},
    {"##mul.[k", sizeof(TABARITH1), 0, 3, "k[]", "i[]k",
     (SUBR)tabarithset1, (SUBR)tabaimult, NULL, NULL},
    {"##mul.k[", sizeof(TABARITH2), 0, 3, "k[]", "kk[]",
     (SUBR)tabarithset2, (SUBR)tabiamult, NULL, NULL},
    {"##mul.k[", sizeof(TABARITH2), 0, 3, "k[]", "ki[]",
     (SUBR)tabarithset2, (SUBR)tabiamult, NULL, NULL},
    {"##mul.[ak", sizeof(TABARITH1), 0, 3, "a[]", "a[]k",
     (SUBR)tabarithset1, (SUBR)tabakmult, NULL, NULL},
    {"##mul.k[a", sizeof(TABARITH2), 0, 3, "a[]", "ka[]",
     (SUBR)tabarithset2, (SUBR)tabkamult, NULL, NULL},
    {"##mul.a[a", sizeof(TABARITH1), 0, 3, "a[]", "a[]a",
     (SUBR)tabarithset1, (SUBR)tabaraml, NULL, NULL},
    {"##mul.aa[", sizeof(TABARITH2), 0, 3, "a[]", "aa[]",
     (SUBR)tabarithset2, (SUBR)tabaarml, NULL, NULL},
    {"##div.[k",  sizeof(TABARITH1), 0, 3, "k[]", "k[]k",
     (SUBR)tabarithset1, (SUBR)tabaidiv, NULL, NULL},
    {"##div.k[",  sizeof(TABARITH2), 0, 3, "k[]", "ki[]",
     (SUBR)tabarithset2, (SUBR)tabiadiv, NULL, NULL},
    {"##div.[k",  sizeof(TABARITH1), 0, 3, "k[]", "i[]k",
     (SUBR)tabarithset1, (SUBR)tabaidiv, NULL, NULL},
    {"##div.k[",  sizeof(TABARITH2), 0, 3, "k[]", "kk[]",
     (SUBR)tabarithset2, (SUBR)tabiadiv, NULL, NULL},
    {"##div.[ak", sizeof(TABARITH1), 0, 3, "a[]", "a[]k",
     (SUBR)tabarithset1, (SUBR)tabakdiv, NULL, NULL},
    {"##div.k[a", sizeof(TABARITH2), 0, 3, "a[]", "ka[]",
     (SUBR)tabarithset2, (SUBR)tabkadiv, NULL, NULL},
    {"##div.a[a", sizeof(TABARITH1), 0, 3, "a[]", "a[]a",
     (SUBR)tabarithset1, (SUBR)tabaardv, NULL, NULL},
    {"##div.a[a", sizeof(TABARITH1), 0, 3, "a[]", "a[]a",
     (SUBR)tabarithset1, (SUBR)tabaradv, NULL, NULL},
    {"##rem.[k",  sizeof(TABARITH1),0,  3, "k[]", "k[]k",
     (SUBR)tabarithset1, (SUBR)tabairem, NULL, NULL},
    {"##rem.[ak",  sizeof(TABARITH1),0,  3, "a[]", "a[]k",
     (SUBR)tabarithset1, (SUBR)tabarkrem, NULL, NULL},
    {"##rem.k[",  sizeof(TABARITH2),0,  3, "k[]", "kk[]",
     (SUBR)tabarithset2, (SUBR)tabiarem, NULL, NULL},
    {"##pow.[]",  sizeof(TABARITH), 0, 3, "k[]", "k[]k[]",
     (SUBR)tabarithset,(SUBR)tabpow, NULL, NULL},
    {"##pow.[i]",  sizeof(TABARITH), 0, 1, "i[]", "i[]i[]",
     (SUBR)tabpowi, NULL, NULL, NULL},
    {"##pow.[i",  sizeof(TABARITH1), 0, 3, "k[]", "k[]i",
     (SUBR)tabarithset1, (SUBR)tabaipow, NULL, NULL},
    {"##pow.i[",  sizeof(TABARITH2), 0, 3, "k[]", "ik[]",
     (SUBR)tabarithset2, (SUBR)tabiapow, NULL, NULL},
    {"##pow.[p",  sizeof(TABARITH1), 0, 1, "i[]", "i[]i", (SUBR)tabaipowi, NULL, NULL, NULL},
    {"##pow.p[",  sizeof(TABARITH2), 0, 1, "i[]", "ii[]", (SUBR)tabiapowi, NULL, NULL, NULL},
    {"##pow.[k",  sizeof(TABARITH1), 0, 3, "k[]", "k[]k",
     (SUBR)tabarithset1, (SUBR)tabaipow, NULL, NULL},
    {"##pow.k[",  sizeof(TABARITH2), 0, 3, "k[]", "kk[]",
     (SUBR)tabarithset2, (SUBR)tabiapow, NULL, NULL},
    {"##pow.ki[",  sizeof(TABARITH2), 0, 3, "k[]", "ki[]",
     (SUBR)tabarithset2, (SUBR)tabiapow, NULL, NULL},

    {"##pow.[ak",  sizeof(TABARITH1), 0, 3, "a[]", "a[]k",
     (SUBR)tabarithset1, (SUBR)tabarkpow, NULL, NULL},
    {"##pow.a[k[", sizeof(TABARITH), 0, 3, "a[]", "a[]k[]",
     (SUBR)tabarithset, (SUBR)tabarkrpw, NULL, NULL},
    { "maxtab.k",sizeof(TABQUERY),_QQ, 3, "kz", "k[]",
      (SUBR) tabqset, (SUBR) tabmax, NULL, NULL},
    { "maxarray.k", sizeof(TABQUERY), 0, 3, "kz", "k[]",
      (SUBR) tabqset,(SUBR) tabmax, NULL, NULL},
    { "maxarray.i", sizeof(TABQUERY), 0, 1, "iI", "i[]",(SUBR) tabmax1, NULL, NULL, NULL},
    { "mintab.k", sizeof(TABQUERY),_QQ, 3, "kz", "k[]",
      (SUBR) tabqset, (SUBR) tabmin, NULL, NULL},
    { "minarray.k", sizeof(TABQUERY),0, 3, "kz", "k[]",(SUBR) tabqset,
      (SUBR) tabmin, NULL, NULL},
    { "minarray.i", sizeof(TABQUERY),0, 1, "iI", "i[]",(SUBR) tabmin1, NULL, NULL, NULL},
    { "sumtab", sizeof(TABQUERY1),_QQ, 3, "k", "k[]",
      (SUBR) tabqset1, (SUBR) tabsum, NULL, NULL},
    { "sumarray.k", sizeof(TABQUERY1),0, 3, "k", "k[]",
      (SUBR) tabqset1, (SUBR) tabsum, NULL, NULL},
    { "sumarray.a", sizeof(TABQUERY1),0, 3, "a", "a[]",
      (SUBR) tabqset1, (SUBR) tabsuma1, NULL, NULL},
    { "sumarray.i", sizeof(TABQUERY1),0, 1, "i", "i[]", (SUBR) tabsum1, NULL, NULL, NULL},
    { "scalet", sizeof(TABSCALE), _QQ|WI, 3, "",  "k[]kkOJ",
      (SUBR) tabscaleset,(SUBR) tabscale, NULL, NULL},
    { "scalearray.k", sizeof(TABSCALE), WI, 3, "",  "k[]kkOJ",
      (SUBR) tabscaleset,(SUBR) tabscale, NULL, NULL},
    { "scalearray.1", sizeof(TABSCALE), WI, 1, "",  "i[]iiOJ",   (SUBR) tabscale1, NULL, NULL, NULL},
    { "=.I", sizeof(TABCPY), 0, 1, "i[]", "i[]", (SUBR)tabcopy1, NULL, NULL, NULL},
    { "fillarray", sizeof(TABCPY), 0, 1, "k[]", "i[]", (SUBR)tabcopy1, NULL, NULL, NULL},
    { "=.J", sizeof(TABCPY), 0, 1, "i[]", "k[]", (SUBR)tabcopy1, NULL, NULL, NULL},
    { "=.IK", sizeof(TABCPY), 0, 3, "k[]", "i[]", (SUBR)tabcopy1, (SUBR)tabcopy1, NULL, NULL},
    { "=.K", sizeof(TABCPY), 0, 3, "k[]", "k[]", (SUBR)tabcopyk_init, (SUBR)tabcopyk, NULL, NULL},
    { "=._", sizeof(TABCPY), 0, 3, ".[]", ".[]", (SUBR)tabcopy, (SUBR)tabcopy, NULL, NULL},
    { "=.L", sizeof(TABCPY), 0, 3, ".[]", ".[]", (SUBR)tabcopy2, (SUBR)tabcopy2, NULL, NULL},
    { "tabgen", sizeof(TABGEN), _QQ, 1, "k[]", "iip", (SUBR) tabgen, NULL, NULL, NULL},
    { "tabmap_i", sizeof(TABMAP), _QQ, 1, "k[]", "k[]S", (SUBR) tabmap_set, NULL, NULL, NULL},
    { "tabmap", sizeof(TABMAP), _QQ, 3, "k[]", "k[]S", (SUBR) tabmap_set,
      (SUBR) tabmap_perf, NULL, NULL},
    { "tabmap", sizeof(TABMAP), _QQ, 3, "k[]", "k[]S", (SUBR) tabmap_set,
      (SUBR) tabmap_perf, NULL, NULL},
    { "genarray.i", sizeof(TABGEN),0, 1, "i[]", "iip", (SUBR) tabgen, NULL, NULL, NULL},
    { "genarray_i", sizeof(TABGEN),0, 1, "k[]", "iip", (SUBR) tabgen, NULL, NULL, NULL},
    { "genarray.k", sizeof(TABGEN),0, 2, "k[]", "kkp", NULL, (SUBR)tabgen, NULL, NULL},
    { "maparray_i", sizeof(TABMAP),0, 1, "k[]", "k[]S", (SUBR) tabmap_set, NULL, NULL, NULL},
    { "maparray.k", sizeof(TABMAP), 0, 3, "k[]", "k[]S", (SUBR) tabmap_set,
      (SUBR) tabmap_perf, NULL, NULL},
    { "maparray.i", sizeof(TABMAP), 0, 1, "i[]", "i[]S", (SUBR) tabmap_set, NULL, NULL, NULL},
    /*  { "maparray.s", sizeof(TABMAP), 0, 3, "S[]", "S[]S", (SUBR) tabmap_set, */
    /*                                               (SUBR) tabmap_perf     }, */
    { "tabslice", sizeof(TABSLICE), _QQ, 2, "k[]", "k[]iip",
      NULL, (SUBR) tabslice, NULL, NULL},
    { "slicearray.i", sizeof(TABSLICE), 0, 1, "i[]", "i[]iip",
      (SUBR) tabslice, NULL, NULL, NULL},
    { "slicearray.k", sizeof(TABSLICE), 0, 2, "k[]", "k[]iip",
      (SUBR) tabslice, (SUBR) tabslice, NULL, NULL},
    { "slicearray.a", sizeof(TABSLICE), 0, 2, "a[]", "a[]iip",
      (SUBR) tabslice, (SUBR) tabslice, NULL, NULL},
    { "slicearray.S", sizeof(TABSLICE), 0, 2, "S[]", "S[]iip",
      (SUBR) tabslice, (SUBR) tabslice, NULL, NULL},
    { "slicearray_i.i", sizeof(TABSLICE), 0, 1, "i[]", "i[]iip",
      (SUBR) tabslice, NULL, NULL, NULL},
    { "slicearray_i.k", sizeof(TABSLICE), 0, 1, "k[]", "k[]iip",
    (SUBR) tabslice, NULL, NULL, NULL},
    { "slicearray_i.S", sizeof(TABSLICE), 0, 1, "S[]", "S[]iip",
      (SUBR) tabslice, NULL, NULL, NULL},
    { "trim.i", sizeof(TRIM), WI, 1, "", "i[]i", (SUBR)trim_i, NULL, NULL, NULL},
    { "trim.k", sizeof(TRIM), WI, 2, "", ".[]k", NULL, (SUBR)trim, NULL, NULL},
    { "trim_i", sizeof(TRIM), WI, 1, "", ".[]i", (SUBR)trim_i, NULL, NULL, NULL},
    { "copy2ftab", sizeof(TABCOPY), TW|_QQ, 2, "", "k[]k", NULL, (SUBR) tab2ftab, NULL, NULL},
    { "copy2ttab", sizeof(TABCOPY), TR|_QQ, 2, "", "k[]k", NULL, (SUBR) ftab2tab, NULL, NULL},
    { "copya2ftab.k", sizeof(TABCOPY), TW, 3, "", "k[]k",
      (SUBR) tab2ftabi, (SUBR) tab2ftab, NULL, NULL},

    { "copyf2array.k", sizeof(TABCOPY), TR, 3, "", "k[]k",
      (SUBR) ftab2tabi, (SUBR) ftab2tab, NULL, NULL},
    { "copyf2array.kk", sizeof(TABCOPY), TR, 3, "", "k[]kk",
      (SUBR) ftab2tabi, (SUBR) ftab2tab, NULL, NULL},
    // { "copya2ftab.i", sizeof(TABCOPY), TW, 1, "", "i[]i", (SUBR) tab2ftabi },
    { "copya2ftab.ii", sizeof(TABCOPY2), TW, 1, "", "i[]io", (SUBR) tab2ftab_offset_i, NULL, NULL, NULL},
    { "copya2ftab.kk", sizeof(TABCOPY2), TW, 2, "", "k[]kO", NULL , (SUBR) tab2ftab_offset, NULL, NULL},
    { "copyf2array.i", sizeof(TABCOPY), TR, 1, "", "i[]i", (SUBR) ftab2tabi, NULL, NULL, NULL},
    /* { "lentab", 0xffff}, */
    { "lentab.i", sizeof(TABQUERY1), _QQ, 1, "i", "k[]p", (SUBR) tablength, NULL, NULL, NULL},
    { "lentab.k", sizeof(TABQUERY1), _QQ, 1, "k", "k[]p", NULL, (SUBR) tablength, NULL, NULL},
    { "lenarray.ix", sizeof(TABQUERY1), 0, 1, "i", ".[]p", (SUBR) tablength, NULL, NULL, NULL},
    { "lenarray.kx", sizeof(TABQUERY1), 0, 2, "k", ".[]p", NULL, (SUBR)tablength, NULL, NULL},
    { "in.A", sizeof(OUTA), 0, 3, "a[]", "", (SUBR)ina_set, (SUBR)ina, NULL, NULL},
    { "monitor.A", sizeof(OUTA), IB, 3, "a[]", "",
      (SUBR)monitora_init, (SUBR)monitora_perf, NULL, NULL},
    { "rfft", sizeof(FFT), 0, 3, "k[]","k[]",
      (SUBR) init_rfft, (SUBR) perf_rfft, NULL, NULL},
    {"rfft", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) rfft_i, NULL, NULL, NULL},
    {"rifft", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_rifft, (SUBR) perf_rifft, NULL, NULL},
    {"rifft", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) rifft_i, NULL, NULL, NULL},
    {"cmplxprod", sizeof(FFT), 0, 3, "k[]","k[]k[]",
     (SUBR) init_rfftmult, (SUBR) perf_rfftmult, NULL, NULL},
    {"fft", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) initialise_fft, (SUBR) perf_fft, NULL, NULL},
    {"fft", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) fft_i, NULL, NULL, NULL},
    {"fftinv", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_ifft, (SUBR) perf_ifft, NULL, NULL},
    {"fftinv", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) ifft_i, NULL, NULL, NULL},
    {"rect2pol", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_recttopol, (SUBR) perf_recttopol, NULL, NULL},
    {"pol2rect", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_recttopol, (SUBR) perf_poltorect, NULL, NULL},
    {"pol2rect.AA", sizeof(FFT), 0, 3, "k[]","k[]k[]",
     (SUBR) init_poltorect2, (SUBR) perf_poltorect2, NULL, NULL},
    {"mags", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_mags, (SUBR) perf_mags, NULL, NULL},
    {"pows", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_mags, (SUBR) perf_pows, NULL, NULL},
    {"phs", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_mags, (SUBR) perf_phs, NULL, NULL},
    {"log", sizeof(FFT), 0, 3, "k[]","k[]i",
     (SUBR) init_logarray, (SUBR) perf_logarray, NULL, NULL},
    {"r2c", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) rtoc_i, NULL, NULL, NULL},
    {"r2c", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_rtoc, (SUBR) perf_rtoc, NULL, NULL},
    {"c2r", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) ctor_i, NULL, NULL, NULL},
    {"c2r", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_ctor, (SUBR) perf_ctor, NULL, NULL},
    {"window", sizeof(FFT), 0, 3, "k[]","k[]Op",
     (SUBR) init_window, (SUBR) perf_window, NULL, NULL},
    {"pvsceps", sizeof(PVSCEPS), 0, 3, "k[]","fo",
     (SUBR) pvsceps_init, (SUBR) pvsceps_perf, NULL, NULL},
    {"cepsinv", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_iceps, (SUBR) perf_iceps, NULL, NULL},
    {"ceps", sizeof(FFT), 0, 3, "k[]","k[]k",
     (SUBR) init_ceps, (SUBR) perf_ceps, NULL, NULL},
    {"getrow", sizeof(FFT), 0, 1, "i[]","i[]i",
     (SUBR) rows_i, NULL, NULL, NULL},
    {"getrow.S", sizeof(FFT), 0, 3, "S[]","S[]k",
     (SUBR) rows_init_S, (SUBR) rows_perf_S, NULL, NULL},
    {"getrow", sizeof(FFT), 0, 3, "k[]","k[]k",
     (SUBR) rows_init, (SUBR) rows_perf, NULL, NULL},
    {"getcol", sizeof(FFT), 0, 1, "i[]","i[]i",
     (SUBR) cols_i, NULL, NULL, NULL},
    {"getcol", sizeof(FFT), 0, 3, "k[]","k[]k",
     (SUBR) cols_init, (SUBR) cols_perf, NULL, NULL},
    {"getcol", sizeof(FFT), 0, 3, "S[]","S[]k",
     (SUBR) cols_init_S, (SUBR) cols_perf_S, NULL, NULL},
    {"setrow", sizeof(FFT), 0, 1, "i[]","i[]i",
     (SUBR) set_rows_i, NULL, NULL, NULL},
    {"setrow", sizeof(FFT), 0, 3, "k[]","k[]k",
     (SUBR) set_rows_init, (SUBR) set_rows_perf, NULL, NULL},
    {"setrow.S", sizeof(FFT), 0, 3, "S[]","S[]k",
     (SUBR) set_rows_init_S, (SUBR) set_rows_perf_S, NULL, NULL},
    {"setcol", sizeof(FFT), 0, 1, "i[]","i[]i",
     (SUBR) set_cols_i, NULL, NULL, NULL},
    {"setcol", sizeof(FFT), 0, 3, "k[]","k[]k",
     (SUBR) set_cols_init, (SUBR) set_cols_perf, NULL, NULL},
    {"setcol", sizeof(FFT), 0, 3, "S[]","S[]k",
     (SUBR) set_cols_init_S, (SUBR) set_cols_perf_S, NULL, NULL},
    {"shiftin", sizeof(FFT), 0, 3, "k[]","a",
     (SUBR) shiftin_init, (SUBR) shiftin_perf, NULL, NULL},
    {"shiftout", sizeof(FFT), 0, 3, "a","k[]o",
     (SUBR) shiftout_init, (SUBR) shiftout_perf, NULL, NULL},
    {"unwrap", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_recttopol, (SUBR) unwrap, NULL, NULL},
    {"=.X", sizeof(TABCOPY), 0, 3, "k[]","k", (SUBR) scalarset, (SUBR) scalarset, NULL, NULL},
    {"=.Z", sizeof(TABCOPY), 0 , 2, "a[]", "a", NULL, (SUBR) arrayass, NULL, NULL},
    {"dct", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_dct, (SUBR) kdct, NULL, NULL},
    {"dct", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR) dct, NULL, NULL, NULL},
    {"dctinv", sizeof(FFT), 0, 3, "k[]","k[]",
     (SUBR) init_dctinv, (SUBR) kdct, NULL, NULL},
    {"dctinv", sizeof(FFT), 0, 1, "i[]","i[]",
     (SUBR)dctinv, NULL, NULL, NULL},
    {"mfb", sizeof(MFB), 0, 3, "k[]","k[]kki",
     (SUBR) mfb_init, (SUBR) mfb, NULL, NULL},
    {"mfb", sizeof(MFB), 0, 1, "i[]","i[]iii",
     (SUBR)mfbi, NULL, NULL, NULL},
    {"centroid", sizeof(CENTR), 0, 1, "i","i[]",
     (SUBR) array_centroid, NULL, NULL, NULL},
    {"centroid", sizeof(CENTR), 0, 2, "k","k[]", NULL,
     (SUBR)array_centroid, NULL, NULL},
    {"interleave", sizeof(INTERL), 0, 1, "i[]","i[]i[]",
     (SUBR)interleave_i, NULL, NULL, NULL},
    {"interleave", sizeof(INTERL), 0, 1, "k[]","k[]k[]",
     (SUBR)interleave_i, (SUBR) interleave_perf, NULL, NULL},
    {"deinterleave", sizeof(INTERL), 0, 1, "i[]i[]","i[]",
     (SUBR)deinterleave_i, NULL, NULL, NULL},
    {"deinterleave", sizeof(INTERL), 0, 1, "k[]k[]","k[]",
     (SUBR)deinterleave_i, (SUBR)deinterleave_perf, NULL, NULL},
    { "taninv2.Ai", sizeof(TABARITH), 0, 1, "i[]", "i[]i[]", (SUBR)taninv2_A, NULL, NULL, NULL},
    { "taninv2.Ak", sizeof(TABARITH), 0, 2, "k[]", "k[]k[]", (SUBR)tabarithset, (SUBR)taninv2_A, NULL, NULL},
    { "taninv2.Aa", sizeof(TABARITH), 0, 2, "a[]", "a[]a[]", (SUBR)tabarithset, (SUBR)taninv2_Aa, NULL, NULL},
    { "autocorr", sizeof(AUTOCORR), 0, 3, "k[]", "k[]", (SUBR) init_autocorr, (SUBR) perf_autocorr, NULL, NULL},
    { "clear", sizeof(TABCLEAR), 0, 3, "", "a[]", (SUBR)tabclearset, (SUBR)tabclear, NULL, NULL},
    { "clear", sizeof(TABCLEAR), 0, 3, "", "k[]", (SUBR)tabclearset, (SUBR)tabcleark, NULL, NULL}
  };

LINKAGE_BUILTIN(arrayvars_localops)
