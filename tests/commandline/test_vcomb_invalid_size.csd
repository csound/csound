<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
instr 1
  aInput = 0
  aOutput vcomb aInput, .1, 1, p4, 0, p5
endin
instr 2
  aInput = 0
  aOutput valpass aInput, .1, 1, p4, 0, p5
endin
</CsInstruments>
<CsScore>
i 1 0 .01 0 1
i 1 0 .01 -.1 0
i 1 0 .01 .5 1
i 1 0 .01 1e20 0
i 2 0 .01 0 0
i 2 0 .01 -1 1
i 2 0 .01 .5 1
i 2 0 .01 1e20 1
e
</CsScore>
</CsoundSynthesizer>
