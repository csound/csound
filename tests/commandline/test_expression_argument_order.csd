<CsTest>
description = "expression argument expansion preserves order, nested chains, and plain arguments"
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

#include "libassert.orc"

struct ArgItem value:i

giStep init 0
opcode ArgStep, i, 0
  giStep += 1
  xout giStep
endop

opcode ArgIdentity, i[], i[]
  iValues[] xin
  xout iValues
endop

opcode ArgWeighted, k, kkkk
  kOne, kTwo, kThree, kFour xin
  xout kOne + 10*kTwo + 100*kThree + 1000*kFour
endop

instr 1
  ; Mix unexpanded leaves and chains of different lengths. Side effects make
  ; missing, duplicated, or reordered argument evaluation observable.
  iValues[] = ArgIdentity(fillarray(100, ArgStep(), 200, ArgStep() + 10, abs(-ArgStep()), 400))
  assertEquals(lenarray(iValues), 6)
  assertEquals(iValues[0], 100)
  assertEquals(iValues[1], 1)
  assertEquals(iValues[2], 200)
  assertEquals(iValues[3], 12)
  assertEquals(iValues[4], 3)
  assertEquals(iValues[5], 400)
  assertEquals(giStep, 3)

  ; Both children already have opcode chains, while empty and plain-only
  ; argument lists must still work.
  iBoth = abs(-ArgStep()) + abs(-ArgStep())
  assertEquals(iBoth, 9)
  iPlain[] = ArgIdentity(fillarray(7, 11, 13))
  assertEquals(iPlain[2], 13)

  items:ArgItem[] init 2
  items[0].value = 17
  items[1].value = 23
  iMembers[] = ArgIdentity(fillarray(items[0].value, 19, items[1].value + 1))
  assertEquals(iMembers[0], 17)
  assertEquals(iMembers[1], 19)
  assertEquals(iMembers[2], 24)

  ; Repeat expansion in the performance chain with changing inputs.
  kCycle timeinstk
  kResult = abs(ArgWeighted(kCycle + 1, 50, abs(-kCycle), (kCycle + 2) * 3))
  kExpected = kCycle + 1 + 500 + 100*kCycle + 3000*(kCycle + 2)
  if kResult != kExpected then
    printks "expression result=%g expected=%g\n", 0, kResult, kExpected
    exitnowk(-1)
  endif
  if kCycle >= 3 then
    turnoff
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
</CsScore>
</CsoundSynthesizer>
