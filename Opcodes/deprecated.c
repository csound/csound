/*
    deprecated.c: various deprecated opcodes 

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

#include "csdl.h"
#include "interlocks.h"
#include "pstream.h"

/**    
 stackops: copyright (C) 2006 Istvan Varga
*/
static int32_t STR_ARG_P(CSOUND *csound, void* arg) {
    CS_TYPE *cs_type = csound->GetTypeForArg(arg);
    if (strcmp("S", cs_type->varTypeName) == 0) {
      return 1;
    } else {
      return 0;
    }
}

static int32_t ASIG_ARG_P(CSOUND *csound, void* arg) {
    CS_TYPE *cs_type = csound->GetTypeForArg(arg);
    if (strcmp("a", cs_type->varTypeName) == 0) {
      return 1;
    } else {
      return 0;
    }
}

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
    int32_t     freeSpaceOffset;
    int32_t     freeSpaceEndOffset;
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
    int32_t     argMap[36];
    CsoundArgStack_t  *pp;
    int32_t     initDone;
} PUSH_OPCODE;

typedef struct POP_OPCODE_ {
    OPDS    h;
    MYFLT   *args[32];
    /* argMap[0]: bit mask of init (0) or perf (1) time arg type */
    /* argMap[1]: number of stack bytes required at i-time */
    /* argMap[2]: number of stack bytes required at performace time */
    /* argMap[3] to argMap[35]: 0 terminated type/offset list */
    int32_t     argMap[36];
    CsoundArgStack_t  *pp;
    int32_t     initDone;
} POP_OPCODE;

/* fsg_assign() was taken from pstream.c, written by Richard Dobson */

static CS_NOINLINE void fsg_assign(CSOUND *csound,
                                PVSDAT *fdst, const PVSDAT *fsrc)
{
  if (UNLIKELY(fsrc->frame.auxp == NULL)) {
      csound->ErrorMsg(csound, "%s",
                       Str("fsig = : source signal is not initialised"));
      return;
  }
    fdst->N = fsrc->N;
    fdst->overlap = fsrc->overlap;
    fdst->winsize = fsrc->winsize;
    fdst->wintype = fsrc->wintype;
    fdst->format = fsrc->format;
    if (fdst->frame.auxp == NULL ||
        fdst->frame.size != (uint32_t)((fdst->N + 2L) * sizeof(float)))
      csound->AuxAlloc(csound,
                       (fdst->N + 2L) * (int64_t) sizeof(float), &(fdst->frame));
    if (fdst->framecount != fsrc->framecount) {
      memcpy((float*) fdst->frame.auxp, (float*) fsrc->frame.auxp,
             ((size_t) fdst->N + (size_t) 2) * sizeof(float));
      fdst->framecount = fsrc->framecount;
    }
}

static inline int32_t csoundStack_Align(int32_t n)
{
    return ((n + (CS_STACK_ALIGN - 1)) & (~(CS_STACK_ALIGN - 1)));
}

static CS_NOINLINE int32_t csoundStack_Error(void *p, const char *msg)
{
    CSOUND  *csound;

    csound = ((OPDS*) p)->insdshead->csound;
    csound->ErrorMsg(csound, "%s: %s", csound->GetOpcodeName(p), msg);

    return NOTOK;
}

static CS_NOINLINE int32_t csoundStack_OverflowError(void *p)
{
    /* CSOUND  *csound= ((OPDS*) p)->insdshead->csound; */
    return csoundStack_Error(p, "stack overflow");
}

static CS_NOINLINE int32_t csoundStack_EmptyError(void *p)
{
    /* CSOUND  *csound((OPDS*) p)->insdshead->csound; */
    return csoundStack_Error(p, "cannot pop from empty stack");
}

static CS_NOINLINE int32_t csoundStack_TypeError(void *p)
{
    /* CSOUND  *csound = ((OPDS*) p)->insdshead->csound; */
    return csoundStack_Error(p, "argument number or type mismatch");
}

/* static CS_NOINLINE int32_t csoundStack_LengthError(void *p) */
/* { */
/*     /\* CSOUND  *csound = ((OPDS*) p)->insdshead->csound; *\/ */
/*     return csoundStack_Error(p, Str("string argument is too long")); */
/* } */

static CS_NOINLINE CsoundArgStack_t *csoundStack_AllocGlobals(CSOUND *csound,
                                                              int32_t stackSize)
{
    CsoundArgStack_t  *pp;
    int32_t               nBytes;

    if (UNLIKELY(stackSize < 1024))
      stackSize = 1024;
    else if (UNLIKELY(stackSize > 16777200))
      stackSize = 16777200;
    nBytes = csoundStack_Align((int32_t) sizeof(CsoundArgStack_t));
    nBytes += stackSize;
    if (UNLIKELY(csound->CreateGlobalVariable(csound,
                                              "csArgStack", (size_t) nBytes)
                 != 0)) {
      csound->ErrorMsg(csound, "%s", Str("Error allocating argument stack"));
      return NULL;
    }
    pp = (CsoundArgStack_t*) csound->QueryGlobalVariable(csound, "csArgStack");
    pp->curBundle = (CsoundArgStack_t*) NULL;
    pp->dataSpace =
        (void*) ((char*) pp
                 + (int32_t) csoundStack_Align((int32_t) sizeof(CsoundArgStack_t)));
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

static CS_NOINLINE int32_t csoundStack_CreateArgMap(PUSH_OPCODE *p, int32_t *argMap,
                                                int32_t isOutput)
{
    CSOUND  *csound;
    int32_t     i, argCnt, argCnt_i, argCnt_p, curOffs_i, curOffs_p;
    MYFLT** args = p->args;

    csound = ((OPDS*) p)->insdshead->csound;
    if (!isOutput) {
      argCnt = csound->GetInputArgCnt(p);
    }
    else {
      argCnt = csound->GetOutputArgCnt(p);
    }
    if (UNLIKELY(argCnt > 31))
      return csoundStack_Error(p, "too many arguments");
    argMap[0] = 0;
    argCnt_i = 0;
    argCnt_p = 0;
    for (i = 0; i < argCnt; i++) {
      int32_t   maskVal = (1 << i);
      if (ASIG_ARG_P(csound, args[i])) {
        argMap[0] |= maskVal;
        argCnt_p++;
      }
      else if (STR_ARG_P(csound, args[i])) {
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
    curOffs_i = (int32_t) sizeof(void*);
    curOffs_i = csoundStack_Align(curOffs_i);
    curOffs_p = curOffs_i;
    curOffs_i += ((int32_t) sizeof(int32_t) * (argCnt_i + 1));
    curOffs_i = csoundStack_Align(curOffs_i);
    curOffs_p += ((int32_t) sizeof(int32_t) * (argCnt_p + 1));
    curOffs_p = csoundStack_Align(curOffs_p);
    for (i = 0; i < argCnt; i++) {
      int32_t   maskVal = (1 << i);
      if (argMap[0] & maskVal) {
        /* performance time types */
        if (ASIG_ARG_P(csound, args[i])) {
          argMap[i + 3] = (curOffs_p | CS_STACK_A);
          curOffs_p += ((int32_t) sizeof(MYFLT) * CS_KSMPS);
        }
        else {
          argMap[i + 3] = (curOffs_p | CS_STACK_K);
          curOffs_p += (int32_t) sizeof(MYFLT);
        }
      }
      else {
        /* init time types */
        if (STR_ARG_P(csound, args[i])) {
          argMap[i + 3] = (curOffs_i | CS_STACK_S);
          curOffs_i += (int32_t) sizeof(STRINGDAT);
            /* curOffs_i = csoundStack_Align(curOffs_i);*/
        }
        else {
          argMap[i + 3] = (curOffs_i | CS_STACK_I);
          curOffs_i += (int32_t) sizeof(MYFLT);
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

static int32_t stack_opcode_init(CSOUND *csound, STACK_OPCODE *p)
{
    if (UNLIKELY(csound->QueryGlobalVariable(csound, "csArgStack") != NULL))
      return csound->InitError(csound, "%s", Str("the stack is already allocated"));
    csoundStack_AllocGlobals(csound, (int32_t) (*(p->iStackSize) + 0.5));
    return OK;
}

static int32_t notinit_opcode_stub_perf(CSOUND *csound, void *p)
{
    return csound->PerfError(csound, &(((STACK_OPCODE*)p)->h),
                             Str("%s: not initialised"),
                             csound->GetOpcodeName(p));
}

static int32_t push_opcode_perf(CSOUND *csound, PUSH_OPCODE *p)
{
     IGN(csound);
    if (p->argMap[2] != 0) {
      void  *bp;
      int32_t   i, *ofsp;
      if (p->pp->freeSpaceOffset + p->argMap[2] > p->pp->freeSpaceEndOffset)
        return csoundStack_OverflowError(p);
      bp = (void*) ((char*) p->pp->dataSpace + (int32_t) p->pp->freeSpaceOffset);
      p->pp->freeSpaceOffset += p->argMap[2];
      *((void**) bp) = p->pp->curBundle;
      p->pp->curBundle = bp;
      ofsp = (int32_t*) ((char*) bp +
                         (int32_t) csoundStack_Align((int32_t) sizeof(void*)));
      for (i = 0; p->argMap[i + 3] != CS_STACK_END; i++) {
        if (p->argMap[0] & (1 << i)) {
          int32_t   curOffs = p->argMap[i + 3];
          *(ofsp++) = curOffs;
          switch (curOffs & (int32_t) 0x7F000000) {
          case CS_STACK_K:
            *((MYFLT*) ((char*) bp + (int32_t) (curOffs & (int32_t) 0x00FFFFFF))) =
                *(p->args[i]);
            break;
          case CS_STACK_A:
            {
              MYFLT *src, *dst;
              uint32_t offset = p->h.insdshead->ksmps_offset;
              uint32_t early  = p->h.insdshead->ksmps_no_end;
              uint32_t nsmps = CS_KSMPS;
              src = p->args[i];
              dst = (MYFLT*) ((char*) bp +
                              (int32_t) (curOffs & (int32_t) 0x00FFFFFF));
              if (UNLIKELY(offset)) memset(dst, '\0', offset*sizeof(MYFLT));
              if (UNLIKELY(early)) {
                nsmps -= early;
                memset(&dst[nsmps], '\0', early*sizeof(MYFLT));
              }
              memcpy(&dst[offset], &src[offset], sizeof(MYFLT)*(nsmps-offset));
              //for (j = 0; j < nsmps; j++)
              //dst[j] = src[j];
            }
          }
        }
      }
      *ofsp = CS_STACK_END;
    }
    return OK;
}

static int32_t push_opcode_init(CSOUND *csound, PUSH_OPCODE *p)
{
    if (!p->initDone) {
      p->pp = csoundStack_GetGlobals(csound);
      if (UNLIKELY(csoundStack_CreateArgMap(p, (int32_t*)&(p->argMap[0]), 0) != OK))
        return NOTOK;
      p->h.opadr = (int32_t (*)(CSOUND *, void *)) push_opcode_perf;
      p->initDone = 1;
    }
    if (p->argMap[1] != 0) {
      void  *bp;
      int32_t   i, *ofsp;
      if (UNLIKELY(p->pp->freeSpaceOffset + p->argMap[1] >
                   p->pp->freeSpaceEndOffset))
        return csoundStack_OverflowError(p);
      bp = (void*) ((char*) p->pp->dataSpace + (int32_t) p->pp->freeSpaceOffset);
      p->pp->freeSpaceOffset += p->argMap[1];
      *((void**) bp) = p->pp->curBundle;
      p->pp->curBundle = bp;
      ofsp = (int32_t*) ((char*) bp +
                         (int32_t) csoundStack_Align((int32_t) sizeof(void*)));
      for (i = 0; p->argMap[i + 3] != CS_STACK_END; i++) {
        if (!(p->argMap[0] & (1 << i))) {
          int32_t   curOffs = p->argMap[i + 3];
          *(ofsp++) = curOffs;
          switch (curOffs & (int32_t) 0x7F000000) {
          case CS_STACK_I:
            *((MYFLT*) ((char*) bp + (int32_t) (curOffs & (int32_t) 0x00FFFFFF))) =
                *(p->args[i]);
            break;
          case CS_STACK_S:
            {
              char  *src;
              STRINGDAT **ans, *dst;
              /* int32_t   j, maxLen; */
              src = ((STRINGDAT*) p->args[i])->data;
              ans = ((STRINGDAT**)(char*) bp +
                     (int32_t) (curOffs & (int32_t) 0x00FFFFFF));
              dst = (STRINGDAT*) csound->Malloc(csound, sizeof(STRINGDAT));
              dst->data = csound->Strdup(csound, src);
              dst->size = strlen(src) + 1;
              *ans = dst;
              /* printf("***dst = %p %p %d, \"%s\"\n", */
              /*        ans, dst, dst->size, dst->data); */
            }
          }
        }
      }
      *ofsp = CS_STACK_END;
    }
    return OK;
}

static int32_t pop_opcode_perf(CSOUND *csound, POP_OPCODE *p)
{
     IGN(csound);
    if (p->argMap[2] != 0) {
      void  *bp;
      int32_t   i, *ofsp;
      if (UNLIKELY(p->pp->curBundle == NULL))
        return csoundStack_EmptyError(p);
      bp = p->pp->curBundle;
      ofsp = (int32_t*) ((char*) bp +
                         (int32_t) csoundStack_Align((int32_t) sizeof(void*)));
      for (i = 0; *ofsp != CS_STACK_END; i++) {
        if (p->argMap[0] & (1 << i)) {
          int32_t   curOffs = p->argMap[i + 3];
          if (curOffs != *ofsp)
            csoundStack_TypeError(p);
          ofsp++;
          switch (curOffs & (int32_t) 0x7F000000) {
          case CS_STACK_K:
            *(p->args[i]) =
                *((MYFLT*) ((char*) bp +
                            (int32_t) (curOffs & (int32_t) 0x00FFFFFF)));
            break;
          case CS_STACK_A:
            {
              MYFLT *src, *dst;
              uint32_t offset = p->h.insdshead->ksmps_offset;
              uint32_t early  = p->h.insdshead->ksmps_no_end;
              uint32_t nsmps = CS_KSMPS;
              src = (MYFLT*) ((char*) bp +
                              (int32_t) (curOffs & (int32_t) 0x00FFFFFF));
              dst = p->args[i];
              if (UNLIKELY(offset)) memset(dst, '\0', offset*sizeof(MYFLT));
              if (UNLIKELY(early)) {
                nsmps -= early;
                memset(&dst[nsmps], '\0', early*sizeof(MYFLT));
              }
              memcpy(&dst[offset], &src[offset], (nsmps-offset)*sizeof(MYFLT));
              //for (j = 0; j < CS_KSMPS; j++)
              //  dst[j] = src[j];
            }
            break;
          }
        }
      }
      p->pp->curBundle = *((void**) bp);
      p->pp->freeSpaceOffset = (int32_t) ((char*) bp - (char*) p->pp->dataSpace);
    }
    return OK;
}

static int32_t pop_opcode_init(CSOUND *csound, POP_OPCODE *p)
{
    if (!p->initDone) {
      p->pp = csoundStack_GetGlobals(csound);
      if (UNLIKELY(csoundStack_CreateArgMap((PUSH_OPCODE*)p,
                                            &(p->argMap[0]), 1) != OK))
        return NOTOK;
      p->h.opadr = (int32_t (*)(CSOUND *, void *)) pop_opcode_perf;
      p->initDone = 1;
    }
    if (p->argMap[1] != 0) {
      void  *bp;
      int32_t   i, *ofsp;
      if (p->pp->curBundle == NULL)
        return csoundStack_EmptyError(p);
      bp = p->pp->curBundle;
      ofsp = (int32_t*) ((char*) bp +
                         (int32_t) csoundStack_Align((int32_t) sizeof(void*)));
      for (i = 0; *ofsp != CS_STACK_END; i++) {
        if (!(p->argMap[0] & (1 << i))) {
          int32_t   curOffs = p->argMap[i + 3];
          if (curOffs != *ofsp)
            csoundStack_TypeError(p);
          ofsp++;
          switch (curOffs & (int32_t) 0x7F000000) {
          case CS_STACK_I:
            *(p->args[i]) =
                *((MYFLT*) ((char*) bp +
                            (int32_t) (curOffs & (int32_t) 0x00FFFFFF)));
            break;
          case CS_STACK_S:
            {
              STRINGDAT **ans =
                ((STRINGDAT**)(char*) bp +
                 (int32_t) (curOffs & (int32_t) 0x00FFFFFF));
              STRINGDAT *str = *ans;
              STRINGDAT *dst = (STRINGDAT*)p->args[i];
              /* printf("***string: %p\nbp=%p Off = %x\n", ans, bp, curOffs); */
              /* printf("***string: %p->%s\n", str, dst->data); */
              if (str==NULL)
                return csound->InitError(csound, "pop of strings broken");
              if (str->size>dst->size) {
                csound->Free(csound,dst->data);
                dst->data = csound->Strdup(csound, str->data);
                dst->size = strlen(dst->data)+1;
              }
              else {
                strcpy((char*) dst->data, str->data);
              }
              csound->Free(csound,str->data);
              csound->Free(csound,str);
              *ans = NULL;
            }
            break;
          }
        }
      }
      p->pp->curBundle = *((void**) bp);
      p->pp->freeSpaceOffset = (int32_t) ((char*) bp - (char*) p->pp->dataSpace);
    }
    return OK;
}

static int32_t push_f_opcode_perf(CSOUND *csound, PUSH_OPCODE *p)
{
    IGN(csound);
    void    *bp;
    int32_t     *ofsp;
    int32_t     offs;

    if (UNLIKELY(p->pp->freeSpaceOffset + p->argMap[2] >
                 p->pp->freeSpaceEndOffset))
      return csoundStack_OverflowError(p);
    bp = (void*) ((char*) p->pp->dataSpace + (int32_t) p->pp->freeSpaceOffset);
    p->pp->freeSpaceOffset += p->argMap[2];
    *((void**) bp) = p->pp->curBundle;
    p->pp->curBundle = bp;
    ofsp = (int32_t*) ((char*) bp +
                       (int32_t) csoundStack_Align((int32_t) sizeof(void*)));
    offs = p->argMap[3];
    *(ofsp++) = offs;
    *((PVSDAT**) ((char*) bp + (int32_t) (offs & (int32_t) 0x00FFFFFF))) =
        (PVSDAT*) ((char*) p->args[0]);
    *ofsp = CS_STACK_END;

    return OK;
}

static int32_t push_f_opcode_init(CSOUND *csound, PUSH_OPCODE *p)
{
    void    *bp;
    int32_t     *ofsp;
    int32_t     offs;

    if (!p->initDone) {
      p->pp = csoundStack_GetGlobals(csound);
      offs = (int32_t) sizeof(void*);
      offs = csoundStack_Align(offs);
      offs += ((int32_t) sizeof(int32_t) * 2);
      offs = csoundStack_Align(offs);
      /* the 'f' type is special: */
      /*   it is copied at both init and performance time */
      p->argMap[0] = 1;
      p->argMap[3] = (CS_STACK_F | offs);
      p->argMap[4] = CS_STACK_END;
      /* FIXME: store only the address of the f-signal on the stack */
      /* this may be dangerous... */
      offs += (int32_t) sizeof(PVSDAT*);
      offs = csoundStack_Align(offs);
      p->argMap[1] = offs;
      p->argMap[2] = offs;
      p->h.opadr = (int32_t (*)(CSOUND *, void *)) push_f_opcode_perf;
      p->initDone = 1;
    }
    if (UNLIKELY(p->pp->freeSpaceOffset + p->argMap[1] > p->pp->freeSpaceEndOffset))
      return csoundStack_OverflowError(p);
    bp = (void*) ((char*) p->pp->dataSpace + (int32_t) p->pp->freeSpaceOffset);
    p->pp->freeSpaceOffset += p->argMap[1];
    *((void**) bp) = p->pp->curBundle;
    p->pp->curBundle = bp;
    ofsp = (int32_t*) ((char*) bp +
                       (int32_t) csoundStack_Align((int32_t) sizeof(void*)));
    offs = p->argMap[3];
    *(ofsp++) = offs;
    *((PVSDAT**) ((char*) bp + (int32_t) (offs & (int32_t) 0x00FFFFFF))) =
        (PVSDAT*) ((char*) p->args[0]);
    *ofsp = CS_STACK_END;

    return OK;
}

static int32_t pop_f_opcode_perf(CSOUND *csound, POP_OPCODE *p)
{
    void    *bp;
    int32_t     *ofsp;
    int32_t     offs;

    if (UNLIKELY(p->pp->curBundle == NULL))
      return csoundStack_EmptyError(p);
    bp = p->pp->curBundle;
    ofsp = (int32_t*) ((char*) bp +
                       (int32_t) csoundStack_Align((int32_t) sizeof(void*)));
    offs = p->argMap[3];
    if (UNLIKELY(offs != *ofsp))
      csoundStack_TypeError(p);
    ofsp++;
    if (UNLIKELY(*ofsp != CS_STACK_END))
      csoundStack_TypeError(p);
    fsg_assign(csound,
            (PVSDAT*) p->args[0],
            *((PVSDAT**) ((char*) bp + (int32_t) (offs & (int32_t) 0x00FFFFFF))));
    p->pp->curBundle = *((void**) bp);
    p->pp->freeSpaceOffset = (int32_t) ((char*) bp - (char*) p->pp->dataSpace);

    return OK;
}

static int32_t pop_f_opcode_init(CSOUND *csound, POP_OPCODE *p)
{
    void    *bp;
    int32_t     *ofsp;
    int32_t     offs;

    if (!p->initDone) {
      p->pp = csoundStack_GetGlobals(csound);
      offs = (int32_t) sizeof(void*);
      offs = csoundStack_Align(offs);
      offs += ((int32_t) sizeof(int32_t) * 2);
      offs = csoundStack_Align(offs);
      /* the 'f' type is special: */
      /*   it is copied at both init and performance time */
      p->argMap[0] = 1;
      p->argMap[3] = (CS_STACK_F | offs);
      p->argMap[4] = CS_STACK_END;
      /* FIXME: only the address of the f-signal is stored on the stack */
      /* this may be dangerous... */
      offs += (int32_t) sizeof(PVSDAT*);
      offs = csoundStack_Align(offs);
      p->argMap[1] = offs;
      p->argMap[2] = offs;
      p->h.opadr = (int32_t (*)(CSOUND *, void *)) pop_f_opcode_perf;
      p->initDone = 1;
    }
    if (UNLIKELY(p->pp->curBundle == NULL))
      return csoundStack_EmptyError(p);
    bp = p->pp->curBundle;
    ofsp = (int32_t*) ((char*) bp +
                       (int32_t) csoundStack_Align((int32_t) sizeof(void*)));
    offs = p->argMap[3];
    if (UNLIKELY(offs != *ofsp))
      csoundStack_TypeError(p);
    ofsp++;
    if (UNLIKELY(*ofsp != CS_STACK_END))
      csoundStack_TypeError(p);
    fsg_assign(csound,
            (PVSDAT*) p->args[0],
            *((PVSDAT**) ((char*) bp + (int32_t) (offs & (int32_t) 0x00FFFFFF))));
    p->pp->curBundle = *((void**) bp);
    p->pp->freeSpaceOffset = (int32_t) ((char*) bp - (char*) p->pp->dataSpace);

    return OK;
}



 /* ------------------------------------------------------------------------ */

static OENTRY localops[] = { 
  { "stack",  sizeof(STACK_OPCODE), SK|_QQ, 1,  "",   "i",
      (SUBR) stack_opcode_init, (SUBR) NULL,                      (SUBR) NULL },
  { "push",   sizeof(PUSH_OPCODE),  SK|_QQ, 3,  "",   "N",
      (SUBR) push_opcode_init,  (SUBR) notinit_opcode_stub_perf,  (SUBR) NULL },
  { "pop",    sizeof(POP_OPCODE),   SK|_QQ, 3,
                                   "NNNNNNNNNNNNNNNNNNNNNNNNNNNNNNN", "",
      (SUBR) pop_opcode_init,   (SUBR) notinit_opcode_stub_perf,  (SUBR) NULL },
  { "push_f", sizeof(PUSH_OPCODE),  SK|_QQ, 3,  "",   "f",
      (SUBR) push_f_opcode_init, (SUBR) notinit_opcode_stub_perf, (SUBR) NULL },
  { "pop_f",  sizeof(POP_OPCODE),   SK|_QQ, 3,  "f",   "",
      (SUBR) pop_f_opcode_init,  (SUBR) notinit_opcode_stub_perf, (SUBR) NULL }
};

LINKAGE

