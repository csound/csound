<CsTest>
description = "i written as an opcode still takes a variable, a signed number, a struct member and an array with its index"

[expect]
exit = 0
output = ["took every value"]
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

struct Pair first:k, second:k

instr 1
  value:k init 5
  pair:Pair init 7, 8
  values:k[] fillarray 10, 20, 30
  index:k init 2
  at:i = 1

  variable:i i value
  negative:i i -1
  member:i i pair.second
  element:i i values, index
  computed:i i values, at + 1

  if variable == 5 && negative == -1 && member == 8 && element == 30 &&
     computed == 30 then
    prints "took every value\n"
  else
    prints "took %g %g %g %g %g\n", variable, negative, member, element, computed
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
