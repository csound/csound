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
#include "pstream.h"
#include "oscils.h"
#include "midifile.h"
#include "midiinterop.h"
#include "ftgen.h"
#if defined(USE_FLTK)
#include "widgets.h"                    /* IV - Aug 23 2002 */
#endif
#include "linevent.h"

#define S       sizeof

#if defined(TCLTK)
int    cntrl_set(void*,void*), control(void*,void*), ocontrol(void*,void*);
int    button_set(void*,void*), button(void*,void*), check_set(void*,void*), check(void*,void*);
int    textflash(void*,void*);
#endif
int    instcount(void*,void*);
int    varicolset(void*,void*), varicol(void*,void*);
int    inh(void*,void*), ino(void*,void*), in16(void*,void*);
int    in32(void*,void*), inall(void*,void*), inz(void*,void*), outh(void*,void*);
int    outo(void*,void*), outx(void*,void*), outX(void*,void*), outch(void*,void*), outall(void*,void*);
int    outz(void*,void*), cpsxpch(void*,void*), cps2pch(void*,void*), cpstmid(void*,void*);
int    cpstun(void*,void*),cpstun_i(void*,void*);
int    wgpsetin(void*,void*), wgpluck(void*,void*), wgpset(void*,void*), wgpluck(void*,void*);
int    clarinset(void*,void*), clarin(void*,void*), fluteset(void*,void*), flute(void*,void*);
int    bowedset(void*,void*), bowed(void*,void*);
int    brassset(void*,void*), brass(void*,void*);
int    adsrset(void*,void*), klnseg(void*,void*), linseg(void*,void*), madsrset(void*,void*);
int    klnsegr(void*,void*), linsegr(void*,void*), xdsrset(void*,void*), kxpseg(void*,void*);
int    expseg(void*,void*), mxdsrset(void*,void*), kxpsegr(void*,void*), expsegr(void*,void*);
int    schedule(void*,void*), schedwatch(void*,void*), ifschedule(void*,void*), kschedule(void*,void*);
int    triginset(void*,void*), ktriginstr(void*,void*), trigseq_set(void*,void*), trigseq(void*,void*);
int    eventOpcode(void*,void*), eventOpcodeI(void*,void*);
int    lfoset(void*,void*), lfok(void*,void*), lfoa(void*,void*);
int    mute_inst(void*,void*);
int    vbap_FOUR_init(void*,void*), vbap_FOUR (void*,void*), vbap_EIGHT_init(void*,void*);
int    vbap_EIGHT(void*,void*), vbap_SIXTEEN_init(void*,void*), vbap_SIXTEEN(void*,void*);
int    vbap_zak_init(void*,void*), vbap_zak(void*,void*), vbap_ls_init(void*,void*);
int    vbap_FOUR_moving_init(void*,void*), vbap_FOUR_moving(void*,void*);
int    vbap_EIGHT_moving_init(void*,void*), vbap_EIGHT_moving(void*,void*);
int    vbap_SIXTEEN_moving_init(void*,void*), vbap_SIXTEEN_moving(void*,void*);
int    vbap_zak_moving_init(void*,void*), vbap_zak_moving(void*,void*);
#ifdef JPFF
int    Foscset(void*,void*), Fosckk(void*,void*), Foscka(void*,void*);
int    Foscak(void*,void*), Foscaa(void*,void*);
#endif
int    lpf18set(void*,void*), lpf18db(void*,void*);
int    pfun(void*,void*);
int    pvsanalset(void*,void*),pvsanal(void*,void*),pvsynthset(void*,void*),pvsynth(void*,void*);
int    pvadsynset(void*,void*), pvadsyn(void*,void*);
int    pvscrosset(void*,void*),pvscross(void*,void*);
int    pvsfreadset(void*,void*), pvsfread(void*,void*);
int    pvsmaskaset(void*,void*),pvsmaska(void*,void*);
int    pvsftwset(void*,void*),pvsftw(void*,void*),pvsftrset(void*,void*),pvsftr(void*,void*);
int    pvsinfo(void*,void*), gettempo(void*,void*);
int    fassign(void*,void*);
int    loopseg_set(void*,void*), loopseg(void*,void*), lpshold(void*,void*);
int    lineto_set(void*,void*), lineto(void*,void*), tlineto_set(void*,void*),tlineto(void*,void*);
int    vibrato_set(void*,void*), vibrato(void*,void*),vibr_set(void*,void*), vibr(void*,void*);
int    oscbnkset(void*,void*), oscbnk(void*,void*), userrnd_set(void*,void*);
int    oscktset(void*,void*), kosclikt(void*,void*), osckkikt(void*,void*), osckaikt(void*,void*);
int    oscakikt(void*,void*), oscaaikt(void*,void*), oscktpset(void*,void*), oscktp(void*,void*);
int    oscktsset(void*,void*), osckts(void*,void*);
int    iDiscreteUserRand(void*,void*), kDiscreteUserRand(void*,void*), Cuserrnd_set(void*,void*);
int    aDiscreteUserRand(void*,void*), iContinuousUserRand(void*,void*);
int    kContinuousUserRand(void*,void*), aContinuousUserRand(void*,void*);
int    ikRangeRand(void*,void*), aRangeRand(void*,void*);
int    randomi_set(void*,void*), krandomi(void*,void*), randomi(void*,void*);
int    randomh_set(void*,void*), krandomh(void*,void*), randomh(void*,void*);
int    random3_set(void*,void*), random3(void*,void*),random3a(void*,void*);
int    ipowoftwo(void*,void*), ilogbasetwo(void*,void*), db(void*,void*);
int    powoftwo_set(void*,void*), powoftwoa(void*,void*), dbi(void*,void*), dba(void*,void*);
int    powoftwo(void*,void*), powoftwoa(void*,void*), semitone(void*,void*), isemitone(void*,void*);
int    asemitone(void*,void*), cent(void*,void*), icent(void*,void*), acent(void*,void*);
int    and_kk(void*,void*), and_ka(void*,void*), and_ak(void*,void*), and_aa(void*,void*);
int    or_kk(void*,void*), or_ka(void*,void*), or_ak(void*,void*), or_aa(void*,void*);
int    xor_kk(void*,void*), xor_ka(void*,void*), xor_ak(void*,void*), xor_aa(void*,void*);
int    not_k(void*,void*), not_a(void*,void*);
int    midichn(void*,void*), pgmassign(void*,void*);
int    midinoteoff(void*,void*), midinoteonkey(void*,void*), midinoteoncps(void*,void*);
int    midinoteonoct(void*,void*), midinoteonpch(void*,void*), midipolyaftertouch(void*,void*);
int    midicontrolchange(void*,void*), midiprogramchange(void*,void*);
int    midichannelaftertouch(void*,void*), midipitchbend(void*,void*), mididefault(void*,void*);
#if defined(USE_FLTK)                  /* IV - Aug 23 2002 */
int    fl_slider(void*,void*), fl_slider_bank(void*,void*);
int    StartPanel(void*,void*), EndPanel(void*,void*), FL_run(void*,void*);
int    fl_widget_color(void*,void*), fl_widget_color2(void*,void*);
int    fl_knob(void*,void*), fl_roller(void*,void*), fl_text(void*,void*);
int    fl_value(void*,void*), StartScroll(void*,void*), EndScroll(void*,void*);
int    StartPack(void*,void*), EndPack(void*,void*), fl_widget_label(void*,void*);
int    fl_setWidgetValuei(void*,void*), fl_setWidgetValue(void*,void*);
int    fl_setWidgetValue_set(void*,void*);
int    fl_update(void*,void*), StartGroup(void*,void*), EndGroup(void*,void*);
int    StartTabs(void*,void*), EndTabs(void*,void*);
int    fl_joystick(void*,void*), fl_button(void*,void*), FLkeyb(void*,void*), fl_counter(void*,void*);
int    set_snap(void*,void*), get_snap(void*,void*);
int    fl_setColor1(void*,void*), fl_setColor2(void*,void*);
int    fl_setTextSize(void*,void*), fl_setTextColor(void*,void*);
int    fl_setFont(void*,void*), fl_setText(void*,void*), fl_setSize(void*,void*);
int    fl_setTextType(void*,void*), fl_setBox(void*,void*);
int    fl_setPosition(void*,void*), fl_hide(void*,void*), fl_show(void*,void*), fl_box(void*,void*);
int    fl_align(void*,void*);
int    save_snap(void*,void*), load_snap(void*,void*), fl_button_bank(void*,void*);
int    FLprintkset(void*,void*), FLprintk(void*,void*);
int    FLprintk2set(void*,void*), FLprintk2(void*,void*);
#endif
int    invalset(void*,void*), kinval(void*,void*), outvalset(void*,void*), koutval(void*,void*);
int    subinstrset(void*,void*), subinstr(void*,void*);    /* IV - Sep 1 2002 */
int    useropcdset(void*,void*), useropcd(void*,void*), setksmpsset(void*,void*); /* IV - Sep 8 2002 */
int    xinset(void*,void*), xoutset(void*,void*);          /* IV - Sep 10 2002 */
int    ingoto(void*,void*), kngoto(void*,void*);
int    nstrnumset(void*,void*);
int    ftsave(void*,void*), ftload(void*,void*), ftsave_k_set(void*,void*), ftsave_k(void*,void*);
int    ftsave_k_set(void*,void*), ftload_k(void*,void*);

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
                S       String or irate
                B       Boolean
                l       Label
     and codes
                m       begins an indef list of iargs (any count)
                M       begins an indef list of args (any count/rate) IV -2002/9/1
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

/* If dsblksize is 0xffff then translate on firdt arg*/
/*                 0xfffe then translate two (oscil) */
/*                 0xfffd then translate on ans (peak) */
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
{ "schedwhen", S(WSCHED),3,     "",     "kkkkm",ifschedule, kschedule, NULL },
{ "schedkwhen", S(TRIGINSTR), 3,"",     "kkkkkz",triginset, ktriginstr, NULL },
{ "schedkwhennamed", S(TRIGINSTR), 3,"", "kkkSkz",triginset, ktriginstr, NULL },
{ "trigseq", S(TRIGSEQ), 3,     "",     "kkkkkz", trigseq_set, trigseq, NULL },
{ "event", S(LINEVENT),  2,     "",     "SSz",  NULL, eventOpcode, NULL   },
{ "event_i", S(LINEVENT),1,     "",     "SSm",  eventOpcodeI, NULL, NULL  },
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
/* IV - Aug 23 2002 */
#if defined(USE_FLTK)
{ "FLslider",S(FLSLIDER), 1,    "ki",   "Siijjjjjjj",   fl_slider, NULL, NULL   },
{ "FLslidBnk",S(FLSLIDERBANK), 1, "", "Siooooooooo", fl_slider_bank, NULL, NULL },
{ "FLknob",S(FLKNOB),     1,    "ki",   "Siijjjjjj",    fl_knob, NULL, NULL     },
{ "FLroller",S(FLROLLER), 1,    "ki",   "Siijjjjjjjj",  fl_roller, NULL, NULL   },
{ "FLtext",S(FLTEXT),     1,    "ki",   "Siijjjjjj",    fl_text, NULL, NULL     },
{ "FLjoy",S(FLJOYSTICK),  1,    "kkii", "Siiiijjjjjjjj", fl_joystick, NULL, NULL},
{ "FLcount",S(FLCOUNTER), 1,    "ki",   "Siiiiiiiiiz", fl_counter, NULL, NULL  },
{ "FLbutton",S(FLBUTTON), 1,    "ki",   "Siiiiiiiz",    fl_button, NULL, NULL   },
{ "FLbutBank",S(FLBUTTONBANK), 1, "ki", "iiiiiiiiz", fl_button_bank, NULL, NULL },
{ "FLkeyb",S(FLKEYB),     1,    "k",    "z",            FLkeyb, NULL, NULL      },
{ "FLcolor",S(FLWIDGCOL), 1,    "",     "jjjjjj",   fl_widget_color, NULL, NULL },
{ "FLcolor2",S(FLWIDGCOL2), 1,  "",     "jjj",     fl_widget_color2, NULL, NULL },
{ "FLlabel",S(FLWIDGLABEL), 1,  "",     "ojojjj",   fl_widget_label, NULL, NULL },
{ "FLsetVal_i",S(FL_SET_WIDGET_VALUE_I), 1, "", "ii", fl_setWidgetValuei, NULL, NULL },
{ "FLsetVali",S(FL_SET_WIDGET_VALUE_I), 1, "", "ii", fl_setWidgetValuei, NULL, NULL },
{ "FLsetVal",S(FL_SET_WIDGET_VALUE), 3, "", "kki", fl_setWidgetValue_set, fl_setWidgetValue },
{ "FLsetColor",S(FL_SET_COLOR), 1, "",  "iiii",     fl_setColor1, NULL, NULL},
{ "FLsetColor2",S(FL_SET_COLOR), 1, "", "iiii",     fl_setColor2, NULL, NULL},
{ "FLsetTextSize",S(FL_SET_TEXTSIZE), 1, "", "ii",  fl_setTextSize, NULL, NULL },
{ "FLsetTextColor",S(FL_SET_COLOR), 1, "", "iiii",  fl_setTextColor, NULL, NULL },
{ "FLsetFont",S(FL_SET_FONT), 1, "",    "ii",   fl_setFont, NULL, NULL      },
{ "FLsetTextType",S(FL_SET_FONT), 1, "", "ii",  fl_setTextType, NULL, NULL  },
{ "FLsetText",S(FL_SET_TEXT), 1, "",    "Si",       fl_setText, NULL, NULL  },
{ "FLsetSize",S(FL_SET_SIZE), 1, "",    "iii",      fl_setSize, NULL, NULL  },
{ "FLsetPosition",S(FL_SET_POSITION), 1, "", "iii", fl_setPosition, NULL, NULL },
{ "FLhide",S(FL_WIDHIDE), 1,    "",     "i",        fl_hide, NULL, NULL     },
{ "FLshow",S(FL_WIDSHOW), 1,    "",     "i",        fl_show, NULL, NULL     },
{ "FLsetBox",S(FL_SETBOX), 1,   "",     "ii",       fl_setBox, NULL, NULL   },
{ "FLsetAlign",S(FL_TALIGN), 1, "",     "ii",       fl_align, NULL, NULL    },
{ "FLbox",S(FL_BOX),      1,    "i",    "Siiiiiii", fl_box, NULL, NULL      },
{ "FLvalue",S(FLVALUE),   1,    "i",    "Sjjjj",    fl_value, NULL, NULL    },
{ "FLpanel",S(FLPANEL),   1,    "",     "Sjjooo",   StartPanel, NULL, NULL  },
{ "FLpanelEnd",S(FLPANELEND), 1, "",    "",         EndPanel, NULL, NULL    },
{ "FLpanel_end",S(FLPANELEND), 1, "",    "",        EndPanel, NULL, NULL    },
{ "FLscroll",S(FLSCROLL), 1,    "",     "iiii",     StartScroll, NULL, NULL },
{ "FLscrollEnd",S(FLSCROLLEND), 1, "",  "",         EndScroll, NULL, NULL   },
{ "FLscroll_end",S(FLSCROLLEND), 1, "",  "",        EndScroll, NULL, NULL   },
{ "FLpack",S(FLPACK),     1,    "",     "iiii",     StartPack, NULL, NULL   },
{ "FLpackEnd",S(FLPACKEND), 1, "",      "",         EndPack, NULL, NULL     },
{ "FLpack_end",S(FLPACKEND), 1, "",      "",        EndPack, NULL, NULL     },
{ "FLtabs",S(FLTABS),     1,    "",     "iiii",     StartTabs, NULL, NULL   },
{ "FLtabsEnd",S(FLTABSEND), 1, "",      "",         EndTabs, NULL, NULL     },
{ "FLtabs_end",S(FLTABSEND), 1, "",      "",        EndTabs, NULL, NULL     },
{ "FLgroup",S(FLGROUP),   1,    "",     "Siiiij",   StartGroup, NULL, NULL  },
{ "FLgroupEnd",S(FLGROUPEND), 1, "",    "",         EndGroup, NULL, NULL    },
{ "FLgroup_end",S(FLGROUPEND), 1, "",    "",        EndGroup, NULL, NULL    },
{ "FLsetsnap",S(FLSETSNAP), 1,  "ii",   "io",       set_snap, NULL, NULL    },
{ "FLgetsnap",S(FLGETSNAP), 1,  "i",    "i",        get_snap, NULL, NULL    },
{ "FLsavesnap",S(FLSAVESNAPS), 1, "",   "S",        save_snap, NULL, NULL   },
{ "FLloadsnap",S(FLLOADSNAPS), 1, "",   "S",        load_snap, NULL, NULL   },
{ "FLrun",S(FLRUN),       1,    "",     "",         FL_run, NULL, NULL      },
{ "FLupdate",S(FLRUN),    1,    "",     "",         fl_update, NULL, NULL   },
{ "FLprintk",S(FLPRINTK), 3,    "",     "iki",  FLprintkset, FLprintk, NULL },
{ "FLprintk2",S(FLPRINTK2), 3,  "",     "ki",   FLprintk2set, FLprintk2, NULL },
#endif
{ "=.f",      S(FASSIGN), 2,    "f",   "f",      NULL, fassign, NULL            },
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
{ "octave.i", S(EVAL),    1,    "i",    "i",     ipowoftwo                      },
{ "octave.k", S(EVAL),    3,    "k",    "k",     powoftwo_set,   powoftwo       },
{ "octave.a", S(EVAL),    5,    "a",    "a",     powoftwo_set, NULL, powoftwoa  },
{ "semitone.i",S(EVAL),   1,    "i",    "i",     isemitone                      },
{ "semitone.k",S(EVAL),   3,    "k",    "k",     powoftwo_set,   semitone       },
{ "semitone.a",S(EVAL),   5,    "a",    "a",     powoftwo_set, NULL, asemitone  },
{ "cent.i",   S(EVAL),    1,    "i",    "i",     icent                          },
{ "cent.k",   S(EVAL),    3,    "k",    "k",     powoftwo_set,   cent           },
{ "cent.a",   S(EVAL),    5,    "a",    "a",     powoftwo_set, NULL, acent      },
{ "db",       0xffff                                                    },
{ "db.i",     S(EVAL),    1,    "i",    "i",     dbi                    },
{ "db.k",     S(EVAL),    3,    "k",    "k",     powoftwo_set, db       },
{ "db.a",     S(EVAL),    5,    "a",    "a",     powoftwo_set, NULL, dba},
{ "midichn",  S(MIDICHN), 1,    "i",    "",     midichn, NULL, NULL     },
{ "pgmassign",S(PGMASSIGN), 1,   "",    "iSo",  pgmassign, NULL, NULL   },
{ "miditempo", S(MIDITEMPO), 2, "k", "", NULL, (SUBR) midiTempoOpcode, NULL },
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
{ ".userOpcode", S(UOPCODE), 7, "", "", useropcdset, useropcd, useropcd },
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
{ "a.k",    S(INTERP),  5,  "a", "k",    (SUBR)a_k_set,NULL,   (SUBR)interp  },
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

