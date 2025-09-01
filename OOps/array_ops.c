/* array_ops.c: array operators

   Copyright (C) 2011,2012,2025 John ffitch, Steven Yi, Victor Lazzarini

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

#include "array_ops.h"

int32_t array_init(CSOUND *csound, ARRAYINIT *p)
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
  }

  {
    CS_VARIABLE* var = arrayDat->arrayType->createVariable(csound, (void *)
                                                           arrayDat->arrayType,
                                                           p->h.insdshead);
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
#include "csound_standard_types.h"
int32_t tabfill(CSOUND *csound, TABFILL *p)
{
  int32_t    nargs = p->INOCOUNT;
  int32_t i, size;
  size_t memMyfltSize;
  MYFLT  **valp = p->iargs;
  tabinit(csound, p->ans, nargs, p->h.insdshead);
  size = p->ans->sizes[0];
  for (i=1; i<p->ans->dimensions; i++) size *= p->ans->sizes[i];
  if (size<nargs) nargs = size;
  memMyfltSize = p->ans->arrayMemberSize / sizeof(MYFLT);
  if(p->ans->arrayType == &CS_VAR_TYPE_B ||
     p->ans->arrayType == &CS_VAR_TYPE_b) {
    for (i=0; i<nargs; i++) {
      // bool is int32_t but each array slot is sizeof(MYFLT)
      int32_t *idat = (int32_t *) (p->ans->data + (i * memMyfltSize));
      *idat = *valp[i] ? 1 : 0;
    }
  }
  else
    for (i=0; i<nargs; i++) {
      p->ans->arrayType->copyValue(csound,
                                   p->ans->arrayType,
                                   p->ans->data + (i * memMyfltSize),
                                   valp[i], p->h.insdshead);

    }
  return OK;
}

#include <ctype.h>
static MYFLT nextval(FILE *f)
{
  /* Read the next character; suppress multiple space and comments to a
     single space */
  int32_t c;
 top:
  c = getc(f);
 top1:
  if (UNLIKELY(feof(f))) return NAN; /* Hope value is ignored */
  if (isdigit(c) || c=='e' || c=='E' || c=='+' || c=='-' || c=='.') {
    double d;                           /* A number starts */
    char buff[128];
    int32_t j = 0;
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

int32_t tabfillf(CSOUND* csound, TABFILLF* p)
{
  int32_t i = 0, flen = 0, size;
  char *fname = p->fname->data;
  FILE    *infile;
  void    *fd = csound->FileOpen(csound, &infile, CSFILE_STD, fname, "r",
                                 "SFDIR;SSDIR;INCDIR", CSFTYPE_FLOATS_TEXT, 0);
  if (UNLIKELY(fd == NULL)) {
    return csound->InitError(csound, Str("error opening ASCII file %s\n"), fname);
  }
  do {
    flen++;
    nextval(infile);
  } while (!feof(infile));
  flen--; // overshoots by 1
  tabinit(csound, p->ans, flen, p->h.insdshead);
  size = p->ans->sizes[0];
  for (i=1; i<p->ans->dimensions; i++) size *= p->ans->sizes[i];
  if (size<flen) flen = size;
  if (p->ans->dimensions == 1 && (flen > p->ans->sizes[0]))
    flen = p->ans->sizes[0];
  else if (p->ans->dimensions == 2 && (flen > p->ans->sizes[0]*p->ans->sizes[1]))
    flen = p->ans->sizes[0]*p->ans->sizes[1];
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
  int32_t c;
  char *f = *ff;
 top:
  c = *f++;
 top1:
  if (UNLIKELY(c=='\0')) { *ff = f; return NAN; }
  if (isdigit(c) || c=='e' || c=='E' || c=='+' || c=='-' || c=='.') {
    double d;                           /* A number starts */
    char buff[128];
    int32_t j = 0;
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

int32_t tabsfill(CSOUND *csound, TABFILLF *p)
{
  int32_t i = 0, flen = 0, size;
  char *string = p->fname->data;
  MYFLT x;
  do {
    x = nextsval(&string);
    if (x==NAN) break;
    flen++;
  } while (*string!='\0');
  tabinit(csound, p->ans, flen, p->h.insdshead);
  size = p->ans->sizes[0];
  for (i=1; i<p->ans->dimensions; i++) size *= p->ans->sizes[i];
  if (size<flen) flen = size;
  if (p->ans->dimensions == 1 && (flen > p->ans->sizes[0]))
    flen = p->ans->sizes[0];
  else if (p->ans->dimensions == 2 && (flen > p->ans->sizes[0]*p->ans->sizes[1]))
    flen = p->ans->sizes[0]*p->ans->sizes[1];
  string = p->fname->data;
  i = 0;
  while ((*string!='\0') && i < flen)
    ((MYFLT*)p->ans->data)[i++] = nextsval(&string);
  return OK;
}

int32_t array_err(CSOUND* csound, ARRAY_SET *p)
{
  IGN(p);
  return csound->InitError(csound,  "%s", Str("Cannot set i-array at k-rate\n"));
}

int32_t array_set(CSOUND* csound, ARRAY_SET *p)
{
  ARRAYDAT* dat = p->arrayDat;
  MYFLT* mem = dat->data;
  int32_t i;
  int32_t end, index, incr;

  int32_t indefArgCount = p->INOCOUNT - 2;
  if (UNLIKELY(indefArgCount == 0)) {
    csound->ErrorMsg(csound, "%s", Str("Error: no indexes set for array set\n"));
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
    if (UNLIKELY(end>=dat->sizes[i])||
        UNLIKELY(end<0))
      return csound->PerfError(csound, &(p->h),
                               Str("Array index %d out of range (0,%d) "
                                   "for dimension %d"),
                               end, dat->sizes[i], i+1);
    index = (index * dat->sizes[i]) + end;
  }
  incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
  mem += incr;
  dat->arrayType->copyValue(csound, dat->arrayType, mem, p->value,  p->h.insdshead);
  return OK;
}

int32_t array_get(CSOUND* csound, ARRAY_GET *p)
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
                             "%s", Str("Error: no indexes set for array get"));
  if (UNLIKELY(indefArgCount!=dat->dimensions)) {
    return csound->PerfError(csound, &(p->h),
                             Str("Array dimension %d out of range "
                                 "for dimensions %d"),
                             indefArgCount, dat->dimensions);
  }
  index = 0;
  for (i=0;i<indefArgCount; i++) {
    end = (int)(*p->indexes[i]);
    if (UNLIKELY(end>=dat->sizes[i]) ||
        UNLIKELY(end<0))
      return csound->PerfError(csound, &(p->h),
                               Str("Array index %d out of range (0,%d) "
                                   "for dimension %d"),
                               end, dat->sizes[i]-1, i+1);
    index = (index * dat->sizes[i]) + end;
  }

  incr = (index * (dat->arrayMemberSize / sizeof(MYFLT)));
  mem += incr;
  dat->arrayType->copyValue(csound, dat->arrayType, (void*)p->out, (void*)mem,
                            p->h.insdshead);
  return OK;
}



int32_t tabarithset(CSOUND *csound, TABARITH *p)
{

  if (LIKELY(p->left->data && p->right->data)) {
    int32_t i;
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
int32_t tabarithset1(CSOUND *csound, TABARITH1 *p)
{
  ARRAYDAT *left = p->left;

  if (p->ans->data == left->data) {
    return OK;
  }

  tabinit_like(csound, p->ans, left);
  return OK;
}

// For cases with array as second arg
int32_t tabarithset2(CSOUND *csound, TABARITH2 *p)
{
  ARRAYDAT *right = p->right;

  if (p->ans->data == right->data) {
    // printf("same ptr\n");
    return OK;
  }

  tabinit_like(csound, p->ans, right);
  return OK;
}

int32_t tabadd(CSOUND *csound, TABARITH *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->left;
  ARRAYDAT *r   = p->right;
  int32_t sizel    = l->sizes[0];
  int32_t sizer    = r->sizes[0];
  int32_t i;

  if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<ans->dimensions; i++) {
    sizel*=l->sizes[i];
    sizer*=r->sizes[i];
  }
  if (sizer<sizel) sizel= sizer;
  for (i=0; i<sizel; i++)
    ans->data[i] = l->data[i] + r->data[i];
  return OK;
}

int32_t tabsub(CSOUND *csound, TABARITH *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->left;
  ARRAYDAT *r   = p->right;
  int32_t sizel    = l->sizes[0];
  int32_t sizer    = r->sizes[0];
  int32_t i;

  if (UNLIKELY(ans->data == NULL || l->data==NULL || r->data==NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<ans->dimensions; i++) {
    sizel*=l->sizes[i];
    sizer*=r->sizes[i];
  }
  if (sizer<sizel) sizel= sizer;
  for (i=0; i<sizel; i++)
    ans->data[i] = l->data[i] - r->data[i];
  return OK;
}

int32_t tabmult(CSOUND *csound, TABARITH *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->left;
  ARRAYDAT *r   = p->right;
  int32_t sizel    = l->sizes[0];
  int32_t sizer    = r->sizes[0];
  int32_t i;

  if (UNLIKELY(ans->data == NULL || l->data== NULL || r->data==NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<ans->dimensions; i++) {
    sizel*=l->sizes[i];
    sizer*=r->sizes[i];
  }
  if (sizer<sizel) sizel= sizer;
  for (i=0; i<sizel; i++)
    ans->data[i] = l->data[i] * r->data[i];
  return OK;
}

int32_t tabdiv(CSOUND *csound, TABARITH *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->left;
  ARRAYDAT *r   = p->right;
  int32_t sizel = l->sizes[0];
  int32_t sizer = r->sizes[0];
  int32_t i;

  if (UNLIKELY(ans->data == NULL || l->data== NULL || r->data==NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

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

int32_t tabrem(CSOUND *csound, TABARITH *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->left;
  ARRAYDAT *r   = p->right;
  int32_t sizel = l->sizes[0];
  int32_t sizer = r->sizes[0];
  int32_t i;

  if (UNLIKELY(ans->data == NULL || l->data== NULL || r->data==NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

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

int32_t tabpow(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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

// Add array and scalar
static int32_t tabiadd(CSOUND *csound, ARRAYDAT *ans, ARRAYDAT *l, MYFLT r, void *p)
{
  int32_t sizel = l->sizes[0];
  int32_t i;

  if (UNLIKELY(ans->data == NULL || l->data== NULL))
    return csound->PerfError(csound, &(((TABARITH *) p)->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<l->dimensions; i++) {
    sizel*=l->sizes[i];
  }
  for (i=0; i<sizel; i++)
    ans->data[i] = l->data[i] + r;
  return OK;
}

// K[]+K
int32_t tabaiadd(CSOUND *csound, TABARITH1 *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->left;
  MYFLT r       = *p->right;
  tabinit_like(csound, ans, l);
  return tabiadd(csound, ans, l, r, p);
}

// K+K[]
int32_t tabiaadd(CSOUND *csound, TABARITH2 *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->right;
  MYFLT r       = *p->left;
  return tabiadd(csound, ans, l, r, p);
}

/* ****************Array versions of addin/subin*********** */
/// K[] += K
int32_t addinAA(CSOUND *csound, TABARITHIN1 *p)
{
  ARRAYDAT *ans = p->ans;
  MYFLT r       = *p->right;
  //tabinit_like(csound, ans, l);
  return tabiadd(csound, ans, ans, r, p);
}

// K[] += K[]
int32_t tabaddinkk(CSOUND *csound, TABARITHIN *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *r   = p->right;
  int32_t sizel    = ans->sizes[0];
  int32_t sizer    = r->sizes[0];
  int32_t i;

  tabinit_like(csound, ans, r);
  if (UNLIKELY(ans->data == NULL || r->data==NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

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
int32_t tabaaddin(CSOUND *csound, TABARITHIN *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabaasubin(CSOUND *csound, TABARITHIN *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabarkrddin(CSOUND *csound, TABARITHIN *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabarkrsbin(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabakaddin(CSOUND *csound, TABARITHIN1 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabaksubin(CSOUND *csound, TABARITHIN1 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabaksub(CSOUND *csound, TABARITH1 *p)
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
                             "%s", Str("array-variable not initialised"));

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

static int32_t tabisub(CSOUND *csound, ARRAYDAT *ans, ARRAYDAT *l,
                        MYFLT r, void *p);
// K[] -= K
int32_t subinAA(CSOUND *csound, TABARITHIN1 *p)
{
  ARRAYDAT *ans = p->ans;
  MYFLT r       = *p->right;
  //tabinit_like(csound, ans, l);
  return tabisub(csound, ans, ans, r, p);
}

// K[] -= K[]
int32_t tabsubinkk(CSOUND *csound, TABARITHIN *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *r   = p->right;
  int32_t sizel    = ans->sizes[0];
  int32_t sizer    = r->sizes[0];
  int32_t i;

  tabinit_like(csound, ans, r);
  if (UNLIKELY(ans->data == NULL || r->data==NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<ans->dimensions; i++) {
    sizel*=ans->sizes[i];
    sizer*=r->sizes[i];
  }
  if (sizer<sizel) sizel= sizer;
  for (i=0; i<sizel; i++)
    ans->data[i] -= r->data[i];
  return OK;
}

static int32_t tabimult(CSOUND *csound, ARRAYDAT *ans, ARRAYDAT *l,
                        MYFLT r, void *p);

/* ****************Array versions of mulin/divin*********** */
/// K[] *= K
int32_t mulinAA(CSOUND *csound, TABARITHIN1 *p)
{
  ARRAYDAT *ans = p->ans;
  MYFLT r       = *p->right;
  //tabinit_like(csound, ans, l);
  return tabimult(csound, ans, ans, r, p);
}

// K[] *= K[]
int32_t tabmulinkk(CSOUND *csound, TABARITHIN *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *r   = p->right;
  int32_t sizel    = ans->sizes[0];
  int32_t sizer    = r->sizes[0];
  int32_t i;

  tabinit_like(csound, ans, r);
  if (UNLIKELY(ans->data == NULL || r->data==NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<ans->dimensions; i++) {
    sizel*=ans->sizes[i];
    sizer*=r->sizes[i];
  }
  if (sizer<sizel) sizel= sizer;
  for (i=0; i<sizel; i++)
    ans->data[i] *= r->data[i];
  return OK;
}

// a[] *= a
int32_t taba1mulin(CSOUND *csound, TABARITHIN1 *p)
{
  ARRAYDAT *ans = p->ans;
  MYFLT *r = p->right;
  int32_t sizel = ans->sizes[0];
  uint32_t offset = p->h.insdshead->ksmps_offset;
  uint32_t early = p->h.insdshead->ksmps_no_end;
  int32_t i, n, nsmps = CS_KSMPS;
  int32_t span = (ans->arrayMemberSize)/sizeof(MYFLT);

  if (UNLIKELY(ans->data == NULL))
    return csound->PerfError(csound, &(p->h), "%s", Str("array-variable not initialised"));

  for (i=1; i<ans->dimensions; i++) {
    sizel *= ans->sizes[i];
  }
  if (UNLIKELY(early)) {
    nsmps -= early;
  }
  for (i=0; i<sizel; i++) {
    MYFLT *aa;
    int32_t j = i*span;
    aa = (MYFLT*)&(ans->data[j]);
    if (UNLIKELY(offset)) memset(aa, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      memset(&aa[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++)
      aa[n] *= r[n];
  }
  return OK;
}

//a[]*=a[]
int32_t tabamulin(CSOUND *csound, TABARITHIN *p)
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
                             "%s", Str("array-variable not initialised"));

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
      aa[n] *= b[n];
  }
  return OK;
}

// a[]/=a[]
int32_t tabaadivin(CSOUND *csound, TABARITHIN *p)
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
                             "%s", Str("array-variable not initialised"));

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
      aa[n] /= b[n];
  }
  return OK;
}

//a[]*=k[]
int32_t tabarkrmulin(CSOUND *csound, TABARITHIN *p)
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
                             "%s", Str("array-variable not initialised"));

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
      aa[n] *= b;
  }
  return OK;
}

// a[]/=k[]
int32_t tabarkrdivin(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
      aa[n] /= b;
  }
  return OK;
}

//a[]*=k
int32_t tabakmulin(CSOUND *csound, TABARITHIN1 *p)
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
                             "%s", Str("array-variable not initialised"));

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
      aa[n] *= l;
  }
  return OK;
}
//========================
//a[]/=k
int32_t tabakdivin(CSOUND *csound, TABARITHIN1 *p)
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
                             "%s", Str("array-variable not initialised"));

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
      aa[n] /= l;
  }
  return OK;
}

static int32_t tabidiv(CSOUND *csound, ARRAYDAT *ans, ARRAYDAT *l,
                        MYFLT r, void *p)
{
  int32_t sizel    = l->sizes[0];
  int32_t i;

  if (UNLIKELY(ans->data == NULL || l->data== NULL))
    return csound->PerfError(csound, &(((TABARITH1 *)p)->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<l->dimensions; i++) {
    sizel*=l->sizes[i];
  }
  for (i=0; i<sizel; i++)
    ans->data[i] = l->data[i] / r;
  return OK;
}


// K[] /= K
int32_t divinAA(CSOUND *csound, TABARITHIN1 *p)
{
  ARRAYDAT *ans = p->ans;
  MYFLT r       = *p->right;
  //tabinit_like(csound, ans, l);
  return tabidiv(csound, ans, ans, r, p);
}

// K[] /= K[]
int32_t tabdivinkk(CSOUND *csound, TABARITHIN *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *r   = p->right;
  int32_t sizel    = ans->sizes[0];
  int32_t sizer    = r->sizes[0];
  int32_t i;

  tabinit_like(csound, ans, r);
  if (UNLIKELY(ans->data == NULL || r->data==NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<ans->dimensions; i++) {
    sizel*=ans->sizes[i];
    sizer*=r->sizes[i];
  }
  if (sizer<sizel) sizel= sizer;
  for (i=0; i<sizel; i++)
    ans->data[i] /= r->data[i];
  return OK;
}

// Subtract K[]-K
int32_t tabaisub(CSOUND *csound, TABARITH1 *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->left;
  MYFLT r       = *p->right;
  int32_t sizel = l->sizes[0];
  int32_t i;
  tabinit_like(csound, ans, l);

  if (UNLIKELY(ans->data == NULL || l->data== NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<l->dimensions; i++) {
    sizel*=l->sizes[i];
  }
  for (i=0; i<sizel; i++)
    ans->data[i] = l->data[i] - r;
  return OK;
}

// Subtract K-K[]
int32_t tabiasub(CSOUND *csound, TABARITH2 *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->right;
  MYFLT r     = *p->left;
  int32_t sizel = l->sizes[0];
  int32_t i;
  tabinit_like(csound, ans, l);

  if (UNLIKELY(ans->data == NULL || l->data== NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<l->dimensions; i++) {
    sizel*=l->sizes[i];
  }
  for (i=0; i<sizel; i++)
    ans->data[i] = r - l->data[i];
  return OK;
}

// Sub scalar by array
static int32_t tabisub(CSOUND *csound, ARRAYDAT *ans, ARRAYDAT *l,
                        MYFLT r, void *p)
{
  int32_t sizel    = l->sizes[0];
  int32_t i;

  if (UNLIKELY(ans->data == NULL || l->data== NULL))
    return csound->PerfError(csound, &(((TABARITH1 *)p)->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<l->dimensions; i++) {
    sizel*=l->sizes[i];
  }
  for (i=0; i<sizel; i++)
    ans->data[i] = l->data[i] - r;
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
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<l->dimensions; i++) {
    sizel*=l->sizes[i];
  }
  for (i=0; i<sizel; i++)
    ans->data[i] = l->data[i] * r;
  return OK;
}

// K[] * K
int32_t tabaimult(CSOUND *csound, TABARITH1 *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->left;
  MYFLT r       = *p->right;
  tabinit_like(csound, ans, l);
  return tabimult(csound, ans, l, r, p);
}

// K * K[]
int32_t tabiamult(CSOUND *csound, TABARITH2 *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->right;
  MYFLT r       = *p->left;
  tabinit_like(csound, ans, l);
  return tabimult(csound, ans, l, r, p);
}

// K[] / K
int32_t tabaidiv(CSOUND *csound, TABARITH1 *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->left;
  MYFLT r       = *p->right;
  int32_t sizel = l->sizes[0];
  int32_t i;
  tabinit_like(csound, ans, l);

  if (UNLIKELY(r==FL(0.0)))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("division by zero in array-var"));
  if (UNLIKELY(ans->data == NULL || l->data== NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<l->dimensions; i++) {
    sizel*=l->sizes[i];
  }
  for (i=0; i<sizel; i++)
    ans->data[i] = l->data[i] / r;
  return OK;
}

// K / K[]
int32_t tabiadiv(CSOUND *csound, TABARITH2 *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->right;
  MYFLT r     = *p->left;
  int32_t sizel    = l->sizes[0];
  int32_t i;
  tabinit_like(csound, ans, l);

  if (UNLIKELY(ans->data == NULL || l->data== NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<l->dimensions; i++) {
    sizel*=l->sizes[i];
  }
  for (i=0; i<sizel; i++) {
    if (UNLIKELY(l->data[i]==FL(0.0)))
      return csound->PerfError(csound, &(p->h),
                               "%s", Str("division by zero in array-var"));
    ans->data[i] = r / l->data[i];
  }
  return OK;
}

// K[] % K
int32_t tabairem(CSOUND *csound, TABARITH1 *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->left;
  MYFLT r       = *p->right;
  int32_t sizel = l->sizes[0];
  int32_t i;

  if (UNLIKELY(r==FL(0.0)))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("division by zero in array-var"));
  if (UNLIKELY(ans->data == NULL || l->data== NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

  for (i=1; i<l->dimensions; i++) {
    sizel*=l->sizes[i];
  }
  for (i=0; i<sizel; i++)
    ans->data[i] = MOD(l->data[i], r);
  return OK;
}

// K % K[]
int32_t tabiarem(CSOUND *csound, TABARITH2 *p)
{
  ARRAYDAT *ans = p->ans;
  ARRAYDAT *l   = p->right;
  MYFLT r       = *p->left;
  int32_t sizel = l->sizes[0];
  int32_t i;

  if (UNLIKELY(ans->data == NULL || l->data== NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

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
int32_t tabaipow(CSOUND *csound, TABARITH1 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabiapow(CSOUND *csound, TABARITH2 *p)
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
                             "%s", Str("array-variable not initialised"));

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

//a[]+a[]
int32_t tabaadd(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabasub(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabamul(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabadiv(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabkamult(CSOUND *csound, TABARITH2 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabakmult(CSOUND *csound, TABARITH1 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabkaadd(CSOUND *csound, TABARITH2 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabakadd(CSOUND *csound, TABARITH1 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabkasub(CSOUND *csound, TABARITH2 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabkadiv(CSOUND *csound, TABARITH2 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabakdiv(CSOUND *csound, TABARITH1 *p)
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
                             "%s", Str("array-variable not initialised"));
  if (UNLIKELY(l==FL(0.0)))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("division by zero in array-var"));

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
int32_t tabarkrem(CSOUND *csound, TABARITH1 *p)
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
                             "%s", Str("array-variable not initialised"));
  if (UNLIKELY(l==FL(0.0)))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("division by zero in array-var"));

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
int32_t tabarkpow(CSOUND *csound, TABARITH1 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabaardd(CSOUND *csound, TABARITH2 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabaarsb(CSOUND *csound, TABARITH2 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabaarml(CSOUND *csound, TABARITH2 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabaardv(CSOUND *csound, TABARITH2 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabaradd(CSOUND *csound, TABARITH1 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabarasb(CSOUND *csound, TABARITH1 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabaraml(CSOUND *csound, TABARITH1 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabaradv(CSOUND *csound, TABARITH1 *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabkrardd(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabkrarsb(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabkrarml(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabkrardv(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabkrarmd(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabarkrdd(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabarkrsb(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabarkrml(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabarkrdv(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabarkrmd(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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
int32_t tabarkrpw(CSOUND *csound, TABARITH *p)
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
                             "%s", Str("array-variable not initialised"));

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


#define IIARRAY(opcode,fn)                              \
  int32_t opcode(CSOUND *csound, TABARITH *p)    \
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


#define IiARRAY(opcode,fn)                              \
  int32_t opcode(CSOUND *csound, TABARITH1 *p)   \
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
  int32_t opcode(CSOUND *csound, TABARITH2 *p)   \
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


int32_t tabqset(CSOUND *csound, TABQUERY *p)
{
  if (LIKELY(p->tab->data)) return OK;
  return csound->InitError(csound, "%s", Str("array-variable not initialised"));
}

int32_t tabqset1(CSOUND *csound, TABQUERY1 *p)
{
  if (LIKELY(p->tab->data)) return OK;
  return csound->InitError(csound, "%s", Str("array-variable not initialised"));
}

int32_t tabmax(CSOUND *csound, TABQUERY *p)
{
  ARRAYDAT *t = p->tab;
  int32_t i, size = 0, pos = 0;;
  MYFLT ans;

  if (UNLIKELY(t->data == NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));
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

int32_t tabmax1(CSOUND *csound, TABQUERY *p)
{
  if (tabqset(csound, p) == OK) return tabmax(csound, p);
  else return NOTOK;
}

int32_t tabmin(CSOUND *csound, TABQUERY *p)
{
  ARRAYDAT *t = p->tab;
  int32_t i, size = 0, pos = 0;
  MYFLT ans;

  if (UNLIKELY(t->data == NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));
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

int32_t tabmin1(CSOUND *csound, TABQUERY *p)
{
  if (LIKELY(tabqset(csound, p) == OK)) return tabmin(csound, p);
  else return NOTOK;
}


int32_t tabsuma(CSOUND *csound, TABQUERY1 *p)
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
                             "%s", Str("array-variable not initialised"));
  if (UNLIKELY(t->dimensions!=1))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not a vector"));


  if (UNLIKELY(offset)) memset(ans, '\0', offset*sizeof(MYFLT));
  if (UNLIKELY(early)) {
    nsmps -= early;
    memset(&ans[nsmps], '\0', early*sizeof(MYFLT));
  }

  for (i=0; i<t->dimensions; i++) numarrays += t->sizes[i];

  memset(&ans[offset], '\0', nsmps*sizeof(MYFLT));

  int32_t numarrays4 = numarrays - (numarrays % 4);

  for (i=0; i<numarrays4; i+=4) {
    in0 = &(t->data[i*span]);
    in1 = &(t->data[(i+1)*span]);
    in2 = &(t->data[(i+2)*span]);
    in3 = &(t->data[(i+3)*span]);

    for(int32_t j=offset; j < nsmps; j++) {
      ans[j] += in0[j] + in1[j] + in2[j] + in3[j];
    }
  }

  for (i=numarrays4;i<numarrays; i++) {
    in0 = &(t->data[i*span]);
    for(int32_t j=offset; j < nsmps; j++) {
      ans[j] += in0[j];
    }
  }
  return OK;
}

int32_t tabclearset(CSOUND *csound, TABCLEAR *p)
{
  if (LIKELY(p->tab->data)) return OK;
  return csound->InitError(csound, "%s", Str("array-variable not initialised"));
}


int32_t tabclear(CSOUND *csound, TABCLEAR *p)
{
  ARRAYDAT *t = p->tab;
  int32_t i;
  int32_t nsmps = CS_KSMPS;
  int32_t size = 1;

  if (UNLIKELY(t->data == NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));
  for(i = 0; i < t->dimensions; i++) size *= t->sizes[i];
  memset(t->data, 0, sizeof(MYFLT)*nsmps*size);

  return OK;
}


int32_t tabcleark(CSOUND *csound, TABCLEAR *p)
{
  ARRAYDAT *t = p->tab;
  int32_t i;
  int32_t size = 1;

  if (UNLIKELY(t->data == NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));

  for(i = 0; i < t->dimensions; i++) size *= t->sizes[i];
  memset(t->data, 0, sizeof(MYFLT)*size);

  return OK;
}



int32_t tabsum(CSOUND *csound, TABQUERY1 *p)
{
  ARRAYDAT *t = p->tab;
  int32_t i, size = 0;
  MYFLT ans;

  if (UNLIKELY(t->data == NULL))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not initialised"));
  if (UNLIKELY(t->dimensions!=1))
    return csound->PerfError(csound, &(p->h),
                             "%s", Str("array-variable not a vector"));
  ans = t->data[0];
  for (i=0; i<t->dimensions; i++) size += t->sizes[i];
  for (i=1; i<size; i++)
    ans += t->data[i];
  *p->ans = ans;
  return OK;
}

int32_t tabsuma1(CSOUND *csound, TABQUERY1 *p)
{
  if (LIKELY(tabqset1(csound, p) == OK)) return tabsuma(csound, p);
  else return NOTOK;
}

int32_t tabsum1(CSOUND *csound, TABQUERY1 *p)
{
  if (LIKELY(tabqset1(csound, p) == OK)) return tabsum(csound, p);
  else return NOTOK;
}

int32_t tabscaleset(CSOUND *csound, TABSCALE *p)
{
  if (LIKELY(p->tab->data && p->tab->dimensions==1)) return OK;
  return csound->InitError(csound, "%s", Str("array-variable not initialised"));
}

int32_t tabscale(CSOUND *csound, TABSCALE *p)
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
  range = (max-min)/(tmax-tmin);
  for (i=strt; i<end; i++) {
    t->data[i] = (t->data[i]-tmin)*range + min;
  }
  return OK;
}

int32_t tabscale1(CSOUND *csound, TABSCALE *p)
{
  if (LIKELY(tabscaleset(csound, p) == OK)) return tabscale(csound, p);
  else return NOTOK;
}



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

int32_t tabcopy(CSOUND *csound, TABCPY *p)
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
                                 (void*)(p->src->data + index), p->h.insdshead);
  }

  return OK;
}

int32_t tabcopyk_init(CSOUND *csound, TABCPY *p) {
  tabinit(csound, p->dst, get_array_total_size(p->src), p->h.insdshead);
  return OK;
}

int32_t tabcopyk(CSOUND *csound, TABCPY *p)
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
                                 (void*)(p->src->data + index), p->h.insdshead);
  }

  return OK;
}


int32_t tabcopy1(CSOUND *csound, TABCPY *p)
{
  int32_t i, arrayTotalSize, memMyfltSize;

  if (UNLIKELY(p->src->data==NULL) || p->src->dimensions <= 0 )
    return csound->InitError(csound, "%s", Str("array-variable not initialised"));
  if (p->dst->dimensions > 0 && p->src->dimensions != p->dst->dimensions)
    return csound->InitError(csound, "%s",
                             Str("array-variable dimensions do not match"));

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
                                 (void*)(p->src->data + index),  p->h.insdshead);
  }

  return OK;
}

int32_t tabcopy2(CSOUND *csound, TABCPY *p)
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

int32_t tab2ftab(CSOUND *csound, TABCOPY *p)
{
  FUNC        *ftp;
  int32_t fsize;
  MYFLT *fdata;
  ARRAYDAT *t = p->tab;
  int32_t i, tlen = 0;

  if (UNLIKELY(p->tab->data==NULL))
    return csound->PerfError(csound,
                             &(p->h), "%s", Str("array-var not initialised"));
  if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL))
    return csound->PerfError(csound,
                             &(p->h), "%s", Str("No table for copy2ftab"));
  for (i=0; i<t->dimensions; i++) tlen += t->sizes[i];
  fsize = ftp->flen;
  fdata = ftp->ftable;
  if (fsize<tlen) tlen = fsize;
  memcpy(fdata, p->tab->data, sizeof(MYFLT)*tlen);
  return OK;
}

int32_t tab2ftabi(CSOUND *csound, TABCOPY *p)
{
  FUNC        *ftp;
  int32_t fsize;
  MYFLT *fdata;
  ARRAYDAT *t = p->tab;
  int32_t i, tlen = 0;

  if (UNLIKELY(p->tab->data==NULL))
    return csound->InitError(csound,  "%s", Str("array-var not initialised"));
  if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL))
    return csound->InitError(csound, "%s", Str("No table for copy2ftab"));
  for (i=0; i<t->dimensions; i++) tlen += t->sizes[i];
  fsize = ftp->flen;
  fdata = ftp->ftable;
  if (fsize<tlen) tlen = fsize;
  memcpy(fdata, p->tab->data, sizeof(MYFLT)*tlen);
  return OK;
}

// copya2ftab k[], iftable, [, koffset=0]
int32_t tab2ftab_offset(CSOUND *csound, TABCOPY2 *p)
{
  FUNC *ftp;
  int32_t fsize;
  MYFLT *fdata;
  ARRAYDAT *t = p->tab;
  int32_t offset = (int)(*p->offset);
  int32_t i, tlen = 0, maxitems;
  if (UNLIKELY(t->data==NULL))
    return csound->PerfError(csound, &(p->h), "%s", Str("array-var not initialised"));
  if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL))
    return csound->PerfError(csound, &(p->h), "%s", Str("No table for copy2ftab"));
  fsize = ftp->flen;
  if (UNLIKELY(offset >= fsize || offset < 0))
    return csound->PerfError(csound, &(p->h), "%s", Str("Offset is out of bounds"));
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
int32_t tab2ftab_offset_i(CSOUND *csound, TABCOPY2 *p)
{
  FUNC *ftp;
  int32_t fsize;
  MYFLT *fdata;
  ARRAYDAT *t = p->tab;
  int32_t offset = (int)*p->offset;;
  int32_t i, tlen = 0, maxitems;
  if (UNLIKELY(t->data==NULL))
    return csound->InitError(csound, "%s",  Str("array-var not initialised"));
  if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL))
    return csound->InitError(csound, "%s",  Str("No table for copy2ftab"));
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

int32_t tabgen(CSOUND *csound, TABGEN *p)
{
  MYFLT *data =  p->tab->data;
  MYFLT start = *p->start;
  MYFLT end   = *p->end;
  MYFLT incr  = *p->incr;
  int32_t i, size =  (end - start)/incr + 1;

  if (UNLIKELY(size < 0))
    return
      csound->InitError(csound, "%s",
                        Str("inconsistent start, end and increment parameters"));
  tabinit(csound, p->tab, size, p->h.insdshead);
  if (UNLIKELY(p->tab->data==NULL)) {
    tabinit(csound, p->tab, size, p->h.insdshead);
  }

  data =  p->tab->data;
  for (i=0; i < size; i++) {
    data[i] = start;
    start += incr;
  }

  return OK;
}


int32_t ftab2tabi(CSOUND *csound, TABCOPY *p)
{
  FUNC        *ftp;
  int32_t         fsize;
  MYFLT       *fdata;
  int32_t tlen;

  if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL))
    return csound->InitError(csound, "%s", Str("No table for copy2ftab"));
  fsize = ftp->flen;
  if (UNLIKELY(p->tab->data==NULL)) {
    tabinit(csound, p->tab, fsize, p->h.insdshead);
    p->tab->sizes[0] = fsize;
  }
  tlen = p->tab->sizes[0];
  fdata = ftp->ftable;
  if (fsize<tlen) tlen = fsize;
  memcpy(p->tab->data, fdata, sizeof(MYFLT)*tlen);
  return OK;
}

int32_t ftab2tab(CSOUND *csound, TABCOPY *p)
{
  FUNC        *ftp;
  int32_t      fsize;
  MYFLT       *fdata;
  int32_t      tlen;

  if (UNLIKELY((ftp = csound->FTFind(csound, p->kfn)) == NULL))
    return csound->PerfError(csound,
                             &(p->h), "%s", Str("No table for copy2ftab"));
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

int32_t trim_i(CSOUND *csound, TRIM *p)
{
  int32_t size = (int)(*p->size);
  tabinit(csound, p->tab, size, p->h.insdshead);
  p->tab->sizes[0] = size;
  return OK;
}

int32_t trim(CSOUND *csound, TRIM *p)
{
  int32_t size = (int)(*p->size);
  int32_t n = tabcheck(csound, p->tab, size, &(p->h));
  if (n != OK) return n;
  p->tab->sizes[0] = size;
  return OK;
}


int32_t tabslice(CSOUND *csound, TABSLICE *p) {

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
  tabinit(csound, p->tab, size, p->h.insdshead);

  for (i = start, destIndex = 0; i < end + 1; i+=inc, destIndex++) {
    p->tab->arrayType->copyValue(csound, p->tab->arrayType,
                                 p->tab->data + (destIndex * memMyfltSize),
                                 tabin + (memMyfltSize * i),  p->h.insdshead);
  }

  return OK;
}


int32_t tabmap_set(CSOUND *csound, TABMAP *p)
{
  MYFLT *data, *tabin = p->tabin->data;
  int32_t n, size;
  const OENTRY *opc  = NULL;
  AEVAL  eval;

  if (UNLIKELY(p->tabin->data == NULL)||p->tabin->dimensions!=1)
    return csound->InitError(csound, "%s", Str("array-var not initialised"));

  size = p->tabin->sizes[0];
  if (UNLIKELY(p->tab->data==NULL)) {
    tabinit(csound, p->tab, size, p->h.insdshead);
    p->tab->sizes[0] = size;
  }
  else size = size < p->tab->sizes[0] ? size : p->tab->sizes[0];
  data =  p->tab->data;


  opc = csound->FindOpcode(csound, 1, p->str->data, "i", "i");
  if (UNLIKELY(opc == NULL))
    return csound->InitError(csound,  Str("%s not found"), p->str->data);
  p->opc = opc;
  for (n=0; n < size; n++) {
    eval.a = &tabin[n];
    eval.r = &data[n];
    opc->init(csound, (void *) &eval);
  }

  opc = csound->FindOpcode(csound, 1, p->str->data, "k", "k");
  p->opc = opc;
  return OK;
}

int32_t tabmap_perf(CSOUND *csound, TABMAP *p)
{
  /* FIXME; eeds check */
  MYFLT *data =  p->tab->data, *tabin = p->tabin->data;
  int32_t n, size;

  const OENTRY *opc  = p->opc;
  AEVAL  eval;

  if (UNLIKELY(p->tabin->data == NULL) || p->tabin->dimensions !=1)
    return csound->PerfError(csound,
                             &(p->h), "%s", Str("array-var not initialised"));
  if (UNLIKELY(p->tab->data==NULL) || p->tab->dimensions !=1)
    return csound->PerfError(csound,
                             &(p->h), "%s", Str("array-var not initialised"));
  size = p->tab->sizes[0];

  if (UNLIKELY(opc == NULL))
    return csound->PerfError(csound,
                             &(p->h), "%s", Str("map fn not found at k rate"));
  for (n=0; n < size; n++) {
    eval.a = &tabin[n];
    eval.r = &data[n];
    opc->perf(csound, (void *) &eval);
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

int32_t asig2array_init(CSOUND *csound, A2ARR *p) {
  int32_t nsmps = CS_KSMPS;
  tabinit(csound, p->res, nsmps, p->h.insdshead);
  return OK;
}

int32_t asig2array_perf(CSOUND *csound, A2ARR *p) {
  size_t bytes = CS_KSMPS*sizeof(MYFLT);
  memcpy(p->res->data, p->asig, bytes);
  return OK;
}



int32_t array2asig_perf(CSOUND *csound, ARR2A *p) {
  size_t bytes = CS_KSMPS*sizeof(MYFLT);
  size_t arrs = p->karr->sizes[0]*sizeof(MYFLT);
  if(bytes > arrs) memset(p->asig, 0, bytes);
  memcpy(p->asig, p->karr->data, bytes < arrs ? bytes : arrs);
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

int32_t taninv2_Ai(CSOUND* csound, TABARITH* p)
{
  ARRAYDAT* ans = p->ans;
  ARRAYDAT* aa = p->left;
  ARRAYDAT* bb = p->right;
  int32_t i, j, k;
  if (tabarithset(csound, p)!=OK)
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

int32_t taninv2_A(CSOUND* csound, TABARITH* p)
{
  ARRAYDAT* ans = p->ans;
  ARRAYDAT* aa = p->left;
  ARRAYDAT* bb = p->right;
  int32_t i, j, k;
  k = 0;
  for (i=0; i<ans->dimensions; i++) {
    for (j=0; j<aa->sizes[i]; j++) {
      ans->data[k] = ATAN2(aa->data[k], bb->data[k]);
      k++;
    }
  }
  return OK;
}

int32_t taninv2_Aa(CSOUND* csound, TABARITH* p)
{
  ARRAYDAT* ans = p->ans;
  ARRAYDAT* aa = p->left;
  ARRAYDAT* bb = p->right;
  int32_t i, j, k;
  uint32_t m;
  k = 0;
  for (i=0; i<ans->dimensions; i++) {
    for (j=0; j<aa->sizes[i]; j++)
      for (m=0; m<CS_KSMPS; m++) {
        ans->data[k] = ATAN2(aa->data[k], bb->data[k]);
        k++;
      }
  }
  return OK;
}
