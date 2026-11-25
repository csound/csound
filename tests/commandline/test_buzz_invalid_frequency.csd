<CsTest>
description = "buzz rejects nonfinite control and audio frequencies"

[expect]
exit = "nonzero"
stderr_regex = ["(?s)(?:buzz: invalid frequency.*){2}"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1
0dbfs = 1
giPower ftgen 1, 0, 128, 10, 1
giOther ftgen 2, 0, -96, 10, 1

instr 1
  iBad strtod "nan"
  aOut buzz 1, iBad, 3, giOther, .25
endin

instr 2
  iBad strtod "inf"
  aFrequency = iBad
  aOut buzz 1, aFrequency, 3, giPower, .25
endin
</CsInstruments>
<CsScore>
i 1 0 .015625
i 2 .03125 .015625
</CsScore>
</CsoundSynthesizer>
