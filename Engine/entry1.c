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

#define S(x)    sizeof(x)

int     ihold(CSOUND *, void *), turnoff(CSOUND *, void *);
int     assign(CSOUND *, void *), rassign(CSOUND *, void *);
int     aassign(CSOUND *, void *);
int     init(CSOUND *, void *), ainit(CSOUND *, void *);
int     gt(CSOUND *, void *), ge(CSOUND *, void *);
int     lt(CSOUND *, void *), le(CSOUND *, void *);
int     eq(CSOUND *, void *), ne(CSOUND *, void *);
int     and(CSOUND *, void *), or(CSOUND *, void *);
int     conval(CSOUND *, void *), aconval(CSOUND *, void *);
int     addkk(CSOUND *, void *), subkk(CSOUND *, void *);
int     mulkk(CSOUND *, void *), divkk(CSOUND *, void *);
int     modkk(CSOUND *, void *);
int     addka(CSOUND *, void *), subka(CSOUND *, void *);
int     mulka(CSOUND *, void *), divka(CSOUND *, void *);
int     modka(CSOUND *, void *);
int     addak(CSOUND *, void *), subak(CSOUND *, void *);
int     mulak(CSOUND *, void *), divak(CSOUND *, void *);
int     modak(CSOUND *, void *);
int     addaa(CSOUND *, void *), subaa(CSOUND *, void *);
int     mulaa(CSOUND *, void *), divaa(CSOUND *, void *);
int     modaa(CSOUND *, void *);
int     divzkk(CSOUND *, void *), divzka(CSOUND *, void *);
int     divzak(CSOUND *, void *), divzaa(CSOUND *, void *);
int     int1(CSOUND *, void *), int1a(CSOUND *, void *);
int     frac1(CSOUND *, void *), frac1a(CSOUND *, void *);
int     int1_round(CSOUND *, void *), int1a_round(CSOUND *, void *);
int     int1_floor(CSOUND *, void *), int1a_floor(CSOUND *, void *);
int     int1_ceil(CSOUND *, void *), int1a_ceil(CSOUND *, void *);
int     rnd1(CSOUND *, void *), birnd1(CSOUND *, void *);
int     abs1(CSOUND *, void *), exp01(CSOUND *, void *);
int     log01(CSOUND *, void *), sqrt1(CSOUND *, void *);
int     sin1(CSOUND *, void *), cos1(CSOUND *, void *);
int     tan1(CSOUND *, void *), asin1(CSOUND *, void *);
int     acos1(CSOUND *, void *), atan1(CSOUND *, void *);
int     sinh1(CSOUND *, void *), cosh1(CSOUND *, void *);
int     tanh1(CSOUND *, void *), log101(CSOUND *, void *);
int     atan21(CSOUND *, void *), atan2aa(CSOUND *, void *);
int     absa(CSOUND *, void *), expa(CSOUND *, void *);
int     loga(CSOUND *, void *), sqrta(CSOUND *, void *);
int     sina(CSOUND *, void *), cosa(CSOUND *, void *);
int     tana(CSOUND *, void *), asina(CSOUND *, void *);
int     acosa(CSOUND *, void *), atana(CSOUND *, void *);
int     sinha(CSOUND *, void *), cosha(CSOUND *, void *);
int     tanha(CSOUND *, void *), log10a(CSOUND *, void *);
int     dbamp(CSOUND *, void *), ampdb(CSOUND *, void *);
int     aampdb(CSOUND *, void *), dbfsamp(CSOUND *, void *);
int     ampdbfs(CSOUND *, void *), aampdbfs(CSOUND *, void *);
int     ftlen(CSOUND *, void *), ftlptim(CSOUND *, void *);
int     ftchnls(CSOUND *, void *), rtclock(CSOUND *, void *);
int     cpsoct(CSOUND *, void *), octpch(CSOUND *, void *);
int     cpspch(CSOUND *, void *), pchoct(CSOUND *, void *);
int     octcps(CSOUND *, void *), acpsoct(CSOUND *, void *);
int     massign(CSOUND *, void *), ctrlinit(CSOUND *, void *);
int     notnum(CSOUND *, void *), veloc(CSOUND *, void *);
int     pchmidi(CSOUND *, void *), pchmidib(CSOUND *, void *);
int     octmidi(CSOUND *, void *), octmidib(CSOUND *, void *);
int     pchmidib_i(CSOUND *, void *), octmidib_i(CSOUND *, void *);
int     icpsmidib_i(CSOUND *, void *), cpsmidi(CSOUND *, void *);
int     icpsmidib(CSOUND *, void *), kcpsmidib(CSOUND *, void *);
int     kmbset(CSOUND *, void *), midibset(CSOUND *, void *);
int     ipchmidib(CSOUND *, void *), ioctmidib(CSOUND *, void *);
int     kpchmidib(CSOUND *, void *), koctmidib(CSOUND *, void *);
int     msclset(CSOUND *, void *), ampmidi(CSOUND *, void *);
int     aftset(CSOUND *, void *), aftouch(CSOUND *, void *);
int     chpress(CSOUND *, void *), ipchbend(CSOUND *, void *);
int     kbndset(CSOUND *, void *), kpchbend(CSOUND *, void *);
int     imidictl(CSOUND *, void *), mctlset(CSOUND *, void *);
int     midictl(CSOUND *, void *), imidiaft(CSOUND *, void *);
int     maftset(CSOUND *, void *), midiaft(CSOUND *, void *);
int     midiout(CSOUND *, void *), turnon(CSOUND *, void *);
int     kmapset(CSOUND *, void *), polyaft(CSOUND *, void *);
int     ichanctl(CSOUND *, void *), chctlset(CSOUND *, void *);
int     chanctl(CSOUND *, void *), linset(CSOUND *, void *);
int     kline(CSOUND *, void *), aline(CSOUND *, void *);
int     expset(CSOUND *, void *), kexpon(CSOUND *, void *);
int     expon(CSOUND *, void *), lsgset(CSOUND *, void *);
int     klnseg(CSOUND *, void *), linseg(CSOUND *, void *);
int     madsrset(CSOUND *, void *), adsrset(CSOUND *, void *);
int     xdsrset(CSOUND *, void *), mxdsrset(CSOUND *, void *);
int     expseg2(CSOUND *, void *), xsgset(CSOUND *, void *);
int     kxpseg(CSOUND *, void *), expseg(CSOUND *, void *);
int     xsgset2(CSOUND *, void *), lsgrset(CSOUND *, void *);
int     klnsegr(CSOUND *, void *), linsegr(CSOUND *, void *);
int     xsgrset(CSOUND *, void *), kxpsegr(CSOUND *, void *);
int     expsegr(CSOUND *, void *), lnnset(CSOUND *, void *);
int     klinen(CSOUND *, void *), linen(CSOUND *, void *);
int     lnrset(CSOUND *, void *), klinenr(CSOUND *, void *);
int     linenr(CSOUND *, void *), evxset(CSOUND *, void *);
int     knvlpx(CSOUND *, void *), envlpx(CSOUND *, void *);
int     evrset(CSOUND *, void *), knvlpxr(CSOUND *, void *);
int     envlpxr(CSOUND *, void *), phsset(CSOUND *, void *);
int     kphsor(CSOUND *, void *), phsor(CSOUND *, void *);
int     itablew1(CSOUND *, void *), itablegpw1(CSOUND *, void *);
int     itablemix1(CSOUND *, void *), itablecopy1(CSOUND *, void *);
int     itable(CSOUND *, void *), itabli(CSOUND *, void *);
int     itabl3(CSOUND *, void *), tabl3(CSOUND *, void *);
int     ktabl3(CSOUND *, void *), tblset(CSOUND *, void *);
int     ktable(CSOUND *, void *), ktabli(CSOUND *, void *);
int     tabli(CSOUND *, void *), tablefn(CSOUND *, void *);
int     tblsetkt(CSOUND *, void *), ktablekt(CSOUND *, void *);
int     tablekt(CSOUND *, void *), ktablikt(CSOUND *, void *);
int     tablikt(CSOUND *, void *), ko1set(CSOUND *, void *);
int     kosc1(CSOUND *, void *), kosc1i(CSOUND *, void *);
int     oscnset(CSOUND *, void *), osciln(CSOUND *, void *);
int     oscset(CSOUND *, void *), koscil(CSOUND *, void *);
int     osckk(CSOUND *, void *), oscka(CSOUND *, void *);
int     oscak(CSOUND *, void *), oscaa(CSOUND *, void *);
int     koscli(CSOUND *, void *), osckki(CSOUND *, void *);
int     osckai(CSOUND *, void *), oscaki(CSOUND *, void *);
int     oscaai(CSOUND *, void *), foscset(CSOUND *, void *);
int     foscil(CSOUND *, void *), foscili(CSOUND *, void *);
int     losset(CSOUND *, void *), loscil(CSOUND *, void *);
int     loscil3(CSOUND *, void *), koscl3(CSOUND *, void *);
int     osckk3(CSOUND *, void *), oscka3(CSOUND *, void *);
int     oscak3(CSOUND *, void *), oscaa3(CSOUND *, void *);
int     adset(CSOUND *, void *), adsyn(CSOUND *, void *);
int     pvset(CSOUND *, void *), pvoc(CSOUND *, void *);
int     pvaddset(CSOUND *, void *), pvadd(CSOUND *, void *);
int     bzzset(CSOUND *, void *), buzz(CSOUND *, void *);
int     gbzset(CSOUND *, void *), gbuzz(CSOUND *, void *);
int     plukset(CSOUND *, void *), pluck(CSOUND *, void *);
int     rndset(CSOUND *, void *), krand(CSOUND *, void *);
int     arand(CSOUND *, void *), rhset(CSOUND *, void *);
int     krandh(CSOUND *, void *), randh(CSOUND *, void *);
int     riset(CSOUND *, void *), krandi(CSOUND *, void *);
int     randi(CSOUND *, void *), rndset2(CSOUND *, void *);
int     krand2(CSOUND *, void *), arand2(CSOUND *, void *);
int     rhset2(CSOUND *, void *), krandh2(CSOUND *, void *);
int     randh2(CSOUND *, void *), riset2(CSOUND *, void *);
int     krandi2(CSOUND *, void *), randi2(CSOUND *, void *);
int     porset(CSOUND *, void *), port(CSOUND *, void *);
int     tonset(CSOUND *, void *), tone(CSOUND *, void *);
int     atone(CSOUND *, void *), rsnset(CSOUND *, void *);
int     reson(CSOUND *, void *), areson(CSOUND *, void *);
int     resonx(CSOUND *, void *), aresonx(CSOUND *, void *);
int     rsnsetx(CSOUND *, void *), tonex(CSOUND *, void *);
int     atonex(CSOUND *, void *), tonsetx(CSOUND *, void *);
int     lprdset(CSOUND *, void *), lpread(CSOUND *, void *);
int     lprsnset(CSOUND *, void *), lpreson(CSOUND *, void *);
int     lpfrsnset(CSOUND *, void *), lpfreson(CSOUND *, void *);
int     lpslotset(CSOUND *, void *), lpitpset(CSOUND *, void *);
int     lpinterpol(CSOUND *, void *);
int     rmsset(CSOUND *, void *), rms(CSOUND *, void *);
int     gainset(CSOUND *, void *), gain(CSOUND *, void *);
int     sndinset(CSOUND *, void *), soundin(CSOUND *, void *);
int     sndo1set(CSOUND *, void *), soundout(CSOUND *, void *);
int     soundouts(CSOUND *, void *);
int     in(CSOUND *, void *), ins(CSOUND *, void *);
int     inq(CSOUND *, void *), inh(CSOUND *, void *);
int     ino(CSOUND *, void *), in16(CSOUND *, void *);
int     in32(CSOUND *, void *), inall(CSOUND *, void *);
int     out(CSOUND *, void *), outs(CSOUND *, void *);
int     outs1(CSOUND *, void *), outs2(CSOUND *, void *);
int     outall(CSOUND *, void *), outq(CSOUND *, void *);
int     outq1(CSOUND *, void *), outq2(CSOUND *, void *);
int     outq3(CSOUND *, void *), outq4(CSOUND *, void *);
int     igoto(CSOUND *, void *), kgoto(CSOUND *, void *);
int     icgoto(CSOUND *, void *), kcgoto(CSOUND *, void *);
int     timset(CSOUND *, void *), timout(CSOUND *, void *);
int     reinit(CSOUND *, void *), rigoto(CSOUND *, void *);
int     rireturn(CSOUND *, void *), tigoto(CSOUND *, void *);
int     tival(CSOUND *, void *), printv(CSOUND *, void *);
int     dspset(CSOUND *, void *), kdsplay(CSOUND *, void *);
int     dsplay(CSOUND *, void *), fftset(CSOUND *, void *);
int     kdspfft(CSOUND *, void *), dspfft(CSOUND *, void *);
int     xyinset(CSOUND *, void *), xyin(CSOUND *, void *);
int     tempeset(CSOUND *, void *), tempest(CSOUND *, void *);
int     tempset(CSOUND *, void *), tempo(CSOUND *, void *);
int     old_kdmpset(CSOUND *, void *), old_kdmp2set(CSOUND *, void *);
int     old_kdmp3set(CSOUND *, void *), old_kdmp4set(CSOUND *, void *);
int     kdmpset(CSOUND *, void *), kdmp2set(CSOUND *, void *);
int     kdmp3set(CSOUND *, void *), kdmp4set(CSOUND *, void *);
int     kdump(CSOUND *, void *), kdump2(CSOUND *, void *);
int     kdump3(CSOUND *, void *), kdump4(CSOUND *, void *);
int     krdset(CSOUND *, void *), krd2set(CSOUND *, void *);
int     krd3set(CSOUND *, void *), krd4set(CSOUND *, void *);
int     kread(CSOUND *, void *), kread2(CSOUND *, void *);
int     kread3(CSOUND *, void *), kread4(CSOUND *, void *);
int     ipow(CSOUND *, void *), apow(CSOUND *, void *);
int     alinear(CSOUND *, void *), iklinear(CSOUND *, void *);
int     atrian(CSOUND *, void *), iktrian(CSOUND *, void *);
int     aexp(CSOUND *, void *), ikexp(CSOUND *, void *);
int     abiexp(CSOUND *, void *), ikbiexp(CSOUND *, void *);
int     agaus(CSOUND *, void *), ikgaus(CSOUND *, void *);
int     acauchy(CSOUND *, void *), ikcauchy(CSOUND *, void *);
int     apcauchy(CSOUND *, void *), ikpcauchy(CSOUND *, void *);
int     abeta(CSOUND *, void *), ikbeta(CSOUND *, void *);
int     aweib(CSOUND *, void *), ikweib(CSOUND *, void *);
int     apoiss(CSOUND *, void *), ikpoiss(CSOUND *, void *);
int     seedrand(CSOUND *, void *);
int     tblesegset(CSOUND *, void *), ktableseg(CSOUND *, void *);
int     ktablexseg(CSOUND *, void *);
int     vpvset(CSOUND *, void *), vpvoc(CSOUND *, void *);
int     pvreadset(CSOUND *, void *), pvread(CSOUND *, void *);
int     pvcrossset(CSOUND *, void *), pvcross(CSOUND *, void *);
int     pvbufreadset(CSOUND *, void *), pvbufread(CSOUND *, void *);
int     pvinterpset(CSOUND *, void *), pvinterp(CSOUND *, void *);
int     auniform(CSOUND *, void *), ikuniform(CSOUND *, void *);
int     newsndinset(CSOUND *, void *), soundinew(CSOUND *, void *);
int     iout_on(CSOUND *, void *), iout_off(CSOUND *, void *);
int     out_controller(CSOUND *, void *), iout_on_dur_set(CSOUND *, void *);
int     iout_on_dur(CSOUND *, void *), iout_on_dur2(CSOUND *, void *);
int     moscil_set(CSOUND *, void *), moscil(CSOUND *, void *);
int     kvar_out_on_set(CSOUND *, void *), kvar_out_on_set1(CSOUND *, void *);
int     kvar_out_on(CSOUND *, void *), out_controller14(CSOUND *, void *);
int     out_pitch_bend(CSOUND *, void *), out_aftertouch(CSOUND *, void *);
int     out_poly_aftertouch(CSOUND*, void*), out_progchange(CSOUND*, void*);
int     release_set(CSOUND *, void *), release(CSOUND *, void *);
int     xtratim(CSOUND *, void *);
int     mclock_set(CSOUND *, void *), mclock(CSOUND *, void *);
int     mrtmsg(CSOUND *, void *);
int     cabasaset(CSOUND *, void *), cabasa(CSOUND *, void *);
int     sekereset(CSOUND *, void *), sandset(CSOUND *, void *);
int     stixset(CSOUND *, void *), crunchset(CSOUND *, void *);
int     guiroset(CSOUND *, void *), guiro(CSOUND *, void *);
int     sekere(CSOUND *, void *);
int     tambourset(CSOUND *, void *), tambourine(CSOUND *, void *);
int     bambooset(CSOUND *, void *), bamboo(CSOUND *, void *);
int     wuterset(CSOUND *, void *), wuter(CSOUND *, void *);
int     sleighset(CSOUND *, void *), sleighbells(CSOUND *, void *);
int     trig_set(CSOUND *, void *), trig(CSOUND *, void *);
int     numsamp(CSOUND *, void *), ftsr(CSOUND *, void *);
int     kon2_set(CSOUND *, void *), kon2(CSOUND *, void *);
int     nrpn(CSOUND *, void *);
int     mdelay(CSOUND *, void *), mdelay_set(CSOUND *, void *);
#if defined(TCLTK)
int     cntrl_set(CSOUND *, void *);
int     control(CSOUND *, void *), ocontrol(CSOUND *, void *);
int     button_set(CSOUND *, void *), button(CSOUND *, void *);
int     check_set(CSOUND *, void *), check(CSOUND *, void *);
#endif
int     sum(CSOUND *, void *), product(CSOUND *, void *);
int     macset(CSOUND *, void *);
int     mac(CSOUND *, void *), maca(CSOUND *, void *);
int     nestedapset(CSOUND *, void *), nestedap(CSOUND *, void *);
int     lorenzset(CSOUND *, void *), lorenz(CSOUND *, void *);
int     filelen(CSOUND *, void *), filenchnls(CSOUND *, void *);
int     filesr(CSOUND *, void *), filepeak(CSOUND *, void *);
int     ilogbasetwo(CSOUND *, void *), logbasetwo_set(CSOUND *, void *);
int     powoftwo(CSOUND *, void *), powoftwoa(CSOUND *, void *);
int     logbasetwo(CSOUND *, void *), logbasetwoa(CSOUND *, void *);
int     lp2_set(CSOUND *, void *), lp2(CSOUND *, void *);
int     phaser2set(CSOUND *, void *), phaser2(CSOUND *, void *);
int     phaser1set(CSOUND *, void *), phaser1(CSOUND *, void *);
int     balnset(CSOUND *, void *), balance(CSOUND *, void *);
int     prealloc(CSOUND *, void *);

/* thread vals, where isub=1, ksub=2, asub=4:
                0 =     1  OR   2  (B out only)
                1 =     1
                2 =             2
                3 =     1  AND  2
                4 =                     4
                5 =     1  AND          4
                7 =     1  AND (2  OR   4)                              */

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
                Z       begins alternating kakaka...list (any count)    */

/* outarg types include:
                m       multiple out aargs
                z               multiple out kargs
                X       multiple args (any rate)        IV - Sep 1 2002
   (these types must agree with rdorch.c)                               */

/* If dsblksize is 0xffff then translate on output arg
                   0xfffe then translate two (oscil)
                   0xfffd then translate on first input arg (peak)
                   0xfffc then translate two (divz)
                   0xfffb then translate on first input arg (loop_l)    */

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

