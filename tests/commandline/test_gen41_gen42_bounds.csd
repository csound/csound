<CsoundSynthesizer>
<CsOptions>
-n -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

gi41RoundAbove ftgen 1, 0, -8, -41, 10, 1, 20, 1, 30, 1
gi41RoundBelow ftgen 2, 0, -8, -41, \
  10, 1, 20, 1, 30, 1, 40, 1, 50, 1, 60, 1
gi42RoundAbove ftgen 3, 0, -8, -42, \
  10, 12, 1, 20, 21, 1, 30, 32, 1
gi42RoundBelow ftgen 4, 0, -8, -42, \
  10, 10, 1, 20, 21, 1, 30, 30, 1, \
  40, 40, 1, 50, 51, 1, 60, 60, 1

opcode AssertTableValue(ndx:i, fn:i, expected:i):void
  value:i = table:i(ndx, fn)
  if value != expected then
    prints("table %d index %d: expected %d, got %f\n", \
           fn, ndx, expected, value)
    exitnow(-1)
  endif
endop

instr 1
  AssertTableValue(0, gi41RoundAbove, 10)
  AssertTableValue(1, gi41RoundAbove, 10)
  AssertTableValue(2, gi41RoundAbove, 10)
  AssertTableValue(3, gi41RoundAbove, 20)
  AssertTableValue(4, gi41RoundAbove, 20)
  AssertTableValue(5, gi41RoundAbove, 30)
  AssertTableValue(6, gi41RoundAbove, 30)
  AssertTableValue(7, gi41RoundAbove, 30)

  AssertTableValue(0, gi41RoundBelow, 10)
  AssertTableValue(1, gi41RoundBelow, 20)
  AssertTableValue(2, gi41RoundBelow, 20)
  AssertTableValue(3, gi41RoundBelow, 30)
  AssertTableValue(4, gi41RoundBelow, 40)
  AssertTableValue(5, gi41RoundBelow, 50)
  AssertTableValue(6, gi41RoundBelow, 50)
  AssertTableValue(7, gi41RoundBelow, 60)

  AssertTableValue(0, gi42RoundAbove, 10)
  AssertTableValue(1, gi42RoundAbove, 11)
  AssertTableValue(2, gi42RoundAbove, 12)
  AssertTableValue(3, gi42RoundAbove, 20)
  AssertTableValue(4, gi42RoundAbove, 21)
  AssertTableValue(5, gi42RoundAbove, 30)
  AssertTableValue(6, gi42RoundAbove, 31)
  AssertTableValue(7, gi42RoundAbove, 32)

  AssertTableValue(0, gi42RoundBelow, 10)
  AssertTableValue(1, gi42RoundBelow, 20)
  AssertTableValue(2, gi42RoundBelow, 21)
  AssertTableValue(3, gi42RoundBelow, 30)
  AssertTableValue(4, gi42RoundBelow, 40)
  AssertTableValue(5, gi42RoundBelow, 50)
  AssertTableValue(6, gi42RoundBelow, 51)
  AssertTableValue(7, gi42RoundBelow, 60)

  iBuiltinZero duserrnd 0
  iBuiltinNegative duserrnd -1
  kBuiltinZeroTable init 0
  kBuiltinNegativeTable init -1
  kBuiltinZero duserrnd kBuiltinZeroTable
  aBuiltinNegative duserrnd kBuiltinNegativeTable

  kDiscrete duserrnd gi41RoundAbove
  kRange urd gi42RoundAbove
  printks "random values: %f %f\n", 1, kDiscrete, kRange
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
i 1 0.02 0.01
</CsScore>
</CsoundSynthesizer>
