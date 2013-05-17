/*
    namedins.h:

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

/* namedins.h -- written by Istvan Varga, Oct 2002 */

#ifndef CSOUND_NAMEDINS_H
#define CSOUND_NAMEDINS_H

#include "csoundCore.h"         /* for INSTRTXT */

#ifdef __cplusplus
extern "C" {
#endif

/* check if the string s is a valid instrument or opcode name */
/* return value is zero if the string is not a valid name */

int check_instr_name(char *);

/* find the instrument number for the specified name */
/* return value is zero if none was found */

int32 named_instr_find(CSOUND *, char *);

/* convert opcode string argument to instrument number */
/* return value is -1 if the instrument cannot be found */
/* (in such cases, csoundInitError() is also called) */

int32 strarg2insno(CSOUND *, void *, int);

/* same as strarg2insno, but runs at perf time, */
/* and does not support numbered instruments */
/* (used by opcodes like event or schedkwhen) */

int32 strarg2insno_p(CSOUND *, char *);

/* convert opcode string argument to instrument number */
/* (also allows user defined opcode names); if the integer */
/* argument is non-zero, only opcode names are searched */
/* return value is -1 if the instrument cannot be found */
/* (in such cases, csoundInitError() is also called) */

int32 strarg2opcno(CSOUND *, void *, int, int);

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
/*   return value:                                              */
/*      pointer to the output string; if 's' is not NULL, it is */
/*      always the same as 's', otherwise it is allocated with  */
/*      mmalloc() and the caller is responsible for freeing the */
/*      allocated memory with mfree() or csound->Free()         */

char *strarg2name(CSOUND *, char *, void *, const char *, int);

/* ----------------------------------------------------------------------- */
/* the following functions are for efficient management of the opcode list */

/* find opcode with the specified name in opcode list */
/* returns index to opcodlst[], or zero if the opcode cannot be found */

int find_opcode(CSOUND *, char *);

/* ----------------------------------------------------------------------- */
/* These functions replace the functionality of strsav() in rdorch.c.      */

/* Allocate space for strsav (called once from rdorchfile()). */

//void strsav_create(CSOUND *);

/* Locate string s in database, and return address of stored string (not */
/* necessarily the same as s). If the string is not defined yet, it is   */
/* copied to the database (in such cases, it is allowed to free s after  */
/* the call).                                                            */

//char *strsav_string(CSOUND *, char *);

/* ----------------------------------------------------------------------- */

/* do not touch this ! */
static const unsigned char strhash_tabl_8[256] = {
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

/* faster version that assumes a non-empty string */
static inline unsigned char name_hash_2(CSOUND *csound, const char *s)
{
    const unsigned char *c = (const unsigned char*) &(s[0]);
    unsigned int  h = 0U;
#ifdef LINUX
    do {
      h = csound->strhash_tabl_8[h ^ *c];
    } while (*(++c) != (unsigned char) 0);
#else
    (void) csound;
    do {
      h = strhash_tabl_8[h ^ *c];
    } while (*(++c) != (unsigned char) 0);
#endif
    return (unsigned char) h;
}

static inline int sCmp(const char *x, const char *y)
{
    int   i = 0;
    while (x[i] == y[i] && x[i] != (char) 0)
      i++;
    return (x[i] != y[i]);
}

/* ----------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif

#endif          /* CSOUND_NAMEDINS_H */

