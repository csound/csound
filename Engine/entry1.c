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
#include "diskin2.h"
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

int    ihold(void*,void*), turnoff(void*,void*);
int    assign(void*,void*), rassign(void*,void*), aassign(void*,void*);
int    init(void*,void*), ainit(void*,void*);
int    gt(void*,void*), ge(void*,void*), lt(void*,void*), le(void*,void*), eq(void*,void*), ne(void*,void*);
int    and(void*,void*), or(void*,void*), conval(void*,void*), aconval(void*,void*);
int    addkk(void*,void*), subkk(void*,void*), mulkk(void*,void*), divkk(void*,void*), modkk(void*,void*);
int    addka(void*,void*), subka(void*,void*), mulka(void*,void*), divka(void*,void*), modka(void*,void*);
int    addak(void*,void*), subak(void*,void*), mulak(void*,void*), divak(void*,void*), modak(void*,void*);
int    addaa(void*,void*), subaa(void*,void*), mulaa(void*,void*), divaa(void*,void*), modaa(void*,void*);
int    divzkk(void*,void*), divzka(void*,void*), divzak(void*,void*), divzaa(void*,void*);
int    int1(void*,void*), frac1(void*,void*), rnd1(void*,void*), birnd1(void*,void*);
int    abs1(void*,void*), exp01(void*,void*), log01(void*,void*), sqrt1(void*,void*);
int    sin1(void*,void*), cos1(void*,void*), tan1(void*,void*), asin1(void*,void*), acos1(void*,void*);
int    atan1(void*,void*), sinh1(void*,void*), cosh1(void*,void*), tanh1(void*,void*), log101(void*,void*);
int    atan21(void*,void*), atan2aa(void*,void*);
int    absa(void*,void*), expa(void*,void*), loga(void*,void*), sqrta(void*,void*);
int    sina(void*,void*), cosa(void*,void*), tana(void*,void*), asina(void*,void*), acosa(void*,void*);
int    atana(void*,void*), sinha(void*,void*), cosha(void*,void*), tanha(void*,void*), log10a(void*,void*);
int    dbamp(void*,void*), ampdb(void*,void*), aampdb(void*,void*);
int    dbfsamp(void*,void*), ampdbfs(void*,void*), aampdbfs(void*,void*);
int    ftlen(void*,void*), ftlptim(void*,void*), ftchnls(void*,void*), rtclock(void*,void*);
int    cpsoct(void*,void*), octpch(void*,void*), cpspch(void*,void*);
int    pchoct(void*,void*), octcps(void*,void*), acpsoct(void*,void*);
int    massign(void*,void*), ctrlinit(void*,void*), notnum(void*,void*), veloc(void*,void*);
int    pchmidi(void*,void*), pchmidib(void*,void*), octmidi(void*,void*), octmidib(void*,void*);
int    pchmidib_i(void*,void*), octmidib_i(void*,void*), icpsmidib_i(void*,void*);
int    cpsmidi(void*,void*), icpsmidib(void*,void*), kcpsmidib(void*,void*), kmbset(void*,void*);
int    midibset(void*,void*), ipchmidib(void*,void*), ioctmidib(void*,void*);
int    kpchmidib(void*,void*), koctmidib(void*,void*), msclset(void*,void*);
int    ampmidi(void*,void*), aftset(void*,void*), aftouch(void*,void*), chpress(void*,void*);
int    ipchbend(void*,void*), kbndset(void*,void*), kpchbend(void*,void*);
int    imidictl(void*,void*), mctlset(void*,void*), midictl(void*,void*);
int    imidiaft(void*,void*), maftset(void*,void*), midiaft(void*,void*);
int    midiout(void*,void*);
int    turnon(void*,void*);
int    kmapset(void*,void*), polyaft(void*,void*), ichanctl(void*,void*), chctlset(void*,void*);
int    chanctl(void*,void*), ftgen(void*,void*), linset(void*,void*), kline(void*,void*), aline(void*,void*);
int    expset(void*,void*), kexpon(void*,void*), expon(void*,void*);
int    lsgset(void*,void*), klnseg(void*,void*), linseg(void*,void*), madsrset(void*,void*);
int    adsrset(void*,void*), xdsrset(void*,void*), mxdsrset(void*,void*), expseg2(void*,void*);
int    xsgset(void*,void*), kxpseg(void*,void*), expseg(void*,void*), xsgset2(void*,void*);
int    lsgrset(void*,void*), klnsegr(void*,void*), linsegr(void*,void*);
int    xsgrset(void*,void*), kxpsegr(void*,void*), expsegr(void*,void*);
int    lnnset(void*,void*), klinen(void*,void*), linen(void*,void*);
int    lnrset(void*,void*), klinenr(void*,void*), linenr(void*,void*);
int    evxset(void*,void*), knvlpx(void*,void*), envlpx(void*,void*);
int    evrset(void*,void*), knvlpxr(void*,void*), envlpxr(void*,void*);
int    phsset(void*,void*), kphsor(void*,void*), phsor(void*,void*);
int    itablew1(void*,void*),itablegpw1(void*,void*),itablemix1(void*,void*),itablecopy1(void*,void*);
int    itable(void*,void*), itabli(void*,void*), itabl3(void*,void*), tabl3(void*,void*), ktabl3(void*,void*);
int    tblset(void*,void*), ktable(void*,void*), ktabli(void*,void*), tabli(void*,void*);
int    tablefn(void*,void*);
int    tblsetkt(void*,void*), ktablekt(void*,void*), tablekt(void*,void*), ktablikt(void*,void*);
int    tablikt(void*,void*), ko1set(void*,void*), kosc1(void*,void*),  kosc1i(void*,void*);
int    oscnset(void*,void*), osciln(void*,void*);
int    oscset(void*,void*), koscil(void*,void*), osckk(void*,void*), oscka(void*,void*), oscak(void*,void*);
int    oscaa(void*,void*), koscli(void*,void*), osckki(void*,void*), osckai(void*,void*);
int    oscaki(void*,void*), oscaai(void*,void*), foscset(void*,void*), foscil(void*,void*);
int    foscili(void*,void*), losset(void*,void*), loscil(void*,void*), loscil3(void*,void*);
int    koscl3(void*,void*), osckk3(void*,void*), oscka3(void*,void*), oscak3(void*,void*);
int    oscaa3(void*,void*);
int    adset(void*,void*), adsyn(void*,void*);
int    pvset(void*,void*), pvoc(void*,void*);
int    pvaddset(void*,void*), pvadd(void*,void*);
int    bzzset(void*,void*), buzz(void*,void*);
int    gbzset(void*,void*), gbuzz(void*,void*);
int    plukset(void*,void*), pluck(void*,void*);
int    rndset(void*,void*), krand(void*,void*), arand(void*,void*);
int    rhset(void*,void*), krandh(void*,void*), randh(void*,void*);
int    riset(void*,void*), krandi(void*,void*), randi(void*,void*);
int    rndset2(void*,void*), krand2(void*,void*), arand2(void*,void*);
int    rhset2(void*,void*), krandh2(void*,void*), randh2(void*,void*);
int    riset2(void*,void*), krandi2(void*,void*), randi2(void*,void*);
int    porset(void*,void*), port(void*,void*);
int    tonset(void*,void*), tone(void*,void*), atone(void*,void*);
int    rsnset(void*,void*), reson(void*,void*), areson(void*,void*);
int    resonx(void*,void*), aresonx(void*,void*), rsnsetx(void*,void*);
int    tonex(void*,void*),  atonex(void*,void*), tonsetx(void*,void*);
int    lprdset(void*,void*), lpread(void*,void*), lprsnset(void*,void*), lpreson(void*,void*);
int    lpfrsnset(void*,void*), lpfreson(void*,void*);
int    lpslotset(void*,void*) ;
int    lpitpset(void*,void*),lpinterpol(void*,void*) ;
int    rmsset(void*,void*), rms(void*,void*), gainset(void*,void*), gain(void*,void*);
int    sndinset(void*,void*), soundin(void*,void*);
int    sndo1set(void*,void*), soundout(void*,void*), sndo2set(void*,void*), soundouts(void*,void*);
int    in(void*,void*),  ins(void*,void*), inq(void*,void*), inh(void*,void*),  ino(void*,void*), in16(void*,void*);
int    in32(void*,void*), inall(void*,void*);
int    out(void*,void*),  outs(void*,void*), outs1(void*,void*), outs2(void*,void*), outall(void*,void*);
int    outq(void*,void*), outq1(void*,void*), outq2(void*,void*), outq3(void*,void*), outq4(void*,void*);
int    igoto(void*,void*), kgoto(void*,void*), icgoto(void*,void*), kcgoto(void*,void*);
int    timset(void*,void*), timout(void*,void*);
int    reinit(void*,void*), rigoto(void*,void*), rireturn(void*,void*);
int    tigoto(void*,void*), tival(void*,void*);
int    printv(void*,void*), dspset(void*,void*), kdsplay(void*,void*), dsplay(void*,void*);
int    fftset(void*,void*), kdspfft(void*,void*), dspfft(void*,void*);
int    infile_act(INFILE *p);

int    xyinset(void*,void*), xyin(void*,void*), tempeset(void*,void*), tempest(void*,void*);
int    tempset(void*,void*), tempo(void*,void*);
int    old_kdmpset(void*,void*), old_kdmp2set(void*,void*),
        old_kdmp3set(void*,void*), old_kdmp4set(void*,void*);
int    kdmpset(void*,void*), kdmp2set(void*,void*), kdmp3set(void*,void*), kdmp4set(void*,void*);
int    kdump(void*,void*), kdump2(void*,void*), kdump3(void*,void*), kdump4(void*,void*);
int    krdset(void*,void*), krd2set(void*,void*), krd3set(void*,void*), krd4set(void*,void*);
int    kread(void*,void*), kread2(void*,void*), kread3(void*,void*), kread4(void*,void*);
int    ipow(void*,void*), apow(void*,void*), alinear(void*,void*), iklinear(void*,void*);
int    atrian(void*,void*), iktrian(void*,void*), aexp(void*,void*);
int    ikexp(void*,void*), abiexp(void*,void*), ikbiexp(void*,void*), agaus(void*,void*), ikgaus(void*,void*);
int    acauchy(void*,void*), ikcauchy(void*,void*), apcauchy(void*,void*), ikpcauchy(void*,void*);
int    abeta(void*,void*), ikbeta(void*,void*), aweib(void*,void*), ikweib(void*,void*), apoiss(void*,void*);
int    ikpoiss(void*,void*), seedrand(void*,void*);

int    tblesegset(void*,void*), ktableseg(void*,void*), ktablexseg(void*,void*);
int    vpvset(void*,void*), vpvoc(void*,void*);
int    pvreadset(void*,void*), pvread(void*,void*), pvcrossset(void*,void*), pvcross(void*,void*);
int    pvbufreadset(void*,void*), pvbufread(void*,void*);
int    pvinterpset(void*,void*), pvinterp(void*,void*);
int    auniform(void*,void*), ikuniform(void*,void*);
int    newsndinset(void*,void*), soundinew(void*,void*);
int    iout_on(void*,void*), iout_off(void*,void*), out_controller(void*,void*);
int    iout_on_dur_set(void*,void*), iout_on_dur(void*,void*),iout_on_dur2(void*,void*);
int    moscil_set(void*,void*), moscil(void*,void*);
int    kvar_out_on_set(void*,void*), kvar_out_on_set1(void*,void*), kvar_out_on(void*,void*);
int    out_controller14(void*,void*), out_pitch_bend(void*,void*);
int    out_aftertouch(void*,void*), out_poly_aftertouch(void*,void*);
int    out_progchange(void*,void*);
int    release_set(void*,void*), release(void*,void*), xtratim(void*,void*);
int    mclock_set(void*,void*), mclock(void*,void*), mrtmsg(void*,void*);
int    cabasaset(void*,void*), cabasa(void*,void*), sekereset(void*,void*), sandset(void*,void*);
int    stixset(void*,void*), crunchset(void*,void*), guiroset(void*,void*), guiro(void*,void*);
int    sekere(void*,void*), tambourset(void*,void*), tambourine(void*,void*), bambooset(void*,void*);
int    bamboo(void*,void*), wuterset(void*,void*), wuter(void*,void*), sleighset(void*,void*), sleighbells(void*,void*);

int    trig_set(void*,void*), trig(void*,void*), numsamp(void*,void*), ftsr(void*,void*);
int    kon2_set(void*,void*), kon2(void*,void*);
int    nrpn(void*,void*), mdelay(void*,void*), mdelay_set(void*,void*);
#if defined(TCLTK)
int    cntrl_set(void*,void*), control(void*,void*), ocontrol(void*,void*);
int    button_set(void*,void*), button(void*,void*), check_set(void*,void*), check(void*,void*);
#endif

int    sum(void*,void*), product(void*,void*), macset(void*,void*), mac(void*,void*), maca(void*,void*);
int    nestedapset(void*,void*), nestedap(void*,void*);
int    lorenzset(void*,void*), lorenz(void*,void*);
int    filelen(void*,void*), filenchnls(void*,void*), filesr(void*,void*), filepeak(void*,void*);
int    ipowoftwo(void*,void*), ilogbasetwo(void*,void*);
int    powoftwo_set(void*,void*), logbasetwo_set(void*,void*);
int    powoftwo(void*,void*), powoftwoa(void*,void*);
int    logbasetwo(void*,void*), logbasetwoa(void*,void*);
/* int    nlalp_set(void*,void*), nlalp(void*,void*); */
int    lp2_set(void*,void*), lp2(void*,void*);
int    phaser2set(void*,void*), phaser2(void*,void*);
int    phaser1set(void*,void*), phaser1(void*,void*);
int    balnset(void*,void*), balance(void*,void*);
int    cvset(void*,void*), convolve(void*,void*);
int    pconvset(void*,void*), pconvolve(void*,void*);

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
                M       begins an indef list of args (any count/rate) IV 2002/9/1
                n       begins an indef list of iargs (nargs odd)
                o       optional, defaulting to 0
                p       "            "          1
                q       "            "         10
                v       "            "          .5
                j       "            "         -1
                h       "            "        127
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
{ "rtclock", 0xffff                                                     },
{ "tablew",  0xfffe                                                     },
{ "tablewkt",  0xfffe                                                   },
{ "ihold",  S(LINK),    1,      "",     "",     ihold                   },
{ "turnoff",S(LINK),    2,      "",     "",     NULL,   turnoff         },
{ "=.r",    S(ASSIGN),  1,      "r",    "i",    rassign                 },
{ "=.i",    S(ASSIGN),  1,      "i",    "i",    assign                  },
{ "=.k",    S(ASSIGN),  2,      "k",    "k",    NULL,   assign          },
{ "=.a",    S(ASSIGN),  4,      "a",    "x",    NULL,   NULL,   aassign },
{ "init.i",  S(ASSIGN), 1,      "i",    "i",    init                    },
{ "init.k",  S(ASSIGN), 1,      "k",    "i",    init                    },
{ "init.a",  S(ASSIGN), 1,      "a",    "i",    ainit                   },
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
{ "add.ii",  S(AOP),    1,      "i",    "ii",   addkk                   },
{ "sub.ii",  S(AOP),    1,      "i",    "ii",   subkk                   },
{ "mul.ii",  S(AOP),    1,      "i",    "ii",   mulkk                   },
{ "div.ii",  S(AOP),    1,      "i",    "ii",   divkk                   },
{ "mod.ii",  S(AOP),    1,      "i",    "ii",   modkk                   },
{ "add.kk",  S(AOP),    2,      "k",    "kk",   NULL,   addkk           },
{ "sub.kk",  S(AOP),    2,      "k",    "kk",   NULL,   subkk           },
{ "mul.kk",  S(AOP),    2,      "k",    "kk",   NULL,   mulkk           },
{ "div.kk",  S(AOP),    2,      "k",    "kk",   NULL,   divkk           },
{ "mod.kk",  S(AOP),    2,      "k",    "kk",   NULL,   modkk           },
{ "add.ka",  S(AOP),    4,      "a",    "ka",   NULL,   NULL,   addka   },
{ "sub.ka",  S(AOP),    4,      "a",    "ka",   NULL,   NULL,   subka   },
{ "mul.ka",  S(AOP),    4,      "a",    "ka",   NULL,   NULL,   mulka   },
{ "div.ka",  S(AOP),    4,      "a",    "ka",   NULL,   NULL,   divka   },
{ "mod.ka",  S(AOP),    4,      "a",    "ka",   NULL,   NULL,   modka   },
{ "add.ak",  S(AOP),    4,      "a",    "ak",   NULL,   NULL,   addak   },
{ "sub.ak",  S(AOP),    4,      "a",    "ak",   NULL,   NULL,   subak   },
{ "mul.ak",  S(AOP),    4,      "a",    "ak",   NULL,   NULL,   mulak   },
{ "div.ak",  S(AOP),    4,      "a",    "ak",   NULL,   NULL,   divak   },
{ "mod.ak",  S(AOP),    4,      "a",    "ak",   NULL,   NULL,   modak   },
{ "add.aa",  S(AOP),    4,      "a",    "aa",   NULL,   NULL,   addaa   },
{ "sub.aa",  S(AOP),    4,      "a",    "aa",   NULL,   NULL,   subaa   },
{ "mul.aa",  S(AOP),    4,      "a",    "aa",   NULL,   NULL,   mulaa   },
{ "div.aa",  S(AOP),    4,      "a",    "aa",   NULL,   NULL,   divaa   },
{ "mod.aa",  S(AOP),    4,      "a",    "aa",   NULL,   NULL,   modaa   },
{ "divz",   0xfffc                                                      },
{ "divz.ii", S(DIVZ),   1,      "i",    "iii",  divzkk, NULL,   NULL    },
{ "divz.kk", S(DIVZ),   2,      "k",    "kkk",  NULL,   divzkk, NULL    },
{ "divz.ak", S(DIVZ),   4,      "a",    "akk",  NULL,   NULL,   divzak  },
{ "divz.ka", S(DIVZ),   4,      "a",    "kak",  NULL,   NULL,   divzka  },
{ "divz.aa", S(DIVZ),   4,      "a",    "aak",  NULL,   NULL,   divzaa  },
{ "int.i",  S(EVAL),    1,      "i",    "i",    int1                    },
{ "frac.i", S(EVAL),    1,      "i",    "i",    frac1                   },
{ "rnd.i",  S(EVAL),    1,      "i",    "i",    rnd1                    },
{ "birnd.i",S(EVAL),    1,      "i",    "i",    birnd1                  },
{ "abs.i",  S(EVAL),    1,      "i",    "i",    abs1                    },
{ "exp.i",  S(EVAL),    1,      "i",    "i",    exp01                   },
{ "log.i",  S(EVAL),    1,      "i",    "i",    log01                   },
{ "sqrt.i", S(EVAL),    1,      "i",    "i",    sqrt1                   },
{ "sin.i",  S(EVAL),    1,      "i",    "i",    sin1                    },
{ "cos.i",  S(EVAL),    1,      "i",    "i",    cos1                    },
{ "tan.i",  S(EVAL),    1,      "i",    "i",    tan1                    },
{ "sininv.i", S(EVAL),  1,      "i",    "i",    asin1                   },
{ "cosinv.i", S(EVAL),  1,      "i",    "i",    acos1                   },
{ "taninv.i", S(EVAL),  1,      "i",    "i",    atan1                   },
{ "taninv2.i",S(AOP),   1,      "i",    "ii",   atan21                  },
{ "log10.i",S(EVAL),    1,      "i",    "i",    log101                  },
{ "sinh.i", S(EVAL),    1,      "i",    "i",    sinh1                   },
{ "cosh.i", S(EVAL),    1,      "i",    "i",    cosh1                   },
{ "tanh.i", S(EVAL),    1,      "i",    "i",    tanh1                   },
{ "int.k",  S(EVAL),    2,      "k",    "k",    NULL,   int1            },
{ "frac.k", S(EVAL),    2,      "k",    "k",    NULL,   frac1           },
{ "rnd.k",  S(EVAL),    2,      "k",    "k",    NULL,   rnd1            },
{ "birnd.k",S(EVAL),    2,      "k",    "k",    NULL,   birnd1          },
{ "abs.k",  S(EVAL),    2,      "k",    "k",    NULL,   abs1            },
{ "exp.k",  S(EVAL),    2,      "k",    "k",    NULL,   exp01           },
{ "log.k",  S(EVAL),    2,      "k",    "k",    NULL,   log01           },
{ "sqrt.k", S(EVAL),    2,      "k",    "k",    NULL,   sqrt1           },
{ "sin.k",  S(EVAL),    2,      "k",    "k",    NULL,   sin1            },
{ "cos.k",  S(EVAL),    2,      "k",    "k",    NULL,   cos1            },
{ "tan.k",  S(EVAL),    2,      "k",    "k",    NULL,   tan1            },
{ "sininv.k", S(EVAL),  2,      "k",    "k",    NULL,   asin1           },
{ "cosinv.k", S(EVAL),  2,      "k",    "k",    NULL,   acos1           },
{ "taninv.k", S(EVAL),  2,      "k",    "k",    NULL,   atan1           },
{ "taninv2.k",S(AOP),   2,      "k",    "kk",   NULL,   atan21          },
{ "sinh.k", S(EVAL),    2,      "k",    "k",    NULL,   sinh1           },
{ "cosh.k", S(EVAL),    2,      "k",    "k",    NULL,   cosh1           },
{ "tanh.k", S(EVAL),    2,      "k",    "k",    NULL,   tanh1           },
{ "log10.k",S(EVAL),    2,      "k",    "k",    NULL,   log101          },
{ "abs.a",  S(EVAL),    4,      "a",    "a",    NULL,   NULL,   absa    },
{ "exp.a",  S(EVAL),    4,      "a",    "a",    NULL,   NULL,   expa    },
{ "log.a",  S(EVAL),    4,      "a",    "a",    NULL,   NULL,   loga    },
{ "sqrt.a", S(EVAL),    4,      "a",    "a",    NULL,   NULL,   sqrta   },
{ "sin.a",  S(EVAL),    4,      "a",    "a",    NULL,   NULL,   sina    },
{ "cos.a",  S(EVAL),    4,      "a",    "a",    NULL,   NULL,   cosa    },
{ "tan.a",  S(EVAL),    4,      "a",    "a",    NULL,   NULL,   tana    },
{ "sininv.a", S(EVAL),  4,      "a",    "a",    NULL,   NULL,   asina   },
{ "cosinv.a", S(EVAL),  4,      "a",    "a",    NULL,   NULL,   acosa   },
{ "taninv.a", S(EVAL),  4,      "a",    "a",    NULL,   NULL,   atana   },
{ "taninv2.a",S(AOP),   4,      "a",    "aa",   NULL,   NULL,   atan2aa },
{ "sinh.a", S(EVAL),    4,      "a",    "a",    NULL,   NULL,   sinha   },
{ "cosh.a", S(EVAL),    4,      "a",    "a",    NULL,   NULL,   cosha   },
{ "tanh.a", S(EVAL),    4,      "a",    "a",    NULL,   NULL,   tanha   },
{ "log10.a",S(EVAL),    4,      "a",    "a",    NULL,   NULL,   log10a  },
{ "ampdb.a",S(EVAL),    4,      "a",    "a",    NULL,   NULL,   aampdb  },
{ "ampdb.i",S(EVAL),    1,      "i",    "i",    ampdb                   },
{ "ampdb.k",S(EVAL),    2,      "k",    "k",    NULL,   ampdb           },
{ "ampdbfs.a",S(EVAL),  4,      "a",    "a",    NULL,   NULL,   aampdbfs },
{ "ampdbfs.i",S(EVAL),  1,      "i",    "i",    ampdbfs                 },
{ "ampdbfs.k",S(EVAL),  2,      "k",    "k",    NULL,   ampdbfs         },
{ "dbamp.i",S(EVAL),    1,      "i",    "i",    dbamp                   },
{ "dbamp.k",S(EVAL),    2,      "k",    "k",    NULL,   dbamp           },
{ "dbfsamp.i",S(EVAL),  1,      "i",    "i",    dbfsamp                 },
{ "dbfsamp.k",S(EVAL),  2,      "k",    "k",    NULL,   dbfsamp         },
{ "rtclock.i",S(EVAL),  1,      "i",    "",     rtclock                 },
{ "rtclock.k",S(EVAL),  2,      "k",    "",     NULL,   rtclock         },
{ "ftlen.i",S(EVAL),    1,      "i",    "i",    ftlen                   },
{ "ftsr.i",S(EVAL),     1,      "i",    "i",    ftsr                    },
{ "ftlptim.i",S(EVAL),  1,      "i",    "i",    ftlptim                 },
{ "ftchnls.i",S(EVAL),  1,      "i",    "i",    ftchnls                 },
{ "i.k",   S(ASSIGN),   1,      "i",    "k",    init                    },
{ "cpsoct.i",S(EVAL),   1,      "i",    "i",    cpsoct                  },
{ "octpch.i",S(EVAL),   1,      "i",    "i",    octpch                  },
{ "cpspch.i",S(EVAL),   1,      "i",    "i",    cpspch                  },
{ "pchoct.i",S(EVAL),   1,      "i",    "i",    pchoct                  },
{ "octcps.i",S(EVAL),   1,      "i",    "i",    octcps                  },
{ "cpsoct.k",S(EVAL),   2,      "k",    "k",    NULL,   cpsoct          },
{ "octpch.k",S(EVAL),   2,      "k",    "k",    NULL,   octpch          },
{ "cpspch.k",S(EVAL),   2,      "k",    "k",    NULL,   cpspch          },
{ "pchoct.k",S(EVAL),   2,      "k",    "k",    NULL,   pchoct          },
{ "octcps.k",S(EVAL),   2,      "k",    "k",    NULL,   octcps          },
{ "cpsoct.a",S(EVAL),   4,      "a",    "a",    NULL,   NULL,   acpsoct },
{ "notnum", S(MIDIKMB), 1,      "i",    "",     notnum                  },
{ "veloc",  S(MIDIMAP), 1,      "i",    "oh",   veloc                   },
{ "pchmidi",S(MIDIKMB), 1,      "i",    "",     pchmidi                 },
{ "octmidi",S(MIDIKMB), 1,      "i",    "",     octmidi                 },
{ "cpsmidi",S(MIDIKMB), 1,      "i",    "",     cpsmidi                 },
{ "pchmidib.i",S(MIDIKMB),1,    "i",    "o",    pchmidib_i              },
{ "octmidib.i",S(MIDIKMB),1,    "i",    "o",    octmidib_i              },
{ "cpsmidib.i",S(MIDIKMB),1,    "i",    "o",    icpsmidib_i             },
{ "pchmidib.k",S(MIDIKMB),3,    "k",    "o",    midibset, pchmidib      },
{ "octmidib.k",S(MIDIKMB),3,    "k",    "o",    midibset, octmidib      },
{ "cpsmidib.k",S(MIDIKMB),3,    "k",    "o",    midibset, icpsmidib     },
{ "ampmidi",S(MIDIAMP), 1,      "i",    "io",   ampmidi                 },
{ "aftouch",S(MIDIKMAP), 3,     "k",    "oh",   aftset, aftouch         },
{ "pchbend.i",S(MIDIMAP),0x21,  "i",    "jp",   ipchbend                },
{ "pchbend.k",S(MIDIKMAP),0x23, "k",    "jp",   kbndset,kpchbend        },
{ "midictrl.i",S(MIDICTL),1,    "i",    "ioh",  imidictl                },
{ "midictrl.k",S(MIDICTL),3,    "k",    "ioh",  mctlset, midictl        },
{ "polyaft.i",S(MIDICTL),1,     "i",    "ioh",  imidiaft                },
{ "polyaft.k",S(MIDICTL),3,     "k",    "ioh",  maftset, midiaft        },
{ "chanctrl.i",S(CHANCTL),1,    "i",    "iioh", ichanctl                },
{ "chanctrl.k",S(CHANCTL),3,    "k",    "iioh", chctlset,chanctl        },
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
{ "table.i", S(TABLE),  1,      "i",    "iiooo",itable                  },
{ "tablei.i", S(TABLE), 1,      "i",    "iiooo",itabli                  },
{ "table3.i", S(TABLE), 1,      "i",    "iiooo",itabl3                  },
{ "table",  S(TABLE),   7,      "s",    "xiooo",tblset, ktable, tablefn },
{ "tablei", S(TABLE),   7,      "s",    "xiooo",tblset, ktabli, tabli   },
{ "table3", S(TABLE),   7,      "s",    "xiooo",tblset, ktabl3, tabl3   },
{ "oscil1", S(OSCIL1),  3,      "k",    "ikii", ko1set, kosc1           },
{ "oscil1i",S(OSCIL1),  3,      "k",    "ikii", ko1set, kosc1i          },
{ "osciln", S(OSCILN),  5,      "a",    "kiii", oscnset,NULL,   osciln  },
{ "oscil.kk",S(OSC),    7,      "s",    "kkio", oscset, koscil, osckk   },
{ "oscil.ka",S(OSC),    5,      "a",    "kaio", oscset, NULL,   oscka   },
{ "oscil.ak",S(OSC),    5,      "a",    "akio", oscset, NULL,   oscak   },
{ "oscil.aa",S(OSC),    5,      "a",    "aaio", oscset, NULL,   oscaa   },
{ "oscili.kk",S(OSC),   7,      "s",    "kkio", oscset, koscli, osckki  },
{ "oscili.ka",S(OSC),   5,      "a",    "kaio", oscset, NULL,   osckai  },
{ "oscili.ak",S(OSC),   5,      "a",    "akio", oscset, NULL,   oscaki  },
{ "oscili.aa",S(OSC),   5,      "a",    "aaio", oscset, NULL,   oscaai  },
{ "oscil3.kk",S(OSC),   7,      "s",    "kkio", oscset, koscl3, osckk3  },
{ "oscil3.ka",S(OSC),   5,      "a",    "kaio", oscset, NULL,   oscka3  },
{ "oscil3.ak",S(OSC),   5,      "a",    "akio", oscset, NULL,   oscak3  },
{ "oscil3.aa",S(OSC),   5,      "a",    "aaio", oscset, NULL,   oscaa3  },
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
{ "soundin",S(SOUNDIN), 5,"mmmmmmmmmmmmmmmmmmmmmmmm","Sooo",sndinset,NULL,soundin },
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
{ "pow.i",    S(POW),   1,      "i",    "iip",  ipow,    NULL,  NULL    },
{ "pow.k",    S(POW),   2,      "k",    "kkp",  NULL,    ipow,  NULL    },
{ "pow.a",    S(POW),   4,      "a",    "akp",  NULL,    NULL,  apow    },
{ "oscilx",   S(OSCILN),5,      "a",    "kiii", oscnset,NULL,   osciln  },
{ "linrand.i",S(PRAND), 1,      "i",    "k",    iklinear, NULL, NULL    },
{ "linrand.k",S(PRAND), 2,      "k",    "k",    NULL, iklinear, NULL    },
{ "linrand.a",S(PRAND), 4,      "a",    "k",    NULL, NULL,     alinear },
{ "trirand.i",S(PRAND), 1,      "i",    "k",    iktrian, NULL,  NULL    },
{ "trirand.k",S(PRAND), 2,      "k",    "k",    NULL, iktrian,  NULL    },
{ "trirand.a",S(PRAND), 4,      "a",    "k",    NULL, NULL,     atrian  },
{ "exprand.i",S(PRAND), 1,      "i",    "k",    ikexp, NULL,    NULL    },
{ "exprand.k",S(PRAND), 2,      "k",    "k",    NULL,    ikexp, NULL    },
{ "exprand.a",S(PRAND), 4,      "a",    "k",    NULL, NULL,     aexp    },
{ "bexprnd.i",S(PRAND), 1,      "i",    "k",    ikbiexp, NULL,  NULL    },
{ "bexprnd.k",S(PRAND), 2,      "k",    "k",    NULL, ikbiexp,  NULL    },
{ "bexprnd.a",S(PRAND), 4,      "a",    "k",    NULL, NULL,     abiexp  },
{ "cauchy.i", S(PRAND), 1,      "i",    "k",    ikcauchy, NULL, NULL    },
{ "cauchy.k", S(PRAND), 2,      "k",    "k",    NULL, ikcauchy, NULL    },
{ "cauchy.a", S(PRAND), 4,      "a",    "k",    NULL,    NULL,  acauchy },
{ "pcauchy.i",S(PRAND), 1,      "i",    "k",    ikpcauchy, NULL,NULL    },
{ "pcauchy.k",S(PRAND), 2,      "k",    "k",    NULL, ikpcauchy,NULL    },
{ "pcauchy.a",S(PRAND), 4,      "a",    "k",    NULL,    NULL,  apcauchy},
{ "poisson.i",S(PRAND), 1,      "i",    "k",    ikpoiss, NULL,  NULL    },
{ "poisson.k",S(PRAND), 2,      "k",    "k",    NULL, ikpoiss,  NULL    },
{ "poisson.a",S(PRAND), 4,      "a",    "k",    NULL,    NULL,  apoiss  },
{ "gauss.i" , S(PRAND), 1,      "i",    "k",    ikgaus,  NULL,  NULL    },
{ "gauss.k" , S(PRAND), 2,      "k",    "k",    NULL, ikgaus,   NULL    },
{ "gauss.a" , S(PRAND), 4,      "a",    "k",    NULL,    NULL,  agaus   },
{ "weibull.i",S(PRAND), 1,      "i",    "kk",   ikweib,  NULL,  NULL    },
{ "weibull.k",S(PRAND), 2,      "k",    "kk",   NULL, ikweib,   NULL    },
{ "weibull.a",S(PRAND), 4,      "a",    "kk",   NULL,    NULL,  aweib   },
{ "betarand.i",S(PRAND),1,      "i",    "kkk",  ikbeta, NULL,  NULL     },
{ "betarand.k",S(PRAND),2,      "k",    "kkk",  NULL,   ikbeta,NULL     },
{ "betarand.a",S(PRAND),4,      "a",    "kkk",  NULL,   NULL,  abeta    },
{ "seed",     S(PRAND), 1,      "",     "i",    seedrand, NULL, NULL    },
{ "convolve", S(CONVOLVE),  5, "mmmm", "aSo",   cvset,   NULL, convolve },
{ "convle",   S(CONVOLVE),  5, "mmmm", "aSo",   cvset,   NULL, convolve },
{ "pconvolve",S(PCONVOLVE), 5, "mmmm", "aSoo", pconvset, NULL, pconvolve },
{ "tableseg", S(TABLESEG), 3,  "",      "iin",  tblesegset, ktableseg},
{ "ktableseg", S(TABLESEG), 3,  "",     "iin",  tblesegset, ktableseg},
{ "tablexseg", S(TABLESEG), 3, "",      "iin",  tblesegset, ktablexseg},
{ "vpvoc",    S(VPVOC), 5,     "a",     "kkSoo", vpvset,        NULL,   vpvoc},
{ "pvread",   S(PVREAD),3,     "kk",    "kSi",  pvreadset, pvread},
{ "pvcross",  S(PVCROSS),  5,  "a",     "kkSkko", pvcrossset, NULL, pvcross},
{ "pvbufread",S(PVBUFREAD), 3, "",      "kS",   pvbufreadset, pvbufread, NULL},
{ "pvinterp", S(PVINTERP), 5,  "a",  "kkSkkkkkk", pvinterpset, NULL, pvinterp},
{ "pvadd",    S(PVADD), 5,     "a", "kkSiiopooo", pvaddset,     NULL,   pvadd},
{ "unirand.i",S(PRAND), 1,     "i",     "k",    ikuniform, NULL,  NULL  },
{ "unirand.k",S(PRAND), 2,     "k",     "k",    NULL,    ikuniform, NULL},
{ "unirand.a",S(PRAND), 4,     "a",     "k",    NULL,    NULL, auniform },
{ "diskin",S(SOUNDINEW),5,  "mmmm",     "Skoooo", newsndinset,NULL, soundinew},
{ "diskin2",S(DISKIN2), 5,  "mmmmmmmmmmmmmmmm",
                            "Skoooooooooooooooooooo",
                            (SUBR) diskin2_init, (SUBR) NULL,
                            (SUBR) diskin2_perf                         },
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
{ "nsamp.i", S(EVAL),    1,     "i",    "i",    numsamp                    },
{ "powoftwo.i",S(EVAL),  1,     "i",    "i",    ipowoftwo                  },
{ "powoftwo.k",S(EVAL),  2,     "k",    "k",    powoftwo_set, powoftwo     },
{ "powoftwo.a",S(EVAL),  4,     "a",    "a",  powoftwo_set, NULL, powoftwoa },
{ "logbtwo.i",S(EVAL),   1,     "i",    "i",    ilogbasetwo                },
{ "logbtwo.k",S(EVAL),   2,     "k",    "k",    logbasetwo_set, logbasetwo },
{ "logbtwo.a",S(EVAL),   4,     "a",    "a", logbasetwo_set, NULL, logbasetwoa },
{ "filelen", S(SNDINFO), 1,     "i",    "S",    filelen, NULL, NULL        },
{ "filenchnls", S(SNDINFO), 1,  "i",    "S",    filenchnls, NULL, NULL     },
{ "filesr", S(SNDINFO),  1,     "i",    "S",    filesr, NULL, NULL         },
{ "filepeak", S(SNDINFOPEAK), 1, "i",   "So",   filepeak, NULL, NULL       },
/*  { "nlalp", S(NLALP),     5,     "a",    "akkoo", nlalp_set, NULL, nlalp     }, */
/* Robin Whittle */
{ "tableiw",  S(TABLEW),1,     "",      "iiiooo", (SUBR)itablew, NULL, NULL},
{ "tablew.kk", S(TABLEW),3,    "",      "kkiooo",(SUBR)tblsetw,(SUBR)ktablew, NULL},
{ "tablew.aa", S(TABLEW),5,    "",      "aaiooo",(SUBR)tblsetw, NULL, (SUBR)tablew},
{ "tablewkt.kk", S(TABLEW),3, "",     "kkkooo",(SUBR)tblsetwkt,(SUBR)ktablewkt,NULL},
{ "tablewkt.aa", S(TABLEW),5, "",     "aakooo",(SUBR)tblsetwkt,NULL,(SUBR)tablewkt},
{ "tableng.i", S(TABLENG),1,  "i",     "i",    (SUBR)itableng, NULL,  NULL},
{ "tableng.k",  S(TABLENG),2, "k",     "k",    NULL,   (SUBR)tableng, NULL},
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
{ "timek.i", S(RDTIME), 1,   "i",  "",     (SUBR)timek,   NULL,  NULL },
{ "times.i", S(RDTIME), 1,   "i",  "",     (SUBR)timesr,  NULL,  NULL },
{ "timek.k",  S(RDTIME), 2,  "k",  "",     NULL,    (SUBR)timek, NULL },
{ "times.k",  S(RDTIME), 2,  "k",  "",     NULL,    (SUBR)timesr,NULL },
{ "timeinstk", S(RDTIME), 3, "k",  "",     (SUBR)instimset, (SUBR)instimek, NULL },
{ "timeinsts", S(RDTIME), 3, "k",  "",     (SUBR)instimset, (SUBR)instimes, NULL },
{ "peak.k",  S(PEAK),   2,   "k",  "k",    NULL,    (SUBR)peakk,    NULL },
{ "peak.a",   S(PEAK),  4,   "k",  "a",    NULL,    NULL,     (SUBR)peaka },
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
{ "limit.i", S(LIMIT),  1, "i",     "iii",  (SUBR)klimit,  NULL,    NULL       },
{ "limit.i", S(LIMIT),  1, "i",     "iii",  (SUBR)klimit,  NULL,    NULL       },
{ "limit.k",  S(LIMIT), 3, "k",     "xkk",  (SUBR)limitset, (SUBR)klimit, NULL },
{ "limit.a",  S(LIMIT), 5, "a",     "xkk",  (SUBR)limitset, NULL,  (SUBR)limit },
};

long oplength_1 = sizeof(opcodlst_1);

