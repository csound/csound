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
  kTrigger = kCycle == 3 || kCycle == 9 ? 1 : 0
  kL triglinseg kTrigger, 0, 4/sr, 1, 16/sr, 0
  aL triglinseg kTrigger, 0, 4/sr, 1, 16/sr, 0
  kE trigexpseg kTrigger, 1, 4/sr, 2, 16/sr, 1
  aE trigexpseg kTrigger, 1, 4/sr, 2, 16/sr, 1
  kN trigexpseg kTrigger, -1, 4/sr, -2, 16/sr, -1
  aN trigexpseg kTrigger, -1, 4/sr, -2, 16/sr, -1
  kAL downsamp aL
  kAE downsamp aE
  kAN downsamp aN
  kAge = kCycle < 3 ? 0 : (kCycle < 9 ? kCycle - 2 : kCycle - 8)
  kExpected = kAge <= 4 ? kAge/4 : max(0, 1 - (kAge - 4)/16)
  kExpExpected = 2 ^ kExpected
  kError = abs(kL-kExpected) + abs(kAL-kExpected)
  kError += abs(kE-kExpExpected) + abs(kAE-kExpExpected)
  kError += abs(kN+kExpExpected) + abs(kAN+kExpExpected)
  if !(kError <= .00001) then
    printks "triggered envelope restart/timing failed at sample %d: %g\n", 0, kCycle, kError
    exitnowk(-1)
  endif
  if kCycle == 32 then
    gkChecks += 1
  endif
endin

instr 2
  setksmps 1
  kCycle init 0
  kCycle += 1
  kTrigger = kCycle == 1 ? 1 : 0
  kL triglinseg kTrigger, 0, 0, 1, 4/sr, 2, 0, 3, 4/sr, 4, 0, 5
  kE trigexpseg kTrigger, 1, 0, 2, 4/sr, 4, 0, 8, 4/sr, 16, 0, 32
  kZeroL triglinseg kTrigger, 1, 0, 2, 0, 3
  kZeroE trigexpseg kTrigger, 1, 0, 2, 0, 3
  kExpected = kCycle <= 4 ? 1 + kCycle/4 : (kCycle <= 8 ? 3 + (kCycle-4)/4 : 5)
  kError = abs(kL-kExpected) + abs(kE-2^kExpected)
  kError += abs(kZeroL-3) + abs(kZeroE-3)
  if !(kError <= .00001) then
    printks "triggered envelope zero stage failed at sample %d: %g\n", 0, kCycle, kError
    exitnowk(-1)
  endif
  if kCycle == 16 then
    gkChecks += 1
  endif
endin

opcode Reference, aa, 0
  setksmps 1
  kSample init 0
  kSample += 1
  kValue = min(kSample/32, 1)
  aL = kValue
  aE = 2 ^ kValue
  xout aL, aE
endop

instr 3
  kFirst init 1
  aL triglinseg kFirst, 0, 32/sr, 1
  aE trigexpseg kFirst, 1, 32/sr, 2
  kL triglinseg kFirst, 0, 32/sr, 1
  kE trigexpseg kFirst, 1, 32/sr, 2
  aRefL, aRefE Reference
  aError = abs(aL-aRefL) + abs(aE-aRefE)
  kError max_k aError, 1, 1
  if p4 == 0 then
    kEnd init 0
    kEnd += ksmps
    kExpected = min(kEnd/32, 1)
    kError += abs(kL-kExpected) + abs(kE-2^kExpected)
  endif
  if !(kError <= .00001) then
    printks "triggered envelope block timing failed at start %g: %g\n", 0, p2, kError
    exitnowk(-1)
  endif
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 5 then
    prints "triggered envelope checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .004
i 2 .01 .002
i 3 .02 .008 0
i 3 .030625 .006 1
i 3 .040625 .001 1
i 99 .05 .002
</CsScore>
</CsoundSynthesizer>
