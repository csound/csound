<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 64
nchnls = 1
0dbfs = 1
instr 1
 aInput = 0
 aLoop, kRec sndloop aInput, 1, 1, p4/sr, p5/sr
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0 0
i 1 0 .01 -1 0
i 1 0 .01 .5 0
i 1 0 .01 1e30 0
i 1 0 .01 8 -1
i 1 0 .01 8 1e30
i 1 0 .01 8 9
; The recording count includes both the loop and the crossfade.
i 1 0 .01 1073741824 1073741824
e
</CsScore>
</CsoundSynthesizer>
