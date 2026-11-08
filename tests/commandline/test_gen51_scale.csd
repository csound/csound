<CsTest>
description = "GEN51 tuning on either side of the base key and across a wide key distance"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1

instr 1
  iScale ftgen 1, 0, 8, -51, 2, 4, 100, 3, 1, 2
  ; Unit repeat interval keeps frequencies finite at a distant base key.
  iWide ftgen 2, 0, 8, -51, 3, 1, 100, -2147483648, 1, 2, 3
  iIndex = 0
  while iIndex < 8 do
    iValue table iIndex, iScale
    iExpected = 100 * 2^(iIndex - 3)
    iWideValue table iIndex, iWide
    iWideExpected = 100 * ((iIndex + 2) % 3 + 1)
    if iValue != iExpected || iWideValue != iWideExpected then
      exitnow -1
    endif
    iIndex += 1
  od
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
