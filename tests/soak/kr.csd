<CsoundSynthesizer>
<CsOptions>

; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac         ;;  -iadc    ;;;RT audio I/O
; For Non-realtime ouput leave only the line below:
; -o kr_example.wav -W ;;; for file output any platform

; By Stefano Cucchi 2020

</CsOptions>

<CsInstruments>

sr     =        44100

kr = 4410 ; NO glitch
;kr = 10 ; Some GLITCH in kFreqMod
;kr = 2 ; Lots of GLITCH in kFreqMod
nchnls = 1
0dbfs = 1

instr 1

kenvelopemod linseg 0, 2, p4, p3-4, p4, 2, 0

kFreqMod expseg 200, p3, 800
amodulator oscil kenvelopemod, kFreqMod, 2

acarf phasor 440
ifn = 1
ixmode = 1
ixoff = 0
iwrap = 1
acarrier tablei acarf+amodulator, ifn, ixmode, ixoff, iwrap

kgenenvelop linseg 0, 0.5, 0.3, p3-1, 0.3, 0.5, 0
out acarrier * kgenenvelop

endin

</CsInstruments>

<CsScore>

f1 0 4096 10 1
f2 0 4096 10 1 0.3 0.5 0.24 0.56 0.367

i1 0 5 0.2 
e
</CsScore>

</CsoundSynthesizer>
