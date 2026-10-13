<CsTest>
description = "GEN40 preserves histogram weights and skips empty bins"

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
  iResult ftgen 0, 0, 8, -40, p4
  iIndex = 0
  while iIndex < 8 do
    iActual table iIndex, iResult
    iExpected table iIndex, p5
    if iActual != iExpected then
      prints "GEN40 source %g at index %g: %g, expected %g\n", p4, iIndex, iActual, iExpected
      exitnow(-1)
    endif
    iIndex += 1
  od
endin
</CsInstruments>
<CsScore>
; Equal weights must give equal numbers of entries for all four bins.
f 1 0 4 -2 1 1 1 1
f 2 0 8 -2 0 0 1 1 2 2 3 3
; Leading, interior and trailing empty bins must not receive entries.
f 3 0 8 -2 0 1 0 3 0 0 0 0
f 4 0 8 -2 1 1 3 3 3 3 3 3
i 1 0 .01 1 2
i 1 0 .01 3 4
</CsScore>
</CsoundSynthesizer>
