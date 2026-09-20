/*  Copyright (C) 2007 Gabriel Maldonado

  Csound is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  Csound is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with Csound; if not, write to the Free Software
  Foundation, Inc., 31 Milk Street, #960789, Boston, MA, 02196, USA
*/
//#include "csdl.h"
#ifdef BUILD_PLUGINS
#include "csdl.h"
#else
#include "csoundCore.h"
#endif
#include "interlocks.h"

#define TABMORPH_WEIGHT(x) \
    ((x) < FL(0.0) ? FL(0.0) : ((x) > FL(1.0) ? FL(1.0) : (x)))

/* Keep the usual cast/modulo path; reduce other values before conversion. */
#define TABMORPH_INDEX(value, length, integer, fraction) do { \
    double tm_index = (double)(value); \
    if (UNLIKELY(!(tm_index >= 0.0 && tm_index < 2147483648.0))) { \
      tm_index = fmod(tm_index, (double)(length)); \
      if (tm_index < 0.0) tm_index += (double)(length); \
      if (!(tm_index >= 0.0 && tm_index < (double)(length))) tm_index = 0.0; \
    } \
    (integer) = (int32_t)tm_index; \
    (fraction) = (MYFLT)(tm_index - (integer)); \
    (integer) %= (length); \
} while (0)

typedef struct {
        OPDS    h;
        MYFLT   *out, *xindex, *xinterpoint, *xtabndx1, *xtabndx2,
                *argums[VARGMAX];
        MYFLT   *table[VARGMAX];
        int32_t     length;
        int64_t    numOfTabs;
} TABMORPH;

typedef struct {
    OPDS    h;
    MYFLT   *out, *xindex, *xinterpoint, *xtabndx1, *xtabndx2;
    ARRAYDAT *table_array;  // Array input
    MYFLT   *table[VARGMAX];
    int32_t     length;
    int64_t    numOfTabs;
} TABMORPH_ARR;

static int32_t tabmorph_set (CSOUND *csound, TABMORPH *p) /*Gab 13-March-2005 */
{
    int32_t numOfTabs,j;
    MYFLT **argp, *first_table = NULL;

    FUNC *ftp;
    int64_t flength = 0;

    numOfTabs = p->numOfTabs =((p->INOCOUNT-4)); /* count segs & alloc if nec */
    if (UNLIKELY(numOfTabs < 1 || numOfTabs >= VARGMAX))
      return csound->InitError(csound, "%s",
                               Str("tabmorph: table count out of range"));
    argp = p->argums;
    for (j=0; j< numOfTabs; j++) {
      if (UNLIKELY((ftp = csound->FTFind(csound, *argp++)) == NULL))
        return csound->InitError(csound, "%s", Str("tabmorph: invalid table number"));
      if (UNLIKELY(ftp->flen == 0 || ftp->flen > INT32_MAX))
        return csound->InitError(csound, "%s", Str("tabmorph: invalid table length"));
      if (UNLIKELY(ftp->flen != flength && flength  != 0))
        return
          csound->InitError(csound,
                            "%s", Str("tabmorph: all tables must have the "
                                "same length!"));
      flength = ftp->flen;
      if (j==0) first_table = ftp->ftable;
      p->table[j] = ftp->ftable;
    }
    p->table[j] = first_table; /* for interpolation */
    p->length = (int32_t) flength;
    return OK;
}

static int32_t tabmorph(CSOUND *csound, TABMORPH *p)
{
    IGN(csound);
    MYFLT /* index, index_frac, */ tabndx1, tabndx2, tabndx1frac, tabndx2frac;
    MYFLT tab1val1,tab1val2, tab2val1, tab2val2, interpoint, val1, val2;
    int64_t index_int;
    int32_t tabndx1int, tabndx2int;
    MYFLT index_frac;
    TABMORPH_INDEX(*p->xindex, p->length, index_int, index_frac);
    IGN(index_frac);

    tabndx1 = *p->xtabndx1;
    TABMORPH_INDEX(tabndx1, p->numOfTabs, tabndx1int, tabndx1frac);
    tab1val1 = (p->table[tabndx1int  ])[index_int];
    tab1val2 = (p->table[tabndx1int+1])[index_int];
    val1 = tab1val1 * (1-tabndx1frac) + tab1val2 * tabndx1frac;

    tabndx2 = *p->xtabndx2;
    TABMORPH_INDEX(tabndx2, p->numOfTabs, tabndx2int, tabndx2frac);
    tab2val1 = (p->table[tabndx2int  ])[index_int];
    tab2val2 = (p->table[tabndx2int+1])[index_int];
    val2 = tab2val1 * (1-tabndx2frac) + tab2val2 * tabndx2frac;

    interpoint = *p->xinterpoint;
    interpoint = TABMORPH_WEIGHT(interpoint);

    *p->out = val1 * (1 - interpoint) + val2 * interpoint;
    return OK;
}

static int32_t tabmorphi(CSOUND *csound, TABMORPH *p) /* interpolation */
{
    IGN(csound);
    MYFLT index, index_frac, tabndx1, tabndx2, tabndx1frac, tabndx2frac;
    MYFLT tab1val1a,tab1val2a, tab2val1a, tab2val2a, interpoint, val1, val2;
    MYFLT val1a, val2a, val1b, val2b, tab1val1b,tab1val2b, tab2val1b, tab2val2b;
    int64_t index_int;
    int32_t tabndx1int, tabndx2int;

    index = *p->xindex;
    TABMORPH_INDEX(index, p->length, index_int, index_frac);

    tabndx1 = *p->xtabndx1;
    TABMORPH_INDEX(tabndx1, p->numOfTabs, tabndx1int, tabndx1frac);

    tab1val1a = (p->table[tabndx1int  ])[index_int];
    tab1val2a = (p->table[tabndx1int+1])[index_int];
    val1a = tab1val1a * (1-tabndx1frac) + tab1val2a * tabndx1frac;

    tab1val1b = (p->table[tabndx1int  ])[index_int+1];
    tab1val2b = (p->table[tabndx1int+1])[index_int+1];
    val1b = tab1val1b * (1-tabndx1frac) + tab1val2b * tabndx1frac;

    val1  = val1a + (val1b-val1a) * index_frac;
    /*--------------*/
    tabndx2 = *p->xtabndx2;
    TABMORPH_INDEX(tabndx2, p->numOfTabs, tabndx2int, tabndx2frac);

    tab2val1a = (p->table[tabndx2int  ])[index_int];
    tab2val2a = (p->table[tabndx2int+1])[index_int];
    val2a = tab2val1a * (1-tabndx2frac) + tab2val2a * tabndx2frac;

    tab2val1b = (p->table[tabndx2int  ])[index_int+1];
    tab2val2b = (p->table[tabndx2int+1])[index_int+1];
    val2b = tab2val1b * (1-tabndx2frac) + tab2val2b * tabndx2frac;

    val2  = val2a + (val2b-val2a) * index_frac;

    interpoint = *p->xinterpoint;
    interpoint = TABMORPH_WEIGHT(interpoint);

    *p->out = val1 * (1 - interpoint) + val2 * interpoint;
    return OK;
}


static int32_t atabmorphia(CSOUND *csound, TABMORPH *p) /* all arguments at a-rate */
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      tablen = p->length;
    MYFLT *out = p->out;
    MYFLT *index = p->xindex;
    const MYFLT *interpoint = p->xinterpoint;
    MYFLT *tabndx1 = p->xtabndx1;
    MYFLT *tabndx2 = p->xtabndx2;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT index_frac,tabndx1frac, tabndx2frac;
      MYFLT tab1val1a,tab1val2a, tab2val1a, tab2val2a, val1, val2;
      MYFLT val1a, val2a, val1b, val2b, tab1val1b,tab1val2b, tab2val1b, tab2val2b;
      int64_t index_int;
      int32_t tabndx1int, tabndx2int;
      MYFLT ndx = index[n] * tablen;
      TABMORPH_INDEX(ndx, tablen, index_int, index_frac);

      TABMORPH_INDEX(tabndx1[n], p->numOfTabs, tabndx1int, tabndx1frac);

      tab1val1a = (p->table[tabndx1int  ])[index_int];
      tab1val2a = (p->table[tabndx1int+1])[index_int];
      val1a = tab1val1a * (1-tabndx1frac) + tab1val2a * tabndx1frac;

      tab1val1b = (p->table[tabndx1int  ])[index_int+1];
      tab1val2b = (p->table[tabndx1int+1])[index_int+1];
      val1b = tab1val1b * (1-tabndx1frac) + tab1val2b * tabndx1frac;

      val1  = val1a + (val1b-val1a) * index_frac;
      /*--------------*/

      TABMORPH_INDEX(tabndx2[n], p->numOfTabs, tabndx2int, tabndx2frac);

      tab2val1a = (p->table[tabndx2int  ])[index_int];
      tab2val2a = (p->table[tabndx2int+1])[index_int];
      val2a = tab2val1a * (1-tabndx2frac) + tab2val2a * tabndx2frac;

      tab2val1b = (p->table[tabndx2int  ])[index_int+1];
      tab2val2b = (p->table[tabndx2int+1])[index_int+1];
      val2b = tab2val1b * (1-tabndx2frac) + tab2val2b * tabndx2frac;

      val2  = val2a + (val2b-val2a) * index_frac;

      MYFLT weight = TABMORPH_WEIGHT(interpoint[n]);

      out[n] = val1 * (1 - weight) + val2 * weight;
    }
    return OK;
}


 /* all args k-rate except out and table index */
static int32_t atabmorphi(CSOUND *csound, TABMORPH *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      tablen = p->length;

    MYFLT *out = p->out;
    MYFLT *index;
    MYFLT tabndx1, tabndx2, tabndx1frac, tabndx2frac, interpoint;
    int32_t tabndx1int, tabndx2int;

    tabndx1 = *p->xtabndx1;
    TABMORPH_INDEX(tabndx1, p->numOfTabs, tabndx1int, tabndx1frac);
    index = p->xindex;


    /*--------------*/
    tabndx2 = *p->xtabndx2;
    TABMORPH_INDEX(tabndx2, p->numOfTabs, tabndx2int, tabndx2frac);

    interpoint = *p->xinterpoint;
    interpoint = TABMORPH_WEIGHT(interpoint);

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT index_frac, tab1val1a,tab1val2a, tab2val1a, tab2val2a, val1, val2;
      MYFLT val1a, val2a, val1b, val2b, tab1val1b,tab1val2b, tab2val1b, tab2val2b;
      int64_t index_int;
      MYFLT ndx = index[n] * tablen;

      TABMORPH_INDEX(ndx, tablen, index_int, index_frac);


      tab1val1a = (p->table[tabndx1int  ])[index_int];
      tab1val2a = (p->table[tabndx1int+1])[index_int];
      val1a = tab1val1a * (1-tabndx1frac) + tab1val2a * tabndx1frac;

      tab1val1b = (p->table[tabndx1int  ])[index_int+1];
      tab1val2b = (p->table[tabndx1int+1])[index_int+1];
      val1b = tab1val1b * (1-tabndx1frac) + tab1val2b * tabndx1frac;

      val1  = val1a + (val1b-val1a) * index_frac;

      tab2val1a = (p->table[tabndx2int  ])[index_int];
      tab2val2a = (p->table[tabndx2int+1])[index_int];
      val2a = tab2val1a * (1-tabndx2frac) + tab2val2a * tabndx2frac;

      tab2val1b = (p->table[tabndx2int  ])[index_int+1];
      tab2val2b = (p->table[tabndx2int+1])[index_int+1];
      val2b = tab2val1b * (1-tabndx2frac) + tab2val2b * tabndx2frac;

      val2  = val2a + (val2b-val2a) * index_frac;

      out[n] = val1 * (1 - interpoint) + val2 * interpoint;
    }
    return OK;
}

//=================================================================================
static int32_t tabmorph_set_array (CSOUND *csound, TABMORPH_ARR *p) /*Gab 13-March-2005 */
{
    int32_t numOfTabs,j;
    MYFLT *first_table = NULL;
    ARRAYDAT *arr = p->table_array;
    FUNC *ftp;
    int64_t flength = 0;

    if (UNLIKELY(arr->dimensions != 1 || arr->sizes == NULL || arr->data == NULL))
      return csound->InitError(csound, "%s",
                               Str("tabmorph: expected a one-dimensional table array"));
    numOfTabs = p->numOfTabs = arr->sizes[0];
    /* One extra pointer closes the interpolation interval at the last table. */
    if (UNLIKELY(numOfTabs < 1 || numOfTabs >= VARGMAX))
      return csound->InitError(csound, "%s",
                               Str("tabmorph: table count out of range"));
    MYFLT *table_nums = arr->data;
    for (j=0; j< numOfTabs; j++) {
        MYFLT table = table_nums[j];
        if (UNLIKELY((ftp = csound->FTFind(csound, &table)) == NULL))
        return csound->InitError(csound, "%s", Str("tabmorph: invalid table number"));
      if (UNLIKELY(ftp->flen == 0 || ftp->flen > INT32_MAX))
        return csound->InitError(csound, "%s", Str("tabmorph: invalid table length"));
      if (UNLIKELY(ftp->flen != flength && flength  != 0))
        return
          csound->InitError(csound,
                            "%s", Str("tabmorph: all tables must have the "
                                "same length!"));
      flength = ftp->flen;
      if (j==0) first_table = ftp->ftable;
      p->table[j] = ftp->ftable;
    }
    p->table[j] = first_table; /* for interpolation */
    p->length = (int32_t) flength;
    return OK;
}

static int32_t tabmorph_array(CSOUND *csound, TABMORPH_ARR *p)
{
    IGN(csound);
    MYFLT /* index, index_frac, */ tabndx1, tabndx2, tabndx1frac, tabndx2frac;
    MYFLT tab1val1,tab1val2, tab2val1, tab2val2, interpoint, val1, val2;
    int64_t index_int;
    int32_t tabndx1int, tabndx2int;
    MYFLT index_frac;
    TABMORPH_INDEX(*p->xindex, p->length, index_int, index_frac);
    IGN(index_frac);

    tabndx1 = *p->xtabndx1;
    TABMORPH_INDEX(tabndx1, p->numOfTabs, tabndx1int, tabndx1frac);
    tab1val1 = (p->table[tabndx1int  ])[index_int];
    tab1val2 = (p->table[tabndx1int+1])[index_int];
    val1 = tab1val1 * (1-tabndx1frac) + tab1val2 * tabndx1frac;

    tabndx2 = *p->xtabndx2;
    TABMORPH_INDEX(tabndx2, p->numOfTabs, tabndx2int, tabndx2frac);
    tab2val1 = (p->table[tabndx2int  ])[index_int];
    tab2val2 = (p->table[tabndx2int+1])[index_int];
    val2 = tab2val1 * (1-tabndx2frac) + tab2val2 * tabndx2frac;

    interpoint = *p->xinterpoint;
    interpoint = TABMORPH_WEIGHT(interpoint);

    *p->out = val1 * (1 - interpoint) + val2 * interpoint;
    return OK;
}

static int32_t tabmorphi_array(CSOUND *csound, TABMORPH_ARR *p) /* interpolation */
{
    IGN(csound);
    MYFLT index, index_frac, tabndx1, tabndx2, tabndx1frac, tabndx2frac;
    MYFLT tab1val1a,tab1val2a, tab2val1a, tab2val2a, interpoint, val1, val2;
    MYFLT val1a, val2a, val1b, val2b, tab1val1b,tab1val2b, tab2val1b, tab2val2b;
    int64_t index_int;
    int32_t tabndx1int, tabndx2int;

    index = *p->xindex;
    TABMORPH_INDEX(index, p->length, index_int, index_frac);

    tabndx1 = *p->xtabndx1;
    TABMORPH_INDEX(tabndx1, p->numOfTabs, tabndx1int, tabndx1frac);

    tab1val1a = (p->table[tabndx1int  ])[index_int];
    tab1val2a = (p->table[tabndx1int+1])[index_int];
    val1a = tab1val1a * (1-tabndx1frac) + tab1val2a * tabndx1frac;

    tab1val1b = (p->table[tabndx1int  ])[index_int+1];
    tab1val2b = (p->table[tabndx1int+1])[index_int+1];
    val1b = tab1val1b * (1-tabndx1frac) + tab1val2b * tabndx1frac;

    val1  = val1a + (val1b-val1a) * index_frac;
    /*--------------*/
    tabndx2 = *p->xtabndx2;
    TABMORPH_INDEX(tabndx2, p->numOfTabs, tabndx2int, tabndx2frac);

    tab2val1a = (p->table[tabndx2int  ])[index_int];
    tab2val2a = (p->table[tabndx2int+1])[index_int];
    val2a = tab2val1a * (1-tabndx2frac) + tab2val2a * tabndx2frac;

    tab2val1b = (p->table[tabndx2int  ])[index_int+1];
    tab2val2b = (p->table[tabndx2int+1])[index_int+1];
    val2b = tab2val1b * (1-tabndx2frac) + tab2val2b * tabndx2frac;

    val2  = val2a + (val2b-val2a) * index_frac;

    interpoint = *p->xinterpoint;
    interpoint = TABMORPH_WEIGHT(interpoint);

    *p->out = val1 * (1 - interpoint) + val2 * interpoint;
    return OK;
}


static int32_t atabmorphia_array(CSOUND *csound, TABMORPH_ARR *p) /* all arguments at a-rate */
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      tablen = p->length;
    MYFLT *out = p->out;
    MYFLT *index = p->xindex;
    const MYFLT *interpoint = p->xinterpoint;
    MYFLT *tabndx1 = p->xtabndx1;
    MYFLT *tabndx2 = p->xtabndx2;

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT index_frac,tabndx1frac, tabndx2frac;
      MYFLT tab1val1a,tab1val2a, tab2val1a, tab2val2a, val1, val2;
      MYFLT val1a, val2a, val1b, val2b, tab1val1b,tab1val2b, tab2val1b, tab2val2b;
      int64_t index_int;
      int32_t tabndx1int, tabndx2int;
      MYFLT ndx = index[n] * tablen;
      TABMORPH_INDEX(ndx, tablen, index_int, index_frac);

      TABMORPH_INDEX(tabndx1[n], p->numOfTabs, tabndx1int, tabndx1frac);

      tab1val1a = (p->table[tabndx1int  ])[index_int];
      tab1val2a = (p->table[tabndx1int+1])[index_int];
      val1a = tab1val1a * (1-tabndx1frac) + tab1val2a * tabndx1frac;

      tab1val1b = (p->table[tabndx1int  ])[index_int+1];
      tab1val2b = (p->table[tabndx1int+1])[index_int+1];
      val1b = tab1val1b * (1-tabndx1frac) + tab1val2b * tabndx1frac;

      val1  = val1a + (val1b-val1a) * index_frac;
      /*--------------*/

      TABMORPH_INDEX(tabndx2[n], p->numOfTabs, tabndx2int, tabndx2frac);

      tab2val1a = (p->table[tabndx2int  ])[index_int];
      tab2val2a = (p->table[tabndx2int+1])[index_int];
      val2a = tab2val1a * (1-tabndx2frac) + tab2val2a * tabndx2frac;

      tab2val1b = (p->table[tabndx2int  ])[index_int+1];
      tab2val2b = (p->table[tabndx2int+1])[index_int+1];
      val2b = tab2val1b * (1-tabndx2frac) + tab2val2b * tabndx2frac;

      val2  = val2a + (val2b-val2a) * index_frac;

      MYFLT weight = TABMORPH_WEIGHT(interpoint[n]);

      out[n] = val1 * (1 - weight) + val2 * weight;
    }
    return OK;
}


 /* all args k-rate except out and table index */
static int32_t atabmorphi_array(CSOUND *csound, TABMORPH_ARR *p)
{
    IGN(csound);
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int32_t      tablen = p->length;

    MYFLT *out = p->out;
    MYFLT *index;
    MYFLT tabndx1, tabndx2, tabndx1frac, tabndx2frac, interpoint;
    int32_t tabndx1int, tabndx2int;

    tabndx1 = *p->xtabndx1;
    TABMORPH_INDEX(tabndx1, p->numOfTabs, tabndx1int, tabndx1frac);
    index = p->xindex;


    /*--------------*/
    tabndx2 = *p->xtabndx2;
    TABMORPH_INDEX(tabndx2, p->numOfTabs, tabndx2int, tabndx2frac);

    interpoint = *p->xinterpoint;
    interpoint = TABMORPH_WEIGHT(interpoint);

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT index_frac, tab1val1a,tab1val2a, tab2val1a, tab2val2a, val1, val2;
      MYFLT val1a, val2a, val1b, val2b, tab1val1b,tab1val2b, tab2val1b, tab2val2b;
      int64_t index_int;
      MYFLT ndx = index[n] * tablen;

      TABMORPH_INDEX(ndx, tablen, index_int, index_frac);


      tab1val1a = (p->table[tabndx1int  ])[index_int];
      tab1val2a = (p->table[tabndx1int+1])[index_int];
      val1a = tab1val1a * (1-tabndx1frac) + tab1val2a * tabndx1frac;

      tab1val1b = (p->table[tabndx1int  ])[index_int+1];
      tab1val2b = (p->table[tabndx1int+1])[index_int+1];
      val1b = tab1val1b * (1-tabndx1frac) + tab1val2b * tabndx1frac;

      val1  = val1a + (val1b-val1a) * index_frac;

      tab2val1a = (p->table[tabndx2int  ])[index_int];
      tab2val2a = (p->table[tabndx2int+1])[index_int];
      val2a = tab2val1a * (1-tabndx2frac) + tab2val2a * tabndx2frac;

      tab2val1b = (p->table[tabndx2int  ])[index_int+1];
      tab2val2b = (p->table[tabndx2int+1])[index_int+1];
      val2b = tab2val1b * (1-tabndx2frac) + tab2val2b * tabndx2frac;

      val2  = val2a + (val2b-val2a) * index_frac;

      out[n] = val1 * (1 - interpoint) + val2 * interpoint;
    }
    return OK;
}


#define S(x)    sizeof(x)

OENTRY tabmoroph_localops[] = {

{ "tabmorph",  S(TABMORPH), TR,   "k", "kkkkm",
               (SUBR) tabmorph_set, (SUBR) tabmorph, NULL},
{ "tabmorphi", S(TABMORPH), TR,   "k", "kkkkm",
               (SUBR) tabmorph_set, (SUBR) tabmorphi, NULL},
{ "tabmorpha", S(TABMORPH), TR,   "a", "aaaam",
               (SUBR) tabmorph_set, (SUBR) atabmorphia},
{ "tabmorphak",S(TABMORPH), TR,   "a", "akkkm",
               (SUBR) tabmorph_set, (SUBR) atabmorphi },

{ "tabmorph",  S(TABMORPH_ARR), TR,   "k", "kkkki[]",
               (SUBR) tabmorph_set_array, (SUBR) tabmorph_array, NULL},
{ "tabmorphi", S(TABMORPH_ARR), TR,   "k", "kkkki[]",
               (SUBR) tabmorph_set_array, (SUBR) tabmorphi_array, NULL},
{ "tabmorpha", S(TABMORPH_ARR), TR,   "a", "aaaai[]",
               (SUBR) tabmorph_set_array, (SUBR) atabmorphia_array},
{ "tabmorphak",S(TABMORPH_ARR), TR,   "a", "akkki[]",
               (SUBR) tabmorph_set_array, (SUBR) atabmorphi_array }
};

int32_t tabmorph_init_(CSOUND *csound) {
    return
      csound->AppendOpcodes(csound, &(tabmoroph_localops[0]),
                            (int32_t) (sizeof(tabmoroph_localops) / sizeof(OENTRY)));
}
