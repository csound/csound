<CsTest>
description = "GEN08 samples fractional segments and truncated curves at the right positions"

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
  iIndex = 0
  while iIndex < 8 do
    iActual table iIndex, p4
    if p4 == 1 then
      ; The same curve sampled four times as densely uses integer lengths.
      iExpected table 4*iIndex, 2
    else
      iTime = iIndex / 16
      iExpected = iTime*iTime*(3-2*iTime)
    endif
    if !(abs(iActual - iExpected) < .00001) then
      prints "GEN08 table %g at index %g: %g, expected %g\n", p4, iIndex, iActual, iExpected
      exitnow(-1)
    endif
    iIndex += 1
  od
  ; Interpolation exposes the extended guard point without clamping to index 7.
  iLast table 7, p4
  iBetween tablei 7.5, p4
  iGuard = (p4 == 1 ? 0 : .5)
  if !(abs(2*iBetween - iLast - iGuard) < .00001) then
    prints "GEN08 table %g has the wrong guard point\n", p4
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 9 -8 0 .5 1 .25 -.5 .25 .5 7 0
f 2 0 33 -8 0 2 1 1 -.5 1 .5 28 0
; The table ends halfway through this segment, where the curve equals .5.
f 3 0 9 -8 0 16 1
i 1 0 .01 1
i 1 0 .01 3
</CsScore>
</CsoundSynthesizer>
