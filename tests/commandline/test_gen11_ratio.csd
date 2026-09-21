<CsTest>
description = "GEN11 preserves cosine partials near positive and negative unit ratios"

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
  ; With one partial the ratio cannot change the cosine waveform.
  iIndex = 0
  while iIndex < 1024 do
    iValue table iIndex, p4
    if !(abs(iValue - cos(2 * $M_PI * iIndex / 1024)) < .00001) then
      prints "GEN11 cosine differs at index %g: %g\n", iIndex, iValue
      exitnow(-1)
    endif
    iIndex += 1
  od
endin

instr 2
  ; Compare the closed form against the stated cosine series.
  iSum = 0
  iWeight = 0
  iAmp = 1
  iPartial = 0
  while iPartial < 1000 do
    iSum += iAmp * cos(2 * $M_PI * p4 / 1024 * (2 + iPartial))
    iWeight += iAmp
    iAmp *= .9995
    iPartial += 1
  od
  iValue table p4, 2
  if !(abs(iValue - iSum / iWeight) < .00001) then
    prints "GEN11 weighted sum differs at index %g: %g, expected %g\n", p4, iValue, iSum / iWeight
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 1024 -11 1 1 -1
f 2 0 1024 -11 1000 2 .9995
f 3 0 1024 -11 1 1 1
i 1 0 .01 1
i 1 0 .01 3
; Cover both the small-denominator fallback and the ordinary quotient.
i 2 0 .01 1
i 2 0 .01 128
</CsScore>
</CsoundSynthesizer>
