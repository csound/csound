<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
-odac      ;;;realtime audio out
;-iadc    ;;;uncomment -iadc if realtime audio input is needed too
; For Non-realtime ouput leave only the line below:
; -o pvsbin.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Richard Boulanger & Menno Knevel

sr = 44100
ksmps  = 1
nchnls = 2
0dbfs  = 1

instr 1

ifftsize = 1024
iwtype   = 1                                      
a1 diskin2 "fox.wav", 1
; to reduce artifacts, try a smaller hopsize, like ifftsize/8
; also using port on frequency and/or amplitude may smooth it if there spikes
fsig pvsanal a1, ifftsize, ifftsize/4, ifftsize, iwtype
kamp, kfr pvsbin fsig, p4

adm poscil3 kamp*4, kfr
outs adm, adm

endin

</CsInstruments>
<CsScore>
;           bin
i 1 0 2.7   15
i 1 3 .     35
i 1 6 .     65
i 1 9 .     95

i 1 13 2.7  15   ; chord
i 1 13 .    35
i 1 13 .    65
i 1 13 .    95
e
</CsScore>
</CsoundSynthesizer>
