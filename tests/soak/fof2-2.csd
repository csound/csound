<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac           -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o fof2.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 128
nchnls = 2

; Example by Chuckk Hubbard 2007

    instr 1            ;table-lookup vocal synthesis

iolaps    =    120
ifna    =    1        ;f1 - sine wave
ifnb    =    2        ;f2 - linear rise shape
itotdur    =    p3
iamp    =    p4 * 0dbfs
ifreq1    =    p5        ;starting frequency
ifreq2    =    p6        ;ending frequency

kamp    linseg    0, .003, iamp, itotdur-.007, iamp, .003, 0, .001, 0
kfund    expseg    ifreq1, itotdur, ifreq2
koct    init    0
kris    init    .003
kdur    init    .02
kdec    init    .007
kphs    init    0
kgliss    init    0

iforma    =    p7        ;starting spectrum
iformb    =    p8        ;ending spectrum

iform1a    tab_i    0, iforma        ;read values of 5 formants for 1st spectrum
iform2a    tab_i    1, iforma
iform3a    tab_i    2, iforma
iform4a    tab_i    3, iforma
iform5a    tab_i    4, iforma
idb1a    tab_i    5, iforma        ;read decibel levels for same 5 formants
idb2a    tab_i    6, iforma
idb3a    tab_i    7, iforma
idb4a    tab_i    8, iforma
idb5a    tab_i    9, iforma
iband1a    tab_i    10, iforma    ;read bandwidths for same 5 formants
iband2a    tab_i    11, iforma
iband3a    tab_i    12, iforma
iband4a    tab_i    13, iforma
iband5a    tab_i    14, iforma
iamp1a    =    ampdb(idb1a)    ;convert db to linear multipliers
iamp2a    =    ampdb(idb2a)
iamp3a    =    ampdb(idb3a)
iamp4a    =    ampdb(idb4a)
iamp5a    =    ampdb(idb5a)

iform1b    tab_i    0, iformb        ;values of 5 formants for 2nd spectrum
iform2b    tab_i    1, iformb
iform3b    tab_i    2, iformb
iform4b    tab_i    3, iformb
iform5b    tab_i    4, iformb
idb1b    tab_i    5, iformb        ;decibel levels for 2nd set of formants
idb2b    tab_i    6, iformb
idb3b    tab_i    7, iformb
idb4b    tab_i    8, iformb
idb5b    tab_i    9, iformb
iband1b    tab_i    10, iformb    ;bandwidths for 2nd set of formants
iband2b    tab_i    11, iformb
iband3b    tab_i    12, iformb
iband4b    tab_i    13, iformb
iband5b    tab_i    14, iformb
iamp1b    =    ampdb(idb1b)    ;convert db to linear multipliers
iamp2b    =    ampdb(idb2b)
iamp3b    =    ampdb(idb3b)
iamp4b    =    ampdb(idb4b)
iamp5b    =    ampdb(idb5b)

kform1    line    iform1a, itotdur, iform1b    ;transition between formants
kform2    line    iform2a, itotdur, iform2b
kform3    line    iform3a, itotdur, iform3b
kform4    line    iform4a, itotdur, iform4b
kform5    line    iform5a, itotdur, iform5b
kband1    line    iband1a, itotdur, iband1b    ;transition of bandwidths
kband2    line    iband2a, itotdur, iband2b
kband3    line    iband3a, itotdur, iband3b
kband4    line    iband4a, itotdur, iband4b
kband5    line    iband5a, itotdur, iband5b
kamp1    line    iamp1a, itotdur, iamp1b    ;transition of amplitudes of formants
kamp2    line    iamp2a, itotdur, iamp2b
kamp3    line    iamp3a, itotdur, iamp3b
kamp4    line    iamp4a, itotdur, iamp4b
kamp5    line    iamp5a, itotdur, iamp5b

;5 formants for each spectrum
a1    fof2    kamp1, kfund, kform1, koct, kband1, kris, kdur, kdec, iolaps, ifna, ifnb, itotdur, kphs, kgliss
a2    fof2    kamp2, kfund, kform2, koct, kband2, kris, kdur, kdec, iolaps, ifna, ifnb, itotdur, kphs, kgliss
a3    fof2    kamp3, kfund, kform3, koct, kband3, kris, kdur, kdec, iolaps, ifna, ifnb, itotdur, kphs, kgliss
a4    fof2    kamp4, kfund, kform4, koct, kband4, kris, kdur, kdec, iolaps, ifna, ifnb, itotdur, kphs, kgliss
a5    fof2    kamp5, kfund, kform5, koct, kband5, kris, kdur, kdec, iolaps, ifna, ifnb, itotdur, kphs, kgliss

aout    =    (a1+a2+a3+a4+a5) * kamp/5    ;sum and scale

aenv linen 1, 0.05, p3, 0.05  ;to avoid clicking

    outs    aout*aenv, aout*aenv
    endin

</CsInstruments>
<CsScore>
f1 0 8192 10 1
f2 0 4096 7 0 4096 1

;****************************************************************
; tables of formant values adapted from MiscFormants.html
; 100's: soprano    200's: alto    300's: countertenor        400's: tenor    500's: bass
; -01: "a" sound    -02: "e" sound    -03: "i" sound    -04: "o" sound    -05: "u" sound
; p-5 through p-9: frequencies of 5 formants
; p-10 through p-14: decibel levels of 5 formants
; p-15 through p-19: bandwidths of 5 formants

;        formant frequencies            decibel levels                bandwidths
;soprano
f101 0 16 -2    800      1150      2900      3900      4950     0.001     -6     -32     -20     -50     80     90     120     130     140
f102 0 16 -2    350     2000     2800     3600     4950     0.001     -20     -15     -40     -56     60     100     120     150     200
f103 0 16 -2    270     2140     2950     3900     4950     0.001    -12     -26     -26     -44     60     90     100     120     120
f104 0 16 -2    450     800     2830     3800     4950     0.001    -11     -22     -22     -50     40     80     100     120     120
f105 0 16 -2    325     700     2700     3800     4950     0.001    -16     -35     -40     -60     50     60     170     180     200
;alto
f201 0 16 -2    800      1150      2800      3500      4950    0.001    -4     -20     -36     -60    80    90     120     130     140
f202 0 16 -2    400      1600      2700      3300      4950    0.001    -24     -30     -35     -60    60     80     120     150     200
f203 0 16 -2     350      1700      2700      3700      4950     0.001    -20     -30     -36     -60     50     100     120     150     200
f204 0 16 -2    450      800      2830      3500      4950     0.001    -9     -16     -28     -55     70     80     100     130     135
f205 0 16 -2    325      700      2530      3500      4950     0.001    -12     -30     -40     -64     50     60     170     180     200
;countertenor
f301 0 16 -2    660      1120      2750      3000      3350     0.001    -6     -23     -24     -38     80     90     120     130     140
f302 0 16 -2    440     1800     2700     3000     3300     0.001    -14     -18     -20     -20     70     80     100     120     120
f303 0 16 -2    270     1850     2900     3350     3590     0.001    -24     -24     -36     -36     40     90     100     120     120
f304 0 16 -2    430     820     2700     3000     3300     0.001    -10     -26     -22     -34     40     80     100     120     120
f305 0 16 -2    370     630     2750     3000     3400     0.001    -20     -23     -30     -34     40     60     100     120     120
;tenor
f401 0 16 -2    650      1080      2650      2900      3250     0.001    -6     -7     -8     -22     80     90     120     130     140
f402 0 16 -2    400     1700     2600     3200     3580     0.001    -14     -12     -14     -20     70     80     100     120     120
f403 0 16 -2    290     1870     2800     3250     3540     0.001    -15     -18     -20     -30     40     90     100     120     120
f404 0 16 -2    400     800     2600     2800     3000     0.001    -10     -12     -12     -26     70     80     100     130     135
f405 0 16 -2    350     600     2700     2900     3300     0.001    -20     -17     -14     -26     40     60     100     120     120
;bass
f501 0 16 -2    600      1040      2250      2450      2750     0.001    -7     -9     -9     -20     60     70     110     120     130
f502 0 16 -2    400      1620      2400      2800      3100     0.001     -12     -9     -12     -18     40     80     100     120     120
f503 0 16 -2    250      1750      2600      3050      3340     0.001    -30     -16     -22     -28     60     90     100     120     120
f504 0 16 -2    400      750      2400      2600      2900    0.001    -11     -21     -20     -40     40     80     100     120     120
f505 0 16 -2    350      600      2400      2675      2950     0.001    -20     -32     -28     -36     40     80     100     120     120
;****************************************************************

;    start dur  amp    start freq    end freq    start formant   end formant
i1    0    1    .8        440		412.5		201        203
i1    +    .    .8        412.5		550		201        204
i1    +    .    .8        495		330		202        205

i1    +    .    .8        110		103.125		501        503
i1    +    .    .8        103.125	137.5 		501        504
i1    +    .    .8        123.75	82.5		502        505

i1    7    .    .4        440		412.5		201        203
i1    8    .    .4        412.5		550		201        204
i1    9    .    .4        495		330		202        205
i1    7    .    .4        110		103.125		501        503
i1    8    .    .4        103.125	137.5		501        504
i1    9    .    .4        123.75	82.5		502        505
i1    +    .    .4        440		412.5		101        103
i1    +    .    .4        412.5		550		101        104
i1    +    .    .4        495		330		102        105
e

</CsScore>
</CsoundSynthesizer>
