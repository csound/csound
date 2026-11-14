<CsTest>
description = "prints formats the values it is given with a string p-field as its format"

[expect]
exit = 0
output = ["value 42 and 7|", "plain|"]
</CsTest>
<CsoundSynthesizer>
<CsOptions>
-n -d -m0
</CsOptions>
<CsInstruments>
sr = 1024
ksmps = 32
nchnls = 1
0dbfs = 1

instr 1
  prints p4, 42, 7
  prints p5
endin

</CsInstruments>
<CsScore>
i 1 0 .1 "value %d and %d|\n" "plain|\n"
e
</CsScore>
</CsoundSynthesizer>
