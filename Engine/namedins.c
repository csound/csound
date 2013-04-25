/*
    namedins.c:

    Copyright (C) 2002, 2005, 2006 Istvan Varga

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

#include "csoundCore.h"
#include "namedins.h"
#include <ctype.h>

static const char* INSTR_NAME_FIRST = "::^inm_first^::";

typedef struct namedInstr {
    int32        instno;
    char        *name;
    INSTRTXT    *ip;
    struct namedInstr   *next;
} INSTRNAME;


typedef struct CsoundOpcodePluginFile_s {
    /* file name (base name only) */
    char                        *fname;
    /* pointer to next link in chain */
    struct CsoundOpcodePluginFile_s *nxt;
    /* file name (full path) */
    char                        *fullName;
    /* is this file already loaded ? (0: no, 1: yes, -1: does not exist) */
    int                         isLoaded;
} CsoundOpcodePluginFile_t;

/* typedef struct CsoundPluginOpcode_s { */
/*     /\* opcode name *\/ */
/*     char                        *opname; */
/*     /\* pointer to next link in chain *\/ */
/*     struct CsoundPluginOpcode_s *nxt; */
/*     /\* associated plugin library *\/ */
/*     CsoundOpcodePluginFile_t    *fp; */
/* } CsoundPluginOpcode_t; */

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

unsigned int csound_str_hash_32(const char *s)
{
    uint64_t      tmp;
    unsigned int  h = 0U;

    while (1) {
      unsigned int  c;
      c = (unsigned int) s[0] & 0xFFU;
      if (!c)
        return h;
      h ^= c;
      c = (unsigned int) s[1] & 0xFFU;
      if (!c)
        break;
      h ^= (c << 8);
      c = (unsigned int) s[2] & 0xFFU;
      if (!c)
        break;
      h ^= (c << 16);
      c = (unsigned int) s[3] & 0xFFU;
      if (!c)
        break;
      h ^= (c << 24);
      tmp = (uint32_t) h * (uint64_t) 0xC2B0C3CCU;
      h = ((unsigned int) tmp ^ (unsigned int) (tmp >> 32)) & 0xFFFFFFFFU;
      c = (unsigned int) s[4] & 0xFFU;
      if (!c)
        return h;
      h ^= c;
      c = (unsigned int) s[5] & 0xFFU;
      if (!c)
        break;
      h ^= (c << 8);
      c = (unsigned int) s[6] & 0xFFU;
      if (!c)
        break;
      h ^= (c << 16);
      c = (unsigned int) s[7] & 0xFFU;
      if (!c)
        break;
      h ^= (c << 24);
      s += 8;
      tmp = (uint32_t) h * (uint64_t) 0xC2B0C3CCU;
      h = ((unsigned int) tmp ^ (unsigned int) (tmp >> 32)) & 0xFFFFFFFFU;
    }
    tmp = (uint32_t) h * (uint64_t) 0xC2B0C3CCU;
    h = ((unsigned int) tmp ^ (unsigned int) (tmp >> 32)) & 0xFFFFFFFFU;

    return h;
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

int32 named_instr_find(CSOUND *csound, char *s)
{
    INSTRNAME     *inm;

    if (!csound->engineState.instrumentNames)
      return 0L;                              /* no named instruments defined */
    /* now find instrument */
    inm = cs_hash_table_get(csound, csound->engineState.instrumentNames, s);

    return (inm == NULL) ? 0L : inm->instno;
}

/* allocate entry for named instrument ip with name s (s must not be freed */
/* after the call, because only the pointer is stored); instrument number */
/* is set to insno */
/* Above remark out of date as I added strdup -- JPff march 2013 */
/* returns zero if the named instr entry could not be allocated */
/* (e.g. because it already exists) */

int named_instr_alloc(CSOUND *csound, char *s, INSTRTXT *ip,
                      int32 insno, ENGINE_STATE *engineState)
{
    INSTRNAME *inm, *inm2, *inm_head;

    if (UNLIKELY(!engineState->instrumentNames))
        engineState->instrumentNames = cs_hash_table_create(csound);

    /* now check if instrument is already defined */
    inm = cs_hash_table_get(csound, engineState->instrumentNames, s);
    if (inm != NULL) {
        return 0; /* error: instr exists */
    }

    /* allocate entry, */
    inm = (INSTRNAME*) mcalloc(csound, sizeof(INSTRNAME));
    inm2 = (INSTRNAME*) mcalloc(csound, sizeof(INSTRNAME));
    /* and store parameters */
    inm->name = strdup(s); inm->ip = ip;
    inm2->instno = insno;
    inm2->name = (char*) inm;   /* hack */
    /* link into chain */
    cs_hash_table_put(csound, engineState->instrumentNames, s, inm);

    inm_head = cs_hash_table_get(csound, engineState->instrumentNames,
                                 (char*)INSTR_NAME_FIRST);
    /* temporary chain for use by named_instr_assign_numbers() */
    if (inm_head == NULL) {
        cs_hash_table_put(csound, engineState->instrumentNames,
                          (char*)INSTR_NAME_FIRST, inm2);
    } else {
        while(inm_head->next != NULL) {
            inm_head = inm_head->next;
        }
        inm_head->next = inm2;
    }

    if (UNLIKELY(csound->oparms->odebug) && engineState == &csound->engineState)
      csound->Message(csound,
                      "named instr name = \"%s\", txtp = %p,\n",
                      s, (void*) ip);
    return 1;
}

/* assign instrument numbers to all named instruments */
/* called by otran */

void named_instr_assign_numbers(CSOUND *csound, ENGINE_STATE *engineState)
{
    INSTRNAME   *inm, *inm2, *inm_first;
    int     num = 0, insno_priority = 0;

    if (!engineState->instrumentNames) return;       /* no named instruments */
    inm_first = cs_hash_table_get(csound, engineState->instrumentNames,
                                  (char*)INSTR_NAME_FIRST);

    while (--insno_priority > -3) {
      if (insno_priority == -2) {
        num = engineState->maxinsno;         /* find last used instr number */
        while (!engineState->instrtxtp[num] && --num);
      }
      for (inm = inm_first; inm; inm = inm->next) {
        if ((int) inm->instno != insno_priority) continue;
        /* the following is based on code by Matt J. Ingalls */
        /* find an unused number and use it */
        while (++num <= engineState->maxinsno && engineState->instrtxtp[num]);
        /* we may need to expand the instrument array */
        if (num > engineState->maxinsno) {
          int m = engineState->maxinsno;
          engineState->maxinsno += MAXINSNO; /* Expand */
          engineState->instrtxtp = (INSTRTXT**)
            mrealloc(csound, engineState->instrtxtp,
                             (1 + engineState->maxinsno) * sizeof(INSTRTXT*));
          /* Array expected to be nulled so.... */
          while (++m <= engineState->maxinsno) engineState->instrtxtp[m] = NULL;
        }
        /* hack: "name" actually points to the corresponding INSTRNAME */
        inm2 = (INSTRNAME*) (inm->name);    /* entry in the table */
        inm2->instno = (int32) num;
        engineState->instrtxtp[num] = inm2->ip;
        if (csound->oparms->msglevel && engineState == &csound->engineState)
          csound->Message(csound, Str("instr %s uses instrument number %d\n"),
                                  inm2->name, num);
      }
    }
    /* clear temporary chains */
    inm = inm_first;
    while (inm) {
      INSTRNAME *nxtinm = inm->next;
      mfree(csound, inm);
      inm = nxtinm;
    }
    cs_hash_table_remove(csound, engineState->instrumentNames,
                         (char*)INSTR_NAME_FIRST);
}


/* convert opcode string argument to instrument number */
/* return value is -1 if the instrument cannot be found */
/* (in such cases, csoundInitError() is also called) */

int32 strarg2insno(CSOUND *csound, void *p, int is_string)
{
    int32    insno;

    if (is_string) {
      if (UNLIKELY((insno = named_instr_find(csound, (char*) p)) <= 0)) {
        csound->InitError(csound, Str("instr %s not found"), (char*) p);
        return -1;
      }
    }
    else {      /* numbered instrument */
      insno = (int32) *((MYFLT*) p);
      if (UNLIKELY(insno < 1 || insno > csound->engineState.maxinsno ||
                   !csound->engineState.instrtxtp[insno])) {
        csound->InitError(csound, Str("Cannot Find Instrument %d"), (int) insno);
        return -1;
      }
    }
    return insno;
}

/* same as strarg2insno, but runs at perf time, */
/* and does not support numbered instruments */
/* (used by opcodes like event or schedkwhen) */

int32 strarg2insno_p(CSOUND *csound, char *s)
{
    int32    insno;

    if (UNLIKELY(!(insno = named_instr_find(csound, s)))) {
      csound->PerfError(csound, Str("instr %s not found"), s);
      return -1;
    }
    return insno;
}

/* convert opcode string argument to instrument number */
/* (also allows user defined opcode names); if the integer */
/* argument is non-zero, only opcode names are searched */
/* return value is -1 if the instrument cannot be found */
/* (in such cases, csoundInitError() is also called) */

int32 strarg2opcno(CSOUND *csound, void *p, int is_string, int force_opcode)
{
    int32    insno = 0;

    if (!force_opcode) {        /* try instruments first, if enabled */
      if (is_string) {
        insno = named_instr_find(csound, (char*) p);
      }
      else {      /* numbered instrument */
        insno = (int32) *((MYFLT*) p);
        if (UNLIKELY(insno < 1 || insno > csound->engineState.maxinsno ||
                     !csound->engineState.instrtxtp[insno])) {
          csound->InitError(csound, Str("Cannot Find Instrument %d"), (int) insno);
          return -1;
        }
      }
    }
    if (!insno && is_string) {              /* if no instrument was found, */
      OPCODINFO *inm = csound->opcodeInfo;  /* search for user opcode */
      while (inm && sCmp(inm->name, (char*) p)) inm = inm->prv;
      if (inm) insno = (int32) inm->instno;
    }
    if (UNLIKELY(insno < 1)) {
      csound->InitError(csound,
                        Str("cannot find the specified instrument or opcode"));
      insno = -1;
    }
    return insno;
}

/* create file name from opcode argument (string or MYFLT)      */
/*   CSOUND *csound:                                            */
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

char *strarg2name(CSOUND *csound, char *s, void *p, const char *baseName,
                                  int is_string)
{
    if (is_string) {
      /* opcode string argument */
      if (s == NULL)
        s = mmalloc(csound, strlen((char*) p) + 1);
      strcpy(s, (char*) p);
    }
    else if (ISSTRCOD(*((MYFLT*) p))) {
      /* p-field string, unquote and copy */
      char  *s2 = get_arg_string(csound, *((MYFLT*)p));
      int   i = 0;
      //printf("strarg2name: %g %s\n", *((MYFLT*)p), s2);
      if (s == NULL)
        s = mmalloc(csound, strlen(s2) + 1);
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


/* find opcode with the specified name in opcode list */
/* returns index to opcodlst[], or zero if the opcode cannot be found */

int find_opcode(CSOUND *csound, char *opname)
{
    int     h, n;

    if (opname[0] == (char) 0 ||
        (opname[0] >= (char) '0' && opname[0] <= (char) '9'))
      return 0;

    /* calculate hash value */
    h = (int) name_hash_2(csound, opname);
    /* now find entry in opcode chain */
    n = ((int*) csound->opcode_list)[h];
    while (n) {

      if (!sCmp(opname, csound->opcodlst[n].opname))
        return n;
      n = csound->opcodlst[n].prvnum;
    }

    return 0;
}


/* -------- IV - Jan 29 2005 -------- */


/**
 * Allocate nbytes bytes of memory that can be accessed later by calling
 * csoundQueryGlobalVariable() with the specified name; the space is
 * cleared to zero.
 * Returns CSOUND_SUCCESS on success, CSOUND_ERROR in case of invalid
 * parameters (zero nbytes, invalid or already used name), or
 * CSOUND_MEMORY if there is not enough memory.
 */
PUBLIC int csoundCreateGlobalVariable(CSOUND *csnd,
                                      const char *name, size_t nbytes)
{
    void* p;
    /* create new empty database if it does not exist yet */
    if (csnd->namedGlobals == NULL) {
      csnd->namedGlobals = cs_hash_table_create(csnd);
      if (UNLIKELY(csnd->namedGlobals == NULL))
        return CSOUND_MEMORY;
    }
    /* check for valid parameters */
    if (UNLIKELY(name == NULL))
      return CSOUND_ERROR;
    if (UNLIKELY(name[0] == '\0'))
      return CSOUND_ERROR;
    if (UNLIKELY(nbytes < (size_t) 1 || nbytes >= (size_t) 0x7F000000L))
      return CSOUND_ERROR;

    if (cs_hash_table_get(csnd, csnd->namedGlobals, (char*)name) != NULL)
      return CSOUND_ERROR;

    p = mcalloc(csnd, nbytes);
    if (UNLIKELY(p == NULL))
      return CSOUND_MEMORY;

    cs_hash_table_put(csnd, csnd->namedGlobals, (char*)name, p);
    return CSOUND_SUCCESS;
}

/**
 * Get pointer to space allocated with the name "name".
 * Returns NULL if the specified name is not defined.
 */
PUBLIC void *csoundQueryGlobalVariable(CSOUND *csnd, const char *name)
{
    /* check if there is an actual database to search */
    if (csnd->namedGlobals == NULL) return NULL;

    /* check for a valid name */
    if (name == NULL) return NULL;
    if (name[0] == '\0') return NULL;

    return cs_hash_table_get(csnd, csnd->namedGlobals, (char*) name);
}

/**
 * This function is the same as csoundQueryGlobalVariable(), except the
 * variable is assumed to exist and no error checking is done.
 * Faster, but may crash or return an invalid pointer if 'name' is
 * not defined.
 */
PUBLIC void *csoundQueryGlobalVariableNoCheck(CSOUND *csnd, const char *name)
{
    return cs_hash_table_get(csnd, csnd->namedGlobals, (char*) name);
}

/**
 * Free memory allocated for "name" and remove "name" from the database.
 * Return value is CSOUND_SUCCESS on success, or CSOUND_ERROR if the name is
 * not defined.
 */
PUBLIC int csoundDestroyGlobalVariable(CSOUND *csnd, const char *name)
{
    void *p = cs_hash_table_get(csnd, csnd->namedGlobals, (char*)name);
    if (UNLIKELY(p == NULL))
      return CSOUND_ERROR;

    mfree(csnd, p);
    cs_hash_table_remove(csnd, csnd->namedGlobals, (char*) name);

    return CSOUND_SUCCESS;
}

/**
 * Free entire global variable database. This function is for internal use
 * only (e.g. by RESET routines).
 */
void csoundDeleteAllGlobalVariables(CSOUND *csound)
{
    if (csound == NULL || csound->namedGlobals == NULL) return;

    cs_hash_table_free_complete(csound, csound->namedGlobals);
    csound->namedGlobals = NULL;
}
