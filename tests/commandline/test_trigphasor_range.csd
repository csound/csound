<CsTest>
description = "trigphasor starts, wraps, and resets within its range"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8000
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

; All four signatures must wrap ordinary steps and steps larger than the range.
instr CheckWrapping
  setksmps 1
  kCycle init 0
  kCycle += 1
  kStart = p4
  kEnd = p5
  iChangeRange = p7
  if iChangeRange == 1 && kCycle >= 4 then
    kStart = -3
    kEnd = 2
  endif
  kRate = p6
  aRate = kRate
  kTrigger = 0
  aTrigger = 0
  kControlPhase trigphasor kTrigger, kRate, kStart, kEnd
  aControlTriggerPhase trigphasor kTrigger, kRate, kStart, kEnd
  aAudioTriggerPhase trigphasor aTrigger, kRate, kStart, kEnd
  aAudioRatePhase trigphasor aTrigger, aRate, kStart, kEnd
  kControlTriggerPhase downsamp aControlTriggerPhase
  kAudioTriggerPhase downsamp aAudioTriggerPhase
  kAudioRatePhase downsamp aAudioRatePhase
  kLevel init 0
  if kCycle == 1 then
    kLevel = kStart
  endif
  kRange = kEnd - kStart
  kExpected = kStart
  if kRange != 0 then
    kExpected = kLevel - kRange * floor((kLevel - kStart) / kRange)
  endif
  kLevel = kExpected + kRate
  kError = abs(kControlPhase - kExpected) + abs(kControlTriggerPhase - kExpected)
  kError += abs(kAudioTriggerPhase - kExpected) + abs(kAudioRatePhase - kExpected)
  if !(kError <= .00001) then
    printks "trigphasor range [%g,%g), step %g, sample %d: error %g\n", 0, kStart, kEnd, kRate, kCycle, kError
    exitnowk(-1)
  endif
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

; A control trigger resets exactly to the explicit reset phase or range start.
instr CheckControlReset
  setksmps 1
  kCycle init 0
  kCycle += 1
  kTrigger = kCycle == 3 ? 1 : -1
  kStart = -2
  kEnd = 5
  kDefault trigphasor kTrigger, .25, kStart, kEnd
  kExplicit trigphasor kTrigger, .25, kStart, kEnd, 0
  aDefault trigphasor kTrigger, .25, kStart, kEnd
  aExplicit trigphasor kTrigger, .25, kStart, kEnd, 0
  kAudioDefault downsamp aDefault
  kAudioExplicit downsamp aExplicit
  kExpectedDefault = -2 + .25 * (kCycle - 1)
  kExpectedExplicit = kExpectedDefault
  if kCycle >= 3 then
    kExpectedDefault = -2 + .25 * (kCycle - 3)
    kExpectedExplicit = .25 * (kCycle - 3)
  endif
  kError = abs(kDefault - kExpectedDefault) + abs(kAudioDefault - kExpectedDefault)
  kError += abs(kExplicit - kExpectedExplicit) + abs(kAudioExplicit - kExpectedExplicit)
  if !(kError <= .00001) then
    printks "trigphasor control trigger did not reset to the requested position\n", 0
    exitnowk(-1)
  endif
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

; An audio crossing can fall between samples; wrap its interpolated reset.
instr CheckAudioReset
  setksmps 1
  kCycle init 0
  kCycle += 1
  aTrigger = kCycle == 3 ? 1 : -1
  aRate = .25
  aAudioTriggerPhase trigphasor aTrigger, .25, -2, 5, 5
  aAudioRatePhase trigphasor aTrigger, aRate, -2, 5, 5
  aDefault trigphasor aTrigger, aRate, -2, 5
  kAudioTriggerPhase downsamp aAudioTriggerPhase
  kAudioRatePhase downsamp aAudioRatePhase
  kDefault downsamp aDefault
  kExpected = -2 + .25 * (kCycle - 1)
  if kCycle >= 3 then
    ; The crossing is halfway through the previous sample interval.
    kExpected = -2 + .125 + .25 * (kCycle - 3)
  endif
  kError = abs(kAudioTriggerPhase - kExpected) + abs(kAudioRatePhase - kExpected) + abs(kDefault - kExpected)
  if !(kError <= .00001) then
    printks "trigphasor audio reset escaped the range or used the wrong crossing time\n", 0
    exitnowk(-1)
  endif
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

; Inactive samples stay zero, even with a nonzero range start.
instr CheckPartialBlock
  kStart = 10
  kEnd = p4
  kRate = p5
  kTrigger = 1
  aTrigger = 1
  aRate = kRate
  aControlTriggerPhase trigphasor kTrigger, kRate, kStart, kEnd
  aAudioTriggerPhase trigphasor aTrigger, kRate, kStart, kEnd
  aAudioRatePhase trigphasor aTrigger, aRate, kStart, kEnd
  aReference = 10
  aError = abs(aControlTriggerPhase - aReference) + abs(aAudioTriggerPhase - aReference) + abs(aAudioRatePhase - aReference)
  kError max_k aError, 1, 1
  if !(kError <= .00001) then
    printks "trigphasor partial block did not use the active start value\n", 0
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr CheckCoverage
  if i(gkChecks) != 10 then
    prints "trigphasor checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
;                                      start end   step change-range
; Forward/reverse steps, including steps larger than the seven-unit range.
i "CheckWrapping" 0 [8/8000]              10  17     .25  0
i "CheckWrapping" 0 [8/8000]              10  17    -.25  0
i "CheckWrapping" 0 [8/8000]              10  17   20     0
i "CheckWrapping" 0 [8/8000]              10  17  -20     0
; Equal endpoints hold one value; moving endpoints change the wrapping range.
i "CheckWrapping" 0 [8/8000]               2   2    1     0
i "CheckWrapping" 0 [8/8000]              10  17    1     1

i "CheckControlReset" [80/8000]  [8/8000]
i "CheckAudioReset"   [160/8000] [8/8000]

; Notes start five samples into a block. Check a stationary phase and a
; collapsed range; both should output 10 only during the note.
;                                                    end step
i "CheckPartialBlock" [(240+5)/8000] [24/8000]          17  0
i "CheckPartialBlock" [(320+5)/8000] [24/8000]          10  1

i "CheckCoverage" [400/8000] [16/8000]
e
</CsScore>
</CsoundSynthesizer>
