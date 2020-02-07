<CsoundSynthesizer>
<CsLicense>
Copyright (C) 2013 by Michael Gogins.
All rights reserved.
</CsLicense>
<CsOptions>
-odac -m3 -d
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 100
nchnls = 2
0dbfs = 5000000

;#define FLTK #fltk#
;#define CSOUNDQT0 #csoundqt#

gkslider1 init 0
gkslider2 init 0
gkslider3 init 0
gkslider4 init 0
gkslider5 init 0
gkbutt1 init 0
gkbutt2 init 0
gkbutt3 init 0
gkbutt4 init 0
gkbutt5 init 0
gktrackpadx init 0
gktrackpady init 0
gkaccelerometerx init 0
gkaccelerometery init 0
gkaccelerometerz init 0

#ifdef FLTK
FLpanel "Csound 6", 510, 580, 150, 150
gkslider1, ihslider1 FLslider "slider1", 0, 1, 0, 23, -1, 500, 20, 5, 5
FLsetVal_i 1, ihslider1
gkslider2, ihslider2 FLslider "slider2", 0, 1, 0, 23, -1, 500, 20, 5, 55
FLsetVal_i 0.5, ihslider2
gkslider3, ihslider3 FLslider "slider3", 0, 1, 0, 23, -1, 500, 20, 5, 105
FLsetVal_i (1-0.5)/(8-0.5), ihslider3
gkslider4, ihslider4 FLslider "slider4", 0, 1, 0, 23, -1, 500, 20, 5, 155
FLsetVal_i 0, ihslider4
gkslider5, ihslider5 FLslider "slider5", 0, 1, 0, 23, -1, 500, 20, 5, 205
FLsetVal_i 0, ihslider5
gktrackpadx, gktrackpady, ihtrackpadx, ihtrackpady FLjoy "trackpad.x/trackpad.y", 0, 1, 0, 1, 0, 0, -1, -1, 500, 250, 5, 250
FLsetVal_i 0.5, ihtrackpadx
FLsetVal_i (0.75-0.5)/(1-0.5), ihtrackpady
gkbutt1, ihbutt1 FLbutton "butt1", 1, 0, 21, 100, 40, 5, 530 , -1
gkbutt2, ihbutt2 FLbutton "butt2", 1, 0, 21, 100, 40, 105, 530 , -1
gkbutt3, ihbutt3 FLbutton "butt3", 1, 0, 21, 100, 40, 205, 530 , -1
gkbutt4, ihbutt4 FLbutton "butt4", 1, 0, 21, 100, 40, 305, 530 , -1
gkbutt5, ihbutt5 FLbutton "butt5", 1, 0, 21, 100, 40, 405, 530 , -1
FLrun
#endif

connect "Bower", "outleft", "ReverbLeft", "inleft"
connect "Bower", "outright", "ReverbRight", "inright"
connect "Phaser", "outleft", "ReverbLeft", "inleft"
connect "Phaser", "outright", "ReverbRight", "inright"
connect "Droner", "outleft", "ReverbLeft", "inleft"
connect "Droner", "outright", "ReverbRight", "inright"
connect "Sweeper", "outleft", "ReverbLeft", "inleft"
connect "Sweeper", "outright", "ReverbRight", "inright"
connect "Buzzer", "outleft", "ReverbLeft", "inleft"
connect "Buzzer", "outright", "ReverbRight", "inright"
connect "Blower", "outleft", "ReverbLeft", "inleft"
connect "Blower", "outright", "ReverbRight", "inright"
connect "Shiner", "outleft", "ReverbLeft", "inleft"
connect "Shiner", "outright", "ReverbRight", "inright"
; connect "ReverbLeft", "outleft", "ParametricEQ", "inleft"
; connect "ReverbRight", "outright", "ParametricEQ", "inright"
; connect "ParametricEQ", "outleft", "MasterOutput", "inleft"
; connect "ParametricEQ", "outright", "MasterOutput", "inright"
connect "ReverbLeft", "outleft", "MasterOutput", "inleft"
connect "ReverbRight", "outright", "MasterOutput", "inright"

#ifndef FLTK
alwayson "Controls"
#endif
alwayson "VariablesForControls"
alwayson "ReverbLeft"
alwayson "ReverbRight"
alwayson "ParametricEQ"
alwayson "MasterOutput"

opcode ratio2midinn, i, iii
ifundamental, inumerator, idenominator xin
ihertz = ifundamental * inumerator / idenominator
; print ihertz
; m = 12*log2(fm/440 Hz) + 69
ilog2 = log(2)
imidinn = 12 * (log(ihertz / 440) / ilog2) + 69
; print imidinn
xout imidinn
endop

instr 101
iinsno = p1
istart = p2
iduration = p3
ifundamental = p4
inumerator = p5
idenominator = p6
ivelocity = p7
ipan = p8
iratio = inumerator / idenominator
ihertz = ifundamental * iratio
ikey ratio2midinn ifundamental, inumerator, idenominator
event_i "i", "Droner", 0, iduration, ikey, ivelocity, 0, ipan
endin

instr 102
iinsno = p1
istart = p2
iduration = p3
ifundamental = p4
inumerator = p5
idenominator = p6
ivelocity = p7
ipan = p8
iratio = inumerator / idenominator
ihertz = ifundamental * iratio
ikey ratio2midinn ifundamental, inumerator, idenominator
event_i "i", "Phaser", 0, iduration, ikey, ivelocity, 0, ipan
endin

instr 103
iinsno = p1
istart = p2
iduration = p3
ifundamental = p4
inumerator = p5
idenominator = p6
ivelocity = p7
ipan = p8
iratio = inumerator / idenominator
ihertz = ifundamental * iratio
ikey ratio2midinn ifundamental, inumerator, idenominator
event_i "i", "Droner", 0, iduration, ikey, ivelocity, 0, ipan
endin

instr 104
iinsno = p1
istart = p2
iduration = p3
ifundamental = p4
inumerator = p5
idenominator = p6
ivelocity = p7
ipan = p8
iratio = inumerator / idenominator
ihertz = ifundamental * iratio
ikey ratio2midinn ifundamental, inumerator, idenominator
event_i "i", "Blower", 0, iduration, ikey, ivelocity, 0, ipan
endin

gioverlap = 20

gkDistortFactor init 0.4
gkFirstHarmonic init 0.4
instr Droner
iinsno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
print iinsno, istart, iduration, ikey, ivelocity, iphase, ipan
k1 init .5
k2 init .05
k3 init .1
k4 init .2
k5 init .1
k6 init .05
k7 init .1
k8 init 0
k9 init 0
k10 init 0
k3 = gkslider1
k4 = gkslider2
k5 = gkslider3
k6 = gkslider4
k7 = gkslider5
kwaveform init 0
iamp = ampdb(ivelocity)
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
print ihertz
isine ftgenonce 0, 0, 65536, 10, 1, 0, .02
if kwaveform == 0 then
asignal poscil3 1, ihertz, isine
endif
if kwaveform == 1 then
asignal vco2 1, ihertz, 8 ; integrated saw
endif
if kwaveform == 2 then
asignal vco2 1, ihertz, 12 ; triangle
endif
asignal chebyshevpoly asignal, 0, k1, k2, k3, k4, k5, k6, k7, k8, k9, k10
asignal = asignal * kenvelope * 10
aleft, aright pan2 asignal, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
outleta "outleft", aleft
outleta "outright", aright
endin

instr Bower
iinsno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
print iinsno, istart, iduration, ikey, ivelocity, iphase, ipan
iamp = ampdb(ivelocity) * 200
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
kamp = kenvelope
kfreq = ihertz
kpres = 0.25
; krat rspline 0.006,0.988,0.1,0.4
krat rspline 0.006,0.988,1,4
kvibf = 4.5
kvibamp = 0
iminfreq = 20
isine ftgenonce 0,0,65536,10,1
aSig wgbow kamp,kfreq,kpres,krat,kvibf,kvibamp,isine,iminfreq
;aSig butlp aSig,2000
;aSig pareq aSig,80,6,0.707
aleft, aright pan2 aSig / 7, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
outleta "outleft", aleft
outleta "outright", aright
endin

gkratio1 = 1
gkratio2 = 1/3
gkindex1 = 1
gkindex2 = 0.0125
instr Phaser
iinsno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
print iinsno, istart, iduration, ikey, ivelocity, iphase, ipan
iamp = ampdb(ivelocity) * 12
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
isine ftgenonce 0,0,65536,10,1
khertz = ihertz
ifunction1 = isine
ifunction2 = isine
a1,a2 crosspm gkratio1, gkratio2, gkindex1, gkindex2, khertz, ifunction1, ifunction2
aleft, aright pan2 a1+a2, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft * kenvelope
aright = adamping * aright * kenvelope
outleta "outleft", aleft
outleta "outright", aright
endin

gkbritel init 0
gkbriteh init 2.9
gkbritels init .2 / 3
gkbritehs init 2.5 / 2
instr Sweeper
iinsno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
print iinsno, istart, iduration, ikey, ivelocity, iphase, ipan
iamp = ampdb(ivelocity)
gisine ftgenonce 0, 0, 65536, 10, 1
gioctfn ftgenonce 0, 0, 65536, -19, 1, 0.5, 270, 0.5
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
icps = ihertz
kamp expseg 0.001,0.02,0.2,p3-0.01,0.001
ktonemoddep jspline 0.01,0.05,0.2
ktonemodrte jspline 6,0.1,0.2
ktone poscil3 ktonemoddep, ktonemodrte, gisine
kbrite rspline gkbritel, gkbriteh, gkbritels, gkbritehs
ibasfreq init icps
ioctcnt init 3
iphs init 0
;a1 hsboscil kamp, ktone, kbrite, ibasfreq, gisine, gioctfn, ioctcnt, iphs
a1 hsboscil kenvelope, ktone, kbrite, ibasfreq, gisine, gioctfn, ioctcnt, iphs
amod poscil3 0.25, ibasfreq*(1/3), gisine
arm = a1*amod
kmix expseg 0.001, 0.01, rnd(1), rnd(3)+0.3, 0.001
kmix=.25
a1 ntrpol a1, arm, kmix
;a1 pareq a1/10, 400, 15, .707
;a1 tone a1, 500
kpanrte jspline 5, 0.05, 0.1
kpandep jspline 0.9, 0.2, 0.4
kpan poscil3 kpandep, kpanrte, gisine
a1,a2 pan2 a1, kpan
a1 delay a1, rnd(0.1)
a2 delay a2, rnd(0.11)
kenv linsegr 1, 1, 0
kenv = kenvelope
aleft = a1*kenv*.02
aright = a2*kenv*.02
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
outleta "outleft", aleft
outleta "outright", aright
endin

instr Buzzer
iinsno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
print iinsno, istart, iduration, ikey, ivelocity, iphase, ipan
iamp = ampdb(ivelocity) * 4
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
;asignal gbuzz kenvelope, ihertz, 3, gkFirstHarmonic, gkDistortFactor, gisine
isine ftgenonce 0, 0, 65536, 10, 1
gkHarmonics = gkslider1 * 20
asignal buzz kenvelope, ihertz, gkHarmonics, isine
asignal = asignal * 3
;asignal vco2 kenvelope, ihertz, 12
;asignal poscil3 kenvelope, ihertz, giharmonics
;asignal distort asignal, gkDistortFactor * .4, giwaveshaping
aleft, aright pan2 asignal, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
outleta "outleft", aleft
outleta "outright", aright
endin

instr Shiner
iinsno = p1
istart = p2
iduration = p3
ikey = p4
ivelocity = p5
iphase = p6
ipan = p7
print iinsno, istart, iduration, ikey, ivelocity, iphase, ipan
iamp = ampdb(ivelocity) * 4
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
ihertz = cpsmidinn(ikey)
;asignal gbuzz kenvelope, ihertz, 3, gkFirstHarmonic, gkDistortFactor, gisine
gkHarmonics = gkslider1 * 20
;asignal buzz kenvelope, ihertz, gkHarmonics, gisine
;asignal = asignal
asignal vco2 kenvelope * 4, ihertz, 12
;asignal poscil3 kenvelope, ihertz, giharmonics
;asignal distort asignal, gkDistortFactor * .4, giwaveshaping
aleft, aright pan2 asignal, ipan
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
outleta "outleft", aleft
outleta "outright", aright
endin

gkgrainDensity init 150
gkgrainDuration init 0.2
gkgrainAmplitudeRange init 100
gkgrainFrequencyRange init .033
instr Blower
 //////////////////////////////////////////////
 // Original by Hans Mikelson.
 // Adapted by Michael Gogins.
 //////////////////////////////////////////////
 pset 0, 0, 3600
i_instrument = p1
i_time = p2
i_duration = p3
i_midikey = p4
i_midivelocity = p5
i_phase = p6
i_pan = p6
i_depth = p8
i_height = p9
i_pitchclassset = p10
i_homogeneity = p11
ifrequency = cpsmidinn(i_midikey)
iamplitude = ampdb(i_midivelocity) / 200
 ;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
 ; f1 0 65536 1 "hahaha.aif" 0 4 0
 ; f2 0 1024 7 0 224 1 800 0
 ; f3 0 8192 7 1 8192 -1
 ; f4 0 1024 7 0 512 1 512 0
 ; f5 0 1024 10 1 .3 .1 0 .2 .02 0 .1 .04
 ; f6 0 1024 10 1 0 .5 0 .33 0 .25 0 .2 0 .167
 ; a0 14 50
 ; p1 p2 p3 p4 p5 p6 p7 p8 p9 p10
 ; Start Dur Amp Freq GrTab WinTab FqcRng Dens Fade
 ; i1 0.0 6.5 700 9.00 5 4 .210 200 1.8
 ; i1 3.2 3.5 800 7.08 . 4 .042 100 0.8
 ; i1 5.1 5.2 600 7.10 . 4 .0320 100 0.9
 ; i1 7.2 6.6 900 8.03 . 4 .021 150 1.6
 ; i1 21.3 4.5 1000 9.00 . 4 .031 150 1.2
 ; i1 26.5 13.5 1100 6.09 . 4 .121 150 1.5
 ; i1 30.7 9.3 900 8.05 . 4 .014 150 2.5
 ; i1 34.2 8.8 700 10.02 . 4 .14 150 1.6
igrtab ftgenonce 0, 0, 65536, 10, 1, .3, .1, 0, .2, .02, 0, .1, .04
iwintab ftgenonce 0, 0, 65536, 10, 1, 0, .5, 0, .33, 0, .25, 0, .2, 0, .167
iHz = ifrequency
ip4 = iamplitude
ip5 = iHz
ip6 = igrtab
ip7 = iwintab
ip8 = 0.033
ip8 = .002
ip9 = 150
ip9 = 100
ip10 = 1.6
ip10 = 3
idur = p3
iamp = iamplitude ; p4
ifqc = iHz ; cpspch(p5)
igrtab = ip6
iwintab = ip7
ifrng = ip8
idens = ip9
ifade = ip10
igdur = 0.2
iattack = gioverlap
idecay = gioverlap
isustain = p3 - gioverlap
p3 = iattack + isustain + idecay
kenvelope transeg 0.0, iattack / 2.0, 1.5, iamp / 2.0, iattack / 2.0, -1.5, iamp, isustain, 0.0, iamp, idecay / 2.0, 1.5, iamp / 2.0, idecay / 2.0, -1.5, 0
; kamp linseg 0, ifade, 1, idur - 2 * ifade, 1, ifade, 0
kamp = kenvelope
; Amp Fqc Dense AmpOff PitchOff GrDur GrTable WinTable MaxGrDur
aoutl grain ip4, ifqc, gkgrainDensity, gkgrainAmplitudeRange, ifqc * gkgrainFrequencyRange, gkgrainDuration, igrtab, iwintab, 5
aoutr grain ip4, ifqc, gkgrainDensity, gkgrainAmplitudeRange, ifqc * gkgrainFrequencyRange, gkgrainDuration, igrtab, iwintab, 5
aleft = aoutl * kamp * iamplitude
aright = aoutr * kamp * iamplitude
adamping linseg 0, 0.03, 1, p3 - 0.1, 1, 0.07, 0
aleft = adamping * aleft
aright = adamping * aright
outleta "outleft", aleft
outleta "outright", aright
prints "instr %4d t %9.4f d %9.4f k %9.4f v %9.4f p %9.4f\n", p1, p2, p3, p4, p5, p7
endin

gkReverbFeedback init 0.975
gkDelayModulation init 0.875


#ifdef CSOUNDQT

instr ReverbLeft
; p4 = gain of reverb. Adjust empirically
; for desired reverb time. .6 gives
; a good small "live" room sound, .8
; a small hall, .9 a large hall,
; .99 an enormous stone cavern.

; p5 = amount of random pitch modulation
; for the delay lines. 1 is the "normal"
; amount, but this may be too high for
; held pitches such as piano tones.
; Adjust to taste.

; p6 = cutoff frequency of lowpass filters
; in feedback loops of delay lines,
; in Hz. Lower cutoff frequencies results
; in a sound with more high-frequency
; damping.

; 8 delay line FDN reverb, with feedback matrix based upon
; physical modeling scattering junction of 8 lossless waveguides
; of equal characteristic impedance. Based on Julius O. Smith III,
; "A New Approach to Digital Reverberation using Closed Waveguide
; Networks," Proceedings of the International Computer Music
; Conference 1985, p. 47-53 (also available as a seperate
; publication from CCRMA), as well as some more recent papers by
; Smith and others.
; Coded by Sean Costello, October 1999
itone = 16000
asignal inleta "inleft"
afilt1 init 0
afilt2 init 0
afilt3 init 0
afilt4 init 0
afilt5 init 0
afilt6 init 0
afilt7 init 0
afilt8 init 0
idel1 = (2473.000/sr)
idel2 = (2767.000/sr)
idel3 = (3217.000/sr)
idel4 = (3557.000/sr)
idel5 = (3907.000/sr)
idel6 = (4127.000/sr)
idel7 = (2143.000/sr)
idel8 = (1933.000/sr)
; k1-k8 are used to add random pitch modulation to the
; delay lines. Helps eliminate metallic overtones
; in the reverb sound.
k1 randi .001, 3.1, .06
k2 randi .0011, 3.5, .9
k3 randi .0017, 1.11, .7
k4 randi .0006, 3.973, .3
k5 randi .001, 2.341, .63
k6 randi .0011, 1.897, .7
k7 randi .0017, 0.891, .9
k8 randi .0006, 3.221, .44
; apj is used to calculate "resultant junction pressure" for
; the scattering junction of 8 lossless waveguides
.; of equal characteristic impedance. If you wish to
; add more delay lines, simply add them to the following
; equation, and replace the .25 by 2/N, where N is the
; number of delay lines.
apj = .25 * (afilt1 + afilt2 + afilt3 + afilt4 + afilt5 + afilt6 + afilt7 + afilt8)
adum1 delayr 1
adel1 deltapi idel1 + k1 * gkDelayModulation
 delayw asignal + apj - afilt1
adum2 delayr 1
adel2 deltapi idel2 + k2 * gkDelayModulation
 delayw asignal + apj - afilt2
adum3 delayr 1
adel3 deltapi idel3 + k3 * gkDelayModulation
 delayw asignal + apj - afilt3
adum4 delayr 1
adel4 deltapi idel4 + k4 * gkDelayModulation
 delayw asignal + apj - afilt4
adum5 delayr 1
adel5 deltapi idel5 + k5 * gkDelayModulation
 delayw asignal + apj - afilt5
adum6 delayr 1
adel6 deltapi idel6 + k6 * gkDelayModulation
 delayw asignal + apj - afilt6
adum7 delayr 1
adel7 deltapi idel7 + k7 * gkDelayModulation
 delayw asignal + apj - afilt7
adum8 delayr 1
adel8 deltapi idel8 + k8 * gkDelayModulation
 delayw asignal + apj - afilt8
; 1st order lowpass filters in feedback
; loops of delay lines.
afilt1 tone adel1 * gkReverbFeedback, itone
afilt2 tone adel2 * gkReverbFeedback, itone
afilt3 tone adel3 * gkReverbFeedback, itone
afilt4 tone adel4 * gkReverbFeedback, itone
afilt5 tone adel5 * gkReverbFeedback, itone
afilt6 tone adel6 * gkReverbFeedback, itone
afilt7 tone adel7 * gkReverbFeedback, itone
afilt8 tone adel8 * gkReverbFeedback, itone
; The outputs of the delay lines are summed
; and sent to the stereo outputs. This could
; easily be modified for a 4 or 8-channel
; sound system.
aout1 = (afilt1 + afilt3 + afilt5 + afilt7)
aout2 = (afilt2 + afilt4 + afilt6 + afilt8)
aleft = aout1 + aout2
; To the master output.
outleta "outleft", aleft
endin

instr ReverbRight
; p4 = gain of reverb. Adjust empirically
; for desired reverb time. .6 gives
; a good small "live" room sound, .8
; a small hall, .9 a large hall,
; .99 an enormous stone cavern.

; p5 = amount of random pitch modulation
; for the delay lines. 1 is the "normal"
; amount, but this may be too high for
; held pitches such as piano tones.
; Adjust to taste.

; p6 = cutoff frequency of lowpass filters
; in feedback loops of delay lines,
; in Hz. Lower cutoff frequencies results
; in a sound with more high-frequency
; damping.

; 8 delay line FDN reverb, with feedback matrix based upon
; physical modeling scattering junction of 8 lossless waveguides
; of equal characteristic impedance. Based on Julius O. Smith III,
; "A New Approach to Digital Reverberation using Closed Waveguide
; Networks," Proceedings of the International Computer Music
; Conference 1985, p. 47-53 (also available as a seperate
; publication from CCRMA), as well as some more recent papers by
; Smith and others.
; Coded by Sean Costello, October 1999
itone = 16000
asignal inleta "inright"
afilt1 init 0
afilt2 init 0
afilt3 init 0
afilt4 init 0
afilt5 init 0
afilt6 init 0
afilt7 init 0
afilt8 init 0
idel1 = (2473.000/sr)
idel2 = (2767.000/sr)
idel3 = (3217.000/sr)
idel4 = (3557.000/sr)
idel5 = (3907.000/sr)
idel6 = (4127.000/sr)
idel7 = (2143.000/sr)
idel8 = (1933.000/sr)
; k1-k8 are used to add random pitch modulation to the
; delay lines. Helps eliminate metallic overtones
; in the reverb sound.
k1 randi .001, 3.1, .06
k2 randi .0011, 3.5, .9
k3 randi .0017, 1.11, .7
k4 randi .0006, 3.973, .3
k5 randi .001, 2.341, .63
k6 randi .0011, 1.897, .7
k7 randi .0017, 0.891, .9
k8 randi .0006, 3.221, .44
; apj is used to calculate "resultant junction pressure" for
; the scattering junction of 8 lossless waveguides
.; of equal characteristic impedance. If you wish to
; add more delay lines, simply add them to the following
; equation, and replace the .25 by 2/N, where N is the
; number of delay lines.
apj = .25 * (afilt1 + afilt2 + afilt3 + afilt4 + afilt5 + afilt6 + afilt7 + afilt8)
adum1 delayr 1
adel1 deltapi idel1 + k1 * gkDelayModulation
 delayw asignal + apj - afilt1
adum2 delayr 1
adel2 deltapi idel2 + k2 * gkDelayModulation
 delayw asignal + apj - afilt2
adum3 delayr 1
adel3 deltapi idel3 + k3 * gkDelayModulation
 delayw asignal + apj - afilt3
adum4 delayr 1
adel4 deltapi idel4 + k4 * gkDelayModulation
 delayw asignal + apj - afilt4
adum5 delayr 1
adel5 deltapi idel5 + k5 * gkDelayModulation
 delayw asignal + apj - afilt5
adum6 delayr 1
adel6 deltapi idel6 + k6 * gkDelayModulation
 delayw asignal + apj - afilt6
adum7 delayr 1
adel7 deltapi idel7 + k7 * gkDelayModulation
 delayw asignal + apj - afilt7
adum8 delayr 1
adel8 deltapi idel8 + k8 * gkDelayModulation
 delayw asignal + apj - afilt8
; 1st order lowpass filters in feedback
; loops of delay lines.
afilt1 tone adel1 * gkReverbFeedback, itone
afilt2 tone adel2 * gkReverbFeedback, itone
afilt3 tone adel3 * gkReverbFeedback, itone
afilt4 tone adel4 * gkReverbFeedback, itone
afilt5 tone adel5 * gkReverbFeedback, itone
afilt6 tone adel6 * gkReverbFeedback, itone
afilt7 tone adel7 * gkReverbFeedback, itone
afilt8 tone adel8 * gkReverbFeedback, itone
; The outputs of the delay lines are summed
; and sent to the stereo outputs. This could
; easily be modified for a 4 or 8-channel
; sound system.
aout1 = (afilt1 + afilt3 + afilt5 + afilt7)
aout2 = (afilt2 + afilt4 + afilt6 + afilt8)
aright = aout1 + aout2
outleta "outright", aright
endin

#else

instr ReverbLeft
aleft init 0
azero init 0
aleft inleta "inleft"
aleft, aright reverbsc aleft, azero, gkReverbFeedback, 15000.
outleta "outleft", aleft
endin

instr ReverbRight
aleft init 0
azero init 0
aright inleta "inright"
aleft, aright reverbsc azero, aright, gkReverbFeedback, 15000.0
outleta "outright", aright
endin

#endif

gkCenterHz init 200
gkGain init 1
gkQ init 0.7071067 ; sqrt(.5)
instr ParametricEQ
aleft inleta "inleft"
aright inleta "inright"
aleft pareq aleft, gkCenterHz, ampdb(gkGain), gkQ, 0
aright pareq aright, gkCenterHz, ampdb(gkGain), gkQ, 0
outleta "outleft", aleft
outleta "outright", aright
endin

gkMasterLevel init 4
instr MasterOutput
aleft inleta "inleft"
aright inleta "inright"
aleft *= gkMasterLevel
aright *= gkMasterLevel
outs aleft, aright
;fout "Drone-IV-performance.wav", 16, aleft, aright
endin

instr Controls
#ifdef CSOUNDQT
gkslider1 invalue "slider1"
gkslider2 invalue "slider2"
gkslider3 invalue "slider3"
gkslider4 invalue "slider4"
gkslider5 invalue "slider5"
gkbutt1 invalue "butt1"
gkbutt2 invalue "butt2"
gkbutt3 invalue "butt3"
gkbutt4 invalue "butt4"
gkbutt5 invalue "butt5"
gktrackpadx invalue "trackpad.x"
gktrackpady invalue "trackpad.y"
gkaccelerometerx invalue "accelerometerX"
gkaccelerometery invalue "accelerometerY"
gkaccelerometerz invalue "accelerometerZ"
#else
gkslider1 chnget "slider1"
gkslider2 chnget "slider2"
gkslider3 chnget "slider3"
gkslider4 chnget "slider4"
gkslider5 chnget "slider5"
gkbutt1 chnget "butt1"
gkbutt2 chnget "butt2"
gkbutt3 chnget "butt3"
gkbutt4 chnget "butt4"
gkbutt5 chnget "butt5"
gktrackpadx chnget "trackpad.x"
gktrackpady chnget "trackpad.y"
gkaccelerometerx chnget "accelerometerX"
gkaccelerometery chnget "accelerometerY"
gkaccelerometerz chnget "accelerometerZ"
#endif
endin

instr VariablesForControls
if gkslider1 > 0 then
 	gkFirstHarmonic = gkslider1 * 2
	gkgrainDensity = gkslider1 * 400
	gkratio2 = gkslider1 ;1/3
endif
if gkslider2 > 0 then
 	gkDistortFactor = gkslider2 * 2
	gkgrainDuration = 0.005 + gkslider2 / 2
	gkindex1 = gkslider2 * 3;1
endif
if gkslider3 > 0 then
 	gkVolume = gkslider3 * 5
	gkgrainAmplitudeRange = gkslider3 * 300
	gkindex2 = gkslider3 ;0.0125
endif
if gkslider4 > 0 then
	gkgrainFrequencyRange = gkslider4 / 10
endif
if gktrackpady > 0 then
 	gkDelayModulation = gktrackpady * 2
 	; gkGain = gktrackpady * 2 - 1
endif
if gktrackpadx > 0 then
 	gkReverbFeedback = (2/3) + (gktrackpadx / 3)
 	; gkCenterHz = 100 + gktrackpadx * 3000
endif
kbutt1 trigger gkbutt1, .5, 0
if kbutt1 > 0 then
 	gkbritels = gkbritels / 1.5
 	gkbritehs = gkbritehs / 1.5
 	; gkQ = gkQ / 2
endif
kbutt2 trigger gkbutt2, .5, 0
if kbutt2 > 0 then
 	gkbritels = gkbritels * 1.5
 	gkbritehs = gkbritehs * 1.5
 	; gkQ = gkQ * 2
endif

endin

</CsInstruments>
<CsScore>

; Change the tempo, if you like.
t 0 27

; p1 p2 p3 p4 p5 p6 p7 p8
; insno onset duration fundamental numerator denominator velocity pan

; C E B
i 101 0 60 [1 * 60] 1 1 60 [-1 + 1 / 2]
i 101 0 60 [2 * 60] 5 4 60 [-1 + 3 / 2]
i 101 0 60 [3 * 60] 28 15 60 [-1 + 2 / 2]
; C Ab E B
i 104 30 30 [1 * 60] 8 5 60 [-1 + 2 / 2]
; G F# B
i 102 60 60 [1 * 60] 3 2 60 [-1 + 1 / 2]
i 102 60 30 [2 * 60] 45 32 64 [-1 + 2 / 2]
i 102 60 60 [3 * 60] 28 15 64 [-1 + 3 / 2]
; G F B
i 102 90 30 [2 * 60] 4 3 64 [-1 + 2 / 2]
; C E B
i 103 120 60 [1 * 60] 1 1 64 [-1 + 1 / 2]
i 103 120 60 [2 * 60] 5 4 56 [-1 + 2 / 2]
i 103 120 30 [3 * 60] 28 15 56 [-1 + 3 / 2]
i 103 150 30 [4 * 60] 1 1 58 [-1 + 3 / 2]
;i 103 120 60 [3 * 60] 1 1 56 [-1 + 3 / 2]
e 10.0
</CsScore>
</CsoundSynthesizer>

<bsbPanel>
 <label>Widgets</label>
 <objectName/>
 <x>0</x>
 <y>0</y>
 <width>611</width>
 <height>695</height>
 <visible>true</visible>
 <uuid/>
 <bgcolor mode="background">
  <r>0</r>
  <g>85</g>
  <b>0</b>
 </bgcolor>
 <bsbObject type="BSBHSlider" version="2">
  <objectName>slider1</objectName>
  <x>20</x>
  <y>47</y>
  <width>502</width>
  <height>25</height>
  <uuid>{5e269dfb-9532-41d3-83d8-d42cfde539f7}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.58167331</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>21</x>
  <y>22</y>
  <width>504</width>
  <height>25</height>
  <uuid>{e87c67e0-c4b2-407f-bb2f-242be34a52ec}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>slider1</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>255</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBHSlider" version="2">
  <objectName>slider2</objectName>
  <x>20</x>
  <y>94</y>
  <width>502</width>
  <height>25</height>
  <uuid>{91144bb6-31e1-44e5-9d98-53b6c9c93532}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.43824701</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>21</x>
  <y>69</y>
  <width>504</width>
  <height>26</height>
  <uuid>{236a0eb0-fd30-47c2-8845-25315155f74b}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>slider2</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>255</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBHSlider" version="2">
  <objectName>slider3</objectName>
  <x>20</x>
  <y>141</y>
  <width>502</width>
  <height>25</height>
  <uuid>{ad3f9dd8-7147-4f62-9770-e09aa418e837}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.58565737</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>21</x>
  <y>116</y>
  <width>502</width>
  <height>28</height>
  <uuid>{8c8248ab-8c18-484d-b822-91f9f4956fdb}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>slider3</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>255</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBHSlider" version="2">
  <objectName>slider4</objectName>
  <x>20</x>
  <y>188</y>
  <width>502</width>
  <height>25</height>
  <uuid>{93eb0db1-8e07-421f-8121-357e84ff85e4}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.50796813</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>21</x>
  <y>163</y>
  <width>502</width>
  <height>28</height>
  <uuid>{2bdfdb11-1cc5-40fa-af6e-774ea1cab987}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>slider4</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>255</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBHSlider" version="2">
  <objectName>slider5</objectName>
  <x>20</x>
  <y>235</y>
  <width>502</width>
  <height>28</height>
  <uuid>{f7ecf097-1755-4681-8f8f-d4b1acfdc09a}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <minimum>0.00000000</minimum>
  <maximum>1.00000000</maximum>
  <value>0.11952191</value>
  <mode>lin</mode>
  <mouseControl act="jump">continuous</mouseControl>
  <resolution>-1.00000000</resolution>
  <randomizable group="0">false</randomizable>
 </bsbObject>
 <bsbObject type="BSBLabel" version="2">
  <objectName/>
  <x>21</x>
  <y>210</y>
  <width>502</width>
  <height>28</height>
  <uuid>{13642f66-f23b-4727-86a3-74ba1ffb5cf1}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>slider5</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>16</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>255</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>butt5</objectName>
  <x>422</x>
  <y>273</y>
  <width>100</width>
  <height>30</height>
  <uuid>{94adda4c-f3e4-4bdc-8d82-eff3a6c64422}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <type>event</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>butt5</text>
  <image>/</image>
  <eventLine>i1 0 10</eventLine>
  <latch>false</latch>
  <latched>false</latched>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>butt3</objectName>
  <x>220</x>
  <y>273</y>
  <width>100</width>
  <height>30</height>
  <uuid>{a8e83217-4340-4868-9762-60ac5272411c}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <type>event</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>butt3</text>
  <image>/</image>
  <eventLine>i1 0 10</eventLine>
  <latch>false</latch>
  <latched>false</latched>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>butt1</objectName>
  <x>20</x>
  <y>273</y>
  <width>100</width>
  <height>30</height>
  <uuid>{558ef416-b71a-40cd-a160-02f70770ca0e}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <type>event</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>butt1</text>
  <image>/</image>
  <eventLine>i1 0 10</eventLine>
  <latch>false</latch>
  <latched>true</latched>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>butt2</objectName>
  <x>120</x>
  <y>273</y>
  <width>100</width>
  <height>30</height>
  <uuid>{dfa120a7-83a3-498c-997e-20c936989eb1}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <type>event</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>butt2</text>
  <image>/</image>
  <eventLine>i1 0 10</eventLine>
  <latch>false</latch>
  <latched>true</latched>
 </bsbObject>
 <bsbObject type="BSBButton" version="2">
  <objectName>butt4</objectName>
  <x>321</x>
  <y>273</y>
  <width>100</width>
  <height>30</height>
  <uuid>{3225236c-43f3-4276-a227-920e79b83baf}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <type>event</type>
  <pressedValue>1.00000000</pressedValue>
  <stringvalue/>
  <text>butt4</text>
  <image>/</image>
  <eventLine>i1 0 10</eventLine>
  <latch>false</latch>
  <latched>false</latched>
 </bsbObject>
 <bsbObject type="BSBController" version="2">
  <objectName>trackpad.x</objectName>
  <x>21</x>
  <y>349</y>
  <width>502</width>
  <height>154</height>
  <uuid>{7bdad897-657c-4a54-9206-709cc8534ebb}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <objectName2>trackpad.y</objectName2>
  <xMin>0.00000000</xMin>
  <xMax>1.00000000</xMax>
  <yMin>0.00000000</yMin>
  <yMax>1.00000000</yMax>
  <xValue>0.54780876</xValue>
  <yValue>0.15584416</yValue>
  <type>crosshair</type>
  <pointsize>1</pointsize>
  <fadeSpeed>0.00000000</fadeSpeed>
  <mouseControl act="press">jump</mouseControl>
  <color>
   <r>0</r>
   <g>234</g>
   <b>0</b>
  </color>
  <randomizable group="0" mode="both">false</randomizable>
  <bgcolor>
   <r>0</r>
   <g>0</g>
   <b>0</b>
  </bgcolor>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>slider1</objectName>
  <x>443</x>
  <y>23</y>
  <width>80</width>
  <height>25</height>
  <uuid>{7ef693ee-baeb-4c01-bbea-44d8b884b57f}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>0.582</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>15</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>255</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>slider1</objectName>
  <x>443</x>
  <y>211</y>
  <width>80</width>
  <height>25</height>
  <uuid>{1ebffaf7-0a51-4395-af00-21494aad303c}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>0.582</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>15</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>255</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>slider4</objectName>
  <x>443</x>
  <y>166</y>
  <width>80</width>
  <height>25</height>
  <uuid>{4a8f2804-f935-48ce-a461-6e0d42a1a9e3}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>0.508</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>15</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>255</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>slider3</objectName>
  <x>443</x>
  <y>118</y>
  <width>80</width>
  <height>25</height>
  <uuid>{d26c20e9-dfb7-4d91-bea8-456064879ffb}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>0.586</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>15</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>255</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>slider2</objectName>
  <x>443</x>
  <y>70</y>
  <width>80</width>
  <height>25</height>
  <uuid>{c5bc6516-4212-4bfc-a3ab-9267190771ff}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>0.438</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>15</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>255</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>trackpad.x</objectName>
  <x>444</x>
  <y>316</y>
  <width>80</width>
  <height>25</height>
  <uuid>{875228fc-e82f-43c2-83bb-5cc963facc98}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>0.548</label>
  <alignment>right</alignment>
  <font>Arial</font>
  <fontsize>15</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>255</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBDisplay" version="2">
  <objectName>trackpad.y</objectName>
  <x>531</x>
  <y>350</y>
  <width>80</width>
  <height>25</height>
  <uuid>{ae997c24-fc31-43b0-89aa-db0a8ffde4b7}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <label>0.156</label>
  <alignment>left</alignment>
  <font>Arial</font>
  <fontsize>15</fontsize>
  <precision>3</precision>
  <color>
   <r>0</r>
   <g>255</g>
   <b>0</b>
  </color>
  <bgcolor mode="nobackground">
   <r>255</r>
   <g>255</g>
   <b>255</b>
  </bgcolor>
  <bordermode>noborder</bordermode>
  <borderradius>1</borderradius>
  <borderwidth>1</borderwidth>
 </bsbObject>
 <bsbObject type="BSBScope" version="2">
  <objectName/>
  <x>20</x>
  <y>514</y>
  <width>504</width>
  <height>181</height>
  <uuid>{f79d2676-2f95-4b20-94c4-f63775334ac9}</uuid>
  <visible>true</visible>
  <midichan>0</midichan>
  <midicc>0</midicc>
  <value>-255.00000000</value>
  <type>scope</type>
  <zoomx>2.00000000</zoomx>
  <zoomy>1.00000000</zoomy>
  <dispx>1.00000000</dispx>
  <dispy>1.00000000</dispy>
  <mode>0.00000000</mode>
 </bsbObject>
</bsbPanel>
<bsbPresets>
</bsbPresets>
