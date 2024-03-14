<CsoundSynthesizer>
<CsOptions>
; Select audio/midi flags here according to platform
; Audio out   Audio in
-odac             ;;;RT audio out
; For Non-realtime ouput leave only the line below:
;-o nreverb_ftable.wav -W ;;; for file output any platform
</CsOptions>
<CsInstruments>

sr = 44100
ksmps = 32
nchnls = 2
0dbfs  = 1

instr 1

a1  soundin "drumsMlp.wav"
a2  nreverb a1, 1.5, .75, 0, 8, 71, 4, 72
outs a1 + a2 * .4, a1 + a2 * .4

endin

</CsInstruments>
<CsScore>
; freeverb time constants, as direct (negative) sample, with arbitrary gains
f71 0 16   -2  -1116 -1188 -1277 -1356 -1422 -1491 -1557 -1617  0.8  0.79  0.78  0.77  0.76  0.75  0.74  0.73
f72 0 16   -2  -556 -441 -341 -225  0.7  0.72  0.74  0.76

i1 0 5
e
</CsScore>
</CsoundSynthesizer>
