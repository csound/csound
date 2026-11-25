<CsTest>
description = "phasorbnk retains float phase detail across bank changes and wraps"
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 65536
ksmps = 16
nchnls = 1
0dbfs = 1
gkChecks init 0

instr 1
  iStep = p4/67108864
  iStart = (p4 > 0 ? 1-1/16777216 : 0)
  aFrequency = iStep*sr
  if p5 == 0 then
    aPhase phasorbnk iStep*sr, 0, 2, iStart
  elseif p5 == 1 then
    aPhase phasorbnk aFrequency, 0, 2, iStart
  else
    aFrequency phasorbnk aFrequency, 0, 2, iStart
    aPhase = aFrequency
  endif
  kCount init 0
  kN = 0
  while kN < ksmps do
    kSample = kCount + kN
    ; Count steps near zero, avoiding an accumulated float reference phase.
    kExpected = (p4 > 0 ? kSample-4 : -kSample)/67108864
    if kExpected < 0 then
      kExpected += 1
    endif
    if kExpected == 1 then
      kExpected = 0
    endif
    kActual vaget kN, aPhase
    if kActual != kExpected then
      printks "FAIL phasorbnk slow phase direction=%g mode=%g sample=%g: %.12g expected %.12g\n", \
          0, p4, p5, kSample, kActual, kExpected
      exitnowk -1
    endif
    kN += 1
  od
  kCount += ksmps
  if kCount == 64 then
    gkChecks += 1
    turnoff
  endif
endin

instr 2
  ; Growth with and without reallocation must preserve both parts of old
  ; phases and clear new banks. A fresh phase must also clear old remainders.
  kCount init p4
  kInitial init 1-1/16777216
  kCycle init 0
  kUnits[] init 8
  iBank = 0
  while iBank < p4 do
    kUnits[iBank] init -4
    iBank += 1
  od
  if kCycle == 2 || kCycle == 6 then
    kNewCount = (kCycle == 2 ? p5 : p6)
    if p7 == 0 then
      kBank = kCount
      while kBank < kNewCount do
        kUnits[kBank] = 0
        kBank += 1
      od
      kInitial = -1
    else
      kBank = 0
      while kBank < kNewCount do
        kUnits[kBank] = -4
        kBank += 1
      od
      kInitial = 1-1/16777216
    endif
    kCount = kNewCount
    reinit RESTART
  endif
  kIndex = kCycle % kCount
RESTART:
  kPhase phasorbnk kr/67108864, kIndex, i(kCount), i(kInitial)
  rireturn
  kExpected = kUnits[kIndex]/67108864
  if kExpected < 0 then
    kExpected += 1
  endif
  if kExpected == 1 then
    kExpected = 0
  endif
  if kPhase != kExpected then
    printks "FAIL phasorbnk remainder after reinit cycle=%g bank=%g: %.12g expected %.12g\n", \
        0, kCycle, kIndex, kPhase, kExpected
    exitnowk -1
  endif
  kUnits[kIndex] += 1
  kCycle += 1
  if kCycle == 36 then
    gkChecks += 1
    turnoff
  endif
endin

instr 3
  ; The first tiny step must survive later quarter-cycle steps. Include
  ; whole-cycle frequencies too: they must leave the saved phase unchanged.
  setksmps 1
  kSample init 0
  kIncrement = (kSample == 0 ? 1/536870912 : .25)
  if kSample == 5 then
    kIncrement = p4*1099511627776
  elseif kSample == 6 then
    kIncrement = p4*2
  endif
  kFrequency = kIncrement*sr
  aFrequency = kFrequency
  kInPlace = kFrequency
  kInPlace phasorbnk kInPlace, 0, 2
  aPhaseK phasorbnk kFrequency, 0, 2
  aFrequency phasorbnk aFrequency, 0, 2
  kAudioK downsamp aPhaseK
  kAudioA downsamp aFrequency
  kSteps = max(0, kSample-1-min(2, max(0, kSample-5)))
  kExpected = (kSample == 0 ? 0 : (kSteps % 4)*.25 + 1/536870912)
  if kInPlace != kExpected || kAudioK != kExpected || kAudioA != kExpected then
    printks "FAIL phasorbnk changing frequency sample=%g: k %.12g ak %.12g aa %.12g expected %.12g\n", \
        0, kSample, kInPlace, kAudioK, kAudioA, kExpected
    exitnowk -1
  endif
  kSample += 1
  if kSample == 32 then
    gkChecks += 1
    turnoff
  endif
endin

instr 99
  if i(gkChecks) != 13 then
    prints "FAIL phasorbnk float checks did not complete\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .01 1 0
i 1 0 .01 1 1
i 1 0 .01 1 2
i 1 0 .01 -1 0
i 1 0 .01 -1 1
i 1 0 .01 -1 2
i 2 .02 .02 2 4 4 0
i 2 .02 .02 4 2 4 0
i 2 .02 .02 2 2 2 0
i 2 .02 .02 2 4 4 1
i 2 .02 .02 4 2 4 1
i 3 .05 .01 1
i 3 .05 .01 -1
i 99 .07 .001
e
</CsScore>
</CsoundSynthesizer>
