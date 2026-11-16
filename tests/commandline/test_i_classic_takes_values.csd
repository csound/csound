<CsTest>
description = "i still takes a variable, a signed number, a struct member, and an array with an index known at init"

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
  offsets:k[] fillarray 2, 1
  at:i = 1

  variable:i i value
  negative:i i -1
  member:i i pair.second
  element:i i values, index
  computed:i i values, at + 1
  read:i = i(values, offsets[0])
  chosen:i = i(values, at > 0)

  if variable == 5 && negative == -1 && member == 8 && element == 30 &&
     computed == 30 && read == 30 && chosen == 20 then
    prints "took every value\n"
  else
    prints "took %g %g %g %g %g %g %g\n", variable, negative, member, element,
           computed, read, chosen
    exitnow(-1)
  endif
endin
</CsInstruments>
<CsScore>
i 1 0 0.01
e
</CsScore>
</CsoundSynthesizer>
