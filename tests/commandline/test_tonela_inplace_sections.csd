<CsTest>
description = "tonela and atonela support in-place operation across score sections"

[expect]
exit = 0
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
  aSig oscili .5, 321
  aSig tonela aSig, 700
  out aSig
endin

instr 2
  aSig oscili .5, 321
  aSig atonela aSig, 700
  out aSig
endin
</CsInstruments>
<CsScore>
i 1 0 .01
i 2 0 .01
s
i 1 0 .01
i 2 0 .01
e
</CsScore>
</CsoundSynthesizer>
