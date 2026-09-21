<CsTest>
description = "voice uses all four formants and follows amplitude and pitch changes"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 32768

instr 1
  kTime timeinsts
  kAmp init .4
  kFreq init 200
  if kTime > .15 then
    kAmp = .8
  endif
  if kTime > .3 then
    kFreq = 400
  endif
  ; A constant wavetable makes the steady output independent of vibrato.
  aVoice voice kAmp * 0dbfs, kFreq, 1, 1, 0, 0, 1, 2
  kValue downsamp aVoice
  ; DC gain of the "ihh" formants at 385, 2056, 2587 and 3150 Hz,
  ; including the OneZero filter and output scale, is 3.94060865.
  kExpected = 3.94060865 * kAmp * 10000 / (1700-kFreq)^2
  if kTime > .1 && !(abs(kValue/0dbfs-kExpected) < .00005) then
    printks "voice gain: got %g, expected %g\n", 0, kValue/0dbfs, kExpected
    exitnowk(-1)
  endif
endin
</CsInstruments>
<CsScore>
f 1 0 256 -7 1 256 1
f 2 0 256 10 1
i 1 0 .45
</CsScore>
</CsoundSynthesizer>
