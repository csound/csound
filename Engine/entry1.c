/*
    entry1.c:

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
#include "aops.h"
#include "midiops.h"
#include "ugens1.h"
#include "ugens2.h"
#include "ugens3.h"
#include "ugens4.h"
#include "ugens5.h"
#include "ugens6.h"
#include "dsputil.h"
#include "ugens8.h"
#include "ugens9.h"
#include "cwindow.h"
#include "windin.h"
#include "disprep.h"
#include "soundio.h"
#include "dumpf.h"
#include "cmath.h"
#include "pvread.h"
#include "pvinterp.h"
#include "vpvoc.h"
#include "fhtfun.h"
#include "diskin.h"
#include "ftgen.h"
#include "oload.h"
#include "midiout.h"
#include "pvadd.h"
#include "sndinfUG.h"
#include "sf.h"
#include "fout.h"
#include "ugrw1.h"
#include "ugrw2.h"

#define S       sizeof

int    ihold(void*), turnoff(void*);
int    assign(void*), rassign(void*), aassign(void*);
int    init(void*), ainit(void*);
int    gt(void*), ge(void*), lt(void*), le(void*), eq(void*), ne(void*);
int    and(void*), or(void*), conval(void*), aconval(void*);
int    addkk(void*), subkk(void*), mulkk(void*), divkk(void*), modkk(void*);
int    addka(void*), subka(void*), mulka(void*), divka(void*), modka(void*);
int    addak(void*), subak(void*), mulak(void*), divak(void*), modak(void*);
int    addaa(void*), subaa(void*), mulaa(void*), divaa(void*), modaa(void*);
int    divzkk(void*), divzka(void*), divzak(void*), divzaa(void*);
int    int1(void*), frac1(void*), rnd1(void*), birnd1(void*);
int    abs1(void*), exp01(void*), log01(void*), sqrt1(void*);
int    sin1(void*), cos1(void*), tan1(void*), asin1(void*), acos1(void*);
int    atan1(void*), sinh1(void*), cosh1(void*), tanh1(void*), log101(void*);
int    atan21(void*), atan2aa(void*);
int    absa(void*), expa(void*), loga(void*), sqrta(void*);
int    sina(void*), cosa(void*), tana(void*), asina(void*), acosa(void*);
int    atana(void*), sinha(void*), cosha(void*), tanha(void*), log10a(void*);
int    dbamp(void*), ampdb(void*), aampdb(void*);
int    dbfsamp(void*), ampdbfs(void*), aampdbfs(void*);
int    ftlen(void*), ftlptim(void*), ftchnls(void*), rtclock(void*);
int    cpsoct(void*), octpch(void*), cpspch(void*);
int    pchoct(void*), octcps(void*), acpsoct(void*);
int    massign(void*), ctrlinit(void*), notnum(void*), veloc(void*);
int    pchmidi(void*), pchmidib(void*), octmidi(void*), octmidib(void*);
int    pchmidib_i(void*), octmidib_i(void*), icpsmidib_i(void*);
int    cpsmidi(void*), icpsmidib(void*), kcpsmidib(void*), kmbset(void*);
int    midibset(void*), ipchmidib(void*), ioctmidib(void*);
int    kpchmidib(void*), koctmidib(void*), msclset(void*);
int    ampmidi(void*), aftset(void*), aftouch(void*), chpress(void*);
int    ipchbend(void*), kbndset(void*), kpchbend(void*);
int    imidictl(void*), mctlset(void*), midictl(void*);
int    imidiaft(void*), maftset(void*), midiaft(void*);
int    midiout(void*);
int    turnon(void*);
int    kmapset(void*), polyaft(void*), ichanctl(void*), chctlset(void*);
int    chanctl(void*), ftgen(void*), linset(void*), kline(void*), aline(void*);
int    expset(void*), kexpon(void*), expon(void*);
int    lsgset(void*), klnseg(void*), linseg(void*), madsrset(void*);
int    adsrset(void*), xdsrset(void*), mxdsrset(void*), expseg2(void*);
int    xsgset(void*), kxpseg(void*), expseg(void*), xsgset2(void*);
int    lsgrset(void*), klnsegr(void*), linsegr(void*);
int    xsgrset(void*), kxpsegr(void*), expsegr(void*);
int    lnnset(void*), klinen(void*), linen(void*);
int    lnrset(void*), klinenr(void*), linenr(void*);
int    evxset(void*), knvlpx(void*), envlpx(void*);
int    evrset(void*), knvlpxr(void*), envlpxr(void*);
int    phsset(void*), kphsor(void*), phsor(void*);
int    itablew1(void*),itablegpw1(void*),itablemix1(void*),itablecopy1(void*);
int    itable(void*), itabli(void*), itabl3(void*), tabl3(void*), ktabl3(void*);
int    tblset(void*), ktable(void*), ktabli(void*), tabli(void*);
int    tablefn(void*);
int    tblsetkt(void*), ktablekt(void*), tablekt(void*), ktablikt(void*);
int    tablikt(void*), ko1set(void*), kosc1(void*),  kosc1i(void*);
int    oscnset(void*), osciln(void*);
int    oscset(void*), koscil(void*), osckk(void*), oscka(void*), oscak(void*);
int    oscaa(void*), koscli(void*), osckki(void*), osckai(void*);
int    oscaki(void*), oscaai(void*), foscset(void*), foscil(void*);
int    foscili(void*), losset(void*), loscil(void*), loscil3(void*);
int    koscl3(void*), osckk3(void*), oscka3(void*), oscak3(void*);
int    oscaa3(void*);
int    adset(void*), adsyn(void*);
int    pvset(void*), pvoc(void*);
int    pvaddset(void*), pvadd(void*);
int    bzzset(void*), buzz(void*);
int    gbzset(void*), gbuzz(void*);
int    plukset(void*), pluck(void*);
int    rndset(void*), krand(void*), arand(void*);
int    rhset(void*), krandh(void*), randh(void*);
int    riset(void*), krandi(void*), randi(void*);
int    rndset2(void*), krand2(void*), arand2(void*);
int    rhset2(void*), krandh2(void*), randh2(void*);
int    riset2(void*), krandi2(void*), randi2(void*);
int    porset(void*), port(void*);
int    tonset(void*), tone(void*), atone(void*);
int    rsnset(void*), reson(void*), areson(void*);
int    resonx(void*), aresonx(void*), rsnsetx(void*);
int    tonex(void*),  atonex(void*), tonsetx(void*);
int    lprdset(void*), lpread(void*), lprsnset(void*), lpreson(void*);
int    lpfrsnset(void*), lpfreson(void*);
int    lpslotset(void*) ;
int    lpitpset(void*),lpinterpol(void*) ;
int    rmsset(void*), rms(void*), gainset(void*), gain(void*);
int    sndinset(void*), soundin(void*);
int    sndo1set(void*), soundout(void*), sndo2set(void*), soundouts(void*);
int    in(void*),  ins(void*), inq(void*), inh(void*),  ino(void*), in16(void*);
int    in32(void*), inall(void*);
int    out(void*),  outs(void*), outs1(void*), outs2(void*), outall(void*);
int    outq(void*), outq1(void*), outq2(void*), outq3(void*), outq4(void*);
int    igoto(void*), kgoto(void*), icgoto(void*), kcgoto(void*);
int    timset(void*), timout(void*);
int    reinit(void*), rigoto(void*), rireturn(void*);
int    tigoto(void*), tival(void*);
int    printv(void*), dspset(void*), kdsplay(void*), dsplay(void*);
int    fftset(void*), kdspfft(void*), dspfft(void*);
int    infile_act(INFILE *p);

int    xyinset(void*), xyin(void*), tempeset(void*), tempest(void*);
int    tempset(void*), tempo(void*);
int    old_kdmpset(void*), old_kdmp2set(void*),
        old_kdmp3set(void*), old_kdmp4set(void*);
int    kdmpset(void*), kdmp2set(void*), kdmp3set(void*), kdmp4set(void*);
int    kdump(void*), kdump2(void*), kdump3(void*), kdump4(void*);
int    krdset(void*), krd2set(void*), krd3set(void*), krd4set(void*);
int    kread(void*), kread2(void*), kread3(void*), kread4(void*);
int    ipow(void*), apow(void*), alinear(void*), iklinear(void*);
int    atrian(void*), iktrian(void*), aexp(void*);
int    ikexp(void*), abiexp(void*), ikbiexp(void*), agaus(void*), ikgaus(void*);
int    acauchy(void*), ikcauchy(void*), apcauchy(void*), ikpcauchy(void*);
int    abeta(void*), ikbeta(void*), aweib(void*), ikweib(void*), apoiss(void*);
int    ikpoiss(void*), seedrand(void*);

int    tblesegset(void*), ktableseg(void*), ktablexseg(void*);
int    vpvset(void*), vpvoc(void*);
int    pvreadset(void*), pvread(void*), pvcrossset(void*), pvcross(void*);
int    pvbufreadset(void*), pvbufread(void*);
int    pvinterpset(void*), pvinterp(void*);
int    auniform(void*), ikuniform(void*);
int    newsndinset(void*), soundinew(void*);
int    iout_on(void*), iout_off(void*), out_controller(void*);
int    iout_on_dur_set(void*), iout_on_dur(void*),iout_on_dur2(void*);
int    moscil_set(void*), moscil(void*);
int    kvar_out_on_set(void*), kvar_out_on_set1(void*), kvar_out_on(void*);
int    out_controller14(void*), out_pitch_bend(void*);
int    out_aftertouch(void*), out_poly_aftertouch(void*);
int    out_progchange(void*);
int    release_set(void*), release(void*), xtratim(void*);
int    mclock_set(void*), mclock(void*), mrtmsg(void*);
int    cabasaset(void*), cabasa(void*), sekereset(void*), sandset(void*);
int    stixset(void*), crunchset(void*), guiroset(void*), guiro(void*);
int    sekere(void*), tambourset(void*), tambourine(void*), bambooset(void*);
int    bamboo(void*), wuterset(void*), wuter(void*), sleighset(void*), sleighbells(void*);

int    trig_set(void*), trig(void*), numsamp(void*), ftsr(void*);
int    kon2_set(void*), kon2(void*);
int    nrpn(void*), mdelay(void*), mdelay_set(void*);
#if defined(TCLTK)
int    cntrl_set(void*), control(void*), ocontrol(void*);
int    button_set(void*), button(void*), check_set(void*), check(void*);
#endif

int    sum(void*), product(void*), macset(void*), mac(void*), maca(void*);
int    nestedapset(void*), nestedap(void*);
int    lorenzset(void*), lorenz(void*);
int    filelen(void*), filenchnls(void*), filesr(void*), filepeak(void*);
int    ipowoftwo(void*), ilogbasetwo(void*);
int    powoftwo_set(void*), logbasetwo_set(void*);
int    powoftwo(void*), powoftwoa(void*);
int    logbasetwo(void*), logbasetwoa(void*);
/* int    nlalp_set(void*), nlalp(void*); */
int    lp2_set(void*), lp2(void*);
int    phaser2set(void*), phaser2(void*);
int    phaser1set(void*), phaser1(void*);
int    balnset(void*), balance(void*);
int    cvset(void*), convolve(void*);
int    pconvset(void*), pconvolve(void*);

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
                M       begins an indef list of args (any count and rate) IV 2002/9/1
                n       begins an indef list of iargs (nargs odd)
                o       optional, defaulting to 0
                p       "            "       1
                q       "            "       10
                v       "            "       .5
                j       "            "       -1
                h       "            "       127
                y       begins indef list of aargs (any count)
                z       begins indef list of kargs (any count)
                Z       begins alternating kakaka...list (any count)
   outarg types include:
                m       multiple outargs (1 to 4 allowed)
   (these types must agree with rdorch.c)                       */

/* If dsblksize is 0xffff then translate */
/*                 0xfffe then translate two (oscil) */
/*                 0xfffd then translate two (peak) */
/*                 0xfffc then translate two (divz) */

OENTRY opcodlst_1[] = {
/* opcode   dspace      thread  outarg  inargs  isub    ksub    asub    */
{ ""                                                                    },
{ "instr",  0,          0,      "",     ""                              },
{ "endin",  0,          0,      "",     ""                              },
/* IV - Sep 8 2002 */
{ "opcode", 0,          0,      "",     "",     NULL, NULL, NULL        },
{ "endop",  0,          0,      "",     "",     NULL, NULL, NULL        },
{ "$label", S(LBLBLK),  0,      "",     ""                              },
{ "strset", S(STRNG),   0,      "",     "iS"    /* oload time only */   },
{ "pset",   S(PVSET),   0,      "",     "m"                             },
{ "ftgen",  S(FTGEN),   1,      "i",   "iiiiSm",ftgen                   },
{ "ctrlinit",S(CTLINIT),1,      "",     "im",   ctrlinit                },
{ "massign",S(MASSIGN), 1,      "",     "iS",   massign                 },
{ "turnon", S(TURNON),  1,      "",     "io",   turnon                  },
{ "=",      0,          0,      "",     ""                              },
{ "init",   0xffff      /* base names for later prefixes,suffixes */    },
{ "betarand", 0xffff                                                    },
{ "bexprnd", 0xffff                                                     },
{ "cauchy",  0xffff                                                     },
{ "chanctrl",0xffff                                                     },
{ "cpsmidib",0xffff                                                     },
{ "exprand", 0xffff                                                     },
{ "gauss" ,  0xffff                                                     },
{ "limit", 0xffff,                                                      },
{ "linrand", 0xffff                                                     },
{ "midictrl",0xffff                                                     },
{ "polyaft",0xffff                                                      },
{ "ntrpol", 0xffff                                                      },
{ "octmidib",0xffff                                                     },
{ "pcauchy", 0xffff                                                     },
{ "pchbend",0xffff                                                      },
{ "pchmidib",0xffff                                                     },
{ "poisson", 0xffff                                                     },
{ "pow",    0xffff,                                                     },
{ "tableng", 0xffff,                                                    },
{ "taninv2", 0xffff                                                     },
{ "timek", 0xffff,                                                      },
{ "times", 0xffff,                                                      },
{ "trirand", 0xffff                                                     },
{ "unirand",0xffff,                                                     },
{ "weibull", 0xffff                                                     },
{ "oscil",  0xfffe                                                      },
{ "oscil3", 0xfffe                                                      },
{ "oscili", 0xfffe                                                      },
{ "peak", 0xfffd                                                        },
{ "rtclock", 0xfffd                                                     },
{ "tablew",  0xfffe                                                     },
{ "ihold",  S(LINK),    1,      "",     "",     ihold                   },
{ "turnoff",S(LINK),    2,      "",     "",     NULL,   turnoff         },
{ "=_r",    S(ASSIGN),  1,      "r",    "i",    rassign                 },
{ "=_i",    S(ASSIGN),  1,      "i",    "i",    assign                  },
{ "=_k",    S(ASSIGN),  2,      "k",    "k",    NULL,   assign          },
{ "=_a",    S(ASSIGN),  4,      "a",    "x",    NULL,   NULL,   aassign },
{ "init_i",  S(ASSIGN), 1,      "i",    "i",    init                    },
{ "init_k",  S(ASSIGN), 1,      "k",    "i",    init                    },
{ "init_a",  S(ASSIGN), 1,      "a",    "i",    ainit                   },
{ ">",      S(RELAT),   0,      "B",    "kk",   gt,     gt              },
{ ">=",     S(RELAT),   0,      "B",    "kk",   ge,     ge              },
{ "<",      S(RELAT),   0,      "B",    "kk",   lt,     lt              },
{ "<=",     S(RELAT),   0,      "B",    "kk",   le,     le              },
{ "==",     S(RELAT),   0,      "B",    "kk",   eq,     eq              },
{ "!=",     S(RELAT),   0,      "B",    "kk",   ne,     ne              },
{ "&&",     S(LOGCL),   0,      "B",    "BB",   and,    and             },
{ "||",     S(LOGCL),   0,      "B",    "BB",   or,     or              },
{ ":i",     S(CONVAL),  1,      "i",    "bii",  conval                  },
{ ":k",     S(CONVAL),  2,      "k",    "Bkk",  NULL,   conval          },
{ ":a",     S(CONVAL),  4,      "a",    "Bxx",  NULL,   NULL,   aconval },
{ "add_ii",  S(AOP),    1,      "i",    "ii",   addkk                   },
{ "sub_ii",  S(AOP),    1,      "i",    "ii",   subkk                   },
{ "mul_ii",  S(AOP),    1,      "i",    "ii",   mulkk                   },
{ "div_ii",  S(AOP),    1,      "i",    "ii",   divkk                   },
{ "mod_ii",  S(AOP),    1,      "i",    "ii",   modkk                   },
{ "add_kk",  S(AOP),    2,      "k",    "kk",   NULL,   addkk           },
{ "sub_kk",  S(AOP),    2,      "k",    "kk",   NULL,   subkk           },
{ "mul_kk",  S(AOP),    2,      "k",    "kk",   NULL,   mulkk           },
{ "div_kk",  S(AOP),    2,      "k",    "kk",   NULL,   divkk           },
{ "mod_kk",  S(AOP),    2,      "k",    "kk",   NULL,   modkk           },
{ "add_ka",  S(AOP),    4,      "a",    "ka",   NULL,   NULL,   addka   },
{ "sub_ka",  S(AOP),    4,      "a",    "ka",   NULL,   NULL,   subka   },
{ "mul_ka",  S(AOP),    4,      "a",    "ka",   NULL,   NULL,   mulka   },
{ "div_ka",  S(AOP),    4,      "a",    "ka",   NULL,   NULL,   divka   },
{ "mod_ka",  S(AOP),    4,      "a",    "ka",   NULL,   NULL,   modka   },
{ "add_ak",  S(AOP),    4,      "a",    "ak",   NULL,   NULL,   addak   },
{ "sub_ak",  S(AOP),    4,      "a",    "ak",   NULL,   NULL,   subak   },
{ "mul_ak",  S(AOP),    4,      "a",    "ak",   NULL,   NULL,   mulak   },
{ "div_ak",  S(AOP),    4,      "a",    "ak",   NULL,   NULL,   divak   },
{ "mod_ak",  S(AOP),    4,      "a",    "ak",   NULL,   NULL,   modak   },
{ "add_aa",  S(AOP),    4,      "a",    "aa",   NULL,   NULL,   addaa   },
{ "sub_aa",  S(AOP),    4,      "a",    "aa",   NULL,   NULL,   subaa   },
{ "mul_aa",  S(AOP),    4,      "a",    "aa",   NULL,   NULL,   mulaa   },
{ "div_aa",  S(AOP),    4,      "a",    "aa",   NULL,   NULL,   divaa   },
{ "mod_aa",  S(AOP),    4,      "a",    "aa",   NULL,   NULL,   modaa   },
{ "divz",   0xfffc                                                      },
{ "divz_ii", S(DIVZ),   1,      "i",    "iii",  divzkk, NULL,   NULL    },
{ "divz_kk", S(DIVZ),   2,      "k",    "kkk",  NULL,   divzkk, NULL    },
{ "divz_ak", S(DIVZ),   4,      "a",    "akk",  NULL,   NULL,   divzak  },
{ "divz_ka", S(DIVZ),   4,      "a",    "kak",  NULL,   NULL,   divzka  },
{ "divz_aa", S(DIVZ),   4,      "a",    "aak",  NULL,   NULL,   divzaa  },
{ "int_i",  S(EVAL),    1,      "i",    "i",    int1                    },
{ "frac_i", S(EVAL),    1,      "i",    "i",    frac1                   },
{ "rnd_i",  S(EVAL),    1,      "i",    "i",    rnd1                    },
{ "birnd_i",S(EVAL),    1,      "i",    "i",    birnd1                  },
{ "abs_i",  S(EVAL),    1,      "i",    "i",    abs1                    },
{ "exp_i",  S(EVAL),    1,      "i",    "i",    exp01                   },
{ "log_i",  S(EVAL),    1,      "i",    "i",    log01                   },
{ "sqrt_i", S(EVAL),    1,      "i",    "i",    sqrt1                   },
{ "sin_i",  S(EVAL),    1,      "i",    "i",    sin1                    },
{ "cos_i",  S(EVAL),    1,      "i",    "i",    cos1                    },
{ "tan_i",  S(EVAL),    1,      "i",    "i",    tan1                    },
{ "sininv_i", S(EVAL),  1,      "i",    "i",    asin1                   },
{ "cosinv_i", S(EVAL),  1,      "i",    "i",    acos1                   },
{ "taninv_i", S(EVAL),  1,      "i",    "i",    atan1                   },
{ "taninv2_i",S(AOP),   1,      "i",    "ii",   atan21                  },
{ "log10_i",S(EVAL),    1,      "i",    "i",    log101                  },
{ "sinh_i", S(EVAL),    1,      "i",    "i",    sinh1                   },
{ "cosh_i", S(EVAL),    1,      "i",    "i",    cosh1                   },
{ "tanh_i", S(EVAL),    1,      "i",    "i",    tanh1                   },
{ "int_k",  S(EVAL),    2,      "k",    "k",    NULL,   int1            },
{ "frac_k", S(EVAL),    2,      "k",    "k",    NULL,   frac1           },
{ "rnd_k",  S(EVAL),    2,      "k",    "k",    NULL,   rnd1            },
{ "birnd_k",S(EVAL),    2,      "k",    "k",    NULL,   birnd1          },
{ "abs_k",  S(EVAL),    2,      "k",    "k",    NULL,   abs1            },
{ "exp_k",  S(EVAL),    2,      "k",    "k",    NULL,   exp01           },
{ "log_k",  S(EVAL),    2,      "k",    "k",    NULL,   log01           },
{ "sqrt_k", S(EVAL),    2,      "k",    "k",    NULL,   sqrt1           },
{ "sin_k",  S(EVAL),    2,      "k",    "k",    NULL,   sin1            },
{ "cos_k",  S(EVAL),    2,      "k",    "k",    NULL,   cos1            },
{ "tan_k",  S(EVAL),    2,      "k",    "k",    NULL,   tan1            },
{ "sininv_k", S(EVAL),  2,      "k",    "k",    NULL,   asin1           },
{ "cosinv_k", S(EVAL),  2,      "k",    "k",    NULL,   acos1           },
{ "taninv_k", S(EVAL),  2,      "k",    "k",    NULL,   atan1           },
{ "taninv2_k",S(AOP),   2,      "k",    "kk",   NULL,   atan21          },
{ "sinh_k", S(EVAL),    2,      "k",    "k",    NULL,   sinh1           },
{ "cosh_k", S(EVAL),    2,      "k",    "k",    NULL,   cosh1           },
{ "tanh_k", S(EVAL),    2,      "k",    "k",    NULL,   tanh1           },
{ "log10_k",S(EVAL),    2,      "k",    "k",    NULL,   log101          },
{ "abs_a",  S(EVAL),    4,      "a",    "a",    NULL,   NULL,   absa    },
{ "exp_a",  S(EVAL),    4,      "a",    "a",    NULL,   NULL,   expa    },
{ "log_a",  S(EVAL),    4,      "a",    "a",    NULL,   NULL,   loga    },
{ "sqrt_a", S(EVAL),    4,      "a",    "a",    NULL,   NULL,   sqrta   },
{ "sin_a",  S(EVAL),    4,      "a",    "a",    NULL,   NULL,   sina    },
{ "cos_a",  S(EVAL),    4,      "a",    "a",    NULL,   NULL,   cosa    },
{ "tan_a",  S(EVAL),    4,      "a",    "a",    NULL,   NULL,   tana    },
{ "sininv_a", S(EVAL),  4,      "a",    "a",    NULL,   NULL,   asina   },
{ "cosinv_a", S(EVAL),  4,      "a",    "a",    NULL,   NULL,   acosa   },
{ "taninv_a", S(EVAL),  4,      "a",    "a",    NULL,   NULL,   atana   },
{ "taninv2_a",S(AOP),   4,      "a",    "aa",   NULL,   NULL,   atan2aa },
{ "sinh_a", S(EVAL),    4,      "a",    "a",    NULL,   NULL,   sinha   },
{ "cosh_a", S(EVAL),    4,      "a",    "a",    NULL,   NULL,   cosha   },
{ "tanh_a", S(EVAL),    4,      "a",    "a",    NULL,   NULL,   tanha   },
{ "log10_a",S(EVAL),    4,      "a",    "a",    NULL,   NULL,   log10a  },
{ "ampdb_a",S(EVAL),    4,      "a",    "a",    NULL,   NULL,   aampdb  },
{ "ampdb_i",S(EVAL),    1,      "i",    "i",    ampdb                   },
{ "ampdb_k",S(EVAL),    2,      "k",    "k",    NULL,   ampdb           },
{ "ampdbfs_a",S(EVAL),  4,      "a",    "a",    NULL,   NULL,   aampdbfs },
{ "ampdbfs_i",S(EVAL),  1,      "i",    "i",    ampdbfs                 },
{ "ampdbfs_k",S(EVAL),  2,      "k",    "k",    NULL,   ampdbfs         },
{ "dbamp_i",S(EVAL),    1,      "i",    "i",    dbamp                   },
{ "dbamp_k",S(EVAL),    2,      "k",    "k",    NULL,   dbamp           },
{ "dbfsamp_i",S(EVAL),  1,      "i",    "i",    dbfsamp                 },
{ "dbfsamp_k",S(EVAL),  2,      "k",    "k",    NULL,   dbfsamp         },
{ "rtclock_i",S(EVAL),  1,      "i",    "",     rtclock                 },
{ "rtclock_k",S(EVAL),  2,      "k",    "",     NULL,   rtclock         },
{ "ftlen_i",S(EVAL),    1,      "i",    "i",    ftlen                   },
{ "ftsr_i",S(EVAL),     1,      "i",    "i",    ftsr                    },
{ "ftlptim_i",S(EVAL),  1,      "i",    "i",    ftlptim                 },
{ "ftchnls_i",S(EVAL),  1,      "i",    "i",    ftchnls                 },
{ "i_k",   S(ASSIGN),   1,      "i",    "k",    init                    },
{ "cpsoct_i",S(EVAL),   1,      "i",    "i",    cpsoct                  },
{ "octpch_i",S(EVAL),   1,      "i",    "i",    octpch                  },
{ "cpspch_i",S(EVAL),   1,      "i",    "i",    cpspch                  },
{ "pchoct_i",S(EVAL),   1,      "i",    "i",    pchoct                  },
{ "octcps_i",S(EVAL),   1,      "i",    "i",    octcps                  },
{ "cpsoct_k",S(EVAL),   2,      "k",    "k",    NULL,   cpsoct          },
{ "octpch_k",S(EVAL),   2,      "k",    "k",    NULL,   octpch          },
{ "cpspch_k",S(EVAL),   2,      "k",    "k",    NULL,   cpspch          },
{ "pchoct_k",S(EVAL),   2,      "k",    "k",    NULL,   pchoct          },
{ "octcps_k",S(EVAL),   2,      "k",    "k",    NULL,   octcps          },
{ "cpsoct_a",S(EVAL),   4,      "a",    "a",    NULL,   NULL,   acpsoct },
{ "notnum", S(MIDIKMB), 1,      "i",    "",     notnum                  },
{ "veloc",  S(MIDIMAP), 1,      "i",    "oh",   veloc                   },
{ "pchmidi",S(MIDIKMB), 1,      "i",    "",     pchmidi                 },
{ "octmidi",S(MIDIKMB), 1,      "i",    "",     octmidi                 },
{ "cpsmidi",S(MIDIKMB), 1,      "i",    "",     cpsmidi                 },
{ "pchmidib_i",S(MIDIKMB),1,    "i",    "o",    pchmidib_i              },
{ "octmidib_i",S(MIDIKMB),1,    "i",    "o",    octmidib_i              },
{ "cpsmidib_i",S(MIDIKMB),1,    "i",    "o",    icpsmidib_i             },
{ "pchmidib_k",S(MIDIKMB),3,    "k",    "o",    midibset, pchmidib      },
{ "octmidib_k",S(MIDIKMB),3,    "k",    "o",    midibset, octmidib      },
{ "cpsmidib_k",S(MIDIKMB),3,    "k",    "o",    midibset, icpsmidib     },
{ "ampmidi",S(MIDIAMP), 1,      "i",    "io",   ampmidi                 },
{ "aftouch",S(MIDIKMAP), 3,     "k",    "oh",   aftset, aftouch         },
{ "pchbend_i",S(MIDIMAP),0x21,  "i",    "jp",   ipchbend                },
{ "pchbend_k",S(MIDIKMAP),0x23, "k",    "jp",   kbndset,kpchbend        },
{ "midictrl_i",S(MIDICTL),1,    "i",    "ioh",  imidictl                },
{ "midictrl_k",S(MIDICTL),3,    "k",    "ioh",  mctlset, midictl        },
{ "polyaft_i",S(MIDICTL),1,     "i",    "ioh",  imidiaft                },
{ "polyaft_k",S(MIDICTL),3,     "k",    "ioh",  maftset, midiaft        },
{ "chanctrl_i",S(CHANCTL),1,    "i",    "iioh", ichanctl                },
{ "chanctrl_k",S(CHANCTL),3,    "k",    "iioh", chctlset,chanctl        },
{ "line",   S(LINE),    7,      "s",    "iii",  linset, kline,  aline   },
{ "expon",  S(EXPON),   7,      "s",    "iii",  expset, kexpon, expon   },
{ "linseg", S(LINSEG),  7,      "s",    "iin",  lsgset, klnseg, linseg  },
{ "linsegr",S(LINSEG),  7,      "s",    "iin",  lsgrset,klnsegr,linsegr },
{ "expseg", S(EXXPSEG),  7,     "s",    "iin",  xsgset, kxpseg, expseg  },
{ "expsega",S(EXPSEG2),  7,     "a",    "iin",  xsgset2, NULL, expseg2  },
{ "expsegr",S(EXPSEG),  7,      "s",    "iin",  xsgrset,kxpsegr,expsegr },
{ "linen",  S(LINEN),   7,      "s",    "xiii", lnnset, klinen, linen   },
{ "linenr", S(LINENR),  7,      "s",    "xiii", lnrset, klinenr,linenr  },
{ "envlpx", S(ENVLPX),  7,      "s","xiiiiiio", evxset, knvlpx, envlpx  },
{ "envlpxr", S(ENVLPR), 7,      "s","xiiiiioo", evrset, knvlpxr,envlpxr },
{ "phasor", S(PHSOR),   7,      "s",    "xo",   phsset, kphsor, phsor   },
{ "table_i", S(TABLE),  1,      "i",    "iiooo",itable                  },
{ "tablei_i", S(TABLE), 1,      "i",    "iiooo",itabli                  },
{ "table3_i", S(TABLE), 1,      "i",    "iiooo",itabl3                  },
{ "table",  S(TABLE),   7,      "s",    "xiooo",tblset, ktable, tablefn },
{ "tablei", S(TABLE),   7,      "s",    "xiooo",tblset, ktabli, tabli   },
{ "table3", S(TABLE),   7,      "s",    "xiooo",tblset, ktabl3, tabl3   },
{ "oscil1", S(OSCIL1),  3,      "k",    "ikii", ko1set, kosc1           },
{ "oscil1i",S(OSCIL1),  3,      "k",    "ikii", ko1set, kosc1i          },
{ "osciln", S(OSCILN),  5,      "a",    "kiii", oscnset,NULL,   osciln  },
{ "oscil_kk",S(OSC),    7,      "s",    "kkio", oscset, koscil, osckk   },
{ "oscil_ka",S(OSC),    5,      "a",    "kaio", oscset, NULL,   oscka   },
{ "oscil_ak",S(OSC),    5,      "a",    "akio", oscset, NULL,   oscak   },
{ "oscil_aa",S(OSC),    5,      "a",    "aaio", oscset, NULL,   oscaa   },
{ "oscili_kk",S(OSC),   7,      "s",    "kkio", oscset, koscli, osckki  },
{ "oscili_ka",S(OSC),   5,      "a",    "kaio", oscset, NULL,   osckai  },
{ "oscili_ak",S(OSC),   5,      "a",    "akio", oscset, NULL,   oscaki  },
{ "oscili_aa",S(OSC),   5,      "a",    "aaio", oscset, NULL,   oscaai  },
{ "oscil3_kk",S(OSC),   7,      "s",    "kkio", oscset, koscl3, osckk3  },
{ "oscil3_ka",S(OSC),   5,      "a",    "kaio", oscset, NULL,   oscka3  },
{ "oscil3_ak",S(OSC),   5,      "a",    "akio", oscset, NULL,   oscak3  },
{ "oscil3_aa",S(OSC),   5,      "a",    "aaio", oscset, NULL,   oscaa3  },
{ "foscil", S(FOSC),    5,      "a",  "xkxxkio",foscset,NULL,   foscil  },
{ "foscili",S(FOSC),    5,      "a",  "xkxxkio",foscset,NULL,   foscili },
{ "loscil", S(LOSC),    5,      "mm","xkiojoojoo",losset,NULL, loscil   },
{ "loscil3", S(LOSC),   5,      "mm","xkiojoojoo",losset,NULL, loscil3  },
{ "adsyn",  S(ADSYN),   5,      "a",    "kkkSo", adset, NULL,   adsyn   },
{ "pvoc",   S(PVOC),    5,      "a",  "kkSoooo", pvset, NULL,   pvoc    },
{ "buzz",   S(BUZZ),    5,      "a",  "xxkio",  bzzset, NULL,   buzz    },
{ "gbuzz",  S(GBUZZ),   5,      "a",  "xxkkkio",gbzset, NULL,   gbuzz   },
{ "pluck",  S(PLUCK),   5,      "a",  "kkiiioo",plukset,NULL,   pluck   },
{ "rand",   S(RAND),    7,      "s",    "xvoo", rndset, krand,  arand   },
{ "randh",  S(RANDH),   7,      "s",    "xxvoo", rhset, krandh, randh   },
{ "randi",  S(RANDI),   7,      "s",    "xxvoo", riset, krandi, randi   },
{ "port",   S(PORT),    3,      "k",    "kio",  porset, port            },
{ "tone",   S(TONE),    5,      "a",    "ako",  tonset, NULL,   tone    },
{ "tonex",  S(TONEX),   5,      "a",    "akoo", tonsetx, NULL,  tonex   },
{ "atone",  S(TONE),    5,      "a",    "ako",  tonset, NULL,   atone   },
{ "atonex", S(TONEX),   5,      "a",    "akoo", tonsetx, NULL,  atonex  },
{ "reson",  S(RESON),   5,      "a",    "akkoo",rsnset, NULL,   reson   },
{ "resonx", S(RESONX),  5,      "a",    "akkooo", rsnsetx, NULL, resonx },
{ "areson", S(RESON),   5,      "a",    "akkoo",rsnset, NULL,   areson  },
{ "lpread", S(LPREAD),  3,      "kkkk", "kSoo", lprdset,lpread          },
{ "lpreson",S(LPRESON), 5,      "a",    "a",    lprsnset,NULL,  lpreson },
{ "lpfreson",S(LPFRESON),5,     "a",    "ak",   lpfrsnset,NULL, lpfreson},
{ "lpslot"  ,  S(LPSLOT),  1,   "",     "i",    lpslotset, NULL, NULL   },
{ "lpinterp", S(LPINTERPOL), 3, "",     "iik",  lpitpset, lpinterpol, NULL},
{ "rms",    S(RMS),     3,      "k",    "aqo",  rmsset, rms             },
{ "gain",   S(GAIN),    5,      "a",    "akqo", gainset,NULL,   gain    },
{ "balance",S(BALANCE), 5,      "a",    "aaqo", balnset,NULL,   balance },
{ "pan",    S(PAN),   5, "aaaa", "akkioo",(SUBR)panset,NULL,   (SUBR)pan     },
{ "reverb", S(REVERB),  5, "a",  "ako",  (SUBR)rvbset, NULL,   (SUBR)reverb  },
{ "delayw", S(DELAYW),  5,  "",  "a",    (SUBR)delwset,NULL,   (SUBR)delayw  },
{ "soundin",S(SOUNDIN), 5,"mmmmmmmmmmmmmmmmmmmmmmmm","Soo",sndinset,NULL,soundin },
{ "soundout",S(SNDOUT), 5,      "",     "aSo",  sndo1set,NULL,  soundout},
/* { "soundouts",S(SNDOUTS),5,     "",     "aaSo", sndo2set,NULL,  soundouts}, */
{ "in",     S(INM),     4,      "a",    "",     NULL,   NULL,   in      },
{ "ins",    S(INS),     4,      "aa",   "",     NULL,   NULL,   ins     },
{ "inq",    S(INQ),     4,      "aaaa", "",     NULL,   NULL,   inq     },
  /* Note that there is code in rdorch.c that assumes that opcodes starting
     with the charcters out followed by a s, q, h, o or x are in this group
     ***BEWARE***
   */
{ "out",    S(OUTM),    4,      "",     "a",    NULL,   NULL,   out     },
{ "outs",   S(OUTS),    4,      "",     "aa",   NULL,   NULL,   outs    },
{ "outq",   S(OUTQ),    4,      "",     "aaaa", NULL,   NULL,   outq    },
{ "outs1",  S(OUTM),    4,      "",     "a",    NULL,   NULL,   outs1   },
{ "outs2",  S(OUTM),    4,      "",     "a",    NULL,   NULL,   outs2   },
{ "outq1",  S(OUTM),    4,      "",     "a",    NULL,   NULL,   outq1   },
{ "outq2",  S(OUTM),    4,      "",     "a",    NULL,   NULL,   outq2   },
{ "outq3",  S(OUTM),    4,      "",     "a",    NULL,   NULL,   outq3   },
{ "outq4",  S(OUTM),    4,      "",     "a",    NULL,   NULL,   outq4   },
{ "igoto",  S(GOTO),    1,      "",     "l",    igoto                   },
{ "kgoto",  S(GOTO),    2,      "",     "l",    NULL,   kgoto           },
{ "goto",   S(GOTO),    3,      "",     "l",    igoto,  kgoto           },
{ "cigoto", S(CGOTO),   1,      "",     "Bl",   icgoto                  },
{ "ckgoto", S(CGOTO),   2,      "",     "Bl",   NULL,   kcgoto          },
{ "cggoto", S(CGOTO),   3,      "",     "Bl",   icgoto, kcgoto          },
{ "timout", S(TIMOUT),  3,      "",     "iil",  timset, timout          },
{ "reinit", S(GOTO),    2,      "",     "l",    NULL,   reinit          },
{ "rigoto", S(GOTO),    1,      "",     "l",    rigoto                  },
{ "rireturn",S(LINK),   1,      "",     "",     rireturn                },
{ "tigoto", S(GOTO),    1,      "",     "l",    tigoto                  },
{ "tival",  S(EVAL),    1,      "i",    "",     tival                   },
{ "print",  S(PRINTV),  1,      "",     "m",    printv                  },
{ "display",S(DSPLAY),  7,      "",     "sioo", dspset, kdsplay,dsplay  },
{ "dispfft",S(DSPFFT),  7,      "",     "siiooo",fftset,kdspfft,dspfft  },
{ "dumpk",  S(KDUMP),   3,      "",     "kSii", kdmpset,kdump           },
{ "dumpk2", S(KDUMP2),  3,      "",     "kkSii",kdmp2set,kdump2         },
{ "dumpk3", S(KDUMP3),  3,      "",     "kkkSii",kdmp3set,kdump3        },
{ "dumpk4", S(KDUMP4),  3,      "",     "kkkkSii",kdmp4set,kdump4       },
{ "readk",  S(KREAD),   3,      "k",    "Siio",  krdset, kread          },
{ "readk2", S(KREAD2),  3,      "kk",   "Siio",  krd2set, kread2        },
{ "readk3", S(KREAD3),  3,      "kkk",  "Siio",  krd3set, kread3        },
{ "readk4", S(KREAD4),  3,      "kkkk", "Siio",  krd4set, kread4        },
{ "xyin",     S(XYIN),    3,    "kk",   "iiiiioo",xyinset,xyin          },
{ "tempest",  S(TEMPEST), 5,    "k","kiiiiiiiiiop",tempeset,NULL,tempest},
{ "tempo",    S(TEMPO),   3,    "",     "ki",   tempset,tempo           },
{ "pow_i",    S(POW),   1,      "i",    "iip",  ipow,    NULL,  NULL    },
{ "pow_k",    S(POW),   2,      "k",    "kkp",  NULL,    ipow,  NULL    },
{ "pow_a",    S(POW),   4,      "a",    "akp",  NULL,    NULL,  apow    },
{ "oscilx",   S(OSCILN),5,      "a",    "kiii", oscnset,NULL,   osciln  },
{ "linrand_i",S(PRAND), 1,      "i",    "k",    iklinear, NULL, NULL    },
{ "linrand_k",S(PRAND), 2,      "k",    "k",    NULL, iklinear, NULL    },
{ "linrand_a",S(PRAND), 4,      "a",    "k",    NULL, NULL,     alinear },
{ "trirand_i",S(PRAND), 1,      "i",    "k",    iktrian, NULL,  NULL    },
{ "trirand_k",S(PRAND), 2,      "k",    "k",    NULL, iktrian,  NULL    },
{ "trirand_a",S(PRAND), 4,      "a",    "k",    NULL, NULL,     atrian  },
{ "exprand_i",S(PRAND), 1,      "i",    "k",    ikexp, NULL,    NULL    },
{ "exprand_k",S(PRAND), 2,      "k",    "k",    NULL,    ikexp, NULL    },
{ "exprand_a",S(PRAND), 4,      "a",    "k",    NULL, NULL,     aexp    },
{ "bexprnd_i",S(PRAND), 1,      "i",    "k",    ikbiexp, NULL,  NULL    },
{ "bexprnd_k",S(PRAND), 2,      "k",    "k",    NULL, ikbiexp,  NULL    },
{ "bexprnd_a",S(PRAND), 4,      "a",    "k",    NULL, NULL,     abiexp  },
{ "cauchy_i", S(PRAND), 1,      "i",    "k",    ikcauchy, NULL, NULL    },
{ "cauchy_k", S(PRAND), 2,      "k",    "k",    NULL, ikcauchy, NULL    },
{ "cauchy_a", S(PRAND), 4,      "a",    "k",    NULL,    NULL,  acauchy },
{ "pcauchy_i",S(PRAND), 1,      "i",    "k",    ikpcauchy, NULL,NULL    },
{ "pcauchy_k",S(PRAND), 2,      "k",    "k",    NULL, ikpcauchy,NULL    },
{ "pcauchy_a",S(PRAND), 4,      "a",    "k",    NULL,    NULL,  apcauchy},
{ "poisson_i",S(PRAND), 1,      "i",    "k",    ikpoiss, NULL,  NULL    },
{ "poisson_k",S(PRAND), 2,      "k",    "k",    NULL, ikpoiss,  NULL    },
{ "poisson_a",S(PRAND), 4,      "a",    "k",    NULL,    NULL,  apoiss  },
{ "gauss_i" , S(PRAND), 1,      "i",    "k",    ikgaus,  NULL,  NULL    },
{ "gauss_k" , S(PRAND), 2,      "k",    "k",    NULL, ikgaus,   NULL    },
{ "gauss_a" , S(PRAND), 4,      "a",    "k",    NULL,    NULL,  agaus   },
{ "weibull_i",S(PRAND), 1,      "i",    "kk",   ikweib,  NULL,  NULL    },
{ "weibull_k",S(PRAND), 2,      "k",    "kk",   NULL, ikweib,   NULL    },
{ "weibull_a",S(PRAND), 4,      "a",    "kk",   NULL,    NULL,  aweib   },
{ "betarand_i",S(PRAND),1,      "i",    "kkk",  ikbeta, NULL,  NULL     },
{ "betarand_k",S(PRAND),2,      "k",    "kkk",  NULL,   ikbeta,NULL     },
{ "betarand_a",S(PRAND),4,      "a",    "kkk",  NULL,   NULL,  abeta      },
{ "seed",     S(PRAND), 1,      "",     "i",    seedrand, NULL, NULL      },
{ "convolve", S(CONVOLVE), 5,   "mmmm", "aSo",  cvset,   NULL,  convolve  },
{ "convle",   S(CONVOLVE), 5,   "mmmm", "aSo",  cvset,   NULL,  convolve  },
{ "pconvolve",S(PCONVOLVE), 5, "mmmm", "aSoo",  pconvset, NULL, pconvolve },
{ "tableseg", S(TABLESEG), 3,  "",      "iin",  tblesegset, ktableseg},
{ "ktableseg", S(TABLESEG), 3,  "",     "iin",  tblesegset, ktableseg},
{ "tablexseg", S(TABLESEG), 3, "",      "iin",  tblesegset, ktablexseg},
{ "vpvoc",    S(VPVOC), 5,     "a",     "kkSoo", vpvset,        NULL,   vpvoc},
{ "pvread",   S(PVREAD),3,     "kk",    "kSi",  pvreadset, pvread},
{ "pvcross",  S(PVCROSS),  5,  "a",     "kkSkko", pvcrossset, NULL, pvcross},
{ "pvbufread",S(PVBUFREAD), 3, "",      "kS",   pvbufreadset, pvbufread, NULL},
{ "pvinterp", S(PVINTERP), 5,  "a",  "kkSkkkkkk", pvinterpset, NULL, pvinterp},
{ "pvadd",    S(PVADD), 5,     "a", "kkSiiopooo", pvaddset,     NULL,   pvadd},
{ "unirand_i",S(PRAND), 1,     "i",     "k",    ikuniform, NULL,  NULL  },
{ "unirand_k",S(PRAND), 2,     "k",     "k",    NULL,    ikuniform, NULL},
{ "unirand_a",S(PRAND), 4,     "a",     "k",    NULL,    NULL, auniform },
{ "diskin",S(SOUNDINEW),5,  "mmmm",     "Skooo", newsndinset,NULL, soundinew},
{ "noteon", S(OUT_ON),  1,      "",     "iii",  iout_on, NULL,   NULL   },
{ "noteoff", S(OUT_ON), 1,      "",     "iii",  iout_off, NULL,    NULL },
{ "noteondur",S(OUT_ON_DUR),3,  "", "iiii", iout_on_dur_set,iout_on_dur,NULL},
{ "noteondur2",S(OUT_ON_DUR),3, "", "iiii", iout_on_dur_set,iout_on_dur2,NULL},
{ "moscil",S(MOSCIL),   3,      "",     "kkkkk",moscil_set, moscil, NULL},
{ "midion",S(KOUT_ON),  3,      "", "kkk", kvar_out_on_set,kvar_out_on,NULL},
{ "outic",S(OUT_CONTR), 1,      "",     "iiiii", out_controller, NULL, NULL},
{ "outkc",S(OUT_CONTR), 2,      "",     "kkkkk", NULL, out_controller, NULL},
{ "outic14",S(OUT_CONTR14),1,   "",     "iiiiii",out_controller14, NULL,NULL},
{ "outkc14",S(OUT_CONTR14),2,   "",     "kkkkkk",NULL, out_controller14, NULL},
{ "outipb",S(OUT_PB),   1,      "",     "iiii", out_pitch_bend, NULL , NULL},
{ "outkpb",S(OUT_PB),   2,      "",     "kkkk", NULL,  out_pitch_bend, NULL},
{ "outiat",S(OUT_ATOUCH),1,     "",     "iiii", out_aftertouch, NULL , NULL},
{ "outkat",S(OUT_ATOUCH),2,     "",     "kkkk", NULL,  out_aftertouch, NULL},
{ "outipc",S(OUT_PCHG), 1,      "",     "iiii", out_progchange, NULL , NULL},
{ "outkpc",S(OUT_PCHG), 2,      "",     "kkkk", NULL,  out_progchange, NULL},
{ "outipat",S(OUT_POLYATOUCH),1,"",    "iiiii", out_poly_aftertouch, NULL,NULL},
{ "outkpat",S(OUT_POLYATOUCH),2,"",    "kkkkk", NULL, out_poly_aftertouch,NULL},
{ "release",S(REL),     3,      "k",    "",     release_set, release , NULL },
{ "xtratim",S(XTRADUR), 1,      "",     "i",    xtratim,    NULL,     NULL },
{ "mclock", S(MCLOCK),  3,      "",     "i",    mclock_set, mclock,   NULL },
{ "mrtmsg", S(XTRADUR), 1,      "",     "i",    mrtmsg,     NULL,     NULL },
{ "midiout",S(MIDIOUT),  2,     "",     "kkkk", NULL, midiout,   NULL      },
{ "midion2", S(KON2),    3,     "",     "kkkk", kon2_set, kon2,   NULL     },
{ "nrpn",   S(NRPN),     2,     "",     "kkk",  NULL,  nrpn ,NULL          },
{ "mdelay", S(MDELAY),   3,     "",     "kkkkk",mdelay_set, mdelay,   NULL },
{ "nsamp_i", S(EVAL),    1,     "i",    "i",    numsamp                    },
{ "powoftwo_i",S(EVAL),  1,     "i",    "i",    ipowoftwo                  },
{ "powoftwo_k",S(EVAL),  2,     "k",    "k",    powoftwo_set, powoftwo     },
{ "powoftwo_a",S(EVAL),  4,     "a",    "a",  powoftwo_set, NULL, powoftwoa },
{ "logbtwo_i",S(EVAL),   1,     "i",    "i",    ilogbasetwo                },
{ "logbtwo_k",S(EVAL),   2,     "k",    "k",    logbasetwo_set, logbasetwo },
{ "logbtwo_a",S(EVAL),   4,     "a",    "a", logbasetwo_set, NULL, logbasetwoa },
{ "filelen", S(SNDINFO), 1,     "i",    "S",    filelen, NULL, NULL        },
{ "filenchnls", S(SNDINFO), 1,  "i",    "S",    filenchnls, NULL, NULL     },
{ "filesr", S(SNDINFO),  1,     "i",    "S",    filesr, NULL, NULL         },
{ "filepeak", S(SNDINFOPEAK), 1, "i",   "So",   filepeak, NULL, NULL       },
/*  { "nlalp", S(NLALP),     5,     "a",    "akkoo", nlalp_set, NULL, nlalp     }, */
/* Robin Whittle */
{ "tableiw",  S(TABLEW),1,     "",      "iiiooo", (SUBR)itablew, NULL, NULL},
{ "tablew_kk", S(TABLEW),3,    "",      "kkiooo",(SUBR)tblsetw,(SUBR)ktablew, NULL},
{ "tablew_aa", S(TABLEW),5,    "",      "aaiooo",(SUBR)tblsetw, NULL, (SUBR)tablew},
{ "tablewkt_kk", S(TABLEW),3, "",     "kkkooo",(SUBR)tblsetwkt,(SUBR)ktablewkt,NULL},
{ "tablewkt_aa", S(TABLEW),5, "",     "aakooo",(SUBR)tblsetwkt,NULL,(SUBR)tablewkt},
{ "tableng_i", S(TABLENG),1,  "i",     "i",    (SUBR)itableng, NULL,  NULL},
{ "tableng_k",  S(TABLENG),2, "k",     "k",    NULL,   (SUBR)tableng, NULL},
{ "tableigpw",S(TABLENG),1,   "",  "i",    (SUBR)itablegpw, NULL,  NULL},
{ "tablegpw", S(TABLENG),2,   "",  "k",    NULL,   (SUBR)tablegpw, NULL},
{ "tableimix",S(TABLEMIX),1,  "",  "iiiiiiiii", (SUBR)itablemix, NULL, NULL},
{ "tablemix", S(TABLEMIX),2,  "",  "kkkkkkkkk",(SUBR)tablemixset, (SUBR)tablemix, NULL},
{ "tableicopy",S(TABLECOPY),1, "", "ii",   (SUBR)itablecopy, NULL, NULL},
{ "tablecopy", S(TABLECOPY),2, "", "kk",   (SUBR)tablecopyset, (SUBR)tablecopy, NULL},
{ "tablera", S(TABLERA),5,   "a",  "kkk",  (SUBR)tableraset, NULL, (SUBR)tablera},
{ "tablewa", S(TABLEWA),5,   "k",  "kak",  (SUBR)tablewaset, NULL, (SUBR)tablewa},
{ "tablekt",  S(TABLE), 7,     "s",  "xkooo",tblsetkt,  ktablekt, tablekt },
{ "tableikt", S(TABLE), 7,     "s",  "xkooo",tblsetkt,  ktablikt, tablikt },
{ "zakinit", S(ZAKINIT), 1,  "",   "ii",   (SUBR)zakinit, NULL,  NULL},
{ "zir",    S(ZKR),     1,   "i",  "i",    (SUBR)zir,     NULL,  NULL},
{ "zkr",    S(ZKR),     3,   "k",  "k",    (SUBR)zkset,   (SUBR)zkr,   NULL},
{ "ziw",    S(ZKW),     1,   "",   "ii",   (SUBR)ziw,     NULL,  NULL},
{ "zkw",    S(ZKW),     3,   "",   "kk",   (SUBR)zkset,   (SUBR)zkw,   NULL},
{ "ziwm",   S(ZKWM),    1,   "",   "iip",  (SUBR)ziwm,    NULL,  NULL},
{ "zkwm",   S(ZKWM),    3,   "",   "kkp",  (SUBR)zkset,   (SUBR)zkwm,  NULL},
{ "zkmod",  S(ZKMOD),   2,   "k",  "kk",   NULL,    (SUBR)zkmod, NULL},
{ "zkcl",   S(ZKCL),    3,   "",  "kk",   (SUBR)zkset,   (SUBR)zkcl,  NULL},
{ "zar",    S(ZAR),     5,   "a", "k",    (SUBR)zaset,   NULL,  (SUBR)zar},
{ "zarg",   S(ZARG),    5,   "a", "kk",   (SUBR)zaset,   NULL,  (SUBR)zarg},
{ "zaw",    S(ZAW),     5,   "",  "ak",   (SUBR)zaset,   NULL,  (SUBR)zaw},
{ "zawm",   S(ZAWM),    5,   "",  "akp",  (SUBR)zaset,   NULL,  (SUBR)zawm},
{ "zamod",  S(ZAMOD),   4,   "a", "ak",   NULL,    NULL,  (SUBR)zamod},
{ "zacl",   S(ZACL),    5,   "",  "kk",   (SUBR)zaset,   NULL,  (SUBR)zacl},
{ "inz",    S(IOZ),     4,   "",   "k",    (SUBR)zaset,  NULL,   (SUBR)inz  },
{ "outz",   S(IOZ),     4,   "",   "k",    (SUBR)zaset,  NULL,   (SUBR)outz },
{ "timek_i", S(RDTIME), 1,   "i",  "",     (SUBR)timek,   NULL,  NULL },
{ "times_i", S(RDTIME), 1,   "i",  "",     (SUBR)timesr,  NULL,  NULL },
{ "timek_k",  S(RDTIME), 2,  "k",  "",     NULL,    (SUBR)timek, NULL },
{ "times_k",  S(RDTIME), 2,  "k",  "",     NULL,    (SUBR)timesr,NULL },
{ "timeinstk", S(RDTIME), 3, "k",  "",     (SUBR)instimset, (SUBR)instimek, NULL },
{ "timeinsts", S(RDTIME), 3, "k",  "",     (SUBR)instimset, (SUBR)instimes, NULL },
{ "peak_k",  S(PEAK),   2,   "k",  "k",    NULL,    (SUBR)peakk,    NULL },
{ "peak_a",   S(PEAK),  4,   "k",  "a",    NULL,    NULL,     (SUBR)peaka },
{ "printk", S(PRINTK),  3,   "",   "iko",  (SUBR)printkset, (SUBR)printk, NULL },
{ "printks",S(PRINTKS), 3,   "",   "SiM",  (SUBR)printksset,(SUBR)printks, NULL },
{ "prints",S(PRINTS),   1,   "",   "SM",   (SUBR)printsset, NULL, NULL },
{ "printk2", S(PRINTK2),3,   "",   "ko",   (SUBR)printk2set, (SUBR)printk2, NULL },
{ "fprints", S(FPRINTF),1,   "",   "SSM",  (SUBR)fprintf_i, NULL, NULL },
{ "fprintks", S(FPRINTF), 3, "",   "SSM",  (SUBR)fprintf_set, (SUBR)fprintf_k, NULL },
{ "vincr", S(INCR),     4, "",     "aa",   NULL, NULL, (SUBR)incr              },
{ "clear", S(CLEARS),   4, "",     "y",    NULL, NULL, (SUBR)clear             },
{ "fout", S(OUTFILE),   5, "",     "Siy",  (SUBR)outfile_set, NULL, (SUBR)outfile },
{ "foutk", S(KOUTFILE), 3, "",     "Siz",  (SUBR)koutfile_set, (SUBR)koutfile  },
{ "fouti", S(IOUTFILE), 1, "",     "iiim", (SUBR)ioutfile_set                  },
{ "foutir", S(IOUTFILE_R), 3, "",  "iiim", (SUBR)ioutfile_set_r, (SUBR)ioutfile_r },
{ "fiopen", S(FIOPEN),  1, "i",    "Si",   (SUBR)fiopen                        },
{ "fin", S(INFILE),     5, "",     "Siiy", (SUBR)infile_set,  NULL, (SUBR)infile_act },
{ "fink", S(KINFILE),   3, "",     "Siiz", (SUBR)kinfile_set, (SUBR)kinfile    },
{ "fini", S(I_INFILE),  1,  "",     "Siim", (SUBR)i_infile                     },
{ "portk",  S(KPORT),   3, "k",     "kko",  (SUBR)kporset, (SUBR)kport, NULL   },
{ "tonek",  S(KTONE),   3, "k",     "kko",  (SUBR)ktonset, (SUBR)ktone, NULL   },
{ "atonek", S(KTONE),   3, "k",     "kko",  (SUBR)ktonset, (SUBR)katone, NULL  },
{ "resonk", S(KRESON),  3, "k",     "kkkpo",(SUBR)krsnset, (SUBR)kreson, NULL  },
{ "aresonk",S(KRESON),  3, "k",     "kkkpo",(SUBR)krsnset, (SUBR)kareson, NULL },
{ "limit_i", S(LIMIT),  1, "i",     "iii",  (SUBR)klimit,  NULL,    NULL       },
{ "limit_i", S(LIMIT),  1, "i",     "iii",  (SUBR)klimit,  NULL,    NULL       },
{ "limit_k",  S(LIMIT), 3, "k",     "xkk",  (SUBR)limitset, (SUBR)klimit, NULL },
{ "limit_a",  S(LIMIT), 5, "a",     "xkk",  (SUBR)limitset, NULL,  (SUBR)limit },
};

long oplength_1 = sizeof(opcodlst_1);

