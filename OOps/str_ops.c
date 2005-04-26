/*
    str_ops.c:

    Copyright (C) 2005 Istvan Varga, Matt J. Ingalls, John ffitch

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

#include "csdl.h"
#define CSOUND_STR_OPS_C    1
#include "str_ops.h"

#define STRSMAX 8

/* strset by John ffitch */

int strset_init(ENVIRON *csound, STRSET_OP *p)
{
    int   indx = (int) ((double) *(p->indx)
                        + (*(p->indx) >= FL(0.0) ? 0.5 : -0.5));
    char  *val = (char*) p->str;

    if (csound->strsets == NULL) {
      csound->strsmax = STRSMAX;
      csound->strsets = (char **) csound->Calloc(csound, (csound->strsmax + 1)
                                                         * sizeof(char*));
    }
    if (indx > (int) csound->strsmax) {
      int   newmax = (int) csound->strsmax + (int) STRSMAX;
      int   i;
      while (indx > newmax)
        newmax += (int) STRSMAX;
      csound->strsets = (char**) csound->ReAlloc(csound, csound->strsets,
                                                 (newmax + 1) * sizeof(char*));
      for (i = (csound->strsmax + 1); i <= newmax; i++)
        csound->strsets[i] = NULL;
      csound->strsmax = newmax;
    }
    if (csound->strsets == NULL || indx < 0) {
      /* No space left or -ve index */
      csound->Die(csound, Str("illegal strset index"));
    }
    if (csound->strsets[indx] != NULL) {
      csound->Warning(csound, Str("strset index conflict at %d"));
      csound->Warning(csound, Str("previous value: '%s', replaced with '%s'"),
                              csound->strsets[indx], val);
    }
    csound->strsets[indx] = val;
    if ((csound->oparms->msglevel & 7) == 7)
      csound->Message(csound, "Strsets[%d]: '%s'\n", indx, val);
    return OK;
}

int strget_init(ENVIRON *csound, STRGET_OP *p)
{
    int   indx;

    ((char*) p->r)[0] = '\0';
    if (*(p->indx) == SSTRCOD) {
      if (csound->currevent->strarg == NULL)
        return OK;
      if ((int) strlen(csound->currevent->strarg) >= csound->strVarMaxLen)
        return csound->InitError(csound, Str("strget: buffer overflow"));
      strcpy((char*) p->r, csound->currevent->strarg);
      return OK;
    }
    indx = (int) ((double) *(p->indx) + (*(p->indx) >= FL(0.0) ? 0.5 : -0.5));
    if (indx < 0 || indx > (int) csound->strsmax ||
        csound->strsets == NULL || csound->strsets[indx] == NULL)
      return OK;
    if ((int) strlen(csound->strsets[indx]) >= csound->strVarMaxLen)
      return csound->InitError(csound, Str("strget: buffer overflow"));
    strcpy((char*) p->r, csound->strsets[indx]);
    return OK;
}

/* strcpy */

int strcpy_opcode_init(ENVIRON *csound, STRCPY_OP *p)
{
    char  *newVal = (char*) p->str;

    if (p->r == p->str)
      return OK;
    if ((int) strlen(newVal) >= csound->strVarMaxLen)
      return csound->InitError(csound, Str("strcpy: buffer overflow"));
    strcpy((char*) p->r, newVal);
    return OK;
}

int strcpy_opcode_perf(ENVIRON *csound, STRCPY_OP *p)
{
    char  *newVal = (char*) p->str;

    if (p->r == p->str)
      return OK;
    if ((int) strlen(newVal) >= csound->strVarMaxLen)
      return csound->PerfError(csound, Str("strcpy: buffer overflow"));
    strcpy((char*) p->r, newVal);
    return OK;
}

/* strcat */

int strcat_opcode_init(ENVIRON *csound, STRCAT_OP *p)
{
    char  *newVal1 = (char*) p->str1;
    char  *newVal2 = (char*) p->str2;

    if ((int) (strlen(newVal1) + strlen(newVal2)) >= csound->strVarMaxLen)
      return csound->InitError(csound, Str("strcat: buffer overflow"));
    if (p->r != p->str2) {
      if (p->r != p->str1)
        strcpy((char*) p->r, newVal1);
      strcat((char*) p->r, newVal2);
      return OK;
    }
    if (newVal1[0] == '\0')
      return OK;
    memmove(newVal2 + strlen(newVal1), newVal2, strlen(newVal2) + 1);
    if (p->r != p->str1)
      memcpy(newVal2, newVal1, strlen(newVal1));
    return OK;
}

int strcat_opcode_perf(ENVIRON *csound, STRCAT_OP *p)
{
    char  *newVal1 = (char*) p->str1;
    char  *newVal2 = (char*) p->str2;

    if ((int) (strlen(newVal1) + strlen(newVal2)) >= csound->strVarMaxLen)
      return csound->PerfError(csound, Str("strcat: buffer overflow"));
    if (p->r != p->str2) {
      if (p->r != p->str1)
        strcpy((char*) p->r, newVal1);
      strcat((char*) p->r, newVal2);
      return OK;
    }
    if (newVal1[0] == '\0')
      return OK;
    memmove(newVal2 + strlen(newVal1), newVal2, strlen(newVal2) + 1);
    if (p->r != p->str1)
      memcpy(newVal2, newVal1, strlen(newVal1));
    return OK;
}

/* strcmp */

int strcmp_opcode(ENVIRON *csound, STRCAT_OP *p)
{
    int i;

    *(p->r) = FL(0.0);
    if (p->str1 == p->str2)
      return OK;
    i = strcmp((char*) p->str1, (char*) p->str2);
    if (i < 0)
      *(p->r) = FL(-1.0);
    else if (i > 0)
      *(p->r) = FL(1.0);
    return OK;
}

