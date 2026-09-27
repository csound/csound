<CsTest>
description = "dam computes finite expansion when the level ratio is below float range"
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
gkChecked init 0
instr 1
  kPeriod init 0
  ; Fill the level window before raising the threshold.
  kThreshold = kPeriod < 32 ? 0 : 1e25
  aInput init 1e-25
  aOutput dam aInput, kThreshold, 1, 2, 0, 0
  kOutput downsamp aOutput
  if kPeriod == 33 then
    ; Level is input/sqrt(2); lower ratio 2 gives gain sqrt(threshold/level).
    ; With input*threshold = 1, the output must be the fourth root of 2.
    kExpected = pow(2, .25)
    if !(abs(kOutput-kExpected) < .000001) then
      printks "Expected finite output %g, got %g\n", 0, kExpected, kOutput
      exitnowk -1
    endif
    gkChecked = 1
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
i 1 0 .03
i 2 .04 .01
e
</CsScore>
</CsoundSynthesizer>
