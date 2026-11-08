<CsTest>
description = "transegr reaches rounded segment boundaries and applies zero-duration jumps"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 4
nchnls = 1
0dbfs = 1
gkChecked init 0
instr 1
  ; Each rise lasts one control period or six audio samples after rounding.
  ; The zero-duration segment jumps from 2 to 3 between the rises.
  kEnvelope transegr 1, 1.6/kr, 0, 2, 0, 0, 3, 1.6/kr, 0, 4, 4/kr, 0, 0
  aEnvelope transegr 1, 1.6/kr, 0, 2, 0, 0, 3, 1.6/kr, 0, 4, 4/kr, 0, 0
  kPeriod init 0
  kReleasing release
  if kReleasing == 0 then
    kExpected = 4
    if kPeriod == 0 then
      kExpected = 1
    elseif kPeriod == 1 then
      kExpected = 3
    endif
    if !(abs(kEnvelope-kExpected) < .00001) then
      printks "Period %g: expected control %g, got %g\n", 0, kPeriod, kExpected, kEnvelope
      exitnowk -1
    endif
    kSample = 0
    while kSample < ksmps do
      kPosition = kPeriod*ksmps+kSample
      if kPosition < 6 then
        kExpected = 1+kPosition/6
      elseif kPosition < 12 then
        kExpected = 3+(kPosition-6)/6
      else
        kExpected = 4
      endif
      kActual vaget kSample, aEnvelope
      if !(abs(kActual-kExpected) < .00001) then
        printks "Sample %g: expected audio %g, got %g\n", 0, kPosition, kExpected, kActual
        exitnowk -1
      endif
      kSample += 1
    od
    if kPeriod == 7 then
      gkChecked = 1
    endif
  endif
  kPeriod += 1
endin
instr 2
  if i(gkChecked) != 1 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 [8/256]
i 2 .1 .004
e
</CsScore>
</CsoundSynthesizer>
