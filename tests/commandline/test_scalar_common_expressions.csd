<CsTest>
description = "Scalar expression reuse keeps each input value and execution rate"
args = ["-n", "-d", "-m0", "--run-unit-tests", "--orc-opt-passes=9"]
[expect]
exit = 0
</CsTest>
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  ; Repeated initialization expressions have the same value.
  iInput = 2
  iFirst = (iInput + 3) * 4
  iSecond = (iInput + 3) * 4
  assert iFirst == 20
  assert iSecond == 20

  ; Reuse must follow the changing input on every control period.
  kInput init 0
  kInput += 1
  kFirst = (kInput + 2) * 3
  kSecond = (kInput + 2) * 3
  assert kFirst == (kInput + 2) * 3
  assert kSecond == kFirst

  ; Writing an operand ends reuse of the old calculation.
  kValue = 2
  kBefore = kValue - 1
  kValue = 5
  kAfter = kValue - 1
  kReversed = 1 - kValue
  assert kBefore == 1
  assert kAfter == 4
  assert kReversed == -4

  ; Each loop iteration needs its own current value.
  kSum = 0
  kIndex = 0
  while kIndex < 3 do
    kOne = kIndex + 1
    kTwo = kIndex + 1
    kSum += kOne + kTwo
    kIndex += 1
  od
  assert kSum == 12
endin
</CsInstruments>
<CsScore>
i 1 0 .02
e
</CsScore>
</CsoundSynthesizer>
