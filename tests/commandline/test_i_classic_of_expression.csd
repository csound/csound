<CsTest>
description = "i written as an opcode refuses an expression, as i() does"

[expect]
exit = "nonzero"
stderr = ["Use of i() with expression not permitted"]
output = ["Parsing failed due to 7 semantic errors"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 44100
ksmps = 32
nchnls = 1
0dbfs = 1

; each expression of k values has no value at init, so each line would give 0
gkvalue init 3
giwritten i gkvalue + 1

opcode Snapshot(value:k):i
  written:i i value + 1
  xout written
endop

instr 1
  value:k init 3
  index:k init 1
  values:k[] fillarray 1, 2
  sum:i i value + 1
  ksum i value + 1
  absolute:i i abs(value)
  negative:i i -value
  element:i i values[index]
  snap:i = Snapshot(value)
  print sum, absolute, negative, element, snap, giwritten
  printk2 ksum
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
