/*
    entry2.c:

    Copyright (C) 1991 Barry Vercoe, John ffitch

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

#include "csoundCore.h"         /*                      ENTRY.C         */
#include "insert.h"
#include "midiops.h"
#if defined(TCLTK)
#include "control.h"
#endif
#include "schedule.h"
#include "cwindow.h"
#include "vbap.h"
#include "aops.h"
#include "ugens1.h"
#include "vdelay.h"
#include "ugens2.h"
#include "ugens6.h"
#include "pstream.h"
#include "oscils.h"
#include "midifile.h"
#include "midiinterop.h"
#include "linevent.h"
#include "str_ops.h"

#define S       sizeof

#if defined(TCLTK)
int     cntrl_set(CSOUND *, void *);
int     control(CSOUND *, void *), ocontrol(CSOUND *, void *);
int     button_set(CSOUND *, void *), button(CSOUND *, void *);
int     check_set(CSOUND *, void *), check(CSOUND *, void *);
int     textflash(CSOUND *, void *);
#endif
int     instcount(CSOUND *, void *);
int     varicolset(CSOUND *, void *), varicol(CSOUND *, void *);
int     inh(CSOUND *, void *), ino(CSOUND *, void *);
int     in16(CSOUND *, void *), in32(CSOUND *, void *);
int     inall(CSOUND *, void *), inz(CSOUND *, void *);
int     outh(CSOUND *, void *), outo(CSOUND *, void *);
int     outx(CSOUND *, void *), outX(CSOUND *, void *);
int     outch(CSOUND *, void *), outall(CSOUND *, void *);
int     outz(CSOUND *, void *);
int     cpsxpch(CSOUND *, void *), cps2pch(CSOUND *, void *);
int     cpstmid(CSOUND *, void *);
int     cpstun(CSOUND *, void *), cpstun_i(CSOUND *, void *);
int     wgpsetin(CSOUND *, void *), wgpluck(CSOUND *, void *);
int     wgpset(CSOUND *, void *), wgpluck(CSOUND *, void *);
int     clarinset(CSOUND *, void *), clarin(CSOUND *, void *);
int     fluteset(CSOUND *, void *), flute(CSOUND *, void *);
int     bowedset(CSOUND *, void *), bowed(CSOUND *, void *);
int     brassset(CSOUND *, void *), brass(CSOUND *, void *);
int     adsrset(CSOUND *, void *);
int     klnseg(CSOUND *, void *), linseg(CSOUND *, void *);
int     madsrset(CSOUND *, void *);
int     klnsegr(CSOUND *, void *), linsegr(CSOUND *, void *);
int     xdsrset(CSOUND *, void *);
int     kxpseg(CSOUND *, void *), expseg(CSOUND *, void *);
int     mxdsrset(CSOUND *, void *);
int     kxpsegr(CSOUND *, void *), expsegr(CSOUND *, void *);
int     schedule(CSOUND *, void *), schedwatch(CSOUND *, void *);
int     ifschedule(CSOUND *, void *), kschedule(CSOUND *, void *);
int     triginset(CSOUND *, void *), ktriginstr(CSOUND *, void *);
int     trigseq_set(CSOUND *, void *), trigseq(CSOUND *, void *);
int     eventOpcode(CSOUND *, void *), eventOpcodeI(CSOUND *, void *);
int     lfoset(CSOUND *, void *);
int     lfok(CSOUND *, void *), lfoa(CSOUND *, void *);
int     mute_inst(CSOUND *, void *);
int     vbap_FOUR_init(CSOUND *, void *), vbap_FOUR(CSOUND *, void *);
int     vbap_EIGHT_init(CSOUND *, void *), vbap_EIGHT(CSOUND *, void *);
int     vbap_SIXTEEN_init(CSOUND *, void *), vbap_SIXTEEN(CSOUND *, void *);
int     vbap_zak_init(CSOUND *, void *), vbap_zak(CSOUND *, void *);
int     vbap_ls_init(CSOUND *, void *);
int     vbap_FOUR_moving_init(CSOUND *, void *);
int     vbap_FOUR_moving(CSOUND *, void *);
int     vbap_EIGHT_moving_init(CSOUND *, void *);
int     vbap_EIGHT_moving(CSOUND *, void *);
int     vbap_SIXTEEN_moving_init(CSOUND *, void *);
int     vbap_SIXTEEN_moving(CSOUND *, void *);
int     vbap_zak_moving_init(CSOUND *, void *);
int     vbap_zak_moving(CSOUND *, void *);
#ifdef JPFF
int     Foscset(CSOUND *, void *);
int     Fosckk(CSOUND *, void *), Foscka(CSOUND *, void *);
int     Foscak(CSOUND *, void *), Foscaa(CSOUND *, void *);
#endif
int     lpf18set(CSOUND *, void *), lpf18db(CSOUND *, void *);
int     pfun(CSOUND *, void *);
int     pvsanalset(CSOUND *, void *), pvsanal(CSOUND *, void *);
int     pvsynthset(CSOUND *, void *), pvsynth(CSOUND *, void *);
int     pvadsynset(CSOUND *, void *), pvadsyn(CSOUND *, void *);
int     pvscrosset(CSOUND *, void *), pvscross(CSOUND *, void *);
int     pvsfreadset(CSOUND *, void *), pvsfread(CSOUND *, void *);
int     pvsmaskaset(CSOUND *, void *), pvsmaska(CSOUND *, void *);
int     pvsftwset(CSOUND *, void *), pvsftw(CSOUND *, void *);
int     pvsftrset(CSOUND *, void *), pvsftr(CSOUND *, void *);
int     pvsinfo(CSOUND *, void *);
int     gettempo(CSOUND *, void *), fassign(CSOUND *, void *);
int     loopseg_set(CSOUND *, void *);
int     loopseg(CSOUND *, void *), lpshold(CSOUND *, void *);
int     lineto_set(CSOUND *, void *), lineto(CSOUND *, void *);
int     tlineto_set(CSOUND *, void *),tlineto(CSOUND *, void *);
int     vibrato_set(CSOUND *, void *), vibrato(CSOUND *, void *);
int     vibr_set(CSOUND *, void *), vibr(CSOUND *, void *);
int     userrnd_set(CSOUND *, void *);
int     iDiscreteUserRand(CSOUND *, void *);
int     kDiscreteUserRand(CSOUND *, void *);
int     Cuserrnd_set(CSOUND *, void *);
int     aDiscreteUserRand(CSOUND *, void *);
int     iContinuousUserRand(CSOUND *, void *);
int     kContinuousUserRand(CSOUND *, void *);
int     aContinuousUserRand(CSOUND *, void *);
int     ikRangeRand(CSOUND *, void *), aRangeRand(CSOUND *, void *);
int     randomi_set(CSOUND *, void *);
int     krandomi(CSOUND *, void *), randomi(CSOUND *, void *);
int     randomh_set(CSOUND *, void *);
int     krandomh(CSOUND *, void *), randomh(CSOUND *, void *);
int     random3_set(CSOUND *, void *);
int     random3(CSOUND *, void *), random3a(CSOUND *, void *);
int     ilogbasetwo(CSOUND *, void *), db(CSOUND *, void *);
int     powoftwoa(CSOUND *, void *), dba(CSOUND *, void *);
int     powoftwo(CSOUND *, void *);
int     semitone(CSOUND *, void *), asemitone(CSOUND *, void *);
int     cent(CSOUND *, void *), acent(CSOUND *, void *);
int     midichn(CSOUND *, void *), pgmassign(CSOUND *, void *);
int     midiin(CSOUND *, void *);
int     midinoteoff(CSOUND *, void *), midinoteonkey(CSOUND *, void *);
int     midinoteoncps(CSOUND *, void *), midinoteonoct(CSOUND *, void *);
int     midinoteonpch(CSOUND *, void *), midipolyaftertouch(CSOUND *, void *);
int     midicontrolchange(CSOUND *, void *);
int     midiprogramchange(CSOUND *, void *);
int     midichannelaftertouch(CSOUND *, void *);
int     midipitchbend(CSOUND *, void *);
int     mididefault(CSOUND *, void *);
int     invalset(CSOUND *, void *), kinval(CSOUND *, void *);
int     outvalset(CSOUND *, void *), koutval(CSOUND *, void *);
int     subinstrset(CSOUND *, void *), subinstr(CSOUND *, void *);
int     useropcdset(CSOUND *, void *), useropcd(CSOUND *, void *);
int     setksmpsset(CSOUND *, void *);
int     xinset(CSOUND *, void *), xoutset(CSOUND *, void *);
int     ingoto(CSOUND *, void *), kngoto(CSOUND *, void *);
int     iingoto(CSOUND *, void *), kingoto(CSOUND *, void *);
int     nstrnumset(CSOUND *, void *), turnoff2(CSOUND *, void *);
int     loop_l_i(CSOUND *, void *), loop_le_i(CSOUND *, void *);
int     loop_g_i(CSOUND *, void *), loop_ge_i(CSOUND *, void *);
int     loop_l_p(CSOUND *, void *), loop_le_p(CSOUND *, void *);
int     loop_g_p(CSOUND *, void *), loop_ge_p(CSOUND *, void *);

/* thread vals, where isub=1, ksub=2, asub=4:
                0 =     1  OR   2  (B out only)
                1 =     1
                2 =             2
                3 =     1  AND  2
                4 =                     4
                5 =     1  AND          4
                7 =     1  AND (2  OR   4)                      */

/* inarg types include the following:

                i       irate scalar
                k       krate scalar
                a       arate vector
                f       fregency variable
                w       spectral variable
                x       krate scalar or arate vector
                S       String
                T       String or i-rate
                U       String or i/k-rate
                B       Boolean
                l       Label
     and codes
                m       begins an indef list of iargs (any count)
                M       begins an indef list of args (any count/rate i,k,a)
                N       begins an indef list of args (any count/rate i,k,a,S)
                n       begins an indef list of iargs (nargs odd)
                o       optional, defaulting to 0
                p          "            "       1
                q          "            "       10
                v          "            "       .5
                j          "            "       -1
                h          "            "       127
                y       begins indef list of aargs (any count)
                z       begins indef list of kargs (any count)
                Z       begins alternating kakaka...list (any count)
   outarg types include:
                m       multiple out aargs
                z               multiple out kargs
                X       multiple args (any rate)        IV - Sep 1 2002
   (these types must agree with rdorch.c)                       */

/* If dsblksize is 0xffff then translate on output arg */
/*                 0xfffe then translate two (oscil) */
/*                 0xfffd then translate on first input arg (peak) */
/*                 0xfffc then translate two (divz) */
/*                 0xfffb then translate on first input arg (loop_l) */

OENTRY opcodlst_2[] = {
/* opcode   dspace      thread  outarg  inargs  isub    ksub    asub    */
{ "inh",    S(INQ),     4,      "aaaaaa","",    NULL,   NULL,   inh     },
{ "ino",    S(INQ),     4,      "aaaaaaaa","",  NULL,   NULL,   ino     },
{ "inx",    S(INALL),   4,      "aaaaaaaaaaaaaaaa","",  NULL,   NULL,   in16 },
{ "in32",   S(INALL),   4,      "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                                        "",     NULL,   NULL,   in32 },
{ "inch",   S(INALL),   4,      "a",    "k",    NULL,   NULL,   inall   },
  /* Note that there is code in rdorch.c that assumes that opcodes starting
     with the charcters out followed by a s, q, h, o or x are in this group
     ***BEWARE***
   */
{ "outh",   S(OUTH),    4,      "",     "aaaaaa",NULL,  NULL,   outh    },
{ "outo",   S(OUTO),    4,      "",     "aaaaaaaa",NULL,NULL,   outo    },
{ "outx",   S(OUTX),    4,      "",     "aaaaaaaaaaaaaaaa",NULL,NULL, outx },
{ "out32",  S(OUTX),    4,      "",     "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
                                                NULL,   NULL,   outX    },
{ "outch",  S(OUTCH),   4,      "",     "Z",    NULL,   NULL,   outch   },
{ "outc",   S(OUTX),    4,      "",     "y",    NULL,   NULL,   outall  },
{ "cpsxpch", S(XENH),   1,      "i",    "iiii", cpsxpch, NULL,  NULL    },
{ "cps2pch", S(XENH),   1,      "i",    "ii",   cps2pch, NULL,  NULL    },
{ "cpstun", S(CPSTUN),  2,      "k",    "kkk",   NULL,   cpstun         },
{ "cpstuni",S(CPSTUNI), 1,      "i",    "ii",   cpstun_i,               },
{ "cpstmid", S(CPSTABLE), 1, "i", "i",    (SUBR)cpstmid                    },
{ "active", 0xffff                                                         },
#if defined(TCLTK)
{ "control", S(CNTRL),   3,     "k",    "k",    cntrl_set, control, NULL   },
{ "setctrl", S(SCNTRL),  1,     "",     "iTi",  ocontrol, NULL, NULL   },
{ "button", S(CNTRL),    3,     "k",    "k",    button_set, button, NULL   },
{ "checkbox", S(CNTRL),  3,     "k",    "k",    check_set, check,   NULL   },
{ "flashtxt", S(TXTWIN), 1,     "",     "iT",   textflash, NULL,    NULL   },
#endif
{ "adsr", S(LINSEG),     7,     "s",    "iiiio",adsrset,klnseg, linseg     },
{ "madsr", S(LINSEG),    7,     "s",    "iiiioj", madsrset,klnsegr, linsegr },
{ "xadsr", S(EXXPSEG),   7,     "s",    "iiiio", xdsrset, kxpseg, expseg    },
{ "mxadsr", S(EXPSEG),   7,     "s",    "iiiioj", mxdsrset, kxpsegr, expsegr },
{ "schedule", S(SCHED),  1,     "",     "Tiim", schedule, schedwatch, NULL },
{ "schedwhen", S(WSCHED),3,     "",     "kkkkm",ifschedule, kschedule, NULL },
{ "schedkwhen", S(TRIGINSTR), 3,"",     "kkkUkz",triginset, ktriginstr, NULL },
{ "schedkwhennamed", S(TRIGINSTR), 3,"", "kkkUkz",triginset, ktriginstr, NULL },
{ "trigseq", S(TRIGSEQ), 3,     "",     "kkkkkz", trigseq_set, trigseq, NULL },
{ "event", S(LINEVENT),  2,     "",     "SUz",  NULL, eventOpcode, NULL   },
{ "event_i", S(LINEVENT),1,     "",     "STm",  eventOpcodeI, NULL, NULL  },
{ "lfo", S(LFO),         7,     "s",    "kko",  lfoset,   lfok,   lfoa    },
{ "vbap4",  S(VBAP_FOUR), 5, "aaaa","aioo", vbap_FOUR_init, NULL, vbap_FOUR },
{ "vbap8",  S(VBAP_EIGHT), 5, "aaaaaaaa","aioo", vbap_EIGHT_init, NULL, vbap_EIGHT },
{ "vbap16", S(VBAP_SIXTEEN), 5, "aaaaaaaaaaaaaaaa","aioo", vbap_SIXTEEN_init, NULL, vbap_SIXTEEN },
{ "vbapz",  S(VBAP_ZAK), 5,     "",    "iiaioo", vbap_zak_init, NULL, vbap_zak },
{ "vbaplsinit",  S(VBAP_LS_INIT), 1, "","iioooooooooooooooooooooooooooooooo", vbap_ls_init},
{ "vbap4move",  S(VBAP_FOUR_MOVING), 5, "aaaa","aiiim", vbap_FOUR_moving_init, NULL, vbap_FOUR_moving },
{ "vbap8move",  S(VBAP_EIGHT_MOVING), 5, "aaaaaaaa","aiiim", vbap_EIGHT_moving_init, NULL, vbap_EIGHT_moving },
{ "vbap16move",  S(VBAP_SIXTEEN_MOVING), 5, "aaaaaaaaaaaaaaaa","aiiim", vbap_SIXTEEN_moving_init, NULL, vbap_SIXTEEN_moving },
{ "vbapzmove",  S(VBAP_ZAK_MOVING), 5, "","iiaiiim", vbap_zak_moving_init, NULL, vbap_zak_moving },
{ "oscils",   S(OSCILS), 5,     "a", "iiio",     (SUBR)oscils_set, NULL, (SUBR)oscils       },
{ "lphasor",  S(LPHASOR),5,     "a", "xooooooo" ,(SUBR)lphasor_set, NULL, (SUBR)lphasor     },
{ "tablexkt", S(TABLEXKT),5,    "a", "xkkiooo",  (SUBR)tablexkt_set, NULL, (SUBR)tablexkt   },
{ "reverb2",  S(NREV2),  5,     "a",    "akkoojoj", (SUBR)reverbx_set,NULL,(SUBR)reverbx    },
{ "nreverb",  S(NREV2),  5,     "a",    "akkoojoj", (SUBR)reverbx_set,NULL,(SUBR) reverbx    },
{ "=.f",      S(FASSIGN), 2,    "f",   "f",      NULL, fassign, NULL    },
{ "init.f",   S(FASSIGN), 2,    "f",   "f",      fassign, NULL, NULL    },
{ "pvsanal",  S(PVSANAL), 5,    "f",   "aiiiioo",  pvsanalset, NULL, pvsanal  },
{ "pvsynth",  S(PVSYNTH), 5,    "a",   "fo",     pvsynthset, NULL, pvsynth  },
{ "pvsadsyn", S(PVADS),   7,    "a",   "fikopo", pvadsynset, pvadsyn, pvadsyn },
{ "pvscross", S(PVSCROSS),3,    "f",   "ffkk",   pvscrosset, pvscross, NULL },
{ "pvsfread", S(PVSFREAD),3,    "f",   "kTo",    pvsfreadset, pvsfread, NULL  },
{ "pvsmaska", S(PVSMASKA),3,    "f",   "fik",    pvsmaskaset, pvsmaska, NULL  },
{ "pvsftw",   S(PVSFTW),  3,    "k",   "fio",    pvsftwset, pvsftw, NULL  },
{ "pvsftr",   S(PVSFTR),  3,    "",    "fio",    pvsftrset, pvsftr, NULL  },
{ "pvsinfo",  S(PVSINFO), 1,    "iiii","f",      pvsinfo, NULL, NULL    },
{ "octave",   0xffff                                                    },
{ "semitone", 0xffff                                                    },
{ "cent",     0xffff                                                    },
{ "octave.i", S(EVAL),    1,    "i",    "i",     powoftwo               },
{ "octave.k", S(EVAL),    2,    "k",    "k",     NULL,  powoftwo        },
{ "octave.a", S(EVAL),    4,    "a",    "a",     NULL, NULL, powoftwoa  },
{ "semitone.i",S(EVAL),   1,    "i",    "i",     semitone               },
{ "semitone.k",S(EVAL),   2,    "k",    "k",     NULL,  semitone        },
{ "semitone.a",S(EVAL),   4,    "a",    "a",     NULL, NULL, asemitone  },
{ "cent.i",   S(EVAL),    1,    "i",    "i",     cent                   },
{ "cent.k",   S(EVAL),    2,    "k",    "k",     NULL,  cent            },
{ "cent.a",   S(EVAL),    4,    "a",    "a",     NULL, NULL, acent      },
{ "db",       0xffff                                                    },
{ "db.i",     S(EVAL),    1,    "i",    "i",     db                     },
{ "db.k",     S(EVAL),    2,    "k",    "k",     NULL, db               },
{ "db.a",     S(EVAL),    4,    "a",    "a",     NULL, NULL, dba        },
{ "midichn",  S(MIDICHN), 1,    "i",    "",      midichn, NULL, NULL    },
{ "pgmassign",S(PGMASSIGN), 1,  "",     "iTo",   pgmassign, NULL, NULL  },
{ "midiin",   S(MIDIIN),  2,    "kkkk", "",      NULL, midiin, NULL     },
{ "miditempo", S(MIDITEMPO), 3, "k",    "",
                (SUBR) midiTempoOpcode, (SUBR) midiTempoOpcode, NULL    },
{ "midinoteoff", S(MIDINOTEON),3,"",    "xx",   midinoteoff, midinoteoff, },
{ "midinoteonkey", S(MIDINOTEON),3, "", "xx",   midinoteonkey, midinoteonkey },
{ "midinoteoncps", S(MIDINOTEON), 3, "", "xx",  midinoteoncps,midinoteoncps },
{ "midinoteonoct", S(MIDINOTEON), 3, "", "xx",  midinoteonoct,midinoteonoct },
{ "midinoteonpch", S(MIDINOTEON), 3, "", "xx",  midinoteonpch, midinoteonpch },
{ "midipolyaftertouch", S(MIDIPOLYAFTERTOUCH),
                   3,   "", "xxoh", midipolyaftertouch, midipolyaftertouch},
{ "midicontrolchange", S(MIDICONTROLCHANGE),
                   3, "", "xxoh",midicontrolchange, midicontrolchange    },
{ "midiprogramchange", S(MIDIPROGRAMCHANGE),
                   3, "", "x", midiprogramchange, midiprogramchange      },
{ "midichannelaftertouch", S(MIDICHANNELAFTERTOUCH),
                   3, "", "xoh",midichannelaftertouch, midichannelaftertouch },
{ "midipitchbend", S(MIDIPITCHBEND),3, "", "xoh", midipitchbend, midipitchbend },
{ "mididefault", S(MIDIDEFAULT), 3, "", "xx",   mididefault, mididefault },
{ "invalue", S(INVAL),     3,   "k",    "T", invalset, kinval, NULL      },
{ "outvalue", S(OUTVAL), 3,     "",     "Tk", outvalset, koutval, NULL   },
/* IV - Oct 20 2002 */
{ "subinstr", S(SUBINST), 5, "mmmmmmmm", "Tm",  subinstrset, NULL, subinstr },
{ "subinstrinit", S(SUBINST), 1, "",    "Tm",   subinstrset, NULL, NULL  },
{ "nstrnum", S(NSTRNUM), 1,     "i",    "T",    nstrnumset, NULL, NULL   },
{ "turnoff2",S(TURNOFF2),2,     "",     "kkk",  NULL, turnoff2, NULL     },
{ "cngoto", S(CGOTO),   3,      "",     "Bl",   ingoto, kngoto, NULL     },
{ "cogoto", S(CGOTO),   3,      "",     "Bl",   iingoto, kingoto, NULL   },
/* IV - Sep 8 2002 - added entries for user defined opcodes, xin, xout */
/* and setksmps */
{ ".userOpcode", S(UOPCODE), 7, "", "", useropcdset, useropcd, useropcd },
/* IV - Sep 10 2002: removed perf time routines of xin and xout */
{ "xin", S(XIN),   1, "XXXXXXXXXXXXXXXXXXXXXXXX", "", xinset, NULL, NULL },
{ "xout", S(XOUT),    1,        "",     "M",    xoutset, NULL, NULL      },
{ "setksmps", S(SETKSMPS), 1,   "",     "i",    setksmpsset, NULL, NULL  },
{ "tempoval", S(GTEMPO), 2,  "k", "",      NULL, (SUBR)gettempo, NULL    },
{ "downsamp",S(DOWNSAMP),3, "k", "ao",   (SUBR)downset,(SUBR)downsamp        },
{ "upsamp", S(UPSAMP),  4,  "a", "k",    NULL,   NULL,   (SUBR)upsamp        },
/* IV - Sep 5 2002 */
{ "interp", S(INTERP),  5,  "a", "koo",  (SUBR)interpset,NULL, (SUBR)interp  },
{ "a.k",    S(INTERP),  5,  "a", "k",    (SUBR)a_k_set,NULL,   (SUBR)interp  },
{ "integ", S(INDIFF), 7, "s", "xo", (SUBR)indfset,(SUBR)kntegrate,(SUBR)integrate},
{ "diff",   S(INDIFF),  7,  "s", "xo",   (SUBR)indfset,(SUBR)kdiff, (SUBR)diff },
{ "samphold",S(SAMPHOLD),7, "s", "xxoo", (SUBR)samphset,(SUBR)ksmphold,(SUBR)samphold},
{ "delay",  S(DELAY),   5,  "a", "aio",  (SUBR)delset, NULL,   (SUBR)delay   },
{ "delayr", S(DELAYR),  5,  "XX","io",   (SUBR)delrset,NULL,   (SUBR)delayr  },
{ "delayw", S(DELAYW),  5,  "",  "a",    (SUBR)delwset,NULL,   (SUBR)delayw  },
{ "delay1", S(DELAY1),  5,  "a", "ao",   (SUBR)del1set,NULL,   (SUBR)delay1  },
{ "deltap", S(DELTAP),  5,  "a", "ko",   (SUBR)tapset, NULL,   (SUBR)deltap  },
{ "deltapi",S(DELTAP),  5,  "a", "xo",   (SUBR)tapset, NULL,   (SUBR)deltapi },
{ "deltapn",S(DELTAP),  5,  "a", "xo",   (SUBR)tapset, NULL,   (SUBR)deltapn },
{ "deltap3",S(DELTAP),  5,  "a", "xo",   (SUBR)tapset, NULL,   (SUBR)deltap3 },
{ "reverb", S(REVERB),  5, "a",  "ako",  (SUBR)rvbset, NULL,   (SUBR)reverb  },
{ "vdelay",   S(VDEL),  5,      "a",    "axio", (SUBR)vdelset, NULL,  (SUBR)vdelay  },
{ "vdelay3",  S(VDEL),  5,      "a",    "axio", (SUBR)vdelset, NULL,  (SUBR)vdelay3 },
{ "vdelayxwq",S(VDELXQ),5,      "aaaa", "aaaaaiio", (SUBR)vdelxqset, NULL, (SUBR)vdelayxwq},
{ "vdelayxws",S(VDELXS),5,      "aa",   "aaaiio", (SUBR)vdelxsset, NULL, (SUBR)vdelayxws},
{ "vdelayxw", S(VDELX), 5,      "a",    "aaiio", (SUBR)vdelxset, NULL, (SUBR)vdelayxw},
{ "vdelayxq", S(VDELXQ),5,      "aaaa", "aaaaaiio", (SUBR)vdelxqset, NULL, (SUBR)vdelayxq},
{ "vdelayxs", S(VDELXS),5,      "aa",   "aaaiio", (SUBR)vdelxsset, NULL, (SUBR)vdelayxs},
{ "vdelayx",  S(VDELX), 5,      "a",    "aaiio", (SUBR)vdelxset, NULL, (SUBR)vdelayx},
{ "deltapx",  S(DELTAPX),5,     "a",    "aio",  (SUBR)tapxset, NULL,  (SUBR)deltapx },
{ "deltapxw", S(DELTAPX),5,     "",     "aaio", (SUBR)tapxset, NULL, (SUBR)deltapxw },
{ "multitap", S(MDEL),  5,      "a",    "am",   (SUBR)multitap_set,NULL,(SUBR)multitap_play},
{ "comb",   S(COMB),    5,      "a",    "akioo", (SUBR)cmbset,NULL,   (SUBR)comb    },
{ "alpass", S(COMB),    5,      "a",    "akioo", (SUBR)cmbset,NULL,   (SUBR)alpass  },
 {  "strset",   S(STRSET_OP),   1,  "",     "iS",
    (SUBR) strset_init, (SUBR) NULL, (SUBR) NULL                        },
 {  "strget",   S(STRGET_OP),   1,  "S",    "i",
    (SUBR) strget_init, (SUBR) NULL, (SUBR) NULL                        },
 {  "strcpy",   S(STRCPY_OP),   1,  "S",    "S",
    (SUBR) strcpy_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "strcpyk",  S(STRCPY_OP),   3,  "S",    "S",
    (SUBR) strcpy_opcode_init, (SUBR) strcpy_opcode_perf, (SUBR) NULL   },
 {  "strcat",   S(STRCAT_OP),   1,  "S",    "SS",
    (SUBR) strcat_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "strcatk",  S(STRCAT_OP),   3,  "S",    "SS",
    (SUBR) strcat_opcode_init, (SUBR) strcat_opcode_perf, (SUBR) NULL   },
 {  "strcmp",   S(STRCAT_OP),   1,  "i",    "SS",
    (SUBR) strcpy_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "strcmpk",  S(STRCAT_OP),   3,  "k",    "SS",
    (SUBR) strcmp_opcode, (SUBR) strcmp_opcode, (SUBR) NULL             },
 {  "sprintf",  S(SPRINTF_OP),  1,  "S",    "SN",
    (SUBR) sprintf_opcode_init, (SUBR) NULL, (SUBR) NULL                },
 {  "sprintfk", S(SPRINTF_OP),  3,  "S",    "SN",
    (SUBR) sprintf_opcode_init, (SUBR) sprintf_opcode_perf, (SUBR) NULL },
 {  "printf_i", S(PRINTF_OP),   1,  "",     "SiN",
    (SUBR) printf_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "printf",   S(PRINTF_OP),   3,  "",     "SkN",
    (SUBR) printf_opcode_set, (SUBR) printf_opcode_perf, (SUBR) NULL    },
 {  "puts",     S(PUTS_OP),     3,  "",     "Sko",
    (SUBR) puts_opcode_init, (SUBR) puts_opcode_perf, (SUBR) NULL       },
 {  "strtod",   S(STRSET_OP),   1,  "i",    "T",
    (SUBR) strtod_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "strtodk",  S(STRSET_OP),   3,  "k",    "U",
    (SUBR) strtod_opcode_init, (SUBR) strtod_opcode_perf, (SUBR) NULL   },
 {  "strtol",   S(STRSET_OP),   1,  "i",    "T",
    (SUBR) strtol_opcode_init, (SUBR) NULL, (SUBR) NULL                 },
 {  "strtolk",  S(STRSET_OP),   3,  "k",    "U",
    (SUBR) strtol_opcode_init, (SUBR) strtol_opcode_perf, (SUBR) NULL   },
 { "loop_l",    0xfffb                                                  },
 { "loop_le",   0xfffb                                                  },
 { "loop_g",    0xfffb                                                  },
 { "loop_ge",   0xfffb                                                  },
 { "loop_l.i",  S(LOOP_OPS),  1,  "", "iiil", (SUBR) loop_l_i, NULL, NULL   },
 { "loop_le.i", S(LOOP_OPS),  1,  "", "iiil", (SUBR) loop_le_i, NULL, NULL  },
 { "loop_g.i",  S(LOOP_OPS),  1,  "", "iiil", (SUBR) loop_g_i, NULL, NULL   },
 { "loop_ge.i", S(LOOP_OPS),  1,  "", "iiil", (SUBR) loop_ge_i, NULL, NULL  },
 { "loop_l.k",  S(LOOP_OPS),  1,  "", "kkkl", NULL, (SUBR) loop_l_p, NULL   },
 { "loop_le.k", S(LOOP_OPS),  1,  "", "kkkl", NULL, (SUBR) loop_le_p, NULL  },
 { "loop_g.k",  S(LOOP_OPS),  1,  "", "kkkl", NULL, (SUBR) loop_g_p, NULL   },
 { "loop_ge.k", S(LOOP_OPS),  1,  "", "kkkl", NULL, (SUBR) loop_ge_p, NULL  },
 /* terminate list */
 {  NULL, 0, 0, NULL, NULL, (SUBR) NULL, (SUBR) NULL, (SUBR) NULL       }
};

