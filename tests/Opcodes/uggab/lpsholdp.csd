<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac     ;;;realtime audio out
; For Non-realtime ouput leave only the line below:
; -o lpsholdp.wav -W ;;; for file output any platform

; by Stefano Cucchi 2020

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1


instr 1

kphase1 phasor 3

kmodulation oscil 0.5, 0.01, 2
kphase2 phasor 3+kmodulation

kamp linseg 0, 0.2, 1, p3-0.4, 1, 0.2, 0
kfreq1  lpsholdp kphase1, cpspch(p4), 6, cpspch(p5), 10, cpspch(p6), 12
kfreq2  lpsholdp kphase2, cpspch(p4), 6, cpspch(p5), 10, cpspch(p6), 12

a1 = poscil(kamp, kfreq1, 1)
a2 = poscil(kamp, kfreq2, 1)

outch 1, a1
outch 2, a2
endin

</CsInstruments> 
<CsScore>

f1 0 8192 10 1 0 1 0 1 0 1 0 1 0 1
f2 0 4096 10 1 0 1 1 1

i1 0 10 6.09 6.02 7.03

e

</CsScore>
</CsoundSynthesizer>
