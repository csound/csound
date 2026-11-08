<CsTest>
description = "lowres filters reject zero resonance before computing coefficients"
[expect]
exit = "nonzero"
stderr = ["lowres: cutoff and resonance must be positive"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1
instr 1
  aInput = .1
  aOut lowres aInput, p4, p5
endin
instr 2
  aInput = .1
  aResonance = 0
  aOut lowresx aInput, 1000, aResonance, 2
endin
instr 3
  aInput = .1
  aOut vlowres aInput, 1000, 0, 2, .5
endin
</CsInstruments>
<CsScore>
i 1 0 .01 1000 0
i 1 0 .01 0 0
i 2 0 .01
i 3 0 .01
</CsScore>
</CsoundSynthesizer>
