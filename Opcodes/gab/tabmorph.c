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
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
  02111-1307 USA
*/
//#include "csdl.h"
#include "csoundCore.h"
#include "interlocks.h"

typedef struct {
        OPDS    h;
        MYFLT   *out, *xindex, *xinterpoint, *xtabndx1, *xtabndx2,
                *argums[VARGMAX];
        MYFLT   *table[VARGMAX];
        int     length;
        long    numOfTabs;
} TABMORPH;

static int tabmorph_set (CSOUND *csound, TABMORPH *p) /*Gab 13-March-2005 */
{
    int numOfTabs,j;
    MYFLT **argp, *first_table = NULL;

    FUNC *ftp;
    long flength = 0;

    numOfTabs = p->numOfTabs =((p->INCOUNT-4)); /* count segs & alloc if nec */
    argp = p->argums;
    for (j=0; j< numOfTabs; j++) {
      if (UNLIKELY((ftp = csound->FTnp2Find(csound, *argp++)) == NULL))
        return csound->InitError(csound, Str("tabmorph: invalid table number"));
      if (UNLIKELY(ftp->flen != flength && flength  != 0))
        return
          csound->InitError(csound,
                            Str("tabmorph: all tables must have the "
                                "same length!"));
      flength = ftp->flen;
      if (j==0) first_table = ftp->ftable;
      p->table[j] = ftp->ftable;
    }
    p->table[j] = first_table; /* for interpolation */
    p->length = flength;
    return OK;
}

static int tabmorph(CSOUND *csound, TABMORPH *p)
{
    MYFLT /* index, index_frac, */ tabndx1, tabndx2, tabndx1frac, tabndx2frac;
    MYFLT tab1val1,tab1val2, tab2val1, tab2val2, interpoint, val1, val2;
    long index_int;
    int tabndx1int, tabndx2int;
    index_int = (int) *p->xindex % p->length;

    tabndx1 = *p->xtabndx1;
    tabndx1int = (int) tabndx1;
    tabndx1frac = tabndx1 - tabndx1int;
    tabndx1int %= p->numOfTabs;
    tab1val1 = (p->table[tabndx1int  ])[index_int];
    tab1val2 = (p->table[tabndx1int+1])[index_int];
    val1 = tab1val1 * (1-tabndx1frac) + tab1val2 * tabndx1frac;

    tabndx2 = *p->xtabndx2;
    tabndx2int = (int) tabndx2;
    tabndx2frac = tabndx2 - tabndx2int;
    tabndx2int %= p->numOfTabs;
    tab2val1 = (p->table[tabndx2int  ])[index_int];
    tab2val2 = (p->table[tabndx2int+1])[index_int];
    val2 = tab2val1 * (1-tabndx2frac) + tab2val2 * tabndx2frac;

    interpoint = *p->xinterpoint;
    interpoint = (interpoint < 0 ? 0 : (interpoint > 1.0 ? 1.0 : interpoint));
    /* interpoint -= (int) interpoint;  to limit to zero to 1 range */

    *p->out = val1 * (1 - interpoint) + val2 * interpoint;
    return OK;
}

static int tabmorphi(CSOUND *csound, TABMORPH *p) /* interpolation */
{
    MYFLT index, index_frac, tabndx1, tabndx2, tabndx1frac, tabndx2frac;
    MYFLT tab1val1a,tab1val2a, tab2val1a, tab2val2a, interpoint, val1, val2;
    MYFLT val1a, val2a, val1b, val2b, tab1val1b,tab1val2b, tab2val1b, tab2val2b;
    long index_int;
    int tabndx1int, tabndx2int;

    index = *p->xindex;
    index_int = (int) index;
    index_frac = index - index_int;
    index_int %= p->length;

    tabndx1 = *p->xtabndx1;
    tabndx1int = (int) tabndx1;
    tabndx1frac = tabndx1 - tabndx1int;
    tabndx1int %= p->numOfTabs;

    tab1val1a = (p->table[tabndx1int  ])[index_int];
    tab1val2a = (p->table[tabndx1int+1])[index_int];
    val1a = tab1val1a * (1-tabndx1frac) + tab1val2a * tabndx1frac;

    tab1val1b = (p->table[tabndx1int  ])[index_int+1];
    tab1val2b = (p->table[tabndx1int+1])[index_int+1];
    val1b = tab1val1b * (1-tabndx1frac) + tab1val2b * tabndx1frac;

    val1  = val1a + (val1b-val1a) * index_frac;
    /*--------------*/
    tabndx2 = *p->xtabndx2;
    tabndx2int = (int) tabndx2;
    tabndx2frac = tabndx2 - tabndx2int;
    tabndx2int %= p->numOfTabs;

    tab2val1a = (p->table[tabndx2int  ])[index_int];
    tab2val2a = (p->table[tabndx2int+1])[index_int];
    val2a = tab2val1a * (1-tabndx2frac) + tab2val2a * tabndx2frac;

    tab2val1b = (p->table[tabndx2int  ])[index_int+1];
    tab2val2b = (p->table[tabndx2int+1])[index_int+1];
    val2b = tab2val1b * (1-tabndx2frac) + tab2val2b * tabndx2frac;

    val2  = val2a + (val2b-val2a) * index_frac;

    interpoint = *p->xinterpoint;
    interpoint -= (int) interpoint; /* to limit to zero to 1 range */

    *p->out = val1 * (1 - interpoint) + val2 * interpoint;
    return OK;
}


static int atabmorphia(CSOUND *csound, TABMORPH *p) /* all arguments at a-rate */
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int      tablen = p->length;
    MYFLT *out = p->out;
    MYFLT *index = p->xindex;
    MYFLT *interpoint = p->xinterpoint;
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
      long index_int;
      int tabndx1int, tabndx2int;
      MYFLT ndx = index[n] * tablen;
      index_int = (int) ndx;
      index_frac = ndx - index_int;
      index_int %= tablen;

      tabndx1int = (int) tabndx1[n];
      tabndx1frac = tabndx1[n] - tabndx1int;
      tabndx1int %= p->numOfTabs;

      tab1val1a = (p->table[tabndx1int  ])[index_int];
      tab1val2a = (p->table[tabndx1int+1])[index_int];
      val1a = tab1val1a * (1-tabndx1frac) + tab1val2a * tabndx1frac;

      tab1val1b = (p->table[tabndx1int  ])[index_int+1];
      tab1val2b = (p->table[tabndx1int+1])[index_int+1];
      val1b = tab1val1b * (1-tabndx1frac) + tab1val2b * tabndx1frac;

      val1  = val1a + (val1b-val1a) * index_frac;
      /*--------------*/

      tabndx2int = (int) tabndx2[n];
      tabndx2frac = tabndx2[n] - tabndx2int;
      tabndx2int %= p->numOfTabs;

      tab2val1a = (p->table[tabndx2int  ])[index_int];
      tab2val2a = (p->table[tabndx2int+1])[index_int];
      val2a = tab2val1a * (1-tabndx2frac) + tab2val2a * tabndx2frac;

      tab2val1b = (p->table[tabndx2int  ])[index_int+1];
      tab2val2b = (p->table[tabndx2int+1])[index_int+1];
      val2b = tab2val1b * (1-tabndx2frac) + tab2val2b * tabndx2frac;

      val2  = val2a + (val2b-val2a) * index_frac;

      interpoint[n] -= (int) interpoint[n]; /* to limit to zero to 1 range */

      out[n] = val1 * (1 - interpoint[n]) + val2 * interpoint[n];
    }
    return OK;
}


 /* all args k-rate except out and table index */
static int atabmorphi(CSOUND *csound, TABMORPH *p)
{
    uint32_t offset = p->h.insdshead->ksmps_offset;
    uint32_t early  = p->h.insdshead->ksmps_no_end;
    uint32_t n, nsmps = CS_KSMPS;
    int      tablen = p->length;

    MYFLT *out = p->out;
    MYFLT *index;
    MYFLT tabndx1, tabndx2, tabndx1frac, tabndx2frac, interpoint;
    int tabndx1int, tabndx2int;

    tabndx1 = *p->xtabndx1;
    tabndx1int = (int) tabndx1;
    tabndx1frac = tabndx1 - tabndx1int;
    tabndx1int %= p->numOfTabs;
    index = p->xindex;


    /*--------------*/
    tabndx2 = *p->xtabndx2;
    tabndx2int = (int) tabndx2;
    tabndx2frac = tabndx2 - tabndx2int;
    tabndx2int %= p->numOfTabs;

    interpoint = *p->xinterpoint;
    interpoint -= (int) interpoint; /* to limit to zero to 1 range */

    if (UNLIKELY(offset)) memset(out, '\0', offset*sizeof(MYFLT));
    if (UNLIKELY(early)) {
      nsmps -= early;
      memset(&out[nsmps], '\0', early*sizeof(MYFLT));
    }
    for (n=offset; n<nsmps; n++) {
      MYFLT index_frac, tab1val1a,tab1val2a, tab2val1a, tab2val2a, val1, val2;
      MYFLT val1a, val2a, val1b, val2b, tab1val1b,tab1val2b, tab2val1b, tab2val2b;
      long index_int;
      MYFLT ndx = index[n] * tablen;

      index_int = (int) ndx;
      index_frac = ndx - index_int;
      index_int %= tablen;


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

{ "tabmorph",  S(TABMORPH), TR, 3,  "k", "kkkkm",
               (SUBR) tabmorph_set, (SUBR) tabmorph, NULL},
{ "tabmorphi", S(TABMORPH), TR, 3,  "k", "kkkkm",
               (SUBR) tabmorph_set, (SUBR) tabmorphi, NULL},
{ "tabmorpha", S(TABMORPH), TR, 5,  "a", "aaaam",
               (SUBR) tabmorph_set,  NULL, (SUBR) atabmorphia},
{ "tabmorphak",S(TABMORPH), TR, 5,  "a", "akkkm",
               (SUBR) tabmorph_set,  NULL, (SUBR) atabmorphi }

};

int tabmorph_init_(CSOUND *csound) {
    return
      csound->AppendOpcodes(csound, &(tabmoroph_localops[0]),
                            (int) (sizeof(tabmoroph_localops) / sizeof(OENTRY)));
}
