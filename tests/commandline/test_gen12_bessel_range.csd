<CsTest>
description = "GEN12 evaluates log(I0) across small and large arguments"

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
  iTable ftgen 0, 0, 9, -12, 2*p4
  iActual table 4, iTable
  if !(abs(iActual-p5) < 1e-30 + 1e-6*abs(p5)) then
    prints "GEN12 at %g: %.12g, expected %.12g\n", p4, iActual, p5
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Reference values from the defining I0 power series, computed at 60 digits.
i 1 0 .01 20 17.589610428244274
i 1 0 .01 40 37.239786861352357
i 1 0 .01 1000 995.627308889869465
i 1 0 .01 0 0
i 1 0 .01 1e-10 2.5e-21
i 1 0 .01 1 .235914358507178649
i 1 0 .01 3.749 2.209501063811140
i 1 0 .01 3.75 2.210354211972019
i 1 0 .01 3.751 2.211207404720945
i 1 0 .01 -20 17.589610428244274
</CsScore>
</CsoundSynthesizer>
