<CsTest>
description = "A /* inside an open comment opens nothing: /*/ closes it"
args = ["-nd"]

[expect]
exit = 0
output = ["instr 1 ran"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

/* a comment that the next line closes
/*/
instr 1
  prints "instr 1 ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
