/*
  entry1.c:

  Copyright (C) 1991 Barry Vercoe, John ffitch

  This file is part of Csound.

  The Csound Library is free software; you can redistribute it
  and/or modify it under the terms of the GNU Lesser General Public
ndf  License as published by the Free Software Foundation; either
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

#include "entry1.h"             /*                      ENTRY1.C        */
#include "interlocks.h"
#include "opcodes.h"

/* thread vals, where isub=1, ksub=2:
   0 =     1  OR   2  (B out only)
   1 =     1
   2 =             2
   3 =     1  AND  2
*/

/* inarg types include the following:
   i       irate scalar
   k       krate scalar
   a       arate vector
   f       frequency variable
   w       spectral variable
   x       krate scalar or arate vector
   S       String
   T       String or i-rate
   U       String or i/k-rate
   B       Boolean k-rate
   b       Boolean i-rate; internally generated as required
   l       Label
   .       required arg of any-type
   and codes
   ?       optional arg of any-type
   m       begins an indef list of iargs (any count)
   M       begins an indef list of args (any count/rate i,k,a)
   N       begins an indef list of args (any count/rate i,k,a,S)
   o       optional i-rate, defaulting to  0
   p              "             "          1
   q              "             "         10
   v              "             "          .5
   j              "             "         -1
   h              "             "        127
   O       optional k-rate, defaulting to  0
   J              "             "         -1
   V              "             "          .5
   P              "             "          1
   W       begins indef list of Strings (any count)
   y       begins indef list of aargs (any count)
   z       begins indef list of kargs (any count)
   Z       begins alternating kakaka...list (any count)
*/

/* outarg types include:
 i, k, a, S as  above
 *       multiple out args of any-type
 m       multiple out aargs
 z       multiple out kargs
 I       multiple out irate (not implemented yet)
 s       deprecated (use a or k as required)
 X       multiple args (a, k, or i-rate)     IV - Sep 1 2002
 N       multiple args (a, k, i, or S-rate)
 F       multiple args (f-rate)#
*/

/* inargs and outargs may also be arrays, e.g. "a[]" is an array of
   arate vectors. Then for polymorphic opcode entries, "opcode.a" is
   for arate vectors, and "opcode.A" is for arrays of arate vectors.
*/

OENTRY opcodlst_1[] = {
  /* opcode   dspace  flags  thread  outarg  inargs  isub    ksub    asub    */
  { "",     0,      0,    0,      "",     "",     NULL, NULL, NULL, NULL },
  { "instr",  0,    0,      0,      "",     "",   NULL, NULL, NULL, NULL },
  { "endin",  0,    0,      0,      "",     "",   NULL, NULL, NULL, NULL },
  /* IV - Sep 8 2002 */
  { "opcode", 0,    0,      0,      "",     "",   NULL, NULL, NULL, NULL },
  { "endop",  0,    0,      0,      "",     "",   NULL, NULL, NULL, NULL },
  { "$label", S(LBLBLK),  0,0,      "",     "",   NULL, NULL, NULL, NULL },
  { "pset",   S(PVSET),   0,0,      "",     "m",  NULL, NULL, NULL, NULL },

  /* IV - Sep 8 2002 - added entries for user defined opcodes, xin, xout */
  /* and setksmps */
  { "##userOpcode", S(UOPCODE),0, 7, "", "", useropcdset, useropcd, useropcd, NULL },
  /* IV - Sep 10 2002: removed perf time routines of xin and xout */
  { "xin",  S(XIN_MAX),0,   1,  "****************", "",  xinset,  NULL, NULL, NULL },
  /* { "xin.64",   S(XIN_HIGH),0,  1,
    "****************************************************************", "",
    xinset,  NULL, NULL },
  { "##xin256",  S(XIN_MAX),0,   1,
    "****************************************************************"
    "****************************************************************"
    "****************************************************************"
    "****************************************************************", "",
    xinset,  NULL, NULL },*/
  { "xout", S(XOUT_MAX),0,  1,  "",         "*", xoutset, NULL, NULL, NULL },
  #ifdef INC_SETKSMPS
  { "setksmps", S(SETKSMPS),0,  1,  "",   "i", setksmpsset, NULL, NULL },
  #endif
#ifdef INC_CTRLINIT
  { "ctrlinit",S(CTLINIT),0,1,      "",  "im", ctrlinit, NULL, NULL, NULL},
  #endif
  #ifdef INC_CTRLINIT_S
  { "ctrlinit.S",S(CTLINITS),0,1,      "",  "Sm", ctrlnameinit, NULL, NULL, NULL},
  #endif
  #ifdef INC_CTRLSAVE
  { "ctrlsave",S(SAVECTRL),0,3,       "k[]","im", savectrl_init, savectrl_perf, NULL, NULL},
  #endif
  #ifdef INC_CTRLPRINT
  { "ctrlprint.S",S(PRINTCTRL),0,3, "", "k[]S", printctrl_init1, printctrl, NULL},
  { "ctrlprint",S(PRINTCTRL),0,3,       "", "k[]", printctrl_init, printctrl, NULL},
  #endif
  #ifdef CTRLPRESET
  { "ctrlpreset", S(PRESETCTRL1), 0,3, "k", "kk[]", presetctrl1_init, presetctrl1_perf, NULL},
  { "ctrlpreset", S(PRESETCTRL), 0,3, "k", "kim", presetctrl_init, presetctrl_perf, NULL},
  #endif
  #ifdef INC_CTRLSELECT
  { "ctrlselect", S(SELECTCTRL), 0,3,"",   "k", selectctrl_init, selectctrl_perf, NULL },
  #endif
  #ifdef INC_CTRLPRINTPRESETS
  { "ctrlprintpresets", S(PRINTPRESETS), 0,3, "", "", printpresets_init, printpresets_perf, NULL},
  { "ctrlprintpresets.S", S(PRINTPRESETS), 0,3, "", "S", printpresets_init1, printpresets_perf, NULL}
  #endif
  #ifdef INC_MASSIGN_P
  { "massign",S(MASSIGN), 0,1,      "",  "iip",massign_p, NULL, NULL, NULL},
  #endif
  #ifdef INC_MASSIG_S
  { "massign.S",S(MASSIGNS), 0,1,  "",  "iSp",massign_S, NULL, NULL, NULL},
  #endif
  { "turnon", S(TURNON),  0,1,      "",     "io", turnon, NULL, NULL, NULL},
  { "turnon.S", S(TURNON),  0,1,    "",     "So", turnon_S, NULL, NULL, NULL},
  { "remoteport", S(REMOTEPORT), 0,1, "",  "i", remoteport, NULL, NULL, NULL},
  { "insremot",S(INSREMOT),0,1,     "",     "SSm",insremot, NULL, NULL, NULL},
  { "midremot",S(MIDREMOT),0,1,     "",     "SSm",midremot, NULL, NULL, NULL},
  { "insglobal",S(INSGLOBAL),0,1,   "",     "Sm", insglobal, NULL, NULL, NULL},
  { "midglobal",S(MIDGLOBAL),0,1,   "",     "Sm", midglobal, NULL, NULL, NULL},
  { "ihold",  S(LINK),0,    1,      "",     "",     ihold, NULL, NULL, NULL  },
  { "turnoff",S(LINK),0,    2,      "",     "",     NULL,   turnoff, NULL, NULL },
  #ifdef INC_STRCPY
  {  "=.S",   S(STRCPY_OP),0,   1,  "S",    "S",
     (SUBR) strcpy_opcode_S, NULL, (SUBR) NULL, NULL    },
  {  "#=.S",   S(STRCPY_OP),0,   2,  "S",    "S",
     NULL, (SUBR) strcpy_opcode_S, (SUBR) NULL, NULL    },
  {  "=.T",   S(STRGET_OP),0,   1,  "S",    "i",
     (SUBR) strcpy_opcode_p, (SUBR) NULL, (SUBR) NULL, NULL                 }, 
  #endif
  #ifdef INC_RASSIGN
  { "=.r",    S(ASSIGN),0,  1,      "r",    "i",    rassign, NULL, NULL, NULL },
  #endif
  #ifdef INC_MINIT
  { "=.i",    S(ASSIGNM),0, 1,      "IIIIIIIIIIIIIIIIIIIIIIII", "m",
    minit, NULL, NULL, NULL  },
  { "=.k",    S(ASSIGNM),0, 2,      "zzzzzzzzzzzzzzzzzzzzzzzz", "z",
    NULL, minit, NULL, NULL },
  #endif
  #ifdef INC_AASSIGN
  { "=.a",    S(ASSIGN),0,  2,      "a",    "a",    NULL, gaassign, NULL },
  { "=.l",    S(ASSIGN),0,  2,      "a",    "a",    NULL,   laassign, NULL },
  #endif
#ifdef INC_UPSAMP
  { "=.up",   S(UPSAMP),0,  2,      "a",    "k",  NULL, (SUBR)upsamp, NULL },
#endif
  #ifdef INC_DOWNSAMP
  { "=.down",   S(DOWNSAMP),0,  3,  "k",    "ao",   (SUBR)downset,(SUBR)downsamp },
  #endif
  #ifdef INC_STRCPY
  { "init.S", S(STRCPY_OP),0, 1,      "S", "S", (SUBR) strcpy_opcode_S  },
  { "init.Si", S(STRCPY_OP),0, 1,      "S", "i", (SUBR) strcpy_opcode_p  },
  #endif
  #ifdef INC_MINIT
  { "init.i", S(ASSIGNM),0, 1,      "IIIIIIIIIIIIIIIIIIIIIIII", "m", minit  },
  { "init.k", S(ASSIGNM),0, 1,      "zzzzzzzzzzzzzzzzzzzzzzzz", "m", minit  },
  #endif
  #ifdef INC_MINIT_A
  { "init.a", S(ASSIGNM),0, 1,      "mmmmmmmmmmmmmmmmmmmmmmmm", "m", mainit },
  #endif
  { ">",      S(RELAT),0,   0,      "b",    "ii",   gt,     gt              },
  { ">.0",      S(RELAT),0,   0,      "B",    "kk",   gt,     gt              },
  { ">=",     S(RELAT),0,   0,      "b",    "ii",   ge,     ge              },
  { ">=.0",     S(RELAT),0,   0,      "B",    "kk",   ge,     ge              },
  { "<",      S(RELAT),0,   0,      "b",    "ii",   lt,     lt              },
  { "<.0",      S(RELAT),0,   0,      "B",    "kk",   lt,     lt              },
  { "<=",     S(RELAT),0,   0,      "b",    "ii",   le,     le              },
  { "<=.0",     S(RELAT),0,   0,      "B",    "kk",   le,     le              },
  { "==",     S(RELAT),0,   0,      "b",    "ii",   eq,     eq              },
  { "==.0",     S(RELAT),0,   0,      "B",    "kk",   eq,     eq              },
  { "!=",     S(RELAT),0,   0,      "b",    "ii",   ne,     ne              },
  { "!=.0",     S(RELAT),0,   0,      "B",    "kk",   ne,     ne              },
  #ifdef INC_BOOLNOT
  { "!",      S(LOGCL),0,   0,      "b",    "b",    b_not,    b_not         },
  { "!.0",      S(LOGCL),0, 0,      "B",    "B",    b_not,    b_not         },
  #endif
  { "&&",     S(LOGCL),0,   0,      "b",    "bb",   and,    and             },
  { "&&.0",     S(LOGCL),0,   0,      "B",    "BB",   and,    and             },
  { "||",     S(LOGCL),0,   0,      "b",    "bb",   or,     or              },
  { "||.0",     S(LOGCL),0,   0,      "B",    "BB",   or,     or              },
  #ifdef INC_CONVAL
  { ":cond.i",  S(CONVAL),0,  1,      "i",    "bii",  conval                  },
  { ":cond.k",  S(CONVAL),0,  2,      "k",    "Bkk",  NULL,   conval          },
  { ":cond.s",  S(CONVAL),0,  1,      "S",    "bSS",  conval, NULL         },
  { ":cond.S",  S(CONVAL),0,  3,      "S",    "BSS",  conval, conval       },
  #endif
  #ifdef INC_CONVAL_A
  { ":cond.a",  S(CONVAL),0,  2,      "a",    "Bxx",  NULL,   aconval },
  #endif
  { "##add.ii",  S(AOP),0,    1,      "i",    "ii",   addkk                   },
  { "##sub.ii",  S(AOP),0,    1,      "i",    "ii",   subkk                   },
  { "##mul.ii",  S(AOP),0,    1,      "i",    "ii",   mulkk                   },
  { "##div.ii",  S(AOP),0,    1,      "i",    "ii",   divkk                   },
  { "##mod.ii",  S(AOP),0,    1,      "i",    "ii",   modkk                   },
  { "##add.kk",  S(AOP),0,    2,      "k",    "kk",   NULL,   addkk           },
  { "##sub.kk",  S(AOP),0,    2,      "k",    "kk",   NULL,   subkk           },
  { "##mul.kk",  S(AOP),0,    2,      "k",    "kk",   NULL,   mulkk           },
  { "##div.kk",  S(AOP),0,    2,      "k",    "kk",   NULL,   divkk           },
  { "##mod.kk",  S(AOP),0,    2,      "k",    "kk",   NULL,   modkk           },
  { "##add.ka",  S(AOP),0,    2,      "a",    "ka",   NULL,   addka   },
  { "##sub.ka",  S(AOP),0,    2,      "a",    "ka",   NULL,   subka   },
  { "##mul.ka",  S(AOP),0,    2,      "a",    "ka",   NULL,   mulka   },
  { "##div.ka",  S(AOP),0,    2,      "a",    "ka",   NULL,   divka   },
  { "##mod.ka",  S(AOP),0,    2,      "a",    "ka",   NULL,   modka   },
  { "##add.ak",  S(AOP),0,    2,      "a",    "ak",   NULL,   addak   },
  { "##sub.ak",  S(AOP),0,    2,      "a",    "ak",   NULL,   subak   },
  { "##mul.ak",  S(AOP),0,    2,      "a",    "ak",   NULL,   mulak   },
  { "##div.ak",  S(AOP),0,    2,      "a",    "ak",   NULL,   divak   },
  { "##mod.ak",  S(AOP),0,    2,      "a",    "ak",   NULL,   modak   },
  { "##add.aa",  S(AOP),0,    2,      "a",    "aa",   NULL,   addaa   },
  { "##sub.aa",  S(AOP),0,    2,      "a",    "aa",   NULL,   subaa   },
  { "##mul.aa",  S(AOP),0,    2,      "a",    "aa",   NULL,   mulaa   },
  { "##div.aa",  S(AOP),0,    2,      "a",    "aa",   NULL,   divaa   },
  { "##mod.aa",  S(AOP),0,    2,      "a",    "aa",   NULL,   modaa   },
  { "##addin.i", S(ASSIGN),0, 1,      "i",    "i",    addin,  NULL    },
  { "##addin.k", S(ASSIGN),0, 2,      "k",    "k",    NULL,   addin   },
  { "##addin.K", S(ASSIGN),0, 2,      "a",    "k",    NULL,   addinak },
  { "##addin.a", S(ASSIGN),0, 2,      "a",    "a",    NULL,   addina  },
  { "##subin.i", S(ASSIGN),0, 1,      "i",    "i",    subin,  NULL    },
  { "##subin.k", S(ASSIGN),0, 2,      "k",    "k",    NULL,   subin   },
  { "##subin.K", S(ASSIGN),0, 2,      "a",    "k",    NULL,   subinak },
  { "##subin.a", S(ASSIGN),0, 2,      "a",    "a",    NULL,   subina  },
  #ifdef INC_DIVZ
  { "divz.ii", S(DIVZ),0,   1,      "i",    "iii",  divzkk, NULL,   NULL    },
  { "divz.kk", S(DIVZ),0,   2,      "k",    "kkk",  NULL,   divzkk, NULL    },
  #endif
  #ifdef INC_DIVZ_AK
  { "divz.ak", S(DIVZ),0,   2,      "a",    "akk",  NULL,   divzak  },
  #endif
  #ifdef INC_DIVZ_KA
  { "divz.ka", S(DIVZ),0,   2,      "a",    "kak",  NULL,   divzka  },
  #endif
  #ifdef INC_DIV_AA
  { "divz.aa", S(DIEVZ),0,   2,      "a",    "aak",  NULL,   divzaa  },
  #endif
  #ifdef INC_INT
  { "int.i",  S(EVAL),0,    1,      "i",    "i",    int1                    },
  #endif
  #ifdef INC_FRAC
  { "frac.i", S(EVAL),0,    1,      "i",    "i",    frac1                   },
  #endif
  #ifdef INC_ROUND
  { "round.i",S(EVAL),0,    1,      "i",    "i",    int1_round              },
  #endif
  #ifdef INC_FLOOR
  { "floor.i",S(EVAL),0,    1,      "i",    "i",    int1_floor              },
  #endif
  #ifdef INC_CEIL
  { "ceil.i", S(EVAL),0,    1,      "i",    "i",    int1_ceil               },
  #endif
  #ifdef INC_RNDSEED
  { "rndseed", S(EVAL),0,    1,      "",    "i",    rnd1seed                },
  #endif
  #ifdef INC_RND
  { "rnd.i",  S(EVAL),0,    1,      "i",    "i",    rnd1                    },
  #endif
  #ifdef INC_BIRND
  { "birnd.i",S(EVAL),0,    1,      "i",    "i",    birnd1                  },
  #endif
  #ifdef INC_ABS
  { "abs.i",  S(EVAL),0,    1,      "i",    "i",    abs1                    },
  #endif
  #ifdef INC_EXP
  { "exp.i",  S(EVAL),0,    1,      "i",    "i",    exp01                   },
  #endif
  #ifdef INC_LOG
  { "log.i",  S(EVAL),0,    1,      "i",    "i",    log01                   },
  #endif
  #ifdef INC_SQRT
  { "sqrt.i", S(EVAL),0,    1,      "i",    "i",    sqrt1                   },
  #endif
  #ifdef INC_SIN
  { "sin.i",  S(EVAL),0,    1,      "i",    "i",    sin1                    },
  #endif
  #ifdef INC_COS
  { "cos.i",  S(EVAL),0,    1,      "i",    "i",    cos1                    },
  #endif
  #ifdef INC_TAN
  { "tan.i",  S(EVAL),0,    1,      "i",    "i",    tan1                    },
  #endif
  #ifdef INC_QINF
  { "qinf.i", S(EVAL),0,    1,      "i",    "i",    is_inf                  },
  #endif
  #ifdef INC_QNAN
  { "qnan.i", S(EVAL),0,    1,      "i",    "i",    is_NaN                  },
  #endif
  #ifdef INC_ASIN
  { "sininv.i", S(EVAL),0,  1,      "i",    "i",    asin1                   },
  #endif
  #ifdef INC_ACOS
  { "cosinv.i", S(EVAL),0,  1,      "i",    "i",    acos1                   },
  #endif
  #ifdef INC_ATAN
  { "taninv.i", S(EVAL),0,  1,      "i",    "i",    atan1                   },
  #endif
  #ifdef INC_ATAN
  { "taninv2.i",S(AOP),0,   1,      "i",    "ii",   atan21                  },
  #endif
  #ifdef INC_LOG10
  { "log10.i",S(EVAL),0,    1,      "i",    "i",    log101                  },
  #endif
  #ifdef INC_LOG2
  { "log2.i", S(EVAL),0,    1,      "i",    "i",    log21                   },
  #endif
  #ifdef INC_SIN
  { "sinh.i", S(EVAL),0,    1,      "i",    "i",    sinh1                   },
  #endif
  #ifdef INC_COSH
  { "cosh.i", S(EVAL),0,    1,      "i",    "i",    cosh1                   },
  #endif
  #ifdef INC_TAN
  { "tanh.i", S(EVAL),0,    1,      "i",    "i",    tanh1                   },
  #endif
#ifdef INC_INT
  { "int.k",  S(EVAL),0,    2,      "k",    "k",    NULL,   int1            },
#endif
#ifdef INC_FRAC
  { "frac.k", S(EVAL),0,    2,      "k",    "k",    NULL,   frac1           },
  #endif
  #ifdef INC_ROUND
  { "round.k",S(EVAL),0,    2,      "k",    "k",    NULL,   int1_round      },
  #endif
  #ifdef INC_FLOOR
  { "floor.k",S(EVAL),0,    2,      "k",    "k",    NULL,   int1_floor      },
  #endif
  #ifdef INC_CEIL
  { "ceil.k", S(EVAL),0,    2,      "k",    "k",    NULL,   int1_ceil       },
  #endif
  #ifdef INC_RND
  { "rnd.k",  S(EVAL),0,    2,      "k",    "k",    NULL,   rnd1            },
  #endif
  #ifdef INC_BIRND
  { "birnd.k",S(EVAL),0,    2,      "k",    "k",    NULL,   birnd1          },
  #endif
  #ifdef INC_ABS
  { "abs.k",  S(EVAL),0,    2,      "k",    "k",    NULL,   abs1            },
  #endif
  #ifdef INC_EXP
  { "exp.k",  S(EVAL),0,    2,      "k",    "k",    NULL,   exp01           },
  #endif
  #ifdef INC_LOG
  { "log.k",  S(EVAL),0,    2,      "k",    "k",    NULL,   log01           },
  #endif
  #ifdef INC_SQRT
  { "sqrt.k", S(EVAL),0,    2,      "k",    "k",    NULL,   sqrt1           },
  #endif
  #ifdef INC_SIN
  { "sin.k",  S(EVAL),0,    2,      "k",    "k",    NULL,   sin1            },
  #endif
  #ifdef INC_COS
  { "cos.k",  S(EVAL),0,    2,      "k",    "k",    NULL,   cos1            },
  #endif
  #ifdef INC_TAN
  { "tan.k",  S(EVAL),0,    2,      "k",    "k",    NULL,   tan1            },
  #endif
  #ifdef INC_QINF
  { "qinf.k", S(EVAL),0,    2,      "k",    "k",    NULL,   is_inf          },
  #endif
  #ifdef INC_QNAN
  { "qnan.k", S(EVAL),0,    2,      "k",    "k",    NULL,   is_NaN          },
  #endif
  #ifdef INC_ASIN
  { "sininv.k", S(EVAL),0,  2,      "k",    "k",    NULL,   asin1           },
  #endif
  #ifdef INC_ACOS
  { "cosinv.k", S(EVAL),0,  2,      "k",    "k",    NULL,   acos1           },
  #endif
  #ifdef INC_ATAN
  { "taninv.k", S(EVAL),0,  2,      "k",    "k",    NULL,   atan1           },
  #endif
  #ifdef INC_ATAN2
  { "taninv2.k",S(AOP),0,   2,      "k",    "kk",   NULL,   atan21          },
  #endif
  #ifdef INC_SINH
  { "sinh.k", S(EVAL),0,    2,      "k",    "k",    NULL,   sinh1           },
  #endif
  #ifdef INC_COSH
  { "cosh.k", S(EVAL),0,    2,      "k",    "k",    NULL,   cosh1           },
  #endif
  #ifdef INC_TANH
  { "tanh.k", S(EVAL),0,    2,      "k",    "k",    NULL,   tanh1           },
  #endif
  #ifdef INC_LOG10
  { "log10.k",S(EVAL),0,    2,      "k",    "k",    NULL,   log101          },
  #endif
  #ifdef INC_LOG2
  { "log2.k", S(EVAL),0,    2,      "k",    "k",    NULL,   log21           },
  #endif
  #ifdef INC_INT_A
  { "int.a",  S(EVAL),0,    2,      "a",    "a",    NULL, int1a       },
  #endif
  #ifdef INC_FRAC_A
  { "frac.a", S(EVAL),0,    2,      "a",    "a",    NULL, frac1a      },
  #endif
  #ifdef INC_ROUND_A
  { "round.a",S(EVAL),0,    2,      "a",    "a",    NULL, int1a_round },
  #endif
  #ifdef INC_FLOOR_A
  { "floor.a",S(EVAL),0,    2,      "a",    "a",    NULL, int1a_floor },
  #endif
#ifdef INC_CEIL_A
  { "ceil.a", S(EVAL),0,    2,      "a",    "a",    NULL, int1a_ceil  },
  #endif
  #ifdef INC_ABS_A
  { "abs.a",  S(EVAL),0,    2,      "a",    "a",    NULL,   absa    },
  #endif
  #ifdef INC_EXP_A
  { "exp.a",  S(EVAL),0,    2,      "a",    "a",    NULL,   expa    },
  #endif
  #ifdef INC_LOG_A
  { "log.a",  S(EVAL),0,    2,      "a",    "a",    NULL,   loga    },
  #endif
  #ifdef INC_SQRT_A
  { "sqrt.a", S(EVAL),0,    2,      "a",    "a",    NULL,   sqrta   },
  #endif
  #ifdef INC_SIN_A
  { "sin.a",  S(EVAL),0,    2,      "a",    "a",    NULL,   sina    },
  #endif
  #ifdef INC_COS_A
  { "cos.a",  S(EVAL),0,    2,      "a",    "a",    NULL,   cosa    },
  #endif
  #ifdef INC_TAN_A
  { "tan.a",  S(EVAL),0,    2,      "a",    "a",    NULL,   tana    },
  #endif
  #ifdef INC_QINF_A
  { "qinf.a", S(EVAL),0,    2,      "a",    "a",    NULL,   is_infa },
  #endif
  #ifdef INC_QNAN_A
  { "qnan.a", S(EVAL),0,    2,      "a",    "a",    NULL,   is_NaNa },
  #endif
  #ifdef INC_ASIN_A
  { "sininv.a", S(EVAL),0,  2,      "a",    "a",    NULL,   asina   },
  #endif
  #ifdef INC_ACOS_A
  { "cosinv.a", S(EVAL),0,  2,      "a",    "a",    NULL,   acosa   },
  #endif
  #ifdef INC_ATAN_A
  { "taninv.a", S(EVAL),0,  2,      "a",    "a",    NULL,   atana   },
  #endif
  #ifdef INC_ATAN2_A
  { "taninv2.a",S(AOP),0,   2,      "a",    "aa",   NULL,   atan2aa },
  #endif
  #ifdef INC_SINH_A
  { "sinh.a", S(EVAL),0,    2,      "a",    "a",    NULL,   sinha   },
  #endif
  #ifdef INC_COSH_A
  { "cosh.a", S(EVAL),0,    2,      "a",    "a",    NULL,   cosha   },
  #endif
  #ifdef INC_TANH_A
  { "tanh.a", S(EVAL),0,    2,      "a",    "a",    NULL,   tanha   },
  #endif
  #ifdef INC_LOG10_A
  { "log10.a",S(EVAL),0,    2,      "a",    "a",    NULL,   log10a  },
  #endif
  #ifdef INC_LOG2_A
  { "log2.a", S(EVAL),0,    2,      "a",    "a",    NULL,   log2a   },
  #endif
  #ifdef INC_AMPDB_A
  { "ampdb.a",S(EVAL),0,    2,      "a",    "a",    NULL,   aampdb  },
  #endif
  #ifdef INC_AMPDB
  { "ampdb.i",S(EVAL),0,    1,      "i",    "i",    ampdb                   },
  { "ampdb.k",S(EVAL),0,    2,      "k",    "k",    NULL,   ampdb           },
  #endif
  #ifdef INC_AMPDBFS_A
  { "ampdbfs.a",S(EVAL),0,  2,      "a",    "a",    NULL,   aampdbfs },
  #endif
  #ifdef INC_AMPDBFS
  { "ampdbfs.i",S(EVAL),0,  1,      "i",    "i",    ampdbfs                 },
  { "ampdbfs.k",S(EVAL),0,  2,      "k",    "k",    NULL,   ampdbfs         },
  #endif
  #ifdef INC_DBAMP
  { "dbamp.i",S(EVAL),0,    1,      "i",    "i",    dbamp                   },
  { "dbamp.k",S(EVAL),0,    2,      "k",    "k",    NULL,   dbamp           },
  #endif
  #ifdef INC_DBFSAMP
  { "dbfsamp.i",S(EVAL),0,  1,      "i",    "i",    dbfsamp                 },
  { "dbfsamp.k",S(EVAL),0,  2,      "k",    "k",    NULL,   dbfsamp         },
  #endif
  #ifdef INC_RTCLOCK
  { "rtclock.i",S(EVAL),0,  1,      "i",    "",     rtclock                 },
  { "rtclock.k",S(EVAL),0,  2,      "k",    "",     NULL,   rtclock         },
  #endif
  #ifdef INC_FTLEN
  { "ftlen.i",S(EVAL),0,    1,      "i",    "i",    ftlen                   },
  #endif
  #ifdef INC_FTSR
  { "ftsr.i",S(EVAL),0,     1,      "i",    "i",    ftsr                    },
  #endif
  #ifdef INC_FTLPTIM
  { "ftlptim.i",S(EVAL),0,  1,      "i",    "i",    ftlptim                 },
  #endif
  #ifdef INC_FTCHNLS
  { "ftchnls.i",S(EVAL),0,  1,      "i",    "i",    ftchnls                 },
  #endif
  #ifdef INC_CPS
  { "ftcps.i",S(EVAL),0,    1,      "i",    "i",    ftcps                   },
  #endif
  #ifdef INC_ASSIGN
{ "i.i",   S(ASSIGN),0,   1,      "i",    "i",    assign                  },
  { "i.k",   S(ASSIGN),0,   1,      "i",    "k",    assign                  },
  { "k.i",   S(ASSIGN),0,   1,      "k",    "i",    assign                  },
  #endif
#ifdef INC_DOWNSAMP
  { "k.a",   S(DOWNSAMP),0, 3,      "k",    "ao",   (SUBR)downset,(SUBR)downsamp },
  #endif
  #ifdef INC_OCTPCH
  { "octpch.i",S(EVAL),0,   1,      "i",    "i",    octpch                  },
  #endif
  #ifdef INC_CPSPCH
  { "cpspch.i",S(EVAL),0,   1,      "i",    "i",    cpspch                  },
  #endif
  #ifdef INC_PCHOCT
  { "pchoct.i",S(EVAL),0,   1,      "i",    "i",    pchoct                  },
  #endif
#ifdef INC_OCTCPS
  { "octcps.i",S(EVAL),0,   1,      "i",    "i",    octcps                  },
  { "octcps.k",S(EVAL),0,   2,      "k",    "k",    NULL,   octcps          },
  #endif
  #ifdef INC_CPSOCT
  { "cpsoct.k",S(EVAL),0,   2,      "k",    "k",    NULL,   cpsoct          },
  #endif
#ifdef INC_OCTPCH
  { "octpch.k",S(EVAL),0,   2,      "k",    "k",    NULL,   octpch          },
  #endif
  #ifdef INC_CPSPCH
  { "cpspch.k",S(EVAL),0,   2,      "k",    "k",    NULL,   cpspch          },
  #endif
  #ifdef INC_PCHOCT
  { "pchoct.k",S(EVAL),0,   2,      "k",    "k",    NULL,   pchoct          },
  #endif
#ifdef INC_CPSOCT_A
  { "cpsoct.a",S(EVAL),0,   2,      "a",    "a",    NULL,   acpsoct },
  #endif
  #ifdef INC_CPSMIDINN
  { "csmidinn.i",S(EVAL),0,1,      "i",    "i",    cpsmidinn               },
  { "cpsmidinn.k",S(EVAL),0,2,      "k",    "k",    NULL,   cpsmidinn       },
  #endif
  #ifdef INC_OCTMIDINN
  { "octmidinn.i",S(EVAL),0,1,      "i",    "i",    octmidinn               },
  { "octmidinn.k",S(EVAL),0,2,      "k",    "k",    NULL,   octmidinn       },
  #endif
  #ifdef INC_PCHMIDINN
  { "pchmidinn.i",S(EVAL),0,1,      "i",    "i",    pchmidinn               },
  { "pchmidinn.k",S(EVAL),0,2,      "k",    "k",    NULL,   pchmidinn       },
  #endif
  #ifdef INC_NOTNUM
  { "notnum", S(MIDIKMB),0, 1,      "i",    "",     notnum                  },
  #endif
  #ifdef INC_VELOC
  { "veloc",  S(MIDIMAP),0, 1,      "i",    "oh",   veloc                   },
  #endif
  #ifdef INC_PCHMIDI
  { "pchmidi",S(MIDIKMB),0, 1,      "i",    "",     pchmidi                 },
  #endif
  #ifdef INC_OCTMIDI
  { "octmidi",S(MIDIKMB),0, 1,      "i",    "",     octmidi                 },
  #endif
  #ifdef INC_CPSMIDI
  { "cpsmidi",S(MIDIKMB),0, 1,      "i",    "",     cpsmidi                 },
  #endif
  #ifdef INC_PCHMIDIB_I
  { "pchmidib.i",S(MIDIKMB),0,1,    "i",    "o",    pchmidib_i              },
  #endif
  #ifdef INC_OCTMIDIB_I
  { "octmidib.i",S(MIDIKMB),0,1,    "i",    "o",    octmidib_i              },
  #endif
  #ifdef CPSMMIDIB_I
  { "cpsmidib.i",S(MIDIKMB),0,1,    "i",    "o",    icpsmidib_i             },
  #endif
  #ifdef INC_PCHMIDIB_K
  { "pchmidib.k",S(MIDIKMB),0,3,    "k",    "o",    midibset, pchmidib      },
  #endif
#ifdef INC_OCTMIDIB
  { "octmidib.k",S(MIDIKMB),0,3,    "k",    "o",    midibset, octmidib      },
  #endif
  #ifdef INC_CPSMIDIB_K
  { "cpsmidib.k",S(MIDIKMB),0,3,    "k",    "o",    midibset, icpsmidib     },
  #endif
  #ifdef INC_AMPMIDI
  { "ampmidi",S(MIDIAMP),0, 1,      "i",    "io",   ampmidi                 },
  #endif
  #ifdef INC_AFTOUCH
  { "aftouch",S(MIDIKMAP),0, 3,     "k",    "oh",   aftset, aftouch         },
  #endif
  #ifdef INC_PCHBEND_I
  { "pchbend.i",S(MIDIMAP),0,1,     "i",    "jp",   ipchbend                },
  #endif
  #ifdef INC_PCHBEND
  { "pchbend.k",S(MIDIKMAP),0,3,    "k",    "jp",   kbndset,kpchbend        },
  #endif
  #ifdef INC_MIDICTRL_I
  { "midictrl.i",S(MIDICTL),0,1,    "i",    "ioh",  imidictl                },
  #endif
  #ifdef INC_MIDICTRL_K
  { "midictrl.k",S(MIDICTL),0,3,    "k",    "ioh",  mctlset, midictl        },
  #endif
  #ifdef INC_POLYAFT_I
  { "polyaft.i",S(MIDICTL),0,1,     "i",    "ioh",  imidiaft                },
  #endif
  #ifdef INC_POLYAFT_K
  { "polyaft.k",S(MIDICTL),0,3,     "k",    "ioh",  maftset, midiaft        },
  #endif
#ifdef INC_CHANCTRL_I
  { "chanctrl.i",S(CHANCTL),0,1,    "i",    "iioh", ichanctl                },
#endif
#ifdef INC_CHANCTRL
  { "chanctrl.k",S(CHANCTL),0,3,    "k",    "iioh", chctlset,chanctl        },
#endif
  #ifdef INC_LINE
  { "line",   S(LINE),0,    3,      "k",    "iii",  linset, kline,  NULL  },
  #endif
  #ifdef INC_LINEA
  { "line.a",   S(LINE),0,    3,    "a",    "iii",  linset,  aline   },
  #endif
  #ifdef INC_EXPON
  { "expon",  S(EXPON),0,   3,      "k",    "iii",  expset, kexpon, NULL   },
  #endif
  #ifdef INC_EXPONA
  { "expon.a",  S(EXPON),0,   3,      "a",    "iii",  expset, expon   },
  #endif
  #ifdef INC_COSSEG
  { "cosseg", S(COSSEG),0,  3,      "k",    "iim",  csgset, kosseg, NULL  },
  #endif
  #ifdef INC_COSSEG_A
  { "cosseg.a", S(COSSEG),0,  3,      "a",    "iim",  csgset, cosseg  },
  #endif
  #ifdef INC_COSSEGB
  { "cossegb", S(COSSEG),0, 3,      "k",    "iim",  csgset_bkpt, kosseg, NULL  },
  #endif
  #ifdef INC_COSSEGB_A
  { "cossegb.a", S(COSSEG),0, 3,      "a",    "iim",  csgset_bkpt, cosseg  },
  #endif
  #ifdef INC_COSSEGR
  { "cossegr", S(COSSEG),0,  3,     "k",    "iim",  csgrset, kcssegr, NULL  },
  #endif
  #ifdef INC_COSSEGR_A
  { "cossegr.a", S(COSESEG),0,  3,     "a",    "iim",  csgrset, cossegr  },
  #endif 
  #ifdef INC_LINSEG
  { "linseg", S(LINSEG),0,  3,      "k",    "iim",  lsgset, klnseg, NULL },
  #endif
  #ifdef INC_LINSEG_A
  { "linseg.a", S(LINSEG),0,  3,      "a",    "iim",  lsgset, linseg  },
  #endif
  #ifdef INC_LINSEGB
  { "linsegb", S(LINSEG),0,  3,     "k",    "iim", lsgset_bkpt, klnseg, NULL  },
  #endif
  #ifdef INC_LINSEGB_A
  { "linsegb.a", S(LINSEG),0,  3,     "a",    "iim", lsgset_bkpt, linseg  },
  #endif
  #ifdef INC_LINSEGR
  { "linsegr",S(LINSEG),0,  3,      "k",    "iim",  lsgrset,klnsegr,NULL },
  #endif
  #ifdef INC_LINSEGR_A
  { "linsegr.a",S(LINSEG),0,  3,      "a",    "iim",  lsgrset,linsegr },
  #endif
  #ifdef INC_EXPSEG
  { "expseg", S(EXXPSEG),0,  3,     "k",    "iim",  xsgset, kxpseg, NULL  },
  #endif
#ifdef INC_EXPSEG_A
  { "expseg.a", S(EXXPSEG),0,  3,     "a",    "iim",  xsgset, expseg  },
#endif
  #ifdef INC_EXPSEGB
  { "expsegb", S(EXXPSEG),0,  3,     "k",    "iim",  xsgset_bkpt, kxpseg, NULL },
  #endif
  #ifdef INC_EXPSEGB_A
  { "expsegb.a", S(EXXPSEG),0, 3,     "a",    "iim",  xsgset_bkpt, expseg },
  #endif
  #ifdef INC_EXPSEGA
  { "expsega",S(EXPSEG2),0,  3,     "a",    "iim",  xsgset2, expseg2  },
  #endif
  #ifdef INC_EXPSEGBA
  { "expsegba",S(EXPSEG2),0,  3,     "a",    "iim",  xsgset2b, expseg2 },
  #endif
  #ifdef INC_EXPSEGR
  { "expsegr",S(EXPSEG),0,  3,      "k",    "iim",  xsgrset,kxpsegr,NULL },
  #endif
  #ifdef INC_EXPSEGR_A
  { "expsegr.a",S(EXPSEG),0,  3,      "a",    "iim",  xsgrset,expsegr },
  #endif
  #ifdef INC_LINEN
  { "linen",  S(LINEN),0,   3,      "k",    "kiii", lnnset, klinen, NULL   },
  #endif
  #ifdef INC_LINEN_X
  { "linen.a",  S(LINEN),0,   3,      "a",    "aiii", alnnset, linen   },
  { "linen.x",  S(LINEN),0,   3,      "a",    "kiii", alnnset, linen   },
  #endif
  #ifdef INC_LINENR
  { "linenr", S(LINENR),0,  3,      "k",    "kiii", lnrset, klinenr,NULL },
  #endif
  #ifdef INC_LINENR_X
  { "linenr.a", S(LINENR),0,  3,      "a",    "aiii", alnrset,linenr  },
  { "linenr.x", S(LINENR),0,  3,      "a",    "kiii", alnrset,linenr  },
  #endif
  #ifdef INC_ENVLPX
  { "envlpx", S(ENVLPX), TR, 3,     "k","kiiiiiio", evxset, knvlpx, NULL },
  #endif
  #ifdef INC_ENVLPXR
  { "envlpxr", S(ENVLPR),TR, 3,     "k","kiiiiioo", evrset, knvlpxr, NULL },
  #endif
  #ifdef INC_ENVLPX_X
  { "envlpx.a", S(ENVLPX), TR, 3,     "a","aiiiiiio", aevxset,envlpx  },
  { "envlpx.x", S(ENVLPX), TR, 3,     "a","kiiiiiio", aevxset,envlpx  },
  #endif
  #ifdef INC_ENVLPXR_X
  { "envlpxr.a", S(ENVLPR),TR, 3,     "a","aiiiiioo", aevrset,envlpxr },
  { "envlpxr.x", S(ENVLPR),TR, 3,     "a","kiiiiioo", aevrset,envlpxr },
  #endif
  #ifdef INC_PHASOR
  { "phasor", S(PHSOR),0,   3,       "a",   "xo",   phsset, phsor   },
  #endif
  #ifdef INC_PHASOR_K
  { "phasor.k", S(PHSOR),0,   3,     "k",   "ko",   phsset, kphsor, NULL  },
  #endif
  #ifdef INC_EPHASOR
  { "ephasor", S(EPHSOR), 0,  3,     "aa",  "xko",  ephsset, ephsor },
  #endif
#ifdef INC_SIGNUM
  { "signum.i", S(ASSIGN), 0, 1,     "i",   "i", signum, NULL, NULL       },
  { "signum.k", S(ASSIGN), 0, 3,     "k",   "k", signum, signum, NULL     },
  #endif
  #ifdef IC_SIGNU_A
  { "signum.a", S(ASSIGN), 0, 2,     "a",   "a", NULL, asignum      },
#endif
  #ifdef INC_TABLE_I
  { "table.i",  S(TABL),TR, 1,      "i",    "iiooo",(SUBR)tabler_init       },
  { "ptable.i",  S(TABLE),TR|_QQ, 1,"i",    "iiooo",(SUBR)tabler_init       },
  #endif
  #ifdef INC_TABLE_K
  { "table.k",  S(TABL),TR, 3,      "k",    "xiooo",(SUBR)tabl_setup,
    (SUBR)tabler_kontrol        },
  { "ptable.k",  S(TABLE),TR|_QQ, 3,     "k",    "xiooo",(SUBR)tabl_setup,
    (SUBR)tabler_kontrol                                                    },
  #endif
  #ifdef INC_TABLE_A
  { "table.a",  S(TABL),TR, 3,      "a",    "xiooo",(SUBR)tabl_setup,
    (SUBR)tabler_audio                                                      },
  { "ptable.a",  S(TABLE),TR|_QQ, 3,     "a",    "xiooo",(SUBR)tabl_setup,
    (SUBR)tabler_audio                                                      },
  #endif
  #ifdef INC_TABLEI_I
  { "tablei.i", S(TABL),TR, 1,      "i",    "iiooo",(SUBR)tableir_init      },
  { "ptablei.i", S(TABLE),TR|_QQ, 1,"i",    "iiooo",(SUBR)tableir_init      },
#endif
  #ifdef INC_TABLEI_K
  { "tablei.k", S(TABL),TR, 3,      "k",    "xiooo",(SUBR)tabl_setup,
    (SUBR)tableir_kontrol                                                   },
  { "ptablei.k", S(TABLE),TR|_QQ, 3,     "k",    "xiooo",(SUBR)tabl_setup,
    (SUBR)tableir_kontrol                                                   },
  #endif
  #ifdef INC_TALBEI_A
  { "tablei.a", S(TABL),TR, 3,      "a",    "xiooo",(SUBR)tabl_setup,
    (SUBR)tableir_audio                                                     },
  { "ptablei.a", S(TABLE),TR|_QQ, 3,     "a",    "xiooo",(SUBR)tabl_setup,
    (SUBR)tableir_audio                                                     },
  #endif
  #ifdef INC_TABLE3_I
  { "table3.i", S(TABL),TR, 1,      "i",    "iiooo",(SUBR)table3r_init      },
  { "ptable3.i", S(TABLE),TR|_QQ, 1,"i",    "iiooo",(SUBR)table3r_init      },
  #endif
  #ifdef INC_TABLE3_K
  { "table3.k", S(TABL),TR, 3,      "k",    "xiooo",(SUBR)tabl_setup,
    (SUBR)table3r_kontrol                                                   },
  { "ptable3.k", S(TABLE),TR|_QQ, 3,     "k",    "xiooo",(SUBR)tabl_setup,
    (SUBR)table3r_kontrol                                                   },
  #endif
  #ifdef INC_TABLE3_A
  { "table3.a", S(TABL),TR, 3,      "a",    "xiooo",(SUBR)tabl_setup,
    (SUBR)table3r_audio                                                     },
  { "ptable3.a", S(TABLE),TR|_QQ, 3,     "a",    "xiooo",(SUBR)tabl_setup,
    (SUBR)table3r_audio         },
  #endif
  #ifdef INC_OSCIL1
  { "oscil1", S(OSCIL1), TR, 3,     "k",    "ikij", ko1set, kosc1          },
  #endif
  #ifdef INC_OSCIL1I
  { "oscil1i",S(OSCIL1), TR, 3,     "k",    "ikij", ko1set, kosc1i         },
  #endif
  #ifdef INC_OSCILN
  { "osciln", S(OSCILN), TR, 3,     "a",    "kiii", oscnset,   osciln },
  { "oscilx",   S(OSCILN), TR, 3,   "a",    "kiii", oscnset,   osciln  },
  #endif
  #ifdef INC_OSCILa
  { "oscil.a",S(OSC),TR,    3,       "a",    "kkjo", oscset,   osckk  },
  #endif
  #ifdef INC_OSCILkkk
  { "oscil.kkk",S(OSC),TR,   3,      "k",    "kkjo", oscset, koscil  },
  #endif
  #ifdef INC_OSCILkka
  { "oscil.kka",S(OSC),TR,   3,      "a",    "kkjo", oscset, osckk  },
  #endif
  #ifdef INC_OSCILka
  { "oscil.ka",S(OSC),TR,    3,      "a",    "kajo", oscset,   oscka  },
  #endif
  #ifdef INC_OSCILak
  { "oscil.ak",S(OSC),TR,    3,      "a",    "akjo", oscset,   oscak  },
  #endif
  #ifdef INC_OSCILaa
  { "oscil.aa",S(OSC),TR,    3,      "a",    "aajo", oscset,   oscaa  },
  #endif
  #ifdef INC_OSCILkkA
  { "oscil.kkA",S(OSC),0,   3,      "k",    "kki[]o", oscsetA, koscil       },
  #endif
#ifdef INC_OSCILkkA
  { "oscil.kkA",S(OSC),0,   3,      "a",    "kki[]o", oscsetA, osckk },
  #endif
#ifdef INC_OSCILlkaA
  { "oscil.kaA",S(OSC),0,   3,      "a",    "kai[]o", oscsetA, oscka },
  #endif
#ifdef INC_OSCILakA
  { "oscil.akA",S(OSC),0,   3,      "a",    "aki[]o", oscsetA, oscak },
#endif
#ifdef INC_OSCILaaA
  { "oscil.aaA",S(OSC),0,   3,      "a",    "aai[]o", oscsetA,oscaa  },
#endif
  /* Change these to
     { "oscil.kkk", S(POSC),TR, 3, "k", "kkjo", posc_set, kposc         },
     { "oscil.kka", S(POSC),TR, 3,, "a", "kkjo"_set, NULL, posckk },
     { "oscil.ka", S(POSC),TR, 3, "a", "kajo", posc_set,  poscka },
     { "oscil.ak", S(POSC),TR, 3, "a", "akjo", posc_set,  poscak },
     { "oscil.aa", S(POSC),TR, 3, "a", "aajo", posc_set,  poscaa },
     { "oscil3.kk",  S(POSC),TR,  7, "s", "kkjo", posc_set, kposc3, posc3 },
  */
#ifdef INC_OSCILIkki
  { "oscili.a",S(OSC),TR,   3,      "a",    "kkjo", oscset, osckki  },
#endif
#ifdef INC_OSCILIkk
  { "oscili.kk",S(OSC),TR,   3,      "k",   "kkjo", oscset, koscli, NULL  },
#endif
#ifdef INC_OSCILIka  
  { "oscili.ka",S(OSC),TR,   3,      "a",   "kajo", oscset,   osckai  },
#endif
#ifdef INC_OSCILIak
  { "oscili.ak",S(OSC),TR,   3,      "a",   "akjo", oscset,   oscaki  },
#endif
#ifdef INC_OSCILIaa
  { "oscili.aa",S(OSC),TR,   3,      "a",   "aajo", oscset,   oscaai  },
#endif
#ifdef INC_OSCILIkkAbbbbb
  { "oscili.aA",S(OSC),0,   3,      "a",   "kki[]o", oscsetA, osckki  },
#endif
#ifdef INC_OSCILIkkA
  { "oscili.kkA",S(OSC),0,   3,      "k",  "kki[]o", oscsetA, koscli, NULL  },
#endif
#ifdef INC_OSCILIkaA
  { "oscili.kaA",S(OSC),0,   3,      "a",  "kai[]o", oscsetA,   osckai  },
#endif
#ifdef INC_OSCILIakA
  { "oscili.akA",S(OSC),0,   3,      "a",  "aki[]o", oscsetA,   oscaki  },
#endif
#ifdef INC_OSCILIaaA
  { "oscili.aaA",S(OSC),0,   3,      "a",  "aai[]o", oscsetA,   oscaai  },
#endif
#ifdef INC_OSCIL3_a
  { "oscil3.a",S(OSC),TR,   3,      "a",    "kkjo", oscset, osckk3  },
#endif
#ifdef INC_OSCIL3kk
  { "oscil3.kk",S(OSC),TR,   3,      "k",   "kkjo", oscset, koscl3, NULL  },
#endif
#ifdef INC_OSCIL3ka
  { "oscil3.ka",S(OSC),TR,   3,      "a",   "kajo", oscset,   oscka3  },
#endif
#ifdef INC_OSCIL3ak
  { "oscil3.ak",S(OSC),TR,   3,      "a",   "akjo", oscset,   oscak3  },
#endif
#ifdef INC_OSCIL3aa
  { "oscil3.aa",S(OSC),TR,   3,      "a",   "aajo", oscset,   oscaa3  },
#endif
#ifdef INC_OSCIL3kkA
  { "oscil3.aA",S(OSC),0,   3,      "a",   "kki[]o", oscsetA, osckk3 },
#endif
#ifdef INC_OSCIL3kkA
  { "oscil3.kkA",S(OSC),0,   3,      "k",  "kki[]o", oscsetA, koscl3, NULL },
#endif
#ifdef INC_OSCIL3kaA
  { "oscil3.kaA",S(OSC),0,   3,      "a",  "kai[]o", oscsetA, oscka3 },
#endif
#ifdef INC_OSCIL3akA
  { "oscil3.akA",S(OSC),0,   3,      "a",  "aki[]o", oscsetA, oscak3 },
#endif
#ifdef INC_OSCIL3
  { "oscil3.aaA",S(OSC),0,   3,      "a",  "aai[]o", oscsetA, oscaa3 },
#endif
  /* end change */
#ifdef INC_FOSCIL
  { "foscil", S(FOSC),TR,  3,      "a",  "xkxxkjo",foscset,   foscil  },
#endif
#ifdef INC_FOSCILI
  { "foscili",S(FOSC),TR,  3,      "a",  "xkxxkjo",foscset,   foscili },
#endif
#ifdef INC_LOSCIL
  { "loscil", S(LOSC),TR,  3,      "mm","xkjojoojoo",losset, loscil   },
#endif
#ifdef INC_LOSCILPHS 
  { "loscilphs", S(LOSCPHS),TR,  3, "amm","xkjojoojoo",losset_phs, loscil_phs   },
#endif
#ifdef INC_LOSCIL3PHS
  { "loscil3phs", S(LOSCPHS),TR,  3,"amm","xkjojoojoo",losset_phs, loscil3_phs  },
#endif
  #ifdef INC_LOSCIL3
  { "loscil3", S(LOSC),TR,  3,      "mm","xkjojoojoo",losset, loscil3  },
#endif
#ifdef INC_ADSYN
  { "adsyn",  S(ADSYN),0,   3,      "a",    "kkkSo", adset_S,   adsyn   },
  { "adsyn.i",  S(ADSYN),0,   3,      "a",    "kkkio", adset,   adsyn   },
#endif
#ifdef INC_BUZZ
  { "buzz",   S(BUZZ),TR,  3,      "a",  "xxkio",  bzzset,   buzz    },
#endif
#ifdef INC_GBUZZ
  { "gbuzz",  S(GBUZZ),TR,  3,      "a",  "xxkkkio",gbzset,   gbuzz   },
#endif
#ifdef INC_PLUCK
  { "pluck",  S(PLUCK), TR, 3,      "a",  "kkiiioo",plukset,   pluck   },
#endif
#ifdef INC_RAND
  { "rand",   S(RAND),0,    3,      "a",    "xvoo", rndset,  arand   },
#endif
#ifdef INC_RAND_K
  { "rand.k",   S(RAND),0,  3,      "k",    "xvoo", rndset, krand,  NULL  },
#endif
#ifdef INC_RANDH
  { "randh",  S(RANDH),0,   3,      "a",    "xxvoo", rhset, randh   },
#endif
  #ifdef INC_RANDH_K
  { "randh.k",  S(RANDH),0, 3,      "k",    "xxvoo", rhset, krandh, NULL   },
#endif
#ifdef INC_RANDI
  { "randi",  S(RANDI),0,   3,      "a",    "xxvoo", riset, randi     },
#endif
#ifdef INC_RANDI_K
  { "randi.k",  S(RANDI),0, 3,      "k",    "xxvoo", riset, krandi    },
#endif
#ifdef INC_RANDC
  { "randc",  S(RANDC),0,   3,      "a",    "xxvoo", rcset, randc     },
#endif
#ifdef INC_RANDC_K
  { "randc.k",  S(RANDC),0, 3,      "k",    "xxvoo", rcset, krandc    },
#endif
#ifdef  INC_PORT
  { "port",   S(PORT),0,    3,      "k",    "kio",  porset, port            },
#endif
#ifdef  INC_TONE
  { "tone.k", S(TONE),0,    3,      "a",    "ako",  tonset,   tone    },
#endif
#ifdef  INC_TONEX
  { "tonex.k",S(TONEX),0,   3,      "a",    "akoo", tonsetx,  tonex   },
#endif
#ifdef  INC_ATONE
  { "atone.k",  S(TONE),0,  3,      "a",    "ako",  tonset,   atone   },
#endif
#ifdef  INC_ATONEX
  { "atonex.k", S(TONEX),0, 3,      "a",    "akoo", tonsetx,  atonex  },
#endif
  #ifdef  INC_RESON
  { "reson", S(RESON),   0, 3,      "a",    "axxoo", rsnset,  reson   },
#endif
#ifdef  INC_RESONX
  { "resonx", S(RESONX),0,  3,      "a",    "axxooo", rsnsetx, resonx },
#endif
#ifdef  INC_ARESON
  { "areson.kk", S(RESON),0,3,      "a",    "akkoo",rsnset,   areson  },
#endif
#ifdef  INC_LPREAD
  { "lpread", S(LPREAD),0,  3,      "kkkk", "kSoo", lprdset_S,lpread          },
  { "lpread.i", S(LPREAD),0,  3,      "kkkk", "kioo", lprdset,lpread          },
#endif
#ifdef  INC_LPFORM
  { "lpform", S(LPFORM),0,  3,      "kk", "k",     lpformantset,lpformant   },
#endif
#ifdef  INC_LPRESON
  { "lpreson",S(LPRESON),0, 3,      "a",    "a",    lprsnset,  lpreson },
#endif
#ifdef  INC_LPFRESON
  { "lpfreson",S(LPFRESON),0,3,     "a",    "ak",   lpfrsnset, lpfreson},
#endif
#ifdef  INC_LPSLOT
  { "lpslot"  ,  S(LPSLOT),0,  1,   "",     "i",    lpslotset, NULL, NULL   },
#endif
#ifdef INC_LPINTERP
  { "lpinterp", S(LPINTERPOL),0, 3, "",     "iik",  lpitpset, lpinterpol, NULL},
#endif
#ifdef  INC_RMS
  { "rms",    S(RMS),0,     3,      "k",    "aqo",  rmsset, rms             },
#endif
#ifdef  INC_GAIN
  { "gain",   S(GAIN),0,    3,      "a",    "akqo", gainset,   gain    },
#endif
#ifdef  INC_BALANCE
  { "balance",S(BALANCE),0, 3,      "a",    "aaqo", balnset,   balance },
#endif
#ifdef  INC_BALANCE2
  { "balance2",S(BALANCE),0, 3,      "a",    "aaqo", balnset,   balance2 },
#endif
  #ifdef INC_PAN
  { "pan",    S(PAN),0,   3, "aaaa", "akkioo",(SUBR)panset, (SUBR)pan  },
  #endif
  #ifdef INC_DISKIN
  { "soundin",S(DISKIN2),0,3,"mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm","Soooo",
    sndinset_S, soundin   },
  { "soundin.i",S(DISKIN2),0,3,"mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm","ioooo",
    sndinset, soundin   },
  #endif
  #ifdef INC_SOUNDOUT
  { "soundout",S(SNDOUT), _QQ, 3,   "",    "aSo",  sndoutset_S, soundout  },
  { "soundout.i",S(SNDOUT), _QQ, 3,   "",    "aio",  sndoutset, soundout  },
  { "soundouts",S(SNDOUTS),_QQ, 3,  "",    "aaSo", sndoutset_S, soundouts },
  { "soundouts.i",S(SNDOUTS),_QQ, 3,  "",    "aaio", sndoutset, soundouts },
  #endif
  #ifdef INC_IN
  { "in.a",   S(INM),0,     2,      "a",    "",     NULL,   in      },
  #endif
  #ifdef ICN_INS
  { "in.s",   S(INS),0,     2,      "aa",    "",     NULL,   ins      },
  { "ins",    S(INS),0,     2,      "aa",   "",     NULL,   ins     },
  #endif
  #ifdef INC_INARRAY
  { "in.A",   S(INA),0,     2,      "a[]",  "",     NULL,   inarray },
  #endif
  #ifdef INC_INQ
  { "inq",    S(INQ),0,     2,      "aaaa", "",     NULL,   inq     },
  #endif
  #ifdef INC_OUT_ARRAY
  { "out.A",  S(OUTARRAY),IR, 3,      "",     "a[]",  outarr_init,  outarr },
  #endif
#ifdef INC_OUT
  { "out.a",  S(OUTX),IR,     3,      "",     "y",    ochn,   outall },
  { "outs",   S(OUTX),IR,     3,      "",     "y",    ochn,   outall },
  { "outq",   S(OUTX),IR,     3,      "",     "y",    ochn,   outall },
  { "outh",   S(OUTX),IR,     3,      "",     "y",    ochn,   outall },
  { "outo",   S(OUTX),IR,     3,      "",     "y",    ochn,   outall },
  { "outx",   S(OUTX),IR,     3,      "",     "y",    ochn,   outall },
  { "out32",  S(OUTX),IR,     3,      "",     "y",    ochn,   outall },
#endif
  #ifdef INC_OUTS1
  { "outs1",  S(OUTM),IR,    2,      "",     "a",    NULL,   outs1   },
  #endif
  #ifdef INC_OUTS2
  { "outs2",  S(OUTM),IR,    3,      "",     "a",    och2,   outs2   },
  #endif
  #ifdef INC_OUTS1
  { "outq1",  S(OUTM),IR,    2,      "",     "a",    NULL,   outs1   },
  #endif
  #ifdef INC_OUTS2
  { "outq2",  S(OUTM),IR,    3,      "",     "a",    och2,   outs2   },
  #endif
  #ifdef INC_OUT3
  { "outq3",  S(OUTM),IR,    3,      "",     "a",    och3,   outq3   },
  #endif
  #ifdef INC_OUT4
  { "outq4",  S(OUTM),IR,    3,      "",     "a",    och2,   outq4   },
  #endif
  #ifdef INC_OUTALL
  { "outall", S(OUTM),IR,    2,      "",     "a",    NULL,   outrep  },
  #endif
  { "igoto",  S(GOTO),0,    1,      "",     "l",    igoto                  },
  { "kgoto",  S(GOTO),0,    2,      "",     "l",    NULL,   kgoto           },
  { "goto",   S(GOTO),0,    3,      "",     "l",    igoto,  kgoto           },
  { "cigoto", S(CGOTO),0,   1,      "",     "Bl",   icgoto                  },
  { "ckgoto", S(CGOTO),0,   2,      "",     "Bl",   NULL,   kcgoto          },
  { "cggoto.0", S(CGOTO),0, 3,      "",     "Bl",   icgoto, kcgoto          },
  { "timout", S(TIMOUT),0,  3,      "",     "iil",  timset, timout          },
  { "reinit", S(GOTO),0,    2,      "",     "l",    NULL,   reinit          },
  { "rigoto", S(GOTO),0,    1,      "",     "l",    rigoto                  },
  { "rireturn",S(LINK),0,   1,      "",     "",     rireturn                },
  { "tigoto", S(GOTO),0,    1,      "",     "l",    tigoto                  },
  { "tival",  S(EVAL),0,    1,      "i",    "",     tival                   },
  #ifdef INC_PRINT
  { "print",  S(PRINTV),WR, 1,      "",     "m",    printv                  },
  #endif
  #ifdef INC_DISPLAY
  { "display.k",S(DSPLAY),0,  3,    "",     "kioo", dspset, kdsplay,NULL    },
  #endif
  #ifdef INC_DISPLAY_A
  { "display.a",S(DSPLAY),0,  3,    "",     "aioo", dspset ,dsplay    },
  #endif
  #ifdef INC_PVSDISPb
  { "pvsdisp",S(FSIGDISP),0,  3,    "",     "foo", fdspset, fdsplay,NULL    },
  #endif
  #ifdef INC_DISPFFT
  { "dispfft.k",S(DSPFFT),0,  3,    "",     "kiiooooo",fftset,kdspfft,NULL  },
  #endif
  #ifdef INC_DISPFFT_A
  { "dispfft.a",S(DSPFFT),0,  3,    "",     "aiiooooo",fftset,dspfft   },
  #endif
  #ifdef INC_DUMPK
  { "dumpk",  S(KDUMP),0,   3,      "",     "kSii", kdmpset_S,kdump          },
  { "dumpk.i",  S(KDUMP),0,   3,      "",     "kiii", kdmpset_p,kdump        },
  #endif
  #ifdef INC_DUMPK2
  { "dumpk2", S(KDUMP2),0,  3,      "",     "kkSii",kdmp2set_S,kdump2        },
  { "dumpk2.i", S(KDUMP2),0,  3,      "",     "kkiii",kdmp2set_p,kdump2      },
  #endif
  #ifdef INC_DUMPK3
  { "dumpk3", S(KDUMP3),0,  3,      "",     "kkkSii",kdmp3set_S,kdump3       },
  { "dumpk3.i", S(KDUMP3),0,  3,      "",     "kkkiii",kdmp3set_p,kdump3     },
  #endif
  #ifdef INC_DUMPK4
  { "dumpk4", S(KDUMP4),0,  3,      "",     "kkkkSii",kdmp4set_S,kdump4      },
  { "dumpk4.i", S(KDUMP4),0,  3,      "",     "kkkkiii",kdmp4set_p,kdump4    },
  #endif
  #ifdef INC_READK
  { "readk",  S(KREAD),0,   3,      "k",    "Sii",   krdset_S, kread         },
  { "readk.i",  S(KREAD),0,   3,      "k",    "iii",   krdset_p, kread       },
  #endif
  #ifdef INC_READK2
  { "readk2", S(KREAD2),0,  3,      "kk",   "Sii",   krd2set_S, kread2       },
  { "readk2.i", S(KREAD2),0,  3,      "kk",   "iii",   krd2set_p, kread2     },
  #endif
  #ifdef INC_READK3
  { "readk3", S(KREAD3),0,  3,      "kkk",  "Sii",   krd3set_S, kread3       },
  { "readk3.i", S(KREAD3),0,  3,      "kkk",  "iii",   krd3set_p, kread3     },
  #endif
  #ifdef INC_READK4
  { "readk4", S(KREAD4),0,  3,      "kkkk", "Sii",   krd4set_S, kread4       },
  { "readk4.i", S(KREAD4),0,  3,      "kkkk", "iii",   krd4set_p, kread4     },
  #endif
  #ifdef INC_READKS
  { "readks", S(KREADS),0,  3,      "S",    "Si",    krdsset_S, kreads       },
  { "readks.i", S(KREADS),0,  3,      "S",    "ii",    krdsset_p, kreads     },
  #endif
  #ifdef DEPRECATED
  { "xyin",   S(XYIN), _QQ, 1,      "kk",   "iiiiioo",xyinset,NULL          },
  #endif
  #ifdef INC_TEMPEST
  { "tempest",  S(TEMPEST),0, 3,    "k","kiiiiiiiiiop",tempeset,tempest},
  #endif
  #ifdef INC_TEMPO
  { "tempo",    S(TEMPO),0,   3,    "",     "ki",   tempset,tempo           },
  #endif
  #ifdef INC_POW
  { "pow.i",    S(POW),0,   1,      "i",    "iip",  ipow,    NULL,  NULL    },
  { "pow.k",    S(POW),0,   2,      "k",    "kkp",  NULL,    ipow,  NULL    },
  { "##pow.i",  S(POW),0,   1,      "i",    "iip",  ipow,    NULL,  NULL    },
  { "##pow.k",  S(POW),0,   2,      "k",    "kkp",  NULL,    ipow,  NULL    },
  #endif
  #ifdef INC_POW_A
  { "pow.a",    S(POW),0,   2,      "a",    "akp",  NULL,  apow    },
  { "##pow.a",  S(POW),0,   2,      "a",    "akp",  NULL,  apow    },
  #endif
  #ifdef INC_LINRAND
  { "linrand.i",S(PRAND),0, 1,      "i",    "k",    iklinear, NULL, NULL    },
  { "linrand.k",S(PRAND),0, 2,      "k",    "k",    NULL, iklinear, NULL    },
  #endif
#ifdef INC_LINRAND_A
  { "linrand.a",S(PRAND),0, 2,      "a",    "k",    NULL,     alinear },
  #endif
  #ifdef INC_TRIRAND
  { "trirand.i",S(PRAND),0, 1,      "i",    "k",    iktrian, NULL,  NULL    },
  { "trirand.k",S(PRAND),0, 2,      "k",    "k",    NULL, iktrian,  NULL    },
  #endif
  #ifdef INC_TRIRAND_A
  { "trirand.a",S(PRAND),0, 2,      "a",    "k",    NULL,     atrian  },
  #endif
  #ifdef INC_EXPRAND
  { "exprand.i",S(PRAND),0, 1,      "i",    "k",    ikexp, NULL,    NULL    },
  { "exprand.k",S(PRAND),0, 2,      "k",    "k",    NULL,    ikexp, NULL    },
  #endif
  #ifdef INC_EXPRAND_A
  { "exprand.a",S(PRAND),0, 2,      "a",    "k",    NULL,     aexp    },
  #endif
  #ifdef INC_BEXPRAND
  { "bexprnd.i",S(PRAND),0, 1,      "i",    "k",    ikbiexp, NULL,  NULL    },
  { "bexprnd.k",S(PRAND),0, 2,      "k",    "k",    NULL, ikbiexp,  NULL    },
  #endif
  #ifdef INC_BEXPRAND_A
  { "bexprnd.a",S(PRAND),0, 2,      "a",    "k",    NULL,     abiexp  },
  #endif
  #ifdef INC_CAUCHYRAND
  { "cauchy.i", S(PRAND),0, 1,      "i",    "k",    ikcauchy, NULL, NULL    },
  { "cauchy.k", S(PRAND),0, 2,      "k",    "k",    NULL, ikcauchy, NULL    },
  #endif
  #ifdef INC_CAUCHYRAND_A
  { "cauchy.a", S(PRAND),0, 2,      "a",    "k",    NULL,  acauchy },
  #endif
  #ifdef INC_PCAUCHYRAND
  { "pcauchy.i",S(PRAND),0, 1,      "i",    "k",    ikpcauchy, NULL,NULL    },
  { "pcauchy.k",S(PRAND),0, 2,      "k",    "k",    NULL, ikpcauchy,NULL    },
  #endif
  #ifdef INC_PCAUCHYRAND_A
  { "pcauchy.a",S(PRAND),0, 2,      "a",    "k",    NULL,  apcauchy},
  #endif
  #ifdef INC_POISONRAND
  { "poisson.i",S(PRAND),0, 1,      "i",    "k",    ikpoiss, NULL,  NULL    },
  { "poisson.k",S(PRAND),0, 2,      "k",    "k",    NULL, ikpoiss,  NULL    },
  #endif
  #ifdef INC_POISSONRAND_A
  { "poisson.a",S(PRAND),0, 2,      "a",    "k",    NULL,  apoiss  },
  #endif
  #ifdef INC_GAUSSRAND
  { "gauss.i" , S(PRAND),0, 1,      "i",    "k",    ikgaus,  NULL,  NULL    },
  { "gauss.k" , S(PRAND),0, 2,      "k",    "k",    NULL, ikgaus,   NULL    },
  #endif
  #ifdef INC_GAUSSRAND_A
  { "gauss.a" , S(PRAND),0, 2,      "a",    "k",    NULL,  agaus   },
  #endif
  #ifdef INC_GAUSSRAND
  { "gauss.iii" , S(GAUSS),0, 1,      "i",    "ii",    gauss_scalar,  NULL,  NULL    },
  { "gauss.kkk" , S(GAUSS),0, 2,      "k",    "kk",    NULL, gauss_scalar,   NULL    },
  #endif
  #ifdef INC_GAUSSRANS_A
  { "gauss.akk" , S(GAUSS),0, 2,      "a",    "kk",    NULL, gauss_vector   },
  #endif
  #ifdef INC_WEIBILLRAND
  { "weibull.i",S(PRAND),0, 1,      "i",    "kk",   ikweib,  NULL,  NULL    },
  { "weibull.k",S(PRAND),0, 2,      "k",    "kk",   NULL, ikweib,   NULL    },
  #endif
  #ifdef INC_WEIBULLRAND_A
  { "weibull.a",S(PRAND),0, 2,      "a",    "kk",   NULL,  aweib   },
  #endif
  #ifdef INC_BETARAND
  { "betarand.i",S(PRAND),0,1,      "i",    "kkk",  ikbeta, NULL,  NULL     },
  { "betarand.k",S(PRAND),0,2,      "k",    "kkk",  NULL,   ikbeta,NULL     },
  #endif
  #ifdef INC_BETARAND_A
  { "betarand.a",S(PRAND),0,2,      "a",    "kkk",  NULL,  abeta    },
  #endif
  #ifdef INC_SEED
  { "seed",     S(PRAND),0, 1,      "",     "i",    seedrand, NULL, NULL    },
  #endif
  #ifdef GETSEED
  { "getseed.i",S(GETSEED),0, 1,    "i",     "",    getseed, NULL, NULL     },
  { "getseed.k",S(GETSEED),0, 3,    "k",     "",    getseed, getseed, NULL  },
  #endif
  #ifdef INC_UNIRAND
  { "unirand.i",S(PRAND),0, 1,     "i",     "k",    ikuniform, NULL,  NULL  },
  { "unirand.k",S(PRAND),0, 2,     "k",     "k",    NULL,    ikuniform, NULL},
  #endif
  #ifdef INC_UNIRAND_A
  { "unirand.a",S(PRAND),0, 2,     "a",     "k",    NULL, auniform },
  #endif
  #ifdef INC_DISKIN
  { "diskin",S(DISKIN2_ARRAY),0, 3,    "a[]",
    "SPooooooo",
    (SUBR) diskin_init_array_S,
    (SUBR) diskin2_perf_array                         },
  { "diskin2",S(DISKIN2_ARRAY),0, 3, "a[]",
    "SPooooooo",
    (SUBR) diskin2_init_array_S,
    (SUBR) diskin2_perf_array                         },
  { "diskin.i",S(DISKIN2_ARRAY),0, 3,    "a[]",
    "iPooooooo",
    (SUBR) diskin_init_array_I,
    (SUBR) diskin2_perf_array                         },
  { "diskin2.i",S(DISKIN2_ARRAY),0, 3, "a[]",
    "iPooooooo",
    (SUBR) diskin2_init_array_I,
    (SUBR) diskin2_perf_array                         },
  { "diskin",S(DISKIN2),0, 3,    "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "SPooooooo",
    (SUBR) diskin_init_S,
    (SUBR) diskin2_perf                         },
  { "diskin2",S(DISKIN2),0, 3, "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "SPooooooo",
    (SUBR) diskin2_init_S,
    (SUBR) diskin2_perf                         },
  { "diskin.i",S(DISKIN2),0, 3,    "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "iPooooooo",
    (SUBR) diskin_init,
    (SUBR) diskin2_perf                         },
  { "diskin2.i",S(DISKIN2),0, 3, "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "iPooooooo",
    (SUBR) diskin2_init,
    (SUBR) diskin2_perf                         },
  #endif
  { "noteon", S(OUT_ON),0,  1,      "",     "iii",  iout_on, NULL,   NULL    },
  { "noteoff", S(OUT_ON),0, 1,      "",     "iii",  iout_off, NULL,    NULL  },
  { "noteondur",S(OUT_ON_DUR),0,3,  "", "iiii", iout_on_dur_set,iout_on_dur,NULL},
  { "noteondur2",S(OUT_ON_DUR),0,3, "", "iiii", iout_on_dur_set,iout_on_dur2,NULL},
  { "moscil",S(MOSCIL),0,   3,      "",     "kkkkk",moscil_set, moscil, NULL},
  { "midion",S(KOUT_ON),0,  3,      "", "kkk", kvar_out_on_set,kvar_out_on,NULL},
  { "outic",S(OUT_CONTR),0, 1,      "",     "iiiii", out_controller, NULL, NULL},
  { "outkc",S(OUT_CONTR),0, 2,      "",     "kkkkk", NULL, out_controller, NULL},
  { "outic14",S(OUT_CONTR14),0,1,   "",     "iiiiii",out_controller14, NULL,NULL},
  { "outkc14",S(OUT_CONTR14),0,2,   "",     "kkkkkk",NULL, out_controller14, NULL},
  { "outipb",S(OUT_PB),0,   1,      "",     "iiii", out_pitch_bend, NULL , NULL},
  { "outkpb",S(OUT_PB),0,   2,      "",     "kkkk", NULL,  out_pitch_bend, NULL},
  { "outiat",S(OUT_ATOUCH),0,1,     "",     "iiii", out_aftertouch, NULL , NULL},
  { "outkat",S(OUT_ATOUCH),0,2,     "",     "kkkk", NULL,  out_aftertouch, NULL},
  { "outipc",S(OUT_PCHG),0, 1,      "",     "iiii", out_progchange, NULL , NULL},
  { "outkpc",S(OUT_PCHG),0, 2,      "",     "kkkk", NULL,  out_progchange, NULL},
  { "outipat",S(OUT_POLYATOUCH),0,1,"",    "iiiii", out_poly_aftertouch, NULL,NULL},
  { "outkpat",S(OUT_POLYATOUCH),0,2,"",    "kkkkk", NULL, out_poly_aftertouch,NULL},
  { "release",S(REL),0,     3,      "k",    "",     release_set, release, NULL },
  { "xtratim",S(XTRADUR),0, 1,      "",     "i",    xtratim,    NULL,     NULL },
  { "mclock", S(MCLOCK),0,  3,      "",     "i",    mclock_set, mclock,   NULL },
  { "mrtmsg", S(XTRADUR),0, 1,      "",     "i",    mrtmsg,     NULL,     NULL },
  { "midiout",S(MIDIOUT),0,  2,     "",     "kkkk", NULL, midiout,   NULL      },
  { "midiout_i",S(MIDIOUT), 0,  1,     "",     "iiii", midiout,   NULL, NULL     },
  { "midion2", S(KON2),0,    3,     "",     "kkkk", kon2_set, kon2,   NULL     },
  { "nrpn",   S(NRPN),0,     2,     "",     "kkk",  NULL,  nrpn ,NULL          },
  { "mdelay", S(MDELAY),0,   3,     "",     "kkkkk",mdelay_set, mdelay,   NULL },
  #ifdef INC_NSAMP
  { "nsamp.i", S(EVAL),0,    1,     "i",    "i",    numsamp                    },
  #endif
  #ifdef INC_OCTAVE
  { "octave.i", S(EVAL),0,    1,    "i",    "i",     powoftwo               },
  { "octave.k", S(EEVAL),0,    2,    "k",    "k",     NULL,  powoftwo        },
  { "powoftwo.i",S(EVAL),0,  1,     "i",    "i",    powoftwo                   },
  { "powoftwo.k",S(EVAL),0,  2,     "k",    "k",    NULL, powoftwo             },
  #endif
  #ifdef INC_POCTVE_A
  { "powoftwo.a",S(EVAL),0,  2,     "a",    "a",    NULL, powoftwoa      },
  { "octave.a", S(EVAL),0,    2,    "a",    "a",     NULL, powoftwoa  },
  #endif
  #ifdef INC_LOGBASE2
  { "logbtwo.i",S(EVAL),0,   1,     "i",    "i",    ilogbasetwo                },
  { "logbtwo.k",S(EVAL),0,   3,     "k",    "k",    logbasetwo_set, logbasetwo },
  #endif
  #ifdef INC_LOGBASE2_A
  { "logbtwo.a",S(EVAL),0,   3,     "a",    "a",
    logbasetwo_set, logbasetwoa },
  #endif
  #ifdef INC_FILELEN_S
  { "filelen", S(SNDINFO),0, 1,     "i",    "Sp",   filelen_S, NULL, NULL        },
  #endif
  #ifdef INC_FILENCHNLS_S
  { "filenchnls", S(SNDINFO),0, 1,  "i",    "Sp",   filenchnls_S, NULL, NULL     },
  #endif
  #ifdef INC_FILESR_S
  { "filesr", S(SNDINFO),0,  1,     "i",    "Sp",   filesr_S, NULL, NULL         },
  #endif
  #ifdef INC_FILEBIT_S
  { "filebit", S(SNDINFO),0,  1,     "i",   "Sp",   filebit_S, NULL, NULL        },
  #endif
  #ifdef INC_FILEPEAK
  { "filepeak", S(SNDINFOPEAK),0, 1, "i",   "So",   filepeak_S, NULL, NULL       },
  #endif
  #ifdef INC_FILEVALID_S
  { "filevalid", S(FILEVALID),0, 1,  "i",   "S",    filevalid_S, NULL, NULL      },
  #endif
  #ifdef INC_FILELEN
  { "filelen.i", S(SNDINFO),0, 1,     "i",    "ip",   filelen, NULL, NULL        },
  #endif
  #ifdef INC_FILENCHNLS
  { "filenchnls.i", S(SNDINFO),0, 1,  "i",    "ip",   filenchnls, NULL, NULL     },
  #endif
  #ifdef INC_FILESR
  { "filesr.i", S(SNDINFO),0,  1,     "i",    "ip",   filesr, NULL, NULL         },
  #endif
  #ifdef INC_FILEBIT
  { "filebit.i", S(SNDINFO),0,  1,     "i",   "ip",   filebit, NULL, NULL        },
  #endif
  #ifdef INC_FILEPEAK
  { "filepeak.i", S(SNDINFOPEAK),0, 1, "i",   "io",   filepeak, NULL, NULL       },
  #endif
  #ifdef INC_FILEVALID
  { "filevalid.i", S(FILEVALID),0, 1,  "i",   "i",    filevalid, NULL, NULL      },
  { "filevalid.k", S(FILEVALID),0, 2,  "k",   "i",    NULL, filevalid, NULL    },
  #endif
  #ifdef INC_FILEVALID_S
  { "filevalid.k", S(FILEVALID),0, 2,  "k",   "S",    NULL, filevalid_S, NULL    },
  #endif
  /*  { "nlalp", S(NLALP),0,     3,     "a",  "akkoo", nlalp_set, nlalp }, */
#ifdef INC_TABLEW
  { "tableiw",  S(TABL),TW|_QQ, 1, "",   "iiiooo", (SUBR)tablew_init, NULL, NULL},
  { "tablew",  S(TABL),TW, 1,    "",   "iiiooo", (SUBR)tablew_init, NULL, NULL},
    #endif
  #ifdef INC_TABLEW_KK
  { "tablew.kk", S(TABL),TW,  3,    "", "kkiooo",(SUBR)tabl_setup,
    (SUBR)tablew_kontrol, NULL          },
  #endif
  #ifdef INC_TABLEW_AA
  { "tablew.aa", S(TABL),TW,  3,    "", "aaiooo",(SUBR)tabl_setup,
    (SUBR)tablew_audio               },
  #endif
  #ifdef INC_TABLEWKT_K
  { "tablewkt.kk", S(TABL),TW,3, "",  "kkkooo",
    (SUBR)tablkt_setup,(SUBR)tablewkt_kontrol,NULL},
  #endif
#ifdef INC_TABLEWKT_A
  { "tablewkt.aa", S(TABL),TW,3, "",  "aakooo",
    (SUBR)tablkt_setup,(SUBR)tablewkt_audio},
  #endif
#ifdef INC_TABLENG
  { "tableng.i", S(TLEN),TR,1,     "i",  "i",    (SUBR)table_length, NULL,  NULL},
  { "tableng.k",  S(TLEN),TR,2,    "k",  "k",    NULL,  (SUBR)table_length, NULL},
#endif
    #ifdef INC_TABLEIGPW
  { "tableigpw",S(TGP), TB, 1,     "",  "i",    (SUBR)table_gpw, NULL,  NULL},
  { "tablegpw", S(TGP), TB,2,      "",  "k",    NULL,   (SUBR)table_gpw, NULL},
    #endif
#ifdef INC_TABLEMIX
  { "tableimix",S(TABLMIX),TB, 1,  "",  "iiiiiiiii", (SUBR)table_mix, NULL, NULL},
  { "tablemix", S(TABLMIX),TB, 2,  "",  "kkkkkkkkk", NULL, (SUBR)table_mix, NULL},
#endif
#ifdef INC_TABLECOPY
  { "tableicopy",S(TGP),TB, 1, "", "ii",   (SUBR)table_copy, NULL, NULL},
  { "tablecopy", S(TGP),TB, 2, "", "kk", NULL, (SUBR)table_copy, NULL},
#endif
    #ifdef INC_TABLERA
  { "tablera", S(TABLRA),TR, 2,   "a",  "kkk",
        NULL/*(SUBR)table_ra_set*/, (SUBR)table_ra},
    #endif
    #ifdef INC_TABLEWA
  { "tablewa", S(TABLWA),TW, 3,   "k",  "kakp",
    (SUBR)table_wa_set, (SUBR)table_wa},
    #endif
#ifdef INC_TABLEKT
  { "tablekt",  S(TABL),TR, 3,   "k",  "xkooo",  (SUBR)tablkt_setup,
    (SUBR)tablerkt_kontrol,
    NULL         },
  #endif
  #ifdef INC_TABLEKT_A
  { "tablekt.a",  S(TABL),TR, 3,   "a",  "xkooo",  (SUBR)tablkt_setup,
    (SUBR)tablerkt_audio         },
  #endif
  #ifdef INC_TABLEIKT
    { "tableikt", S(TABL),TR, 3,    "k",  "xkooo", (SUBR)tablkt_setup,
    (SUBR)tableirkt_kontrol,
    NULL          },
  #endif
  #ifdef INC_TABLEIEKT_A
  { "tableikt.a", S(TABL),TR, 3,    "a",  "xkooo", (SUBR)tablkt_setup,
    (SUBR)tableirkt_audio          },
  #endif
#ifdef INC_TABLE3KT
  { "table3kt", S(TABL),TR, 3,  "k",  "xkooo", (SUBR)tablkt_setup,
    (SUBR)table3rkt_kontrol,
    NULL         },
  #endif
  #ifdef INC_TABLE3KT_A
  { "table3kt.a", S(TABL),TR, 3,  "a",  "xkooo", (SUBR)tablkt_setup,
    (SUBR)table3rkt_audio          },
  #endif
  #ifdef INC_INZ
  { "inz",    S(IOZ),    ZW, 2,   "",   "k",  NULL,   (SUBR)inz  },
  #endif
  #ifdef INC_OUTZ
  { "outz",   S(IOZ),ZR|IR,  2,   "",   "k",    NULL,   (SUBR)outz },
  #endif
  #ifdef INC_TIMEK
  { "timek.i", S(RDTIME),0, 1,   "i",  "",     (SUBR)timek,   NULL,  NULL },
  { "timek.k",  S(RDTIME),0, 2,  "k",  "",     NULL,    (SUBR)timek, NULL },
  #endif
  #ifdef INC_TIMESR
  { "times.i", S(RDTIME),0, 1,   "i",  "",     (SUBR)timesr,  NULL,  NULL },
  { "times.k",  S(RDTIME),0, 2,  "k",  "",     NULL,    (SUBR)timesr,NULL },
  #endif
  #ifdef INC_TIMEINSTK
  { "timeinstk", S(RDTIME),0, 3, "k",  "",
    (SUBR)instimset, (SUBR)instimek, NULL },
  #endif
  #ifdef INC_TIMEINSTS
  { "timeinsts", S(RDTIME),0, 3, "k",  "",
    (SUBR)instimset, (SUBR)instimes, NULL },
  #endif
  #ifdef INC_PEAK
  { "peak.k",  S(PEAK),0,   2,   "k",  "k",    NULL,    (SUBR)peakk,    NULL    },
  #endif
  #ifdef INC_PEAKA
  { "peak.a",   S(PEAK),0,  2,   "k",  "a",    NULL,     (SUBR)peaka   },
  #endif
  #ifdef INC_PRINTK
  { "printk", S(PRINTK),WR,  3,"",     "ikoooo",
    (SUBR)printkset, (SUBR)printk, NULL },
  #endif
  #ifdef INC_PRINTKS_S
  { "printks",S(PRINTKS),WR, 3,   "",   "SiN",
    (SUBR)printksset_S,(SUBR)printks, NULL },
  #endif
  #ifdef INC_PRINTKS2
  { "printks2", sizeof(PRINTK3),0, 3, "", "Sk", (SUBR)printk3set, (SUBR)printk3 },
  #endif
  #ifdef INC_PRINTKS
  { "printks.i",S(PRINTKS),WR, 3,   "",   "iiN",
    (SUBR)printksset,(SUBR)printks, NULL },
  #endif
  #ifdef INC_PRINTS_S
  { "prints",S(PRINTS),0,   1,   "",   "SN",   (SUBR)printsset_S, NULL, NULL },
  #endif
  #ifdef INC_PRINTS
  { "prints.i",S(PRINTS),0,   1,   "",   "iN",   (SUBR)printsset, NULL, NULL },
  #endif
  #ifdef INC_PRINTK2
  { "printk2", S(PRINTK2), WR, 3, "",   "koo",
    (SUBR)printk2set, (SUBR)printk2, NULL },
  #endif
#ifdef  INC_PORTK
  { "portk",  S(PORT),0,   3, "k",     "kko",  (SUBR)porset, (SUBR)kport, NULL },
#endif
#ifdef  INC_TONEK
  { "tonek",  S(KTONE),0,   3, "k",     "kko",  (SUBR)ktonset, (SUBR)ktone, NULL },
#endif
#ifdef  INC_ATONEK
  { "atonek", S(KTONE),0,   3, "k",     "kko",  (SUBR)ktonset, (SUBR)katone, NULL},
#endif
#ifdef  INC_RESONK
{ "resonk", S(KRESON),0,  3, "k",     "kkkpo",(SUBR)rsnset, (SUBR)kreson, NULL},
#endif
#ifdef  INC_ARESONK
  { "aresonk",S(KRESON),0,  3, "k",     "kkkpo",(SUBR)rsnset, (SUBR)kareson, NULL},
#endif
#ifdef  INC_LIMIT
  { "limit.i", S(LIMIT),0,  1, "i",     "iii",  (SUBR)klimit,  NULL,    NULL      },
  { "limit.k",  S(LIMIT),0, 2, "k",     "kkk",  NULL,          (SUBR)klimit, NULL },
  { "limit.a",  S(LIMIT),0, 2, "a",     "akk",  NULL,  (SUBR)limit },
#endif
  { "prealloc", S(AOP),0,   1, "",      "iio",  (SUBR)prealloc, NULL, NULL  },
   { "prealloc", S(AOP),0,   1, "",      "Sio",  (SUBR)prealloc_S, NULL, NULL  },
  /* opcode   dspace      thread  outarg  inargs  isub    ksub    asub    */
   #ifdef IC_INH
  { "inh",    S(INH),0,     2,      "aaaaaa","",    NULL,   inh     },
  #endif
  #ifdef INC_INO
  { "ino",    S(INO),0,     2,      "aaaaaaaa","",  NULL,   ino     },
  #endif
  #ifdef INC_INX
  { "inx",    S(INALL),0,   2,      "aaaaaaaaaaaaaaaa","",  NULL,   in16 },
  #endif
  #ifdef INC_IN32
  { "in32",   S(INALL),0,   2,      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
    "",     NULL,   in32 },
  #endif
  #ifdef INC_INCH
  { "inch",   S(INCH1),0,    3,      "a",
    "k",    inch1_set,   (SUBR) inch_opcode1 },
  #endif
  #ifdef INC_INCH_M
  { "inch.m",   S(INCH),0,    3,      "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "z",    inch_set,   inch_opcode },
  #endif
  #ifdef INC_INALL
  { "_in",    S(INALL),0,   3,      "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm",
    "",     inch_set,   inall_opcode },
  #endif
  /* Note that there is code in rdorch.c that assumes that opcodes starting
     with the characters out followed by a s, q, h, o or x are in this group
     ***BEWARE***
     CODE REMOVED 2011-Dec-14
  */
  #ifdef INC_OUTCH
  { "outch",  S(OUTCH),IR,   2,      "",     "Z",    NULL,   outch   },
  #endif
  #ifdef INC_OUTC
  { "outc",   S(OUTX),IR,    2,      "",     "y",    ochn,   outall  },
  #endif
  #ifdef INC_CPSXPCH
  { "cpsxpch", S(XENH),TR, 1,      "i",    "iiii", cpsxpch, NULL,  NULL    },
  #endif
  #ifdef INC_CPS2PCH
  { "cps2pch", S(XENH),TR, 1,      "i",    "ii",   cps2pch, NULL,  NULL    },
  #endif
  #ifdef INC_CPSTUN
  { "cpstun", S(CPSTUN),  TR, 2,      "k",    "kkk",   NULL,   cpstun         },
  #endif
  #ifdef CPSTUNI
  { "cpstuni",S(CPSTUNI), TR, 1,      "i",    "ii",   cpstun_i,               },
  #endif
  #ifdef INC_CPSTMID
  { "cpstmid", S(CPSTABLE),0, 1, "i", "i",    (SUBR)cpstmid                    },
  #endif
  #ifdef INC_ADSR
  { "adsr", S(LINSEG),0,     3,     "k",    "iiiio",adsrset,klnseg, NULL },
  #endif
  #ifdef INC_ADSR_A
  { "adsr.a", S(LINSEG),0,     3,     "a",    "iiiio",adsrset, linseg     },
  #endif
#ifdef INC_MADSR
  { "madsr", S(LINSEG),0,    3,     "k",    "iiiioj", madsrset,klnsegr,NULL },
  #endif
  #ifdef INC_MADSR_A
  { "madsr.a", S(LINSEG),0,    3,     "a",    "iiiioj", madsrset, linsegr },
  #endif
  #ifdef INC_XADSR
  { "xadsr", S(EXXPSEG),0,   3,     "k",    "iiiio", xdsrset, kxpseg, NULL   },
  #endif
  #ifdef INC_XADSR_A
  { "xadsr.a", S(EXXPSEG),0,   3,     "a",    "iiiio", xdsrset, expseg    },
  #endif
  #ifdef INC_MXADSR
  { "mxadsr", S(EXPSEG),0,   3,     "k",    "iiiioj", mxdsrset, kxpsegr, NULL},
  #endif
#ifdef INC_MXADSR_A
  { "mxadsr.a", S(EXPSEG),0,   3,     "a",    "iiiioj", mxdsrset, expsegr},
  #endif
  #ifdef INC_SCHEDULE
  { "schedule", S(SCHED),0,  1,     "",     "iiim",
    schedule, NULL, NULL },
  #endif
  #ifdef INC_SCHEDULE_N
  { "schedule.N", S(SCHED),0,  1,     "",     "iiiN",
    schedule_N, NULL, NULL },
  #endif
  #ifdef INC_SCHEDULE_S
  { "schedule.S", S(SCHED),0,  1,     "",     "Siim",
    schedule_S, NULL, NULL },
  #endif
  #ifdef INC_SCHEDULE_SN
  { "schedule.SN", US(SCHED),0,  1,     "",     "SiiN",
    schedule_SN, NULL, NULL },
  #endif
  #ifdef INC_SCHEDULE_ARRAY
  { "schedule.array", S(SCHED),0,  1,     "",     "i[]",
    schedule_array, NULL, NULL },
  #endif
  /* **** Experimental schedulek opcodes **** */
  #ifdef INC_SCHEDULE
  { "schedulek",   S(SCHED),0,  2,     "",     "kkkM",
    NULL, schedule, NULL },
  #endif
  #ifdef INC_SCHEDULE_N
  { "schedulek.N", S(SCHED),0,  2,     "",     "kkkN",
    NULL, schedule_N, NULL },
  #endif
  #ifdef INC_SCHEDULE_S
  { "schedulek.S", S(SCHED),0,  2,     "",     "SkkM",
    NULL, schedule_S, NULL },
  #endif
  #ifdef INC_SCHEDULE_SN  
  { "schedulek.SN", S(SCHED),0, 2,     "",     "SkkN",
    NULL, schedule_SN, NULL },
  { "schedulek.array",   S(SCHED),0,  2,     "",     "k[]",
    NULL, schedule_array, NULL },
  #endif
  /* **** End of schedulek **** */
  #ifdef INC_SCHEEDWHEN
  { "schedwhen", S(WSCHED),0,3,     "",     "kkkkm",ifschedule, kschedule, NULL },
  { "schedwhen", S(WSCHED),0,3,     "",     "kSkkm",ifschedule, kschedule, NULL },
  #endif
  #ifdef INC_SCHEDKWHEN
  { "schedkwhen", S(TRIGINSTR),0, 3,"",     "kkkkkz",triginset, ktriginstr, NULL },
  #endif
  #ifdef INC_SCHEDKWHEN_S
  { "schedkwhen.S", S(TRIGINSTR),0, 3,"",    "kkkSkz",
                                             triginset_S, ktriginstr_S, NULL },
  #endif
  #ifdef INC_SCHEDKWHEN
  { "schedkwhennamed", S(TRIGINSTR),0, 3,"", "kkkkkz",triginset, ktriginstr, NULL },
  #endif
  #ifdef INC_SCHEDKWHEN_S
  { "schedkwhennamed.S", S(TRIGINSTR),0, 3,"",
                                        "kkkSkz",triginset_S, ktriginstr_S, NULL },
  #endif
  #ifdef INC_TRIGSEQ
  { "trigseq", S(TRIGSEQ),0, 3,     "",     "kkkkkz", trigseq_set, trigseq, NULL },
  #endif
  { "event", S(LINEVENT),0,  2,     "",     "Skz",  NULL, eventOpcode, NULL   },
  { "event_i", S(LINEVENT),0,1,     "",     "Sim",  eventOpcodeI, NULL, NULL  },
  { "event.S", S(LINEVENT),0,  2,     "",    "SSz",  NULL, eventOpcode_S, NULL   },
  { "event_i.S", S(LINEVENT),0,1,     "",    "SSm",  eventOpcodeI_S, NULL, NULL  },
  { "nstance", S(LINEVENT2),0,2,     "k",  "kkz",  NULL, instanceOpcode, NULL   },
  { "nstance.i", S(LINEVENT2),0,1,   "i",  "iiim",  instanceOpcode, NULL, NULL  },
  { "nstance.kS", S(LINEVENT2),0, 2, "k",  "SSz",  NULL, instanceOpcode_S, NULL },
  { "nstance.S", S(LINEVENT2),0, 1,  "i",  "Siim",  instanceOpcode_S, NULL, NULL},
  { "turnoff.i", S(KILLOP),0,1,     "",     "i", kill_instance, NULL, NULL  },
  { "turnoff.k", S(KILLOP),0,2,     "",     "k", NULL, kill_instance, NULL},
  #ifdef INC_LFO
  { "lfo", S(LFO),0,         3,     "k",    "kko",  lfoset,   lfok,   NULL   },
  #endif
  #ifdef LFOA
  { "lfo.a", S(LFO),0,         3,     "a",    "kko",  lfoset,   lfoa    },
  #endif
  #ifdef INC_OSCILS
  { "oscils",   S(OSCILS),0, 3,     "a", "iiio",
    (SUBR)oscils_set, (SUBR)oscils  },
  #endif
  #ifdef INC_LPHASOR
  { "lphasor",  S(LPHASOR),0,3,     "a", "xooooooo" ,
    (SUBR)lphasor_set, (SUBR)lphasor },
  #endif
  #ifdef INC_TABLEXKT
  { "tablexkt", S(TABLEXKT),TR, 3, "a", "xkkiooo", (SUBR)tablexkt_set,
    (SUBR)tablexkt              },
  #endif
  #ifdef INC_REVERB2
  { "reverb2",  S(NREV2),0,  3,     "a",    "akkoojoj",
    (SUBR)reverbx_set,(SUBR)reverbx  },
  { "nreverb",  S(NREV2),0,  3,     "a",    "akkoojoj",
    (SUBR)reverbx_set,(SUBR) reverbx },
  #endif
  #ifdef INC_FASSIGN
  { "=.f",      S(FASSIGN),0, 3,    "f",   "f", (SUBR)fassign_set, (SUBR)fassign },
  { "init.f",   S(FASSIGN),0, 1,    "f",   "f", (SUBR)fassign_set, NULL, NULL    },
  #endif
  #ifdef INC_PVSANAL
  { "pvsanal",  S(PVSANAL), 0, 3,   "f",   "aiiiioo", pvsanalset, pvsanal   },
  #endif
  #ifdef INC_PVSYNTH
  { "pvsynth",  S(PVSYNTH),0, 3,    "a",   "fo",     pvsynthset, pvsynth },
  #endif
#ifdef INC_PVSADSYN  
  { "pvsadsyn", S(PVADS),0,   3,    "a",   "fikopo", pvadsynset, pvadsyn, NULL },
  #endif
  #ifdef INC_PVSCROSS
  { "pvscross", S(PVSCROSS),0,3,    "f",   "ffkk",   pvscrosset, pvscross, NULL },
  #endif
  #ifdef INC_PVSFREAD
  { "pvsfread", S(PVSFREAD),0,3,    "f",   "kSo",    pvsfreadset_S, pvsfread, NULL},
  { "pvsfread.i", S(PVSFREAD),0,3,  "f",   "kio",    pvsfreadset, pvsfread, NULL},
  #endif
  #ifdef INC_PVSMASKA
  { "pvsmaska", S(PVSMASKA),0,3,    "f",   "fik",    pvsmaskaset, pvsmaska, NULL  },
  #endif
  #ifdef INC_PVSFTW
  { "pvsftw",   S(PVSFTW),  TW, 3,  "k",   "fio",    pvsftwset, pvsftw, NULL  },
  #endif
  #ifdef INC_PVSFTR
  { "pvsftr",   S(PVSFTR),TR, 3,    "",    "fio",    pvsftrset, pvsftr, NULL  },
  #endif
  #ifdef INC_PVSINFO
  { "pvsinfo",  S(PVSINFO),0, 1,    "iiii","f",      pvsinfo, NULL, NULL    },
  #endif
  #ifdef INC_SEMITONE
  { "semitone.i",S(EVAL),0,   1,    "i",    "i",     semitone               },
  { "semitone.k",S(EVAL),0,   2,    "k",    "k",     NULL,  semitone        },
  #endif
  #ifdef INC_SEMITONE_A
  { "semitone.a",S(EVAL),0,   2,    "a",    "a",     NULL, asemitone  },
  #endif
  #ifdef INC_CENT
  { "cent.i",   S(EVAL),0,    1,    "i",    "i",     cent                   },
  #endif
#ifdef INC_CENT_A
  { "cent.k",   S(EVAL),0,    2,    "k",    "k",     NULL,  cent            },
  { "cent.a",   S(EVAL),0,    2,    "a",    "a",     NULL, acent      },
  #endif
  #ifdef INC_DB
  { "db.i",     S(EVAL),0,    1,    "i",    "i",     db                     },
  { "db.k",     S(EVAL),0,    2,    "k",    "k",     NULL, db               },
  #endif
  #ifdef INC_DB_A
  { "db.a",     S(EVAL),0,    2,    "a",    "a",     NULL, dba        },
  #endif
#ifdef INC_MIDICHN
  { "midichn",  S(MIDICHN),0, 1,    "i",    "",      midichn, NULL, NULL    },
#endif
#ifdef INC_PGMASSIGN
  { "pgmassign",S(PGMASSIGN),0, 1,  "",     "iio",   pgmassign, NULL, NULL  },
  { "pgmassign.S",S(PGMASSIGN),0, 1, "",    "iSo",   pgmassign_S, NULL, NULL  },
#endif
#ifdef INC_MIDIIN
  { "midiin",   S(MIDIIN),0,  3,    "kkkk", "",      midiin_set, midiin, NULL },
#endif
#ifdef INC_PGMCHN
  { "pgmchn",   S(PGMIN),0,  3,     "kk",   "o",     pgmin_set, pgmin, NULL },
#endif
#ifdef INC_CTLCHN
  { "ctlchn",   S(CTLIN),0,  3,     "kkk",  "oo",    ctlin_set, ctlin, NULL },
#endif
  { "miditemptempoo", S(MIDITEMPO),0, 3, "k",    "",
    (SUBR) midiTempoOpcode, (SUBR) midiTempoOpcode, NULL    },
  { "midifilestatus", S(MIDITEMPO),0, 2, "k",    "",
   NULL, (SUBR) midiFileStatus, NULL },
  { "midinoteoff", S(MIDINOTEON),0,3   ,"", "xx",   midinoteoff, midinoteoff, },
  { "midinoteonkey", S(MIDINOTEON),0,3, "", "xx",   midinoteonkey, midinoteonkey },
  { "midinoteoncps", S(MIDINOTEON),0, 3, "", "xx",  midinoteoncps,midinoteoncps },
  { "midinoteonoct", S(MIDINOTEON),0, 3, "", "xx",  midinoteonoct,midinoteonoct },
  { "midinoteonpch", S(MIDINOTEON),0, 3, "", "xx",  midinoteonpch, midinoteonpch },
  { "midipolyaftertouch", S(MIDIPOLYAFTERTOUCH),0,
    3,   "", "xxoh", midipolyaftertouch, midipolyaftertouch},
  { "midicontrolchange", S(MIDICONTROLCHANGE),0,
    3, "", "xxoh",midicontrolchange, midicontrolchange    },
  { "midiprogramchange", S(MIDIPROGRAMCHANGE),0,
    3, "", "x", midiprogramchange, midiprogramchange      },
  { "midichannelaftertouch", S(MIDICHANNELAFTERTOUCH),0,
    3, "", "xoh",midichannelaftertouch, midichannelaftertouch },
  { "midipitchbend", S(MIDIPITCHBEND),0,3, "", "xoh",
    midipitchbend, midipitchbend },
  { "mididefault", S(MIDIDEFAULT),0, 3, "", "xx",   mididefault, mididefault },
  { "invalue",   0xFFFF,   _CR,    0,   NULL,   NULL, NULL, NULL },
  { "invalue.k", S(INVAL),_CR, 3, "k", "i", (SUBR) invalset,(SUBR)kinval, NULL },
  { "invalue.i", S(INVAL),_CR, 1, "i", "i", (SUBR) invalsetgo, NULL, NULL },
  { "invalue.iS", S(INVAL),_CR, 1, "i", "S", (SUBR) invalsetSgo, NULL, NULL },
  { "invalue.kS", S(INVAL),_CR, 3, "k", "S",(SUBR) invalset_S,(SUBR)kinval, NULL },
  { "invalue.S", S(INVAL),_CR, 3, "S", "i",
                                      (SUBR) invalset_string, (SUBR)kinvalS, NULL },
  { "invalue.SS", S(INVAL),_CR, 3, "S", "S",
                                    (SUBR) invalset_string_S, (SUBR)kinvalS, NULL },
  { "outvalue", S(OUTVAL), _CW, 3, "", "ik", (SUBR) outvalset, (SUBR)koutval, NULL},
  { "outvalue.i", S(OUTVAL), _CW, 1, "", "ii", (SUBR) outvalsetgo, NULL, NULL},
  { "outvalue.Si", S(OUTVAL), _CW, 1, "", "Si",
                                          (SUBR) outvalsetSgo, NULL, NULL},
  { "outvalue.k", S(OUTVAL), _CW, 3, "", "Sk",
                                          (SUBR) outvalset_S, (SUBR)koutval, NULL},
  { "outvalue.S", S(OUTVAL), _CW, 3, "", "iS",
                                     (SUBR) outvalset_string, (SUBR)koutvalS, NULL},
  { "outvalue.SS", S(OUTVAL), _CW, 3, "", "SS",
                                   (SUBR) outvalset_string_S, (SUBR)koutvalS, NULL},
  /* IV - Oct 20 2002 */
  { "subinstr", S(SUBINST),0, 3, "mmmmmmmm", "SN",  subinstrset_S, subinstr },
  { "subinstrinit", S(SUBINST),0, 1, "",    "SN",   subinstrset_S, NULL, NULL     },
  { "subinstr.i", S(SUBINST),0, 3, "mmmmmmmm", "iN",  subinstrset, subinstr },
  { "subinstrinit.i", S(SUBINST),0, 1, "",    "iN",   subinstrset, NULL, NULL     },
  { "nstrnum", S(NSTRNUM),0, 1,     "i",    "S",    nstrnumset_S, NULL, NULL      },
  { "nstrnum.i", S(NSTRNUM),0, 1,     "i",    "i",    nstrnumset, NULL, NULL      },
  { "nstrstr", S(NSTRSTR),0, 1,       "S",    "i",    nstrstr, NULL, NULL      },
  { "nstrstr.k", S(NSTRSTR),0, 2,     "S",    "k",    NULL, nstrstr, NULL      },
  { "turnoff2",   0xFFFB,   _CW,    0, NULL,   NULL,   NULL, NULL, NULL          },
  { "turnoff2_i.S",S(TURNOFF2),_CW,1,     "",     "Soo",  turnoff2S, NULL     },
  { "turnoff2_i.i",S(TURNOFF2),_CW,1,     "",     "ioo",  turnoff2k, NULL     },
  { "turnoff2.S",S(TURNOFF2),_CW,2,     "",     "Skk",  NULL, turnoff2S, NULL     },
  { "turnoff2.c",S(TURNOFF2),_CW,2,     "",     "ikk",  NULL, turnoff2k, NULL     },
  { "turnoff2.k",S(TURNOFF2),_CW,2,     "",     "kkk",  NULL, turnoff2k, NULL     },
  { "turnoff2.i",S(TURNOFF2),_CW,2,     "",     "ikk",  NULL, turnoff2k, NULL     },
  { "turnoff2.r",S(TURNOFF2),_CW,2,     "",     "ikk",  NULL, turnoff2k, NULL     },
  { "turnoff3.S",S(TURNOFF2),_CW,2,     "",     "S",  NULL, turnoff3S, NULL     },
  { "turnoff3.c",S(TURNOFF2),_CW,2,     "",     "i",  NULL, turnoff3k, NULL     },
  { "turnoff3.k",S(TURNOFF2),_CW,2,     "",     "k",  NULL, turnoff3k, NULL     },
  { "turnoff3.i",S(TURNOFF2),_CW,2,     "",     "i",  NULL, turnoff3k, NULL     },
  { "turnoff3.r",S(TURNOFF2),_CW,2,     "",     "i",  NULL, turnoff3k, NULL     },
  { "cngoto", S(CGOTO),0,   3,      "",     "Bl",   ingoto, kngoto, NULL     },
  { "cnkgoto", S(CGOTO),0,   2,      "",     "Bl",   NULL,  kngoto, NULL     },
  { "cingoto", S(CGOTO),0,   1,      "",     "Bl",   ingoto, NULL, NULL     },
  { "tempoval", S(GTEMPO),0, 2,  "k", "",      NULL, (SUBR)gettempo, NULL    },
#ifdef INC_DOWNSAMP
  { "downsamp",S(DOWNSAMP),0,3, "k", "ao",   (SUBR)downset,(SUBR)downsamp        },
#endif
#ifdef INC_UPSAMP
  { "upsamp", S(UPSAMP),0,  2,  "a", "k",    NULL,   (SUBR)upsamp        },
#endif
  /* IV - Sep 5 2002 */
#ifdef INC_INTERP
  { "interp", S(INTERP),0,  3,  "a", "kooo", (SUBR)interpset, (SUBR)interp  },
#endif
#ifdef INC_A_K
  { "a.k",    S(INTERP),0,  3,  "a", "k",    (SUBR)a_k_set,   (SUBR)interp  },
#endif
#ifdef INC_INTEG
  { "integ", S(INDIFF),  0, 3,  "a", "xo",
    (SUBR)indfset,(SUBR)integrate},
#endif
#ifdef INC_INTEG_K
  { "integ.k", S(INDIFF),  0, 3,  "k", "xo",
    (SUBR)indfset,(SUBR)kntegrate, NULL},
#endif
#ifdef INC_DIFF
  { "diff",   S(INDIFF),0,  3,  "a", "xo", (SUBR)indfset, (SUBR)diff },
#endif
#ifdef INC_DIFF_K
  { "diff.k",   S(INDIFF),0,  3,  "k", "xo", (SUBR)indfset,(SUBR)kdiff, NULL },
#endif
#ifdef INC_SAMPHOLD
  { "samphold",S(SAMPHOLD),0,3, "a", "xxoo",
    (SUBR)samphset,(SUBR)samphold    },
#endif
#ifdef INC_SAMPHOLD_K
  { "samphold.k",S(SAMPHOLD),0,3, "k", "xxoo",
    (SUBR)samphset,(SUBR)ksmphold,NULL  },
#endif
  #ifdef INC_DELAY
  { "delay",  S(DELAY),0,   3,  "a", "aio",  (SUBR)delset,   (SUBR)delay   },
  #endif
  #ifdef INC_DELAYR
  { "delayr", S(DELAYR),0,  3,  "aX","io",   (SUBR)delrset,   (SUBR)delayr  },
  #endif
  #ifdef INC_DELAYW
  { "delayw", S(DELAYW),0,  3,  "",  "a",    (SUBR)delwset,   (SUBR)delayw  },
  #endif
  #ifdef INC_DELAY1
  { "delay1", S(DELAY1),0,  3,  "a", "ao",   (SUBR)del1set,   (SUBR)delay1  },
  #endif
  #ifdef INC_DELTAP
  { "deltap", S(DELTAP),0,  3,  "a", "ko",   (SUBR)tapset,   (SUBR)deltap  },
  #endif
  #ifdef INC_DELTEIAP
  { "deltapi",S(DELTAP),0,  3,  "a", "xo",   (SUBR)tapset,   (SUBR)deltapi },
  #endif
  #ifdef INC_DELTAPN
  { "deltapn",S(DELTAP),0,  3,  "a", "xo",   (SUBR)tapset,   (SUBR)deltapn },
  #endif
  #ifdef INC_DELTAP3
  { "deltap3",S(DELTAP),0,  3,  "a", "xo",   (SUBR)tapset,   (SUBR)deltap3 },
  #endif
  #ifdef INC_REVERB
  { "reverb", S(REVERB),0,  3,  "a", "ako",  (SUBR)rvbset,   (SUBR)reverb  },
  #endif
  #ifdef INC_VDELAY
  { "vdelay",   S(VDEL),0,  3,  "a", "axio", (SUBR)vdelset,  (SUBR)vdelay  },
  #endif
  #ifdef INC_VDELAY3
  { "vdelay3",  S(VDEL),0,  3,  "a", "axio", (SUBR)vdelset,  (SUBR)vdelay3 },
  #endif
  #ifdef INC_VDELAYXWS
  { "vdelayxwq",S(VDELXQ),0,3,  "aaaa", "aaaaaiio",
        (SUBR)vdelxqset, (SUBR)vdelayxwq},
  #endif
  #ifdef INC_VDELAYXWS
  { "vdelayxws",S(VDELXS),0,3,  "aa", "aaaiio", (SUBR)vdelxsset,
    (SUBR)vdelayxws                  },
  #endif
  #ifdef INC_VDELAYXW
  { "vdelayxw", S(VDELX),0, 3,  "a",  "aaiio",
    (SUBR)vdelxset, (SUBR)vdelayxw},
  #endif
  #ifdef INC_VDELAYXQ
  { "vdelayxq", S(VDELXQ),0,3,  "aaaa", "aaaaaiio",
    (SUBR)vdelxqset, (SUBR)vdelayxq},
  #endif
  #ifdef INC_VDELAYXS
  { "velayxs", S(VDELXS),0,3,  "aa", "aaaiio",
    (SUBR)vdelxsset, (SUBR)vdelayxs},
  #endif
  #ifdef INC_VDELAYX
  { "vdelayx",  S(VDELX),0, 3,  "a",  "aaiio", (SUBR)vdelxset, (SUBR)vdelayx},
  #endif
  #ifdef INC_DELTAPX
  { "deltapx",  S(DELTAPX),0,3, "a",  "aio",  (SUBR)tapxset,  (SUBR)deltapx },
  #endif
  #ifdef INC_DELTAPXW
  { "deltapxw", S(DELTAPX),0,3,  "",  "aaio", (SUBR)tapxset, (SUBR)deltapxw },
  #endif
  #ifdef INC_MULTITAP
  { "multitap", S(MDEL),0,  3,   "a", "am",
    (SUBR)multitap_set,(SUBR)multitap_play},
  #endif
  #ifdef INC_COMB
  { "comb",   S(COMB),0,    3,  "a",  "akioo", (SUBR)cmbset, (SUBR)comb    },
  #endif
  #ifdef INC_COMINV
  { "combinv",S(COMB),0,    3,  "a",  "akioo", (SUBR)cmbset, (SUBR)invcomb },
  #endif
  #ifdef INC_ALPASS
  { "alpass", S(COMB),0,    3,  "a",  "axioo", (SUBR)cmbset, (SUBR)alpass  },
  #endif
  #ifdef INC_STRSET
  { "strset",   S(STRSET_OP),0,   1,  "",     "iS",
     (SUBR) strset_init, NULL, NULL                        },
  #endif
  #ifdef INC_STRGET
  { "strget",   S(STRGET_OP),0,   1,  "S",    "i",
     (SUBR) strget_init, NULL, NULL                        },
  #endif
  #ifdef INC_S
  {  "S",   S(STRGET_OP),0,   1,  "S",    "i",
     (SUBR) s_opcode, NULL, NULL                           },
  {  "S",   S(STRGET_OP),0,   3,  "S",    "k",
     (SUBR) s_opcode,(SUBR) s_opcode_k, NULL                       },
  #endif
  #ifdef INC_STRCPY
  {  "strcpy",   S(STRCPY_OP),0,   1,  "S",    "S",
     (SUBR) strcpy_opcode_S, NULL, NULL                     },
  {  "strcpy",   S(STRGET_OP),0,   1,  "S",    "i",
     (SUBR) strcpy_opcode_p, NULL, NULL                     },
  {  "strcpyk",  S(STRCPY_OP),0,   3,  "S",    "S",
     (SUBR) strcpy_opcode_S, (SUBR) strcpy_opcode_S, NULL          },
  {  "strcpyk.k",  S(STRGET_OP),0,   3,  "S",    "k",
     (SUBR) strcpy_opcode_p, (SUBR) strcpy_opcode_p, NULL          },
  #endif
  #ifdef INC_STRCAT
  {  "strcat",   S(STRCAT_OP),0,   1,  "S",    "SS",
     (SUBR) strcat_opcode, NULL, NULL                      },
  {  "strcatk",  S(STRCAT_OP),0,   3,  "S",    "SS",
     (SUBR) strcat_opcode, (SUBR) strcat_opcode, NULL             },
  #endif
  #ifdef INC_STRCMP
  {  "strcmp",   S(STRCMP_OP),0,   1,  "i",    "SS",
     (SUBR) strcmp_opcode, NULL, NULL                      },
  {  "strcmpk",  S(STRCAT_OP),0,   3,  "k",    "SS",
     (SUBR) strcmp_opcode, (SUBR) strcmp_opcode, NULL             },
  #endif
  #ifdef INC_SPRINTF
  {  "sprintf",  S(SPRINTF_OP),0,  1,  "S",    "STN",
     (SUBR) sprintf_opcode, NULL, NULL                     },
  {  "sprintfk", S(SPRINTF_OP),WR,  3,  "S",    "SUN",
     (SUBR) sprintf_opcode, (SUBR) sprintf_opcode, NULL           },
  #endif
  #ifdef INC_PRINTF
  {  "printf_i", S(PRINTF_OP),0,   1,  "",     "SiN", /* SiTN */
     (SUBR) printf_opcode_init, NULL, NULL                 },
  {  "printf",   S(PRINTF_OP),WR,   3,  "",     "SkN", /* SkUN */
     (SUBR) printf_opcode_set, (SUBR) printf_opcode_perf, NULL    },
  #endif
  #ifdef INC_PUTS
  {  "puts",     S(PUTS_OP),WR,     3,  "",     "Sko",
     (SUBR) puts_opcode_init, (SUBR) puts_opcode_perf, NULL       },
  #endif
  #ifdef INC_STRTOD
  {  "strtod",   S(STRSET_OP),0,   1,  "i",    "S",
     (SUBR) strtod_opcode_S, NULL, NULL                      },
  {  "strtod",   S(STRTOD_OP),0,   1,  "i",    "i",
     (SUBR) strtod_opcode_p, NULL, NULL                      },
  {  "strtodk",  S(STRSET_OP),0,   3,  "k",    "S",
     (SUBR) strtod_opcode_S, (SUBR) strtod_opcode_S, NULL          },
  #endif
  #ifdef INC_STRTOL
  {  "strtol",   S(STRSET_OP),0,   1,  "i",    "S",
     (SUBR) strtol_opcode_S, NULL, NULL                      },
  {  "strtol",   S(STRTOD_OP),0,   1,  "i",    "i",
     (SUBR) strtol_opcode_p, NULL, NULL                      },
  {  "strtolk",  S(STRSET_OP),0,   3,  "k",    "S",
     (SUBR) strtol_opcode_S, (SUBR) strtol_opcode_S, NULL         },
  #endif
  #ifdef INC_STRSUB
  {  "strsub",   S(STRSUB_OP),0,   1,  "S",    "Soj",
     (SUBR) strsub_opcode, NULL, NULL                      },
  {  "strsubk",  S(STRSUB_OP),0,   3,  "S",    "Skk",
     (SUBR) strsub_opcode, (SUBR) strsub_opcode, NULL             },
  #endif
  #ifdef INC_STRCHAR
  {  "strchar",  S(STRCHAR_OP),0,  1,  "i",    "So",
     (SUBR) strchar_opcode, NULL, NULL                     },
  {  "strchark", S(STRCHAR_OP),0,  3,  "k",    "SO",
     (SUBR) strchar_opcode, (SUBR) strchar_opcode, NULL           },
  #endif
  #ifdef INC_STRLEN
  {  "strlen",   S(STRLEN_OP),0,   1,  "i",    "S",
     (SUBR) strlen_opcode, NULL, NULL                      },
  {  "strlenk",  S(STRLEN_OP),0,   3,  "k",    "S",
     (SUBR) strlen_opcode, (SUBR) strlen_opcode, NULL             },
  #endif
  #ifdef STRUPPER
  {  "strupper", S(STRUPPER_OP),0, 1,  "S",    "S",
     (SUBR) strupper_opcode, NULL, NULL                    },
  {  "strupperk", S(STRUPPER_OP),0, 3, "S",    "S",
     (SUBR) strupper_opcode, (SUBR) strupper_opcode, NULL         },
  #endif
  #ifdef STRLOWER
  {  "strlower", S(STRUPPER_OP),0, 1,  "S",    "S",
     (SUBR) strlower_opcode, NULL, NULL                    },
  {  "strlowerk", S(STRUPPER_OP),0, 3, "S",    "S",
     (SUBR) strlower_opcode, (SUBR) strlower_opcode, NULL         },
  #endif
  #ifdef INC_GETCFG
  {  "getcfg",   S(GETCFG_OP),0,   1,  "S",    "i",
     (SUBR) getcfg_opcode, NULL, NULL                      },
  #endif
  #ifdef STRINDEX
  {  "strindex", S(STRINDEX_OP),0, 1,  "i",    "SS",
     (SUBR) strindex_opcode, NULL, NULL                    },
  {  "strindexk", S(STRINDEX_OP),0, 3, "k",    "SS",
     (SUBR) strindex_opcode, (SUBR) strindex_opcode, NULL         },
  #endif
  #ifdef STRRINDEX
  {  "strrindex", S(STRINDEX_OP),0, 1, "i",    "SS",
     (SUBR) strrindex_opcode, NULL, NULL                   },
  {  "strrindexk", S(STRINDEX_OP),0, 3, "k",   "SS",
     (SUBR) strrindex_opcode, (SUBR) strrindex_opcode, NULL       },
  #endif
  #ifdef INC_PRINT_TYPE
  {  "print_type", S(PRINT_TYPE_OP),0, 1, "",   ".",
     (SUBR) print_type_opcode, NULL, NULL       },
  #endif
#ifdef HAVE_CURL
  {  "strfromurl", S(STRCPY_OP), 0, 1, "S", "S", (SUBR) str_from_url     },
#endif
  #ifdef INC_CHANGED_S
  {  "changed.S", S(STRCHGD),0, 3, "k",   "S",
     (SUBR) str_changed, (SUBR) str_changed_k, NULL       },
  {  "changed2.S", S(STRCHGD),0, 3, "k",   "S",
     (SUBR) str_changed, (SUBR) str_changed_k, NULL       },
  #endif
  { "loop_lt.i", S(LOOP_OPS),0,  1,  "", "iiil", (SUBR) loop_l_i, NULL, NULL   },
  { "loop_le.i", S(LOOP_OPS),0,  1,  "", "iiil", (SUBR) loop_le_i, NULL, NULL  },
  { "loop_gt.i", S(LOOP_OPS),0,  1,  "", "iiil", (SUBR) loop_g_i, NULL, NULL   },
  { "loop_ge.i", S(LOOP_OPS),0,  1,  "", "iiil", (SUBR) loop_ge_i, NULL, NULL  },
  { "loop_lt.k", S(LOOP_OPS),0,  2,  "", "kkkl", NULL, (SUBR) loop_l_p, NULL   },
  { "loop_le.k", S(LOOP_OPS),0,  2,  "", "kkkl", NULL, (SUBR) loop_le_p, NULL  },
  { "loop_gt.k", S(LOOP_OPS),0,  2,  "", "kkkl", NULL, (SUBR) loop_g_p, NULL   },
  { "loop_ge.k", S(LOOP_OPS),0,  2,  "", "kkkl", NULL, (SUBR) loop_ge_p, NULL  },
  { "chnget",      0xFFFF,    _CR                                             },
  { "chnget.i",    S(CHNGET),_CR,           1,      "i",            "S",
    (SUBR) chnget_opcode_init_i, NULL, NULL               },
  { "chnget.k",    S(CHNGET),_CR,           3,      "k",            "S",
    (SUBR) chnget_opcode_init_k, (SUBR) notinit_opcode_stub, NULL },
  { "chngeti.i",    S(CHNGET),_CR,           1,      "i[]",            "S[]",
    (SUBR) chnget_array_opcode_init_i, NULL, NULL               },
  { "chngeta.a",    S(CHNGET),_CR,           3,      "a[]",            "S[]",
    (SUBR) chnget_array_opcode_init, (SUBR) notinit_opcode_stub, NULL },
  { "chngets.s",    S(CHNGET),_CR,           3,      "S[]",            "S[]",
    (SUBR) chnget_array_opcode_init, (SUBR) notinit_opcode_stub, NULL },
  { "chngetk.k",    S(CHNGET),_CR,           3,      "k[]",            "S[]",
    (SUBR) chnget_array_opcode_init, (SUBR) notinit_opcode_stub, NULL },
  { "chnget.a",    S(CHNGET),_CR,           3,      "a",            "S",
    (SUBR) chnget_opcode_init_a, (SUBR) notinit_opcode_stub },
  { "chnget.S",    S(CHNGET),_CR,           3,      "S",            "S",
    (SUBR) chnget_opcode_init_S, (SUBR) chnget_opcode_perf_S, NULL},
  { "chngetks",    S(CHNGET),_CR,           2,      "S",            "S",
    NULL, (SUBR) chnget_opcode_perf_S, NULL},
  { "chnset",      0xFFFB,              _CW                               },

  { "chnseti.i",    S(CHNGET),_CW,          1,      "",             "i[]S[]",
    (SUBR) chnset_array_opcode_init_i, NULL, NULL               },
  { "chnsetk.k",    S(CHNGET),_CW,           3,      "",             "k[]S[]",
    (SUBR) chnset_array_opcode_init, (SUBR) notinit_opcode_stub, NULL },
  { "chnseta.a",    S(CHNGET),_CW,           3,      "",             "a[]S[]",
    (SUBR) chnset_array_opcode_init, (SUBR) notinit_opcode_stub, NULL },
  { "chnsets.s",    S(CHNGET),_CW,           3,      "",             "S[]S[]",
    (SUBR) chnset_array_opcode_init, (SUBR) notinit_opcode_stub, NULL },

  { "chnset.i",    S(CHNGET),_CW,          1,      "",             "iS",
    (SUBR) chnset_opcode_init_i, NULL, NULL               },
    { "chnset.r",    S(CHNGET),0,           1,      "",             "iS",
      (SUBR) chnset_opcode_init_i, NULL, NULL               },
    { "chnset.c",    S(CHNGET),0,           1,      "",             "iS",
      (SUBR) chnset_opcode_init_i, NULL, NULL               },
  { "chnset.k",    S(CHNGET),_CW,           3,      "",             "kS",
    (SUBR) chnset_opcode_init_k, (SUBR) notinit_opcode_stub, NULL },
  { "chnset.a",    S(CHNGET),_CW,           3,      "",             "aS",
    (SUBR) chnset_opcode_init_a, (SUBR) notinit_opcode_stub },
  { "chnset.S",    S(CHNGET),_CW,           3,      "",             "SS",
    (SUBR) chnset_opcode_init_S, (SUBR) chnset_opcode_perf_S, NULL },
  { "chnsetks",    S(CHNGET),_CW,           2,      "",             "SS",
    NULL, (SUBR) chnset_opcode_perf_S, NULL },
  { "chnmix",      S(CHNGET),           _CB, 3,      "",             "aS",
    (SUBR) chnmix_opcode_init, (SUBR) notinit_opcode_stub  },
  { "chnclear",    S(CHNCLEAR),        _CW, 3,      "",             "W",
    (SUBR) chnclear_opcode_init, (SUBR) notinit_opcode_stub },
  { "chn_k",       S(CHN_OPCODE_K),    _CW, 1,      "",             "SiooooooooN",
    (SUBR) chn_k_opcode_init, NULL, NULL                  },
  { "chn_k",       S(CHN_OPCODE_K),    _CW, 1,      "",             "SSooooooooN",
    (SUBR) chn_k_opcode_init_S, NULL, NULL},
  { "chn_a",       S(CHN_OPCODE),      _CW, 1,      "",             "Si",
    (SUBR) chn_a_opcode_init, NULL, NULL                  },
  { "chn_S",       S(CHN_OPCODE),      _CW, 1,      "",             "Si",
    (SUBR) chn_S_opcode_init, NULL, NULL                  },
  { "chnexport",   0xFFFF,  0,    0,   NULL,   NULL, NULL, NULL },
  { "chnexport.i", S(CHNEXPORT_OPCODE),0, 1,      "i",            "Sioooo",
    (SUBR) chnexport_opcode_init, NULL, NULL              },
  { "chnexport.k", S(CHNEXPORT_OPCODE),0, 1,      "k",            "Sioooo",
    (SUBR) chnexport_opcode_init, NULL, NULL              },
  { "chnexport.a", S(CHNEXPORT_OPCODE),0, 1,      "a",            "Si",
    (SUBR) chnexport_opcode_init, NULL, NULL              },
  { "chnexport.S", S(CHNEXPORT_OPCODE),0, 1,      "S",            "Si",
    (SUBR) chnexport_opcode_init, NULL, NULL              },
  { "chnparams",   S(CHNPARAMS_OPCODE),_CR, 1,      "iiiiii",       "S",
    (SUBR) chnparams_opcode_init, NULL, NULL              },
  /*  these opcodes have never been fully implemented
      { "chnrecv",     S(CHNSEND),       _CR, 3,      "",             "So",
      (SUBR) chnrecv_opcode_init, (SUBR) notinit_opcode_stub, NULL },
      { "chnsend",     S(CHNSEND),0,          3,      "",             "So",
      (SUBR) chnsend_opcode_init, (SUBR) notinit_opcode_stub, NULL },
  */
  { "chano",       0xFFFD,  _CW, 0,      NULL, NULL, NULL, NULL },
  { "chano.k",     S(ASSIGN),_CW,           2,      "",             "kk",
    NULL, (SUBR) chano_opcode_perf_k, NULL                },
  { "chano.a",     S(ASSIGN),_CW,           2,      "",             "ak",
    NULL, (SUBR) chano_opcode_perf_a                },
  { "pvsout",     S(FCHAN),0,           3,      "",             "fk",
    (SUBR) pvsout_init, (SUBR) pvsout_perf, NULL                        },
  { "chani",      0xFFFF,  _CR,      0,   NULL, NULL, NULL, NULL },
  { "chani.k",     S(ASSIGN),_CR,           2,      "k",            "k",
    NULL, (SUBR) chani_opcode_perf_k, NULL                },
  { "chani.a",     S(ASSIGN),_CR,           2,      "a",            "k",
    NULL, (SUBR) chani_opcode_perf_a                },
  { "pvsin",     S(FCHAN),0,           3,      "f",            "kooopo",
    (SUBR)  pvsin_init, (SUBR) pvsin_perf, NULL                  },
  { "sense",       S(KSENSE),0,           2,      "kz",           "",
    NULL, (SUBR) sensekey_perf, NULL                      },
  { "sensekey",    S(KSENSE),0,           2,      "kz",           "",
    NULL, (SUBR) sensekey_perf, NULL                      },
  { "remove",      S(DELETEIN),0,         1,      "",             "T",
    (SUBR) delete_instr, NULL, NULL                       },
  { "##error",S(ERRFN),0, 1,          "i",     "i",   error_fn, NULL,    NULL    },
  #ifdef INC_EXPRANDI
  { "exprandi.i",S(PRANDI),0, 1,      "i",    "kxx",  iexprndi, NULL,    NULL    },
  { "exprandi.k",S(PRANDI),0, 3,      "k",    "kxx",  exprndiset, kexprndi, NULL },
  #endif
  #ifdef INC_EXPRANDI_A
  { "exprandi.a",S(PRANDI),0, 2,      "a",    "kxx",  exprndiset, aexprndi },
  #endif
  #ifdef CAUCHYRAND
  { "cauchyi.i", S(PRANDI),0, 1,      "i",    "kxx",  icauchyi, NULL,    NULL    }<
  { "cauchyi.k", S(PRANDI),0, 3,      "k",    "kxx",  cauchyiset, kcauchyi, NULL },
  #endif
  #ifdef INC_CAUCHYRAND_A
  { "cauchyi.a", S(PRANDI),0, 2,      "a",    "kxx",  cauchyiset, acauchyi },
  #endif
  #ifdef INC_GAUSSRAND
  { "gaussi.i", S(PRANDI),0, 1,      "i",    "kxx",  igaussi, NULL,    NULL    },
  { "gaussi.k", S(PRANDI),0, 3,      "k",    "kxx",  gaussiset, kgaussi, NULL },
  #endif
  #ifdef INC_GAUSSRAND_A
  { "gaussi.a", S(PRANDI),0, 2,      "a",    "kxx",  gaussiset, agaussi },
  #endif
  { "ftresizei", S(RESIZE), TB, 1, "i", "ii", (SUBR) resize_table, NULL, NULL },
  { "ftresize",  S(RESIZE), TB, 2, "k", "kk", NULL, (SUBR) resize_table, NULL },
  { "compileorc",  S(COMPILE), 0, 1, "i", "S",  (SUBR) compile_orc_i, NULL, NULL },
  { "compilecsd",  S(COMPILE), 0, 1, "i", "S",  (SUBR) compile_csd_i, NULL, NULL },
  { "compilestr",  S(COMPILE), 0, 1, "i", "S",  (SUBR) compile_str_i, NULL, NULL },
  { "evalstr",  S(COMPILE), 0, 1, "i", "S",  (SUBR) eval_str_i, NULL, NULL },
  { "evalstr",  S(COMPILE), 0, 2, "k", "Sk",  NULL, (SUBR) eval_str_k, NULL },
  { "readscore",  S(COMPILE), 0, 1, "i", "S",  (SUBR) read_score_i, NULL, NULL },
  { "return",  S(RETVAL), 0, 1, "", "i",  (SUBR) retval_i, NULL, NULL },
  /* ----------------------------------------------------------------------- */
  #ifdef INC_MONITOR
  { "monitor",  sizeof(MONITOR_OPCODE), IB, 3,  "mmmmmmmmmmmmmmmmmmmmxsmmmm", "",
    (SUBR) monitor_opcode_init, (SUBR) notinit_opcode_stub,  NULL },
  #endif
  #ifdef INC_OUTRG
  { "outrg", S(OUTSRANGE), IR,3, "", "ky",
    (SUBR)outRange_i, (SUBR)outRange},
  #endif
  #ifdef INC_HW_CHANNELS
  { "nchnls_hw", S(ASSIGN), 0,1, "ii", "",
    (SUBR)hw_channels},
  #endif
#ifdef INC_MIDIARP
   { "midiarp",   S(MIDIARP),0,  3,    "kk", "kO",
     midiarp_set, midiarp, NULL },
#endif
   {"lpcfilter", S(LPCFIL), 0, 3, "a", "akkiiio",
   (SUBR) lpfil_init, (SUBR) lpfil_perf},
   {"lpcfilter", S(LPCFIL2), 0, 3, "a", "aakkiio",
   (SUBR) lpfil2_init, (SUBR) lpfil2_perf},
   {"allpole", S(LPCFIL3), 0, 3, "a", "ak[]",
   (SUBR) lpfil3_init, (SUBR) lpfil3_perf},
   {"lpcanal", S(LPREDA), 0, 3, "k[]kkk", "kkiiio",
   (SUBR) lpred_alloc, (SUBR) lpred_run},
   {"lpcanal", S(LPREDA2), 0, 3, "k[]kkk", "akkiio",
   (SUBR) lpred_alloc2, (SUBR) lpred_run2},
   {"lpcanal", S(LPREDA), 0, 1, "i[]iii", "iiii",
   (SUBR) lpred_i, NULL},
   {"pvslpc", S(LPCPVS), 0, 3, "f", "aiiio",
    (SUBR) lpcpvs_init, (SUBR) lpcpvs},
   {"pvscfs", S(PVSCFS), 0, 3, "k[]kk", "fip",
    (SUBR) pvscoefs_init, (SUBR) pvscoefs},
   {"apoleparams", S(CF2P), 0, 3, "k[]", "k[]",
    (SUBR) coef2parm_init, (SUBR) coef2parm},
   {"resonbnk", S(RESONB), 0, 3, "a", "ak[]kkipoo",
   (SUBR) resonbnk_init, (SUBR) resonbnk},
   {"zzzmin7ffitch", S(EVAL), 0, 0, NULL},
   /*t erminate list */
  {  NULL, 0, 0, 0, NULL, NULL, NULL, NULL, NULL       }
};

