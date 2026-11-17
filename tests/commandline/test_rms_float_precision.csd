<CsTest>
description = "rms retains low-cutoff decay and finite large input levels"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0
instr Detector
  aInput init p5
  kLevel rms aInput, p4
  kSample init 0
  kSample += 1
  if kSample == sr then
    ; p6 is the closed-form detector level after 48000 samples.
    if p6 == 0 then
      kGood = kLevel == 0 ? 1 : 0
    else
      kGood = abs(kLevel/p6-1) < .00001 ? 1 : 0
    endif
    if kGood == 0 then
      printks "rms cutoff %g, input %g: expected %g, got %g\n", 0, p4, p5, p6, kLevel
      exitnowk -1
    endif
    gkChecks += 1
  endif
endin
instr LevelChanges
  kSample init 0
  aInput = (kSample < 32 ? 1e20 : (kSample < 160 ? 0 : 1e-20))
  kLevel rms aInput, sr/2
  if kSample == 79 || kSample == 159 || kSample == 199 then
    kExpected = (kSample == 79 ? p4 : (kSample == 159 ? p5 : 1e-20))
    if !(abs(kLevel/kExpected-1) < .00001) then
      printks "rms level change sample %g: expected %g, got %g\n", 0, kSample, kExpected, kLevel
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin
instr KeepHistory
  aInput init 1
  kReference rms aInput, .0001
  kSample init 0
  if kSample == 128 then
    reinit DETECTOR
  endif
DETECTOR:
  kLevel rms aInput, .0001, 1
  rireturn
  if kSample == 256 then
    if !(abs(kLevel/kReference-1) < .00001) then
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin
instr CheckCompletion
  if i(gkChecks) != 11 then
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Reference: level*sqrt(1-(1-c1)^48000), where
; c1=2*s/(sqrt(1+s*s)+s), s=abs(sin(pi*cutoff/sr)).
i "Detector" 0 1.01 0.0001 1 0.025062345859179779
i "Detector" 0 1.01 10 1 1
i "Detector" 0 1.01 24000 1 1
i "Detector" 0 1.01 0 1 0
i "Detector" 0 1.01 10 1e+20 1e+20
i "Detector" 0 1.01 10 1e-30 1e-30
i "Detector" 0 1.01 .0001 1e20 2.5062345859179779e18
i "LevelChanges" 0 .01 42.3415842754171364407182576705407443948169381061396688738282 1.01090949747928544074290314862888593751058988742467667752788E-29
i "KeepHistory" 0 .01
i "CheckCompletion" 1.02 .01
e
</CsScore>
</CsoundSynthesizer>
