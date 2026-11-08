<CsTest>
description = "ftslice stops copying at the source table end"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 16
nchnls = 1

instr 1
  iSource ftgen 0, 0, -4, -2, 1, 2, 3, 4
  iInitDest ftgen 0, 0, -4, -2, -1, -2, -3, -4
  iControlDest ftgen 0, 0, -4, -2, -1, -2, -3, -4
  ; Copy indices 1 and 3, leaving the rest of each destination untouched.
  ftslicei iSource, iInitDest, 1, 100, 2
  ftslice iSource, iControlDest, 1, 100, 2
  iExpected[] fillarray 2, 4, -3, -4
  kIndex = 0
  while kIndex < 4 do
    kInitValue table kIndex, iInitDest
    kControlValue table kIndex, iControlDest
    if kInitValue != iExpected[kIndex] || kControlValue != iExpected[kIndex] then
      printks "ftslice copied past the source end or changed the wrong entries\n", 0
      exitnowk -1
    endif
    kIndex += 1
  od
endin
</CsInstruments>
<CsScore>
i 1 0 .015625
e
</CsScore>
</CsoundSynthesizer>
