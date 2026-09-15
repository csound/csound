<CsTest>
description = "reject empty Chebyshev coefficients"

[expect]
exit = "nonzero"
stderr = ["chebyshevpoly2: coefficients must be a non-empty one-dimensional array"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 8
nchnls = 1
0dbfs = 1

instr 1
  ain = .25
  kcoeff[] init 0
  aout chebyshevpoly2 ain, kcoeff
endin
</CsInstruments>
<CsScore>
i 1 0 .01
</CsScore>
</CsoundSynthesizer>
