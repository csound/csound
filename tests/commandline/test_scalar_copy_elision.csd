<CsTest>
description = "Scalar copy removal alone preserves local values and execution rates"
args = ["-n", "-d", "-m0", "--run-unit-tests", "--optimize=none", "--optimize-copy-removal"]
[expect]
exit = 0
output = ["scalar copies passed"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
</CsOptions>
<CsInstruments>
sr = 48000
ksmps = 16
nchnls = 1
0dbfs = 1

instr 1
  ; Each calculation can write directly to its local i-rate destination.
  iValue = p4 + 1
  assert iValue == 3
  iValue = 20 - iValue
  assert iValue == 17
  iValue = iValue * 2
  assert iValue == 34

  ; The final assignment must remain k-rate. It resets kValue every block.
  kValue = p4 + 1
  kValue += 1
  assert kValue == 4

  ; Nested expressions must keep their evaluation order.
  result:i = (p4 + 1) * (p4 - 5)
  assert result == -9
endin

instr 2
  prints "scalar copies passed\n"
endin
</CsInstruments>
<CsScore>
i 1 0 0.01 2
i 2 0.02 0.001
e
</CsScore>
</CsoundSynthesizer>
