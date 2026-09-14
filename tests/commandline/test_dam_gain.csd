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
  kCycle init 0
  kCycle += 1
  aInput = kCycle < 80 ? p4 : (kCycle < 160 ? 0 : p4)
  aOutput dam aInput, .1, 1, 1, .01, .5
  aInPlace = aInput
  aInPlace dam aInPlace, .1, 1, 1, .01, .5
  aError = abs(aOutput-aInput) + abs(aInPlace-aInput)
  kError max_k aError, 1, 1
  if !(kError <= .000001) then
    printks "dam changed unity-ratio input: %g\n", 0, kError
    exitnowk(-1)
  endif
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 2
  setksmps 1
  kSample init 0
  kSample += 1
  aInput = .25
  aOutput dam aInput, 0, p4, 1, p5/sr, p6/sr
  kOutput downsamp aOutput
  if p4 > 1 then
    if p5 <= 0 then
      kExpected = p4
    else
      kExpected = min(1+kSample/p5, p4)
    endif
  else
    if p6 <= 0 then
      kExpected = p4
    else
      kExpected = max(1-kSample/p6, p4)
    endif
  endif
  if !(abs(kOutput-.25*kExpected) <= .000001) then
    printks "dam gain timing failed at sample %d: expected %g, got %g\n", 0, kSample, .25*kExpected, kOutput
    exitnowk(-1)
  endif
  if kSample == 40 then
    gkChecks += 1
  endif
endin

instr 3
  setksmps 1
  kSample init 0
  kSample += 1
  kInput = kSample <= 1200 ? 1 : (kSample <= 2300 ? 0 : .25)
  aInput = kInput
  aOutput dam aInput, .1, .5, .5, 0, 0
  kOutput downsamp aOutput
  kOnes = max(0, min(kSample,1200)-max(0,kSample-1000))
  kQuarters = min(max(kSample-2300,0),1000)
  kSeed = max(1000-kSample,0)
  kPower = (.1*kSeed + (kOnes+.25*kQuarters)/sqrt(2))/1000
  if kPower > .1 then
    kGain = .5 + .05/kPower
  else
    kGain = kPower/.1
  endif
  if !(abs(kOutput-kInput*kGain) <= .000002) then
    printks "dam moving level differs at sample %d: expected %g, got %g\n", 0, kSample, kInput*kGain, kOutput
    exitnowk(-1)
  endif
  if kSample == 3400 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 10 then
    prints "dam checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .4 .5
i 1 .410625 .004 -.05
i 1 .420625 .001 .5
i 2 .44 .005 1.3 16 32
i 2 .45 .005 2 32 32
i 2 .46 .005 .27 16 32
i 2 .47 .005 .25 16 16
i 2 .48 .005 2 0 0
i 2 .49 .005 .25 0 0
i 3 .5 .425
i 99 .94 .002
</CsScore>
</CsoundSynthesizer>
