<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr=1024
ksmps=16
nchnls=1
0dbfs=1
gkNotes init 0

instr 1
  ; p4 trigger period, p5 input scale, p6 input bias, p7 local block size.
  setksmps p7
  iSize = p7
  iStart = int(p2*sr+.5)
  iEnd = int((p2+p3)*sr+.5)
  iBlock = iStart-iStart%iSize
  kCycle init 0
  kTrigger = (kCycle%p4 == p4-1 ? 1 : 0)
  aPhase phasor sr/16
  aInput = (aPhase+p6)*p5
  kAbs maxk aInput, kTrigger, 1
  kMax maxk aInput, kTrigger, 2
  kMin maxk aInput, kTrigger, 3
  kMean maxk aInput, kTrigger, 4
  kAliasAbs max_k aInput, kTrigger, 1
  kAliasMax max_k aInput, kTrigger, 2
  kAliasMin max_k aInput, kTrigger, 3
  kAliasMean max_k aInput, kTrigger, 4
  kSum init 0
  kCount init 0
  kHigh init 0
  kLow init 0
  kAbsolute init 0
  kExpectedAbs init 0
  kExpectedMax init 0
  kExpectedMin init 0
  kExpectedMean init 0
  kN = 0
  while kN < iSize do
    kSample = iBlock+kCycle*iSize+kN
    if kSample >= iStart && kSample < iEnd then
      kValue vaget kN, aInput
      if kCount == 0 then
        kHigh = kValue
        kLow = kValue
      endif
      kHigh = max(kHigh,kValue)
      kLow = min(kLow,kValue)
      kAbsolute = max(kAbsolute,abs(kValue))
      kSum += kValue
      kCount += 1
    endif
    kN += 1
  od
  if kTrigger != 0 then
    kExpectedAbs = kAbsolute
    kExpectedMax = kHigh
    kExpectedMin = kLow
    kExpectedMean = kSum/kCount
    kSum = 0
    kCount = 0
    kAbsolute = 0
  endif
  kTolerance = max(1,abs(p5))*.000001
  if abs(kAbs-kExpectedAbs) > kTolerance || abs(kMax-kExpectedMax) > kTolerance || abs(kMin-kExpectedMin) > kTolerance || abs(kMean-kExpectedMean) > kTolerance then
    printks "maxk block %g: %g %g %g %g, expected %g %g %g %g\n", 0, kCycle, kAbs, kMax, kMin, kMean, kExpectedAbs, kExpectedMax, kExpectedMin, kExpectedMean
    exitnowk -1
  endif
  if kAliasAbs != kAbs || kAliasMax != kMax || kAliasMin != kMin || kAliasMean != kMean then
    printks "max_k and maxk differ\n", 0
    exitnowk -1
  endif
  if kCycle == 0 then
    gkNotes += 1
  endif
  kCycle += 1
endin

instr 99
  if i(gkNotes) != 8 then
    prints "maxk cases did not all run\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Full and partial blocks, with immediate and delayed triggers.
i 1 0 .0625 1 1 -.5 16
i 1 .0703125 .03125 1 1 1 16
i 1 .1279296875 .0712890625 3 1 -.5 16
; Extrema exceed the old +/- 2147483647 starting values.
i 1 .25 .0625 1 4294967296 2 16
i 1 .3154296875 .0205078125 1 -4294967296 2 16
i 1 .3779296875 .0712890625 3 -4294967296 2 16
; One active sample and one-sample local blocks.
i 1 .5029296875 .0009765625 1 -1 2 16
i 1 .53125 .0078125 2 1 -.5 1
i 99 .625 0
e
</CsScore>
</CsoundSynthesizer>
