/*
  entry1.h:

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

#include "csoundCore.h"         /*                      ENTRY1.H        */
#include "insert.h"
#include "aops.h"
#include "midiops.h"
#include "ugens1.h"
#include "ugens2.h"
#include "ugens3.h"
#include "ugens4.h"
#include "ugens5.h"
#include "ugens6.h"
#include "cwindow.h"
#include "windin.h"
#include "disprep.h"
#include "soundio.h"
#include "dumpf.h"
#include "cmath.h"
#include "diskin2.h"
#include "oload.h"
#include "midiout.h"
#include "sndinfUG.h"
#include "ugrw1.h"
#include "ugrw2.h"
#include "schedule.h"
#include "vdelay.h"
#include "pstream.h"
#include "oscils.h"
#include "midifile.h"
#include "midiinterop.h"
#include "linevent.h"
#include "str_ops.h"
#include "bus.h"
#include "pstream.h"
#include "remote.h"
#include "resize.h"
#include "cs_par_ops.h"
#include "ugtabs.h"
#include "compile_ops.h"

#define S(x)    sizeof(x)

int     ihold(CSOUND *, void *), turnoff(CSOUND *, void *);
int     assign(CSOUND *, void *), rassign(CSOUND *, void *);
int     aassign(CSOUND *, void *);
int     init(CSOUND *, void *), ainit(CSOUND *, void *);
int     minit(CSOUND *, void *), mainit(CSOUND *, void *);
/* int     tinit(CSOUND *, void *), tassign(CSOUND *, void *); */
/* int     tabref_check(CSOUND *, void *), tabref(CSOUND *, void *); */
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
int     is_NaN(CSOUND *, void *), is_NaNa(CSOUND *, void *);
int     is_inf(CSOUND *, void *), is_infa(CSOUND *, void *);
int     tan1(CSOUND *, void *), asin1(CSOUND *, void *);
int     acos1(CSOUND *, void *), atan1(CSOUND *, void *);
int     sinh1(CSOUND *, void *), cosh1(CSOUND *, void *);
int     tanh1(CSOUND *, void *), log101(CSOUND *, void *), log21(CSOUND *, void *);
int     atan21(CSOUND *, void *), atan2aa(CSOUND *, void *);
int     absa(CSOUND *, void *), expa(CSOUND *, void *);
int     loga(CSOUND *, void *), sqrta(CSOUND *, void *);
int     sina(CSOUND *, void *), cosa(CSOUND *, void *);
int     tana(CSOUND *, void *), asina(CSOUND *, void *);
int     acosa(CSOUND *, void *), atana(CSOUND *, void *);
int     sinha(CSOUND *, void *), cosha(CSOUND *, void *);
int     tanha(CSOUND *, void *), log10a(CSOUND *, void *), log2a(CSOUND *, void *);
int     dbamp(CSOUND *, void *), ampdb(CSOUND *, void *);
int     aampdb(CSOUND *, void *), dbfsamp(CSOUND *, void *);
int     ampdbfs(CSOUND *, void *), aampdbfs(CSOUND *, void *);
int     ftlen(CSOUND *, void *), ftlptim(CSOUND *, void *);
int     ftchnls(CSOUND *, void *), ftcps(CSOUND *, void *);
int     signum(CSOUND *, void *), asignum(CSOUND *, void *);
int     rtclock(CSOUND *, void *);
int     cpsoct(CSOUND *, void *), octpch(CSOUND *, void *);
int     cpspch(CSOUND *, void *), pchoct(CSOUND *, void *);
int     octcps(CSOUND *, void *), acpsoct(CSOUND *, void *);
int     cpsmidinn(CSOUND *, void *), octmidinn(CSOUND *, void *);
int     pchmidinn(CSOUND *, void *);int     massign_S(CSOUND *, void *);
int     massign_p(CSOUND *, void *), ctrlinit(CSOUND *, void *);
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
int     turnon_S(CSOUND *, void *);
int     kmapset(CSOUND *, void *), polyaft(CSOUND *, void *);
int     ichanctl(CSOUND *, void *), chctlset(CSOUND *, void *);
int     chanctl(CSOUND *, void *), linset(CSOUND *, void *);
int     kline(CSOUND *, void *), aline(CSOUND *, void *);
int     expset(CSOUND *, void *), kexpon(CSOUND *, void *);
int     expon(CSOUND *, void *), lsgset(CSOUND *, void *);
int     klnseg(CSOUND *, void *), linseg(CSOUND *, void *);
int     csgset(CSOUND *, void *), kosseg(CSOUND *, void *);
int     csgset_bkpt(CSOUND *, void *), cosseg(CSOUND *, void *);
int     csgrset(CSOUND *, void *);
int     kcssegr(CSOUND *, void *), cossegr(CSOUND *, void *);
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
int     ephsset(CSOUND *, void *), ephsor(CSOUND *, void *);
int     kphsor(CSOUND *, void *), phsor(CSOUND *, void *);
int     itablew1(CSOUND *, void *), itablegpw1(CSOUND *, void *);
int     itablemix1(CSOUND *, void *), itablecopy1(CSOUND *, void *);
int     itable(CSOUND *, void *), itabli(CSOUND *, void *);
int     itabl3(CSOUND *, void *), tabl3(CSOUND *, void *);
int     ktabl3(CSOUND *, void *), tblset(CSOUND *, void *);
int     ktable(CSOUND *, void *), ktabli(CSOUND *, void *);
int     tabli(CSOUND *, void *), tablefn(CSOUND *, void *);
int     pitable(CSOUND *, void *), pitabli(CSOUND *, void *);
int     pitabl3(CSOUND *, void *), ptabl3(CSOUND *, void *);
int     pktabl3(CSOUND *, void *), itblchkw(CSOUND *, void *);
int     pktable(CSOUND *, void *), pktabli(CSOUND *, void *);
int     ptabli(CSOUND *, void *), ptablefn(CSOUND *, void *);
int     tblsetkt(CSOUND *, void *), ktablekt(CSOUND *, void *);
int     pitablew(CSOUND *, void *), ptblsetw(CSOUND *, void *);
int     pktablew(CSOUND *, void *), ptablew(CSOUND *, void *);
int     tablekt(CSOUND *, void *), ktablikt(CSOUND *, void *);
int     tablikt(CSOUND *, void *), ko1set(CSOUND *, void *);
int     kosc1(CSOUND *, void *), kosc1i(CSOUND *, void *);
int     oscnset(CSOUND *, void *), osciln(CSOUND *, void *);
int     oscset(CSOUND *, void *), koscil(CSOUND *, void *);
int     oscsetA(CSOUND *, void *);
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
int     lpformantset(CSOUND *, void *), lpformant(CSOUND *, void*);
int     lprsnset(CSOUND *, void *), lpreson(CSOUND *, void *);
int     lpfrsnset(CSOUND *, void *), lpfreson(CSOUND *, void *);
int     lpslotset(CSOUND *, void *), lpitpset(CSOUND *, void *);
int     lpinterpol(CSOUND *, void *);
int     rmsset(CSOUND *, void *), rms(CSOUND *, void *);
int     gainset(CSOUND *, void *), gain(CSOUND *, void *);
int     sndinset(CSOUND *, void *), sndinset_S(CSOUND *, void *),
        soundin(CSOUND *, void *);
int     sndoutset(CSOUND *, void *), sndoutset_S(CSOUND *, void *),
        soundout(CSOUND *, void *);
int     soundouts(CSOUND *, void *), inarray(CSOUND *, void *);
int     in(CSOUND *, void *), ins(CSOUND *, void *);
int     inq(CSOUND *, void *), inh(CSOUND *, void *);
int     ino(CSOUND *, void *), in16(CSOUND *, void *);
int     in32(CSOUND *, void *), outarr(CSOUND *, void *);
int     inch_opcode(CSOUND *, void *), inall_opcode(CSOUND *, void *);
int     inch_set(CSOUND*, void*), outmultiple(CSOUND *, void *);
/* int     out(CSOUND *, void *), outs(CSOUND *, void *); */
int     outs1(CSOUND *, void *), outs2(CSOUND *, void *);
/* int     outq(CSOUND *, void *); */
int     outq1(CSOUND *, void *), outq2(CSOUND *, void *);
int     outq3(CSOUND *, void *), outq4(CSOUND *, void *);
/* int     outh(CSOUND *, void *), outo(CSOUND *, void *); */
/* int     outx(CSOUND *, void *), outX(CSOUND *, void *); */
int     outch(CSOUND *, void *), outall(CSOUND *, void *);
int     igoto(CSOUND *, void *), kgoto(CSOUND *, void *);
int     icgoto(CSOUND *, void *), kcgoto(CSOUND *, void *);
int     timset(CSOUND *, void *), timout(CSOUND *, void *);
int     reinit(CSOUND *, void *), rigoto(CSOUND *, void *);
int     rireturn(CSOUND *, void *), tigoto(CSOUND *, void *);
int     tival(CSOUND *, void *), printv(CSOUND *, void *);
int     dspset(CSOUND *, void *), kdsplay(CSOUND *, void *);
int     fdspset(CSOUND *, void *), fdsplay(CSOUND *, void *);
int     dsplay(CSOUND *, void *), fftset(CSOUND *, void *);
int     kdspfft(CSOUND *, void *), dspfft(CSOUND *, void *);
int     xyinset(CSOUND *, void *);
int     tempeset(CSOUND *, void *), tempest(CSOUND *, void *);
int     tempset(CSOUND *, void *), tempo(CSOUND *, void *);
int     old_kdmpset(CSOUND *, void *), old_kdmp2set(CSOUND *, void *);
int     old_kdmp3set(CSOUND *, void *), old_kdmp4set(CSOUND *, void *);
int     kdmpset_p(CSOUND *, void *), kdmp2set_p(CSOUND *, void *);
int     kdmp3set_p(CSOUND *, void *), kdmp4set_p(CSOUND *, void *);
int     kdmpset_S(CSOUND *, void *), kdmp2set_S(CSOUND *, void *);
int     kdmp3set_S(CSOUND *, void *), kdmp4set_S(CSOUND *, void *);
int     kdump(CSOUND *, void *), kdump2(CSOUND *, void *);
int     kdump3(CSOUND *, void *), kdump4(CSOUND *, void *);
int     krdset_S(CSOUND *, void *), krd2set_S(CSOUND *, void *);
int     krd3set_S(CSOUND *, void *), krd4set_S(CSOUND *, void *);
int     krdset_p(CSOUND *, void *), krd2set_p(CSOUND *, void *);
int     krd3set_p(CSOUND *, void *), krd4set_p(CSOUND *, void *);
int     kread(CSOUND *, void *), kread2(CSOUND *, void *);
int     kread3(CSOUND *, void *), kread4(CSOUND *, void *);
int     krdsset_S(CSOUND *, void *),krdsset_p(CSOUND *, void *),
        kreads(CSOUND *, void *);
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
int     seedrand(CSOUND *, void *), getseed(CSOUND *, void *);
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
int     sum(CSOUND *, void *), product(CSOUND *, void *);
int     macset(CSOUND *, void *);
int     mac(CSOUND *, void *), maca(CSOUND *, void *);
int     nestedapset(CSOUND *, void *), nestedap(CSOUND *, void *);
int     lorenzset(CSOUND *, void *), lorenz(CSOUND *, void *);
int     filelen(CSOUND *, void *), filenchnls(CSOUND *, void *);
int     filesr(CSOUND *, void *), filepeak(CSOUND *, void *);
int     filevalid(CSOUND *, void *);
int     filelen_S(CSOUND *, void *), filenchnls_S(CSOUND *, void *);
int     filesr_S(CSOUND *, void *), filepeak_S(CSOUND *, void *);
int     filevalid_S(CSOUND *, void *);
int     ilogbasetwo(CSOUND *, void *), logbasetwo_set(CSOUND *, void *);
int     powoftwo(CSOUND *, void *), powoftwoa(CSOUND *, void *);
int     logbasetwo(CSOUND *, void *), logbasetwoa(CSOUND *, void *);
int     lp2_set(CSOUND *, void *), lp2(CSOUND *, void *);
int     phaser2set(CSOUND *, void *), phaser2(CSOUND *, void *);
int     phaser1set(CSOUND *, void *), phaser1(CSOUND *, void *);
int     balnset(CSOUND *, void *), balance(CSOUND *, void *);
int     prealloc(CSOUND *, void *);
int     prealloc_S(CSOUND *, void *), active_alloc(CSOUND*, void*);
int     cpsxpch(CSOUND *, void *), cps2pch(CSOUND *, void *);
int     cpstmid(CSOUND *, void *);
int     cpstun(CSOUND *, void *), cpstun_i(CSOUND *, void *);
int     wgpsetin(CSOUND *, void *);
int     wgpset(CSOUND *, void *), wgpluck(CSOUND *, void *);
int     clarinset(CSOUND *, void *), clarin(CSOUND *, void *);
int     fluteset(CSOUND *, void *), flute(CSOUND *, void *);
int     bowedset(CSOUND *, void *), bowed(CSOUND *, void *);
int     brassset(CSOUND *, void *), brass(CSOUND *, void *);
int     schedule(CSOUND *, void *), schedule_S(CSOUND *, void *);
int     schedule_N(CSOUND *, void *), schedule_SN(CSOUND *, void *);
int     ifschedule(CSOUND *, void *), kschedule(CSOUND *, void *);
int     triginset(CSOUND *, void *), ktriginstr(CSOUND *, void *);
int     triginset_S(CSOUND *, void *), ktriginstr_S(CSOUND *, void *);
int     trigseq_set(CSOUND *, void *), trigseq(CSOUND *, void *);
int     eventOpcode(CSOUND *, void *), eventOpcodeI(CSOUND *, void *);
int     eventOpcode_S(CSOUND *, void *), eventOpcodeI_S(CSOUND *, void *);
int     instanceOpcode(CSOUND *, void *), instanceOpcode_S(CSOUND *, void *);
int     kill_instance(CSOUND *csound, void *p);
int     lfoset(CSOUND *, void *);
int     lfok(CSOUND *, void *), lfoa(CSOUND *, void *);
int     mute_inst(CSOUND *, void *);
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
int     tlineto_set(CSOUND *, void *), tlineto(CSOUND *, void *);
int     vibrato_set(CSOUND *, void *), vibrato(CSOUND *, void *);
int     vibr_set(CSOUND *, void *), vibr(CSOUND *, void *);
int     randomi_set(CSOUND *, void *);
int     krandomi(CSOUND *, void *), randomi(CSOUND *, void *);
int     randomh_set(CSOUND *, void *);
int     krandomh(CSOUND *, void *), randomh(CSOUND *, void *);
int     random3_set(CSOUND *, void *);
int     random3(CSOUND *, void *), random3a(CSOUND *, void *);
int     db(CSOUND *, void *), dba(CSOUND *, void *);
int     semitone(CSOUND *, void *), asemitone(CSOUND *, void *);
int     cent(CSOUND *, void *), acent(CSOUND *, void *);
int     midichn(CSOUND *, void *), pgmassign(CSOUND *, void *),
        pgmassign_S(CSOUND *, void *);
int     midiin_set(CSOUND *, void *), midiin(CSOUND *, void *);
int     pgmin_set(CSOUND *, void *), pgmin(CSOUND *, void *);
int     ctlin_set(CSOUND *, void *), ctlin(CSOUND *, void *);
int     midinoteoff(CSOUND *, void *), midinoteonkey(CSOUND *, void *);
int     midinoteoncps(CSOUND *, void *), midinoteonoct(CSOUND *, void *);
int     midinoteonpch(CSOUND *, void *), midipolyaftertouch(CSOUND *, void *);
int     midicontrolchange(CSOUND *, void *);
int     midiprogramchange(CSOUND *, void *);
int     midichannelaftertouch(CSOUND *, void *);
int     midipitchbend(CSOUND *, void *);
int     mididefault(CSOUND *, void *);
int     subinstrset_S(CSOUND *, void *);
int     subinstrset(CSOUND *, void *), subinstr(CSOUND *, void *);
int     useropcdset(CSOUND *, void *), useropcd(CSOUND *, void *);
int     setksmpsset(CSOUND *, void *);
int     xinset(CSOUND *, void *), xoutset(CSOUND *, void *);
int     ingoto(CSOUND *, void *), kngoto(CSOUND *, void *);
int     iingoto(CSOUND *, void *), kingoto(CSOUND *, void *);
int     nstrnumset(CSOUND *, void *), turnoff2k(CSOUND *, void *);
int     nstrnumset_S(CSOUND *, void *);
int     turnoff2S(CSOUND *, void *) ;
int     loop_l_i(CSOUND *, void *), loop_le_i(CSOUND *, void *);
int     loop_g_i(CSOUND *, void *), loop_ge_i(CSOUND *, void *);
int     loop_l_p(CSOUND *, void *), loop_le_p(CSOUND *, void *);
int     loop_g_p(CSOUND *, void *), loop_ge_p(CSOUND *, void *);
int     delete_instr(CSOUND *, void *);
int     insremot(CSOUND *, void *), insglobal(CSOUND *, void *);
int     midremot(CSOUND *, void *), midglobal(CSOUND *, void *);
int     remoteport(CSOUND *, void *);
int     globallock(CSOUND *, void *);
int     globalunlock(CSOUND *, void *);
int     filebit(CSOUND *, void *); int     filebit_S(CSOUND *, void *);
int     iexprndi(CSOUND *, void *), exprndiset(CSOUND *, void *);
int     kexprndi(CSOUND *, void *), aexprndi(CSOUND *, void *);
int     icauchyi(CSOUND *, void *), cauchyiset(CSOUND *, void *);
int     kcauchyi(CSOUND *, void *), acauchyi(CSOUND *, void *);
int     igaussi(CSOUND *, void *), gaussiset(CSOUND *, void *);
int     kgaussi(CSOUND *, void *), agaussi(CSOUND *, void *);
int     lsgset_bkpt(CSOUND *csound, void *p);
int     xsgset_bkpt(CSOUND *csound, void *p);
int     xsgset_bkpt(CSOUND *csound, void *p), xsgset2b(CSOUND *, void *);
int     resize_table(CSOUND *csound, void *p);
int     error_fn(CSOUND *csound, void *p);
int fassign_set(CSOUND *csound, FASSIGN *p);
int tabler_init(CSOUND *csound, TABL *p);
int tabl_setup(CSOUND *csound, TABL *p);
int tabler_kontrol(CSOUND *csound, TABL *p);
int tabler_audio(CSOUND *csound, TABL *p);
int tableir_init(CSOUND *csound, TABL *p);
int tableir_audio(CSOUND *csound, TABL *p);
int tableir_kontrol(CSOUND *csound, TABL *p);
int tableir_audio(CSOUND *csound, TABL *p);
int table3r_init(CSOUND *csound, TABL *p);
int table3r_kontrol(CSOUND *csound, TABL *p);
int table3r_audio(CSOUND *csound, TABL *p);
int tablerkt_kontrol(CSOUND *csound, TABL *p);
int tablerkt_audio(CSOUND *csound, TABL *p);
int tableirkt_kontrol(CSOUND *csound, TABL *p);
  int tableirkt_audio(CSOUND *csound, TABL *p);
int table3rkt_kontrol(CSOUND *csound, TABL *p);
int table3rkt_audio(CSOUND *csound, TABL *p);
int tablew_init(CSOUND *csound, TABL *p);
int tablew_kontrol(CSOUND *csound, TABL *p);
int tablew_audio(CSOUND *csound, TABL *p);
int tablewkt_kontrol(CSOUND *csound, TABL *p);
int tablewkt_audio(CSOUND *csound, TABL *p);
int table_length(CSOUND *csound, TLEN *p);
int table_gpw(CSOUND *csound, TGP *p);
int table_copy(CSOUND *csound, TGP *p);
int table_mix(CSOUND *csound, TABLMIX *p);
int table_ra_set(CSOUND *csound, TABLRA *p);
int table_ra(CSOUND *csound, TABLRA *p);
int table_wa_set(CSOUND *csound, TABLWA *p);
int table_wa(CSOUND *csound, TABLWA *p);
int tablkt_setup(CSOUND *csound, TABL *p);
int diskin_init(CSOUND *csound, DISKIN2 *p);
int diskin_init_S(CSOUND *csound, DISKIN2 *p);
int inch1_set(CSOUND *csound, void *p);
int inch_opcode1(CSOUND *csound, void *p);
int adset_S(CSOUND *csound, void *p);
int lprdset_S(CSOUND *csound, void *p);
int pvsfreadset_S(CSOUND *csound, void *p);
int alnnset(CSOUND *csound, void *p);
int alnrset(CSOUND *csound, void *p);
int aevxset(CSOUND *csound, void *p);
int aevrset(CSOUND *csound, void *p);
