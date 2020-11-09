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
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
    02110-1301 USA
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
#include "lpred.h"

#define S(x)    sizeof(x)

int32_t ihold(CSOUND *, void *), turnoff(CSOUND *, void *);
int32_t gaassign(CSOUND *, void *), rassign(CSOUND *, void *);
int32_t aassign(CSOUND *, void *), laassign(CSOUND *, void *);
int32_t assign(CSOUND *, void *);
int32_t init(CSOUND *, void *), ainit(CSOUND *, void *);
int32_t minit(CSOUND *, void *), mainit(CSOUND *, void *);
/* int32_t tinit(CSOUND *, void *), tassign(CSOUND *, void *); */
/* int32_t tabref_check(CSOUND *, void *), tabref(CSOUND *, void *); */
int32_t gt(CSOUND *, void *), ge(CSOUND *, void *);
int32_t lt(CSOUND *, void *), le(CSOUND *, void *);
int32_t eq(CSOUND *, void *), ne(CSOUND *, void *);
int32_t and(CSOUND *, void *), or(CSOUND *, void *);
int32_t b_not(CSOUND *, void*);
int32_t conval(CSOUND *, void *), aconval(CSOUND *, void *);
int32_t addkk(CSOUND *, void *), subkk(CSOUND *, void *);
int32_t mulkk(CSOUND *, void *), divkk(CSOUND *, void *);
int32_t modkk(CSOUND *, void *);
int32_t addka(CSOUND *, void *), subka(CSOUND *, void *);
int32_t mulka(CSOUND *, void *), divka(CSOUND *, void *);
int32_t modka(CSOUND *, void *);
int32_t addak(CSOUND *, void *), subak(CSOUND *, void *);
int32_t mulak(CSOUND *, void *), divak(CSOUND *, void *);
int32_t modak(CSOUND *, void *);
int32_t addaa(CSOUND *, void *), subaa(CSOUND *, void *);
int32_t mulaa(CSOUND *, void *), divaa(CSOUND *, void *);
int32_t modaa(CSOUND *, void *);
int32_t addin(CSOUND *, void *), addina(CSOUND *, void *);
int32_t subin(CSOUND *, void *), subina(CSOUND *, void *);
int32_t addinak(CSOUND *, void *), subinak(CSOUND *, void *);
int32_t divzkk(CSOUND *, void *), divzka(CSOUND *, void *);
int32_t divzak(CSOUND *, void *), divzaa(CSOUND *, void *);
int32_t int1(CSOUND *, void *), int1a(CSOUND *, void *);
int32_t frac1(CSOUND *, void *), frac1a(CSOUND *, void *);
int32_t int1_round(CSOUND *, void *), int1a_round(CSOUND *, void *);
int32_t int1_floor(CSOUND *, void *), int1a_floor(CSOUND *, void *);
int32_t int1_ceil(CSOUND *, void *), int1a_ceil(CSOUND *, void *);
int32_t rnd1(CSOUND *, void *), birnd1(CSOUND *, void *);
int32_t rnd1seed(CSOUND *, void *);
int32_t abs1(CSOUND *, void *), exp01(CSOUND *, void *);
int32_t log01(CSOUND *, void *), sqrt1(CSOUND *, void *);
int32_t sin1(CSOUND *, void *), cos1(CSOUND *, void *);
int32_t is_NaN(CSOUND *, void *), is_NaNa(CSOUND *, void *);
int32_t is_inf(CSOUND *, void *), is_infa(CSOUND *, void *);
int32_t tan1(CSOUND *, void *), asin1(CSOUND *, void *);
int32_t acos1(CSOUND *, void *), atan1(CSOUND *, void *);
int32_t sinh1(CSOUND *, void *), cosh1(CSOUND *, void *);
int32_t tanh1(CSOUND *, void *), log101(CSOUND *, void *), log21(CSOUND *, void *);
int32_t atan21(CSOUND *, void *), atan2aa(CSOUND *, void *);
int32_t absa(CSOUND *, void *), expa(CSOUND *, void *);
int32_t loga(CSOUND *, void *), sqrta(CSOUND *, void *);
int32_t sina(CSOUND *, void *), cosa(CSOUND *, void *);
int32_t tana(CSOUND *, void *), asina(CSOUND *, void *);
int32_t acosa(CSOUND *, void *), atana(CSOUND *, void *);
int32_t sinha(CSOUND *, void *), cosha(CSOUND *, void *);
int32_t tanha(CSOUND *, void *), log10a(CSOUND *, void *), log2a(CSOUND *, void *);
int32_t dbamp(CSOUND *, void *), ampdb(CSOUND *, void *);
int32_t aampdb(CSOUND *, void *), dbfsamp(CSOUND *, void *);
int32_t ampdbfs(CSOUND *, void *), aampdbfs(CSOUND *, void *);
int32_t ftlen(CSOUND *, void *), ftlptim(CSOUND *, void *);
int32_t ftchnls(CSOUND *, void *), ftcps(CSOUND *, void *);
int32_t signum(CSOUND *, void *), asignum(CSOUND *, void *);
int32_t rtclock(CSOUND *, void *);
int32_t cpsoct(CSOUND *, void *), octpch(CSOUND *, void *);
int32_t cpspch(CSOUND *, void *), pchoct(CSOUND *, void *);
int32_t octcps(CSOUND *, void *), acpsoct(CSOUND *, void *);
int32_t cpsmidinn(CSOUND *, void *), octmidinn(CSOUND *, void *);
int32_t pchmidinn(CSOUND *, void *);int32_t massign_S(CSOUND *, void *);
int32_t massign_p(CSOUND *, void *), ctrlinit(CSOUND *, void *);
int32_t ctrlnameinit(CSOUND *, void *);
int32_t notnum(CSOUND *, void *), veloc(CSOUND *, void *);
int32_t pchmidi(CSOUND *, void *), pchmidib(CSOUND *, void *);
int32_t octmidi(CSOUND *, void *), octmidib(CSOUND *, void *);
int32_t pchmidib_i(CSOUND *, void *), octmidib_i(CSOUND *, void *);
int32_t icpsmidib_i(CSOUND *, void *), cpsmidi(CSOUND *, void *);
int32_t icpsmidib(CSOUND *, void *), kcpsmidib(CSOUND *, void *);
int32_t kmbset(CSOUND *, void *), midibset(CSOUND *, void *);
int32_t ipchmidib(CSOUND *, void *), ioctmidib(CSOUND *, void *);
int32_t kpchmidib(CSOUND *, void *), koctmidib(CSOUND *, void *);
int32_t msclset(CSOUND *, void *), ampmidi(CSOUND *, void *);
int32_t aftset(CSOUND *, void *), aftouch(CSOUND *, void *);
int32_t chpress(CSOUND *, void *), ipchbend(CSOUND *, void *);
int32_t kbndset(CSOUND *, void *), kpchbend(CSOUND *, void *);
int32_t imidictl(CSOUND *, void *), mctlset(CSOUND *, void *);
int32_t midictl(CSOUND *, void *), imidiaft(CSOUND *, void *);
int32_t maftset(CSOUND *, void *), midiaft(CSOUND *, void *);
int32_t midiout(CSOUND *, void *), turnon(CSOUND *, void *);
int32_t turnon_S(CSOUND *, void *);
int32_t kmapset(CSOUND *, void *), polyaft(CSOUND *, void *);
int32_t ichanctl(CSOUND *, void *), chctlset(CSOUND *, void *);
int32_t chanctl(CSOUND *, void *), linset(CSOUND *, void *);
int32_t kline(CSOUND *, void *), aline(CSOUND *, void *);
int32_t expset(CSOUND *, void *), kexpon(CSOUND *, void *);
int32_t expon(CSOUND *, void *), lsgset(CSOUND *, void *);
int32_t klnseg(CSOUND *, void *), linseg(CSOUND *, void *);
int32_t csgset(CSOUND *, void *), kosseg(CSOUND *, void *);
int32_t csgset_bkpt(CSOUND *, void *), cosseg(CSOUND *, void *);
int32_t csgrset(CSOUND *, void *);
int32_t kcssegr(CSOUND *, void *), cossegr(CSOUND *, void *);
int32_t madsrset(CSOUND *, void *), adsrset(CSOUND *, void *);
int32_t xdsrset(CSOUND *, void *), mxdsrset(CSOUND *, void *);
int32_t expseg2(CSOUND *, void *), xsgset(CSOUND *, void *);
int32_t kxpseg(CSOUND *, void *), expseg(CSOUND *, void *);
int32_t xsgset2(CSOUND *, void *), lsgrset(CSOUND *, void *);
int32_t klnsegr(CSOUND *, void *), linsegr(CSOUND *, void *);
int32_t xsgrset(CSOUND *, void *), kxpsegr(CSOUND *, void *);
int32_t expsegr(CSOUND *, void *), lnnset(CSOUND *, void *);
int32_t klinen(CSOUND *, void *), linen(CSOUND *, void *);
int32_t lnrset(CSOUND *, void *), klinenr(CSOUND *, void *);
int32_t linenr(CSOUND *, void *), evxset(CSOUND *, void *);
int32_t knvlpx(CSOUND *, void *), envlpx(CSOUND *, void *);
int32_t evrset(CSOUND *, void *), knvlpxr(CSOUND *, void *);
int32_t envlpxr(CSOUND *, void *), phsset(CSOUND *, void *);
int32_t ephsset(CSOUND *, void *), ephsor(CSOUND *, void *);
int32_t kphsor(CSOUND *, void *), phsor(CSOUND *, void *);
int32_t ko1set(CSOUND *, void *);
int32_t kosc1(CSOUND *, void *), kosc1i(CSOUND *, void *);
int32_t oscnset(CSOUND *, void *), osciln(CSOUND *, void *);
int32_t oscset(CSOUND *, void *), koscil(CSOUND *, void *);
int32_t oscsetA(CSOUND *, void *);
int32_t osckk(CSOUND *, void *), oscka(CSOUND *, void *);
int32_t oscak(CSOUND *, void *), oscaa(CSOUND *, void *);
int32_t koscli(CSOUND *, void *), osckki(CSOUND *, void *);
int32_t osckai(CSOUND *, void *), oscaki(CSOUND *, void *);
int32_t oscaai(CSOUND *, void *), foscset(CSOUND *, void *);
int32_t foscil(CSOUND *, void *), foscili(CSOUND *, void *);
int32_t losset(CSOUND *, void *), loscil(CSOUND *, void *);
int32_t loscil3(CSOUND *, void *), koscl3(CSOUND *, void *);
int32_t osckk3(CSOUND *, void *), oscka3(CSOUND *, void *);
int32_t oscak3(CSOUND *, void *), oscaa3(CSOUND *, void *);
int32_t adset(CSOUND *, void *), adsyn(CSOUND *, void *);
int32_t bzzset(CSOUND *, void *), buzz(CSOUND *, void *);
int32_t gbzset(CSOUND *, void *), gbuzz(CSOUND *, void *);
int32_t plukset(CSOUND *, void *), pluck(CSOUND *, void *);
int32_t rndset(CSOUND *, void *), krand(CSOUND *, void *);
int32_t arand(CSOUND *, void *), rhset(CSOUND *, void *);
int32_t krandh(CSOUND *, void *), randh(CSOUND *, void *);
int32_t riset(CSOUND *, void *), krandi(CSOUND *, void *);
int32_t rcset(CSOUND *, void *), randc(CSOUND *, void *);
int32_t krandc(CSOUND *, void *);
int32_t randi(CSOUND *, void *), rndset2(CSOUND *, void *);
int32_t krand2(CSOUND *, void *), arand2(CSOUND *, void *);
int32_t rhset2(CSOUND *, void *), krandh2(CSOUND *, void *);
int32_t randh2(CSOUND *, void *), riset2(CSOUND *, void *);
int32_t krandi2(CSOUND *, void *), randi2(CSOUND *, void *);
int32_t porset(CSOUND *, void *), port(CSOUND *, void *);
int32_t tonset(CSOUND *, void *), tone(CSOUND *, void *);
int32_t atone(CSOUND *, void *), rsnset(CSOUND *, void *);
int32_t reson(CSOUND *, void *), areson(CSOUND *, void *);
int32_t resonx(CSOUND *, void *), aresonx(CSOUND *, void *);
int32_t rsnsetx(CSOUND *, void *), tonex(CSOUND *, void *);
int32_t atonex(CSOUND *, void *), tonsetx(CSOUND *, void *);
int32_t lprdset(CSOUND *, void *), lpread(CSOUND *, void *);
int32_t lpformantset(CSOUND *, void *), lpformant(CSOUND *, void*);
int32_t lprsnset(CSOUND *, void *), lpreson(CSOUND *, void *);
int32_t lpfrsnset(CSOUND *, void *), lpfreson(CSOUND *, void *);
int32_t lpslotset(CSOUND *, void *), lpitpset(CSOUND *, void *);
int32_t lpinterpol(CSOUND *, void *);
int32_t rmsset(CSOUND *, void *), rms(CSOUND *, void *);
int32_t gainset(CSOUND *, void *), gain(CSOUND *, void *);
int32_t sndinset(CSOUND *, void *), sndinset_S(CSOUND *, void *),
        soundin(CSOUND *, void *);
int32_t sndoutset(CSOUND *, void *), sndoutset_S(CSOUND *, void *),
        soundout(CSOUND *, void *);
int32_t soundouts(CSOUND *, void *), inarray(CSOUND *, void *);
int32_t in(CSOUND *, void *), ins(CSOUND *, void *);
int32_t inq(CSOUND *, void *), inh(CSOUND *, void *);
int32_t ino(CSOUND *, void *), in16(CSOUND *, void *);
int32_t in32(CSOUND *, void *), outarr_init(CSOUND *, void *);
int32_t outarr(CSOUND *, void *), outrep(CSOUND*, void*);
int32_t inch_opcode(CSOUND *, void *), inall_opcode(CSOUND *, void *);
int32_t inch_set(CSOUND*, void*), outmultiple(CSOUND *, void *);
/* int32_t out(CSOUND *, void *), outs(CSOUND *, void *); */
int32_t outs1(CSOUND *, void *), outs2(CSOUND *, void *);
int32_t och2(CSOUND *, void *), och3(CSOUND *, void *);
int32_t och4(CSOUND *, void *), ochn(CSOUND *, void *);
int32_t outq1(CSOUND *, void *), outq2(CSOUND *, void *);
int32_t outq3(CSOUND *, void *), outq4(CSOUND *, void *);
/* int32_t outh(CSOUND *, void *), outo(CSOUND *, void *); */
/* int32_t outx(CSOUND *, void *), outX(CSOUND *, void *); */
int32_t outch(CSOUND *, void *), outall(CSOUND *, void *);
int32_t igoto(CSOUND *, void *), kgoto(CSOUND *, void *);
int32_t icgoto(CSOUND *, void *), kcgoto(CSOUND *, void *);
int32_t timset(CSOUND *, void *), timout(CSOUND *, void *);
int32_t reinit(CSOUND *, void *), rigoto(CSOUND *, void *);
int32_t rireturn(CSOUND *, void *), tigoto(CSOUND *, void *);
int32_t tival(CSOUND *, void *), printv(CSOUND *, void *);
int32_t dspset(CSOUND *, void *), kdsplay(CSOUND *, void *);
int32_t fdspset(CSOUND *, void *), fdsplay(CSOUND *, void *);
int32_t dsplay(CSOUND *, void *), fftset(CSOUND *, void *);
int32_t kdspfft(CSOUND *, void *), dspfft(CSOUND *, void *);
int32_t xyinset(CSOUND *, void *);
int32_t tempeset(CSOUND *, void *), tempest(CSOUND *, void *);
int32_t tempset(CSOUND *, void *), tempo(CSOUND *, void *);
int32_t old_kdmpset(CSOUND *, void *), old_kdmp2set(CSOUND *, void *);
int32_t old_kdmp3set(CSOUND *, void *), old_kdmp4set(CSOUND *, void *);
int32_t kdmpset_p(CSOUND *, void *), kdmp2set_p(CSOUND *, void *);
int32_t kdmp3set_p(CSOUND *, void *), kdmp4set_p(CSOUND *, void *);
int32_t kdmpset_S(CSOUND *, void *), kdmp2set_S(CSOUND *, void *);
int32_t kdmp3set_S(CSOUND *, void *), kdmp4set_S(CSOUND *, void *);
int32_t kdump(CSOUND *, void *), kdump2(CSOUND *, void *);
int32_t kdump3(CSOUND *, void *), kdump4(CSOUND *, void *);
int32_t krdset_S(CSOUND *, void *), krd2set_S(CSOUND *, void *);
int32_t krd3set_S(CSOUND *, void *), krd4set_S(CSOUND *, void *);
int32_t krdset_p(CSOUND *, void *), krd2set_p(CSOUND *, void *);
int32_t krd3set_p(CSOUND *, void *), krd4set_p(CSOUND *, void *);
int32_t kread(CSOUND *, void *), kread2(CSOUND *, void *);
int32_t kread3(CSOUND *, void *), kread4(CSOUND *, void *);
int32_t krdsset_S(CSOUND *, void *),krdsset_p(CSOUND *, void *),
        kreads(CSOUND *, void *);
int32_t ipow(CSOUND *, void *), apow(CSOUND *, void *);
int32_t alinear(CSOUND *, void *), iklinear(CSOUND *, void *);
int32_t atrian(CSOUND *, void *), iktrian(CSOUND *, void *);
int32_t aexp(CSOUND *, void *), ikexp(CSOUND *, void *);
int32_t abiexp(CSOUND *, void *), ikbiexp(CSOUND *, void *);
int32_t agaus(CSOUND *, void *), ikgaus(CSOUND *, void *);
int32_t acauchy(CSOUND *, void *), ikcauchy(CSOUND *, void *);
int32_t apcauchy(CSOUND *, void *), ikpcauchy(CSOUND *, void *);
int32_t abeta(CSOUND *, void *), ikbeta(CSOUND *, void *);
int32_t aweib(CSOUND *, void *), ikweib(CSOUND *, void *);
int32_t apoiss(CSOUND *, void *), ikpoiss(CSOUND *, void *);
int32_t seedrand(CSOUND *, void *), getseed(CSOUND *, void *);
int32_t auniform(CSOUND *, void *), ikuniform(CSOUND *, void *);
int32_t newsndinset(CSOUND *, void *), soundinew(CSOUND *, void *);
int32_t iout_on(CSOUND *, void *), iout_off(CSOUND *, void *);
int32_t out_controller(CSOUND *, void *), iout_on_dur_set(CSOUND *, void *);
int32_t iout_on_dur(CSOUND *, void *), iout_on_dur2(CSOUND *, void *);
int32_t moscil_set(CSOUND *, void *), moscil(CSOUND *, void *);
int32_t kvar_out_on_set(CSOUND *, void *), kvar_out_on_set1(CSOUND *, void *);
int32_t kvar_out_on(CSOUND *, void *), out_controller14(CSOUND *, void *);
int32_t out_pitch_bend(CSOUND *, void *), out_aftertouch(CSOUND *, void *);
int32_t out_poly_aftertouch(CSOUND*, void*), out_progchange(CSOUND*, void*);
int32_t release_set(CSOUND *, void *), release(CSOUND *, void *);
int32_t xtratim(CSOUND *, void *);
int32_t mclock_set(CSOUND *, void *), mclock(CSOUND *, void *);
int32_t mrtmsg(CSOUND *, void *);
int32_t cabasaset(CSOUND *, void *), cabasa(CSOUND *, void *);
int32_t sekereset(CSOUND *, void *), sandset(CSOUND *, void *);
int32_t stixset(CSOUND *, void *), crunchset(CSOUND *, void *);
int32_t guiroset(CSOUND *, void *), guiro(CSOUND *, void *);
int32_t sekere(CSOUND *, void *);
int32_t tambourset(CSOUND *, void *), tambourine(CSOUND *, void *);
int32_t bambooset(CSOUND *, void *), bamboo(CSOUND *, void *);
int32_t wuterset(CSOUND *, void *), wuter(CSOUND *, void *);
int32_t sleighset(CSOUND *, void *), sleighbells(CSOUND *, void *);
int32_t trig_set(CSOUND *, void *), trig(CSOUND *, void *);
int32_t numsamp(CSOUND *, void *), ftsr(CSOUND *, void *);
int32_t kon2_set(CSOUND *, void *), kon2(CSOUND *, void *);
int32_t nrpn(CSOUND *, void *);
int32_t mdelay(CSOUND *, void *), mdelay_set(CSOUND *, void *);
int32_t sum(CSOUND *, void *), product(CSOUND *, void *);
int32_t macset(CSOUND *, void *);
int32_t mac(CSOUND *, void *), maca(CSOUND *, void *);
int32_t nestedapset(CSOUND *, void *), nestedap(CSOUND *, void *);
int32_t lorenzset(CSOUND *, void *), lorenz(CSOUND *, void *);
int32_t filelen(CSOUND *, void *), filenchnls(CSOUND *, void *);
int32_t filesr(CSOUND *, void *), filepeak(CSOUND *, void *);
int32_t filevalid(CSOUND *, void *);
int32_t filelen_S(CSOUND *, void *), filenchnls_S(CSOUND *, void *);
int32_t filesr_S(CSOUND *, void *), filepeak_S(CSOUND *, void *);
int32_t filevalid_S(CSOUND *, void *);
int32_t ilogbasetwo(CSOUND *, void *), logbasetwo_set(CSOUND *, void *);
int32_t powoftwo(CSOUND *, void *), powoftwoa(CSOUND *, void *);
int32_t logbasetwo(CSOUND *, void *), logbasetwoa(CSOUND *, void *);
int32_t lp2_set(CSOUND *, void *), lp2(CSOUND *, void *);
int32_t phaser2set(CSOUND *, void *), phaser2(CSOUND *, void *);
int32_t phaser1set(CSOUND *, void *), phaser1(CSOUND *, void *);
int32_t balnset(CSOUND *, void *), balance(CSOUND *, void *);
int32_t prealloc(CSOUND *, void *);
int32_t prealloc_S(CSOUND *, void *), active_alloc(CSOUND*, void*);
int32_t cpsxpch(CSOUND *, void *), cps2pch(CSOUND *, void *);
int32_t cpstmid(CSOUND *, void *);
int32_t cpstun(CSOUND *, void *), cpstun_i(CSOUND *, void *);
int32_t wgpsetin(CSOUND *, void *);
int32_t wgpset(CSOUND *, void *), wgpluck(CSOUND *, void *);
int32_t clarinset(CSOUND *, void *), clarin(CSOUND *, void *);
int32_t fluteset(CSOUND *, void *), flute(CSOUND *, void *);
int32_t bowedset(CSOUND *, void *), bowed(CSOUND *, void *);
int32_t brassset(CSOUND *, void *), brass(CSOUND *, void *);
int32_t schedule(CSOUND *, void *), schedule_S(CSOUND *, void *);
int32_t schedule_N(CSOUND *, void *), schedule_SN(CSOUND *, void *);
int32_t ifschedule(CSOUND *, void *), kschedule(CSOUND *, void *);
int32_t triginset(CSOUND *, void *), ktriginstr(CSOUND *, void *);
int32_t triginset_S(CSOUND *, void *), ktriginstr_S(CSOUND *, void *);
int32_t trigseq_set(CSOUND *, void *), trigseq(CSOUND *, void *);
int32_t eventOpcode(CSOUND *, void *), eventOpcodeI(CSOUND *, void *);
int32_t eventOpcode_S(CSOUND *, void *), eventOpcodeI_S(CSOUND *, void *);
int32_t instanceOpcode(CSOUND *, void *), instanceOpcode_S(CSOUND *, void *);
int32_t kill_instance(CSOUND *csound, void *p);
int32_t lfoset(CSOUND *, void *);
int32_t lfok(CSOUND *, void *), lfoa(CSOUND *, void *);
int32_t mute_inst(CSOUND *, void *);
int32_t pvsanalset(CSOUND *, void *), pvsanal(CSOUND *, void *);
int32_t pvsynthset(CSOUND *, void *), pvsynth(CSOUND *, void *);
int32_t pvadsynset(CSOUND *, void *), pvadsyn(CSOUND *, void *);
int32_t pvscrosset(CSOUND *, void *), pvscross(CSOUND *, void *);
int32_t pvsfreadset(CSOUND *, void *), pvsfread(CSOUND *, void *);
int32_t pvsmaskaset(CSOUND *, void *), pvsmaska(CSOUND *, void *);
int32_t pvsftwset(CSOUND *, void *), pvsftw(CSOUND *, void *);
int32_t pvsftrset(CSOUND *, void *), pvsftr(CSOUND *, void *);
int32_t pvsinfo(CSOUND *, void *);
int32_t gettempo(CSOUND *, void *), fassign(CSOUND *, void *);
int32_t loopseg_set(CSOUND *, void *);
int32_t loopseg(CSOUND *, void *), lpshold(CSOUND *, void *);
int32_t lineto_set(CSOUND *, void *), lineto(CSOUND *, void *);
int32_t tlineto_set(CSOUND *, void *), tlineto(CSOUND *, void *);
int32_t vibrato_set(CSOUND *, void *), vibrato(CSOUND *, void *);
int32_t vibr_set(CSOUND *, void *), vibr(CSOUND *, void *);
int32_t randomi_set(CSOUND *, void *);
int32_t krandomi(CSOUND *, void *), randomi(CSOUND *, void *);
int32_t randomh_set(CSOUND *, void *);
int32_t krandomh(CSOUND *, void *), randomh(CSOUND *, void *);
int32_t random3_set(CSOUND *, void *);
int32_t random3(CSOUND *, void *), random3a(CSOUND *, void *);
int32_t db(CSOUND *, void *), dba(CSOUND *, void *);
int32_t semitone(CSOUND *, void *), asemitone(CSOUND *, void *);
int32_t cent(CSOUND *, void *), acent(CSOUND *, void *);
int32_t midichn(CSOUND *, void *), pgmassign(CSOUND *, void *),
        pgmassign_S(CSOUND *, void *);
int32_t midiin_set(CSOUND *, void *), midiin(CSOUND *, void *);
int32_t pgmin_set(CSOUND *, void *), pgmin(CSOUND *, void *);
int32_t ctlin_set(CSOUND *, void *), ctlin(CSOUND *, void *);
int32_t midinoteoff(CSOUND *, void *), midinoteonkey(CSOUND *, void *);
int32_t midinoteoncps(CSOUND *, void *), midinoteonoct(CSOUND *, void *);
int32_t midinoteonpch(CSOUND *, void *), midipolyaftertouch(CSOUND *, void *);
int32_t midicontrolchange(CSOUND *, void *);
int32_t midiprogramchange(CSOUND *, void *);
int32_t midichannelaftertouch(CSOUND *, void *);
int32_t midipitchbend(CSOUND *, void *);
int32_t mididefault(CSOUND *, void *);
int32_t subinstrset_S(CSOUND *, void *);
int32_t subinstrset(CSOUND *, void *), subinstr(CSOUND *, void *);
int32_t useropcdset(CSOUND *, void *), useropcd(CSOUND *, void *);
int32_t setksmpsset(CSOUND *, void *);
int32_t xinset(CSOUND *, void *), xoutset(CSOUND *, void *);
int32_t ingoto(CSOUND *, void *), kngoto(CSOUND *, void *);
int32_t nstrnumset(CSOUND *, void *), turnoff2k(CSOUND *, void *);
int32_t nstrnumset_S(CSOUND *, void *), nstrstr(CSOUND *, void *);
int32_t turnoff2S(CSOUND *, void *) ;
int32_t turnoff3S(CSOUND *, void *), turnoff3k(CSOUND *, void *);
int32_t savectrl_init(CSOUND*, void*), savectrl_perf(CSOUND*, void*);
int32_t printctrl(CSOUND*, void*);
int32_t printctrl_init(CSOUND*, void*), printctrl_init1(CSOUND*, void*);
int32_t presetctrl_init(CSOUND*, void*), presetctrl_perf(CSOUND*, void*);
int32_t presetctrl1_init(CSOUND*, void*), presetctrl1_perf(CSOUND*, void*);
int32_t selectctrl_init(CSOUND*, void*), selectctrl_perf(CSOUND*, void*);
int32_t printpresets_init(CSOUND*, void*), printpresets_init1(CSOUND*, void*);
int32_t printpresets_perf(CSOUND*, void*);
int32_t loop_l_i(CSOUND *, void *), loop_le_i(CSOUND *, void *);
int32_t loop_g_i(CSOUND *, void *), loop_ge_i(CSOUND *, void *);
int32_t loop_l_p(CSOUND *, void *), loop_le_p(CSOUND *, void *);
int32_t loop_g_p(CSOUND *, void *), loop_ge_p(CSOUND *, void *);
int32_t delete_instr(CSOUND *, void *);
int32_t insremot(CSOUND *, void *), insglobal(CSOUND *, void *);
int32_t midremot(CSOUND *, void *), midglobal(CSOUND *, void *);
int32_t remoteport(CSOUND *, void *);
int32_t globallock(CSOUND *, void *);
int32_t globalunlock(CSOUND *, void *);
int32_t filebit(CSOUND *, void *); int32_t filebit_S(CSOUND *, void *);
int32_t iexprndi(CSOUND *, void *), exprndiset(CSOUND *, void *);
int32_t kexprndi(CSOUND *, void *), aexprndi(CSOUND *, void *);
int32_t icauchyi(CSOUND *, void *), cauchyiset(CSOUND *, void *);
int32_t kcauchyi(CSOUND *, void *), acauchyi(CSOUND *, void *);
int32_t igaussi(CSOUND *, void *), gaussiset(CSOUND *, void *);
int32_t kgaussi(CSOUND *, void *), agaussi(CSOUND *, void *);
int32_t lsgset_bkpt(CSOUND *csound, void *p);
int32_t xsgset_bkpt(CSOUND *csound, void *p);
int32_t xsgset_bkpt(CSOUND *csound, void *p), xsgset2b(CSOUND *, void *);
int32_t resize_table(CSOUND *csound, void *p);
int32_t error_fn(CSOUND *csound, void *p);
int32_t fassign_set(CSOUND *csound, FASSIGN *p);
int32_t tabler_init(CSOUND *csound, TABL *p);
int32_t tabl_setup(CSOUND *csound, TABL *p);
int32_t tabler_kontrol(CSOUND *csound, TABL *p);
int32_t tabler_audio(CSOUND *csound, TABL *p);
int32_t tableir_init(CSOUND *csound, TABL *p);
int32_t tableir_audio(CSOUND *csound, TABL *p);
int32_t tableir_kontrol(CSOUND *csound, TABL *p);
int32_t tableir_audio(CSOUND *csound, TABL *p);
int32_t table3r_init(CSOUND *csound, TABL *p);
int32_t table3r_kontrol(CSOUND *csound, TABL *p);
int32_t table3r_audio(CSOUND *csound, TABL *p);
int32_t tablerkt_kontrol(CSOUND *csound, TABL *p);
int32_t tablerkt_audio(CSOUND *csound, TABL *p);
int32_t tableirkt_kontrol(CSOUND *csound, TABL *p);
  int32_t tableirkt_audio(CSOUND *csound, TABL *p);
int32_t table3rkt_kontrol(CSOUND *csound, TABL *p);
int32_t table3rkt_audio(CSOUND *csound, TABL *p);
int32_t tablew_init(CSOUND *csound, TABL *p);
int32_t tablew_kontrol(CSOUND *csound, TABL *p);
int32_t tablew_audio(CSOUND *csound, TABL *p);
int32_t tablewkt_kontrol(CSOUND *csound, TABL *p);
int32_t tablewkt_audio(CSOUND *csound, TABL *p);
int32_t table_length(CSOUND *csound, TLEN *p);
int32_t table_gpw(CSOUND *csound, TGP *p);
int32_t table_copy(CSOUND *csound, TGP *p);
int32_t table_mix(CSOUND *csound, TABLMIX *p);
int32_t table_ra_set(CSOUND *csound, TABLRA *p);
int32_t table_ra(CSOUND *csound, TABLRA *p);
int32_t table_wa_set(CSOUND *csound, TABLWA *p);
int32_t table_wa(CSOUND *csound, TABLWA *p);
int32_t tablkt_setup(CSOUND *csound, TABL *p);
int32_t diskin_init(CSOUND *csound, DISKIN2 *p);
int32_t diskin_init_S(CSOUND *csound, DISKIN2 *p);
int32_t inch1_set(CSOUND *csound, void *p);
int32_t inch_opcode1(CSOUND *csound, void *p);
int32_t adset_S(CSOUND *csound, void *p);
int32_t lprdset_S(CSOUND *csound, void *p);
int32_t pvsfreadset_S(CSOUND *csound, void *p);
int32_t alnnset(CSOUND *csound, void *p);
int32_t alnrset(CSOUND *csound, void *p);
int32_t aevxset(CSOUND *csound, void *p);
int32_t aevrset(CSOUND *csound, void *p);
int32_t midiarp_set(CSOUND *, void *);
int32_t midiarp(CSOUND *, void *);
int32_t losset_phs(CSOUND *, void *);
int32_t loscil_phs(CSOUND *, void *);
int32_t loscil3_phs(CSOUND *, void *);
int32_t balance2(CSOUND *, void *);
int32_t gauss_scalar(CSOUND *csound, void *p);
int32_t gauss_vector(CSOUND *csound, void *p);
int32_t lpfil_init(CSOUND *csound, void *p);
int32_t lpfil_perf(CSOUND *csound, void *p);
int32_t lpfil2_init(CSOUND *csound, void *p);
int32_t lpfil2_perf(CSOUND *csound, void *p);
int32_t lpred_run(CSOUND *csound, void *p);
int32_t lpred_alloc(CSOUND *csound, void *p);
int32_t lpred_i(CSOUND *csound, void *p);
int32_t lpfil3_init(CSOUND *csound, void *p);
int32_t lpfil3_perf(CSOUND *csound, void *p);
int32_t lpred_run2(CSOUND *csound, void *p);
int32_t lpred_alloc2(CSOUND *csound, void *p);
int32_t lpcpvs(CSOUND *csound, void *p);
int32_t lpcpvs_init(CSOUND *csound, void *p);
int32_t pvscoefs_init(CSOUND *csound, void *p);
int32_t pvscoefs(CSOUND *csound, void *p);
int32_t coef2parm_init(CSOUND *csound, void *p);
int32_t coef2parm(CSOUND *csound, void *p);
int32_t resonbnk_init(CSOUND *csound, void *p);
int32_t resonbnk(CSOUND *csound, void *p);
int32_t schedule_array(CSOUND *csound, void *p);

