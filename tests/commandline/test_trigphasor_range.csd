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

instr 1
  setksmps 1
  kCycle init 0
  kCycle += 1
  kStart = p4
  kEnd = p5
  if p7 == 1 && kCycle >= 4 then
    kStart = -3
    kEnd = 2
  endif
  kRate = p6
  aRate = kRate
  kTrigger = 0
  aTrigger = 0
  kOut trigphasor kTrigger, kRate, kStart, kEnd
  aKK trigphasor kTrigger, kRate, kStart, kEnd
  aAK trigphasor aTrigger, kRate, kStart, kEnd
  aAA trigphasor aTrigger, aRate, kStart, kEnd
  kKK downsamp aKK
  kAK downsamp aAK
  kAA downsamp aAA
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
  kError = abs(kOut - kExpected) + abs(kKK - kExpected)
  kError += abs(kAK - kExpected) + abs(kAA - kExpected)
  if !(kError <= .00001) then
    printks "trigphasor range [%g,%g), step %g, sample %d: error %g\n", 0, kStart, kEnd, kRate, kCycle, kError
    exitnowk(-1)
  endif
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 2
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
  kADefault downsamp aDefault
  kAExplicit downsamp aExplicit
  kExpectedDefault = -2 + .25 * (kCycle - 1)
  kExpectedExplicit = kExpectedDefault
  if kCycle >= 3 then
    kExpectedDefault = -2 + .25 * (kCycle - 3)
    kExpectedExplicit = .25 * (kCycle - 3)
  endif
  kError = abs(kDefault - kExpectedDefault) + abs(kADefault - kExpectedDefault)
  kError += abs(kExplicit - kExpectedExplicit) + abs(kAExplicit - kExpectedExplicit)
  if !(kError <= .00001) then
    printks "trigphasor control trigger did not reset to the requested position\n", 0
    exitnowk(-1)
  endif
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 3
  setksmps 1
  kCycle init 0
  kCycle += 1
  aTrigger = kCycle == 3 ? 1 : -1
  aRate = .25
  aAK trigphasor aTrigger, .25, -2, 5, 5
  aAA trigphasor aTrigger, aRate, -2, 5, 5
  aDefault trigphasor aTrigger, aRate, -2, 5
  kAK downsamp aAK
  kAA downsamp aAA
  kDefault downsamp aDefault
  kExpected = -2 + .25 * (kCycle - 1)
  if kCycle >= 3 then
    ; Preserve the existing audio-trigger interpolation, then wrap the result.
    kExpected = -2 + .375 + .25 * (kCycle - 3)
  endif
  kError = abs(kAK - kExpected) + abs(kAA - kExpected) + abs(kDefault - kExpected)
  if !(kError <= .00001) then
    printks "trigphasor audio reset escaped the range or changed interpolation\n", 0
    exitnowk(-1)
  endif
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 4
  kStart = 10
  kEnd = p4
  kTrigger = 1
  aTrigger = 1
  aRate = p5
  aKK trigphasor kTrigger, p5, kStart, kEnd
  aAK trigphasor aTrigger, p5, kStart, kEnd
  aAA trigphasor aTrigger, aRate, kStart, kEnd
  aReference = 10
  aError = abs(aKK - aReference) + abs(aAK - aReference) + abs(aAA - aReference)
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

instr 99
  if i(gkChecks) != 10 then
    prints "trigphasor checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .001 10 17 .25 0
i 1 0 .001 10 17 -.25 0
i 1 0 .001 10 17 20 0
i 1 0 .001 10 17 -20 0
i 1 0 .001 2 2 1 0
i 1 0 .001 10 17 1 1
i 2 .01 .001
i 3 .02 .001
i 4 .030625 .003 17 0
i 4 .040625 .003 10 1
i 99 .05 .002
</CsScore>
</CsoundSynthesizer>
