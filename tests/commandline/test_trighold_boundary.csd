<CsTest>
description = "trighold accepts triggers at expiry and preserves active holds"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 1
nchnls = 1
0dbfs = 1
gkChecks init 0

instr CheckPulseTrain
  iDurationSamples = p4
  iHoldSamples = p5
  ; Leave a zero input between triggers, even for a one-sample hold.
  iPeriodSamples = max(2, iHoldSamples)
  kSample init 0
  kAge = kSample % iPeriodSamples
  kPulseLevel = .25 + floor(kSample/iPeriodSamples)*.25
  kTrigger = kAge == 0 ? kPulseLevel : 0
  aTrigger = kTrigger
  kHeld trighold kTrigger, iDurationSamples/kr
  aHeld trighold aTrigger, iDurationSamples/sr
  kAudioHeld downsamp aHeld

  kExpected = kAge < iHoldSamples ? kPulseLevel : 0
  if kHeld != kExpected || kAudioHeld != kExpected then
    printks "trighold duration=%g sample=%g expected=%g control=%g audio=%g\n", \
      0, iDurationSamples, kSample, kExpected, kHeld, kAudioHeld
    exitnowk(-1)
  endif
  kSample += 1
  if kSample == 16 then
    gkChecks += 1
  endif
endin

instr CheckActiveHold
  ; The trigger at sample 2 occurs during a four-sample hold. Ignore it.
  ; Staying positive through expiry must not create another trigger.
  iTriggers[] fillarray 1, 0, 2, 3, 3, 3, 0, 4, 0, 0, 0, 0
  iExpected[] fillarray 1, 1, 1, 1, 0, 0, 0, 4, 4, 4, 4, 0
  kSample init 0
  kTrigger = iTriggers[kSample]
  aTrigger = kTrigger
  kHeld trighold kTrigger, 4/kr
  aHeld trighold aTrigger, 4/sr
  kAudioHeld downsamp aHeld
  kExpected = iExpected[kSample]
  if kHeld != kExpected || kAudioHeld != kExpected then
    printks "trighold changed an active hold at sample %g: expected=%g control=%g audio=%g\n", \
      0, kSample, kExpected, kHeld, kAudioHeld
    exitnowk(-1)
  endif
  kSample += 1
  if kSample == 12 then
    gkChecks += 1
  endif
endin

instr CheckDurationChange
  ; Changing kdur cannot shorten an active hold. The trigger at sample 3
  ; starts a new four-sample hold as soon as the first hold expires.
  iTriggers[] fillarray 1, 0, 0, 2, 0, 0, 0, 0
  iDurations[] fillarray 3, 1, 1, 4, 0, 0, 0, 0
  iExpected[] fillarray 1, 1, 1, 2, 2, 2, 2, 0
  kSample init 0
  kTrigger = iTriggers[kSample]
  aTrigger = kTrigger
  kDurationSamples = iDurations[kSample]
  kHeld trighold kTrigger, kDurationSamples/kr
  aHeld trighold aTrigger, kDurationSamples/sr
  kAudioHeld downsamp aHeld
  kExpected = iExpected[kSample]
  if kHeld != kExpected || kAudioHeld != kExpected then
    printks "trighold duration change at sample %g: expected=%g control=%g audio=%g\n", \
      0, kSample, kExpected, kHeld, kAudioHeld
    exitnowk(-1)
  endif
  kSample += 1
  if kSample == 8 then
    gkChecks += 1
  endif
endin

instr CheckLongHold
  ; A hold longer than 2^32 samples must retain its counter across calls,
  ; including on platforms where long is only 32 bits.
  kSample init 0
  kTrigger = kSample == 0 ? .5 : 0
  aTrigger = kTrigger
  kHeld trighold kTrigger, 4294967297/kr
  aHeld trighold aTrigger, 4294967297/sr
  kAudioHeld downsamp aHeld
  if kHeld != .5 || kAudioHeld != .5 then
    printks "trighold lost a long hold at sample %g\n", 0, kSample
    exitnowk(-1)
  endif
  kSample += 1
  if kSample == 8 then
    gkChecks += 1
  endif
endin

instr CheckCoverage
  if i(gkChecks) != 10 then
    prints "trighold completed %g cases; expected 10\n", i(gkChecks)
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
; Two-, three-, and four-sample holds must accept each adjacent pulse.
;                                duration  expected hold (samples)
i "CheckPulseTrain" 0 [16/8192] 2  2
i "CheckPulseTrain" 0 [16/8192] 3  3
i "CheckPulseTrain" 0 [16/8192] 4  4
; Fractional durations round to the nearest sample, with .5 rounded up.
i "CheckPulseTrain" 0 [16/8192] 2.49  2
i "CheckPulseTrain" 0 [16/8192] 2.5   3
; Zero and sub-sample durations still emit a one-sample pulse.
i "CheckPulseTrain" 0 [16/8192] 0   1
i "CheckPulseTrain" 0 [16/8192] .4  1

i "CheckActiveHold" 0 [12/8192]
i "CheckDurationChange" 0 [8/8192]
i "CheckLongHold" 0 [8/8192]
i "CheckCoverage" [32/8192] [1/8192]
e
</CsScore>
</CsoundSynthesizer>
