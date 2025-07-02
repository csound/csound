/*
    str_ops.h:

    Copyright (C) 2005, 2006 Istvan Varga
              (C) 2005       Matt J. Ingalls, John ffitch

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

#ifndef CSOUND_STR_OPS_H
#define CSOUND_STR_OPS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    OPDS    h;
    MYFLT   *indx;
    STRINGDAT  *str;
} STRSET_OP;

typedef struct {
    OPDS    h;
    MYFLT   *r;
    STRINGDAT  *str;
    char *mem;
} STRCHGD;

typedef struct {
    OPDS    h;
    MYFLT   *indx;
    MYFLT *str;
} STRTOD_OP;

typedef struct {
    OPDS    h;
    STRINGDAT   *r;
    MYFLT   *indx;
} STRGET_OP;

typedef struct {
    OPDS    h;
    STRINGDAT  *r;
    STRINGDAT   *str;
} STRCPY_OP;

typedef struct {
    OPDS    h;
    STRINGDAT   *r;
    STRINGDAT   *str1;
    STRINGDAT   *str2;
} STRCAT_OP;

typedef struct {
    OPDS    h;
    MYFLT   *r;
    STRINGDAT   *str1;
    STRINGDAT   *str2;
    MYFLT res;
} STRCMP_OP;

typedef struct {
    OPDS    h;
    STRINGDAT   *r;
    STRINGDAT   *sfmt;
    MYFLT   *args[64];
} SPRINTF_OP;

typedef struct {
    OPDS    h;
    STRINGDAT   *sfmt;
    MYFLT   *ktrig;
    MYFLT   *args[64];
    MYFLT   prv_ktrig;
} PRINTF_OP;

typedef struct {
    OPDS    h;
    STRINGDAT   *str;
    MYFLT   *ktrig;
    MYFLT   *no_newline;
    MYFLT   prv_ktrig;
    int32_t     noNewLine;
} PUTS_OP;

typedef struct {
    OPDS    h;
    STRINGDAT   *Sdst;
    STRINGDAT   *Ssrc;
    MYFLT   *istart;
    MYFLT   *iend;
} STRSUB_OP;

typedef struct {
    OPDS    h;
    MYFLT   *ichr;
    STRINGDAT   *Ssrc;
    MYFLT   *ipos;
} STRCHAR_OP;

typedef struct {
    OPDS    h;
    MYFLT   *ilen;
    STRINGDAT   *Ssrc;
} STRLEN_OP;

typedef struct {
    OPDS    h;
    STRINGDAT   *Sdst;
    STRINGDAT   *Ssrc;
} STRUPPER_OP;

typedef struct {
    OPDS    h;
    STRINGDAT   *Sdst;
    MYFLT   *iopt;
} GETCFG_OP;

typedef struct {
    OPDS    h;
    MYFLT   *ipos;
    STRINGDAT   *Ssrc1;
    STRINGDAT   *Ssrc2;
} STRINDEX_OP;

typedef struct {
    OPDS    h;
    MYFLT   *inVar;
} PRINT_TYPE_OP;

#ifndef CSOUND_STR_OPS_C

int32_t     strset_init(CSOUND *, void *);
int32_t     strget_init(CSOUND *, void *);
int32_t     strcpy_opcode_p(CSOUND *, void *);
int32_t     strcpy_opcode_S(CSOUND *, void *);
int32_t     strassign_k(CSOUND *, void *);
int32_t     strcat_opcode(CSOUND *, void *);
int32_t     strcmp_opcode(CSOUND *, void *);
int32_t     sprintf_opcode(CSOUND *, void *);
int32_t     printf_opcode_init(CSOUND *, void *);
int32_t     printf_opcode_set(CSOUND *, void *);
int32_t     printf_opcode_perf(CSOUND *, void *);
int32_t     puts_opcode_init(CSOUND *, void *);
int32_t     puts_opcode_perf(CSOUND *, void *);
int32_t     strtod_opcode_p(CSOUND *, void *);
int32_t     strtod_opcode_S(CSOUND *, void *);
int32_t     strtol_opcode_p(CSOUND *, void *);
int32_t     strtol_opcode_S(CSOUND *, void *);
int32_t     strsub_opcode(CSOUND *, void *);
int32_t     strchar_opcode(CSOUND *, void *);
int32_t     strlen_opcode(CSOUND *, void *);
int32_t     strupper_opcode(CSOUND *, void *);
int32_t     strlower_opcode(CSOUND *, void *);
int32_t     getcfg_opcode(CSOUND *, void *);
int32_t     strindex_opcode(CSOUND *, void *);
int32_t     strrindex_opcode(CSOUND *, void *);
int32_t     str_changed(CSOUND *csound, STRCHGD *p);
int32_t     str_changed_k(CSOUND *csound, STRCHGD *p);
int32_t     str_from_url(CSOUND *csound, STRCPY_OP *p);
int32_t     print_type_opcode(CSOUND*, void*);
  int32_t     s_opcode(CSOUND *csound, void *p);
  int32_t     s_opcode_k(CSOUND *csound, void *p);
#endif      /* CSOUND_STR_OPS_C */

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_STR_OPS_H */
