<CsTest>
description = "GEN43 averages selected channels and retains the Nyquist bin"

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
0dbfs = 1
instr 1
  if p4 == 1 then
    SFile = "test_pvsdiskin_single.pvx"
    iBase = .125
  else
    SFile = "test_pvsdiskin_stereo.pvx"
    ; Eight frames: amplitude = channel/8 + frame/64 + bin/1024.
    iBase = (p5 == 0 ? 1.5 : p5)/8 + 3.5/64
  endif
  iTable ftgen 0, 0, p6, p7, SFile, p5
  iBin = 0
  while iBin <= 32 do
    if iBin < ftlen(iTable) then
      iActual table iBin, iTable
    else
      iLast table iBin-1, iTable
      iBetween tablei iBin-.5, iTable
      iActual = 2*iBetween-iLast
    endif
    iExpected = iBase + iBin/1024
    if p7 > 0 then
      iExpected /= iBase + 32/1024
    endif
    if !(abs(iActual-iExpected) < .000001) then
      prints "GEN43 channel %g bin %g: %g, expected %g\n", p5, iBin, iActual, iExpected
      exitnow(-1)
    endif
    iBin += 1
  od
endin
</CsInstruments>
<CsScore>
; Fixture channel count, selected channel, table size, GEN number.
i 1 0 .01 2 0 32 -43
i 1 0 .01 2 1 32 -43
i 1 0 .01 2 2 33 -43
i 1 0 .01 1 1 32 -43
i 1 0 .01 2 0 32 43
</CsScore>
</CsoundSynthesizer>
