<CsoundSynthesizer>
<CsOptions>

; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o lfo_advanced.wav -W ;;; for file output any platform

; By Stefano Cucchi 2020

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1
instr 1

; LFO 1
kAmpLFO1 linseg p6, p3, p7
kFreqLFO1 linseg p8, p3, p9
kLFO1 lfo kAmpLFO1, kFreqLFO1, p10

; LFO 2
kAmpLFO2 linseg p7, p3, p6
kFreqLFO2 linseg p9, p3, p8
kLFO2 lfo kAmpLFO2, kFreqLFO1, p11

; AUDIO SIGNAL
asig1 oscili p4+kLFO1, p5+kLFO2,1

; LFO DELAY
kAmpDELAY1 lfo, 100, 1.35, 1
kAmpDELAY1 = kAmpDELAY1 + 50

; DELAY SIGNAL
adel1 vdelay asig1, kAmpDELAY1, 500

kdeclick linseg 0, 1.5, p12, p3-3, p12, 1.5, 0
outch 1, (asig1  + (adel1 *0.3)) * kdeclick
outch 2, (asig1  + (adel1 *0.3)) * kdeclick

endin

</CsInstruments>
<CsScore>


f 1 0 4096 10 1 0.3 0.2 0.1 0.03 0 0.3

i1 0 8 0.3 250 0.22 0.65 3 6 0 3 0.8
i1 8 8 0.3 123 0.12 10.85 70 6 1 5 0.09


e
</CsScore>
</CsoundSynthesizer>
