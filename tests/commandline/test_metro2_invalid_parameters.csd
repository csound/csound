<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 64
nchnls = 1
instr 1
  kTick metro2 p4, p5, 1, p6
endin
instr 2
  iPhase = log(p4)
  kTick metro2 2, .5, 1, iPhase
endin
instr 3
  kValue init p4
  kFrequency = log(kValue)
  kTick metro2 kFrequency, .5
endin
</CsInstruments>
<CsScore>
i 1 0 .1 -1 .5 0
i 1 0 .1 2 -.1 0
i 1 0 .1 2 1.1 0
i 1 0 .1 2 .5 -1
i 2 0 .1 -1
i 2 0 .1 0
i 3 0 .1 -1
i 3 0 .1 0
e
</CsScore>
</CsoundSynthesizer>
