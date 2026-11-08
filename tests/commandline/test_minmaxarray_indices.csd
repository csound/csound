<CsTest>
description = "minarray and maxarray keep the first matching index and use the current array size"
[expect]
exit = 0
</CsTest>

<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
instr 1
  iValues[][] init 2, 3
  iValues fillarray 3, -2, -2, 8, 8, 4
  iMin, iMinIndex minarray iValues
  iMax, iMaxIndex maxarray iValues
  if iMin != -2 || iMinIndex != 1 || iMax != 8 || iMaxIndex != 3 then
    exitnow -1
  endif
  kValues[] fillarray 3, -2, -2, 8, 8, 4
  kCycle init 0
  kLength = (kCycle == 0 ? 6 : 1)
  trim kValues, kLength
  kMin, kMinIndex minarray kValues
  kMax, kMaxIndex maxarray kValues
  if kCycle == 0 then
    if kMin != -2 || kMinIndex != 1 || kMax != 8 || kMaxIndex != 3 then
      exitnowk -1
    endif
  else
    if kMin != 3 || kMax != 3 || kMinIndex != 0 || kMaxIndex != 0 then
      exitnowk -1
    endif
    turnoff
  endif
  kCycle += 1
endin
</CsInstruments>
<CsScore>
i 1 0 .1
e
</CsScore>
</CsoundSynthesizer>
