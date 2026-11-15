<CsTest>
description = "The star of a /* that ends a chunk of a long line closes nothing"
args = ["-nd"]

[expect]
exit = 0
output = ["instr 1 ran", "instr 2 ran"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
  prints "instr 1 ran\n"
endin
; The next line's first 4095 bytes end with the /* that opens a comment.
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /*/ still in the comment
</CsInstruments>
<CsVersion>
Before 1.0
</CsVersion>
*/
instr 2
  prints "instr 2 ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0
i 2 0 0
</CsScore>
</CsoundSynthesizer>
