<CsTest>
description = "New-style UDOs copy literal outputs without changing their inputs"
[expect]
exit = 0
stderr = ["Failed: 0", "ALL TESTS PASSED"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0 --run-unit-tests
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 32
nchnls = 1
0dbfs = 1

opcode LiteralPair(value:i):(i,i,i)
  value += 77
  xout 77, value, 77
endop
opcode LiteralControl():k
  xout 77
endop
opcode LiteralString():S
  xout "voice"
endop
opcode PfieldValue():i
  xout p4
endop
opcode UpperPfieldValue():i
  xout P4
endop
opcode Wide(flag:i):(i,i,i,i,i,i,i,i,i,i,i,i,i)
  if flag == 0 igoto done
  xout 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13
done:
endop

instr 1
  iSource = 1
  iFirst, iSecond, iThird = LiteralPair(iSource)
  assert(iFirst == 77)
  assert(iSecond == 78)
  assert(iThird == 77)
  assert(iSource == 78)
  assert(strcmp(LiteralString(), "voice") == 0)
  assert(PfieldValue() == p4)
  assert(UpperPfieldValue() == p4)
  kValue = LiteralControl()
  if kValue != 77 then
    exitnowk -1
  endif
  kValue += 5
endin
instr 2
  i1, i2, i3, i4, i5, i6, i7, i8, i9, i10, i11, i12, i13 = Wide(p4)
  if p4 != 0 then
    assert(i13 == 13)
  endif
endin
</CsInstruments>
<CsScore>
i1 0 0.01 29
i1 0.02 0.01 29
i2 0 0.01 0
i2 0.02 0.01 1
i2 0.04 0.01 0
</CsScore>
</CsoundSynthesizer>
