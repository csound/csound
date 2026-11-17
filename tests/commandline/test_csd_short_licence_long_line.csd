<CsTest>
description = "The end tag of <CsShortLicence> is found after long blanks"
args = ["-nd"]

[expect]
exit = 0
output = ["instr 1 ran"]
</CsTest>
<CsoundSynthesizer>
; The end tag's line below starts with 4090 blanks: a chunk of 4095 bytes cuts the tag.
<CsShortLicence>
6
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          </CsShortLicence>
<CsInstruments>

instr 1
  prints "instr 1 ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
