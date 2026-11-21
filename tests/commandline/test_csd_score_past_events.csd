<CsTest>
description = "a CSD's score joins the --events= score sorted before it, and plays past its end"
args = ["-nd", "--events=i 1 0 0.1"]

[expect]
exit = 0
output = ["note at 0.000", "note at 0.050", "note at 0.500"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
  prints "note at %.3f\n", times:i()
endin

</CsInstruments>
<CsScore>
i 1 0.05 0
i 1 0.5 0
</CsScore>
</CsoundSynthesizer>
