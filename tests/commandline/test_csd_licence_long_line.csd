<CsTest>
description = "The end tag of <CsLicence> is found after long blanks"
args = ["-nd"]

[expect]
exit = 0
output = ["End of Licence Information", "instr 1 ran"]
</CsTest>
<CsoundSynthesizer>
; The end tag's line below starts with 4090 blanks: a chunk of 4095 bytes cuts the tag.
<CsLicence>
Free to use.
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          </CsLicence>
<CsInstruments>

instr 1
  prints "instr 1 ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
</CsoundSynthesizer>
