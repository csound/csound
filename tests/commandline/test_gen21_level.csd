<CsTest>
description = "GEN21 preserves uniform levels and Poisson counts for either GEN sign"

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
  seed 12345
  iPositive ftgen 1, 0, p6, 21, p4, p5
  seed 12345
  iNegative ftgen 2, 0, p6, -21, p4, p5
  iIndex = 0
  while iIndex < 16 do
    ; Half-step reads also compare the guard point.
    iPositiveValue tablei iIndex, iPositive
    iNegativeValue tablei iIndex, iNegative
    if iPositiveValue != iNegativeValue then
      exitnow -1
    endif
    iValue table int(iIndex), iPositive
    if p4 == 1 && !(iValue >= 0 && iValue <= p5) then
      exitnow -1
    elseif p4 == 11 && iValue != int(iValue) then
      exitnow -1
    endif
    iIndex += 0.5
  od
endin
</CsInstruments>
<CsScore>
i 1 0 0.01 1 0.25 16
i 1 0.02 0.01 11 4 17
e
</CsScore>
</CsoundSynthesizer>
