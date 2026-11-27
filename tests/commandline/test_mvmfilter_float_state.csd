<CsTest>
description = "mvmfilter preserves state, input reuse, and changing parameters"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  iOffset = int(p2*sr+.5) % ksmps
  kCount init 0
  kInput[] init 128
  kFrequency[] init 128
  kTau[] init 128
  kStart = (kCount == 0 ? iOffset : 0)
  kEnd = min(ksmps, kStart+128-kCount)
  aInput = 0
  aFrequency = 0
  aTau = 0
  kN = kStart
  while kN < kEnd do
    kSample = kCount+kN-kStart
    kFI = (p4 == 0 || p4 == 2 ? kCount : kSample)
    kTI = (p4 < 2 ? kCount : kSample)
    kInput[kSample] = (kSample == 0 ? 1 : (kSample % 7-3)/8)
    kFrequency[kSample] = (kFI % 17 == 0 ? .75*sr : (kFI % 5-2)*sr/4)
    kTau[kSample] = (kTI < 32 ? 1024 : (kTI < 64 ? .002 : .00001))
    if kTI == 35 || kTI == 64 then
      kTau[kSample] = -1
    endif
    vaset kInput[kSample], kN, aInput
    vaset kFrequency[kSample], kN, aFrequency
    vaset kTau[kSample], kN, aTau
    kN += 1
  od
  if p4 == 0 then
    aOut mvmfilter aInput, kFrequency[kCount], kTau[kCount]
  elseif p4 == 1 then
    aOut mvmfilter aInput, aFrequency, kTau[kCount]
  elseif p4 == 2 then
    aOut mvmfilter aInput, kFrequency[kCount], aTau
  else
    aOut mvmfilter aInput, aFrequency, aTau
  endif
  aInPlace = aInput
  aInPlace mvmfilter aInPlace, aFrequency, aTau
  aFreqPlace = aFrequency
  aFreqPlace mvmfilter aInput, aFreqPlace, aTau
  aTauPlace = aTau
  aTauPlace mvmfilter aInput, aFrequency, aTauPlace
  kN = 0
  while kN < ksmps do
    kExpected = 0
    if kN >= kStart && kN < kEnd then
      kSample = kCount+kN-kStart
      kExpected = kInput[kSample]
      kPhase = 0
      kDamping = 0
      kJ = kSample
      ; Direct impulse-response sum: no recursive rotation in the oracle.
      while kJ > 0 && kTau[kJ] > 0 do
        kPhase += min(kFrequency[kJ], sr/2)/sr
        kDamping -= 1/(sr*kTau[kJ])
        kJ -= 1
        kExpected += kInput[kJ]*exp(kDamping)*cos(2*$M_PI*kPhase)
      od
    endif
    kActual vaget kN, aOut
    kInPlace vaget kN, aInPlace
    kFreqPlace vaget kN, aFreqPlace
    kTauPlace vaget kN, aTauPlace
    if !(abs(kActual-kExpected) < .00002 && abs(kInPlace-kExpected) < .00002 && abs(kFreqPlace-kExpected) < .00002 && abs(kTauPlace-kExpected) < .00002) then
      printks "FAIL mvmfilter mode=%g sample=%g: %g / %g / %g / %g expected %g\n", \
          0, p4, kCount+kN-kStart, kActual, kInPlace, kFreqPlace, kTauPlace, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += kEnd-kStart
  if kCount == 128 then
    gkChecks += 1
  endif
endin

instr 2
  setksmps 1
  kSample init 0
  kKeep init 0
  aInput = (kSample == 0 || kSample == 20 ? 1 : 0)
  aReference mvmfilter aInput, 1000, 1024
  if kSample == 16 then
    kKeep = p4
    reinit FILTER
  endif
FILTER:
  aActual mvmfilter aInput, 1000, 1024, i(kKeep)
  rireturn
  kActual downsamp aActual
  kReference downsamp aReference
  if p4 == 0 && kSample >= 16 then
    kReference = (kSample < 20 ? 0 : exp(-(kSample-20)/(sr*1024))*cos(2*$M_PI*1000*(kSample-20)/sr))
  endif
  if !(abs(kActual-kReference) < .00002) then
    printks "FAIL mvmfilter reinit keep=%g sample=%g: %g expected %g\n", 0, p4, kSample, kActual, kReference
    exitnowk -1
  endif
  kSample += 1
  if kSample == 64 then
    gkChecks += 1
  endif
endin

instr 3
  setksmps 1
  aImpulse mpulse 1, 0
  aOut mvmfilter aImpulse, 0, 1/(25*sr)
  kValue downsamp aOut
  kSample init 0
  if kSample == 1 then
    if !(abs(kValue/exp(-25)-1) < .00001) then
      printks "FAIL mvmfilter lost a small nonzero decay pole: %.12g\n", 0, kValue
      exitnowk -1
    endif
    gkChecks += 1
  endif
  kSample += 1
endin

instr 99
  if i(gkChecks) != 11 then
    prints "FAIL mvmfilter state checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .015625 0
i 1 0 .015625 1
i 1 0 .015625 2
i 1 0 .015625 3
i 1 .0316162109375 .015625 0
i 1 .0316162109375 .015625 1
i 1 .0316162109375 .015625 2
i 1 .0316162109375 .015625 3
i 2 .0625 .0078125 0
i 2 .0625 .0078125 1
i 3 .078125 .00048828125
i 99 .09375 .001
e
</CsScore>
</CsoundSynthesizer>
