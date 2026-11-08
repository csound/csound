<CsTest>
description = "cossegr emits its release endpoint before the note ends"
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
gkLastControl init -1
gkLastAudio init -1
gkReleasePeriods init 0

instr Envelope
  ; Rise from 0 to 1, hold until note-off, then release to 0.
  ; p4 is the release length in control periods. p5 adds a shared tail.
  gkReleasePeriods = 0
  xtratim p5/kr
  if p6 == 0 then
    kEnvelope cossegr 0, 4/kr, 1, p4/kr, 0
    aEnvelope cossegr 0, 4/kr, 1, p4/kr, 0
  else
    ; With only a release segment, hold the start value until note-off.
    kEnvelope cossegr 1, p4/kr, 0
    aEnvelope cossegr 1, p4/kr, 0
  endif
  gkLastControl = kEnvelope
  gkLastAudio downsamp aEnvelope

  kReleasing release
  kPeriods init 0
  kPrevious init 0
  kPreviousAudio init 0
  kStart init 0
  kStartAudio init 0
  if kReleasing == 1 then
    if kPeriods == 0 then
      kStart = kPrevious
      kStartAudio = kPreviousAudio
    endif
    ; Another extender may keep the note alive, but must not slow the curve.
    iSteps = int(p4 + .5)
    kExpected = 0
    if iSteps > 0 then
      kExpected = kStart * (1 + cos($M_PI * min(kPeriods/iSteps, 1))) / 2
    endif
    if !(abs(kEnvelope-kExpected) < .00001) then
      printks "Release period %g: expected %g, got %g\n", 0, kPeriods, kExpected, kEnvelope
      exitnowk -1
    endif
    kSample = 0
    while kSample < ksmps do
      kExpectedAudio = 0
      if iSteps > 0 then
        kPosition = min((kPeriods*ksmps+kSample)/(iSteps*ksmps), 1)
        kExpectedAudio = kStartAudio * (1 + cos($M_PI*kPosition)) / 2
      endif
      kActualAudio vaget kSample, aEnvelope
      if !(abs(kActualAudio-kExpectedAudio) < .00001) then
        printks "Release period %g, sample %g: expected %g, got %g\n", 0, kPeriods, kSample, kExpectedAudio, kActualAudio
        exitnowk -1
      endif
      kSample += 1
    od
    kPeriods += 1
    gkReleasePeriods = kPeriods
  endif
  kPrevious = kEnvelope
  kPreviousAudio vaget ksmps-1, aEnvelope
endin

instr CheckLastOutput
  if !(abs(i(gkLastControl)) < .00001 && abs(i(gkLastAudio)) < .00001) then
    prints "Release must end at 0: last control=%g, last audio=%g\n", i(gkLastControl), i(gkLastAudio)
    exitnow -1
  endif
  if i(gkReleasePeriods) != p4 then
    prints "Expected %g release periods, got %g\n", p4, i(gkReleasePeriods)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Four-step release needs a fifth period to emit its endpoint.
i "Envelope" 0 [32/1024] 4 0
i "CheckLastOutput" .1 .004 5
; Zero and sub-half-period releases still need to emit the endpoint.
i "Envelope" .2 [32/1024] 0 0
i "CheckLastOutput" .3 .004 1
i "Envelope" .4 [32/1024] .4 0
i "CheckLastOutput" .5 .004 1
; Round to the nearest control period.
i "Envelope" .6 [32/1024] .6 0
i "CheckLastOutput" .7 .004 2
; An equal shared tail still needs the endpoint period.
i "Envelope" .8 [32/1024] 4 4
i "CheckLastOutput" .9 .004 5
; A longer shared tail must not change the release curve.
i "Envelope" 1 [32/1024] 4 8
i "CheckLastOutput" 1.1 .004 8
; Note-off during the rise releases from the last output.
i "Envelope" 1.2 [8/1024] 4 0
i "CheckLastOutput" 1.3 .004 5
; The same rounding applies when note-off interrupts the rise.
i "Envelope" 1.4 [8/1024] .6 0
i "CheckLastOutput" 1.5 .004 2
; A single release segment also uses whole control periods at audio rate.
i "Envelope" 1.6 [32/1024] .6 0 1
i "CheckLastOutput" 1.7 .004 2
e
</CsScore>
</CsoundSynthesizer>
