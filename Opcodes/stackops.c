/*
    stackops.c:

    Copyright (C) 2006 Istvan Varga

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
#include "interlocks.h"
#include "pstream.h"

typedef struct CsoundArgStack_s CsoundArgStack_t;

#define CS_STACK_ALIGN          8

#define CS_STACK_END            0
#define CS_STACK_I              (1 << 24)
#define CS_STACK_K              (2 << 24)
#define CS_STACK_A              (3 << 24)
#define CS_STACK_S              (4 << 24)
#define CS_STACK_F              (5 << 24)

struct CsoundArgStack_s {
    void    *curBundle;
    void    *dataSpace;
    int     freeSpaceOffset;
    int     freeSpaceEndOffset;
};

typedef struct STACK_OPCODE_ {
    OPDS    h;
    MYFLT   *iStackSize;
} STACK_OPCODE;

typedef struct PUSH_OPCODE_ {
    OPDS    h;
    MYFLT   *args[32];
    /* argMap[0]: bit mask of init (0) or perf (1) time arg type */
    /* argMap[1]: number of stack bytes required at i-time */
    /* argMap[2]: number of stack bytes required at performace time */
    /* argMap[3] to argMap[35]: 0 terminated type/offset list */
    int     argMap[36];
    CsoundArgStack_t  *pp;
    int     initDone;
} PUSH_OPCODE;

typedef struct POP_OPCODE_ {
    OPDS    h;
    MYFLT   *args[32];
    /* argMap[0]: bit mask of init (0) or perf (1) time arg type */
    /* argMap[1]: number of stack bytes required at i-time */
    /* argMap[2]: number of stack bytes required at performace time */
    /* argMap[3] to argMap[35]: 0 terminated type/offset list */
    int     argMap[36];
    CsoundArgStack_t  *pp;
    int     initDone;
} POP_OPCODE;

/* fassign() was taken from pstream.c, written by Richard Dobson */

static CS_NOINLINE void fassign(CSOUND *csound,
                                PVSDAT *fdst, const PVSDAT *fsrc)
{
    if (UNLIKELY(fsrc->frame.auxp == NULL))
      csound->Die(csound, Str("fsig = : source signal is not initialised"));
    fdst->N = fsrc->N;
    fdst->overlap = fsrc->overlap;
    fdst->winsize = fsrc->winsize;
    fdst->wintype = fsrc->wintype;
    fdst->format = fsrc->format;
    if (fdst->frame.auxp == NULL ||
        fdst->frame.size != (fdst->N + 2L) * (long) sizeof(float))
      csound->AuxAlloc(csound,
                       (fdst->N + 2L) * (long) sizeof(float), &(fdst->frame));
    if (fdst->framecount != fsrc->framecount) {
      memcpy((float*) fdst->frame.auxp, (float*) fsrc->frame.auxp,
             ((size_t) fdst->N + (size_t) 2) * sizeof(float));
      fdst->framecount = fsrc->framecount;
    }
}

static inline int csoundStack_Align(int n)
{
    return ((n + (CS_STACK_ALIGN - 1)) & (~(CS_STACK_ALIGN - 1)));
}

static CS_NOINLINE int csoundStack_Error(void *p, const char *msg)
{
    CSOUND  *csound;

    csound = ((OPDS*) p)->insdshead->csound;
    if (UNLIKELY(csound->ids != NULL && csound->pds == NULL)) {
      csound->InitError(csound, "%s: %s", csound->GetOpcodeName(p), msg);
      csound->LongJmp(csound, CSOUND_INITIALIZATION);
    }
    else if (UNLIKELY(csound->ids == NULL && csound->pds != NULL)) {
      csound->PerfError(csound, "%s: %s", csound->GetOpcodeName(p), msg);
      csound->LongJmp(csound, CSOUND_PERFORMANCE);
    }
    else
      csound->Die(csound, "%s: %s", csound->GetOpcodeName(p), msg);
    /* this is actually never reached */
    return NOTOK;
}

static CS_NOINLINE int csoundStack_OverflowError(void *p)
{
    /* CSOUND  *csound= ((OPDS*) p)->insdshead->csound; */
    return csoundStack_Error(p, Str("stack overflow"));
}

static CS_NOINLINE int csoundStack_EmptyError(void *p)
{
    /* CSOUND  *csound((OPDS*) p)->insdshead->csound; */
    return csoundStack_Error(p, Str("cannot pop from empty stack"));
}

static CS_NOINLINE int csoundStack_TypeError(void *p)
{
    /* CSOUND  *csound = ((OPDS*) p)->insdshead->csound; */
    return csoundStack_Error(p, Str("argument number or type mismatch"));
}

static CS_NOINLINE int csoundStack_LengthError(void *p)
{
    /* CSOUND  *csound = ((OPDS*) p)->insdshead->csound; */
    return csoundStack_Error(p, Str("string argument is too long"));
}

static CS_NOINLINE CsoundArgStack_t *csoundStack_AllocGlobals(CSOUND *csound,
                                                              int stackSize)
{
    CsoundArgStack_t  *pp;
    int               nBytes;

    if (UNLIKELY(stackSize < 1024))
      stackSize = 1024;
    else if (UNLIKELY(stackSize > 16777200))
      stackSize = 16777200;
    nBytes = csoundStack_Align((int) sizeof(CsoundArgStack_t));
    nBytes += stackSize;
    if (UNLIKELY(csound->CreateGlobalVariable(csound,
                                              "csArgStack", (size_t) nBytes)
                 != 0))
      csound->Die(csound, Str("Error allocating argument stack"));
    pp = (CsoundArgStack_t*) csound->QueryGlobalVariable(csound, "csArgStack");
    pp->curBundle = (CsoundArgStack_t*) NULL;
    pp->dataSpace =
        (void*) ((char*) pp
                 + (int) csoundStack_Align((int) sizeof(CsoundArgStack_t)));
    pp->freeSpaceOffset = 0;
    pp->freeSpaceEndOffset = stackSize;

    return pp;
}

static CS_NOINLINE CsoundArgStack_t *csoundStack_GetGlobals(CSOUND *csound)
{
    CsoundArgStack_t  *pp;

    pp = (CsoundArgStack_t*) csound->QueryGlobalVariable(csound, "csArgStack");
    if (pp == NULL)
      pp = csoundStack_AllocGlobals(csound, 32768);
    return pp;
}

static CS_NOINLINE int csoundStack_CreateArgMap(void *p, int *argMap,
                                                int isOutput)
{
    CSOUND  *csound;
    int     i, argCnt, argCnt_i, argCnt_p, aMask, sMask, curOffs_i, curOffs_p;

    csound = ((OPDS*) p)->insdshead->csound;
    if (!isOutput) {
      argCnt = csound->GetInputArgCnt(p);
      aMask = csound->GetInputArgAMask(p);
      sMask = csound->GetInputArgSMask(p);
    }
    else {
      argCnt = csound->GetOutputArgCnt(p);
      aMask = csound->GetOutputArgAMask(p);
      sMask = csound->GetOutputArgSMask(p);
    }
    if (UNLIKELY(argCnt > 31))
      return csoundStack_Error(p, Str("too many arguments"));
    argMap[0] = 0;
    argCnt_i = 0;
    argCnt_p = 0;
    for (i = 0; i < argCnt; i++) {
      int   maskVal = (1 << i);
      if (aMask & maskVal) {
        argMap[0] |= maskVal;
        argCnt_p++;
      }
      else if (sMask & maskVal) {
        argCnt_i++;
      }
      else {
        const char  *argName;
        if (!isOutput)
          argName = csound->GetInputArgName(p, i);
        else
          argName = csound->GetOutputArgName(p, i);
        if (argName != (char*) 0 &&
            (argName[0] == (char) 'k' ||
             (argName[0] == (char) 'g' && argName[1] == (char) 'k') ||
             (argName[0] == (char) '#' && argName[1] == (char) 'k'))) {
          argMap[0] |= maskVal;
          argCnt_p++;
        }
        else
          argCnt_i++;
      }
    }
    curOffs_i = (int) sizeof(void*);
    curOffs_i = csoundStack_Align(curOffs_i);
    curOffs_p = curOffs_i;
    curOffs_i += ((int) sizeof(int) * (argCnt_i + 1));
    curOffs_i = csoundStack_Align(curOffs_i);
    curOffs_p += ((int) sizeof(int) * (argCnt_p + 1));
    curOffs_p = csoundStack_Align(curOffs_p);
    for (i = 0; i < argCnt; i++) {
      int   maskVal = (1 << i);
      if (argMap[0] & maskVal) {
        /* performance time types */
        if (aMask & maskVal) {
          argMap[i + 3] = (curOffs_p | CS_STACK_A);
          curOffs_p += ((int) sizeof(MYFLT) * csound->ksmps);
        }
        else {
          argMap[i + 3] = (curOffs_p | CS_STACK_K);
          curOffs_p += (int) sizeof(MYFLT);
        }
      }
      else {
        /* init time types */
        if (sMask & maskVal) {
          argMap[i + 3] = (curOffs_i | CS_STACK_S);
          curOffs_i += csound->strVarMaxLen;
          curOffs_i = csoundStack_Align(curOffs_i);
        }
        else {
          argMap[i + 3] = (curOffs_i | CS_STACK_I);
          curOffs_i += (int) sizeof(MYFLT);
        }
      }
    }
    argMap[i + 3] = CS_STACK_END;
    if (argCnt_i > 0)
      argMap[1] = csoundStack_Align(curOffs_i);
    else
      argMap[1] = 0;
    if (argCnt_p > 0)
      argMap[2] = csoundStack_Align(curOffs_p);
    else
      argMap[2] = 0;

    return OK;
}

static int stack_opcode_init(CSOUND *csound, STACK_OPCODE *p)
{
    if (UNLIKELY(csound->QueryGlobalVariable(csound, "csArgStack") != NULL))
      return csound->InitError(csound, Str("the stack is already allocated"));
    csoundStack_AllocGlobals(csound, (int) (*(p->iStackSize) + 0.5));
    return OK;
}

static int notinit_opcode_stub_perf(CSOUND *csound, void *p)
{
    return csound->PerfError(csound, Str("%s: not initialised"),
                                     csound->GetOpcodeName(p));
}

static int push_opcode_perf(CSOUND *csound, PUSH_OPCODE *p)
{
    if (p->argMap[2] != 0) {
      void  *bp;
      int   i, *ofsp;
      if (p->pp->freeSpaceOffset + p->argMap[2] > p->pp->freeSpaceEndOffset)
        return csoundStack_OverflowError(p);
      bp = (void*) ((char*) p->pp->dataSpace + (int) p->pp->freeSpaceOffset);
      p->pp->freeSpaceOffset += p->argMap[2];
      *((void**) bp) = p->pp->curBundle;
      p->pp->curBundle = bp;
      ofsp = (int*) ((char*) bp + (int) csoundStack_Align((int) sizeof(void*)));
      for (i = 0; p->argMap[i + 3] != CS_STACK_END; i++) {
        if (p->argMap[0] & (1 << i)) {
          int   curOffs = p->argMap[i + 3];
          *(ofsp++) = curOffs;
          switch (curOffs & (int) 0x7F000000) {
          case CS_STACK_K:
            *((MYFLT*) ((char*) bp + (int) (curOffs & (int) 0x00FFFFFF))) =
                *(p->args[i]);
            break;
          case CS_STACK_A:
            {
              MYFLT *src, *dst;
              int   j;
              src = p->args[i];
              dst = (MYFLT*) ((char*) bp + (int) (curOffs & (int) 0x00FFFFFF));
              //memcpy(dst, src, sizeof(MYFLT)*csound->ksmps;
              for (j = 0; j < csound->ksmps; j++)
                dst[j] = src[j];
            }
          }
        }
      }
      *ofsp = CS_STACK_END;
    }
    return OK;
}

static int push_opcode_init(CSOUND *csound, PUSH_OPCODE *p)
{
    if (!p->initDone) {
      p->pp = csoundStack_GetGlobals(csound);
      if (UNLIKELY(csoundStack_CreateArgMap(p, &(p->argMap[0]), 0) != OK))
        return NOTOK;
      p->h.opadr = (int (*)(CSOUND *, void *)) push_opcode_perf;
      p->initDone = 1;
    }
    if (p->argMap[1] != 0) {
      void  *bp;
      int   i, *ofsp;
      if (UNLIKELY(p->pp->freeSpaceOffset + p->argMap[1] >
                   p->pp->freeSpaceEndOffset))
        return csoundStack_OverflowError(p);
      bp = (void*) ((char*) p->pp->dataSpace + (int) p->pp->freeSpaceOffset);
      p->pp->freeSpaceOffset += p->argMap[1];
      *((void**) bp) = p->pp->curBundle;
      p->pp->curBundle = bp;
      ofsp = (int*) ((char*) bp + (int) csoundStack_Align((int) sizeof(void*)));
      for (i = 0; p->argMap[i + 3] != CS_STACK_END; i++) {
        if (!(p->argMap[0] & (1 << i))) {
          int   curOffs = p->argMap[i + 3];
          *(ofsp++) = curOffs;
          switch (curOffs & (int) 0x7F000000) {
          case CS_STACK_I:
            *((MYFLT*) ((char*) bp + (int) (curOffs & (int) 0x00FFFFFF))) =
                *(p->args[i]);
            break;
          case CS_STACK_S:
            {
              char  *src, *dst;
              int   j, maxLen;
              src = (char*) p->args[i];
              dst = (char*) bp + (int) (curOffs & (int) 0x00FFFFFF);
              maxLen = csound->strVarMaxLen - 1;
              for (j = 0; src[j] != (char) 0; j++) {
                dst[j] = src[j];
                if (j >= maxLen) {
                  dst[j] = (char) 0;
                  csoundStack_LengthError(p);
                }
              }
              dst[j] = (char) 0;
            }
          }
        }
      }
      *ofsp = CS_STACK_END;
    }
    return OK;
}

static int pop_opcode_perf(CSOUND *csound, POP_OPCODE *p)
{
    if (p->argMap[2] != 0) {
      void  *bp;
      int   i, *ofsp;
      if (UNLIKELY(p->pp->curBundle == NULL))
        return csoundStack_EmptyError(p);
      bp = p->pp->curBundle;
      ofsp = (int*) ((char*) bp + (int) csoundStack_Align((int) sizeof(void*)));
      for (i = 0; *ofsp != CS_STACK_END; i++) {
        if (p->argMap[0] & (1 << i)) {
          int   curOffs = p->argMap[i + 3];
          if (curOffs != *ofsp)
            csoundStack_TypeError(p);
          ofsp++;
          switch (curOffs & (int) 0x7F000000) {
          case CS_STACK_K:
            *(p->args[i]) =
                *((MYFLT*) ((char*) bp + (int) (curOffs & (int) 0x00FFFFFF)));
            break;
          case CS_STACK_A:
            {
              MYFLT *src, *dst;
              int   j;
              src = (MYFLT*) ((char*) bp + (int) (curOffs & (int) 0x00FFFFFF));
              dst = p->args[i];
              for (j = 0; j < csound->ksmps; j++)
                dst[j] = src[j];
            }
            break;
          }
        }
      }
      p->pp->curBundle = *((void**) bp);
      p->pp->freeSpaceOffset = (int) ((char*) bp - (char*) p->pp->dataSpace);
    }
    return OK;
}

static int pop_opcode_init(CSOUND *csound, POP_OPCODE *p)
{
    if (!p->initDone) {
      p->pp = csoundStack_GetGlobals(csound);
      if (UNLIKELY(csoundStack_CreateArgMap(p, &(p->argMap[0]), 1) != OK))
        return NOTOK;
      p->h.opadr = (int (*)(CSOUND *, void *)) pop_opcode_perf;
      p->initDone = 1;
    }
    if (p->argMap[1] != 0) {
      void  *bp;
      int   i, *ofsp;
      if (p->pp->curBundle == NULL)
        return csoundStack_EmptyError(p);
      bp = p->pp->curBundle;
      ofsp = (int*) ((char*) bp + (int) csoundStack_Align((int) sizeof(void*)));
      for (i = 0; *ofsp != CS_STACK_END; i++) {
        if (!(p->argMap[0] & (1 << i))) {
          int   curOffs = p->argMap[i + 3];
          if (curOffs != *ofsp)
            csoundStack_TypeError(p);
          ofsp++;
          switch (curOffs & (int) 0x7F000000) {
          case CS_STACK_I:
            *(p->args[i]) =
                *((MYFLT*) ((char*) bp + (int) (curOffs & (int) 0x00FFFFFF)));
            break;
          case CS_STACK_S:
            strcpy((char*) p->args[i],
                   (char*) bp + (int) (curOffs & (int) 0x00FFFFFF));
            break;
          }
        }
      }
      p->pp->curBundle = *((void**) bp);
      p->pp->freeSpaceOffset = (int) ((char*) bp - (char*) p->pp->dataSpace);
    }
    return OK;
}

static int push_f_opcode_perf(CSOUND *csound, PUSH_OPCODE *p)
{
    void    *bp;
    int     *ofsp;
    int     offs;

    if (UNLIKELY(p->pp->freeSpaceOffset + p->argMap[2] >
                 p->pp->freeSpaceEndOffset))
      return csoundStack_OverflowError(p);
    bp = (void*) ((char*) p->pp->dataSpace + (int) p->pp->freeSpaceOffset);
    p->pp->freeSpaceOffset += p->argMap[2];
    *((void**) bp) = p->pp->curBundle;
    p->pp->curBundle = bp;
    ofsp = (int*) ((char*) bp + (int) csoundStack_Align((int) sizeof(void*)));
    offs = p->argMap[3];
    *(ofsp++) = offs;
    *((PVSDAT**) ((char*) bp + (int) (offs & (int) 0x00FFFFFF))) =
        (PVSDAT*) ((char*) p->args[0]);
    *ofsp = CS_STACK_END;

    return OK;
}

static int push_f_opcode_init(CSOUND *csound, PUSH_OPCODE *p)
{
    void    *bp;
    int     *ofsp;
    int     offs;

    if (!p->initDone) {
      p->pp = csoundStack_GetGlobals(csound);
      offs = (int) sizeof(void*);
      offs = csoundStack_Align(offs);
      offs += ((int) sizeof(int) * 2);
      offs = csoundStack_Align(offs);
      /* the 'f' type is special: */
      /*   it is copied at both init and performance time */
      p->argMap[0] = 1;
      p->argMap[3] = (CS_STACK_F | offs);
      p->argMap[4] = CS_STACK_END;
      /* FIXME: store only the address of the f-signal on the stack */
      /* this may be dangerous... */
      offs += (int) sizeof(PVSDAT*);
      offs = csoundStack_Align(offs);
      p->argMap[1] = offs;
      p->argMap[2] = offs;
      p->h.opadr = (int (*)(CSOUND *, void *)) push_f_opcode_perf;
      p->initDone = 1;
    }
    if (UNLIKELY(p->pp->freeSpaceOffset + p->argMap[1] > p->pp->freeSpaceEndOffset))
      return csoundStack_OverflowError(p);
    bp = (void*) ((char*) p->pp->dataSpace + (int) p->pp->freeSpaceOffset);
    p->pp->freeSpaceOffset += p->argMap[1];
    *((void**) bp) = p->pp->curBundle;
    p->pp->curBundle = bp;
    ofsp = (int*) ((char*) bp + (int) csoundStack_Align((int) sizeof(void*)));
    offs = p->argMap[3];
    *(ofsp++) = offs;
    *((PVSDAT**) ((char*) bp + (int) (offs & (int) 0x00FFFFFF))) =
        (PVSDAT*) ((char*) p->args[0]);
    *ofsp = CS_STACK_END;

    return OK;
}

static int pop_f_opcode_perf(CSOUND *csound, POP_OPCODE *p)
{
    void    *bp;
    int     *ofsp;
    int     offs;

    if (UNLIKELY(p->pp->curBundle == NULL))
      return csoundStack_EmptyError(p);
    bp = p->pp->curBundle;
    ofsp = (int*) ((char*) bp + (int) csoundStack_Align((int) sizeof(void*)));
    offs = p->argMap[3];
    if (UNLIKELY(offs != *ofsp))
      csoundStack_TypeError(p);
    ofsp++;
    if (UNLIKELY(*ofsp != CS_STACK_END))
      csoundStack_TypeError(p);
    fassign(csound,
            (PVSDAT*) p->args[0],
            *((PVSDAT**) ((char*) bp + (int) (offs & (int) 0x00FFFFFF))));
    p->pp->curBundle = *((void**) bp);
    p->pp->freeSpaceOffset = (int) ((char*) bp - (char*) p->pp->dataSpace);

    return OK;
}

static int pop_f_opcode_init(CSOUND *csound, POP_OPCODE *p)
{
    void    *bp;
    int     *ofsp;
    int     offs;

    if (!p->initDone) {
      p->pp = csoundStack_GetGlobals(csound);
      offs = (int) sizeof(void*);
      offs = csoundStack_Align(offs);
      offs += ((int) sizeof(int) * 2);
      offs = csoundStack_Align(offs);
      /* the 'f' type is special: */
      /*   it is copied at both init and performance time */
      p->argMap[0] = 1;
      p->argMap[3] = (CS_STACK_F | offs);
      p->argMap[4] = CS_STACK_END;
      /* FIXME: only the address of the f-signal is stored on the stack */
      /* this may be dangerous... */
      offs += (int) sizeof(PVSDAT*);
      offs = csoundStack_Align(offs);
      p->argMap[1] = offs;
      p->argMap[2] = offs;
      p->h.opadr = (int (*)(CSOUND *, void *)) pop_f_opcode_perf;
      p->initDone = 1;
    }
    if (UNLIKELY(p->pp->curBundle == NULL))
      return csoundStack_EmptyError(p);
    bp = p->pp->curBundle;
    ofsp = (int*) ((char*) bp + (int) csoundStack_Align((int) sizeof(void*)));
    offs = p->argMap[3];
    if (UNLIKELY(offs != *ofsp))
      csoundStack_TypeError(p);
    ofsp++;
    if (UNLIKELY(*ofsp != CS_STACK_END))
      csoundStack_TypeError(p);
    fassign(csound,
            (PVSDAT*) p->args[0],
            *((PVSDAT**) ((char*) bp + (int) (offs & (int) 0x00FFFFFF))));
    p->pp->curBundle = *((void**) bp);
    p->pp->freeSpaceOffset = (int) ((char*) bp - (char*) p->pp->dataSpace);

    return OK;
}

 /* ------------------------------------------------------------------------ */

typedef struct MONITOR_OPCODE_ {
    OPDS    h;
    MYFLT   *ar[24];
} MONITOR_OPCODE;

static int monitor_opcode_perf(CSOUND *csound, MONITOR_OPCODE *p)
{
    int     i, j;

    if (csound->spoutactive) {
      int   k = 0;
      i = 0;
      do {
        j = 0;
        do {
          p->ar[j][i] = csound->spout[k++];
        } while (++j < csound->nchnls);
      } while (++i < csound->ksmps);
    }
    else {
      j = 0;
      do {
        i = 0;
        do {
          p->ar[j][i] = FL(0.0);
        } while (++i < csound->ksmps);
      } while (++j < csound->nchnls);
    }
    return OK;
}

static int monitor_opcode_init(CSOUND *csound, MONITOR_OPCODE *p)
{
    if (UNLIKELY(csound->GetOutputArgCnt(p) != csound->nchnls))
      return csound->InitError(csound, Str("number of arguments != nchnls"));
    p->h.opadr = (SUBR) monitor_opcode_perf;
    return OK;
}

 /* ------------------------------------------------------------------------ */

static OENTRY stackops_localops[] = {
  { "stack",  sizeof(STACK_OPCODE), SB|1,  "",                                "i",
      (SUBR) stack_opcode_init, (SUBR) NULL,                      (SUBR) NULL },
  { "push",   sizeof(PUSH_OPCODE),  SB|3,  "",                                "N",
      (SUBR) push_opcode_init,  (SUBR) notinit_opcode_stub_perf,  (SUBR) NULL },
  { "pop",    sizeof(POP_OPCODE),   SB|3,  "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN", "",
      (SUBR) pop_opcode_init,   (SUBR) notinit_opcode_stub_perf,  (SUBR) NULL },
  { "push_f", sizeof(PUSH_OPCODE),  SB|3,  "",                                "f",
      (SUBR) push_f_opcode_init, (SUBR) notinit_opcode_stub_perf, (SUBR) NULL },
  { "pop_f",  sizeof(POP_OPCODE),   SB|3,  "f",                               "",
      (SUBR) pop_f_opcode_init,  (SUBR) notinit_opcode_stub_perf, (SUBR) NULL },
  /* ----------------------------------------------------------------------- */
  { "monitor",  sizeof(MONITOR_OPCODE), 3,  "mmmmmmmmmmmmmmmmmmmmmmmm", "",
    (SUBR) monitor_opcode_init, (SUBR) notinit_opcode_stub_perf,  (SUBR) NULL }
};

LINKAGE1(stackops_localops)

