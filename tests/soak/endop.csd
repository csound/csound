<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o endop.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

/* example opcode 1: simple oscillator */

        opcode Oscillator, a, kk

kamp, kcps      xin             ; read input parameters
a1      vco2 kamp, kcps         ; sawtooth oscillator
        xout a1                 ; write output

        endop

/* example opcode 2: lowpass filter with local ksmps */

        opcode Lowpass, a, akk

        setksmps 1              ; need sr=kr
ain, ka1, ka2   xin             ; read input parameters
aout    init 0                  ; initialize output
aout    =  ain*ka1 + aout*ka2   ; simple tone-like filter
        xout aout               ; write output

        endop

/* example opcode 3: recursive call */

        opcode RecursiveLowpass, a, akkpp

ain, ka1, ka2, idep, icnt       xin     ; read input parameters
        if (icnt >= idep) goto skip1    ; check if max depth reached
ain     RecursiveLowpass ain, ka1, ka2, idep, icnt + 1
skip1:
aout    Lowpass ain, ka1, ka2           ; call filter
        xout aout                       ; write output

        endop

/* example opcode 4: de-click envelope */

        opcode DeClick, a, a

ain     xin
aenv    linseg 0, 0.02, 1, p3 - 0.05, 1, 0.02, 0, 0.01, 0
        xout ain * aenv         ; apply envelope and write output

        endop

/* instr 1 uses the example opcodes */

        instr 1

kamp    =  .6                ; amplitude
kcps    expon 50, p3, 500       ; pitch
a1      Oscillator kamp, kcps                   ; call oscillator
kflt    linseg 0.4, 1.5, 0.4, 1, 0.8, 1.5, 0.8  ; filter envelope
a1      RecursiveLowpass a1, kflt, 1 - kflt, 10 ; 10th order lowpass
a1      DeClick a1
        outs a1, a1

endin
</CsInstruments>
<CsScore>

i 1 0 4
e

</CsScore>
</CsoundSynthesizer>
