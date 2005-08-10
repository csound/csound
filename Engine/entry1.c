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

#include "csoundCore.h"         /*                      ENTRY.C         */
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
#include "oload.h"
#include "midiout.h"
#include "pvadd.h"
#include "sndinfUG.h"
#include "ugrw1.h"
#include "ugrw2.h"

#define S       sizeof

int     ihold(ENVIRON *, void *), turnoff(ENVIRON *, void *);
int     assign(ENVIRON *, void *), rassign(ENVIRON *, void *);
int     aassign(ENVIRON *, void *);
int     init(ENVIRON *, void *), ainit(ENVIRON *, void *);
int     gt(ENVIRON *, void *), ge(ENVIRON *, void *);
int     lt(ENVIRON *, void *), le(ENVIRON *, void *);
int     eq(ENVIRON *, void *), ne(ENVIRON *, void *);
int     and(ENVIRON *, void *), or(ENVIRON *, void *);
int     conval(ENVIRON *, void *), aconval(ENVIRON *, void *);
int     addkk(ENVIRON *, void *), subkk(ENVIRON *, void *);
int     mulkk(ENVIRON *, void *), divkk(ENVIRON *, void *);
int     modkk(ENVIRON *, void *);
int     addka(ENVIRON *, void *), subka(ENVIRON *, void *);
int     mulka(ENVIRON *, void *), divka(ENVIRON *, void *);
int     modka(ENVIRON *, void *);
int     addak(ENVIRON *, void *), subak(ENVIRON *, void *);
int     mulak(ENVIRON *, void *), divak(ENVIRON *, void *);
int     modak(ENVIRON *, void *);
int     addaa(ENVIRON *, void *), subaa(ENVIRON *, void *);
int     mulaa(ENVIRON *, void *), divaa(ENVIRON *, void *);
int     modaa(ENVIRON *, void *);
int     divzkk(ENVIRON *, void *), divzka(ENVIRON *, void *);
int     divzak(ENVIRON *, void *), divzaa(ENVIRON *, void *);
int     int1(ENVIRON *, void *), int1a(ENVIRON *, void *);
int     frac1(ENVIRON *, void *), frac1a(ENVIRON *, void *);
int     int1_round(ENVIRON *, void *), int1a_round(ENVIRON *, void *);
int     int1_floor(ENVIRON *, void *), int1a_floor(ENVIRON *, void *);
int     int1_ceil(ENVIRON *, void *), int1a_ceil(ENVIRON *, void *);
int     rnd1(ENVIRON *, void *), birnd1(ENVIRON *, void *);
int     abs1(ENVIRON *, void *), exp01(ENVIRON *, void *);
int     log01(ENVIRON *, void *), sqrt1(ENVIRON *, void *);
int     sin1(ENVIRON *, void *), cos1(ENVIRON *, void *);
int     tan1(ENVIRON *, void *), asin1(ENVIRON *, void *);
int     acos1(ENVIRON *, void *), atan1(ENVIRON *, void *);
int     sinh1(ENVIRON *, void *), cosh1(ENVIRON *, void *);
int     tanh1(ENVIRON *, void *), log101(ENVIRON *, void *);
int     atan21(ENVIRON *, void *), atan2aa(ENVIRON *, void *);
int     absa(ENVIRON *, void *), expa(ENVIRON *, void *);
int     loga(ENVIRON *, void *), sqrta(ENVIRON *, void *);
int     sina(ENVIRON *, void *), cosa(ENVIRON *, void *);
int     tana(ENVIRON *, void *), asina(ENVIRON *, void *);
int     acosa(ENVIRON *, void *), atana(ENVIRON *, void *);
int     sinha(ENVIRON *, void *), cosha(ENVIRON *, void *);
int     tanha(ENVIRON *, void *), log10a(ENVIRON *, void *);
int     dbamp(ENVIRON *, void *), ampdb(ENVIRON *, void *);
int     aampdb(ENVIRON *, void *), dbfsamp(ENVIRON *, void *);
int     ampdbfs(ENVIRON *, void *), aampdbfs(ENVIRON *, void *);
int     ftlen(ENVIRON *, void *), ftlptim(ENVIRON *, void *);
int     ftchnls(ENVIRON *, void *), rtclock(ENVIRON *, void *);
int     cpsoct(ENVIRON *, void *), octpch(ENVIRON *, void *);
int     cpspch(ENVIRON *, void *), pchoct(ENVIRON *, void *);
int     octcps(ENVIRON *, void *), acpsoct(ENVIRON *, void *);
int     massign(ENVIRON *, void *), ctrlinit(ENVIRON *, void *);
int     notnum(ENVIRON *, void *), veloc(ENVIRON *, void *);
int     pchmidi(ENVIRON *, void *), pchmidib(ENVIRON *, void *);
int     octmidi(ENVIRON *, void *), octmidib(ENVIRON *, void *);
int     pchmidib_i(ENVIRON *, void *), octmidib_i(ENVIRON *, void *);
int     icpsmidib_i(ENVIRON *, void *), cpsmidi(ENVIRON *, void *);
int     icpsmidib(ENVIRON *, void *), kcpsmidib(ENVIRON *, void *);
int     kmbset(ENVIRON *, void *), midibset(ENVIRON *, void *);
int     ipchmidib(ENVIRON *, void *), ioctmidib(ENVIRON *, void *);
int     kpchmidib(ENVIRON *, void *), koctmidib(ENVIRON *, void *);
int     msclset(ENVIRON *, void *), ampmidi(ENVIRON *, void *);
int     aftset(ENVIRON *, void *), aftouch(ENVIRON *, void *);
int     chpress(ENVIRON *, void *), ipchbend(ENVIRON *, void *);
int     kbndset(ENVIRON *, void *), kpchbend(ENVIRON *, void *);
int     imidictl(ENVIRON *, void *), mctlset(ENVIRON *, void *);
int     midictl(ENVIRON *, void *), imidiaft(ENVIRON *, void *);
int     maftset(ENVIRON *, void *), midiaft(ENVIRON *, void *);
int     midiout(ENVIRON *, void *), turnon(ENVIRON *, void *);
int     kmapset(ENVIRON *, void *), polyaft(ENVIRON *, void *);
int     ichanctl(ENVIRON *, void *), chctlset(ENVIRON *, void *);
int     chanctl(ENVIRON *, void *), linset(ENVIRON *, void *);
int     kline(ENVIRON *, void *), aline(ENVIRON *, void *);
int     expset(ENVIRON *, void *), kexpon(ENVIRON *, void *);
int     expon(ENVIRON *, void *), lsgset(ENVIRON *, void *);
int     klnseg(ENVIRON *, void *), linseg(ENVIRON *, void *);
int     madsrset(ENVIRON *, void *), adsrset(ENVIRON *, void *);
int     xdsrset(ENVIRON *, void *), mxdsrset(ENVIRON *, void *);
int     expseg2(ENVIRON *, void *), xsgset(ENVIRON *, void *);
int     kxpseg(ENVIRON *, void *), expseg(ENVIRON *, void *);
int     xsgset2(ENVIRON *, void *), lsgrset(ENVIRON *, void *);
int     klnsegr(ENVIRON *, void *), linsegr(ENVIRON *, void *);
int     xsgrset(ENVIRON *, void *), kxpsegr(ENVIRON *, void *);
int     expsegr(ENVIRON *, void *), lnnset(ENVIRON *, void *);
int     klinen(ENVIRON *, void *), linen(ENVIRON *, void *);
int     lnrset(ENVIRON *, void *), klinenr(ENVIRON *, void *);
int     linenr(ENVIRON *, void *), evxset(ENVIRON *, void *);
int     knvlpx(ENVIRON *, void *), envlpx(ENVIRON *, void *);
int     evrset(ENVIRON *, void *), knvlpxr(ENVIRON *, void *);
int     envlpxr(ENVIRON *, void *), phsset(ENVIRON *, void *);
int     kphsor(ENVIRON *, void *), phsor(ENVIRON *, void *);
int     itablew1(ENVIRON *, void *), itablegpw1(ENVIRON *, void *);
int     itablemix1(ENVIRON *, void *), itablecopy1(ENVIRON *, void *);
int     itable(ENVIRON *, void *), itabli(ENVIRON *, void *);
int     itabl3(ENVIRON *, void *), tabl3(ENVIRON *, void *);
int     ktabl3(ENVIRON *, void *), tblset(ENVIRON *, void *);
int     ktable(ENVIRON *, void *), ktabli(ENVIRON *, void *);
int     tabli(ENVIRON *, void *), tablefn(ENVIRON *, void *);
int     tblsetkt(ENVIRON *, void *), ktablekt(ENVIRON *, void *);
int     tablekt(ENVIRON *, void *), ktablikt(ENVIRON *, void *);
int     tablikt(ENVIRON *, void *), ko1set(ENVIRON *, void *);
int     kosc1(ENVIRON *, void *), kosc1i(ENVIRON *, void *);
int     oscnset(ENVIRON *, void *), osciln(ENVIRON *, void *);
int     oscset(ENVIRON *, void *), koscil(ENVIRON *, void *);
int     osckk(ENVIRON *, void *), oscka(ENVIRON *, void *);
int     oscak(ENVIRON *, void *), oscaa(ENVIRON *, void *);
int     koscli(ENVIRON *, void *), osckki(ENVIRON *, void *);
int     osckai(ENVIRON *, void *), oscaki(ENVIRON *, void *);
int     oscaai(ENVIRON *, void *), foscset(ENVIRON *, void *);
int     foscil(ENVIRON *, void *), foscili(ENVIRON *, void *);
int     losset(ENVIRON *, void *), loscil(ENVIRON *, void *);
int     loscil3(ENVIRON *, void *), koscl3(ENVIRON *, void *);
int     osckk3(ENVIRON *, void *), oscka3(ENVIRON *, void *);
int     oscak3(ENVIRON *, void *), oscaa3(ENVIRON *, void *);
int     adset(ENVIRON *, void *), adsyn(ENVIRON *, void *);
int     pvset(ENVIRON *, void *), pvoc(ENVIRON *, void *);
int     pvaddset(ENVIRON *, void *), pvadd(ENVIRON *, void *);
int     bzzset(ENVIRON *, void *), buzz(ENVIRON *, void *);
int     gbzset(ENVIRON *, void *), gbuzz(ENVIRON *, void *);
int     plukset(ENVIRON *, void *), pluck(ENVIRON *, void *);
int     rndset(ENVIRON *, void *), krand(ENVIRON *, void *);
int     arand(ENVIRON *, void *), rhset(ENVIRON *, void *);
int     krandh(ENVIRON *, void *), randh(ENVIRON *, void *);
int     riset(ENVIRON *, void *), krandi(ENVIRON *, void *);
int     randi(ENVIRON *, void *), rndset2(ENVIRON *, void *);
int     krand2(ENVIRON *, void *), arand2(ENVIRON *, void *);
int     rhset2(ENVIRON *, void *), krandh2(ENVIRON *, void *);
int     randh2(ENVIRON *, void *), riset2(ENVIRON *, void *);
int     krandi2(ENVIRON *, void *), randi2(ENVIRON *, void *);
int     porset(ENVIRON *, void *), port(ENVIRON *, void *);
int     tonset(ENVIRON *, void *), tone(ENVIRON *, void *);
int     atone(ENVIRON *, void *), rsnset(ENVIRON *, void *);
int     reson(ENVIRON *, void *), areson(ENVIRON *, void *);
int     resonx(ENVIRON *, void *), aresonx(ENVIRON *, void *);
int     rsnsetx(ENVIRON *, void *), tonex(ENVIRON *, void *);
int     atonex(ENVIRON *, void *), tonsetx(ENVIRON *, void *);
int     lprdset(ENVIRON *, void *), lpread(ENVIRON *, void *);
int     lprsnset(ENVIRON *, void *), lpreson(ENVIRON *, void *);
int     lpfrsnset(ENVIRON *, void *), lpfreson(ENVIRON *, void *);
int     lpslotset(ENVIRON *, void *), lpitpset(ENVIRON *, void *);
int     lpinterpol(ENVIRON *, void *);
int     rmsset(ENVIRON *, void *), rms(ENVIRON *, void *);
int     gainset(ENVIRON *, void *), gain(ENVIRON *, void *);
int     sndinset(ENVIRON *, void *), soundin(ENVIRON *, void *);
int     sndo1set(ENVIRON *, void *), soundout(ENVIRON *, void *);
int     soundouts(ENVIRON *, void *);
int     in(ENVIRON *, void *), ins(ENVIRON *, void *);
int     inq(ENVIRON *, void *), inh(ENVIRON *, void *);
int     ino(ENVIRON *, void *), in16(ENVIRON *, void *);
int     in32(ENVIRON *, void *), inall(ENVIRON *, void *);
int     out(ENVIRON *, void *), outs(ENVIRON *, void *);
int     outs1(ENVIRON *, void *), outs2(ENVIRON *, void *);
int     outall(ENVIRON *, void *), outq(ENVIRON *, void *);
int     outq1(ENVIRON *, void *), outq2(ENVIRON *, void *);
int     outq3(ENVIRON *, void *), outq4(ENVIRON *, void *);
int     igoto(ENVIRON *, void *), kgoto(ENVIRON *, void *);
int     icgoto(ENVIRON *, void *), kcgoto(ENVIRON *, void *);
int     timset(ENVIRON *, void *), timout(ENVIRON *, void *);
int     reinit(ENVIRON *, void *), rigoto(ENVIRON *, void *);
int     rireturn(ENVIRON *, void *), tigoto(ENVIRON *, void *);
int     tival(ENVIRON *, void *), printv(ENVIRON *, void *);
int     dspset(ENVIRON *, void *), kdsplay(ENVIRON *, void *);
int     dsplay(ENVIRON *, void *), fftset(ENVIRON *, void *);
int     kdspfft(ENVIRON *, void *), dspfft(ENVIRON *, void *);
int     xyinset(ENVIRON *, void *), xyin(ENVIRON *, void *);
int     tempeset(ENVIRON *, void *), tempest(ENVIRON *, void *);
int     tempset(ENVIRON *, void *), tempo(ENVIRON *, void *);
int     old_kdmpset(ENVIRON *, void *), old_kdmp2set(ENVIRON *, void *);
int     old_kdmp3set(ENVIRON *, void *), old_kdmp4set(ENVIRON *, void *);
int     kdmpset(ENVIRON *, void *), kdmp2set(ENVIRON *, void *);
int     kdmp3set(ENVIRON *, void *), kdmp4set(ENVIRON *, void *);
int     kdump(ENVIRON *, void *), kdump2(ENVIRON *, void *);
int     kdump3(ENVIRON *, void *), kdump4(ENVIRON *, void *);
int     krdset(ENVIRON *, void *), krd2set(ENVIRON *, void *);
int     krd3set(ENVIRON *, void *), krd4set(ENVIRON *, void *);
int     kread(ENVIRON *, void *), kread2(ENVIRON *, void *);
int     kread3(ENVIRON *, void *), kread4(ENVIRON *, void *);
int     ipow(ENVIRON *, void *), apow(ENVIRON *, void *);
int     alinear(ENVIRON *, void *), iklinear(ENVIRON *, void *);
int     atrian(ENVIRON *, void *), iktrian(ENVIRON *, void *);
int     aexp(ENVIRON *, void *), ikexp(ENVIRON *, void *);
int     abiexp(ENVIRON *, void *), ikbiexp(ENVIRON *, void *);
int     agaus(ENVIRON *, void *), ikgaus(ENVIRON *, void *);
int     acauchy(ENVIRON *, void *), ikcauchy(ENVIRON *, void *);
int     apcauchy(ENVIRON *, void *), ikpcauchy(ENVIRON *, void *);
int     abeta(ENVIRON *, void *), ikbeta(ENVIRON *, void *);
int     aweib(ENVIRON *, void *), ikweib(ENVIRON *, void *);
int     apoiss(ENVIRON *, void *), ikpoiss(ENVIRON *, void *);
int     seedrand(ENVIRON *, void *);
int     tblesegset(ENVIRON *, void *), ktableseg(ENVIRON *, void *);
int     ktablexseg(ENVIRON *, void *);
int     vpvset(ENVIRON *, void *), vpvoc(ENVIRON *, void *);
int     pvreadset(ENVIRON *, void *), pvread(ENVIRON *, void *);
int     pvcrossset(ENVIRON *, void *), pvcross(ENVIRON *, void *);
int     pvbufreadset(ENVIRON *, void *), pvbufread(ENVIRON *, void *);
int     pvinterpset(ENVIRON *, void *), pvinterp(ENVIRON *, void *);
int     auniform(ENVIRON *, void *), ikuniform(ENVIRON *, void *);
int     newsndinset(ENVIRON *, void *), soundinew(ENVIRON *, void *);
int     iout_on(ENVIRON *, void *), iout_off(ENVIRON *, void *);
int     out_controller(ENVIRON *, void *), iout_on_dur_set(ENVIRON *, void *);
int     iout_on_dur(ENVIRON *, void *), iout_on_dur2(ENVIRON *, void *);
int     moscil_set(ENVIRON *, void *), moscil(ENVIRON *, void *);
int     kvar_out_on_set(ENVIRON *, void *), kvar_out_on_set1(ENVIRON *, void *);
int     kvar_out_on(ENVIRON *, void *), out_controller14(ENVIRON *, void *);
int     out_pitch_bend(ENVIRON *, void *), out_aftertouch(ENVIRON *, void *);
int     out_poly_aftertouch(ENVIRON*, void*), out_progchange(ENVIRON*, void*);
int     release_set(ENVIRON *, void *), release(ENVIRON *, void *);
int     xtratim(ENVIRON *, void *);
int     mclock_set(ENVIRON *, void *), mclock(ENVIRON *, void *);
int     mrtmsg(ENVIRON *, void *);
int     cabasaset(ENVIRON *, void *), cabasa(ENVIRON *, void *);
int     sekereset(ENVIRON *, void *), sandset(ENVIRON *, void *);
int     stixset(ENVIRON *, void *), crunchset(ENVIRON *, void *);
int     guiroset(ENVIRON *, void *), guiro(ENVIRON *, void *);
int     sekere(ENVIRON *, void *);
int     tambourset(ENVIRON *, void *), tambourine(ENVIRON *, void *);
int     bambooset(ENVIRON *, void *), bamboo(ENVIRON *, void *);
int     wuterset(ENVIRON *, void *), wuter(ENVIRON *, void *);
int     sleighset(ENVIRON *, void *), sleighbells(ENVIRON *, void *);
int     trig_set(ENVIRON *, void *), trig(ENVIRON *, void *);
int     numsamp(ENVIRON *, void *), ftsr(ENVIRON *, void *);
int     kon2_set(ENVIRON *, void *), kon2(ENVIRON *, void *);
int     nrpn(ENVIRON *, void *);
int     mdelay(ENVIRON *, void *), mdelay_set(ENVIRON *, void *);
#if defined(TCLTK)
int     cntrl_set(ENVIRON *, void *);
int     control(ENVIRON *, void *), ocontrol(ENVIRON *, void *);
int     button_set(ENVIRON *, void *), button(ENVIRON *, void *);
int     check_set(ENVIRON *, void *), check(ENVIRON *, void *);
#endif
int     sum(ENVIRON *, void *), product(ENVIRON *, void *);
int     macset(ENVIRON *, void *);
int     mac(ENVIRON *, void *), maca(ENVIRON *, void *);
int     nestedapset(ENVIRON *, void *), nestedap(ENVIRON *, void *);
int     lorenzset(ENVIRON *, void *), lorenz(ENVIRON *, void *);
int     filelen(ENVIRON *, void *), filenchnls(ENVIRON *, void *);
int     filesr(ENVIRON *, void *), filepeak(ENVIRON *, void *);
int     ilogbasetwo(ENVIRON *, void *), logbasetwo_set(ENVIRON *, void *);
int     powoftwo(ENVIRON *, void *), powoftwoa(ENVIRON *, void *);
int     logbasetwo(ENVIRON *, void *), logbasetwoa(ENVIRON *, void *);
int     lp2_set(ENVIRON *, void *), lp2(ENVIRON *, void *);
int     phaser2set(ENVIRON *, void *), phaser2(ENVIRON *, void *);
int     phaser1set(ENVIRON *, void *), phaser1(ENVIRON *, void *);
int     balnset(ENVIRON *, void *), balance(ENVIRON *, void *);
int     prealloc(ENVIRON *, void *);

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
{ "pset",   S(PVSET),   0,      "",     "m"                             },
{ "ctrlinit",S(CTLINIT),1,      "",     "im",   ctrlinit                },
{ "massign",S(MASSIGN), 1,      "",     "iT",   massign                 },
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
{ "round.i",S(EVAL),    1,      "i",    "i",    int1_round              },
{ "floor.i",S(EVAL),    1,      "i",    "i",    int1_floor              },
{ "ceil.i", S(EVAL),    1,      "i",    "i",    int1_ceil               },
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
{ "round.k",S(EVAL),    2,      "k",    "k",    NULL,   int1_round      },
{ "floor.k",S(EVAL),    2,      "k",    "k",    NULL,   int1_floor      },
{ "ceil.k", S(EVAL),    2,      "k",    "k",    NULL,   int1_ceil       },
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
{ "int.a",  S(EVAL),    4,      "a",    "a",    NULL, NULL, int1a       },
{ "frac.a", S(EVAL),    4,      "a",    "a",    NULL, NULL, frac1a      },
{ "round.a",S(EVAL),    4,      "a",    "a",    NULL, NULL, int1a_round },
{ "floor.a",S(EVAL),    4,      "a",    "a",    NULL, NULL, int1a_floor },
{ "ceil.a", S(EVAL),    4,      "a",    "a",    NULL, NULL, int1a_ceil  },
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
{ "k.i",   S(ASSIGN),   1,      "k",    "i",    init                    },
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
{ "table",  0xffff                                                      },
{ "tablei", 0xffff                                                      },
{ "table3", 0xffff                                                      },
{ "table.i",  S(TABLE), 1,      "i",    "iiooo",itable                  },
{ "table.k",  S(TABLE), 3,      "k",    "kiooo",tblset, ktable          },
{ "table.a",  S(TABLE), 5,      "a",    "aiooo",tblset, NULL, tablefn   },
{ "tablei.i", S(TABLE), 1,      "i",    "iiooo",itabli                  },
{ "tablei.k", S(TABLE), 3,      "k",    "kiooo",tblset, ktabli          },
{ "tablei.a", S(TABLE), 5,      "a",    "aiooo",tblset, NULL, tabli     },
{ "table3.i", S(TABLE), 1,      "i",    "iiooo",itabl3                  },
{ "table3.k", S(TABLE), 3,      "k",    "kiooo",tblset, ktabl3          },
{ "table3.a", S(TABLE), 5,      "a",    "aiooo",tblset, NULL, tabl3     },
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
{ "adsyn",  S(ADSYN),   5,      "a",    "kkkTo", adset, NULL,   adsyn   },
{ "pvoc",   S(PVOC),    5,      "a",  "kkToooo", pvset, NULL,   pvoc    },
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
{ "lpread", S(LPREAD),  3,      "kkkk", "kToo", lprdset,lpread          },
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
{ "soundin",S(SOUNDIN_),5,"mmmmmmmmmmmmmmmmmmmmmmmm","Toooo",
                                                sndinset, NULL, soundin   },
{ "soundout",S(SNDOUT), 5,      "",     "aTo",  sndo1set, NULL, soundout  },
{ "soundouts",S(SNDOUTS),5,     "",     "aaTo", sndo1set, NULL, soundouts },
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
{ "dumpk",  S(KDUMP),   3,      "",     "kTii", kdmpset,kdump           },
{ "dumpk2", S(KDUMP2),  3,      "",     "kkTii",kdmp2set,kdump2         },
{ "dumpk3", S(KDUMP3),  3,      "",     "kkkTii",kdmp3set,kdump3        },
{ "dumpk4", S(KDUMP4),  3,      "",     "kkkkTii",kdmp4set,kdump4       },
{ "readk",  S(KREAD),   3,      "k",    "Tiio",  krdset, kread          },
{ "readk2", S(KREAD2),  3,      "kk",   "Tiio",  krd2set, kread2        },
{ "readk3", S(KREAD3),  3,      "kkk",  "Tiio",  krd3set, kread3        },
{ "readk4", S(KREAD4),  3,      "kkkk", "Tiio",  krd4set, kread4        },
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
{ "tableseg", S(TABLESEG), 3,  "",      "iin",  tblesegset, ktableseg},
{ "ktableseg", S(TABLESEG), 3,  "",     "iin",  tblesegset, ktableseg},
{ "tablexseg", S(TABLESEG), 3, "",      "iin",  tblesegset, ktablexseg},
{ "vpvoc",    S(VPVOC), 5,     "a",     "kkToo", vpvset,        NULL,   vpvoc},
{ "pvread",   S(PVREAD),3,     "kk",    "kTi",  pvreadset, pvread},
{ "pvcross",  S(PVCROSS),  5,  "a",     "kkTkko", pvcrossset, NULL, pvcross},
{ "pvbufread",S(PVBUFREAD), 3, "",      "kT",   pvbufreadset, pvbufread, NULL},
{ "pvinterp", S(PVINTERP), 5,  "a",  "kkTkkkkkk", pvinterpset, NULL, pvinterp},
{ "pvadd",    S(PVADD), 5,     "a", "kkTiiopooo", pvaddset,     NULL,   pvadd},
{ "unirand.i",S(PRAND), 1,     "i",     "k",    ikuniform, NULL,  NULL  },
{ "unirand.k",S(PRAND), 2,     "k",     "k",    NULL,    ikuniform, NULL},
{ "unirand.a",S(PRAND), 4,     "a",     "k",    NULL,    NULL, auniform },
{ "diskin",S(SOUNDINEW),5,  "mmmmmmmmmmmmmmmmmmmmmmmm", "Tkoooo",
                            (SUBR) newsndinset, NULL, (SUBR) soundinew  },
{ "diskin2",S(DISKIN2), 5,  "mmmmmmmmmmmmmmmmmmmmmmmm", "Tkoooooo",
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
{ "powoftwo.i",S(EVAL),  1,     "i",    "i",    powoftwo                   },
{ "powoftwo.k",S(EVAL),  2,     "k",    "k",    NULL, powoftwo             },
{ "powoftwo.a",S(EVAL),  4,     "a",    "a",    NULL, NULL, powoftwoa      },
{ "logbtwo.i",S(EVAL),   1,     "i",    "i",    ilogbasetwo                },
{ "logbtwo.k",S(EVAL),   3,     "k",    "k",    logbasetwo_set, logbasetwo },
{ "logbtwo.a",S(EVAL),   5,     "a",    "a", logbasetwo_set, NULL, logbasetwoa },
{ "filelen", S(SNDINFO), 1,     "i",    "T",    filelen, NULL, NULL        },
{ "filenchnls", S(SNDINFO), 1,  "i",    "T",    filenchnls, NULL, NULL     },
{ "filesr", S(SNDINFO),  1,     "i",    "T",    filesr, NULL, NULL         },
{ "filepeak", S(SNDINFOPEAK), 1, "i",   "To",   filepeak, NULL, NULL       },
/*  { "nlalp", S(NLALP),     5,     "a",    "akkoo", nlalp_set, NULL, nlalp }, */
/* Robin Whittle */
{ "tableiw",  S(TABLEW),1,     "",      "iiiooo", (SUBR)itablew, NULL, NULL},
{ "tablew.kk", S(TABLEW),3,    "", "kkiooo",(SUBR)tblsetw,(SUBR)ktablew, NULL},
{ "tablew.aa", S(TABLEW),5,    "", "aaiooo",(SUBR)tblsetw, NULL, (SUBR)tablew},
{ "tablewkt.kk", S(TABLEW),3, "",  "kkkooo",(SUBR)tblsetwkt,(SUBR)ktablewkt,NULL},
{ "tablewkt.aa", S(TABLEW),5, "",  "aakooo",(SUBR)tblsetwkt,NULL,(SUBR)tablewkt},
{ "tableng.i", S(TABLENG),1,  "i",     "i",    (SUBR)itableng, NULL,  NULL},
{ "tableng.k",  S(TABLENG),2, "k",     "k",    NULL,   (SUBR)tableng, NULL},
{ "tableigpw",S(TABLENG),1,   "",  "i",    (SUBR)itablegpw, NULL,  NULL},
{ "tablegpw", S(TABLENG),2,   "",  "k",    NULL,   (SUBR)tablegpw, NULL},
{ "tableimix",S(TABLEMIX),1,  "",  "iiiiiiiii", (SUBR)itablemix, NULL, NULL},
{ "tablemix", S(TABLEMIX),2,  "",  "kkkkkkkkk",
                                   (SUBR)tablemixset, (SUBR)tablemix, NULL},
{ "tableicopy",S(TABLECOPY),1, "", "ii",   (SUBR)itablecopy, NULL, NULL},
{ "tablecopy", S(TABLECOPY),2, "", "kk",
                                      (SUBR)tablecopyset, (SUBR)tablecopy, NULL},
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
{ "printks",S(PRINTKS), 3,   "",   "TiM",  (SUBR)printksset,(SUBR)printks, NULL },
{ "prints",S(PRINTS),   1,   "",   "TM",   (SUBR)printsset, NULL, NULL },
{ "printk2", S(PRINTK2),3,   "",   "ko",   (SUBR)printk2set, (SUBR)printk2, NULL },
{ "portk",  S(KPORT),   3, "k",     "kko",  (SUBR)kporset, (SUBR)kport, NULL   },
{ "tonek",  S(KTONE),   3, "k",     "kko",  (SUBR)ktonset, (SUBR)ktone, NULL   },
{ "atonek", S(KTONE),   3, "k",     "kko",  (SUBR)ktonset, (SUBR)katone, NULL  },
{ "resonk", S(KRESON),  3, "k",     "kkkpo",(SUBR)krsnset, (SUBR)kreson, NULL  },
{ "aresonk",S(KRESON),  3, "k",     "kkkpo",(SUBR)krsnset, (SUBR)kareson, NULL },
{ "limit.i", S(LIMIT),  1, "i",     "iii",  (SUBR)klimit,  NULL,    NULL       },
{ "limit.i", S(LIMIT),  1, "i",     "iii",  (SUBR)klimit,  NULL,    NULL       },
{ "limit.k",  S(LIMIT), 3, "k",     "xkk",  (SUBR)limitset, (SUBR)klimit, NULL },
{ "limit.a",  S(LIMIT), 5, "a",     "xkk",  (SUBR)limitset, NULL,  (SUBR)limit },
{ "prealloc", S(AOP),   1, "",      "Tio",  (SUBR)prealloc, NULL, NULL  },
/* terminate list */
{ NULL, 0, 0, NULL, NULL, (SUBR) NULL, (SUBR) NULL, (SUBR) NULL }
};

