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

; Independent recurrence for a unit step with explicit initial value zero.
opcode LagReference, a, ii
  setksmps 1
  iTime, iFirstSamples xin
  iTarget = exp(log(.001) / (iTime * sr))
  kSample init 0
  kSample += 1
  kCoefficient = iTarget
  if kSample <= iFirstSamples then
    kCoefficient = 1 + (iTarget - 1) * kSample / iFirstSamples
  endif
  kValue init 0
  kValue = 1 + kCoefficient * (kValue - 1)
  aValue = kValue
  xout aValue
endop

instr 1
  kCycle init 0
  kCycle += 1
  kTime = kCycle <= 2 ? .01 : (kCycle == 3 ? 0 : (kCycle % 2 == 0 ? .01 : .02))
  kInput = kCycle <= 4 ? 1 : -.5
  kLag lag kInput, kTime, 0
  kAlias sclag kInput, kTime, 0
  kLagUD lagud kInput, kTime, kTime, 0
  kDefault lag kInput, kTime
  kExpected init 0
  kExpectedDefault init 1
  kCoefficient = 0
  if kTime > 0 then
    kCoefficient = exp(log(.001) / (kTime * kr))
  endif
  kExpected = kInput + kCoefficient * (kExpected - kInput)
  kExpectedDefault = kInput + kCoefficient * (kExpectedDefault - kInput)
  if abs(kLag - kExpected) + abs(kAlias - kExpected) + \
     abs(kLagUD - kExpected) + abs(kDefault - kExpectedDefault) > .00001 then
    printks "lag state differs from recurrence on control cycle %d\n", 0, kCycle
    exitnowk(-1)
  endif
  if kCycle == 8 then
    gkChecks += 1
  endif
endin

instr 2, 3
  aInput = p4
  if p1 == 2 then
    ; Omitting the initial value must use the first active input sample.
    aLag lag aInput, .01
    aLagUD lagud aInput, .01, .01
    aReference = aInput
  else
    iOffset = round(p2 * sr) % ksmps
    iFirstSamples = min(ksmps - iOffset, round(p3 * sr))
    aLag lag aInput, .01, 0
    aLagUD lagud aInput, .01, .01, 0
    aReference LagReference .01, iFirstSamples
    aReference *= p4
  endif
  aError = abs(aLag - aReference) + abs(aLagUD - aReference)
  kError max_k aError, 1, 1
  if !(kError <= .00001) then
    printks "lag audio mismatch in instrument %d: %g\n", 0, p1, kError
    exitnowk(-1)
  endif
  kFirst init 1
  if kFirst == 1 then
    gkChecks += 1
    kFirst = 0
  endif
endin

instr 99
  if i(gkChecks) != 11 then
    prints "lag checks did not complete\n"
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 .016
; Whole blocks and several partial-block layouts, with and without initial values.
i 2 .02 .004 1
i 3 .02 .004 1
i 2 .030625 .003 1
i 3 .030625 .003 1
i 2 .04 .0035 1
i 3 .04 .0035 1
i 2 .050125 .0015 1
i 3 .050125 .0015 1
i 2 .060625 .003 -1
i 3 .060625 .003 -1
i 99 .07 .002
</CsScore>
</CsoundSynthesizer>
