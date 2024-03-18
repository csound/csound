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

;By Andres Cabrera 2007

    instr 1            ;table-lookup vocal synthesis

kris init p12
kdur init p13
kdec init p14

iolaps init p15

ifna init 1  ; Sine wave
ifnb init 2  ; Straight line rise shape

itotdur init p3

kphs init 0  ; No phase modulation (constant kphs)


kfund line p4, p3, p5
kform line p6, p3, p7
koct line p8, p3, p9
kband line p10, p3, p11
kgliss line p16, p3, p17

kenv linen 5000, 0.03, p3, 0.03  ;to avoid clicking


aout    fof2    kenv, kfund, kform, koct, kband, kris, kdur, kdec, iolaps, \
      ifna, ifnb, itotdur, kphs, kgliss


    outs    aout, aout
    endin

</CsInstruments>
<CsScore>
f1 0 8192 10 1
f2 0 4096 7 0 4096 1

;              kfund1  kfund2  kform1  kform2  koct1  koct2  kband1 kband2 kris  kdur  kdec  iolaps  kgliss1 kgliss2
i1    0    4    220     220      510     510       0      0      30   30   0.01  0.03  0.01   20       1        1
i1    +    .    220     220      510     910       0      0      30   30   0.01  0.03  0.01   20       1        1
i1    +    .    220     220      510     510       0      0     100   30   0.01  0.03  0.01   20       1        1
i1    +    .    220     220      510     510       0      1      30   30   0.01  0.03  0.01   20       1        1
i1    +    .    220     220      510     510       0      0      30   30   0.01  0.03  0.01   20       1        2
i1    +    .    220     220      510     510       0      0      30   30   0.01  0.03  0.01   20       0.5      1
i1    +    .    220     220      510     510       0      0      30   30   0.01  0.05  0.01   100      1        1
i1    +    .    220     440      510     510       0      0      30   30   0.01  0.05  0.01   100      1        1

e

</CsScore>
</CsoundSynthesizer>
