<CsTest>
description = "ephasor retains phase detail when the increment changes magnitude"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 32768
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr ChangingStep
 ; The first step leaves the true phase at 1-2^-30. Neither of the
 ; following half-cycle steps crosses a boundary, so no reset should occur.
 kSample init 0
 kStep = (kSample == 0 ? 1/16777216-1/1073741824 : (kSample == 1 ? -.5 : (kSample == 2 ? .5 : 0)))
 iPhase = 1-1/16777216
 aFrequency = kStep*sr
 aAmplitude, aPhase ephasor kStep*sr, .5, iPhase
 aAudioAmplitude, aAudioPhase ephasor aFrequency, .5, iPhase
 aInplaceAmplitude, aFrequency ephasor aFrequency, .5, iPhase
 kAmp downsamp aAmplitude
 kAudioAmp downsamp aAudioAmplitude
 kInplaceAmp downsamp aInplaceAmplitude
 kExpected = .5^kSample
 if kAmp != kExpected || kAudioAmp != kExpected || kInplaceAmp != kExpected then
  printks "Changing increment sample=%g amplitude=%g/%g/%g expected=%g\n", 0, kSample, kAmp, kAudioAmp, kInplaceAmp, kExpected
  exitnowk(-1)
 endif
 kSample += 1
 if kSample == 8 then
  gkChecks += 1
  turnoff
 endif
endin

instr CheckCompletion
 if i(gkChecks) != 1 then
  prints "Changing increment checks did not complete\n"
  exitnow(-1)
 endif
endin
</CsInstruments>
<CsScore>
i "ChangingStep" 0 .001
i "CheckCompletion" .002 .001
e
</CsScore>
</CsoundSynthesizer>
