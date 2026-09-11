<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --sample-accurate
</CsOptions>
<CsInstruments>
sr = 8192
ksmps = 32
nchnls = 1
0dbfs = 1
gkChecks init 0

opcode Check, 0, aa
  aActual, aExpected xin
  kSample = 0
  while kSample < ksmps do
    kActual vaget kSample, aActual
    kExpected vaget kSample, aExpected
    if !(abs(kActual - kExpected) < .00001) then
      printks "filter2 sample %g: %g expected %g\n", 0, kSample, kActual, kExpected
      exitnowk -1
    endif
    kSample += 1
  od
endop

; One-sample recurrence, independent of the filter's delay-line code.
opcode Reference, a, ai
  setksmps 1
  aInput, iFeedback xin
  kPrevious init 0
  kInput downsamp aInput
  kValue = .5 * kInput + iFeedback * kPrevious
  kPrevious = kValue
  aResult = kValue
  xout aResult
endop

instr 1
  iNumerator = (p4 == 0 ? 1 : p4)
  iDenominator = p5
  aInput oscils .5, 311, 0
  aExpected = .75 * aInput
  aPlain filter2 aInput, iNumerator, iDenominator, .75
  aWarped zfilter2 aInput, .5, .5, iNumerator, iDenominator, .75
  Check aPlain, aExpected
  Check aWarped, aExpected
  kInput = .25
  kActual filter2 kInput, iNumerator, iDenominator, .75
  if kActual != .75 * kInput then
    exitnowk -1
  endif
  kCycle timeinstk
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 2
  aInput oscils .5, 311, 0
  aExpected Reference aInput, .5
  aPlain filter2 aInput, 1, 1, .5, -.5
  ; Magnitude and phase controls must leave a real pole unchanged.
  aWarped zfilter2 aInput, .5, .5, 1, 1, .5, -.5
  Check aPlain, aExpected
  Check aWarped, aExpected
  kPrevious init 0
  kInput = .25
  kExpected = .5 * kInput + .5 * kPrevious
  kActual filter2 kInput, 1, 1, .5, -.5
  kPrevious = kExpected
  if abs(kActual - kExpected) > .00001 then
    exitnowk -1
  endif
  kCycle timeinstk
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 3
  aInput oscils .5, 311, 0
  aPrevious delay1 aInput
  aExpected = .5 * (aInput + aPrevious)
  aPlain filter2 aInput, 2, 0, .5, .5
  aWarped zfilter2 aInput, .5, .5, 2, 0, .5, .5
  Check aPlain, aExpected
  Check aWarped, aExpected
  kCycle timeinstk
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 4
  aInput oscils .5, 311, 0
  ; Initial poles are +/- .5i. Compare with the derived coefficients
  ; after changing their radius or angle.
  aWarped zfilter2 aInput, p4, p5, 1, 2, .5, 0, .25
  aExpected filter2 aInput, 1, 2, .5, p6, p7
  Check aWarped, aExpected
  kCycle timeinstk
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

#define Z10 #0, 0, 0, 0, 0, 0, 0, 0, 0, 0#
#define Z9 #0, 0, 0, 0, 0, 0, 0, 0, 0#
instr 5
  aInput oscils .5, 311, 0
  aDelayed delay aInput, 50 / sr
  aExpected = .5 * aInput + .25 * aDelayed
  ; Both orders at the supported maximum; all poles are at zero.
  aPlain filter2 aInput, 51, 50, .5, $Z10, $Z10, $Z10, $Z10, $Z9, .25, $Z10, $Z10, $Z10, $Z10, $Z10
  aWarped zfilter2 aInput, .5, .5, 51, 50, .5, $Z10, $Z10, $Z10, $Z10, $Z9, .25, $Z10, $Z10, $Z10, $Z10, $Z10
  Check aPlain, aExpected
  Check aWarped, aExpected
  kCycle timeinstk
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 6
  kOrder init p4
  kCycle init 0
  kCycle += 1
  aInput oscils .5, 311, 0
  if kCycle == 4 then
    kOrder = 1 - p4
    reinit FILTER
  endif
FILTER:
  iOrder = i(kOrder)
  aExpected Reference aInput, .5 * iOrder
  aPlain filter2 aInput, 1, iOrder, .5, -.5
  aWarped zfilter2 aInput, .5, .5, 1, iOrder, .5, -.5
  rireturn
  Check aPlain, aExpected
  Check aWarped, aExpected
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 7
  aInput oscils .5, 311, 0
  ; A negative real pole must not suppress phase changes to complex poles.
  aPhase zfilter2 aInput, 0, .5, 1, 3, .5, .5, .25, .125
  aPhaseExpected filter2 aInput, 1, 3, .5, .8826834323650897, .4413417161825449, .125
  Check aPhase, aPhaseExpected
  ; Skip a larger real pole when finding the largest complex-pole radius.
  aMagnitude zfilter2 aInput, .5, 0, 1, 3, .5, .9, .25, .225
  aMagnitudeExpected filter2 aInput, 1, 3, .5, .9, .5625, .50625
  Check aMagnitude, aMagnitudeExpected
  kCycle timeinstk
  if kCycle == 1 then
    gkChecks += 1
  endif
endin

instr 99
  if i(gkChecks) != 17 then
    prints "not all filter2 checks ran\n"
    exitnow -1
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .0625
; Fractional orders truncate toward zero, including a denominator above -1.
i 1 0 .0625 1.75 -.5
i 2 0 .0625
i 3 0 .0625
i 4 0 .0625 0 0 0 .25
i 4 0 .0625 .5 0 0 .5625
i 4 0 .0625 -.5 0 0 .0625
i 4 0 .0625 0 -.5 -.7071067811865475 .25
i 4 0 .0625 0 .5 .3826834323650897 .25
i 5 0 .0625
i 6 0 .0625 0
i 6 0 .0625 1
i 7 0 .0625
; Partial blocks and an instance reused after its first note ends.
i 1 .0006103515625 .0013427734375
i 2 .0006103515625 .0130615234375
i 3 .0006103515625 .0130615234375
i 2 .125 .0625
i 99 .2 .01
e
</CsScore>
</CsoundSynthesizer>
