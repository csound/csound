<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac             ;;;RT audio out
; For Non-realtime ouput leave only the line below:
; -o shaker.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

; by Menno Knevel

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

knum  =   p5
kfreq	line p4, p3, 440
a1 shaker .5, kfreq, 8, 0.999, knum
outs a1, a1

endin

</CsInstruments>
<CsScore>
;       frq     #
i 1 0 1 440     3
i 1 2 1 440    300
i 1 4 1 440    3000
i 1 6 2 4000    100

</CsScore>
</CsoundSynthesizer>
