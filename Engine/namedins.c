/*
    namedins.c:

    Copyright (C) 2002, 2005 Istvan Varga

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

/* namedins.c -- written by Istvan Varga, Oct 2002, Jan 2005 */

#include "cs.h"
#include "namedins.h"
#include <ctype.h>

typedef struct namedInstr {
        long    instno;
        char    *name;
        INSTRTXT    *ip;
        struct namedInstr   *prv;
} INSTRNAME;

/* do not touch this ! */

const unsigned char strhash_tabl_8[256] = {
        230,  69,  87,  14,  21, 133, 233, 229,  54, 111, 196,  53, 128,  23,
         66, 225,  67,  79, 173, 110, 116,  56,  48, 129,  89, 188,  29, 251,
        186, 159, 102, 162, 227,  57, 220, 244, 165, 243, 215, 216, 214,  33,
        172, 254, 247, 241, 121, 197,  83,  22, 142,  61, 199,  50, 140, 192,
          6, 237, 183,  46, 206,  81,  18, 105, 147, 253,  15,  97, 179, 163,
        108, 123,  59, 198,  19, 141,   9,  95,  25, 219, 222,   1,   5,  52,
         90, 138,  11, 234,  55,  60, 209,  39,  80, 203, 120,   4,  64, 146,
        153, 157, 194, 134, 174, 100, 107, 125, 236, 160, 150,  41,  12, 223,
        135, 189, 122, 171,  10, 221,  71,  68, 106,  73, 218, 115,   2, 152,
        132, 190, 185, 113, 139, 104, 151, 154, 248, 117, 193, 118, 136, 204,
         17, 239, 158,  77, 103, 182, 250, 191, 170,  13,  75,  85,  62,   0,
        164,   8, 178,  93,  47,  42, 177,   3, 212, 255,  35, 137,  31, 224,
        242,  88, 161, 145,  49, 119, 143, 245, 201,  38, 211,  96, 169,  98,
         78, 195,  58, 109,  40, 238, 114,  20,  99,  24, 175, 200, 148, 112,
         45,   7,  28, 168,  27, 249,  94, 205, 156,  44,  37,  82, 217,  36,
         30,  16, 101,  72,  43, 149, 144, 187,  65, 131, 184, 166,  51,  32,
        226, 202, 231, 213, 126, 210, 235,  74, 208, 252, 181, 155, 246,  92,
         63, 228, 180, 176,  76, 167, 232,  91, 130,  84, 124,  86,  34,  26,
        207, 240, 127,  70
};

static inline unsigned char name_hash(const char *s)
{
    unsigned char *c = (unsigned char*) &(s[0]);
    unsigned char h = (unsigned char) 0;
    for ( ; *c != (unsigned char) 0; c++)
      h = strhash_tabl_8[*c ^ h];
    return h;
}

static inline int sCmp(const char *x, const char *y)
{
    int tmp = 0;
    while (x[tmp] == y[tmp] && x[tmp] != (char) 0)
      tmp++;
    return (x[tmp] != y[tmp]);
}

/* check if the string s is a valid instrument or opcode name */
/* return value is zero if the string is not a valid name */

int check_instr_name(char *s)
{
    char    *c = s;

    if (!*c) return 0;  /* empty */
    if (!isalpha(*c) && *c != '_') return 0;    /* chk if 1st char is valid */
    while (*++c)
      if (!isalnum(*c) && *c != '_') return 0;
    return 1;   /* no errors */
}

/* find the instrument number for the specified name */
/* return value is zero if none was found */

long named_instr_find(ENVIRON *csound, char *s)
{
    INSTRNAME     *inm;
    unsigned char h = name_hash(s);     /* calculate hash value */

    if (!csound->instrumentNames) return 0L;  /* no named instruments defined */
    /* now find instrument */
    if (!(inm = ((INSTRNAME**) csound->instrumentNames)[h])) return 0L;
    if (!inm->prv) return (long) inm->instno;
    /* multiple entries for hash value, need to search */
    while (inm && sCmp(inm->name, s)) inm = inm->prv;
    if (!inm)
      return 0L;        /* not found */
    else
      return (long) inm->instno;
}

/* allocate entry for named instrument ip with name s (s must not be freed */
/* after the call, because only the pointer is stored); instrument number */
/* is set to insno */
/* returns zero if the named instr entry could not be allocated */
/* (e.g. because it already exists) */

int named_instr_alloc(ENVIRON *csound, char *s, INSTRTXT *ip, long insno)
{
    INSTRNAME   **inm_base = (INSTRNAME**) csound->instrumentNames, *inm, *inm2;
    unsigned char h = name_hash(s);     /* calculate hash value */

    if (!inm_base)
      /* no named instruments defined yet */
      inm_base = csound->instrumentNames =
                 (void*) mcalloc(csound, sizeof(INSTRNAME*) * 258);
    /* now check if instrument is already defined */
    if ((inm = inm_base[h])) {
      while (inm && sCmp(inm->name, s)) inm = inm->prv;
      if (inm) return 0;        /* error: instr exists */
    }
    /* allocate entry, */
    inm = (INSTRNAME*) mcalloc(csound, sizeof(INSTRNAME));
    inm2 = (INSTRNAME*) mcalloc(csound, sizeof(INSTRNAME));
    /* and store parameters */
    inm->name = s; inm->ip = ip;
    inm2->instno = insno;
    inm2->name = (char*) inm;   /* hack */
    /* link into chain */
    inm->prv = inm_base[h];
    inm_base[h] = inm;
    /* temporary chain for use by named_instr_assign_numbers() */
    if (!inm_base[256])
      inm_base[256] = inm2;
    else
      inm_base[257]->prv = inm2;
    inm_base[257] = inm2;
    if (csound->oparms->odebug)
      csound->Message(csound,
                      "named instr name = \"%s\", hash = %d, txtp = %p\n",
                      s, (int) h, (void*) ip);
    return 1;
}

/* assign instrument numbers to all named instruments */
/* called by otran */

void named_instr_assign_numbers(ENVIRON *csound)
{
    INSTRNAME   *inm, *inm2, **inm_first, **inm_last;
    int     num = 0, insno_priority = 0;

    if (!csound->instrumentNames) return;       /* no named instruments */
    inm_first = (INSTRNAME**) csound->instrumentNames + 256;
    inm_last = inm_first + 1;
    while (--insno_priority > -3) {
      if (insno_priority == -2) {
        num = csound->maxinsno;         /* find last used instr number */
        while (!csound->instrtxtp[num] && --num);
      }
      for (inm = *inm_first; inm; inm = inm->prv) {
        if ((int) inm->instno != insno_priority) continue;
        /* the following is based on code by Matt J. Ingalls */
        /* find an unused number and use it */
        while (++num <= csound->maxinsno && csound->instrtxtp[num]);
        /* we may need to expand the instrument array */
        if (num > csound->maxinsno) {
          int m = csound->maxinsno;
          csound->maxinsno += MAXINSNO; /* Expand */
          csound->instrtxtp = (INSTRTXT**)
            mrealloc(csound, csound->instrtxtp,
                             (1 + csound->maxinsno) * sizeof(INSTRTXT*));
          /* Array expected to be nulled so.... */
          while (++m <= csound->maxinsno) csound->instrtxtp[m] = NULL;
        }
        /* hack: "name" actually points to the corresponding INSTRNAME */
        inm2 = (INSTRNAME*) (inm->name);    /* entry in the table */
        inm2->instno = (long) num;
        csound->instrtxtp[num] = inm2->ip;
        if (csound->oparms->msglevel)
          csound->Message(csound, Str("instr %s uses instrument number %d\n"),
                                  inm2->name, num);
      }
    }
    /* clear temporary chains */
    inm = *inm_first;
    while (inm) {
      INSTRNAME *nxtinm = inm->prv;
      mfree(csound, inm);
      inm = nxtinm;
    }
    *inm_first = *inm_last = NULL;
}

/* free memory used by named instruments */
/* called by tranRESET() */

void named_instr_free(ENVIRON *csound)
{
    INSTRNAME   *inm;
    int     i;

    if (!csound->instrumentNames) return;       /* nothing to free */
    for (i = 0; i < 256; i++) {
      inm = ((INSTRNAME**) csound->instrumentNames)[i];
      while (inm) {
        INSTRNAME   *prvinm = inm->prv;
        mfree(csound, inm);
        inm = prvinm;
      }
    }
}

/* convert opcode string argument to instrument number */
/* return value is -1 if the instrument cannot be found */
/* (in such cases, csoundInitError() is also called) */

long strarg2insno(ENVIRON *csound, void *p, int is_string)
{
    long    insno;

    if (is_string) {
      if ((insno = named_instr_find(csound, (char*) p)) <= 0) {
        csound->InitError(csound, "instr %s not found", (char*) p);
        return -1;
      }
    }
    else {      /* numbered instrument */
      insno = (long) *((MYFLT*) p);
      if (insno < 1 || insno > csound->maxinsno || !csound->instrtxtp[insno]) {
        csound->InitError(csound, "Cannot Find Instrument %d", (int) insno);
        return -1;
      }
    }
    return insno;
}

/* same as strarg2insno, but runs at perf time, */
/* and does not support numbered instruments */
/* (used by opcodes like event or schedkwhen) */

long strarg2insno_p(ENVIRON *csound, char *s)
{
    long    insno;

    if (!(insno = named_instr_find(csound, s))) {
      csound->PerfError(csound, "instr %s not found", s);
      return -1;
    }
    return insno;
}

/* convert opcode string argument to instrument number */
/* (also allows user defined opcode names); if the integer */
/* argument is non-zero, only opcode names are searched */
/* return value is -1 if the instrument cannot be found */
/* (in such cases, csoundInitError() is also called) */

long strarg2opcno(ENVIRON *csound, void *p, int is_string, int force_opcode)
{
    long    insno = 0;

    if (!force_opcode) {        /* try instruments first, if enabled */
      if (is_string) {
        insno = named_instr_find(csound, (char*) p);
      }
      else {      /* numbered instrument */
        insno = (long) *((MYFLT*) p);
        if (insno < 1 || insno > csound->maxinsno ||
            !csound->instrtxtp[insno]) {
          csound->InitError(csound, "Cannot Find Instrument %d", (int) insno);
          return -1;
        }
      }
    }
    if (!insno && is_string) {              /* if no instrument was found, */
      OPCODINFO *inm = csound->opcodeInfo;  /* search for user opcode */
      while (inm && sCmp(inm->name, (char*) p)) inm = inm->prv;
      if (inm) insno = (long) inm->instno;
    }
    if (insno < 1) {
      csound->InitError(csound,
                        Str("cannot find the specified instrument or opcode"));
      insno = -1;
    }
    return insno;
}

/* create file name from opcode argument (string or MYFLT)      */
/*   ENVIRON *csound:                                           */
/*      pointer to Csound instance                              */
/*   char *s:                                                   */
/*      output buffer, should have enough space; if NULL, the   */
/*      required amount of memory is allocated and returned     */
/*   void *p:                                                   */
/*      opcode argument, is interpreted as char* or MYFLT*,     */
/*      depending on the 'is_string' parameter                  */
/*   const char *baseName:                                      */
/*      name prefix to be used if the 'p' argument is MYFLT,    */
/*      and it is neither SSTRCOD, nor a valid index to strset  */
/*      space.                                                  */
/*      For example, if "soundin." is passed as baseName, file  */
/*      names in the format "soundin.%d" will be generated.     */
/*      baseName may be an empty string, but should not be NULL */
/*   int is_string:                                             */
/*      if non-zero, 'p' is interpreted as a char* pointer and  */
/*      is used as the file name. Otherwise, it is expected to  */
/*      point to a MYFLT value, and the following are tried:    */
/*        1. if the value is SSTRCOD, the string argument of    */
/*           the current score event is used (string p-field)   */
/*        2. if the value, rounded to the nearest integer, is a */
/*           valid index to strset space, the strset string is  */
/*           used                                               */
/*        3. the file name is generated using baseName and the  */
/*           value rounded to the nearest integer, as described */
/*           above                                              */
/*      'is_string' is usually p->XSTRCODE for an opcode with   */
/*      only one string argument, otherwise it is               */
/*      p->XSTRCODE & (1 << (argno - 1))                        */
/*   return value:                                              */
/*      pointer to the output string; if 's' is not NULL, it is */
/*      always the same as 's', otherwise it is allocated with  */
/*      mmalloc() and the caller is responsible for freeing the */
/*      allocated memory with mfree() or csound->Free()         */

char *strarg2name(ENVIRON *csound, char *s, void *p, const char *baseName,
                                   int is_string)
{
    if (is_string) {
      /* opcode string argument */
      if (s == NULL)
        s = mmalloc(csound, strlen((char*) p) + 1);
      strcpy(s, (char*) p);
    }
    else if (*((MYFLT*) p) == SSTRCOD) {
      /* p-field string, unquote and copy */
      char  *s2 = csound->currevent->strarg;
      int   i = 0;
      if (s == NULL)
        s = mmalloc(csound, strlen(csound->currevent->strarg) + 1);
      if (*s2 == '"')
        s2++;
      while (*s2 != '"' && *s2 != '\0')
        s[i++] = *(s2++);
      s[i] = '\0';
    }
    else {
      int   i = (int) ((double) *((MYFLT*) p)
                       + (*((MYFLT*) p) >= FL(0.0) ? 0.5 : -0.5));
      if (i >= 0 && i <= (int) csound->strsmax &&
          csound->strsets != NULL && csound->strsets[i] != NULL) {
        if (s == NULL)
          s = mmalloc(csound, strlen(csound->strsets[i]) + 1);
        strcpy(s, csound->strsets[i]);
      }
      else {
        if (s == NULL)
          /* allocate +20 characters, assuming sizeof(int) <= 8 */
          s = mmalloc(csound, strlen(baseName) + 21);
        sprintf(s, "%s%d", baseName, i);
      }
    }
    return s;
}

/* ----------------------------------------------------------------------- */
/* the following functions are for efficient management of the opcode list */

/* create new opcode list from opcodlst[] */

void opcode_list_create(ENVIRON *csound)
{
    int     n = csound->oplstend - csound->opcodlst;

    if (csound->opcode_list) {
      csound->Die(csound,
                  Str("internal error: opcode list has already been created"));
      return;
    }
    csound->opcode_list = (void*) mcalloc(csound, sizeof(int) * 256);
    /* add all entries, except #0 which is unused */
    while (--n)
      opcode_list_add_entry(csound, n, 0);
}

/* add new entry to opcode list, with optional check for redefined opcodes */

void opcode_list_add_entry(ENVIRON *csound, int opnum, int check_redefine)
{
    /* calculate hash value for opcode name */
    unsigned char h = name_hash(csound->opcodlst[opnum].opname);
    /* link entry into chain */
    if (check_redefine) {
      int   *n = (int*) csound->opcode_list + h;
      /* check if an opcode with the same name is already defined */
      while (*n && sCmp(csound->opcodlst[*n].opname,
                        csound->opcodlst[opnum].opname)) {
        n = &(csound->opcodlst[*n].prvnum);
      }
      if (!*n) goto newopc;
      /* redefine */
      csound->opcodlst[opnum].prvnum = csound->opcodlst[*n].prvnum;
      *n = opnum;
    }
    else {
newopc:
      /* new opcode */
      csound->opcodlst[opnum].prvnum = ((int*) csound->opcode_list)[h];
      ((int*) csound->opcode_list)[h] = opnum;
    }
    if (csound->oparms->odebug && (csound->oparms->msglevel & 0x100000))
      csound->Message(csound,
                      "Added opcode opname = %s, hash = %d, opnum = %d\n",
                      csound->opcodlst[opnum].opname, (int) h, opnum);
}

/* free memory used by opcode list */

extern int useropcdset(void*, void*);

void opcode_list_free(ENVIRON *csound)
{
    OENTRY  *ep = csound->oplstend;

    if (!csound->opcode_list) return;
    /* free memory used by temporary list, */
    mfree(csound, csound->opcode_list);
    csound->opcode_list = NULL;
    /* user defined opcode argument types, */
    while ((--ep)->iopadr == (SUBR) useropcdset) {
      mfree(csound, ep->intypes);
      mfree(csound, ep->outypes);
    }
    /* and opcodlst */
    mfree(csound, csound->opcodlst);
    csound->opcodlst = csound->oplstend = NULL;
}

/* find opcode with the specified name in opcode list */
/* returns index to opcodlst[], or zero if the opcode cannot be found */

int find_opcode(ENVIRON *csound, char *opname)
{
    int           n;
    unsigned char h = name_hash(opname);    /* calculate hash value */

    /* now find entry in opcode chain */
    n = ((int*) csound->opcode_list)[h];
    while (n && sCmp(csound->opcodlst[n].opname, opname))
      n = csound->opcodlst[n].prvnum;

    return n;
}

/* ----------------------------------------------------------------------- */
/* These functions replace the functionality of strsav() in rdorch.c.      */

#define STRSPACE    (8000)      /* number of bytes in a buffer      */

typedef struct strsav_t {
        struct strsav_t *nxt;   /* pointer to next structure    */
        char    s[1];           /* the string stored            */
} STRSAV;

typedef struct strsav_space_t {
        char    sp[STRSPACE];           /* string space                */
        int     splim;                  /* number of bytes allocated   */
        struct strsav_space_t   *prv;   /* ptr to previous buffer      */
} STRSAV_SPACE;

#define STRSAV_STR_     ((STRSAV**) (csound->strsav_str))
#define STRSAV_SPACE_   ((STRSAV_SPACE*) (csound->strsav_space))

/* allocate space for strsav (called once from rdorchfile()) */

void strsav_create(ENVIRON *csound)
{
    if (csound->strsav_space != NULL) return;   /* already allocated */
    csound->strsav_space = mcalloc(csound, sizeof(STRSAV_SPACE));
    csound->strsav_str = mcalloc(csound, sizeof(STRSAV*) * 256);
}

/* Locate string s in database, and return address of stored string (not */
/* necessarily the same as s). If the string is not defined yet, it is   */
/* copied to the database (in such cases, it is allowed to free s after  */
/* the call).                                                            */

char *strsav_string(ENVIRON *csound, char *s)
{
    STRSAV        *ssp;
    int           n;
    unsigned char h = name_hash(s);     /* calculate hash value */

    /* now find entry in database */
    ssp = STRSAV_STR_[h];
    while (ssp) {
      if (!sCmp(ssp->s, s)) return (ssp->s);    /* already defined */
      ssp = ssp->nxt;
    }
    /* not found, so need to allocate a new entry */
    n = (int) sizeof(STRSAV) + (int) strlen(s); /* number of bytes */
    n = ((n + 15) >> 4) << 4;   /* round up for alignment (16 bytes) */
    if ((STRSAV_SPACE_->splim + n) > STRSPACE) {
      STRSAV_SPACE  *sp;
      /* not enough space, allocate new buffer */
      if (n > STRSPACE) {
        /* this should not happen */
        csound->Message(csound,
                        "internal error: strsav: string length > STRSPACE\n");
        return NULL;
      }
      sp = (STRSAV_SPACE*) mcalloc(csound, sizeof(STRSAV_SPACE));
      sp->prv = STRSAV_SPACE_;
      csound->strsav_space = sp;
    }
    /* use space from buffer */
    ssp = (STRSAV*) ((char*) STRSAV_SPACE_->sp + STRSAV_SPACE_->splim);
    STRSAV_SPACE_->splim += n;
    strcpy(ssp->s, s);          /* save string */
    /* link into chain */
    ssp->nxt = STRSAV_STR_[h];
    STRSAV_STR_[h] = ssp;
    return (ssp->s);
}

/* Free all memory used by strsav space. Called from orchRESET(). */

void strsav_destroy(ENVIRON *csound)
{
    STRSAV_SPACE  *sp = STRSAV_SPACE_;

    if (!sp) return;                    /* nothing to free */
    csound->strsav_space = NULL;
    do {
      STRSAV_SPACE  *prvsp = sp->prv;
      mfree(csound, sp);
      sp = prvsp;
    } while (sp);

    mfree(csound, STRSAV_STR_);
    csound->strsav_str = NULL;
}

/* -------- IV - Jan 29 2005 -------- */

/* #define CSGLOBALS_USE_TREE  1 */

#ifdef CSGLOBALS_USE_TREE

static void fix_pointers_in_db(void **p,
                               unsigned char *oldp, unsigned char *newp)
{
    void **pp;
    int  i, j;
    /* hack to fix pointers in globals database after realloc() */
    for (i = 0; i < 16; i++) {
      if (p[i] == (void*) NULL)
        continue;       /* nothing here */
      pp = (void**) ((unsigned char*) (p[i]) + (newp - oldp));
      p[i] = (void*) pp;
      for (j = 0; j < 16; j += 2) {
        if (pp[j] != (void*) NULL) {
          /* recursively search entire database */
          pp[j] = (void*) ((unsigned char*) (pp[j]) + (newp - oldp));
          fix_pointers_in_db((void**) pp[j], oldp, newp);
        }
      }
    }
}

static void **extendNamedGlobals(ENVIRON *p, int n, int storeIndex)
{
    void  **ptr = NULL;
    int   i, oldlimit;

    oldlimit = p->namedGlobalsCurrLimit;
    p->namedGlobalsCurrLimit += n;
    if (p->namedGlobalsCurrLimit > p->namedGlobalsMaxLimit) {
      p->namedGlobalsMaxLimit = p->namedGlobalsCurrLimit;
      p->namedGlobalsMaxLimit += (p->namedGlobalsMaxLimit >> 3);
      p->namedGlobalsMaxLimit = (p->namedGlobalsMaxLimit + 15) & (~15);
      ptr = p->namedGlobals;
      p->namedGlobals = (void**) realloc((void*) p->namedGlobals,
                                         sizeof(void*)
                                         * (size_t) p->namedGlobalsMaxLimit);
      if (p->namedGlobals == NULL) {
        p->namedGlobalsCurrLimit = p->namedGlobalsMaxLimit = 0;
        return NULL;
      }
      if (p->namedGlobals != ptr && ptr != NULL) {
        /* realloc() moved the data, need to fix pointers */
        fix_pointers_in_db(p->namedGlobals, (unsigned char*) ptr,
                                            (unsigned char*) p->namedGlobals);
      }
      /* clear new allocated space to zero */
      for (i = oldlimit; i < p->namedGlobalsMaxLimit; i++)
        p->namedGlobals[i] = (void*) NULL;
    }
    ptr = (void**) p->namedGlobals + (int) oldlimit;
    if (storeIndex >= 0) {
      /* if requested, store pointer to new area at specified array index */
      p->namedGlobals[storeIndex] = (void*) ptr;
    }
    return ptr;
}

/**
 * Allocate nbytes bytes of memory that can be accessed later by calling
 * csoundQueryGlobalVariable() with the specified name; the space is
 * cleared to zero.
 * Returns CSOUND_SUCCESS on success, CSOUND_ERROR in case of invalid
 * parameters (zero nbytes, invalid or already used name), or
 * CSOUND_MEMORY if there is not enough memory.
 */
PUBLIC int csoundCreateGlobalVariable(void *csound, const char *name,
                                      size_t nbytes)
{
    ENVIRON *csnd = (ENVIRON*) csound;
    void    **p = NULL;
    int     i, j, k, len;
    /* create new empty database if it does not exist yet */
    if (csnd->namedGlobals == NULL) {
      if (extendNamedGlobals(csnd, 16, -1) == (void**) NULL)
        return CSOUND_MEMORY;
    }
    /* check for a valid name */
    if (name == NULL)
      return CSOUND_ERROR;
    if (name[0] == '\0')
      return CSOUND_ERROR;
    len = (int) strlen(name);
    for (i = 0; i < len; i++)
      if ((unsigned char) name[i] >= (unsigned char) 0x80)
        return CSOUND_ERROR;
    /* cannot allocate zero bytes */
    if ((int) nbytes < 1)
      return CSOUND_ERROR;
    /* store in tree */
    i = -1;
    p = csnd->namedGlobals;
    while (++i < (len - 1)) {
      j = ((int) name[i] & 0x78) >> 3;  /* bits 3-6 */
      k = ((int) name[i] & 0x07) << 1;  /* bits 0-2 */
      if (p[j] == (void*) NULL) {
        p = extendNamedGlobals(csnd, 16, (int) ((void**) &(p[j])
                                                - csnd->namedGlobals));
        if (p == NULL)
          return CSOUND_MEMORY;
      }
      else
        p = (void**) (p[j]);
      if (p[k] == (void*) NULL) {
        p = extendNamedGlobals(csnd, 16, (int) ((void**) &(p[k])
                                                - csnd->namedGlobals));
        if (p == NULL)
          return CSOUND_MEMORY;
      }
      else
        p = (void**) (p[k]);
    }
    j = ((int) name[i] & 0x78) >> 3;    /* bits 3-6 */
    k = ((int) name[i] & 0x07) << 1;    /* bits 0-2 */
    if (p[j] == (void*) NULL) {
      p = extendNamedGlobals(csnd, 16, (int) ((void**) &(p[j])
                                              - csnd->namedGlobals));
      if (p == NULL)
        return CSOUND_MEMORY;
    }
    else
      p = (void**) (p[j]);
    if (p[k + 1] != (void*) NULL)
      return CSOUND_ERROR;              /* name is already defined */
    /* allocate memory and store pointer */
    p[k + 1] = (void*) malloc(nbytes);
    if (p[k + 1] == (void*) NULL)
      return CSOUND_MEMORY;
    memset(p[k + 1], 0, nbytes);        /* clear space to zero */
    /* successfully finished */
    return CSOUND_SUCCESS;
}

/**
 * Get pointer to space allocated with the name "name".
 * Returns NULL if the specified name is not defined.
 */
PUBLIC void *csoundQueryGlobalVariable(void *csound, const char *name)
{
    ENVIRON *csnd = (ENVIRON*) csound;
    void    **p = NULL;
    int     i, j, k, len;
    /* check if there is an actual database to search */
    if (csnd->namedGlobals == NULL)
      return NULL;
    /* check for a valid name */
    if (name == NULL)
      return NULL;
    if (name[0] == '\0')
      return NULL;
    len = (int) strlen(name);
    /* search tree */
    i = -1;
    p = csnd->namedGlobals;
    while (++i < (len - 1)) {
      if ((unsigned char) name[i] >= (unsigned char) 0x80)
        return NULL;            /* invalid name: must be 7-bit ASCII */
      j = ((int) name[i] & 0x78) >> 3;  /* bits 3-6 */
      k = ((int) name[i] & 0x07) << 1;  /* bits 0-2 */
      if (p[j] == (void*) NULL)
        return NULL;            /* not found */
      else
        p = (void**) (p[j]);
      if (p[k] == (void*) NULL)
        return NULL;            /* not found */
      else
        p = (void**) (p[k]);
    }
    if ((unsigned char) name[i] >= (unsigned char) 0x80)
      return NULL;              /* invalid name: must be 7-bit ASCII */
    j = ((int) name[i] & 0x78) >> 3;    /* bits 3-6 */
    k = ((int) name[i] & 0x07) << 1;    /* bits 0-2 */
    if (p[j] == (void*) NULL)
      return NULL;              /* not found */
    else
      p = (void**) (p[j]);
    /* return with pointer (will be NULL for undefined name) */
    return ((void*) p[k + 1]);
}

/**
 * This function is the same as csoundQueryGlobalVariable(), except the
 * variable is assumed to exist and no error checking is done.
 * Faster, but may crash or return an invalid pointer if 'name' is
 * not defined.
 */
PUBLIC void *csoundQueryGlobalVariableNoCheck(void *csound, const char *name)
{
    ENVIRON *csnd = (ENVIRON*) csound;
    void    **p = NULL;
    int     i, j, k, len;

    len = (int) strlen(name);
    /* search tree */
    i = -1;
    p = csnd->namedGlobals;
    while (++i < (len - 1)) {
      j = ((int) name[i] & 0x78) >> 3;  /* bits 3-6 */
      k = ((int) name[i] & 0x07) << 1;  /* bits 0-2 */
      p = (void**) (p[j]);
      p = (void**) (p[k]);
    }
    j = ((int) name[i] & 0x78) >> 3;    /* bits 3-6 */
    k = ((int) name[i] & 0x07) << 1;    /* bits 0-2 */
    p = (void**) (p[j]);
    /* return with pointer */
    return ((void*) p[k + 1]);
}

/**
 * Free memory allocated for "name" and remove "name" from the database.
 * Return value is CSOUND_SUCCESS on success, or CSOUND_ERROR if the name is
 * not defined.
 */
PUBLIC int csoundDestroyGlobalVariable(void *csound, const char *name)
{
    ENVIRON *csnd = (ENVIRON*) csound;
    void    **p = NULL;
    int     i, j, k, len;
    /* check for a valid name */
    if (csoundQueryGlobalVariable(csound, name) == (void*) NULL)
      return CSOUND_ERROR;
    len = (int) strlen(name);
    /* search tree (simple version, as the name will surely be found) */
    i = -1;
    p = csnd->namedGlobals;
    while (++i < (len - 1)) {
      j = ((int) name[i] & 0x78) >> 3;  /* bits 3-6 */
      k = ((int) name[i] & 0x07) << 1;  /* bits 0-2 */
      p = (void**) (p[j]);
      p = (void**) (p[k]);
    }
    j = ((int) name[i] & 0x78) >> 3;    /* bits 3-6 */
    k = ((int) name[i] & 0x07) << 1;    /* bits 0-2 */
    p = (void**) (p[j]);
    /* free memory and clear pointer */
    free((void*) p[k + 1]);
    p[k + 1] = (void*) NULL;
    /* done */
    return CSOUND_SUCCESS;
}

/* recursively free all allocated globals */

static void free_global_variable(void **p)
{
    void **pp;
    int  i, j;
    for (i = 0; i < 16; i++) {
      if (p[i] == (void*) NULL)
        continue;       /* nothing here */
      pp = (void**) p[i];
      for (j = 0; j < 16; j += 2) {
        if (pp[j + 1] != (void*) NULL) {
          /* found allocated memory, free it now */
          free((void*) pp[j + 1]);
          pp[j + 1] = NULL;
        }
        /* recursively search entire database */
        if (pp[j] != (void*) NULL)
          free_global_variable((void**) pp[j]);
      }
    }
}

/**
 * Free entire global variable database. This function is for internal use
 * only (e.g. by RESET routines).
 */
void csoundDeleteAllGlobalVariables(void *csound)
{
    ((ENVIRON*) csound)->namedGlobalsCurrLimit = 0;
    ((ENVIRON*) csound)->namedGlobalsMaxLimit = 0;
    if (((ENVIRON*) csound)->namedGlobals == NULL)
      return;
    free_global_variable(((ENVIRON*) csound)->namedGlobals);
    free(((ENVIRON*) csound)->namedGlobals);
    ((ENVIRON*) csound)->namedGlobals = (void**) NULL;
}

#else   /* CSGLOBALS_USE_TREE */

typedef struct GlobalVariableEntry_s {
    struct GlobalVariableEntry_s *nxt;
    unsigned char *name;
    void *p;
    void *dummy;
} GlobalVariableEntry_t;

/**
 * Allocate nbytes bytes of memory that can be accessed later by calling
 * csoundQueryGlobalVariable() with the specified name; the space is
 * cleared to zero.
 * Returns CSOUND_SUCCESS on success, CSOUND_ERROR in case of invalid
 * parameters (zero nbytes, invalid or already used name), or
 * CSOUND_MEMORY if there is not enough memory.
 */
PUBLIC int csoundCreateGlobalVariable(void *csound, const char *name,
                                      size_t nbytes)
{
    ENVIRON               *csnd = (ENVIRON*) csound;
    GlobalVariableEntry_t *p, **pp;
    int                   i, structBytes, nameBytes, allocBytes;
    unsigned char         h;
    /* create new empty database if it does not exist yet */
    if (csnd->namedGlobals == NULL) {
      csnd->namedGlobals = (void**) malloc(sizeof(void*) * 256);
      if (csnd->namedGlobals == NULL)
        return CSOUND_MEMORY;
      for (i = 0; i < 256; i++)
        csnd->namedGlobals[i] = (void*) NULL;
    }
    /* check for valid parameters */
    if (name == NULL)
      return CSOUND_ERROR;
    if (name[0] == '\0')
      return CSOUND_ERROR;
    if (nbytes < (size_t) 1 || nbytes >= (size_t) 0x7F000000L)
      return CSOUND_ERROR;
    /* calculate hash value */
    h = name_hash(name);
    /* number of bytes to allocate */
    structBytes = ((int) sizeof(GlobalVariableEntry_t) + 15) & (~15);
    nameBytes = (((int) strlen(name) + 1) + 15) & (~15);
    allocBytes = ((int) nbytes + 15) & (~15);
    /* allocate memory */
    i = structBytes + nameBytes + allocBytes;
    p = (GlobalVariableEntry_t*) malloc((size_t) i);
    if (p == NULL)
      return CSOUND_MEMORY;
    /* initialise structure */
    memset((void*) p, 0, (size_t) i);
    p->nxt = (GlobalVariableEntry_t*) NULL;
    p->name = (unsigned char*) p + (int) structBytes;
    p->p = (void*) ((unsigned char*) p + (int) (structBytes + nameBytes));
    strcpy((char*) (p->name), name);
    /* link into database */
    if (csnd->namedGlobals[(int) h] == (void*) NULL) {
      /* hash value is not used yet */
      csnd->namedGlobals[(int) h] = (void*) p;
      return CSOUND_SUCCESS;
    }
    /* need to search */
    pp = (GlobalVariableEntry_t**) &(csnd->namedGlobals[(int) h]);
    do {
      /* check for a conflicting name */
      if (sCmp(name, (char*) ((*pp)->name)) == 0) {
        free((void*) p);
        return CSOUND_ERROR;
      }
      if ((*pp)->nxt == NULL)
        break;
      pp = &((*pp)->nxt);
    } while (1);
    (*pp)->nxt = (GlobalVariableEntry_t*) p;
    /* successfully finished */
    return CSOUND_SUCCESS;
}

/**
 * Get pointer to space allocated with the name "name".
 * Returns NULL if the specified name is not defined.
 */
PUBLIC void *csoundQueryGlobalVariable(void *csound, const char *name)
{
    ENVIRON               *csnd = (ENVIRON*) csound;
    GlobalVariableEntry_t *p;
    unsigned char         h;
    /* check if there is an actual database to search */
    if (csnd->namedGlobals == NULL)
      return NULL;
    /* check for a valid name */
    if (name == NULL)
      return NULL;
    if (name[0] == '\0')
      return NULL;
    /* calculate hash value */
    h = name_hash(name);
    /* search tree */
    p = (GlobalVariableEntry_t*) (csnd->namedGlobals[(int) h]);
    if (p == NULL)
      return NULL;
    while (sCmp(name, (char*) (p->name)) != 0) {
      p = (GlobalVariableEntry_t*) p->nxt;
      if (p == NULL)
        return NULL;
    }
    return (void*) (p->p);
}

/**
 * This function is the same as csoundQueryGlobalVariable(), except the
 * variable is assumed to exist and no error checking is done.
 * Faster, but may crash or return an invalid pointer if 'name' is
 * not defined.
 */
PUBLIC void *csoundQueryGlobalVariableNoCheck(void *csound, const char *name)
{
    ENVIRON               *csnd = (ENVIRON*) csound;
    GlobalVariableEntry_t *p;
    unsigned char         h;

    /* calculate hash value */
    h = name_hash(name);
    /* search tree */
    p = (GlobalVariableEntry_t*) (csnd->namedGlobals[(int) h]);
    while (p->nxt != NULL && sCmp(name, (char*) (p->name)) != 0)
      p = (GlobalVariableEntry_t*) p->nxt;
    return (void*) (p->p);
}

/**
 * Free memory allocated for "name" and remove "name" from the database.
 * Return value is CSOUND_SUCCESS on success, or CSOUND_ERROR if the name is
 * not defined.
 */
PUBLIC int csoundDestroyGlobalVariable(void *csound, const char *name)
{
    ENVIRON               *csnd = (ENVIRON*) csound;
    GlobalVariableEntry_t *p, *prvp;
    unsigned char         h;

    /* check for a valid name */
    if (csoundQueryGlobalVariable(csound, name) == (void*) NULL)
      return CSOUND_ERROR;
    /* calculate hash value */
    h = name_hash(name);
    /* search database (simple version, as the name will surely be found) */
    prvp = NULL;
    p = (GlobalVariableEntry_t*) (csnd->namedGlobals[(int) h]);
    while (sCmp(name, (char*) (p->name)) != 0) {
      prvp = p;
      p = (GlobalVariableEntry_t*) p->nxt;
    }
    if (prvp != NULL)
      prvp->nxt = (struct GlobalVariableEntry_s *) (p->nxt);
    else
      csnd->namedGlobals[(int) h] = (void*) (p->nxt);
    free((void*) p);
    /* done */
    return CSOUND_SUCCESS;
}

/**
 * Free entire global variable database. This function is for internal use
 * only (e.g. by RESET routines).
 */
void csoundDeleteAllGlobalVariables(void *csound)
{
    GlobalVariableEntry_t *p, *prvp;
    int                   i;

    if (((ENVIRON*) csound)->namedGlobals == NULL)
      return;
    for (i = 0; i < 256; i++) {
      prvp = (GlobalVariableEntry_t*) NULL;
      p = (GlobalVariableEntry_t*) (((ENVIRON*) csound)->namedGlobals[i]);
      while (p != NULL) {
        prvp = p;
        p = (GlobalVariableEntry_t*) (p->nxt);
        if (prvp != NULL)
          free((void*) prvp);
      }
    }
    free((void*) (((ENVIRON*) csound)->namedGlobals));
    ((ENVIRON*) csound)->namedGlobals = (void**) NULL;
}

#endif  /* CSGLOBALS_USE_TREE */

#ifdef __cplusplus
};
#endif

