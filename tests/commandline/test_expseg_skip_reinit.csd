<CsTest>
description = "A nonpositive first duration preserves exponential envelope state during reinit"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckReinit
  kSample init 0
  kSkip init 0
  ; Reinit once in the first segment and once in the final segment.
  if kSample == 2 || kSample == 6 then
    kSkip = 1
    reinit ENVELOPES
  endif
ENVELOPES:
  iFirstDuration = (i(kSkip) == 0 ? .125 : p4)
  ; Negative endpoints must work just like positive ones.
  aCurve expsega -1, iFirstDuration, -2, .125, -1
  aCurveOld expseg -1, iFirstDuration, -2, .125, -1
  kCurve expseg -1, iFirstDuration, -2, .125, -1
  aBreak expsegba -1, iFirstDuration, -2, .25, -1
  aBreakOld expsegb -1, iFirstDuration, -2, .25, -1
  kBreak expsegb -1, iFirstDuration, -2, .25, -1
  rireturn
  kAudio downsamp aCurve
  kAudioOld downsamp aCurveOld
  kAudioBreak downsamp aBreak
  kAudioBreakOld downsamp aBreakOld
  kExpected = (kSample < 4 ? -2^(kSample/4) : -2*2^(-(kSample-4)/4))
  if !(abs(kAudio-kExpected) < .00001 && abs(kAudioOld-kExpected) < .00001 && \
       abs(kCurve-kExpected) < .00001 && abs(kAudioBreak-kExpected) < .00001 && \
       abs(kAudioBreakOld-kExpected) < .00001 && abs(kBreak-kExpected) < .00001) then
    printks "reinit duration=%g sample=%g: expected %g, got %g/%g/%g/%g/%g/%g\n", \
      0, p4, kSample, kExpected, kAudio, kAudioOld, kCurve, kAudioBreak, kAudioBreakOld, kBreak
    exitnowk -1
  endif
  kSample += 1
  gkChecks += 1
endin

instr CheckResults
  if i(gkChecks) != 24 then
    prints "Envelope reinit checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i "CheckReinit" 0 .375 0
i "CheckReinit" 0 .375 -1
i "CheckResults" .375 .03125
e
</CsScore>
</CsoundSynthesizer>
