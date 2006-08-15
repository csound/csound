<CsoundSynthesizer>
<CsOptions>
csound -M0 -odac8 --midi-key-oct=4 --midi-velocity=5 temp.orc temp.sco
</CsOptions>
<CsInstruments>
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; O R C H E S T R A   F O R   V S T   A N D   M I D I   U S E
; Copyright (c) 2006 by Michael Gogins.
; All rights reserved.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; HEADER
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sr                      =                       44100
; Adjust ksmps for best balance between performance and stability.
ksmps                   =                       30
nchnls                  =                       2
; Adjust 0dbfs to allow for MIDI velocity as decibels.
0dbfs                   =                       1000000

; Note that -1 dB for float is 29205.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Channel to instrument assignments.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                        massign                 1,1
                        massign                 2,13
                        massign                 3,6

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Tables
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Waveform for the string-pad
giwave                  ftgen                   1, 0, 8193,     10,     1, .5, .33, .25,  .0, .1,  .1, .1
gisine                  ftgen                   2, 0, 65537,    10,     1
giharpsichord           ftgen                   0, 0, 8193,     7,      -1, 1024, 1, 1024, -1 ; Kelley harpsichord.
gicosine                ftgen                   0, 0, 65537,    11,     1 ; Cosine wave. Get that noise down on the most widely used table!
giexponentialrise       ftgen                   0, 0, 8193,     5,      .001, 513, 1 ; Exponential rise.
githirteen              ftgen                   0, 0, 8193,     9,      1, .3, 0
giln                    ftgen                   0, 0, 8193,     -12,    20.0 ; Unscaled ln(I(x)) from 0 to 20.0.
gibergeman              ftgen                   0, 0, 8193,     10,     .28, 1, .74, .66, .78, .48, .05, .33, .12, .08, .01, .54, .19, .08, .05, .16, .01, .11, .3, .02, .2 ; Bergeman f1
gicookblank             ftgen                   0, 0, 8193,     10,     0 ; Blank wavetable for some Cook FM opcodes.
gicook3                 ftgen                   0, 0, 8193,     10,     1, .4, .2, .1, .1, .05
gikellyflute            ftgen                   0, 0, 8193,     10,     1, .25, .1 ; Kelley flute.
gichebychev             ftgen                   0, 0, 8193,     -7,     -1, 150, .1, 110, 0, 252, 0
giffitch1               ftgen                   0, 0, 8193,     10,     1
giffitch2               ftgen                   0, 0, 8193,     5,      1, 1024, .01
giffitch3               ftgen                   0, 0, 8193,     5,      1, 1024, .001
; Rotor Tables
gitonewheel1            ftgen                   0, 0, 8193,     10,     1, .02, .01
gitonewheel2            ftgen                   0, 0, 8193,     10,     1, 0, .2, 0, .1, 0, .05, 0, .02
; Rotating Speaker Filter Envelopes
gitonewheel3            ftgen                   0, 0, 8193,     7,      0, 110, 0, 18, 1, 18, 0, 110, 0
gitonewheel4            ftgen                   0, 0, 8193,     7,      0, 80, .2, 16, 1, 64, 1, 16, .2, 80, 0
; Distortion Tables
gitonewheel5            ftgen                   0, 0, 8193,     8,      -.8, 336, -.78,  800, -.7, 5920, .7,  800, .78, 336, .8
gitonewheel6            ftgen                   0, 0, 8193,     8       -.8, 336, -.76, 3000, -.7, 1520, .7, 3000, .76, 336, .8

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Instruments
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

                        instr                   1 ; Tone wheel organ by Mikelson
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
; Pitch.
ifrequency              =                       cpsoct(p4)
; Amplitude.
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) / 16.0
; Constant-power pan.
iangle                  =                       p6 / 2.0
ijunk7                  =                       p7
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
ileftpan                =                       sqrt(2.0) / 2.0 * (cos(iangle) + sin(iangle)) * iamplitude;
irightpan               =                       sqrt(2.0) / 2.0 * (cos(iangle) - sin(iangle)) * iamplitude;
iattack                 =                       .05
isustain                =                       p3
irelease                =                       .1
iduration               =                       iattack + isustain + irelease
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
iphase                  =                       p2
ikey                    =                       12 * int(p4 - 6) + 100 * (p4 - 6)
ifqc                    =                       ifrequency
; The lower tone wheels have increased odd harmonic content.
iwheel1                 =                       ((ikey - 12) > 12 ? gitonewheel1 : gitonewheel2)
iwheel2                 =                       ((ikey +  7) > 12 ? gitonewheel1 : gitonewheel2)
iwheel3                 =                        (ikey       > 12 ? gitonewheel1 : gitonewheel2)
iwheel4                 =                       1
;  Start Dur   Amp   Pitch SubFund Sub3rd Fund 2nd 3rd 4th 5th 6th 8th
;i1   0    6    200    8.04   8       8     8    8   3   2   1   0   4
asubfund                oscili                  8, .5*ifqc,      iwheel1, iphase/(ikey-12)
asub3rd                 oscili                  8, 1.4983*ifqc,  iwheel2, iphase/(ikey+7)
afund                   oscili                  8, ifqc,         iwheel3, iphase/ikey
a2nd                    oscili                  8, 2*ifqc,       iwheel4, iphase/(ikey+12)
a3rd                    oscili                  3, 2.9966*ifqc,  iwheel4, iphase/(ikey+19)
a4th                    oscili                  2, 4*ifqc,       iwheel4, iphase/(ikey+24)
a5th                    oscili                  1, 5.0397*ifqc,  iwheel4, iphase/(ikey+28)
a6th                    oscili                  0, 5.9932*ifqc,  iwheel4, iphase/(ikey+31)
a8th                    oscili                  4, 8*ifqc,       iwheel4, iphase/(ikey+36)
asignal                 =                       kdamping * (asubfund + asub3rd + afund + a2nd + a3rd + a4th + a5th + a6th + a8th)
asig1                   =                       asignal * irightpan
asig2                   =                       asignal * ileftpan
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

			instr                   Xanadu1
ishift      =           .00666667               ;shift it 8/1200.
ipch        =           cpspch(p5)              ;convert parameter 5 to cps.
ioct        =           octpch(p5)              ;convert parameter 5 to oct.
kvib        oscili       1/120, ipch/50, gisine       ;vibrato
ag          pluck       2000, cpsoct(ioct+kvib), 1000, 1, 1
agleft      pluck       2000, cpsoct(ioct+ishift), 1000, 1, 1
agright     pluck       2000, cpsoct(ioct-ishift), 1000, 1, 1
kf1         expon       .1, p3, 1.0             ;exponential from 0.1 to 1.0
kf2         expon       1.0, p3, .1             ;exponential from 1.0 to 0.1
adump       delayr      2.0                     ;set delay line of 2.0 sec
atap1       deltapi     kf1                     ;tap delay line with kf1 func.
atap2       deltapi     kf2                     ;tap delay line with kf2 func.
ad1         deltap      2.0                     ;delay 2 sec.
ad2         deltap      1.1                     ;delay 1.1 sec.
            delayw      ag                      ;put ag signal into delay line.
            outs        agleft+atap1+ad1, agright+atap2+ad2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   2 ; Xanadu instr 1
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
; Envelope initialization.
iattack                 =                       0.005
isustain                =                       p3
irelease                =                       0.05
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                 0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
iduration               =                       p3 + iattack + irelease
ifrequency              =                       cpsoct(p4)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) / 2000.
ijunk6                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       (sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))) * iamplitude
ileftgain               =                       (sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))) * iamplitude
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
asig1, asig2            subinstr                "Xanadu1", p4, pchoct(p4)
asig1                   =                       asig1 * kdamping * irightgain
asig2                   =                       asig2 * kdamping * ileftgain
						outs					asig1, asig2
                        endin

                        instr                   Xanadu2
ishift      =           .00666667               ;shift it 8/1200.
ipch        =           cpspch(p5 - 4)              ;convert parameter 5 to cps.
ioct        =           octpch(p5)              ;convert parameter 5 to oct.
kvib        oscili       1/80, 6.1, gisine       ;vibrato
iattack                 =                       0.05
isustain                =                       p3
irelease                =                       0.05
kdamping                linsegr                  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
ag          pluck       1000, cpsoct(ioct + kvib), 1000,gisine, 1
agleft      pluck       1000, cpsoct(ioct+ishift), 1000,gisine, 1
agright     pluck       1000, cpsoct(ioct-ishift), 1000,gisine, 1
adump       delayr      0.4                     ;set delay line of 0.3 sec
ad1         deltap      0.1                     ;delay 100 msec.
ad2         deltap      0.2                     ;delay 200 msec.
            delayw      ag * kdamping                      ;put ag sign into del line.
            outs        kdamping *(agleft+ad1), kdamping*(agright+ad2)
endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   3 ; Xanadu instr 2
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
;  Envelope initialization.
iattack                 =                       0.05
isustain                =                       p3
irelease                =                       0.05
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
iduration               =                       p3 + iattack + irelease
ifrequency              =                       cpsoct(p4)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) / 600.
ijunk6                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       (sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))) * iamplitude
ileftgain               =                       (sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))) * iamplitude
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
asig1, asig2            subinstr                "Xanadu2", p4, pchoct(p4)
asig1                   =                       asig1 * kdamping * irightgain
asig2                   =                       asig2 * kdamping * ileftgain
						outs					asig1, asig2
endin

                        instr                   Xanadu3
ishift      =           .00666667               ;shift it 8/1200.
ipch        =           cpspch(p5)              ;convert parameter 5 to cps.
ioct        =           octpch(p5)              ;convert parameter 5 to oct.
kadsr       linsegr      0, p3/3, 1.0, p3/3, 1.0, p3/3, 0 ;ADSR envelope
kmodi       linsegr      0, p3/3, 5, p3/3, 3, p3/3, 0 ;ADSR envelope for I
kmodr       linsegr      p6, p3, p7              ;r moves from p6->p7 in p3 sec.
a1          =           kmodi*(kmodr-1/kmodr)/2
a1ndx       =           abs(a1*2/20)            ;a1*2 is normalized from 0-1.
a2          =           kmodi*(kmodr+1/kmodr)/2
a3          tablei      a1ndx,giln, 1             ;lookup tbl in f3, normal index
ao1         oscili       a1, ipch, gicosine             ;cosine
a4          =           exp(-0.5*a3+ao1)
ao2         oscili       a2*ipch, ipch, gicosine        ;cosine
aoutl       oscili       1000*kadsr*a4, ao2+cpsoct(ioct+ishift), gisine ;fnl outleft
aoutr       oscili       1000*kadsr*a4, ao2+cpsoct(ioct-ishift), gisine ;fnl outright
            outs        aoutl, aoutr
            endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   4 ; Xanadu instr 3
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
; Envelope initialization.
iattack                 =                       0.05
isustain                =                       p3
irelease                =                       0.05
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
iduration               =                       p3 + iattack + irelease
ifrequency              =                       cpsoct(p4)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) / 2400.
ijunk6                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       (sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))) * iamplitude
ileftgain               =                       (sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))) * iamplitude
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
asig1, asig2            subinstr                "Xanadu3", p4, pchoct(p4), 5.07, 0.5, 1.7
asig1                   =                       asig1 * kdamping * irightgain
asig2                   =                       asig2 * kdamping * ileftgain
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   5 ; Tone wheel organ by Mikelson
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
; Pitch.
ifrequency              =                       cpsoct(p4)
; Amplitude.
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) / 16.0
; Constant-power pan.
iangle                  =                       p6 / 2.0
ijunk7                  =                       p7
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
ileftpan                =                       sqrt(2.0) / 2.0 * (cos(iangle) + sin(iangle)) * iamplitude;
irightpan               =                       sqrt(2.0) / 2.0 * (cos(iangle) - sin(iangle)) * iamplitude;
iattack                 =                       .05
isustain                =                       p3
irelease                =                       .1
iduration               =                       iattack + isustain + irelease
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
iphase                  =                       p2
ikey                    =                       12 * int(p4 - 6) + 100 * (p4 - 6)
ifqc                    =                       ifrequency
; The lower tone wheels have increased odd harmonic content.
iwheel1                 =                       ((ikey - 12) > 12 ? gitonewheel1 : gitonewheel2)
iwheel2                 =                       ((ikey +  7) > 12 ? gitonewheel1 : gitonewheel2)
iwheel3                 =                        (ikey       > 12 ? gitonewheel1 : gitonewheel2)
iwheel4                 =                       1
;  Start Dur   Amp   Pitch SubFund Sub3rd Fund 2nd 3rd 4th 5th 6th 8th
;i1   0    6    200    8.04   8       8     8    8   3   2   1   0   4
asubfund                oscili                  8, .5*ifqc,      iwheel1, iphase/(ikey-12)
asub3rd                 oscili                  8, 1.4983*ifqc,  iwheel2, iphase/(ikey+7)
afund                   oscili                  8, ifqc,         iwheel3, iphase/ikey
a2nd                    oscili                  8, 2*ifqc,       iwheel4, iphase/(ikey+12)
a3rd                    oscili                  3, 2.9966*ifqc,  iwheel4, iphase/(ikey+19)
a4th                    oscili                  2, 4*ifqc,       iwheel4, iphase/(ikey+24)
a5th                    oscili                  1, 5.0397*ifqc,  iwheel4, iphase/(ikey+28)
a6th                    oscili                  0, 5.9932*ifqc,  iwheel4, iphase/(ikey+31)
a8th                    oscili                  4, 8*ifqc,       iwheel4, iphase/(ikey+36)
asignal                 =                       kdamping * (asubfund + asub3rd + afund + a2nd + a3rd + a4th + a5th + a6th + a8th)
asig1                   =                       asignal * irightpan
asig2                   =                       asignal * ileftpan
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   6 ; Guitar, Michael Gogins
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
iattack                 =                       .01
isustain                =                       p3
irelease                =                       .05
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                 0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
ifrequency              =                       cpsoct(p4)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) * 5
ijunk6                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
kamp                    linsegr                  0.0, iattack, iamplitude, isustain, iamplitude, irelease, 0.0
asigcomp                pluck                   kamp, 440, 440, 0, 1
asig                    pluck                   kamp, ifrequency, ifrequency, 0, 1
af1                     reson                   asig, 110, 80
af2                     reson                   asig, 220, 100
af3                     reson                   asig, 440, 80
aout                    balance                 0.6 * af1+ af2 + 0.6 * af3 + 0.4 * asig, asigcomp
kexp                    expseg                  1.0, iattack, 2.0, isustain, 1.0, irelease, 1.0
kenv                    =                       kexp - 1.0
asig1                   =                       aout * ileftgain * kenv * kdamping,
asig2                   =                       aout* irightgain * kenv * kdamping
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   7 ; Harpsichord, James Kelley
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
; Envelope initialization.
iattack                 =                       0.005
isustain                =                       p3
irelease                =                       0.05
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                 0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
iduration               =                       p3 + iattack + irelease
ifrequency              =                       cpsoct(p4 + 1)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) * 2.0
ijunk6                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
; KONTROL
kenvelope               transeg                 0, iamplitude, iduration, -3, 1.0, irelease, -2.5, 0
; AUDIO
apluck                  pluck                   iamplitude, ifrequency, ifrequency, 0, 1
aharp                   oscili                  kenvelope, ifrequency, giharpsichord
aharp2                  balance                 apluck, aharp
aoutsignal              =                       apluck + aharp2
asig1                   =                       aoutsignal * ileftgain * kdamping
asig2                   =                       aoutsignal * irightgain * kdamping
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   8 ; Heavy metal model, Perry Cook
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
                        ; mididefault             60, p3
                        ; midinoteonoct           p4, p5
                        ; print                         p2, p3, p4, p5, p6, p7, p8, p9, p10, p11
iattack                 =                       0.01
idecay                  =                       2.0
isustain                =                       p3
irelease                =                       0.125
                        ; xtratim                       iattack + idecay + irelease
p3                      =                       iattack + iattack + idecay + irelease
kdamping                linsegr                 0.0, iattack, 1.0, idecay + isustain, 1.0, irelease, 0.0
iindex                  =                       1
icrossfade              =                       3
ivibedepth              =                       0.02
iviberate               =                       4.8
ifn1                    =                       gisine
ifn2                    =                       giexponentialrise
ifn3                    =                       githirteen
ifn4                    =                       gisine
ivibefn                 =                       gicosine
ifrequency              =                       cpsoct(p4)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) * 32.0
ijunk6                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta)) * iamplitude
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta)) * iamplitude
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
; AUDIO
adecay                  transeg                 0.0, iattack, 4, 1.0, idecay, -4, 0.1, irelease, -4, 0.0
asignal                 fmmetal                 0.1, ifrequency, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
asig1                   =                       asignal * ileftgain * kdamping * adecay
asig2                   =                       asignal * irightgain * kdamping * adecay
						outs					asig1, asig2
endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   9 ; Xing by Andrew Horner
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
; p4 pitch in octave.pch
; original pitch        = A6
; range                 = C6 - C7
; extended range        = F4 - C7
                        ; mididefault             60, p3
                        ; midinoteonoct           p4, p5
                        ; print                 p2, p3, p4, p5, p7, p8, p9, p10, p11
isine                   =                       1
iinstrument             =                       p1
istarttime              =                       p2
isustain                =                       p3
iattack                 =                       .005
irelease                =                       .06
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
ioctave                 =                       p4
ifrequency              =                       cpsoct(ioctave)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) * 875.0
iphase                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
ixpan                   =                       p7
iradians                =                       ixpan * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
iypan                   =                       p8
izpan                   =                       p9
imason                  =                       p10
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11

idur                    =                       p3
ifreq                   =                       ifrequency
iamp                    =                       iamplitude
inorm                   =                       32310

aamp1                   linsegr                  0,.001,5200,.001,800,.001,3000,.0025,1100,.002,2800,.0015,1500,.001,2100,.011,1600,.03,1400,.95,700,1,320,1,180,1,90,1,40,1,20,1,12,1,6,1,3,1,0,1,0
kdevamp1                linsegr                  0, .05, .3, idur - .05, 0
kdev1                   oscili                  kdevamp1, 6.7, gisine, .8
amp1                    =                       aamp1 * (1 + kdev1)

aamp2                   linsegr                  0,.0009,22000,.0005,7300,.0009,11000,.0004,5500,.0006,15000,.0004,5500,.0008,2200,.055,7300,.02,8500,.38,5000,.5,300,.5,73,.5,5.,5,0,1,1
kdevamp2                linsegr                  0,.12,.5,idur-.12,0
kdev2                   oscili                  kdevamp2, 10.5, gisine, 0
amp2                    =                       aamp2 * (1 + kdev2)

aamp3                   linsegr                  0,.001,3000,.001,1000,.0017,12000,.0013,3700,.001,12500,.0018,3000,.0012,1200,.001,1400,.0017,6000,.0023,200,.001,3000,.001,1200,.0015,8000,.001,1800,.0015,6000,.08,1200,.2,200,.2,40,.2,10,.4,0,1,0
kdevamp3                linsegr                  0, .02, .8, idur - .02, 0
kdev3                   oscili                  kdevamp3, 70, gisine ,0
amp3                    =                       aamp3 * (1 + kdev3),

awt1                    oscili                  amp1, ifreq, gisine
awt2                    oscili                  amp2, 2.7 * ifreq, gisine
awt3                    oscili                  amp3, 4.95 * ifreq, gisine
asig                    =                       awt1 + awt2 + awt3
krel                    linenr                  1,0, idur, .06
asig                    =                       asig * krel * (iamp / inorm)
asig1                   =                       asig * ileftgain * kdamping *.005
asig2                   =                       asig * irightgain * kdamping *.005
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   10 ; FM modulated left and right detuned chorusing, Thomas Kung
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
iattack                 =                       0.333333
irelease                =                       0.25
isustain                =                       p3
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) / 400
iphase                  =                       p6
; Constant-power pan.
; x location ranges from hard left = -1 through center = 0 to hard right = 1.
; angle of pan ranges from hard left = - pi / 2 through center = 0 to hard right = pi / 2.
ix                      =                       p7
iangle                  =                       ix * 3.14159265359 / 2.0
ileftpan                =                       sqrt(2.0) / 2.0 * (cos(iangle) + sin(iangle))
irightpan               =                       sqrt(2.0) / 2.0 * (cos(iangle) - sin(iangle))
iy                      =                       p8
iz                      =                       p9
imason                  =                       p10
ihomogeneity            =                       p11
ip6                     =                       0.3
ip7                     =                       2.2
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
; shift it.
ishift                  =                       4.0 / 12000
; convert parameter 5 to cps.
ipch                    =                       cpsoct(p4)
; convert parameter 5 to oct.
ioct                    =                       p4
; KONTROL
kadsr                   linen                   1.0, iattack, irelease, 0.01
kmodi                   linseg                  0, iattack, 5, isustain, 2, irelease, 0
; r moves from ip6 to ip7 in p3 secs.
kmodr                   linseg                  ip6, p3, ip7
; AUDIO
a1                      =                       kmodi * (kmodr - 1 / kmodr) / 2
; a1*2 is argument normalized from 0-1.
a1ndx                   =                       abs(a1 * 2 / 20)
a2                      =                       kmodi * (kmodr + 1 / kmodr) / 2
; Look up table is in f43, normalized index.
a3                      tablei                  a1ndx, giln, 1
; Cosine
ao1                     oscili                  a1, ipch, gicosine
a4                      =                       exp(-0.5 * a3 + ao1)
; Cosine
ao2                     oscili                  a2 * ipch, ipch, gicosine
; Final output left
aoutl                   oscili                  iamplitude * kadsr * a4, ao2 + cpsoct(ioct + ishift), gisine
; Final output right
aoutr                   oscili                  iamplitude * kadsr * a4, ao2 + cpsoct(ioct - ishift), gisine
asig1                   =                       ileftpan * aoutl * kdamping
asig2                   =                       irightpan * aoutr * kdamping
                        outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   11 ; String pad
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
; String-pad borrowed from the piece "Dorian Gray",
; http://akozar.spymac.net/music/ Modified to fit my needs
    ij1=p1
    ij2=p2
    ij3=p3
    ij4=p4
    ij5=p5
    ij6=p6
    ij7=p7
    ij8=p8
    ij9=p9
    ij10=p10
    ij11=p11
  ihz  = cpsoct(p4)
  idb  = p5
  ipos = p7
  iamp = ampdb(idb) * 1.5

  ; Slow attack and release
  kctrl   linsegr  0, p3*0.5, iamp, p3*.5, 0
  ; Slight chorus effect
  afund   oscili   kctrl, ihz,      giwave       ; audio oscillator
  acel1   oscili   kctrl, ihz - .1, giwave       ; audio oscillator - flat
  acel2   oscili   kctrl, ihz + .1, giwave       ; audio oscillator - sharp
  asig    =   afund + acel1 + acel2

  ; Cut-off high frequencies depending on midi-velocity
  ; (larger velocity implies more brighter sound)
  asig butterlp asig, (p5-60)*40+900

  ; Panning
  ippan =       ipos*1.570796325  ; half of PI (radians of 90o angle)
  ipleft        =       cos(ippan)        ; half sign "down"
  ipright       =       sin(ippan)        ; half sign "up"
  ; print giwave,ihz,iamp,ipos,ipleft,ipright
  asig1 = asig*ipleft;
  asig2 = asig*ipright;
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   12 ; Filtered chorus, Michael Bergeman
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
; Original pfields
; p1 p2 p3 p4 p5 p6 p7 p8 p9
; ins st dur db func at dec freq1 freq2
iattack                 =                       0.03
isustain                =                       p3
irelease                =                       0.52
p3                      =                       p3 + iattack + irelease
kdamping                linsegr                  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
ip4                     =                       p5
idb                     =                       ampdb(p5) * 1.5
ijunk6                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
ip5                     =                       gibergeman
ip3                     =                       p3
ip6                     =                       p3 * .25
ip7                     =                       p3 * .75
ip8                     =                       cpsoct(p4 - .01)
ip9                     =                       cpsoct(p4 + .01)
ip10                    =                       p10
isc                     =                       idb * .333
k1                      line                    40, p3, 800
k2                      line                    440, p3, 220
k3                      linen                   isc, ip6, p3, ip7
k4                      line                    800, ip3,40
k5                      line                    220, ip3,440
k6                      linen                   isc, ip6, ip3, ip7
k7                      linen                   1, ip6, ip3, ip7
a5                      oscili                  k3, ip8, ip5
a6                      oscili                  k3, ip8 * .999, ip5
a7                      oscili                  k3, ip8 * 1.001, ip5
a1                      =                       a5 + a6 + a7
a8                      oscili                  k6, ip9, ip5
a9                      oscili                  k6, ip9 * .999, ip5
a10                     oscili                  k6, ip9 * 1.001, ip5
a11                     =                       a8 + a9 + a10
a2                      butterbp                a1, k1, 40
a3                      butterbp                a2, k5, k2 * .8
a4                      balance                 a3, a1
a12                     butterbp                a11, k4, 40
a13                     butterbp                a12, k2, k5 * .8
a14                     balance                 a13, a11
a15                     reverb2                 a4, 5, .3
a16                     reverb2                 a4, 4, .2
a17                     =                       (a15 + a4) * ileftgain * k7
a18                     =                       (a16 + a4) * irightgain * k7
asig1                   =                       a17 * kdamping
asig2                   =                       a18 * kdamping
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   13 ; Plain plucked string, Michael Gogins
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
iattack                 =                       .002
isustain                =                       p3
irelease                =                       .075
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
ifrequency              =                       cpsoct(p4)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) * 1.25
ijunk6                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
kenvelope               transeg                 0, iattack, -4, iamplitude,  isustain, -4, iamplitude / 10.0, irelease, -4, 0
aexcite                 oscili                  1, 1, gisine
;asignal1               wgpluck                 ifrequency * 1.00003, 1.0, .5, .1, 10, 1000, aexcite
;asignal2               wgpluck                 ifrequency * 0.99997, 1.0, .5, .1, 10, 1000, aexcite
asignal1                pluck                   1, ifrequency, ifrequency * 1.002, 0, 1
asignal2                pluck                   1, ifrequency * 1.003, ifrequency, 0, 1
apluckout               =                       (asignal1 + asignal2) * kenvelope
asig1                   =                       ileftgain * apluckout * kdamping
asig2                   =                       irightgain * apluckout * kdamping
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr 14                ; Rhodes electric piano model, Perry Cook
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
iattack                 =                       0.01
isustain                =                       p3
irelease                =                       0.125
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                 0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
iindex                  =                       4
icrossfade              =                       3
ivibedepth              =                       0.2
iviberate               =                       6
ifn1                    =                       gisine
ifn2                    =                       gicosine
ifn3                    =                       gisine
ifn4                    =                       gicookblank
ivibefn                 =                       gisine
ifrequency              =                       cpsoct(p4)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) * 1.25
ijunk6                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
; AUDIO
asignal                 fmrhode                 iamplitude, ifrequency, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
asig1                   =                       ileftgain * asignal * kdamping
asig2                   =                       irightgain * asignal * kdamping
                        outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr 15                ; Tubular bell model, Perry Cook
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
iattack                 =                       0.01
isustain                =                       p3
irelease                =                       0.125
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                 0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
iindex                  =                       1
icrossfade              =                       2
ivibedepth              =                       0.2
iviberate               =                       6
ifn1                    =                       gisine
ifn2                    =                       gicook3
ifn3                    =                       gisine
ifn4                    =                       gisine
ivibefn                 =                       gicosine
ifrequency              =                       cpsoct(p4)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) * 2.0
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
; AUDIO
asignal                 fmbell                  1.0, ifrequency, iindex, icrossfade, ivibedepth, iviberate, ifn1, ifn2, ifn3, ifn4, ivibefn
asig1                   =                       ileftgain * asignal * iamplitude * kdamping
asig2                   =                       irightgain * asignal * iamplitude * kdamping
                        outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   16 ; FM moderate index, Michael Gogins
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
icarrier                =                       1
iratio                  =                       1.25
ifmamplitude            =                       8
index                   =                       5.4
iattack                 =                       0.02
isustain                =                       p3
irelease                =                       0.05
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                 0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
ifrequency              =                       cpsoct(p4)
ifrequencyb             =                       ifrequency * 1.003
icarrierb               =                       icarrier * 1.004
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5)
ijunk6                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
kindenv                 expseg                  .000001, iattack, 1, isustain, .125, irelease, .000001
        kindex                  =                       kindenv * index * ifmamplitude
aouta                   foscili                 iamplitude, ifrequency, icarrier, iratio, index, 1
aoutb                   foscili                 iamplitude, ifrequencyb, icarrierb, iratio, index, 1
; Plus amplitude correction.
afmout                  =                       (aouta + aoutb) * kindenv
asig1                   =                       ileftgain * afmout * kdamping
asig2                   =                       irightgain * afmout * kdamping
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   17 ; FM moderate index 2, Michael Gogins
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
icarrier                =                       1
iratio                  =                       1
ifmamplitude            =                       6
index                   =                       2.5
iattack                 =                       0.02
isustain                =                       p3
irelease                =                       0.05
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                 0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
ifrequency              =                       cpsoct(p4)
ifrequencyb             =                       ifrequency * 1.003
icarrierb               =                       icarrier * 1.004
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5)
ijunk6                  =                       p6
; Constant-power pan.
; x location ranges from hard left = -1 through center = 0 to hard right = 1.
; angle of pan ranges from hard left = - pi / 2 through center = 0 to hard right = pi / 2.
ix                      =                       p7
iangle                  =                       ix * 3.14159265359 / 2.0
ileftpan                =                       sqrt(2.0) / 2.0 * (cos(iangle) + sin(iangle))
irightpan               =                       sqrt(2.0) / 2.0 * (cos(iangle) - sin(iangle))
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
; KONTROL
kindenv                 expseg                  .000001, iattack, 1.0, isustain, .0125, irelease, .000001
kindex                  =                       kindenv * index * ifmamplitude - .000001
; AUDIO
aouta                   foscili                 iamplitude, ifrequency, icarrier, iratio, index, 1
aoutb                   foscili                 iamplitude, ifrequencyb, icarrierb, iratio, index, 1
; Plus amplitude correction.
afmout                  =                       (aouta + aoutb) * kindenv
asig1                   =                       ileftpan * afmout * kdamping
asig2                   =                       irightpan * afmout * kdamping
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   18 ; Guitar, Michael Gogins
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
iattack                 =                       .01
isustain                =                       p3
irelease                =                       .05
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                 0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
ifrequency              =                       cpsoct(p4)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) * 5
ijunk6                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
kamp                    linsegr                  0.0, iattack, iamplitude, isustain, iamplitude, irelease, 0.0
asigcomp                pluck                   kamp, 440, 440, 0, 1
asig                    pluck                   kamp, ifrequency, ifrequency, 0, 1
af1                     reson                   asig, 110, 80
af2                     reson                   asig, 220, 100
af3                     reson                   asig, 440, 80
aout                    balance                 0.6 * af1+ af2 + 0.6 * af3 + 0.4 * asig, asigcomp
kexp                    expseg                  1.0, iattack, 2.0, isustain, 1.0, irelease, 1.0
kenv                    =                       kexp - 1.0
asig1                   =                       aout * ileftgain * kenv * kdamping,
asig2                   =                       aout* irightgain * kenv * kdamping
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   19 ;  Flute, James Kelley
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
                        ; print                         p2, p3, p4, p5, p6, p7, p8, p9, p10, p11
                        ; mididefault             60, p3
                        ; midinoteonoct           p4, p5
; Do some phasing.
icpsp1                  =                       cpsoct(p4 - .0002)
icpsp2                  =                       cpsoct(p4 + .0002)
; Normalize to 80 dB = ampdb(80).
ip6                     =                       ampdb(p5)
ijunk6                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
iradians                =                       p7 * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
iattack                 =                       0.04
isustain                =                       p3
irelease                =                       0.15
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
ip4                     =                       0
                        if                      (ip4 == int(ip4 / 2) * 2) goto initslurs
                        ihold
initslurs:
iatttm                  =                       0.09
idectm                  =                       0.1
isustm                  =                       p3 - iatttm - idectm
idec                    =                       ip6
ireinit                 =                       -1
                        if                      (ip4 > 1) goto checkafterslur
ilast                   =                       0
checkafterslur:
                        if                      (ip4 == 1 || ip4 == 3) goto doneslurs
idec                    =                       0
ireinit                 =                       0
; KONTROL
doneslurs:
                        if                      (isustm <= 0)   goto simpleenv
kamp                    linsegr                  ilast, iatttm, ip6, isustm, ip6, idectm, idec, 0, idec
                        goto                    doneenv
simpleenv:
kamp                    linsegr                  ilast, p3 / 2,ip6, p3 / 2, idec, 0, idec
doneenv:
ilast                   =                       ip6
; Some vibrato.
kvrandamp               rand                    .1
kvamp                   =                       (8 + p4) *.06 + kvrandamp
kvrandfreq              rand                    1
kvfreq                  =                       5.5 + kvrandfreq
kvbra                   oscili                  kvamp, kvfreq, 1, ireinit
kfreq1                  =                       icpsp1 + kvbra
kfreq2                  =                       icpsp2 + kvbra
; Noise for burst at beginning of note.
knseenv                 expon                   ip6 / 4, .2, 1
; AUDIO
anoise1                 rand                    knseenv
anoise                  tone                    anoise1, 200
a1                      oscili                  kamp, kfreq1, gikellyflute, ireinit
a2                      oscili                  kamp, kfreq2, gikellyflute, ireinit
a3                      =                       a1 + a2 + anoise
asig1                   =                       a3 * ileftgain * kdamping
asig2                   =                       a3 * irightgain * kdamping
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   20 ; Delayed plucked string, Michael Gogins
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
                        ; print                         p2, p3, p4, p5, p6, p7, p8, p9, p10, p11
                        ; mididefault             60, p3
                        ; midinoteonoct           p4, p5
iattack                 =                       0.02
isustain                =                       p3
irelease                =                       0.15
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
ioctave                 =                       p4
ihertz                  =                       cpsoct(ioctave)
; Detuning of strings by 4 cents each way.
idetune                 =                       4.0 / 1200.0
ihertzleft              =                       cpsoct(ioctave + idetune)
ihertzright             =                       cpsoct(ioctave - idetune)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5)
ijunk6                  =                       p6
; Constant-power pan.
; x location ranges from hard left = -1 through center = 0 to hard right = 1.
; Angle of pan ranges from hard left = - pi / 2 through center = 0 to hard right = pi / 2.
ix                      =                       p7
iangle                  =                       ix * 3.14159265359 / 2.0
ileftpan                =                       sqrt(2.0) / 2.0 * (cos(iangle) + sin(iangle))
irightpan               =                       sqrt(2.0) / 2.0 * (cos(iangle) - sin(iangle))
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
; INITIALIZATION
igenleft                =                       gisine
igenright               =                       gicosine
; KONTROL
kvibrato                oscili                  1.0 / 120.0, 7.0, 1
; AUDIO
kexponential            expseg                  1.0, p3 + iattack, 0.0001, irelease, .0001
kenvelope               =                       (kexponential - 0.0001) * kdamping
ag                      pluck                   iamplitude, cpsoct(ioctave + kvibrato), 200, igenleft, 1
agleft                  pluck                   iamplitude, ihertzleft, 200, igenleft, 1
agright                 pluck                   iamplitude, ihertzright, 200, igenright, 1
imsleft                 =                       0.2 * 1000
imsright                =                       0.21 * 1000
adelayleft              vdelay                  ag * kenvelope, imsleft, imsleft + 100
adelayright             vdelay                  ag * kenvelope, imsright, imsright + 100
asignal                 =                       kdamping * (agleft + adelayleft + agright + adelayright)
; Highpass filter to exclude speaker cone excursions.
asignal1                butterhp                asignal, 32.0
asignal2                balance                 asignal1, asignal
asig1                   =                       ileftpan * asignal * kdamping
asig2                   =                       irightpan * asignal * kdamping
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   21 ; Melody (Chebyshev / FM / additive), Jon Nelson
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
                        ; print                         p2, p3, p4, p5, p6, p7, p8, p9, p10, p11
                        ; mididefault             60, p3
                        ; midinoteonoct           p4, p5
; Pitch.
i1                      =                       cpsoct(p4)
; Amplitude.
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) * 1.0 / 5000.0
; Constant-power pan.
iangle                  =                       p6 / 2.0
ijunk7                  =                       p7
ijunk8                  =                       p8
ijunk9                  =                       p9
ijunk10                 =                       p10
ijunk11                 =                       p11
ileftpan                =                       sqrt(2.0) / 2.0 * (cos(iangle) + sin(iangle)) * iamplitude;
irightpan               =                       sqrt(2.0) / 2.0 * (cos(iangle) - sin(iangle)) * iamplitude;
;ip6                    =                       cheby no
ip6                     =                       gichebychev
iattack                 =                       .05
isustain                =                       p3
irelease                =                       .1
iduration               =                       iattack + isustain + irelease
                        ; xtratim                       iattack + irelease
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                  0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
k100                    randi                   1,10
k101                    oscili                  1, 5 + k100, gisine
k102                    linsegr                  0, .5, 1, (iduration - .5), 1
k100                    =                       i1 + (k101 * k102)
; Envelope for driving oscillator.
k1                      linenr                  .5, iduration * .3, iduration * .2, 0.01
k2                      line                    1, iduration, .5
k1                      =                       k2 * k1
; Amplitude envelope.
k10                     expseg                  .0001, iattack, iamplitude, isustain, iamplitude * .8, irelease, .0001
k10                     =                       (k10 - .0001) * kdamping
; Power to partials.
k20                     linsegr                  1.485, iattack, 1.5, (isustain + irelease), 1.485
;a1-3 are for cheby with p6=1-4
a1                      oscili                  k1, k100 - .025, gicook3
; Tables a1 to fn13, others normalize,
a2                      tablei                  a1, ip6, 1, .5
a3                      balance                 a2, a1
; Try other waveforms as well.
a4                      foscil                  1, k100 + .04, 1, 2.005, k20, gisine
a5                      oscili                  1, k100, gisine
a6                      =                       ((a3 * .1) + (a4 * .1) + (a5 * .8)) * k10
a7                      comb                    a6, .5, 1 / i1
a8                      =                       (a6 * .9) + (a7 * .1)
a9          balance         a8, a1 * 32767.0
asig1                   =                       (a9 * ileftpan) * kdamping
asig2                   =                       (a9 * irightpan) * kdamping
						outs					asig1, asig2
                        endin

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                        instr                   22 ; Enhanced FM bell, John ffitch
                        pset                    0, 0, 1000, 0, 0, 0, 0, 0, 0, 0, 0
iinstrument             =                       p1
istarttime              =                       p2
iattack                 =                       .005
isustain                =                       p3
irelease                =                       .06
p3                      =                       isustain + iattack + irelease
kdamping                linsegr                 0.0, iattack, 1.0, isustain, 1.0, irelease, 0.0
ioctave                 =                       p4
ifrequency              =                       cpsoct(ioctave)
; Normalize so iamplitude for p5 of 80 == ampdb(80).
iamplitude              =                       ampdb(p5) / 3
iphase                  =                       p6
; Constant-power pan.
ipi                     =                       4.0 * taninv(1.0)
ixpan                   =                       p7
iradians                =                       ixpan * ipi / 2.0
itheta                  =                       iradians / 2.0
; Translate angle in [-1, 1] to left and right gain factors.
irightgain              =                       sqrt(2.0) / 2.0 * (cos(itheta) + sin(itheta))
ileftgain               =                       sqrt(2.0) / 2.0 * (cos(itheta) - sin(itheta))
iypan                   =                       p8
izpan                   =                       p9
imason                  =                       p10
ihomogeneity            =                       p11
idur                    =                       15
iamp                    =                       iamplitude
ifenv                   =                       giffitch2                       ; BELL SETTINGS:
ifdyn                   =                       giffitch3                       ; AMP AND INDEX ENV ARE EXPONENTIAL
ifq1                    =                       cpsoct(p4 - 1) * 5              ; DECREASING, N1:N2 IS 5:7, imax=10
if1                     =                       giffitch1                               ; DURATION = 15 sec
ifq2                    =                       cpsoct(p4 - 1) * 7
if2                     =                       giffitch1
imax                    =                       10
aenv                    oscili                  iamp, 1 / idur, ifenv           ; ENVELOPE
adyn                    oscili                  ifq2 * imax, 1 / idur, ifdyn    ; DYNAMIC
anoise                  rand                    50
amod                    oscili                  adyn + anoise, ifq2, if2        ; MODULATOR
acar                    oscili                  aenv, ifq1 + amod, if1          ; CARRIER
                        timout                  0.5, idur, noisend
knenv                   linsegr                  iamp, 0.2, iamp, 0.3, 0
anoise3                 rand                    knenv
anoise4                 butterbp                anoise3, iamp, 100
anoise5                 balance                 anoise4, anoise3
noisend:
arvb                    nreverb                 acar, 2, 0.1
asignal                 =                       acar + anoise5 + arvb
asig1                   =                       asignal * ileftgain * kdamping
asig2                   =                       asignal * irightgain * kdamping
						outs					asig1, asig2
                        endin
</CsInstruments>
<CsScore>
f 0 3600

</CsScore>
</CsoundSynthesizer>
