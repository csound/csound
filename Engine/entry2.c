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

#include "cs.h"                 /*                      ENTRY.C         */
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
/* #include "babo.h" */
#include "pstream.h"
#include "oscils.h"
#include "midiinterop.h"
#include "ftgen.h"
#if defined(USE_FLTK)
#include "widgets.h"                    /* IV - Aug 23 2002 */
#endif

#define S       sizeof

#if defined(TCLTK)
int    cntrl_set(void*), control(void*), ocontrol(void*);
int    button_set(void*), button(void*), check_set(void*), check(void*);
int    textflash(void*);
#endif
int    instcount(void*);
int    varicolset(void*), varicol(void*);
int    inh(void*), ino(void*), in16(void*);
int    in32(void*), inall(void*), inz(void*), outh(void*);
int    outo(void*), outx(void*), outX(void*), outch(void*), outall(void*);
int    outz(void*), cpsxpch(void*), cps2pch(void*), cpstmid(void*);
int    cpstun(void*),cpstun_i(void*);
int    wgpsetin(void*), wgpluck(void*), wgpset(void*), wgpluck(void*);
int    clarinset(void*), clarin(void*), fluteset(void*), flute(void*);
int    bowedset(void*), bowed(void*);
int    brassset(void*), brass(void*);
int    adsrset(void*), klnseg(void*), linseg(void*), madsrset(void*);
int    klnsegr(void*), linsegr(void*), xdsrset(void*), kxpseg(void*);
int    expseg(void*), mxdsrset(void*), kxpsegr(void*), expsegr(void*);
int    schedule(void*), schedwatch(void*), ifschedule(void*), kschedule(void*);
int    triginset(void*), ktriginstr(void*), trigseq_set(void*), trigseq(void*);
int    event_set(void*), eventOpcode(void*);
int    seqtim_set(void*), seqtim(void*), lfoset(void*), lfok(void*);
int    lfoa(void*);
int    mute_inst(void*);
int    vbap_FOUR_init(void*), vbap_FOUR (void*), vbap_EIGHT_init(void*);
int    vbap_EIGHT(void*), vbap_SIXTEEN_init(void*), vbap_SIXTEEN(void*);
int    vbap_zak_init(void*), vbap_zak(void*), vbap_ls_init(void*);
int    vbap_FOUR_moving_init(void*), vbap_FOUR_moving(void*);
int    vbap_EIGHT_moving_init(void*), vbap_EIGHT_moving(void*);
int    vbap_SIXTEEN_moving_init(void*), vbap_SIXTEEN_moving(void*);
int    vbap_zak_moving_init(void*), vbap_zak_moving(void*);
#ifdef JPFF
int    Foscset(void*), Fosckk(void*), Foscka(void*);
int    Foscak(void*), Foscaa(void*);
#endif
int    lpf18set(void*), lpf18db(void*);
int    pfun(void*);
int    pvsanalset(void*),pvsanal(void*),pvsynthset(void*),pvsynth(void*);
int    pvadsynset(void*), pvadsyn(void*);
int    pvscrosset(void*),pvscross(void*);
int    pvsfreadset(void*), pvsfread(void*);
int    pvsmaskaset(void*),pvsmaska(void*);
int    pvsftwset(void*),pvsftw(void*),pvsftrset(void*),pvsftr(void*);
int    pvsinfo(void*), gettempo(void*);
int    fassign(void*);
int    loopseg_set(void*), loopseg(void*), lpshold(void*);
int    lineto_set(void*), lineto(void*), tlineto_set(void*),tlineto(void*);
int    vibrato_set(void*), vibrato(void*),vibr_set(void*), vibr(void*);
int    oscbnkset(void*), oscbnk(void*), userrnd_set(void*);
int    oscktset(void*), kosclikt(void*), osckkikt(void*), osckaikt(void*);
int    oscakikt(void*), oscaaikt(void*), oscktpset(void*), oscktp(void*);
int    oscktsset(void*), osckts(void*);
int    iDiscreteUserRand(void*), kDiscreteUserRand(void*), Cuserrnd_set(void*);
int    aDiscreteUserRand(void*), iContinuousUserRand(void*);
int    kContinuousUserRand(void*), aContinuousUserRand(void*);
int    ikRangeRand(void*), aRangeRand(void*);
int    randomi_set(void*), krandomi(void*), randomi(void*);
int    randomh_set(void*), krandomh(void*), randomh(void*);
int    random3_set(void*), random3(void*),random3a(void*);
int    ipowoftwo(void*), ilogbasetwo(void*), db(void*);
int    powoftwo_set(void*), powoftwoa(void*), dbi(void*), dba(void*);
int    powoftwo(void*), powoftwoa(void*), semitone(void*), isemitone(void*);
int    asemitone(void*), cent(void*), icent(void*), acent(void*);
int    and_kk(void*), and_ka(void*), and_ak(void*), and_aa(void*);
int    or_kk(void*), or_ka(void*), or_ak(void*), or_aa(void*);
int    xor_kk(void*), xor_ka(void*), xor_ak(void*), xor_aa(void*);
int    not_k(void*), not_a(void*);
int    midichn(void*), pgmassign(void*);
int    midinoteoff(void*), midinoteonkey(void*), midinoteoncps(void*);
int    midinoteonoct(void*), midinoteonpch(void*), midipolyaftertouch(void*);
int    midicontrolchange(void*), midiprogramchange(void*);
int    midichannelaftertouch(void*), midipitchbend(void*), mididefault(void*);
#if defined(USE_FLTK)                  /* IV - Aug 23 2002 */
int    fl_slider(void*), fl_slider_bank(void*);
int    StartPanel(void*), EndPanel(void*), FL_run(void*);
int    fl_widget_color(void*), fl_widget_color2(void*);
int    fl_knob(void*), fl_roller(void*), fl_text(void*);
int    fl_value(void*), StartScroll(void*), EndScroll(void*);
int    StartPack(void*), EndPack(void*), fl_widget_label(void*);
int    fl_setWidgetValuei(void*), fl_setWidgetValue(void*);
int    fl_update(void*), StartGroup(void*), EndGroup(void*);
int    StartTabs(void*), EndTabs(void*);
int    fl_joystick(void*), fl_button(void*), FLkeyb(void*), fl_counter(void*);
int    set_snap(void*), get_snap(void*);
int    fl_setColor1(void*), fl_setColor2(void*);
int    fl_setTextSize(void*), fl_setTextColor(void*);
int    fl_setFont(void*), fl_setText(void*), fl_setSize(void*);
int    fl_setTextType(void*), fl_setBox(void*);
int    fl_setPosition(void*), fl_hide(void*), fl_show(void*), fl_box(void*);
int    fl_align(void*);
int    save_snap(void*), load_snap(void*), fl_button_bank(void*);
int    FLprintkset(void*), FLprintk(void*);
int    FLprintk2set(void*), FLprintk2(void*);
#endif
int    invalset(void*), kinval(void*), outvalset(void*), koutval(void*);
int    subinstrset(void*), subinstr(void*);    /* IV - Sep 1 2002 */
int    useropcdset(void*), useropcd(void*), setksmpsset(void*); /* IV - Sep 8 2002 */
int    xinset(void*), xoutset(void*);          /* IV - Sep 10 2002 */
int    ingoto(void*), kngoto(void*);
int    nstrnumset(void*);
int    ftsave(void*), ftload(void*), ftsave_k_set(void*), ftsave_k(void*);
int    ftsave_k_set(void*), ftload_k(void*);

/* thread vals, where isub=1, ksub=2, asub=4:
                0 =     1  OR   2  (B out only)
                1 =     1
                2 =             2
                3 =     1  AND  2
                4 =                     4
                5 =     1  AND          4
                7 =     1  AND (2  OR   4)                      */

/* inarg types include the following:
                m       begins an indef list of iargs (any count)
                M       begins an indef list of args (any count and rate)   IV - Sep 1 2002
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

/* If dsblksize is 0xffff then translate */
/*                 0xfffe then translate two (oscil) */
/*                 0xfffd then translate two (peak) */
/*                 0xfffc then translate two (divz) */

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
{ "setctrl", S(SCNTRL),  1,     "",     "iSi",  ocontrol, NULL, NULL   },
{ "button", S(CNTRL),    3,     "k",    "k",    button_set, button, NULL   },
{ "checkbox", S(CNTRL),  3,     "k",    "k",    check_set, check,   NULL   },
{ "flashtxt", S(TXTWIN), 1,     "",     "iS",   textflash, NULL,    NULL   },
#endif
{ "adsr", S(LINSEG),     7,     "s",    "iiiio",adsrset,klnseg, linseg     },
{ "madsr", S(LINSEG),    7,     "s",    "iiiioj", madsrset,klnsegr, linsegr },
{ "xadsr", S(EXXPSEG),   7,     "s",    "iiiio", xdsrset, kxpseg, expseg    },
{ "mxadsr", S(EXPSEG),   7,     "s",    "iiiioj", mxdsrset, kxpsegr, expsegr },
{ "schedule", S(SCHED),  1,     "",     "Siim", schedule, schedwatch, NULL },
{ "schedwhen", S(WSCHED),3,     "",     "Skkkm",ifschedule, kschedule, NULL },
{ "schedkwhen", S(TRIGINSTR), 3,"",     "kkkkkz",triginset, ktriginstr, NULL },
{ "schedkwhennamed", S(TRIGINSTR), 3,"", "kkkSkz",triginset, ktriginstr, NULL },
{ "trigseq", S(TRIGSEQ), 3,     "",     "kkkkkz", trigseq_set, trigseq, NULL },
{ "seqtime", S(SEQTIM),  3,     "k",    "kkkkk", seqtim_set, seqtim, NULL   },
{ "event", S(LINEVENT),  3,     "",     "SSz",  event_set, eventOpcode, NULL },
{ "lfo", S(LFO),         7,     "s",    "kko",  lfoset,   lfok,   lfoa     },
{ "vbap4",  S(VBAP_FOUR), 5, "aaaa","aioo", vbap_FOUR_init, NULL, vbap_FOUR },
{ "vbap8",  S(VBAP_EIGHT), 5, "aaaaaaaa","aioo", vbap_EIGHT_init, NULL, vbap_EIGHT },
{ "vbap16", S(VBAP_SIXTEEN), 5, "aaaaaaaaaaaaaaaa","aioo", vbap_SIXTEEN_init, NULL, vbap_SIXTEEN },
{ "vbapz",  S(VBAP_ZAK), 5,     "",    "iiaioo", vbap_zak_init, NULL, vbap_zak },
{ "vbaplsinit",  S(VBAP_LS_INIT), 1, "","iioooooooooooooooooooooooooooooooo", vbap_ls_init},
{ "vbap4move",  S(VBAP_FOUR_MOVING), 5, "aaaa","aiiim", vbap_FOUR_moving_init, NULL, vbap_FOUR_moving },
{ "vbap8move",  S(VBAP_EIGHT_MOVING), 5, "aaaaaaaa","aiiim", vbap_EIGHT_moving_init, NULL, vbap_EIGHT_moving },
{ "vbap16move",  S(VBAP_SIXTEEN_MOVING), 5, "aaaaaaaaaaaaaaaa","aiiim", vbap_SIXTEEN_moving_init, NULL, vbap_SIXTEEN_moving },
{ "vbapzmove",  S(VBAP_ZAK_MOVING), 5, "","iiaiiim", vbap_zak_moving_init, NULL, vbap_zak_moving },
{ "oscils",   S(OSCILS), 5,     "a", "iiio",     oscils_set, NULL, oscils       },
{ "lphasor",  S(LPHASOR),5,     "a", "xooooooo" ,lphasor_set, NULL, lphasor     },
{ "tablexkt", S(TABLEXKT),5,    "a", "xkkiooo",  tablexkt_set, NULL, tablexkt   },
{ "reverb2",  S(NREV2),  5,     "a",    "akkoojoj", (SUBR)reverbx_set,NULL,(SUBR)reverbx    },
{ "nreverb",  S(NREV2),  5,     "a",    "akkoojoj", (SUBR)reverbx_set,NULL,(SUBR) reverbx    },
/* IV - Aug 23 2002 */
#if defined(USE_FLTK)
{ "FLslider",S(FLSLIDER), 1,    "ki",   "Siijjjjjjj",   fl_slider, NULL, NULL   },
{ "FLslidBnk",S(FLSLIDERBANK), 1, "", "Siooooooooo", fl_slider_bank, NULL, NULL },
{ "FLknob",S(FLKNOB),     1,    "ki",   "Siijjjjjj",    fl_knob, NULL, NULL     },
{ "FLroller",S(FLROLLER), 1,    "ki",   "Siijjjjjjjj",  fl_roller, NULL, NULL   },
{ "FLtext",S(FLTEXT),     1,    "ki",   "Siijjjjjj",    fl_text, NULL, NULL     },
{ "FLjoy",S(FLJOYSTICK),  1,    "kkii", "Siiiijjjjjjjj", fl_joystick, NULL, NULL},
{ "FLcount",S(FLCOUNTER), 1,    "ki",   "Siiiiiiiiiiz", fl_counter, NULL, NULL  },
{ "FLbutton",S(FLBUTTON), 1,    "ki",   "Siiiiiiiz",    fl_button, NULL, NULL   },
{ "FLbutBank",S(FLBUTTONBANK), 1, "ki", "iiiiiiiiz", fl_button_bank, NULL, NULL },
{ "FLkeyb",S(FLKEYB),     1,    "k",    "z",            FLkeyb, NULL, NULL      },
{ "FLcolor",S(FLWIDGCOL), 1,    "",     "jjjjjj",   fl_widget_color, NULL, NULL },
{ "FLcolor2",S(FLWIDGCOL2), 1,  "",     "jjj",     fl_widget_color2, NULL, NULL },
{ "FLlabel",S(FLWIDGLABEL), 1,  "",     "ojojjj",   fl_widget_label, NULL, NULL },
{ "FLsetVal_i",S(FL_SET_WIDGET_VALUE_I), 1, "", "ii", fl_setWidgetValuei, NULL, NULL },
{ "FLsetVal",S(FL_SET_WIDGET_VALUE_I), 1, "", "ii", fl_setWidgetValue, NULL, NULL },
{ "FLsetColor",S(FL_SET_COLOR), 1, "",  "iiii",         fl_setColor1, NULL, NULL},
{ "FLsetColor2",S(FL_SET_COLOR), 1, "", "iiii",         fl_setColor2, NULL, NULL},
{ "FLsetTextSize",S(FL_SET_TEXTSIZE), 1, "", "ii",   fl_setTextSize, NULL, NULL },
{ "FLsetTextColor",S(FL_SET_COLOR), 1, "", "iiii",  fl_setTextColor, NULL, NULL },
{ "FLsetFont",S(FL_SET_FONT), 1, "",    "ii",           fl_setFont, NULL, NULL  },
{ "FLsetTextType",S(FL_SET_FONT), 1, "", "ii",       fl_setTextType, NULL, NULL },
{ "FLsetText",S(FL_SET_TEXT), 1, "",    "Si",           fl_setText, NULL, NULL  },
{ "FLsetSize",S(FL_SET_SIZE), 1, "",    "iii",          fl_setSize, NULL, NULL  },
{ "FLsetPosition",S(FL_SET_POSITION), 1, "", "iii",  fl_setPosition, NULL, NULL },
{ "FLhide",S(FL_WIDHIDE), 1,    "",     "i",            fl_hide, NULL, NULL     },
{ "FLshow",S(FL_WIDSHOW), 1,    "",     "i",            fl_show, NULL, NULL     },
{ "FLsetBox",S(FL_SETBOX), 1,   "",     "ii",           fl_setBox, NULL, NULL   },
{ "FLsetAlign",S(FL_TALIGN), 1, "",     "ii",           fl_align, NULL, NULL    },
{ "FLbox",S(FL_BOX),      1,    "i",    "Siiiiiii",     fl_box, NULL, NULL      },
{ "FLvalue",S(FLVALUE),   1,    "i",    "Sjjjj",        fl_value, NULL, NULL    },
{ "FLpanel",S(FLPANEL),   1,    "",     "Sjjooo",       StartPanel, NULL, NULL  },
{ "FLpanelEnd",S(FLPANELEND), 1, "",    "",             EndPanel, NULL, NULL    },
{ "FLscroll",S(FLSCROLL), 1,    "",     "iiii",         StartScroll, NULL, NULL },
{ "FLscrollEnd",S(FLSCROLLEND), 1, "",  "",             EndScroll, NULL, NULL   },
{ "FLpack",S(FLPACK),     1,    "",     "iiii",         StartPack, NULL, NULL   },
{ "FLpackEnd",S(FLPACKEND), 1, "",      "",             EndPack, NULL, NULL     },
{ "FLtabs",S(FLTABS),     1,    "",     "iiii",         StartTabs, NULL, NULL   },
{ "FLtabsEnd",S(FLTABSEND), 1, "",      "",             EndTabs, NULL, NULL     },
{ "FLgroup",S(FLGROUP),   1,    "",     "Siiiij",       StartGroup, NULL, NULL  },
{ "FLgroupEnd",S(FLGROUPEND), 1, "",    "",             EndGroup, NULL, NULL    },
{ "FLsetsnap",S(FLSETSNAP), 1,  "ii",   "io",           set_snap, NULL, NULL    },
{ "FLgetsnap",S(FLGETSNAP), 1,  "i",    "i",            get_snap, NULL, NULL    },
{ "FLsavesnap",S(FLSAVESNAPS), 1, "",   "S",            save_snap, NULL, NULL   },
{ "FLloadsnap",S(FLLOADSNAPS), 1, "",   "S",            load_snap, NULL, NULL   },
{ "FLrun",S(FLRUN),       1,    "",     "",             FL_run, NULL, NULL      },
{ "FLupdate",S(FLRUN),    1,    "",     "",             fl_update, NULL, NULL   },
{ "FLprintk",S(FLPRINTK), 3,    "",     "iki",      FLprintkset, FLprintk, NULL },
{ "FLprintk2",S(FLPRINTK2), 3,  "",     "ki",     FLprintk2set, FLprintk2, NULL },
#endif
{ "=_f",      S(FASSIGN), 2,    "f",   "f",      NULL, fassign, NULL            },
{ "pvsanal",  S(PVSANAL), 5,    "f",   "aiiiioo",  pvsanalset, NULL, pvsanal    },
{ "pvsynth",  S(PVSYNTH), 5,    "a",   "fo",     pvsynthset, NULL, pvsynth      },
{ "pvsadsyn", S(PVADS),   7,    "a",   "fikopo", pvadsynset, pvadsyn, pvadsyn   },
{ "pvscross", S(PVSCROSS),3,    "f",   "ffkk",   pvscrosset, pvscross, NULL     },
{ "pvsfread", S(PVSFREAD),3,    "f",   "kSo",    pvsfreadset, pvsfread, NULL    },
{ "pvsmaska", S(PVSMASKA),3,    "f",   "fik",    pvsmaskaset, pvsmaska, NULL    },
{ "pvsftw",   S(PVSFTW),  3,    "k",   "fio",    pvsftwset, pvsftw, NULL        },
{ "pvsftr",   S(PVSFTR),  3,    "",    "fio",    pvsftrset, pvsftr, NULL        },
{ "pvsinfo",  S(PVSINFO), 1,    "iiii","f",      pvsinfo, NULL, NULL            },
{ "octave",   0xffff                                                            },
{ "semitone", 0xffff                                                            },
{ "cent",     0xffff                                                            },
{ "octave_i", S(EVAL),    1,    "i",    "i",     ipowoftwo                      },
{ "octave_k", S(EVAL),    3,    "k",    "k",     powoftwo_set,   powoftwo       },
{ "octave_a", S(EVAL),    5,    "a",    "a",     powoftwo_set, NULL, powoftwoa  },
{ "semitone_i",S(EVAL),   1,    "i",    "i",     isemitone                      },
{ "semitone_k",S(EVAL),   3,    "k",    "k",     powoftwo_set,   semitone       },
{ "semitone_a",S(EVAL),   5,    "a",    "a",     powoftwo_set, NULL, asemitone  },
{ "cent_i",   S(EVAL),    1,    "i",    "i",     icent                          },
{ "cent_k",   S(EVAL),    3,    "k",    "k",     powoftwo_set,   cent           },
{ "cent_a",   S(EVAL),    5,    "a",    "a",     powoftwo_set, NULL, acent      },
{ "db",       0xffff                                                    },
{ "db_i",     S(EVAL),    1,    "i",    "i",     dbi                    },
{ "db_k",     S(EVAL),    3,    "k",    "k",     powoftwo_set, db       },
{ "db_a",     S(EVAL),    5,    "a",    "a",     powoftwo_set, NULL, dba},
{ "midichn",  S(MIDICHN), 1,    "i",    "",     midichn, NULL, NULL     },
{ "pgmassign",S(PGMASSIGN), 1,   "",    "iS",   pgmassign, NULL, NULL   },
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
{ "invalue", S(INVAL),     3,   "k",    "S", invalset, kinval, NULL      },
{ "outvalue", S(OUTVAL), 3,     "",     "Sk", outvalset, koutval, NULL   },
/* IV - Oct 20 2002 */
{ "subinstr", S(SUBINST), 5, "mmmmmmmm", "Sm",  subinstrset, NULL, subinstr },
{ "subinstrinit", S(SUBINST), 1, "",    "Sm",   subinstrset, NULL, NULL  },
{ "nstrnum", S(NSTRNUM), 1,     "i",    "S",    nstrnumset, NULL, NULL   },
{ "cngoto", S(CGOTO),   3,      "",     "Bl",   ingoto, kngoto, NULL     },
/* IV - Sep 8 2002 - added entries for user defined opcodes, xin, xout */
/* and setksmps */
{ "userOpcode_#", S(UOPCODE), 7, "", "", useropcdset, useropcd, useropcd },
/* IV - Sep 10 2002: removed perf time routines of xin and xout */
{ "xin", S(XIN),   1, "XXXXXXXXXXXXXXXXXXXXXXXX", "", xinset, NULL, NULL },
{ "xout", S(XOUT),    1,        "",     "M",    xoutset, NULL, NULL      },
{ "setksmps", S(SETKSMPS), 1,   "",     "i",    setksmpsset, NULL, NULL  },
{ "ftsave",S(FTLOAD), 1,        "",     "Sim", ftsave                    },
{ "ftload",S(FTLOAD), 1,        "",     "Sim", ftload                    },
{ "ftsavek",S(FTLOAD_K), 3,    "",      "Skim", ftsave_k_set, ftsave_k   },
{ "ftloadk",S(FTLOAD_K), 3,    "",      "Skim", ftsave_k_set, ftload_k   },
{ "tempoval", S(GTEMPO), 2,  "k", "",      NULL, (SUBR)gettempo, NULL    },
{ "downsamp",S(DOWNSAMP),3, "k", "ao",   (SUBR)downset,(SUBR)downsamp        },
{ "upsamp", S(UPSAMP),  4,  "a", "k",    NULL,   NULL,   (SUBR)upsamp        },
/* IV - Sep 5 2002 */
{ "interp", S(INTERP),  5,  "a", "koo",  (SUBR)interpset,NULL, (SUBR)interp  },
{ "a_k",    S(INTERP),  5,  "a", "k",    (SUBR)a_k_set,NULL,   (SUBR)interp  },
{ "integ", S(INDIFF), 7, "s", "xo", (SUBR)indfset,(SUBR)kntegrate,(SUBR)integrate},
{ "diff",   S(INDIFF),  7,  "s", "xo",   (SUBR)indfset,(SUBR)kdiff, (SUBR)diff },
{ "samphold",S(SAMPHOLD),7, "s", "xxoo", (SUBR)samphset,(SUBR)ksmphold,(SUBR)samphold},
{ "delay",  S(DELAY),   5,  "a", "aio",  (SUBR)delset, NULL,   (SUBR)delay   },
{ "delayr", S(DELAYR),  5,  "a", "io",   (SUBR)delrset,NULL,   (SUBR)delayr  },
{ "delayw", S(DELAYW),  5,  "",  "a",    (SUBR)delwset,NULL,   (SUBR)delayw  },
{ "delay1", S(DELAY1),  5,  "a", "ao",   (SUBR)del1set,NULL,   (SUBR)delay1  },
{ "deltap", S(DELTAP),  5,  "a", "k",    (SUBR)tapset, NULL,   (SUBR)deltap  },
{ "deltapi",S(DELTAP),  5,  "a", "x",    (SUBR)tapset, NULL,   (SUBR)deltapi },
{ "deltapn",S(DELTAP),  5,  "a", "x",    (SUBR)tapset, NULL,   (SUBR)deltapn },
{ "deltap3",S(DELTAP),  5,  "a", "x",    (SUBR)tapset, NULL,   (SUBR)deltap3 },
{ "reverb", S(REVERB),  5, "a",  "ako",  (SUBR)rvbset, NULL,   (SUBR)reverb  },
{ "vdelay",   S(VDEL),  5,      "a",    "axio", (SUBR)vdelset, NULL,  (SUBR)vdelay  },
{ "vdelay3",  S(VDEL),  5,      "a",    "axio", (SUBR)vdelset, NULL,  (SUBR)vdelay3 },
{ "vdelayxwq",S(VDELXQ),5,      "aaaa", "aaaaaiio", (SUBR)vdelxqset, NULL, (SUBR)vdelayxwq},
{ "vdelayxws",S(VDELXS),5,      "aa",   "aaaiio", (SUBR)vdelxsset, NULL, (SUBR)vdelayxws},
{ "vdelayxw", S(VDELX), 5,      "a",    "aaiio", (SUBR)vdelxset, NULL, (SUBR)vdelayxw},
{ "vdelayxq", S(VDELXQ),5,      "aaaa", "aaaaaiio", (SUBR)vdelxqset, NULL, (SUBR)vdelayxq},
{ "vdelayxs", S(VDELXS),5,      "aa",   "aaaiio", (SUBR)vdelxsset, NULL, (SUBR)vdelayxs},
{ "vdelayx",  S(VDELX), 5,      "a",    "aaiio", (SUBR)vdelxset, NULL, (SUBR)vdelayx},
{ "deltapx",  S(DELTAPX),5,     "a",    "ai",   (SUBR)tapxset, NULL,  (SUBR)deltapx },
{ "deltapxw", S(DELTAPX),5,     "",     "aai",  (SUBR)tapxset, NULL, (SUBR)deltapxw },
{ "multitap", S(MDEL),  5,      "a",    "am",   (SUBR)multitap_set,NULL,(SUBR)multitap_play},
{ "comb",   S(COMB),    5,      "a",    "akioo", (SUBR)cmbset,NULL,   (SUBR)comb    },
{ "alpass", S(COMB),    5,      "a",    "akioo", (SUBR)cmbset,NULL,   (SUBR)alpass  },
};

long oplength_2 = sizeof(opcodlst_2);

