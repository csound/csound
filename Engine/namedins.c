/*  
    namedins.c:

    Copyright (C) 2002 Istvan Varga

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


/* namedins.c -- written by Istvan Varga, Oct 2002 */

#include "namedins.h"
#include <ctype.h>

typedef struct namedInstr {
        long    instno;
        char    *name;
        INSTRTXT    *ip;
        struct namedInstr   *prv;
} INSTRNAME;

/* do not touch this ! */

static const unsigned char tabl[256] = {
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

#define name_hash(x,y) (tabl[(unsigned char) x ^ (unsigned char) y])

/* check if the string s is a valid instrument or opcode name */
/* return value is zero if the string is not a valid name */

int check_instr_name (char *s)
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

long named_instr_find (char *s)
{
    INSTRNAME   *inm;
    unsigned char h = 0, *c = (unsigned char*) s - 1;

    if (!instrumentNames) return 0L;    /* no named instruments defined */
    /* calculate hash value */
    while (*++c) h = name_hash(h, *c);
    /* now find instrument */
    if (!(inm = ((INSTRNAME**) instrumentNames)[h])) return 0L;
    if (!inm->prv) return (long) inm->instno;
    /* multiple entries for hash value, need to search */
    while (inm && strcmp(inm->name, s)) inm = inm->prv;
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

int named_instr_alloc (char *s, INSTRTXT *ip, long insno)
{
    INSTRNAME   **inm_base = (INSTRNAME**) instrumentNames, *inm, *inm2;
    unsigned char h = 0, *c = (unsigned char*) s - 1;

    if (!inm_base)
      /* no named instruments defined yet */
      inm_base = instrumentNames = (void*) mcalloc (sizeof(INSTRNAME*) * 258);
    /* calculate hash value */
    while (*++c) h = name_hash(h, *c);
    /* now check if instrument is already defined */
    if ((inm = inm_base[h])) {
      while (inm && strcmp(inm->name, s)) inm = inm->prv;
      if (inm) return 0;        /* error: instr exists */
    }
    /* allocate entry, */
    inm = (INSTRNAME*) mcalloc (sizeof(INSTRNAME));
    inm2 = (INSTRNAME*) mcalloc (sizeof(INSTRNAME));
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
    if (O.odebug)
      printf("named instr name = \"%s\", hash = %d, txtp = %p\n", s, (int) h, ip);
    return 1;
}

/* assign instrument numbers to all named instruments */
/* called by otran */

void named_instr_assign_numbers (void)
{
    INSTRNAME   *inm, *inm2, **inm_first, **inm_last;
    int     num = 0, insno_priority = 0;

    if (!instrumentNames) return;       /* no named instruments */
    inm_first = (INSTRNAME**) instrumentNames + 256;
    inm_last = inm_first + 1;
    while (--insno_priority > -3) {
      if (insno_priority == -2) {
        num = maxinsno;                 /* find last used instr number */
        while (!instrtxtp[num] && --num);
      }
      for (inm = *inm_first; inm; inm = inm->prv) {
        if ((int) inm->instno != insno_priority) continue;
        /* the following is based on code by Matt J. Ingalls */
        /* find an unused number and use it */
        while (++num <= maxinsno && instrtxtp[num]);
        /* we may need to expand the instrument array */
        if (num > maxinsno) {
          int m = maxinsno;
          maxinsno += MAXINSNO; /* Expand */
          instrtxtp = (INSTRTXT**)
            mrealloc(instrtxtp, (long) ((1 + maxinsno) * sizeof(INSTRTXT*)));
          /* Array expected to be nulled so.... */
          while (++m <= maxinsno) instrtxtp[m] = NULL;
        }
        /* hack: "name" actually points to the corresponding INSTRNAME */
        inm2 = (INSTRNAME*) (inm->name);    /* entry in the table */
        inm2->instno = (long) num;
        instrtxtp[num] = inm2->ip;
        if (O.msglevel)
          printf(Str(X_19,"instr %s uses instrument number %d\n"),
                 inm2->name, num);
      }
    }
    /* clear temporary chains */
    inm = *inm_first;
    while (inm) {
      INSTRNAME *nxtinm = inm->prv;
      mfree(inm);
      inm = nxtinm;
    }
    *inm_first = *inm_last = NULL;
}

/* free memory used by named instruments */
/* called by tranRESET() */

void named_instr_free (void)
{
    INSTRNAME   *inm;
    int     i;

    if (!instrumentNames) return;       /* nothing to free */
    for (i = 0; i < 256; i++) {
      inm = ((INSTRNAME**) instrumentNames)[i];
      while (inm) {
        INSTRNAME   *prvinm = inm->prv;
        mfree(inm);
        inm = prvinm;
      }
    }
}

/* convert opcode string argument to instrument number */
/* return value is -1 if the instrument cannot be found */
/* (in such cases, initerror() is also called) */

long strarg2insno (MYFLT *p, char *s)
{
    long    insno;

    if (*p == SSTRCOD) {
      if (!(insno = named_instr_find(s))) {
        sprintf(errmsg, "instr %s not found", s);
        initerror(errmsg); return -1;
      }
    }
    else {      /* numbered instrument */
      insno = (long) *p;
      if (insno < 1 || insno > maxinsno || !instrtxtp[insno]) {
        sprintf(errmsg, "Cannot Find Instrument %d", (int) insno);
        initerror(errmsg); return -1;
      }
    }
    return insno;
}

/* same as strarg2insno, but runs at perf time, */
/* and does not support numbered instruments */
/* (used by opcodes like event or schedkwhen) */

long strarg2insno_p (char *s)
{
    long    insno;

    if (!(insno = named_instr_find(s))) {
      sprintf(errmsg, "instr %s not found", s);
      perferror(errmsg); return -1;
    }
    return insno;
}

/* convert opcode string argument to instrument number */
/* (also allows user defined opcode names); if the integer */
/* argument is non-zero, only opcode names are searched */
/* return value is -1 if the instrument cannot be found */
/* (in such cases, initerror() is also called) */

long strarg2opcno (MYFLT *p, char *s, int force_opcode)
{
    long    insno = 0;

    if (!force_opcode) {        /* try instruments first, if enabled */
      if (*p == SSTRCOD) {
        insno = named_instr_find(s);
      }
      else {      /* numbered instrument */
        insno = (long) *p;
        if (insno < 1 || insno > maxinsno || !instrtxtp[insno]) {
          sprintf(errmsg, "Cannot Find Instrument %d", (int) insno);
          initerror(errmsg); return -1;
        }
      }
    }
    if (!insno && *p == SSTRCOD) {      /* if no instrument was found, */
      OPCODINFO *inm = opcodeInfo;      /* search for user opcode */
      while (inm && strcmp(inm->name, s)) inm = inm->prv;
      if (inm) insno = (long) inm->instno;
    }
    if (insno < 1) {
      initerror(Str(X_21,"cannot find the specified instrument or opcode"));
      insno = -1;
    }
    return insno;
}

/* ----------------------------------------------------------------------- */
/* the following functions are for efficient management of the opcode list */

/* create new opcode list from opcodlst[] */

void opcode_list_create (void)
{
    int     n = oplstend - opcodlst;

    if (opcode_list) {
      die(Str(X_23,"internal error: opcode list has already been created"));
      return;
    }
    opcode_list = (void*) mcalloc(sizeof(int) * 256);
    /* add all entries, except #0 which is unused */
    while (--n)
      opcode_list_add_entry(n, 0);
}

/* add new entry to opcode list, with optional check for redefined opcodes */

void opcode_list_add_entry (int opnum, int check_redefine)
{
    unsigned char   h = 0, *c = (unsigned char*) opcodlst[opnum].opname - 1;

    /* calculate hash value for opcode name */
    while (*++c) h = name_hash(h, *c);
    /* link entry into chain */
    if (check_redefine) {
      int   *n = (int*) opcode_list + h;
      /* check if an opcode with the same name is already defined */
      while (*n && strcmp(opcodlst[*n].opname, opcodlst[opnum].opname)) {
        n = &(opcodlst[*n].prvnum);
      }
      if (!*n) goto newopc;
      opcodlst[opnum].prvnum = opcodlst[*n].prvnum;         /* redefine */
      *n = opnum;
    }
    else {
newopc:
      opcodlst[opnum].prvnum = ((int*) opcode_list)[h];     /* new opcode */
      ((int*) opcode_list)[h] = opnum;
    }
    if (O.odebug && (O.msglevel & 0x100000))
      printf("Added opcode opname = %s, hash = %d, opnum = %d\n",
             opcodlst[opnum].opname, (int) h, opnum);
}

/* free memory used by opcode list */

extern void useropcdset(void*);

void opcode_list_free (void)
{
    OENTRY  *ep = oplstend;

    if (!opcode_list) return;
    /* free memory used by temporary list, */
    mfree(opcode_list);
    opcode_list = NULL;
    /* user defined opcode argument types, */
    while ((--ep)->iopadr == (SUBR) useropcdset) {
      mfree(ep->intypes);
      mfree(ep->outypes);
    }
    /* and opcodlst */
    mfree(opcodlst);
    opcodlst = oplstend = NULL;
}

/* find opcode with the specified name in opcode list */
/* returns index to opcodlst[], or zero if the opcode cannot be found */

int find_opcode(char *opname)
{
    unsigned char   h = 0, *c = (unsigned char*) opname - 1;
    int     n;

    /* calculate hash value for opcode name */
    while (*++c) h = name_hash(h, *c);
    /* now find entry in opcode chain */
    n = ((int*) opcode_list)[h];
    while (n && strcmp(opcodlst[n].opname, opname)) n = opcodlst[n].prvnum;

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

static STRSAV **strsav_str = NULL;
static STRSAV_SPACE *strsav_space = NULL;

/* allocate space for strsav (called once from rdorchfile()) */

void strsav_create(void)
{
    if (strsav_space != NULL) return;           /* already allocated */
    strsav_space = (STRSAV_SPACE*) mcalloc(sizeof(STRSAV_SPACE));
    strsav_str = (STRSAV**) mcalloc(sizeof(STRSAV*) * 256);
}

/* Locate string s in database, and return address of stored string (not */
/* necessarily the same as s). If the string is not defined yet, it is   */
/* copied to the database (in such cases, it is allowed to free s after  */
/* the call).                                                            */

char *strsav_string(char *s)
{
    unsigned char   h = 0, *c = (unsigned char*) s - 1;
    STRSAV  *ssp;
    int     n;

    /* calculate hash value for the specified string */
    while (*++c) h = name_hash(h, *c);
    /* now find entry in database */
    ssp = strsav_str[h];
    while (ssp) {
      if (!strcmp(ssp->s, s)) return (ssp->s);  /* already defined */
      ssp = ssp->nxt;
    }
    /* not found, so need to allocate a new entry */
    n = (int) sizeof(STRSAV) + (int) strlen(s); /* number of bytes */
    n = ((n + 15) >> 4) << 4;   /* round up for alignment (16 bytes) */
    if ((strsav_space->splim + n) > STRSPACE) {
      STRSAV_SPACE  *sp;
      /* not enough space, allocate new buffer */
      if (n > STRSPACE) {
        /* this should not happen */
        err_printf("internal error: strsav: string length > STRSPACE\n");
        return NULL;
      }
      sp = (STRSAV_SPACE*) mcalloc(sizeof(STRSAV_SPACE));
      sp->prv = strsav_space;
      strsav_space = sp;
    }
    /* use space from buffer */
    ssp = (STRSAV*) ((char*) strsav_space->sp + strsav_space->splim);
    strsav_space->splim += n;
    strcpy(ssp->s, s);          /* save string */
    /* link into chain */
    ssp->nxt = strsav_str[h];
    strsav_str[h] = ssp;
    return (ssp->s);
}

/* Free all memory used by strsav space. Called from orchRESET(). */

void strsav_destroy(void)
{
    STRSAV_SPACE  *sp = strsav_space;

    if (!sp) return;                    /* nothing to free */
    strsav_space = NULL;
    do {
      STRSAV_SPACE  *prvsp = sp->prv;
      mfree(sp);
      sp = prvsp;
    } while (sp);

    mfree(strsav_str);
    strsav_str = NULL;
}

