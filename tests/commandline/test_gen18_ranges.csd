<CsTest>
description = "GEN18 preserves large tables and adds overlapping single-sample ranges"

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
  iSource ftgen 1, 0, 65536, -7, 0, 65536, 65536
  iCopy ftgen 2, 0, 65536, -18, iSource, 1, 0, 65535
  iIndex = 0
  while iIndex < 65536 do
    iValue table iIndex, iCopy
    if iValue != iIndex then
      exitnow -1
    endif
    iIndex += 1
  od

  iSmall ftgen 3, 0, 4, -2, 2, 4, 6, 8
  iMix ftgen 4, 0, 8, -18, iSmall, 0.5, 2, 5, iSmall, 3, 4, 4
  iExpected[] fillarray 0, 0, 1, 2, 9, 4, 0, 0
  iIndex = 0
  while iIndex < 8 do
    iValue table iIndex, iMix
    if iValue != iExpected[iIndex] then
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
