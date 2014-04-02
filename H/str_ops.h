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
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA
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
    int     noNewLine;
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

/*
*********These are not correct for csound 6 as they omit the flags field********
 {  "strset",   S(STRSET_OP),   1,  "",     "iS",
    (SUBR) strset_init, (SUBR) NULL, (SUBR) NULL                        },
 {  "strget",   S(STRGET_OP),   1,  "S",    "i",
    (SUBR) strget_init, (SUBR) NULL, (SUBR) NULL                        },
 {  "strcpy",   S(STRCPY_OP),   1,  "S",    "S",
    (SUBR) strcpy_opcode, (SUBR) NULL, (SUBR) NULL                      },
 {  "strcpyk",  S(STRCPY_OP),   3,  "S",    "S",
    (SUBR) strcpy_opcode, (SUBR) strcpy_opcode, (SUBR) NULL             },
 {  "strcat",   S(STRCAT_OP),   1,  "S",    "SS",
    (SUBR) strcat_opcode, (SUBR) NULL, (SUBR) NULL                      },
 {  "strcatk",  S(STRCAT_OP),   3,  "S",    "SS",
    (SUBR) strcat_opcode, (SUBR) strcat_opcode, (SUBR) NULL             },
 {  "strcmp",   S(STRCAT_OP),   1,  "i",    "SS",
    (SUBR) strcmp_opcode, (SUBR) NULL, (SUBR) NULL                      },
 {  "strcmpk",  S(STRCAT_OP),   3,  "k",    "SS",
    (SUBR) strcmp_opcode, (SUBR) strcmp_opcode, (SUBR) NULL             },
 {  "sprintf",  S(SPRINTF_OP),  1,  "S",    "SN",
    (SUBR) sprintf_opcode, (SUBR) NULL, (SUBR) NULL                     },
 {  "sprintfk", S(SPRINTF_OP),  3,  "S",    "SN",
    (SUBR) sprintf_opcode, (SUBR) sprintf_opcode, (SUBR) NULL           },
 {  "printf_i", S(PRINTF_OP),   1,  "",     "SiN",
    (SUBR) printf_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "printf",   S(PRINTF_OP),   3,  "",     "SkN",
    (SUBR) printf_opcode_set, (SUBR) printf_opcode_perf, (SUBR) NULL    },
 {  "puts",     S(PUTS_OP),     3,  "",     "Sko",
    (SUBR) puts_opcode_init, (SUBR) puts_opcode_perf, (SUBR) NULL       },
 {  "strtod",   S(STRSET_OP),   1,  "i",    "T",
    (SUBR) strtod_opcode, (SUBR) NULL, (SUBR) NULL                      },
 {  "strtodk",  S(STRSET_OP),   3,  "k",    "U",
    (SUBR) strtod_opcode, (SUBR) strtod_opcode, (SUBR) NULL             },
 {  "strtol",   S(STRSET_OP),   1,  "i",    "T",
    (SUBR) strtol_opcode, (SUBR) NULL, (SUBR) NULL                      },
 {  "strtolk",  S(STRSET_OP),   3,  "k",    "U",
    (SUBR) strtol_opcode, (SUBR) strtol_opcode, (SUBR) NULL             },
 {  "strsub",   S(STRSUB_OP),   1,  "S",    "Soj",
    (SUBR) strsub_opcode, (SUBR) NULL, (SUBR) NULL                      },
 {  "strsubk",  S(STRSUB_OP),   3,  "S",    "Skk",
    (SUBR) strsub_opcode, (SUBR) strsub_opcode, (SUBR) NULL             },
 {  "strchar",  S(STRCHAR_OP),  1,  "i",    "So",
    (SUBR) strchar_opcode, (SUBR) NULL, (SUBR) NULL                     },
 {  "strchark", S(STRCHAR_OP),  3,  "k",    "SO",
    (SUBR) strchar_opcode, (SUBR) strchar_opcode, (SUBR) NULL           },
 {  "strlen",   S(STRLEN_OP),   1,  "i",    "S",
    (SUBR) strlen_opcode, (SUBR) NULL, (SUBR) NULL                      },
 {  "strlenk",  S(STRLEN_OP),   3,  "k",    "S",
    (SUBR) strlen_opcode, (SUBR) strlen_opcode, (SUBR) NULL             },
 {  "strupper", S(STRUPPER_OP), 1,  "S",    "S",
    (SUBR) strupper_opcode, (SUBR) NULL, (SUBR) NULL                    },
 {  "strupperk", S(STRUPPER_OP), 3, "S",    "S",
    (SUBR) strupper_opcode, (SUBR) strupper_opcode, (SUBR) NULL         },
 {  "strlower", S(STRUPPER_OP), 1,  "S",    "S",
    (SUBR) strlower_opcode, (SUBR) NULL, (SUBR) NULL                    },
 {  "strlowerk", S(STRUPPER_OP), 3, "S",    "S",
    (SUBR) strlower_opcode, (SUBR) strlower_opcode, (SUBR) NULL         },
 {  "getcfg",   S(GETCFG_OP),   1,  "S",    "i",
    (SUBR) getcfg_opcode, (SUBR) NULL, (SUBR) NULL                      },
 {  "strindex", S(STRINDEX_OP), 1,  "i",    "SS",
    (SUBR) strindex_opcode, (SUBR) NULL, (SUBR) NULL                    },
 {  "strindexk", S(STRINDEX_OP), 3, "k",    "SS",
    (SUBR) strindex_opcode, (SUBR) strindex_opcode, (SUBR) NULL         },
 {  "strrindex", S(STRINDEX_OP), 1, "i",    "SS",
    (SUBR) strrindex_opcode, (SUBR) NULL, (SUBR) NULL                   },
 {  "strrindexk", S(STRINDEX_OP), 3, "k",   "SS",
    (SUBR) strrindex_opcode, (SUBR) strrindex_opcode, (SUBR) NULL       },
*/

#ifndef CSOUND_STR_OPS_C

int     strset_init(CSOUND *, void *);
int     strget_init(CSOUND *, void *);
int     strcpy_opcode_p(CSOUND *, void *);
int     strcpy_opcode_S(CSOUND *, void *);
int     strassign_opcode_S(CSOUND *, void *);
int     strassign_opcode_Sk(CSOUND *, void *);
int     strcat_opcode(CSOUND *, void *);
int     strcmp_opcode(CSOUND *, void *);
int     sprintf_opcode(CSOUND *, void *);
int     printf_opcode_init(CSOUND *, void *);
int     printf_opcode_set(CSOUND *, void *);
int     printf_opcode_perf(CSOUND *, void *);
int     puts_opcode_init(CSOUND *, void *);
int     puts_opcode_perf(CSOUND *, void *);
int     strtod_opcode_p(CSOUND *, void *);
int     strtod_opcode_S(CSOUND *, void *);
int     strtol_opcode_p(CSOUND *, void *);
int     strtol_opcode_S(CSOUND *, void *);
int     strsub_opcode(CSOUND *, void *);
int     strchar_opcode(CSOUND *, void *);
int     strlen_opcode(CSOUND *, void *);
int     strupper_opcode(CSOUND *, void *);
int     strlower_opcode(CSOUND *, void *);
int     getcfg_opcode(CSOUND *, void *);
int     strindex_opcode(CSOUND *, void *);
int     strrindex_opcode(CSOUND *, void *);
  int str_changed(CSOUND *csound, STRCHGD *p);
  int str_changed_k(CSOUND *csound, STRCHGD *p);
  int str_from_url(CSOUND *csound, STRCPY_OP *p);

#endif      /* CSOUND_STR_OPS_C */

#ifdef __cplusplus
}
#endif

#endif      /* CSOUND_STR_OPS_H */

