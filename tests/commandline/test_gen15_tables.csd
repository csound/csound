<CsTest>
description = "GEN15 creates both tables across a registry boundary and supports DC alone"

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
0dbfs = 1

instr 1
  iCos ftgen 1, 0, 9, -13, 1, 1, 0.5, 2, 0
  iSin ftgen 2, 0, 9, -14, 1, 1, 0, 3
  ; The table registry grows in blocks of 100 slots.
  iPair ftgen 100, 0, 9, -15, 1, 1, 0.5, 0, 2, 0, 3, 90
  if ftlen(iPair) != 8 || ftlen(iPair + 1) != 8 then
    exitnow -1
  endif
  iIndex = 0
  while iIndex < 8 do
    ; Half-step reads also check the extended guard point.
    iFirst tablei iIndex, iPair
    iSecond tablei iIndex, iPair + 1
    iExpectedFirst tablei iIndex, iCos
    iExpectedSecond tablei iIndex, iSin
    if !(abs(iFirst - iExpectedFirst) < 0.00001) || !(abs(iSecond - iExpectedSecond) < 0.00001) then
      exitnow -1
    endif
    iIndex += 0.5
  od
endin

instr 2
  iPair ftgen 200, 0, 9, -15, 1, 1, 2, 0
  iIndex = 0
  while iIndex < 8 do
    iFirst tablei iIndex, iPair
    iSecond tablei iIndex, iPair + 1
    if iFirst != 1 || iSecond != 0 then
      exitnow -1
    endif
    iIndex += 0.5
  od
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
; Also replace the pair, retaining the same table sizes.
i 1 0.02 0.01
i 2 0.04 0.01
e
</CsScore>
</CsoundSynthesizer>
