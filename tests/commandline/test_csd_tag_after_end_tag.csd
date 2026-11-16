<CsTest>
description = "A tag at byte 4096 of the line that ends a section is text"
args = ["-nd"]

[expect]
exit = 0
output = ["instr 1 ran"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
  prints "instr 1 ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0
; The next line's 4096th byte starts a tag, which a chunk would start.
</CsScore>                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     <CsVersion>
Before 1.0
</CsVersion>
</CsoundSynthesizer>
