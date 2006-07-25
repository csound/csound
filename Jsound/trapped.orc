;============================================================================;
;============================================================================;
;============================================================================;
;                            == TRAPPED IN CONVERT ==                        ;
;                                Richard Boulanger                           ;
;============================================================================;
;============================================================================;
;                                   ORCHESTRA                                ;
;============================================================================;
;============================================================================;
;============================================================================;
;                            written July 1979 in music11                    ;
;                          M.I.T. Experimental Music Studio                  ;
;                            revised June 1986 in Csound                     ;
;                                M.I.T. Media Lab                            ;
;                          revised July 1996 in SHARCsound                   ;
;                                Analog Devices Inc.                         ;
;============================================================================;
;============================================================================;
;=================================== HEADER =================================;
;============================================================================;
sr     =        44100
kr     =        4410
ksmps  =        10
nchnls =        2
;============================================================================;
;=============================== INITIALIZATION =============================;
;============================================================================;
garvb  init     0
gadel  init     0
;============================================================================;
;================================== INSTRUMENTS =============================;
;============================================================================;
;==================================== IVORY =================================;
;============================================================================;
       instr    1                            ; p6 = amp
ifreq  =        cpspch(p5)                   ; p7 = vib rate
                                             ; p8 = glis. del time (default < 1)
aglis  expseg   1, p8, 1, p3 - p8, p9        ; p9 = freq drop factor

k1     line     0, p3, 5
k2     oscil    k1, p7, 1
k3     linseg   0, p3 * .7, p6, p3 * .3, 0
a1     oscil    k3, (ifreq + k2) * aglis, 1

k4     linseg   0, p3 * .6, 6, p3 * .4, 0
k5     oscil    k4, p7 * .9, 1, 1.4
k6     linseg   0, p3 * .9, p6, p3 * .1, 0
a3     oscil    k6, ((ifreq + .009) + k5) * aglis, 9, .2

k7     linseg   9, p3 * .7, 1, p3 * .3, 1
k8     oscil    k7, p7 * 1.2, 1, .7
k9     linen    p6, p3 * .5, p3, p3 * .333
a5     oscil    k9, ((ifreq + .007) + k8) * aglis, 10, .3

k10    expseg   1, p3 * .99, 3.1, p3 * .01, 3.1
k11    oscil    k10, p7 * .97, 1, .6
k12    expseg   .001, p3 * .8, p6, p3 * .2, .001
a7     oscil    k12,((ifreq + .005) + k11) * aglis, 11, .5

k13    expseg   1, p3 * .4, 3, p3 * .6, .02
k14    oscil    k13, p7 * .99, 1, .4
k15    expseg   .001, p3 *.5, p6, p3 *.1, p6 *.6, p3 *.2, p6 *.97, p3 *.2, .001
a9     oscil    k15, ((ifreq + .003) + k14) * aglis, 12, .8

k16    expseg   4, p3 * .91, 1, p3 * .09, 1
k17    oscil    k16, p7 * 1.4, 1, .2
k18    expseg   .001, p3 *.6, p6, p3 *.2, p6 *.8, p3 *.1, p6 *.98, p3 *.1, .001
a11    oscil    k18, ((ifreq + .001) + k17) * aglis, 13, 1.3

       outs     a1 + a3 + a5, a7 + a9 + a11
       endin
;============================================================================;
;==================================== BLUE ==================================;
;============================================================================;
       instr 2                               ; p6 = amp
ifreq  =        cpspch(p5)                   ; p7 = reverb send factor
                                             ; p8 = lfo freq
k1     randi    1, 30                        ; p9 = number of harmonic
k2     linseg   0, p3 * .5, 1, p3 * .5, 0    ; p10 = sweep rate
k3     linseg   .005, p3 * .71, .015, p3 * .29, .01
k4     oscil    k2, p8, 1,.2
k5     =        k4 + 2

ksweep linseg   p9, p3 * p10, 1, p3 * (p3 - (p3 * p10)), 1

kenv   expseg   .001, p3 * .01, p6, p3 * .99, .001
asig   gbuzz    kenv, ifreq + k3, k5, ksweep, k1, 15

       outs     asig, asig
garvb  =        garvb + (asig * p7)
       endin
;============================================================================;
;================================== VIOLET ==================================;
;============================================================================;
       instr   3                             ; p6 = amp
ifreq  =       cpspch(p5)                    ; p7 = reverb send factor
                                             ; p8 = rand freq
k3     expseg  1, p3 * .5, 30 ,p3 * .5, 2
k4     expseg  10, p3 *.7, p8, p3 *.3, 6
k8     linen   p6, p3 * .333, p3, p3 * .333
k13    line    0, p3, -1

k14    randh   k3, k4, .5
a1     oscil   k8, ifreq + (p5 * .05) + k14 + k13, 1, .1

k1     expseg  1, p3 * .8, 6, p3 *.2, 1
k6     linseg  .4, p3 * .9, p8 * .96, p3 * .1, 0
k7     linseg  8, p3 * .2, 10, p3 * .76, 2

kenv2  expseg  .001, p3 * .4, p6 * .99, p3 * .6, .0001
k15    randh   k6, k7
a2     buzz    kenv2, ifreq + (p5 * .009) + k15 + k13, k1, 1, .2

kenv1  linen   p6, p3 * .25, p3, p3 * .4
k16    randh   k4 * 1.4, k7 * 2.1, .2
a3     oscil   kenv1, ifreq + (p5 * .1) + k16 + k13, 16, .3

amix   =       a1 + a2 + a3
       outs    a1 + a3, a2 + a3
garvb  =       garvb + (amix * p7)
       endin
;============================================================================;
;==================================== BLACK =================================;
;============================================================================;
       instr   4                             ; p6 = amp
ifreq  =       cpspch(p5)                    ; p7 = filtersweep strtfreq
                                             ; p8 = filtersweep endfreq
k1     expon   p7, p3, p8                    ; p9 = bandwidth
anoise rand    8000                          ; p10 = reverb send factor
a1     reson   anoise, k1, k1 / p9, 1
k2     oscil   .6, 11.3, 1, .1
k3     expseg  .001,p3 * .001, p6, p3 * .999, .001
a2     oscil   k3, ifreq + k2, 15

       outs   (a1 * .8) + a2, (a1 * .6) + (a2 * .7)
garvb  =      garvb + (a2 * p10)
       endin
;============================================================================;
;==================================== GREEN =================================;
;============================================================================;
        instr  5                             ; p6 = amp
ifreq   =      cpspch(p5)                    ; p7 = reverb send factor
                                             ; p8 = pan direction
k1     line    p9, p3, 1                     ; ... (1.0 = L -> R, 0.1 = R -> L)
k2     line    1, p3, p10                    ; p9 = carrier freq
k4     expon   2, p3, p12                    ; p10 = modulator freq
k5     linseg  0, p3 * .8, 8, p3 * .2, 8     ; p11 = modulation index
k7     randh   p11, k4                       ; p12 = rand freq
k6     oscil   k4, k5, 1, .3

kenv1  linen   p6, .03, p3, .2
a1     foscil  kenv1, ifreq + k6, k1, k2, k7, 1

kenv2  linen   p6, .1, p3, .1
a2     oscil   kenv2, ifreq * 1.001, 1

amix   =       a1 + a2
kpan   linseg  int(p8), p3 * .7, frac(p8), p3 * .3, int(p8)
       outs    amix * kpan, amix * (1 - kpan)
garvb  =       garvb + (amix * p7)
       endin
;============================================================================;
;================================== COPPER ==================================;
;============================================================================;
        instr  6                             ; p5 = FilterSweep StartFreq
ifuncl  =      8                             ; p6 = FilterSweep EndFreq
                                             ; p7 = bandwidth
k1      phasor p4                            ; p8 = reverb send factor
k2      table  k1 * ifuncl, 19               ; p9 = amp
anoise  rand   8000
k3      expon  p5, p3, p6
a1      reson  anoise, k3 * k2, k3 / p7, 1

kenv    linen  p9, .01, p3, .05
asig    =      a1 * kenv

        outs   asig, asig
garvb   =      garvb + (asig * p8)
        endin
;============================================================================;
;==================================== PEWTER ================================;
;============================================================================;
       instr   7                             ; p4 = amp
ifuncl =       512                           ; p5 = freq
ifreq  =       cpspch(p5)                    ; p6 = begin phase point
                                             ; p7 = end phase point
a1     oscil   1, ifreq, p10                 ; p8 = ctrl osc amp (.1 -> 1)
k1     linseg  p6, p3 * .5, p7, p3 * .5, p6  ; p9 = ctrl osc func
a3     oscili  p8, ifreq + k1, p9            ; p10 = main osc func (f2 or f3)
a4     phasor  ifreq                         ; ...(function length must be 512!)
a5     table   (a4 + a3) * ifuncl, p10       ; p11 = reverb send factor

kenv   linen   p4, p3 * .4, p3, p3 * .5
asig   =       kenv * ((a1 + a5) * .2)

       outs    asig, asig
garvb  =       garvb + (asig * p11)
       endin
;============================================================================;
;==================================== RED ===================================;
;============================================================================;
       instr   8                             ; p4 = amp
ifuncl =       16                            ; p5 = FilterSweep StartFreq
                                             ; p6 = FilterSweep EndFreq
k1     expon   p5, p3, p6                    ; p7 = bandwidth
k2     line    p8, p3, p8 * .93              ; p8 = cps of rand1
k3     phasor  k2                            ; p9 = cps of rand2
k4     table   k3 * ifuncl, 20               ; p10 = reverb send factor
anoise rand    8000
aflt1  reson   anoise, k1, 20 + (k4 * k1 / p7), 1

k5     linseg  p6 * .9, p3 * .8, p5 * 1.4, p3 * .2, p5 * 1.4
k6     expon   p9 * .97, p3, p9
k7     phasor  k6
k8     tablei  k7 * ifuncl, 21
aflt2  reson   anoise, k5, 30 + (k8 * k5 / p7 * .9), 1

abal   oscil   1000, 1000, 1
a3     balance aflt1, abal
a5     balance aflt2, abal

k11    linen   p4, .15, p3, .5
a3     =       a3 * k11
a5     =       a5 * k11

k9     randh   1, k2
aleft  =       ((a3 * k9) * .7) + ((a5 * k9) * .3)
k10    randh   1, k6
aright =       ((a3 * k10) * .3)+((a5 * k10) * .7)
       outs    aleft, aright
garvb  =       garvb + (a3 * p10)
endin
;============================================================================;
;==================================== SAND ==================================;
;============================================================================;
        instr  9                             ; p4 = delay send factor
ifreq   =      cpspch(p5)                    ; p5 = freq
                                             ; p6 = amp
k2      randh  p8, p9, .1                    ; p7 = reverb send factor
k3      randh  p8 * .98, p9 * .91, .2        ; p8 = rand amp
k4      randh  p8 * 1.2, p9 * .96, .3        ; p9 = rand freq
k5      randh  p8 * .9, p9 * 1.3

kenv    linen  p6, p3 *.1, p3, p3 * .8

a1      oscil  kenv, ifreq + k2, 1, .2
a2      oscil  kenv * .91, (ifreq + .004) + k3, 2, .3
a3      oscil  kenv * .85, (ifreq + .006) + k4, 3, .5
a4      oscil  kenv * .95, (ifreq + .009) + k5, 4, .8

amix    =      a1 + a2 + a3 + a4

        outs   a1 + a3, a2 + a4
garvb   =      garvb + (amix * p7)
gadel   =      gadel + (amix * p4)
        endin
;============================================================================;
;==================================== TAUPE =================================;
;============================================================================;
        instr  10
ifreq   =      cpspch(p5)                    ; p5 = freq
                                             ; p6 = amp
k2      randh  p8, p9, .1                    ; p7 = reverb send factor
k3      randh  p8 * .98, p9 * .91, .2        ; p8 = rand amp
k4      randh  p8 * 1.2, p9 * .96, .3        ; p9 = rand freq
k5      randh  p8 * .9, p9 * 1.3

kenv    linen  p6, p3 *.1, p3, p3 * .8

a1      oscil  kenv, ifreq + k2, 1, .2
a2      oscil  kenv * .91, (ifreq + .004) + k3, 2, .3
a3      oscil  kenv * .85, (ifreq + .006) + k4, 3, .5
a4      oscil  kenv * .95, (ifreq + .009) + k5, 4, .8

amix    =      a1 + a2 + a3 + a4

        outs   a1 + a3, a2 + a4
garvb   =      garvb + (amix * p7)
        endin
;============================================================================;
;==================================== RUST ==================================;
;============================================================================;
       instr   11                            ; p4 = delay send factor
ifreq  =       cpspch(p5)                    ; p5 = freq
                                             ; p6 = amp
k1     expseg  1, p3 * .5, 40, p3 * .5, 2    ; p7 = reverb send factor
k2     expseg  10, p3 * .72, 35, p3 * .28, 6
k3     linen   p6, p3* .333, p3, p3 * .333
k4     randh   k1, k2, .5
a4     oscil   k3, ifreq + (p5 * .05) + k4, 1, .1

k5     linseg  .4, p3 * .9, 26, p3 * .1, 0
k6     linseg  8, p3 * .24, 20, p3 * .76, 2
k7     linen   p6, p3 * .5, p3, p3 * .46
k8     randh   k5, k6, .4
a3     oscil   k7, ifreq + (p5 * .03) + k8, 14, .3

k9     expseg  1, p3 * .7, 50, p3 * .3, 2
k10    expseg  10, p3 * .3, 45, p3 * .7, 6
k11    linen   p6, p3 * .25, p3, p3 * .25
k12    randh   k9, k10, .5
a2     oscil   k11, ifreq + (p5 * .02) + k12, 1, .1

k13    linseg  .4, p3 * .6, 46, p3 * .4, 0
k14    linseg  18, p3 * .1, 50, p3 * .9, 2
k15    linen   p6, p3 * .2, p3, p3 * .3
k16    randh   k13, k14, .8
a1     oscil   k15, ifreq + (p5 * .01) + k16, 14, .3

amix   =       a1 + a2 + a3 + a4
       outs    a1 + a3, a2 + a4
garvb  =       garvb + (amix * p7)
gadel  =       gadel + (amix * p4)
       endin
;============================================================================;
;==================================== TEAL ==================================;
;============================================================================;
       instr   12                            ; p6 = amp
ifreq  =       octpch(p5)                    ; p7 = FilterSweep StartFreq
ifuncl =       8                             ; p8 = FilterSweep PeakFreq
                                             ; p9 = bandwdth
k1     linseg  0, p3 * .8, 9, p3 * .2, 1     ; p10 = reverb send factor
k2     phasor  k1
k3     table   k2 * ifuncl, 22
k4     expseg  p7, p3 * .7, p8, p3 * .3, p7 * .9

anoise rand    8000

aflt   reson   anoise, k4, k4 / p9, 1
kenv1  expseg  .001, p3 *.1, p6, p3 *.1, p6 *.5, p3 *.3, p6 *.8, p3 *.5,.001
a3     oscil   kenv1, cpsoct(ifreq + k3) + aflt * .8, 1

       outs    a3,(a3 * .98) + (aflt * .3)
garvb  =       garvb + (anoise * p10)
       endin
;============================================================================;
;==================================== FOAM ==================================;
;============================================================================;
       instr   13                            ; p6 = amp
ifreq  =       octpch(p5)                    ; p7 = vibrato rate
                                             ; p8 = glis. factor
k1     line    0, p3, p8
k2     oscil   k1, p7, 1
k3     linseg  0, p3 * .7, p6, p3 * .3, 1
a1     oscil   k3, cpsoct(ifreq + k2), 1

k4     linseg  0, p3 * .6, p8 * .995, p3 * .4, 0
k5     oscil   k4, p7 * .9, 1, .1
k6     linseg  0, p3 * .9, p6, p3 * .1, 3
a2     oscil   k6, cpsoct((ifreq + .009) + k5), 4, .2

k7     linseg  p8 * .985, p3 * .7, 0, p3 * .3, 0
k8     oscil   k7, p7 * 1.2, 1, .7
k9     linen   p6, p3 * .5, p3, p3 * .333
a3     oscil   k6, cpsoct((ifreq + .007) + k8), 5, .5

k10    expseg  1, p3 * .8, p8, p3 * .2, 4
k11    oscil   k10, p7 * .97, 1, .6
k12    expseg  .001, p3 * .99, p6 * .97, p3 * .01, p6 * .97
a4     oscil   k12, cpsoct((ifreq + .005) + k11), 6, .8

k13    expseg  .002, p3 * .91, p8 * .99, p3 * .09, p8 * .99
k14    oscil   k13, p7 * .99, 1, .4
k15    expseg  .001, p3 *.5, p6, p3 *.1, p6 *.6, p3 *.2, p6 *.97, p3 *.2, .001
a5     oscil   k15, cpsoct((ifreq + .003) + k14), 7, .9

k16    expseg  p8 * .98, p3 * .81, .003, p3 * .19, .003
k17    oscil   k16, p7 * 1.4, 1, .2
k18    expseg  .001, p3 *.6, p6, p3 *.2, p6 *.8, p3 *.1, p6 *.98, p3 *.1, .001
a6     oscil   k18, cpsoct((ifreq + .001) + k17), 8, .1

       outs    a1 + a3 + a5, a2 + a4 + a6
       endin
;============================================================================;
;==================================== SMEAR =================================;
;============================================================================;
        instr  98
asig    delay  gadel, .08
        outs   asig, asig
gadel   =      0
        endin
;============================================================================;
;==================================== SWIRL =================================;
;============================================================================;
       instr   99                            ; p4 = panrate
k1     oscil   .5, p4, 1
k2     =       .5 + k1
k3     =       1 - k2
asig   reverb  garvb, 2.1
       outs    asig * k2, (asig * k3) * (-1)
garvb  =       0
       endin
;============================================================================;
;============================================================================;
;============================================================================;
;============================================================================;
