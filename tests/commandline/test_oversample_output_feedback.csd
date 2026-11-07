<CsTest>
description = "an output of an oversampled UDO that is also its input moves once per caller k-period"

[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 64
nchnls = 1
0dbfs = 1

gkChecked init 0

; Classic, with xin before the rate, and modern.
opcode Grow, S, Sii
  SValue, iFactor, iMode xin
  oversample iFactor, iMode
  SGrown strcatk SValue, "x"
  xout SGrown
endop

opcode GrowModern(value:S, factor:i, mode:i):S
  oversample(factor, mode)
  grown:S = strcatk(value, "x")
  xout(grown)
endop

opcode GrowArray(values:S[], factor:i, mode:i):S[]
  oversample(factor, mode)
  grown:S[] init 2
  grown[0] = strcatk(values[0], "x")
  grown[1] = strcatk(values[1], "xx")
  xout(grown)
endop

; At an even factor, going round every local cycle flips it back.
opcode Flip(value:B, factor:i, mode:i):B
  oversample(factor, mode)
  flipped:B = !value
  xout(flipped)
endop

opcode Step(value:Complex, factor:i, mode:i):Complex
  oversample(factor, mode)
  stepped:Complex = value + complex(1, 0)
  xout(stepped)
endop

opcode StepArray(values:Complex[], factor:i, mode:i):Complex[]
  oversample(factor, mode)
  stepped:Complex[] init 2
  stepped[0] = values[0] + complex(1, 0)
  stepped[1] = values[1] + complex(2, 0)
  xout(stepped)
endop

; Empty at init, so a k[] with no converter, at any converter setting.
opcode IncrementEach, k[], k[]ii
  kValues[], iFactor, iMode xin
  oversample iFactor, iMode
  kResults[] = kValues + 1
  xout kResults
endop

; The local cycle count goes out: the caller must get the last of each group.
opcode LocalPeriod(factor:i, mode:i):S
  oversample(factor, mode)
  period:S = sprintfk("%d", timeinstk())
  xout(period)
endop

instr Check
  iFactor = p4
  iMode = p5
  kPeriod = timeinstk()

  ; strcatk and a Complex sum run at init too: one round before k-period 1.
  SClassic init ""
  SClassic Grow SClassic, iFactor, iMode
  kLength = strlenk(SClassic)
  if kLength != kPeriod + 1 then
    printks "factor %g, mode %g, k-period %g: S length %g\n", \
      0, iFactor, iMode, kPeriod, kLength
    exitnowk -1
  endif

  modern:S init ""
  modern = GrowModern(modern, iFactor, iMode)
  kLength = strlenk(modern)
  if kLength != kPeriod + 1 then
    printks "factor %g, mode %g, k-period %g: modern S length %g\n", \
      0, iFactor, iMode, kPeriod, kLength
    exitnowk -1
  endif

  SArray[] init 2
  SArray GrowArray SArray, iFactor, iMode
  kLength0 = strlenk(SArray[0])
  kLength1 = strlenk(SArray[1])
  if kLength0 != kPeriod + 1 || kLength1 != 2 * kPeriod + 2 then
    printks "factor %g, mode %g, k-period %g: S[] lengths %g, %g\n", \
      0, iFactor, iMode, kPeriod, kLength0, kLength1
    exitnowk -1
  endif

  ; Unlike the inits above, this does not clear a reused instance's value.
  flag:B init 0
  if kPeriod == 1 then
    flag = B(0)
  endif
  flag = Flip(flag, iFactor, iMode)
  kFlag = 0
  if flag then
    kFlag = 1
  endif
  if kFlag != kPeriod % 2 then
    printks "factor %g, mode %g, k-period %g: B %g\n", \
      0, iFactor, iMode, kPeriod, kFlag
    exitnowk -1
  endif

  steps:Complex init 0, 0
  steps = Step(steps, iFactor, iMode)
  kSteps = real(steps)
  if kSteps != kPeriod + 1 then
    printks "factor %g, mode %g, k-period %g: Complex %g\n", \
      0, iFactor, iMode, kPeriod, kSteps
    exitnowk -1
  endif

  stepsArray:Complex[] init 2
  stepsArray = StepArray(stepsArray, iFactor, iMode)
  kSteps0 = real(stepsArray[0])
  kSteps1 = real(stepsArray[1])
  if kSteps0 != kPeriod + 1 || kSteps1 != 2 * kPeriod + 2 then
    printks "factor %g, mode %g, k-period %g: Complex[] %g, %g\n", \
      0, iFactor, iMode, kPeriod, kSteps0, kSteps1
    exitnowk -1
  endif

  SLocal = LocalPeriod(iFactor, iMode)
  kLocal = strtodk(SLocal)
  if kLocal != kPeriod * iFactor then
    printks "factor %g, mode %g, k-period %g: returned local k-period %g\n", \
      0, iFactor, iMode, kPeriod, kLocal
    exitnowk -1
  endif

  gkChecked += 1
endin

instr CheckEmpty
  iFactor = p4
  iMode = p5
  kPeriod = timeinstk()
  kGrown[] init 0
  ; Only the UDO runs at init, where the array has nothing to read.
  igoto Grow
  if kPeriod == 1 then
    trim kGrown, 1
    kGrown[0] = 0
  endif
Grow:
  kGrown IncrementEach kGrown, iFactor, iMode
  igoto Read
  kFirst = kGrown[0]
Read:
  if kFirst != kPeriod then
    printks "factor %g, mode %g, k-period %g: empty k[] first element %g\n", \
      0, iFactor, iMode, kPeriod, kFirst
    exitnowk -1
  endif

  gkChecked += 1
endin

instr CheckResults
  ; Twenty notes of 75 k-periods.
  if i(gkChecked) != 1500 then
    prints "Expected 1500 k-periods checked, got %g\n", i(gkChecked)
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
; Every converter, sinc to linear, at two factors.
i "Check" 0 0.1 2 0
i "Check" + . 2 1
i "Check" + . 2 2
i "Check" + . 2 3
i "Check" + . 2 4
i "Check" + . 4 0
i "Check" + . 4 1
i "Check" + . 4 2
i "Check" + . 4 3
i "Check" + . 4 4
i "CheckEmpty" 1 0.1 2 0
i "CheckEmpty" + . 2 1
i "CheckEmpty" + . 2 2
i "CheckEmpty" + . 2 3
i "CheckEmpty" + . 2 4
i "CheckEmpty" + . 4 0
i "CheckEmpty" + . 4 1
i "CheckEmpty" + . 4 2
i "CheckEmpty" + . 4 3
i "CheckEmpty" + . 4 4
i "CheckResults" 2 0.1
e
</CsScore>
</CsoundSynthesizer>
