<CsTest>
description = "a CSD's score plays beside --events=, which is sorted first"
args = ["-nd", "--events=i 1 0 0.1"]

[expect]
exit = 0
output = ["note at 0.000", "note at 0.010", "note at 0.020"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
  prints "note at %.3f\n", p2
endin

</CsInstruments>
<CsScore>
i 1 0.01 0
i 1 0.02 0
e
</CsScore>
</CsoundSynthesizer>
