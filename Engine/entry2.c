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
#include "spectra.h"
#include "vbap.h"
#include "aops.h"
#include "ugens1.h"
#include "fhtfun.h"
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
int     cntrl_set(ENVIRON *, void *);
int     control(ENVIRON *, void *), ocontrol(ENVIRON *, void *);
int     button_set(ENVIRON *, void *), button(ENVIRON *, void *);
int     check_set(ENVIRON *, void *), check(ENVIRON *, void *);
int     textflash(ENVIRON *, void *);
#endif
int     instcount(ENVIRON *, void *);
int     varicolset(ENVIRON *, void *), varicol(ENVIRON *, void *);
int     inh(ENVIRON *, void *), ino(ENVIRON *, void *);
int     in16(ENVIRON *, void *), in32(ENVIRON *, void *);
int     inall(ENVIRON *, void *), inz(ENVIRON *, void *);
int     outh(ENVIRON *, void *), outo(ENVIRON *, void *);
int     outx(ENVIRON *, void *), outX(ENVIRON *, void *);
int     outch(ENVIRON *, void *), outall(ENVIRON *, void *);
int     outz(ENVIRON *, void *);
int     cpsxpch(ENVIRON *, void *), cps2pch(ENVIRON *, void *);
int     cpstmid(ENVIRON *, void *);
int     cpstun(ENVIRON *, void *), cpstun_i(ENVIRON *, void *);
int     wgpsetin(ENVIRON *, void *), wgpluck(ENVIRON *, void *);
int     wgpset(ENVIRON *, void *), wgpluck(ENVIRON *, void *);
int     clarinset(ENVIRON *, void *), clarin(ENVIRON *, void *);
int     fluteset(ENVIRON *, void *), flute(ENVIRON *, void *);
int     bowedset(ENVIRON *, void *), bowed(ENVIRON *, void *);
int     brassset(ENVIRON *, void *), brass(ENVIRON *, void *);
int     adsrset(ENVIRON *, void *);
int     klnseg(ENVIRON *, void *), linseg(ENVIRON *, void *);
int     madsrset(ENVIRON *, void *);
int     klnsegr(ENVIRON *, void *), linsegr(ENVIRON *, void *);
int     xdsrset(ENVIRON *, void *);
int     kxpseg(ENVIRON *, void *), expseg(ENVIRON *, void *);
int     mxdsrset(ENVIRON *, void *);
int     kxpsegr(ENVIRON *, void *), expsegr(ENVIRON *, void *);
int     schedule(ENVIRON *, void *), schedwatch(ENVIRON *, void *);
int     ifschedule(ENVIRON *, void *), kschedule(ENVIRON *, void *);
int     triginset(ENVIRON *, void *), ktriginstr(ENVIRON *, void *);
int     trigseq_set(ENVIRON *, void *), trigseq(ENVIRON *, void *);
int     eventOpcode(ENVIRON *, void *), eventOpcodeI(ENVIRON *, void *);
int     lfoset(ENVIRON *, void *);
int     lfok(ENVIRON *, void *), lfoa(ENVIRON *, void *);
int     mute_inst(ENVIRON *, void *);
int     vbap_FOUR_init(ENVIRON *, void *), vbap_FOUR(ENVIRON *, void *);
int     vbap_EIGHT_init(ENVIRON *, void *), vbap_EIGHT(ENVIRON *, void *);
int     vbap_SIXTEEN_init(ENVIRON *, void *), vbap_SIXTEEN(ENVIRON *, void *);
int     vbap_zak_init(ENVIRON *, void *), vbap_zak(ENVIRON *, void *);
int     vbap_ls_init(ENVIRON *, void *);
int     vbap_FOUR_moving_init(ENVIRON *, void *);
int     vbap_FOUR_moving(ENVIRON *, void *);
int     vbap_EIGHT_moving_init(ENVIRON *, void *);
int     vbap_EIGHT_moving(ENVIRON *, void *);
int     vbap_SIXTEEN_moving_init(ENVIRON *, void *);
int     vbap_SIXTEEN_moving(ENVIRON *, void *);
int     vbap_zak_moving_init(ENVIRON *, void *);
int     vbap_zak_moving(ENVIRON *, void *);
#ifdef JPFF
int     Foscset(ENVIRON *, void *);
int     Fosckk(ENVIRON *, void *), Foscka(ENVIRON *, void *);
int     Foscak(ENVIRON *, void *), Foscaa(ENVIRON *, void *);
#endif
int     lpf18set(ENVIRON *, void *), lpf18db(ENVIRON *, void *);
int     pfun(ENVIRON *, void *);
int     pvsanalset(ENVIRON *, void *), pvsanal(ENVIRON *, void *);
int     pvsynthset(ENVIRON *, void *), pvsynth(ENVIRON *, void *);
int     pvadsynset(ENVIRON *, void *), pvadsyn(ENVIRON *, void *);
int     pvscrosset(ENVIRON *, void *), pvscross(ENVIRON *, void *);
int     pvsfreadset(ENVIRON *, void *), pvsfread(ENVIRON *, void *);
int     pvsmaskaset(ENVIRON *, void *), pvsmaska(ENVIRON *, void *);
int     pvsftwset(ENVIRON *, void *), pvsftw(ENVIRON *, void *);
int     pvsftrset(ENVIRON *, void *), pvsftr(ENVIRON *, void *);
int     pvsinfo(ENVIRON *, void *);
int     gettempo(ENVIRON *, void *), fassign(ENVIRON *, void *);
int     loopseg_set(ENVIRON *, void *);
int     loopseg(ENVIRON *, void *), lpshold(ENVIRON *, void *);
int     lineto_set(ENVIRON *, void *), lineto(ENVIRON *, void *);
int     tlineto_set(ENVIRON *, void *),tlineto(ENVIRON *, void *);
int     vibrato_set(ENVIRON *, void *), vibrato(ENVIRON *, void *);
int     vibr_set(ENVIRON *, void *), vibr(ENVIRON *, void *);
int     oscbnkset(ENVIRON *, void *), oscbnk(ENVIRON *, void *);
int     userrnd_set(ENVIRON *, void *);
int     oscktset(ENVIRON *, void *), kosclikt(ENVIRON *, void *);
int     osckkikt(ENVIRON *, void *), osckaikt(ENVIRON *, void *);
int     oscakikt(ENVIRON *, void *), oscaaikt(ENVIRON *, void *);
int     oscktpset(ENVIRON *, void *), oscktp(ENVIRON *, void *);
int     oscktsset(ENVIRON *, void *), osckts(ENVIRON *, void *);
int     iDiscreteUserRand(ENVIRON *, void *);
int     kDiscreteUserRand(ENVIRON *, void *);
int     Cuserrnd_set(ENVIRON *, void *);
int     aDiscreteUserRand(ENVIRON *, void *);
int     iContinuousUserRand(ENVIRON *, void *);
int     kContinuousUserRand(ENVIRON *, void *);
int     aContinuousUserRand(ENVIRON *, void *);
int     ikRangeRand(ENVIRON *, void *), aRangeRand(ENVIRON *, void *);
int     randomi_set(ENVIRON *, void *);
int     krandomi(ENVIRON *, void *), randomi(ENVIRON *, void *);
int     randomh_set(ENVIRON *, void *);
int     krandomh(ENVIRON *, void *), randomh(ENVIRON *, void *);
int     random3_set(ENVIRON *, void *);
int     random3(ENVIRON *, void *), random3a(ENVIRON *, void *);
int     ilogbasetwo(ENVIRON *, void *), db(ENVIRON *, void *);
int     powoftwoa(ENVIRON *, void *), dba(ENVIRON *, void *);
int     powoftwo(ENVIRON *, void *);
int     semitone(ENVIRON *, void *), asemitone(ENVIRON *, void *);
int     cent(ENVIRON *, void *), acent(ENVIRON *, void *);
int     and_kk(ENVIRON *, void *), and_ka(ENVIRON *, void *);
int     and_ak(ENVIRON *, void *), and_aa(ENVIRON *, void *);
int     or_kk(ENVIRON *, void *), or_ka(ENVIRON *, void *);
int     or_ak(ENVIRON *, void *), or_aa(ENVIRON *, void *);
int     xor_kk(ENVIRON *, void *), xor_ka(ENVIRON *, void *);
int     xor_ak(ENVIRON *, void *), xor_aa(ENVIRON *, void *);
int     not_k(ENVIRON *, void *), not_a(ENVIRON *, void *);
int     midichn(ENVIRON *, void *), pgmassign(ENVIRON *, void *);
int     midiin(ENVIRON *, void *);
int     midinoteoff(ENVIRON *, void *), midinoteonkey(ENVIRON *, void *);
int     midinoteoncps(ENVIRON *, void *), midinoteonoct(ENVIRON *, void *);
int     midinoteonpch(ENVIRON *, void *), midipolyaftertouch(ENVIRON *, void *);
int     midicontrolchange(ENVIRON *, void *);
int     midiprogramchange(ENVIRON *, void *);
int     midichannelaftertouch(ENVIRON *, void *);
int     midipitchbend(ENVIRON *, void *);
int     mididefault(ENVIRON *, void *);
int     invalset(ENVIRON *, void *), kinval(ENVIRON *, void *);
int     outvalset(ENVIRON *, void *), koutval(ENVIRON *, void *);
int     subinstrset(ENVIRON *, void *), subinstr(ENVIRON *, void *);
int     useropcdset(ENVIRON *, void *), useropcd(ENVIRON *, void *);
int     setksmpsset(ENVIRON *, void *);
int     xinset(ENVIRON *, void *), xoutset(ENVIRON *, void *);
int     ingoto(ENVIRON *, void *), kngoto(ENVIRON *, void *);
int     iingoto(ENVIRON *, void *), kingoto(ENVIRON *, void *);
int     nstrnumset(ENVIRON *, void *), turnoff2(ENVIRON *, void *);
int     loop_l_i(ENVIRON *, void *), loop_le_i(ENVIRON *, void *);
int     loop_g_i(ENVIRON *, void *), loop_ge_i(ENVIRON *, void *);
int     loop_l_p(ENVIRON *, void *), loop_le_p(ENVIRON *, void *);
int     loop_g_p(ENVIRON *, void *), loop_ge_p(ENVIRON *, void *);

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
{ "event", S(LINEVENT),  2,     "",     "UUz",  NULL, eventOpcode, NULL   },
{ "event_i", S(LINEVENT),1,     "",     "TTm",  eventOpcodeI, NULL, NULL  },
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

