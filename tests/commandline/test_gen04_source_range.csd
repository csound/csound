<CsTest>
description = "GEN04 scans the full source for unequal table lengths and midpoint mode"

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
  iIndex = 0
  ; Half-sample reads also check interpolation towards the extended guard.
  while iIndex < ftlen(p4) do
    iActual tablei iIndex, p4
    iExpected tablei iIndex, p5
    if !(abs(iActual-iExpected) < .00001) then
      prints "GEN04 table %g at %g: %g, expected %g\n", p4, iIndex, iActual, iExpected
      exitnow(-1)
    endif
    iIndex += .5
  od
endin
</CsInstruments>
<CsScore>
; Left-to-right scan of ten source intervals into eight output intervals.
f 1 0 -10 -2 1 2 3 4 5 6 7 8 9 10 10
f 2 0 9 -4 1 0
f 3 0 9 -2 1 .5 [1/3] .25 [1/6] [1/7] .125 [1/9] .1
; Midpoint scans of even and odd source lengths, including both ends.
f 4 0 -10 -2 10 9 8 7 6 1 2 3 4 5 11
f 5 0 5 -4 4 1
f 6 0 5 -2 1 [1/6] [1/7] .125 [1/11]
f 7 0 -11 -2 10 9 8 7 6 1 2 3 4 5 11 12
f 8 0 5 -4 7 1
f 9 0 5 -2 1 [1/6] .125 [1/9] [1/12]
; An exact size ratio keeps the existing mapping.
f 10 0 3 -4 1 0
f 11 0 3 -2 1 [1/6] .1
i 1 0 .01 2 3
i 1 0 .01 5 6
i 1 0 .01 8 9
i 1 0 .01 10 11
</CsScore>
</CsoundSynthesizer>
