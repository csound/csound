<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 1
nchnls = 1
0dbfs = 1
instr 1
  asyncin = 0
  kcps = exp(1000)
  aphase, async syncphasor kcps, asyncin
endin
instr 2
  asyncin = 0
  kbad = exp(1000)
  acps = kbad
  aphase, async syncphasor acps, asyncin
endin
</CsInstruments>
<CsScore>
i 1 0 .001
i 2 .002 .001
</CsScore>
</CsoundSynthesizer>
