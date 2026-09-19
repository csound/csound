<CsTest>
description = "ftset writes strided values and clears slices at init time"
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

instr InitWrites
  iStrided ftgen 0, 0, -8, -2, 1, 2, 3, 4, 5, 6, 7, 8
  iCleared ftgen 0, 0, -8, -2, 1, 2, 3, 4, 5, 6, 7, 8

  ; All arguments and checks are i-rate: the writes must happen during init.
  ftset iStrided, 9, 1, 7, 2
  iExpectedStrided[] fillarray 1, 9, 3, 9, 5, 9, 7, 8

  ; A negative end leaves the last two entries unchanged.
  ftset iCleared, 0, 2, -2
  iExpectedCleared[] fillarray 1, 2, 0, 0, 0, 0, 7, 8

  iIndex = 0
  while iIndex < 8 do
    iStridedValue table iIndex, iStrided
    iClearedValue table iIndex, iCleared
    if iStridedValue != iExpectedStrided[iIndex] then
      prints "ftset init strided write failed at index %g\n", iIndex
      exitnow -1
    endif
    if iClearedValue != iExpectedCleared[iIndex] then
      prints "ftset init clear failed at index %g\n", iIndex
      exitnow -1
    endif
    iIndex += 1
  od
endin
</CsInstruments>
<CsScore>
i "InitWrites" 0 .015625
</CsScore>
</CsoundSynthesizer>
