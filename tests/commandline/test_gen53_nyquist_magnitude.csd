<CsTest>
description = "GEN53 uses Nyquist magnitude for linear- and minimum-phase output"

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
  iResult ftgen 0, 0, 8, -53, p4, p5
  iPeak = (p5 % 2 == 0 ? 4 : 0)
  iIndex = 0
  while iIndex < 8 do
    iActual table iIndex, iResult
    iExpected = (iIndex == iPeak ? 1 : 0)
    if !(abs(iActual - iExpected) < .00001) then
      prints "GEN53 source %g mode %g at index %g: %g, expected %g\n", p4, p5, iIndex, iActual, iExpected
      exitnow(-1)
    endif
    iIndex += 1
  od
endin
</CsInstruments>
<CsScore>
; All three impulses have the same flat magnitude response.
f 1 0 8 -2 1 0 0 0 0 0 0 0
f 2 0 8 -2 -1 0 0 0 0 0 0 0
f 3 0 8 -2 0 1 0 0 0 0 0 0
; An explicit amplitude response with negative Nyquist (extended guard point).
f 4 0 5 -2 1 1 1 1 -1
; Existing mode bits: 2 = impulse input, 1 = minimum-phase output.
i 1 0 .01 1 2
i 1 0 .01 2 2
i 1 0 .01 3 2
i 1 0 .01 4 0
i 1 0 .01 2 3
i 1 0 .01 4 1
</CsScore>
</CsoundSynthesizer>
