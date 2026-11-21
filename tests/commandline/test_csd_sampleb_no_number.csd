<CsTest>
description = "A <CsSampleB> tag without a file number is skipped, with a warning"
args = ["-nd"]

[expect]
exit = 0
output = ["<CsSampleB> without a file number, skipped", "instr 1 ran"]
output_excludes = ["unknown CSD tag"]
</CsTest>
<CsoundSynthesizer>
<CsInstruments>

instr 1
  prints "instr 1 ran\n"
endin

</CsInstruments>
<CsScore>
i 1 0 0
</CsScore>
<CsSampleB filename="sample">
aGVsbG8K
</CsSampleB>
<CsSampleB filename="another">
aGVsbG8K</CsSampleB>
</CsoundSynthesizer>
