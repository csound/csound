<CsTest>
description = "padsynth produces the requested frequency and preserves silent tables"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  iTab ftgen 0, 0, p7, "padsynth", p6, .3, 0, 1, p4, .5, p5
  iPeak = 0
  iIndex = 0
  iCoefficient = 2*cos(2*$M_PI*p6/sr)
  while iIndex < ftlen(iTab) do
    iValue table iIndex, iTab
    if !(abs(iValue) <= 1.00001) then
      prints "padsynth produced an invalid sample\n"
      exitnow(-1)
    endif
    iPeak = max(iPeak, abs(iValue))
    ; These narrow profiles occupy one FFT bin, so the result is a sine.
    if p8 == 1 && iIndex < ftlen(iTab)-2 then
      iNext table iIndex+1, iTab
      iAfter table iIndex+2, iTab
      if !(abs(iAfter-iCoefficient*iNext+iValue) < .00001) then
        prints "padsynth produced the wrong frequency\n"
        exitnow(-1)
      endif
    endif
    iIndex += 1
  od
  iFirst table 0, iTab
  iLast table ftlen(iTab)-1, iTab
  iBetween tablei ftlen(iTab)-.5, iTab
  if !(abs(iPeak-p8) < .00001) || !(abs(2*iBetween-iLast-iFirst) < .00001) then
    prints "padsynth peak or guard point is wrong\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Profile, amplitude, frequency, size, expected peak.
i 1 0 .01 1 1 512 1024 1
i 1 0 .01 2 1 512 1024 1
i 1 0 .01 3 1 512 1025 1
; Zero amplitudes and a square profile between FFT bins must stay silent.
i 1 0 .01 1 0 512 1024 0
i 1 0 .01 2 1 444 1024 0
</CsScore>
</CsoundSynthesizer>
