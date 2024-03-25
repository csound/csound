<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac   ;;;realtime audio out
; For Non-realtime ouput leave only the line below:
; -o tableshuffle.wav -W ;;; for file output any platform

; By Stefano Cucchi 2020

</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs = 1

instr 1 
kIndex phasor 1/p3
kIndex = kIndex * 16

if kIndex >= 15.99 then 
tableshuffle 1; shuffle table 1
endif

kFreq table kIndex, 1, 0
asound oscili 0.3, kFreq
outch 1, asound
outch 2, asound

endin

</CsInstruments>
<CsScore>

f 1 0	16	-2	200 250 300 350 400 450 500 550 600 650 700 750 800 850 900 950

i1 0 4
i1 5 4
e

</CsScore>
</CsoundSynthesizer>
